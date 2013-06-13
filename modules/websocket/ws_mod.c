/*
 * $Id$
 *
 * Copyright (C) 2012 Crocodile RCS Ltd
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * Kamailio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * Kamailio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "../../dprint.h"
#include "../../events.h"
#include "../../ip_addr.h"
#include "../../locking.h"
#include "../../sr_module.h"
#include "../../tcp_conn.h"
#include "../../timer_proc.h"
#include "../../lib/kcore/kstats_wrapper.h"
#include "../../lib/kmi/mi.h"
#include "../../mem/mem.h"
#include "../../parser/msg_parser.h"
#include "ws_conn.h"
#include "ws_handshake.h"
#include "ws_frame.h"
#include "ws_mod.h"

MODULE_VERSION

/* Maximum number of connections to display when using the ws.dump MI command */
#define MAX_WS_CONNS_DUMP		50

static int mod_init(void);
static int child_init(int rank);
static void destroy(void);

sl_api_t ws_slb;
int *ws_enabled;

#define DEFAULT_KEEPALIVE_INTERVAL	1
static int ws_keepalive_interval = DEFAULT_KEEPALIVE_INTERVAL;

#define DEFAULT_KEEPALIVE_PROCESSES	1
static int ws_keepalive_processes = DEFAULT_KEEPALIVE_PROCESSES;

static cmd_export_t cmds[]= 
{
	/* ws_handshake.c */
	{ "ws_handle_handshake", (cmd_function) ws_handle_handshake,
	  0, 0, 0,
	  ANY_ROUTE },

	{ 0, 0, 0, 0, 0, 0 }
};

static param_export_t params[]=
{
	/* ws_frame.c */
	{ "keepalive_mechanism",	INT_PARAM, &ws_keepalive_mechanism },
	{ "keepalive_timeout",		INT_PARAM, &ws_keepalive_timeout },
	{ "ping_application_data",	STR_PARAM, &ws_ping_application_data.s},

	/* ws_handshake.c */
	{ "sub_protocols",		INT_PARAM, &ws_sub_protocols},

	/* ws_mod.c */
	{ "keepalive_interval",		INT_PARAM, &ws_keepalive_interval },
	{ "keepalive_processes",	INT_PARAM, &ws_keepalive_processes },

	{ 0, 0, 0 }
};

static stat_export_t stats[] =
{
	/* ws_conn.c */
	{ "ws_current_connections",       0, &ws_current_connections },
	{ "ws_max_concurrent_connections",0, &ws_max_concurrent_connections },

	/* ws_frame.c */
	{ "ws_failed_connections",        0, &ws_failed_connections },
	{ "ws_local_closed_connections",  0, &ws_local_closed_connections },
	{ "ws_received_frames",           0, &ws_received_frames },
	{ "ws_remote_closed_connections", 0, &ws_remote_closed_connections },
	{ "ws_transmitted_frames",        0, &ws_transmitted_frames },

	/* ws_handshake.c */
	{ "ws_failed_handshakes",         0, &ws_failed_handshakes },
	{ "ws_successful_handshakes",     0, &ws_successful_handshakes },

	{ 0, 0, 0 }
};

static mi_export_t mi_cmds[] =
{
	/* ws_conn.c */
	{ "ws.dump",	ws_mi_dump,    0, 0, 0 },

	/* ws_frame.c */
	{ "ws.close",   ws_mi_close,   0, 0, 0 },
	{ "ws.ping",    ws_mi_ping,    0, 0, 0 },
	{ "ws.pong",	ws_mi_pong,    0, 0, 0 },

	/* ws_handshake.c */
	{ "ws.disable", ws_mi_disable, 0, 0, 0 },
	{ "ws.enable",	ws_mi_enable,  0, 0, 0 },

	{ 0, 0, 0, 0, 0 }
};

struct module_exports exports= 
{
	"websocket",
	DEFAULT_DLFLAGS,	/* dlopen flags */
	cmds,			/* Exported functions */
	params,			/* Exported parameters */
	stats,			/* exported statistics */
	mi_cmds,		/* exported MI functions */
	0,			/* exported pseudo-variables */
	0,			/* extra processes */
	mod_init,		/* module initialization function */
	0,			/* response function */
	destroy,		/* destroy function */
	child_init		/* per-child initialization function */
};

static int mod_init(void)
{
	if (sl_load_api(&ws_slb) != 0)
	{
		LM_ERR("binding to SL\n");
		goto error;
	}

	if (sr_event_register_cb(SREV_TCP_WS_FRAME_IN, ws_frame_receive) != 0)
	{
		LM_ERR("registering WebSocket receive call-back\n");
		goto error;
	}

	if (sr_event_register_cb(SREV_TCP_WS_FRAME_OUT, ws_frame_transmit) != 0)
	{
		LM_ERR("registering WebSocket transmit call-back\n");
		goto error;
	}

	if (register_module_stats(exports.name, stats) != 0)
	{
		LM_ERR("registering core statistics\n");
		goto error;
	}

	if (register_mi_mod(exports.name, mi_cmds) != 0)
	{
		LM_ERR("registering MI commands\n");
		goto error;
	}

	if (wsconn_init() < 0)
	{
		LM_ERR("initialising WebSocket connections table\n");
		goto error;
	}

	if ((ws_enabled = (int *) shm_malloc(sizeof(int))) == NULL)
	{
		LM_ERR("allocating shared memory\n");
		goto error;
	}
	*ws_enabled = 1;

	
	if (ws_ping_application_data.s != 0)
		ws_ping_application_data.len =
					strlen(ws_ping_application_data.s);
	if (ws_ping_application_data.len < 1
		|| ws_ping_application_data.len > 125)
	{
		ws_ping_application_data.s = DEFAULT_PING_APPLICATION_DATA + 8;
		ws_ping_application_data.len =
					DEFAULT_PING_APPLICATION_DATA_LEN - 8;
	}

	if (ws_keepalive_mechanism != KEEPALIVE_MECHANISM_NONE)
	{
		if (ws_keepalive_timeout < 1 || ws_keepalive_timeout > 3600)
			ws_keepalive_timeout = DEFAULT_KEEPALIVE_TIMEOUT;

		switch(ws_keepalive_mechanism)
		{
		case KEEPALIVE_MECHANISM_PING:
		case KEEPALIVE_MECHANISM_PONG:
			break;
		default:
			ws_keepalive_mechanism = DEFAULT_KEEPALIVE_MECHANISM;
			break;
		}

		if (ws_keepalive_interval < 1 || ws_keepalive_interval > 60)
			ws_keepalive_interval = DEFAULT_KEEPALIVE_INTERVAL;

		if (ws_keepalive_processes < 1 || ws_keepalive_processes > 16)
			ws_keepalive_processes = DEFAULT_KEEPALIVE_PROCESSES;

		/* Add extra process/timer for the keepalive process */
		register_sync_timers(ws_keepalive_processes);
	}

	if (ws_sub_protocols & SUB_PROTOCOL_MSRP
		&& !sr_event_enabled(SREV_TCP_MSRP_FRAME))
		ws_sub_protocols &= ~SUB_PROTOCOL_MSRP;

	if ((ws_sub_protocols & SUB_PROTOCOL_ALL) == 0)
	{
		LM_ERR("no sub-protocols enabled\n");
		goto error;
	}

	if ((ws_sub_protocols | SUB_PROTOCOL_ALL) != SUB_PROTOCOL_ALL)
	{
		LM_ERR("unrecognised sub-protocols enabled\n");
		goto error;
	}

	return 0;

error:
	wsconn_destroy();
	shm_free(ws_enabled);

	return -1;
}

static int child_init(int rank)
{
	int i;

	if (rank == PROC_INIT || rank == PROC_TCP_MAIN)
		return 0;

	if (rank == PROC_MAIN
		&& ws_keepalive_mechanism != KEEPALIVE_MECHANISM_NONE)
	{
		for (i = 0; i < ws_keepalive_processes; i++)
		{
			if (fork_sync_timer(PROC_TIMER, "WEBSOCKET KEEPALIVE",
						1, ws_keepalive, NULL,
						ws_keepalive_interval) < 0)
			{
				LM_ERR("starting keepalive process\n");
				return -1;
			}
		}

	}

	return 0;
}

static void destroy(void)
{
	wsconn_destroy();
	shm_free(ws_enabled);
}
