/* $Id$
 *
 * Copyright (C) 2003-2008 Sippy Software, Inc., http://www.sippysoft.com
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
 * History:
 * ---------
 * 2003-10-09	nat_uac_test introduced (jiri)
 *
 * 2003-11-06   nat_uac_test permitted from onreply_route (jiri)
 *
 * 2003-12-01   unforce_rtp_proxy introduced (sobomax)
 *
 * 2004-01-07	RTP proxy support updated to support new version of the
 *		RTP proxy (20040107).
 *
 *		force_rtp_proxy() now inserts a special flag
 *		into the SDP body to indicate that this session already
 *		proxied and ignores sessions with such flag.
 *
 *		Added run-time check for version of command protocol
 *		supported by the RTP proxy.
 *
 * 2004-01-16   Integrated slightly modified patch from Tristan Colgate,
 *		force_rtp_proxy function with IP as a parameter (janakj)
 *
 * 2004-01-28	nat_uac_test extended to allow testing SDP body (sobomax)
 *
 *		nat_uac_test extended to allow testing top Via (sobomax)
 *
 * 2004-02-21	force_rtp_proxy now accepts option argument, which
 *		consists of string of chars, each of them turns "on"
 *		some feature, currently supported ones are:
 *
 *		 `a' - flags that UA from which message is received
 *		       doesn't support symmetric RTP;
 *		 `l' - force "lookup", that is, only rewrite SDP when
 *		       corresponding session is already exists in the
 *		       RTP proxy. Only makes sense for SIP requests,
 *		       replies are always processed in "lookup" mode;
 *		 `i' - flags that message is received from UA in the
 *		       LAN. Only makes sense when RTP proxy is running
 *		       in the bridge mode.
 *
 *		force_rtp_proxy can now be invoked without any arguments,
 *		as previously, with one argument - in this case argument
 *		is treated as option string and with two arguments, in
 *		which case 1st argument is option string and the 2nd
 *		one is IP address which have to be inserted into
 *		SDP (IP address on which RTP proxy listens).
 *
 * 2004-03-12	Added support for IPv6 addresses in SDPs. Particularly,
 *		force_rtp_proxy now can work with IPv6-aware RTP proxy,
 *		replacing IPv4 address in SDP with IPv6 one and vice versa.
 *		This allows creating full-fledged IPv4<->IPv6 gateway.
 *		See 4to6.cfg file for example.
 *
 *		Two new options added into force_rtp_proxy:
 *
 *		 `f' - instructs nathelper to ignore marks inserted
 *		       by another nathelper in transit to indicate
 *		       that the session is already goes through another
 *		       proxy. Allows creating chain of proxies.
 *		 `r' - flags that IP address in SDP should be trusted.
 *		       Without this flag, nathelper ignores address in the
 *		       SDP and uses source address of the SIP message
 *		       as media address which is passed to the RTP proxy.
 *
 *		Protocol between nathelper and RTP proxy in bridge
 *		mode has been slightly changed. Now RTP proxy expects SER
 *		to provide 2 flags when creating or updating session
 *		to indicate direction of this session. Each of those
 *		flags can be either `e' or `i'. For example `ei' means
 *		that we received INVITE from UA on the "external" network
 *		network and will send it to the UA on "internal" one.
 *		Also possible `ie' (internal->external), `ii'
 *		(internal->internal) and `ee' (external->external). See
 *		example file alg.cfg for details.
 *
 * 2004-03-15	If the rtp proxy test failed (wrong version or not started)
 *		retry test from time to time, when some *rtpproxy* function
 *		is invoked. Minimum interval between retries can be
 *		configured via rtpproxy_disable_tout module parameter (default
 *		is 60 seconds). Setting it to -1 will disable periodic
 *		rechecks completely, setting it to 0 will force checks
 *		for each *rtpproxy* function call. (andrei)
 *
 * 2004-03-22	Fix assignment of rtpproxy_retr and rtpproxy_tout module
 *		parameters.
 *
 * 2004-03-22	Fix get_body position (should be called before get_callid)
 * 				(andrei)
 *
 * 2004-03-24	Fix newport for null ip address case (e.g onhold re-INVITE)
 * 				(andrei)
 *
 * 2004-09-30	added received port != via port test (andrei)
 *
 * 2004-10-10   force_socket option introduced (jiri)
 *
 * 2005-02-24	Added support for using more than one rtp proxy, in which
 *		case traffic will be distributed evenly among them. In addition,
 *		each such proxy can be assigned a weight, which will specify
 *		which share of the traffic should be placed to this particular
 *		proxy.
 *
 *		Introduce failover mechanism, so that if SER detects that one
 *		of many proxies is no longer available it temporarily decreases
 *		its weight to 0, so that no traffic will be assigned to it.
 *		Such "disabled" proxies are periodically checked to see if they
 *		are back to normal in which case respective weight is restored
 *		resulting in traffic being sent to that proxy again.
 *
 *		Those features can be enabled by specifying more than one "URI"
 *		in the rtpproxy_sock parameter, optionally followed by the weight,
 *		which if absent is assumed to be 1, for example:
 *
 *		rtpproxy_sock="unix:/foo/bar=4 udp:1.2.3.4:3456=3 udp:5.6.7.8:5432=1"
 *
 * 2005-02-25	Force for pinging the socket returned by USRLOC (bogdan)
 *
 * 2005-03-22	support for multiple media streams added (netch)
 *
 * 2005-07-11  SIP ping support added (bogdan)
 *
 * 2005-07-14  SDP origin (o=) IP may be also changed (bogdan)
 *
 * 2006-03-08  fix_nated_sdp() may take one more param to force a specific IP;
 *             force_rtp_proxy() accepts a new flag 's' to swap creation/
 *              confirmation between requests/replies; 
 *             add_rcv_param() may take as parameter a flag telling if the
 *              parameter should go to the contact URI or contact header;
 *             (bogdan)
 * 2006-03-28 Support for changing session-level SDP connection (c=) IP when
 *            media-description also includes connection information (bayan)
 * 2007-04-13 Support multiple sets of rtpproxies and set selection added
 *            (ancuta)
 * 2007-04-26 Added some MI commands:
 *             nh_enable_ping used to enable or disable natping
 *             nh_enable_rtpp used to enable or disable a specific rtp proxy
 *             nh_show_rtpp   used to display information for all rtp proxies 
 *             (ancuta)
 * 2007-05-09 New function start_recording() allowing to start recording RTP 
 *             session in the RTP proxy (Carsten Bock - ported from SER)
 * 2007-09-11 Separate timer process and support for multiple timer processes
 *             (bogdan)
 * 2008-12-12 Support for RTCP attribute in the SDP
 *              (Min Wang/BASIS AudioNet - ported from SER)
 * 2010-08-05 Core SDP parser integrated into nathelper (osas)
 * 2010-10-08 Removal of deprecated force_rtp_proxy and swap flag (osas)
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#ifndef __USE_BSD
#define  __USE_BSD
#endif
#include <netinet/ip.h>
#ifndef __FAVOR_BSD
#define __FAVOR_BSD
#endif
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../flags.h"
#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../data_lump.h"
#include "../../data_lump_rpl.h"
#include "../../error.h"
#include "../../forward.h"
#include "../../mem/mem.h"
#include "../../parser/parse_from.h"
#include "../../parser/parse_to.h"
#include "../../parser/parse_uri.h"
#include "../../parser/parser_f.h"
#include "../../parser/sdp/sdp.h"
#include "../../resolve.h"
#include "../../timer.h"
#include "../../trim.h"
#include "../../ut.h"
#include "../../pt.h"
#include "../../timer_proc.h"
#include "../../lib/kmi/mi.h"
#include "../../pvar.h"
#include "../../lvalue.h"
#include "../../msg_translator.h"
#include "../../usr_avp.h"
#include "../../socket_info.h"
#include "../../mod_fix.h"
#include "../../dset.h"
#include "../../route.h"
#include "../../modules/tm/tm_load.h"
#include "rtpproxy.h"
#include "rtpproxy_funcs.h"
#include "bencode.h"
 
MODULE_VERSION

#if !defined(AF_LOCAL)
#define	AF_LOCAL AF_UNIX
#endif
#if !defined(PF_LOCAL)
#define	PF_LOCAL PF_UNIX
#endif

/* NAT UAC test constants */
#define	NAT_UAC_TEST_C_1918	0x01
#define	NAT_UAC_TEST_RCVD	0x02
#define	NAT_UAC_TEST_V_1918	0x04
#define	NAT_UAC_TEST_S_1918	0x08
#define	NAT_UAC_TEST_RPORT	0x10


#define DEFAULT_RTPP_SET_ID		0

#define MI_SET_NATPING_STATE		"nh_enable_ping"
#define MI_DEFAULT_NATPING_STATE	1

#define MI_ENABLE_RTP_PROXY			"nh_enable_rtpp"
#define MI_MIN_RECHECK_TICKS		0
#define MI_MAX_RECHECK_TICKS		(unsigned int)-1

#define MI_SHOW_RTP_PROXIES			"nh_show_rtpp"

#define MI_RTP_PROXY_NOT_FOUND		"RTP proxy not found"
#define MI_RTP_PROXY_NOT_FOUND_LEN	(sizeof(MI_RTP_PROXY_NOT_FOUND)-1)
#define MI_PING_DISABLED			"NATping disabled from script"
#define MI_PING_DISABLED_LEN		(sizeof(MI_PING_DISABLED)-1)
#define MI_SET						"set"
#define MI_SET_LEN					(sizeof(MI_SET)-1)
#define MI_INDEX					"index"
#define MI_INDEX_LEN				(sizeof(MI_INDEX)-1)
#define MI_DISABLED					"disabled"
#define MI_DISABLED_LEN				(sizeof(MI_DISABLED)-1)
#define MI_WEIGHT					"weight"
#define MI_WEIGHT_LEN				(sizeof(MI_WEIGHT)-1)
#define MI_RECHECK_TICKS			"recheck_ticks"
#define MI_RECHECK_T_LEN			(sizeof(MI_RECHECK_TICKS)-1)



/* Supported version of the RTP proxy command protocol */
#define	SUP_CPROTOVER	20040107
/* Required additional version of the RTP proxy command protocol */
#define	REQ_CPROTOVER	"20050322"
/* Additional version necessary for re-packetization support */
#define	REP_CPROTOVER	"20071116"
#define	PTL_CPROTOVER	"20081102"

#define	CPORT		"22222"

struct rtpproxy_flags {
	bencode_item_t *dictionary,
	               *flags,
		       *direction,
		       *replace;
	int flookup, to;
	str callid;
};

static char *gencookie();
static int rtpp_test(struct rtpp_node*, int, int);
static int unforce_rtp_proxy_f(struct sip_msg *, char *, char *);
static int force_rtp_proxy(struct sip_msg *, char *, char *, int, int);
static int start_recording_f(struct sip_msg *, char *, char *);
static int rtpproxy_answer1_f(struct sip_msg *, char *, char *);
static int rtpproxy_answer2_f(struct sip_msg *, char *, char *);
static int rtpproxy_offer1_f(struct sip_msg *, char *, char *);
static int rtpproxy_offer2_f(struct sip_msg *, char *, char *);
static int rtpproxy_manage0(struct sip_msg *msg, char *flags, char *ip);
static int rtpproxy_manage1(struct sip_msg *msg, char *flags, char *ip);
static int rtpproxy_manage2(struct sip_msg *msg, char *flags, char *ip);

static int add_rtpproxy_socks(struct rtpp_set * rtpp_list, char * rtpproxy);
static int fixup_set_id(void ** param, int param_no);
static int set_rtp_proxy_set_f(struct sip_msg * msg, char * str1, char * str2);
static struct rtpp_set * select_rtpp_set(int id_set);

static int rtpproxy_set_store(modparam_t type, void * val);
static int rtpproxy_add_rtpproxy_set( char * rtp_proxies);

static int mod_init(void);
static int child_init(int);
static void mod_destroy(void);

/* Pseudo-Variables */
static int pv_get_rtpstat_f(struct sip_msg *, pv_param_t *, pv_value_t *);

/*mi commands*/
static struct mi_root* mi_enable_rtp_proxy(struct mi_root* cmd_tree,
		void* param );
static struct mi_root* mi_show_rtpproxies(struct mi_root* cmd_tree,
		void* param);


static int rtpproxy_disable_tout = 60;
static int rtpproxy_retr = 5;
static int rtpproxy_tout = 1;
static pid_t mypid;
static unsigned int myseqn = 0;
static str extra_id_pv_param = {NULL, 0};

static char ** rtpp_strings=0;
static int rtpp_sets=0; /*used in rtpproxy_set_store()*/
static int rtpp_set_count = 0;
static unsigned int current_msg_id = (unsigned int)-1;
/* RTP proxy balancing list */
struct rtpp_set_head * rtpp_set_list =0;
struct rtpp_set * selected_rtpp_set =0;
struct rtpp_set * default_rtpp_set=0;
static char *ice_candidate_priority_avp_param = NULL;
static int ice_candidate_priority_avp_type;
static int_str ice_candidate_priority_avp;

/* array with the sockets used by rtpporxy (per process)*/
static unsigned int rtpp_no = 0;
static int *rtpp_socks = 0;


typedef struct rtpp_set_link {
	struct rtpp_set *rset;
	pv_spec_t *rpv;
} rtpp_set_link_t;

/* tm */
static struct tm_binds tmb;

/*0-> disabled, 1 ->enabled*/
unsigned int *natping_state=0;

static str timeout_socket_str = {0, 0};
static pv_elem_t *extra_id_pv = NULL;

static cmd_export_t cmds[] = {
	{"set_rtp_proxy_set",  (cmd_function)set_rtp_proxy_set_f,    1,
		fixup_set_id, 0,
		ANY_ROUTE},
	{"unforce_rtp_proxy",  (cmd_function)unforce_rtp_proxy_f,    0,
		0, 0,
		ANY_ROUTE},
	{"rtpproxy_destroy",   (cmd_function)unforce_rtp_proxy_f,    0,
		0, 0,
		ANY_ROUTE},
	{"unforce_rtp_proxy",  (cmd_function)unforce_rtp_proxy_f,    1,
		0, 0,
		ANY_ROUTE},
	{"rtpproxy_destroy",   (cmd_function)unforce_rtp_proxy_f,    1,
		0, 0,
		ANY_ROUTE},
	{"start_recording",    (cmd_function)start_recording_f,      0,
		0, 0,
		ANY_ROUTE },
	{"rtpproxy_offer",	(cmd_function)rtpproxy_offer1_f,     0,
		0, 0,
		ANY_ROUTE},
	{"rtpproxy_offer",	(cmd_function)rtpproxy_offer1_f,     1,
		0, 0,
		ANY_ROUTE},
	{"rtpproxy_offer",	(cmd_function)rtpproxy_offer2_f,     2,
		0, 0,
		ANY_ROUTE},
	{"rtpproxy_answer",	(cmd_function)rtpproxy_answer1_f,    0,
		0, 0,
		ANY_ROUTE},
	{"rtpproxy_answer",	(cmd_function)rtpproxy_answer1_f,    1,
		0, 0,
		ANY_ROUTE},
	{"rtpproxy_answer",	(cmd_function)rtpproxy_answer2_f,    2,
		0, 0,
		ANY_ROUTE},
#if 0
	{"rtpproxy_stream2uac",(cmd_function)rtpproxy_stream2uac2_f, 2,
		fixup_var_str_int, 0,
		ANY_ROUTE },
	{"rtpproxy_stream2uas",(cmd_function)rtpproxy_stream2uas2_f, 2,
		fixup_var_str_int, 0,
		ANY_ROUTE },
	{"rtpproxy_stop_stream2uac",(cmd_function)rtpproxy_stop_stream2uac2_f,0,
		NULL, 0,
		ANY_ROUTE },
	{"rtpproxy_stop_stream2uas",(cmd_function)rtpproxy_stop_stream2uas2_f,0,
		NULL, 0,
		ANY_ROUTE },
#endif
	{"rtpproxy_manage",	(cmd_function)rtpproxy_manage0,     0,
		0, 0,
		ANY_ROUTE},
	{"rtpproxy_manage",	(cmd_function)rtpproxy_manage1,     1,
		fixup_spve_null, fixup_free_spve_null,
		ANY_ROUTE},
	{"rtpproxy_manage",	(cmd_function)rtpproxy_manage2,     2,
		fixup_spve_str, fixup_free_spve_str,
		ANY_ROUTE},
	{0, 0, 0, 0, 0, 0}
};

static pv_export_t mod_pvs[] = {
    {{"rtpstat", (sizeof("rtpstat")-1)}, /* RTP-Statistics */
     PVT_OTHER, pv_get_rtpstat_f, 0, 0, 0, 0, 0},
    {{0, 0}, 0, 0, 0, 0, 0, 0, 0}
};

static param_export_t params[] = {
	{"rtpproxy_sock",         STR_PARAM|USE_FUNC_PARAM,
	                         (void*)rtpproxy_set_store          },
	{"rtpproxy_disable_tout", INT_PARAM, &rtpproxy_disable_tout },
	{"rtpproxy_retr",         INT_PARAM, &rtpproxy_retr         },
	{"rtpproxy_tout",         INT_PARAM, &rtpproxy_tout         },
	{"timeout_socket",    	  STR_PARAM, &timeout_socket_str.s  },
	{"ice_candidate_priority_avp", STR_PARAM,
	 &ice_candidate_priority_avp_param},
	{"extra_id_pv",           STR_PARAM, &extra_id_pv_param.s },
	{0, 0, 0}
};

static mi_export_t mi_cmds[] = {
	{MI_ENABLE_RTP_PROXY,     mi_enable_rtp_proxy,  0,                0, 0},
	{MI_SHOW_RTP_PROXIES,     mi_show_rtpproxies,   MI_NO_INPUT_FLAG, 0, 0},
	{ 0, 0, 0, 0, 0}
};


struct module_exports exports = {
	"rtpproxy-ng",
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,
	params,
	0,           /* exported statistics */
	mi_cmds,     /* exported MI functions */
	mod_pvs,     /* exported pseudo-variables */
	0,           /* extra processes */
	mod_init,
	0,           /* reply processing */
	mod_destroy, /* destroy function */
	child_init
};


static int rtpproxy_set_store(modparam_t type, void * val){

	char * p;
	int len;

	p = (char* )val;

	if(p==0 || *p=='\0'){
		return 0;
	}

	if(rtpp_sets==0){
		rtpp_strings = (char**)pkg_malloc(sizeof(char*));
		if(!rtpp_strings){
			LM_ERR("no pkg memory left\n");
			return -1;
		}
	} else {/*realloc to make room for the current set*/
		rtpp_strings = (char**)pkg_realloc(rtpp_strings, 
										  (rtpp_sets+1)* sizeof(char*));
		if(!rtpp_strings){
			LM_ERR("no pkg memory left\n");
			return -1;
		}
	}
	
	/*allocate for the current set of urls*/
	len = strlen(p);
	rtpp_strings[rtpp_sets] = (char*)pkg_malloc((len+1)*sizeof(char));

	if(!rtpp_strings[rtpp_sets]){
		LM_ERR("no pkg memory left\n");
		return -1;
	}
	
	memcpy(rtpp_strings[rtpp_sets], p, len);
	rtpp_strings[rtpp_sets][len] = '\0';
	rtpp_sets++;

	return 0;
}


static int add_rtpproxy_socks(struct rtpp_set * rtpp_list, 
										char * rtpproxy){
	/* Make rtp proxies list. */
	char *p, *p1, *p2, *plim;
	struct rtpp_node *pnode;
	int weight;

	p = rtpproxy;
	plim = p + strlen(p);

	for(;;) {
			weight = 1;
		while (*p && isspace((int)*p))
			++p;
		if (p >= plim)
			break;
		p1 = p;
		while (*p && !isspace((int)*p))
			++p;
		if (p <= p1)
			break; /* may happen??? */
		/* Have weight specified? If yes, scan it */
		p2 = memchr(p1, '=', p - p1);
		if (p2 != NULL) {
			weight = strtoul(p2 + 1, NULL, 10);
		} else {
			p2 = p;
		}
		pnode = shm_malloc(sizeof(struct rtpp_node));
		if (pnode == NULL) {
			LM_ERR("no shm memory left\n");
			return -1;
		}
		memset(pnode, 0, sizeof(*pnode));
		pnode->idx = rtpp_no++;
		pnode->rn_recheck_ticks = 0;
		pnode->rn_weight = weight;
		pnode->rn_umode = 0;
		pnode->rn_disabled = 0;
		pnode->rn_url.s = shm_malloc(p2 - p1 + 1);
		if (pnode->rn_url.s == NULL) {
			shm_free(pnode);
			LM_ERR("no shm memory left\n");
			return -1;
		}
		memmove(pnode->rn_url.s, p1, p2 - p1);
		pnode->rn_url.s[p2 - p1] 	= 0;
		pnode->rn_url.len 			= p2-p1;

		LM_DBG("url is %s, len is %i\n", pnode->rn_url.s, pnode->rn_url.len);
		/* Leave only address in rn_address */
		pnode->rn_address = pnode->rn_url.s;
		if (strncasecmp(pnode->rn_address, "udp:", 4) == 0) {
			pnode->rn_umode = 1;
			pnode->rn_address += 4;
		} else if (strncasecmp(pnode->rn_address, "udp6:", 5) == 0) {
			pnode->rn_umode = 6;
			pnode->rn_address += 5;
		} else if (strncasecmp(pnode->rn_address, "unix:", 5) == 0) {
			pnode->rn_umode = 0;
			pnode->rn_address += 5;
		}

		if (rtpp_list->rn_first == NULL) {
			rtpp_list->rn_first = pnode;
		} else {
			rtpp_list->rn_last->rn_next = pnode;
		}

		rtpp_list->rn_last = pnode;
		rtpp_list->rtpp_node_count++;
	}
	return 0;
}


/*	0-succes
 *  -1 - erorr
 * */
static int rtpproxy_add_rtpproxy_set( char * rtp_proxies)
{
	char *p,*p2;
	struct rtpp_set * rtpp_list;
	unsigned int my_current_id;
	str id_set;
	int new_list;

	/* empty definition? */
	p= rtp_proxies;
	if(!p || *p=='\0'){
		return 0;
	}

	for(;*p && isspace(*p);p++);
	if(*p=='\0'){
		return 0;
	}

	rtp_proxies = strstr(p, "==");
	if(rtp_proxies){
		if(*(rtp_proxies +2)=='\0'){
			LM_ERR("script error -invalid rtp proxy list!\n");
			return -1;
		}
			
		*rtp_proxies = '\0';
		p2 = rtp_proxies-1;
		for(;isspace(*p2); *p2 = '\0',p2--);
		id_set.s = p;	id_set.len = p2 - p+1;
			
		if(id_set.len <= 0 ||str2int(&id_set, &my_current_id)<0 ){
		LM_ERR("script error -invalid set_id value!\n");
			return -1;
		}
			
		rtp_proxies+=2;
	}else{
		rtp_proxies = p;
		my_current_id = DEFAULT_RTPP_SET_ID;
	}

	for(;*rtp_proxies && isspace(*rtp_proxies);rtp_proxies++);

	if(!(*rtp_proxies)){
		LM_ERR("script error -empty rtp_proxy list\n");
		return -1;;
	}

	/*search for the current_id*/
	rtpp_list = rtpp_set_list ? rtpp_set_list->rset_first : 0;
	while( rtpp_list != 0 && rtpp_list->id_set!=my_current_id)
		rtpp_list = rtpp_list->rset_next;

	if(rtpp_list==NULL){	/*if a new id_set : add a new set of rtpp*/
		rtpp_list = shm_malloc(sizeof(struct rtpp_set));
		if(!rtpp_list){
			LM_ERR("no shm memory left\n");
			return -1;
		}
		memset(rtpp_list, 0, sizeof(struct rtpp_set));
		rtpp_list->id_set = my_current_id;
		new_list = 1;
	} else {
		new_list = 0;
	}

	if(add_rtpproxy_socks(rtpp_list, rtp_proxies)!= 0){
		/*if this list will not be inserted, clean it up*/
		goto error;
	}

	if (new_list) {
		if(!rtpp_set_list){/*initialize the list of set*/
			rtpp_set_list = shm_malloc(sizeof(struct rtpp_set_head));
			if(!rtpp_set_list){
				LM_ERR("no shm memory left\n");
				return -1;
			}
			memset(rtpp_set_list, 0, sizeof(struct rtpp_set_head));
		}

		/*update the list of set info*/
		if(!rtpp_set_list->rset_first){
			rtpp_set_list->rset_first = rtpp_list;
		}else{
			rtpp_set_list->rset_last->rset_next = rtpp_list;
		}

		rtpp_set_list->rset_last = rtpp_list;
		rtpp_set_count++;

		if(my_current_id == DEFAULT_RTPP_SET_ID){
			default_rtpp_set = rtpp_list;
		}
	}

	return 0;
error:
	return -1;
}


static int fixup_set_id(void ** param, int param_no)
{
	int int_val, err;
	struct rtpp_set* rtpp_list;
	rtpp_set_link_t *rtpl = NULL;
	str s;

	rtpl = (rtpp_set_link_t*)pkg_malloc(sizeof(rtpp_set_link_t));
	if(rtpl==NULL) {
		LM_ERR("no more pkg memory\n");
		return -1;
	}
	memset(rtpl, 0, sizeof(rtpp_set_link_t));
	s.s = (char*)*param;
	s.len = strlen(s.s);

	if(s.s[0] == PV_MARKER) {
		int_val = pv_locate_name(&s);
		if(int_val<0 || int_val!=s.len) {
			LM_ERR("invalid parameter %s\n", s.s);
			return -1;
		}
		rtpl->rpv = pv_cache_get(&s);
		if(rtpl->rpv == NULL) {
			LM_ERR("invalid pv parameter %s\n", s.s);
			return -1;
		}
	} else {
		int_val = str2s(*param, strlen(*param), &err);
		if (err == 0) {
			pkg_free(*param);
			if((rtpp_list = select_rtpp_set(int_val)) ==0){
				LM_ERR("rtpp_proxy set %i not configured\n", int_val);
				return E_CFG;
			}
			rtpl->rset = rtpp_list;
		} else {
			LM_ERR("bad number <%s>\n",	(char *)(*param));
			return E_CFG;
		}
	}
	*param = (void*)rtpl;
	return 0;
}

static struct mi_root* mi_enable_rtp_proxy(struct mi_root* cmd_tree, 
												void* param )
{	struct mi_node* node;
	str rtpp_url;
	unsigned int enable;
	struct rtpp_set * rtpp_list;
	struct rtpp_node * crt_rtpp;
	int found;

	found = 0;

	if(rtpp_set_list ==NULL)
		goto end;

	node = cmd_tree->node.kids;
	if(node == NULL)
		return init_mi_tree( 400, MI_MISSING_PARM_S, MI_MISSING_PARM_LEN);

	if(node->value.s == NULL || node->value.len ==0)
		return init_mi_tree( 400, MI_BAD_PARM_S, MI_BAD_PARM_LEN);

	rtpp_url = node->value;

	node = node->next;
	if(node == NULL)
		return init_mi_tree( 400, MI_MISSING_PARM_S, MI_MISSING_PARM_LEN);

	enable = 0;
	if( strno2int( &node->value, &enable) <0)
		goto error;

	for(rtpp_list = rtpp_set_list->rset_first; rtpp_list != NULL; 
					rtpp_list = rtpp_list->rset_next){

		for(crt_rtpp = rtpp_list->rn_first; crt_rtpp != NULL;
						crt_rtpp = crt_rtpp->rn_next){
			/*found a matching rtpp*/
			
			if(crt_rtpp->rn_url.len == rtpp_url.len){
				
				if(strncmp(crt_rtpp->rn_url.s, rtpp_url.s, rtpp_url.len) == 0){
					/*set the enabled/disabled status*/
					found = 1;
					crt_rtpp->rn_recheck_ticks = 
						enable? MI_MIN_RECHECK_TICKS : MI_MAX_RECHECK_TICKS;
					crt_rtpp->rn_disabled = enable?0:1;
				}
			}
		}
	}

end:
	if(found)
		return init_mi_tree( 200, MI_OK_S, MI_OK_LEN);
	return init_mi_tree(404,MI_RTP_PROXY_NOT_FOUND,MI_RTP_PROXY_NOT_FOUND_LEN);
error:
	return init_mi_tree( 400, MI_BAD_PARM_S, MI_BAD_PARM_LEN);
}



#define add_rtpp_node_int_info(_parent, _name, _name_len, _value, _child,\
								_len, _string, _error)\
	do {\
		(_string) = int2str((_value), &(_len));\
		if((_string) == 0){\
			LM_ERR("cannot convert int value\n");\
				goto _error;\
		}\
		if(((_child) = add_mi_node_child((_parent), MI_DUP_VALUE, (_name), \
				(_name_len), (_string), (_len))   ) == 0)\
			goto _error;\
	}while(0);

static struct mi_root* mi_show_rtpproxies(struct mi_root* cmd_tree, 
												void* param)
{
	struct mi_node* node, *crt_node, *child;
	struct mi_root* root;
	struct mi_attr * attr;
	struct rtpp_set * rtpp_list;
	struct rtpp_node * crt_rtpp;
	char * string, *id;
	int id_len, len;

	string = id = 0;

	root = init_mi_tree(200, MI_OK_S, MI_OK_LEN);
	if (!root) {
		LM_ERR("the MI tree cannot be initialized!\n");
		return 0;
	}

	if(rtpp_set_list ==NULL)
		return root;

	node = &root->node;

	for(rtpp_list = rtpp_set_list->rset_first; rtpp_list != NULL; 
					rtpp_list = rtpp_list->rset_next){

		for(crt_rtpp = rtpp_list->rn_first; crt_rtpp != NULL;
						crt_rtpp = crt_rtpp->rn_next){

			id =  int2str(rtpp_list->id_set, &id_len);
			if(!id){
				LM_ERR("cannot convert set id\n");
				goto error;
			}

			if(!(crt_node = add_mi_node_child(node, 0, crt_rtpp->rn_url.s, 
					crt_rtpp->rn_url.len, 0,0)) ) {
				LM_ERR("cannot add the child node to the tree\n");
				goto error;
			}

			LM_DBG("adding node name %s \n",crt_rtpp->rn_url.s );

			if((attr = add_mi_attr(crt_node, MI_DUP_VALUE, MI_SET, MI_SET_LEN, 
									id, id_len))== 0){
				LM_ERR("cannot add attributes to the node\n");
				goto error;
			}

			add_rtpp_node_int_info(crt_node, MI_INDEX, MI_INDEX_LEN,
				crt_rtpp->idx, child, len,string,error);
			add_rtpp_node_int_info(crt_node, MI_DISABLED, MI_DISABLED_LEN,
				crt_rtpp->rn_disabled, child, len,string,error);
			add_rtpp_node_int_info(crt_node, MI_WEIGHT, MI_WEIGHT_LEN,
				crt_rtpp->rn_weight,  child, len, string,error);
			add_rtpp_node_int_info(crt_node, MI_RECHECK_TICKS,MI_RECHECK_T_LEN,
				crt_rtpp->rn_recheck_ticks, child, len, string, error);
		}
	}

	return root;
error:
	if (root)
		free_mi_tree(root);
	return 0;
}


static int
mod_init(void)
{
	int i;
	pv_spec_t avp_spec;
	str s;
	unsigned short avp_flags;

	if(register_mi_mod(exports.name, mi_cmds)!=0)
	{
		LM_ERR("failed to register MI commands\n");
		return -1;
	}

	/* any rtpproxy configured? */
	if(rtpp_set_list)
		default_rtpp_set = select_rtpp_set(DEFAULT_RTPP_SET_ID);

	/* storing the list of rtp proxy sets in shared memory*/
	for(i=0;i<rtpp_sets;i++){
		if(rtpproxy_add_rtpproxy_set(rtpp_strings[i]) !=0){
			for(;i<rtpp_sets;i++)
				if(rtpp_strings[i])
					pkg_free(rtpp_strings[i]);
			pkg_free(rtpp_strings);
			return -1;
		}
		if(rtpp_strings[i])
			pkg_free(rtpp_strings[i]);
	}
	if (timeout_socket_str.s==NULL || timeout_socket_str.s[0]==0) {
		timeout_socket_str.len = 0;
		timeout_socket_str.s = NULL;
	} else {
		timeout_socket_str.len = strlen(timeout_socket_str.s);
	}

	if (ice_candidate_priority_avp_param) {
	    s.s = ice_candidate_priority_avp_param; s.len = strlen(s.s);
	    if (pv_parse_spec(&s, &avp_spec) == 0 || avp_spec.type != PVT_AVP) {
		LM_ERR("malformed or non AVP definition <%s>\n", ice_candidate_priority_avp_param);
		return -1;
	    }
	    if (pv_get_avp_name(0, &(avp_spec.pvp), &ice_candidate_priority_avp, &avp_flags) != 0) {
		LM_ERR("invalid AVP definition <%s>\n", ice_candidate_priority_avp_param);
		return -1;
	    }
	    ice_candidate_priority_avp_type = avp_flags;
	}

	if (extra_id_pv_param.s && *extra_id_pv_param.s) {
		extra_id_pv_param.len = strlen(extra_id_pv_param.s);
		if(pv_parse_format(&extra_id_pv_param, &extra_id_pv) < 0) {
			LM_ERR("malformed PV string: %s\n", extra_id_pv_param.s);
			return -1;
		}
	} else {
		extra_id_pv = NULL;
	}

	if (rtpp_strings)
		pkg_free(rtpp_strings);

	if (load_tm_api( &tmb ) < 0)
	{
		LM_DBG("could not load the TM-functions - answer-offer model"
				" auto-detection is disabled\n");
		memset(&tmb, 0, sizeof(struct tm_binds));
	}

	return 0;
}


static int
child_init(int rank)
{
	int n;
	char *cp;
	struct addrinfo hints, *res;
	struct rtpp_set  *rtpp_list;
	struct rtpp_node *pnode;

	if(rtpp_set_list==NULL )
		return 0;

	/* Iterate known RTP proxies - create sockets */
	mypid = getpid();

	rtpp_socks = (int*)pkg_malloc( sizeof(int)*rtpp_no );
	if (rtpp_socks==NULL) {
		LM_ERR("no more pkg memory\n");
		return -1;
	}

	for(rtpp_list = rtpp_set_list->rset_first; rtpp_list != 0;
		rtpp_list = rtpp_list->rset_next){

		for (pnode=rtpp_list->rn_first; pnode!=0; pnode = pnode->rn_next){
			char *hostname;

			if (pnode->rn_umode == 0) {
				rtpp_socks[pnode->idx] = -1;
				goto rptest;
			}

			/*
			 * This is UDP or UDP6. Detect host and port; lookup host;
			 * do connect() in order to specify peer address
			 */
			hostname = (char*)pkg_malloc(sizeof(char) * (strlen(pnode->rn_address) + 1));
			if (hostname==NULL) {
				LM_ERR("no more pkg memory\n");
				return -1;
			}
			strcpy(hostname, pnode->rn_address);

			cp = strrchr(hostname, ':');
			if (cp != NULL) {
				*cp = '\0';
				cp++;
			}
			if (cp == NULL || *cp == '\0')
				cp = CPORT;

			memset(&hints, 0, sizeof(hints));
			hints.ai_flags = 0;
			hints.ai_family = (pnode->rn_umode == 6) ? AF_INET6 : AF_INET;
			hints.ai_socktype = SOCK_DGRAM;
			if ((n = getaddrinfo(hostname, cp, &hints, &res)) != 0) {
				LM_ERR("%s\n", gai_strerror(n));
				pkg_free(hostname);
				return -1;
			}
			pkg_free(hostname);

			rtpp_socks[pnode->idx] = socket((pnode->rn_umode == 6)
			    ? AF_INET6 : AF_INET, SOCK_DGRAM, 0);
			if ( rtpp_socks[pnode->idx] == -1) {
				LM_ERR("can't create socket\n");
				freeaddrinfo(res);
				return -1;
			}

			if (connect( rtpp_socks[pnode->idx], res->ai_addr, res->ai_addrlen) == -1) {
				LM_ERR("can't connect to a RTP proxy\n");
				close( rtpp_socks[pnode->idx] );
				rtpp_socks[pnode->idx] = -1;
				freeaddrinfo(res);
				return -1;
			}
			freeaddrinfo(res);
rptest:
			pnode->rn_disabled = rtpp_test(pnode, 0, 1);
		}
	}

	return 0;
}


static void mod_destroy(void)
{
	struct rtpp_set * crt_list, * last_list;
	struct rtpp_node * crt_rtpp, *last_rtpp;

	/*free the shared memory*/
	if (natping_state)
		shm_free(natping_state);

	if(rtpp_set_list == NULL)
		return;

	for(crt_list = rtpp_set_list->rset_first; crt_list != NULL; ){

		for(crt_rtpp = crt_list->rn_first; crt_rtpp != NULL;  ){

			if(crt_rtpp->rn_url.s)
				shm_free(crt_rtpp->rn_url.s);

			last_rtpp = crt_rtpp;
			crt_rtpp = last_rtpp->rn_next;
			shm_free(last_rtpp);
		}

		last_list = crt_list;
		crt_list = last_list->rset_next;
		shm_free(last_list);
	}

	shm_free(rtpp_set_list);
}



#define	ADD_ADIRECTION	0x01
#define	FIX_MEDIP	0x02
#define	ADD_ANORTPPROXY	0x04
#define	FIX_ORGIP	0x08

#define	ADIRECTION	"a=direction:active"
#define	ADIRECTION_LEN	(sizeof(ADIRECTION) - 1)

#define	AOLDMEDIP	"a=oldmediaip:"
#define	AOLDMEDIP_LEN	(sizeof(AOLDMEDIP) - 1)

#define	AOLDMEDIP6	"a=oldmediaip6:"
#define	AOLDMEDIP6_LEN	(sizeof(AOLDMEDIP6) - 1)

#define	AOLDMEDPRT	"a=oldmediaport:"
#define	AOLDMEDPRT_LEN	(sizeof(AOLDMEDPRT) - 1)



static char * gencookie(void)
{
	static char cook[34];

	sprintf(cook, "%d_%u ", (int)mypid, myseqn);
	myseqn++;
	return cook;
}

#if 0
/* XXX */
static int
rtpp_checkcap(struct rtpp_node *node, char *cap, int caplen)
{
	char *cp;
	struct iovec vf[4] = {{NULL, 0}, {"VF", 2}, {" ", 1}, {NULL, 0}};

	vf[3].iov_base = cap;
	vf[3].iov_len = caplen;

	cp = send_rtpp_command(node, vf, 4);
	if (cp == NULL)
		return -1;
	if (cp[0] == 'E' || atoi(cp) != 1)
		return 0;
	return 1;
}
#endif

/* XXX */
static int
rtpp_test(struct rtpp_node *node, int isdisabled, int force)
{
	bencode_buffer_t bencbuf;
	bencode_item_t *dict;
	char *cp;
	int ret;

	if(node->rn_recheck_ticks == MI_MAX_RECHECK_TICKS){
	    LM_DBG("rtpp %s disabled for ever\n", node->rn_url.s);
		return 1;
	}
	if (force == 0) {
		if (isdisabled == 0)
			return 0;
		if (node->rn_recheck_ticks > get_ticks())
			return 1;
	}

	if (bencode_buffer_init(&bencbuf)) {
		LM_ERR("could not initialized bencode_buffer_t\n");
		return 1;
	}
	dict = bencode_dictionary(&bencbuf);
	bencode_dictionary_add_string(dict, "command", "ping");
	if (bencbuf.error)
		goto benc_error;

	cp = send_rtpp_command(node, dict, &ret);
	if (!cp) {
		LM_ERR("proxy did not respond to ping\n");
		goto error;
	}

	dict = bencode_decode_expect(&bencbuf, cp, ret, BENCODE_DICTIONARY);
	if (!dict || bencode_dictionary_get_strcmp(dict, "result", "pong")) {
		LM_ERR("proxy responded with invalid response\n");
		goto error;
	}

	LM_INFO("rtp proxy <%s> found, support for it %senabled\n",
	    node->rn_url.s, force == 0 ? "re-" : "");

	bencode_buffer_free(&bencbuf);
	return 0;

benc_error:
        LM_ERR("out of memory - bencode failed\n");
error:
	bencode_buffer_free(&bencbuf);
	return 1;

#if 0
	int rtpp_ver, rval;
	char *cp;
	struct iovec v[2] = {{NULL, 0}, {"V", 1}};

	cp = send_rtpp_command(node, v, 2);
	if (cp == NULL) {
		LM_WARN("can't get version of the RTP proxy\n");
		goto error;
	}
	rtpp_ver = atoi(cp);
	if (rtpp_ver != SUP_CPROTOVER) {
		LM_WARN("unsupported version of RTP proxy <%s> found: %d supported,"
				"%d present\n", node->rn_url.s, SUP_CPROTOVER, rtpp_ver);
		goto error;
	}
	rval = rtpp_checkcap(node, REQ_CPROTOVER, sizeof(REQ_CPROTOVER) - 1);
	if (rval == -1) {
		LM_WARN("RTP proxy went down during version query\n");
		goto error;
	}
	if (rval == 0) {
		LM_WARN("of RTP proxy <%s> doesn't support required protocol version"
				"%s\n", node->rn_url.s, REQ_CPROTOVER);
		goto error;
	}
	LM_INFO("rtp proxy <%s> found, support for it %senabled\n",
	    node->rn_url.s, force == 0 ? "re-" : "");
	/* Check for optional capabilities */
	rval = rtpp_checkcap(node, REP_CPROTOVER, sizeof(REP_CPROTOVER) - 1);
	if (rval != -1) {
		node->rn_rep_supported = rval;
	} else {
		node->rn_rep_supported = 0;
	}
	rval = rtpp_checkcap(node, PTL_CPROTOVER, sizeof(PTL_CPROTOVER) - 1);
	if (rval != -1) {
		node->rn_ptl_supported = rval;
	} else {
		node->rn_ptl_supported = 0;
	}
	return 0;
error:
	LM_WARN("support for RTP proxy <%s> has been disabled%s\n", node->rn_url.s,
	    rtpproxy_disable_tout < 0 ? "" : " temporarily");
	if (rtpproxy_disable_tout >= 0)
		node->rn_recheck_ticks = get_ticks() + rtpproxy_disable_tout;

	return 1;
#endif
}

char *
send_rtpp_command(struct rtpp_node *node, bencode_item_t *dict, int *outlen)
{
	struct sockaddr_un addr;
	int fd, len, i, vcnt;
	char *cp;
	static char buf[4096];
	struct pollfd fds[1];
	struct iovec *v;

	v = bencode_iovec(dict, &vcnt, 1, 0);
	if (!v) {
		LM_ERR("error converting bencode to iovec\n");
		return NULL;
	}

	len = 0;
	cp = buf;
	if (node->rn_umode == 0) {
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_LOCAL;
		strncpy(addr.sun_path, node->rn_address,
		    sizeof(addr.sun_path) - 1);
#ifdef HAVE_SOCKADDR_SA_LEN
		addr.sun_len = strlen(addr.sun_path);
#endif

		fd = socket(AF_LOCAL, SOCK_STREAM, 0);
		if (fd < 0) {
			LM_ERR("can't create socket\n");
			goto badproxy;
		}
		if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
			close(fd);
			LM_ERR("can't connect to RTP proxy\n");
			goto badproxy;
		}

		do {
			len = writev(fd, v + 1, vcnt);
		} while (len == -1 && errno == EINTR);
		if (len <= 0) {
			close(fd);
			LM_ERR("can't send command to a RTP proxy\n");
			goto badproxy;
		}
		do {
			len = read(fd, buf, sizeof(buf) - 1);
		} while (len == -1 && errno == EINTR);
		close(fd);
		if (len <= 0) {
			LM_ERR("can't read reply from a RTP proxy\n");
			goto badproxy;
		}
	} else {
		fds[0].fd = rtpp_socks[node->idx];
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		/* Drain input buffer */
		while ((poll(fds, 1, 0) == 1) &&
		    ((fds[0].revents & POLLIN) != 0)) {
			recv(rtpp_socks[node->idx], buf, sizeof(buf) - 1, 0);
			fds[0].revents = 0;
		}
		v[0].iov_base = gencookie();
		v[0].iov_len = strlen(v[0].iov_base);
		for (i = 0; i < rtpproxy_retr; i++) {
			do {
				len = writev(rtpp_socks[node->idx], v, vcnt + 1);
			} while (len == -1 && (errno == EINTR || errno == ENOBUFS));
			if (len <= 0) {
				LM_ERR("can't send command to a RTP proxy\n");
				goto badproxy;
			}
			while ((poll(fds, 1, rtpproxy_tout * 1000) == 1) &&
			    (fds[0].revents & POLLIN) != 0) {
				do {
					len = recv(rtpp_socks[node->idx], buf, sizeof(buf)-1, 0);
				} while (len == -1 && errno == EINTR);
				if (len <= 0) {
					LM_ERR("can't read reply from a RTP proxy\n");
					goto badproxy;
				}
				if (len >= (v[0].iov_len - 1) &&
				    memcmp(buf, v[0].iov_base, (v[0].iov_len - 1)) == 0) {
					len -= (v[0].iov_len - 1);
					cp += (v[0].iov_len - 1);
					if (len != 0) {
						len--;
						cp++;
					}
					goto out;
				}
				fds[0].revents = 0;
			}
		}
		if (i == rtpproxy_retr) {
			LM_ERR("timeout waiting reply from a RTP proxy\n");
			goto badproxy;
		}
	}

out:
	cp[len] = '\0';
	*outlen = len;
	return cp;
badproxy:
	LM_ERR("proxy <%s> does not respond, disable it\n", node->rn_url.s);
	node->rn_disabled = 1;
	node->rn_recheck_ticks = get_ticks() + rtpproxy_disable_tout;
	
	return NULL;
}

/*
 * select the set with the id_set id
 */

static struct rtpp_set * select_rtpp_set(int id_set ){

	struct rtpp_set * rtpp_list;
	/*is it a valid set_id?*/
	
	if(!rtpp_set_list || !rtpp_set_list->rset_first){
		LM_ERR("no rtp_proxy configured\n");
		return 0;
	}

	for(rtpp_list=rtpp_set_list->rset_first; rtpp_list!=0 && 
		rtpp_list->id_set!=id_set; rtpp_list=rtpp_list->rset_next);
	if(!rtpp_list){
		LM_ERR(" script error-invalid id_set to be selected\n");
	}

	return rtpp_list;
}
/*
 * Main balancing routine. This does not try to keep the same proxy for
 * the call if some proxies were disabled or enabled; proxy death considered
 * too rare. Otherwise we should implement "mature" HA clustering, which is
 * too expensive here.
 */
struct rtpp_node *
select_rtpp_node(str callid, int do_test)
{
	unsigned sum, sumcut, weight_sum;
	struct rtpp_node* node;
	int was_forced;

	if(!selected_rtpp_set){
		LM_ERR("script error -no valid set selected\n");
		return NULL;
	}
	/* Most popular case: 1 proxy, nothing to calculate */
	if (selected_rtpp_set->rtpp_node_count == 1) {
		node = selected_rtpp_set->rn_first;
		if (node->rn_disabled && node->rn_recheck_ticks <= get_ticks())
			node->rn_disabled = rtpp_test(node, 1, 0);
		return node->rn_disabled ? NULL : node;
	}

	/* XXX Use quick-and-dirty hashing algo */
	for(sum = 0; callid.len > 0; callid.len--)
		sum += callid.s[callid.len - 1];
	sum &= 0xff;

	was_forced = 0;
retry:
	weight_sum = 0;
	for (node=selected_rtpp_set->rn_first; node!=NULL; node=node->rn_next) {

		if (node->rn_disabled && node->rn_recheck_ticks <= get_ticks()){
			/* Try to enable if it's time to try. */
			node->rn_disabled = rtpp_test(node, 1, 0);
		}
		if (!node->rn_disabled)
			weight_sum += node->rn_weight;
	}
	if (weight_sum == 0) {
		/* No proxies? Force all to be redetected, if not yet */
		if (was_forced)
			return NULL;
		was_forced = 1;
		for(node=selected_rtpp_set->rn_first; node!=NULL; node=node->rn_next) {
			node->rn_disabled = rtpp_test(node, 1, 1);
		}
		goto retry;
	}
	sumcut = sum % weight_sum;
	/*
	 * sumcut here lays from 0 to weight_sum-1.
	 * Scan proxy list and decrease until appropriate proxy is found.
	 */
	for (node=selected_rtpp_set->rn_first; node!=NULL; node=node->rn_next) {
		if (node->rn_disabled)
			continue;
		if (sumcut < node->rn_weight)
			goto found;
		sumcut -= node->rn_weight;
	}
	/* No node list */
	return NULL;
found:
	if (do_test) {
		node->rn_disabled = rtpp_test(node, node->rn_disabled, 0);
		if (node->rn_disabled)
			goto retry;
	}
	return node;
}

static int
get_extra_id(struct sip_msg* msg, str *id_str) {
	if(msg==NULL || extra_id_pv==NULL || id_str==NULL) {
		LM_ERR("bad parameters\n");
		return -1;
	}
	if (pv_printf_s(msg, extra_id_pv, id_str)<0) {
		LM_ERR("cannot print the additional id\n");
		return -1;
	}

	return 1;

}



static inline int parse_rtpproxy_flags(const char *str1, struct rtpproxy_flags *flags, bencode_item_t *dict, int offer, struct sip_msg *msg)
{
	const char *cp;
	int via, ret;
	str viabranch;
	bencode_item_t *item;

	if (!dict)
		return -1;

	via = 0;

	for (cp = str1; cp != NULL && *cp != '\0'; cp++) {
		switch (*cp) {
		case '1':
		case '2':
			via = *cp - '0';
			break;

		case '3':
			if(msg && msg->first_line.type == SIP_REPLY)
				via = 2;
			else
				via = 1;
			break;

		case 'b':
		case 'B':
			via = -1;
			break;

		case 'a':
		case 'A':
			bencode_list_add_string(flags->flags, "asymmetric");
			bencode_list_add_string(flags->flags, "trust-address");
			break;

		case 'i':
		case 'I':
			bencode_list_add_string(flags->direction, "internal");
			break;

		case 'e':
		case 'E':
			bencode_list_add_string(flags->direction, "external");
			break;

		case 'l':
		case 'L':
			if (offer == 0)
				return -1;
			flags->flookup = 1;
			break;

		case 'r':
		case 'R':
			bencode_list_add_string(flags->flags, "trust-address");
			break;

		case 'o':
		case 'O':
			bencode_list_add_string(flags->replace, "origin");
			break;

		case 'c':
		case 'C':
			bencode_list_add_string(flags->replace, "session-connection");
			break;

		case 'f':
		case 'F':
			bencode_list_add_string(flags->flags, "force");
			break;

		case 'w':
		case 'W':
			bencode_list_add_string(flags->flags, "symmetric");
			break;

		case 'x':
		case 'X':
			bencode_list_add_string(flags->flags, "auto-bridge");
			break;

#if 0
		case 'z':
		case 'Z':
			if (append_opts(&rep_opts, 'Z') == -1) {
				LM_ERR("out of pkg memory\n");
				goto error;
			}
			/* If there are any digits following Z copy them into the command */
			for (; cp[1] != '\0' && isdigit(cp[1]); cp++) {
				if (append_opts(&rep_opts, cp[1]) == -1) {
					LM_ERR("out of pkg memory\n");
					goto error;
				}
			}
			break;
#endif

		default:
			LM_ERR("unknown option `%c'\n", *cp);
			return -1;
		}
	}

	/* only add those if any flags were given at all */
	if (flags->direction && flags->direction->child)
		bencode_dictionary_add(dict, "direction", flags->direction);
	if (flags->flags && flags->flags->child)
		bencode_dictionary_add(dict, "flags", flags->flags);
	if (flags->replace && flags->replace->child)
		bencode_dictionary_add(dict, "replace", flags->replace);

	/* add basic message parameters, common to all calls */
	if (get_callid(msg, &flags->callid) == -1 || flags->callid.len == 0) {
		LM_ERR("can't get Call-Id field\n");
		return -1;
	}
	bencode_dictionary_add_str(dict, "call-id", &flags->callid);

	if (via) {
		if (via == 1)
			ret = get_via_branch(msg, 1, &viabranch);
		else if (via == 2)
			ret = get_via_branch(msg, 2, &viabranch);
		else if (via == -1 && extra_id_pv)
			ret = get_extra_id(msg, &viabranch);
		else
			ret = -1;
		if (ret == -1 || viabranch.len == 0) {
			LM_ERR("can't get Via branch/extra ID\n");
			return -1;
		}
		bencode_dictionary_add_str(dict, "via-branch", &viabranch);
	}

	item = bencode_list(dict->buffer);
	bencode_dictionary_add(dict, "received-from", item);
	bencode_list_add_string(item, (msg->rcv.src_ip.af == AF_INET) ? "IP4" : (
#ifdef USE_IPV6
		(msg->rcv.src_ip.af == AF_INET6) ? "IP6" :
#endif
		"?"
	) );
	bencode_list_add_string(item, ip_addr2a(&msg->rcv.src_ip));

	return 0;
}



static int
unforce_rtp_proxy_f(struct sip_msg* msg, char* str1, char* str2)
{
	bencode_buffer_t bencbuf;
	bencode_item_t *dict;
	int ret;
	str from_tag, to_tag, s;
	char *cp;
	struct rtpp_node *node;
	struct rtpproxy_flags flags;

	if (bencode_buffer_init(&bencbuf)) {
		LM_ERR("could not initialized bencode_buffer_t\n");
		goto error_nb;
	}
	dict = bencode_dictionary(&bencbuf);

	memset(&flags, 0, sizeof(flags));

	if (parse_rtpproxy_flags(str1, &flags, dict, 0, msg))
		goto error;

	to_tag.s = 0;
	to_tag.len = 0;
	if ((flags.to == 1) && get_to_tag(msg, &to_tag) == -1) {
		LM_ERR("can't get To tag\n");
		goto error;
	}
	if (get_from_tag(msg, &from_tag) == -1 || from_tag.len == 0) {
		LM_ERR("can't get From tag\n");
		goto error;
	}
	bencode_dictionary_add_str(dict, "from-tag", &from_tag);
	if (to_tag.s && to_tag.len)
		bencode_dictionary_add_str(dict, "to-tag", &to_tag);
	
	if(msg->id != current_msg_id)
		selected_rtpp_set = default_rtpp_set;
	
	bencode_dictionary_add_string(dict, "command", "delete");

	if (bencbuf.error)
		goto benc_error;

	node = select_rtpp_node(flags.callid, 1);
	if (!node) {
		LM_ERR("no available proxies\n");
		goto error;
	}

	cp = send_rtpp_command(node, dict, &ret);
	if (!cp) {
		LM_ERR("no response from proxy\n");
		goto error;
	}

	dict = bencode_decode_expect(&bencbuf, cp, ret, BENCODE_DICTIONARY);
	if (!dict) {
		LM_ERR("failed to decode bencoded reply from proxy: %.*s\n", ret, cp);
		goto error;
	}
	if (bencode_dictionary_get_strcmp(dict, "result", "ok")) {
		if (!bencode_dictionary_get_str(dict, "error-reason", &s))
			LM_ERR("proxy didn't give an \"ok\" response and didn't give an error reason: %.*s\n", ret, cp);
		else
			LM_ERR("proxy replied with error: %.*s\n", s.len, s.s);
		goto error;
	}

	bencode_buffer_free(&bencbuf);
	return 1;

benc_error:
	LM_ERR("out of memory - bencode failed\n");
error:
	bencode_buffer_free(&bencbuf);
error_nb:
	return -1;
}

/* This function assumes p points to a line of requested type. */

static int
set_rtp_proxy_set_f(struct sip_msg * msg, char * str1, char * str2)
{
	rtpp_set_link_t *rtpl;
	pv_value_t val;

	rtpl = (rtpp_set_link_t*)str1;

	current_msg_id = 0;
	selected_rtpp_set = 0;

	if(rtpl->rset != NULL) {
		current_msg_id = msg->id;
		selected_rtpp_set = rtpl->rset;
	} else {
		if(pv_get_spec_value(msg, rtpl->rpv, &val)<0) {
			LM_ERR("cannot evaluate pv param\n");
			return -1;
		}
		if(!(val.flags & PV_VAL_INT)) {
			LM_ERR("pv param must hold an integer value\n");
			return -1;
		}
		selected_rtpp_set = select_rtpp_set(val.ri);
		if(selected_rtpp_set==NULL) {
			LM_ERR("could not locate rtpproxy set %d\n", val.ri);
			return -1;
		}
		current_msg_id = msg->id;
	}
	return 1;
}

static int
rtpproxy_manage(struct sip_msg *msg, char *flags, char *ip)
{
	char *cp = NULL;
	char newip[IP_ADDR_MAX_STR_SIZE];
	int method;
	int nosdp;

	if(msg->cseq==NULL && ((parse_headers(msg, HDR_CSEQ_F, 0)==-1)
				|| (msg->cseq==NULL)))
	{
		LM_ERR("no CSEQ header\n");
		return -1;
	}

	method = get_cseq(msg)->method_id;

	if(!(method==METHOD_INVITE || method==METHOD_ACK || method==METHOD_CANCEL
				|| method==METHOD_BYE || method==METHOD_UPDATE))
		return -1;

	if(method==METHOD_CANCEL || method==METHOD_BYE)
		return unforce_rtp_proxy_f(msg, flags, 0);

	if(ip==NULL)
	{
		cp = ip_addr2a(&msg->rcv.dst_ip);
		strcpy(newip, cp);
	}

	if(msg->msg_flags & FL_SDP_BODY)
		nosdp = 0;
	else
		nosdp = parse_sdp(msg);

	if(msg->first_line.type == SIP_REQUEST) {
		if(method==METHOD_ACK && nosdp==0)
			return force_rtp_proxy(msg, flags, (cp!=NULL)?newip:ip, 0,
					(ip!=NULL)?1:0);
		if(method==METHOD_UPDATE && nosdp==0)
			return force_rtp_proxy(msg, flags, (cp!=NULL)?newip:ip, 1,
					(ip!=NULL)?1:0);
		if(method==METHOD_INVITE && nosdp==0) {
			msg->msg_flags |= FL_SDP_BODY;
			if(tmb.t_gett!=NULL && tmb.t_gett()!=NULL
					&& tmb.t_gett()!=T_UNDEFINED)
				tmb.t_gett()->uas.request->msg_flags |= FL_SDP_BODY;
			if(route_type==FAILURE_ROUTE)
				return unforce_rtp_proxy_f(msg, flags, 0);
			return force_rtp_proxy(msg, flags, (cp!=NULL)?newip:ip, 1,
					(ip!=NULL)?1:0);
		}
	} else if(msg->first_line.type == SIP_REPLY) {
		if(msg->first_line.u.reply.statuscode>=300)
			return unforce_rtp_proxy_f(msg, flags, 0);
		if(nosdp==0) {
			if(method==METHOD_UPDATE)
				return force_rtp_proxy(msg, flags, (cp!=NULL)?newip:ip, 0,
					(ip!=NULL)?1:0);
			if(tmb.t_gett==NULL || tmb.t_gett()==NULL
					|| tmb.t_gett()==T_UNDEFINED)
				return force_rtp_proxy(msg, flags, (cp!=NULL)?newip:ip, 0,
					(ip!=NULL)?1:0);
			if(tmb.t_gett()->uas.request->msg_flags & FL_SDP_BODY)
				return force_rtp_proxy(msg, flags, (cp!=NULL)?newip:ip, 0,
					(ip!=NULL)?1:0);
			return force_rtp_proxy(msg, flags, (cp!=NULL)?newip:ip, 1,
					(ip!=NULL)?1:0);
		}
	}
	return -1;
}

static int
rtpproxy_manage0(struct sip_msg *msg, char *flags, char *ip)
{
	return rtpproxy_manage(msg, 0, 0);
}

static int
rtpproxy_manage1(struct sip_msg *msg, char *flags, char *ip)
{
	str flag_str;
	fixup_get_svalue(msg, (gparam_p)flags, &flag_str);
	return rtpproxy_manage(msg, flag_str.s, 0);
}

static int
rtpproxy_manage2(struct sip_msg *msg, char *flags, char *ip)
{
	str flag_str;
	str ip_str;
	fixup_get_svalue(msg, (gparam_p)flags, &flag_str);
	fixup_get_svalue(msg, (gparam_p)ip, &ip_str);
	return rtpproxy_manage(msg, flag_str.s, ip_str.s);
}

static int
rtpproxy_offer1_f(struct sip_msg *msg, char *str1, char *str2)
{
        char *cp;
        char newip[IP_ADDR_MAX_STR_SIZE];

        cp = ip_addr2a(&msg->rcv.dst_ip);
        strcpy(newip, cp);
	return force_rtp_proxy(msg, str1, newip, 1, 0);
}

static int
rtpproxy_offer2_f(struct sip_msg *msg, char *param1, char *param2)
{
	return force_rtp_proxy(msg, param1, param2, 1, 1);
}

static int
rtpproxy_answer1_f(struct sip_msg *msg, char *str1, char *str2)
{
        char *cp;
        char newip[IP_ADDR_MAX_STR_SIZE];

	if (msg->first_line.type == SIP_REQUEST)
		if (msg->first_line.u.request.method_value != METHOD_ACK)
			return -1;

        cp = ip_addr2a(&msg->rcv.dst_ip);
        strcpy(newip, cp);
	return force_rtp_proxy(msg, str1, newip, 0, 0);
}

static int
rtpproxy_answer2_f(struct sip_msg *msg, char *param1, char *param2)
{

	if (msg->first_line.type == SIP_REQUEST)
		if (msg->first_line.u.request.method_value != METHOD_ACK)
			return -1;

	return force_rtp_proxy(msg, param1, param2, 0, 1);
}

static int
force_rtp_proxy(struct sip_msg* msg, char* str1, char* str2, int offer, int forcedIP)
{
	bencode_buffer_t bencbuf;
	bencode_item_t *dict;
	int ret, create;
	str body, newbody;
	str from_tag, to_tag, tmp;
	struct lump *anchor;
	struct rtpp_node *node;
	char *cp;
	struct rtpproxy_flags flags;

	newbody.s = NULL;
	if (bencode_buffer_init(&bencbuf)) {
		LM_ERR("could not initialized bencode_buffer_t\n");
		goto error_nb;
	}
	dict = bencode_dictionary(&bencbuf);

	memset(&flags, 0, sizeof(flags));

	flags.flags = bencode_list(&bencbuf);
	flags.direction = bencode_list(&bencbuf);
	flags.replace = bencode_list(&bencbuf);

	if (parse_rtpproxy_flags(str1, &flags, dict, offer, msg))
		goto error;

	create = offer ? 1 : 0;

	/* extract_body will also parse all the headers in the message as
	 * a side effect => don't move get_callid/get_to_tag in front of it
	 * -- andrei */
	if (extract_body(msg, &body) == -1) {
		LM_ERR("can't extract body from the message\n");
		goto error;
	}
	bencode_dictionary_add_str(dict, "sdp", &body);

	to_tag.s = 0;
	if (get_to_tag(msg, &to_tag) == -1) {
		LM_ERR("can't get To tag\n");
		goto error;
	}
	if (get_from_tag(msg, &from_tag) == -1 || from_tag.len == 0) {
		LM_ERR("can't get From tag\n");
		goto error;
	}
	if (flags.flookup != 0) {
		if (to_tag.len == 0) {
			goto error;
		}
		create = 0;
	} else if ((msg->first_line.type == SIP_REPLY && offer != 0)
			|| (msg->first_line.type == SIP_REQUEST && offer == 0)) {
		if (to_tag.len == 0) {
			goto error;
		}
		tmp = from_tag;
		from_tag = to_tag;
		to_tag = tmp;
	}
	bencode_dictionary_add_str(dict, "from-tag", &from_tag);
	if (to_tag.s && to_tag.len)
		bencode_dictionary_add_str(dict, "to-tag", &to_tag);

	if(msg->id != current_msg_id)
		selected_rtpp_set = default_rtpp_set;

	bencode_dictionary_add_string(dict, "command", (create == 0) ? "answer" : "offer");

	if (bencbuf.error)
		goto benc_error;

	do {
		node = select_rtpp_node(flags.callid, 1);
		if (!node) {
			LM_ERR("no available proxies\n");
			goto error;
		}

		cp = send_rtpp_command(node, dict, &ret);
	} while (cp == NULL);
	LM_DBG("proxy reply: %.*s\n", ret, cp);

	dict = bencode_decode_expect(&bencbuf, cp, ret, BENCODE_DICTIONARY);
	if (!dict) {
		LM_ERR("failed to decode bencoded reply from proxy: %.*s\n", ret, cp);
		goto error;
	}
	if (bencode_dictionary_get_strcmp(dict, "result", "ok")) {
		if (!bencode_dictionary_get_str(dict, "error-reason", &newbody))
			LM_ERR("proxy didn't give an \"ok\" response and didn't give an error reason: %.*s\n", ret, cp);
		else
			LM_ERR("proxy replied with error: %.*s\n", newbody.len, newbody.s);
		goto error;
	}

	if (!bencode_dictionary_get_str_dup(dict, "sdp", &newbody)) {
		LM_ERR("failed to extract sdp body from proxy reply: %.*s\n", ret, cp);
		goto error;
	}

	anchor = del_lump(msg, body.s - msg->buf, body.len, 0);
	if (!anchor) {
		LM_ERR("del_lump failed\n");
		goto error;
	}
	if (!insert_new_lump_after(anchor, newbody.s, newbody.len, 0)) {
		LM_ERR("insert_new_lump_after failed\n");
		goto error;
	}
	newbody.s = NULL;

	bencode_buffer_free(&bencbuf);
	return 1;

benc_error:
	LM_ERR("out of memory - bencode failed\n");
error:
	bencode_buffer_free(&bencbuf);
error_nb:
	if (newbody.s)
		pkg_free(newbody.s);
	return -1;
}


static int start_recording_f(struct sip_msg* msg, char *foo, char *bar)
{
#if 0
	int nitems;
	str callid = {0, 0};
	str from_tag = {0, 0};
	str to_tag = {0, 0};
	struct rtpp_node *node;
	struct iovec v[1 + 4 + 3] = {{NULL, 0}, {"R", 1}, {" ", 1}, {NULL, 0}, {" ", 1}, {NULL, 0}, {" ", 1}, {NULL, 0}};
	                             /* 1 */   /* 2 */   /* 3 */    /* 4 */   /* 5 */    /* 6 */   /* 1 */

	if (get_callid(msg, &callid) == -1 || callid.len == 0) {
		LM_ERR("can't get Call-Id field\n");
		return -1;
	}

	if (get_to_tag(msg, &to_tag) == -1) {
		LM_ERR("can't get To tag\n");
		return -1;
	}

	if (get_from_tag(msg, &from_tag) == -1 || from_tag.len == 0) {
		LM_ERR("can't get From tag\n");
		return -1;
	}

	if(msg->id != current_msg_id){
		selected_rtpp_set = default_rtpp_set;
	}

	STR2IOVEC(callid, v[3]);
	STR2IOVEC(from_tag, v[5]);
	STR2IOVEC(to_tag, v[7]);
	node = select_rtpp_node(callid, 1);
	if (!node) {
		LM_ERR("no available proxies\n");
		return -1;
	}

	nitems = 8;
	if (msg->first_line.type == SIP_REPLY) {
		if (to_tag.len == 0)
			return -1;
		STR2IOVEC(to_tag, v[5]);
		STR2IOVEC(from_tag, v[7]);
	} else {
		STR2IOVEC(from_tag, v[5]);
		STR2IOVEC(to_tag, v[7]);
		if (to_tag.len <= 0)
			nitems = 6;
	}
	send_rtpp_command(node, v, nitems);

#endif
	return 1;
}

/*
 * Returns the current RTP-Statistics from the RTP-Proxy
 */
static int
pv_get_rtpstat_f(struct sip_msg *msg, pv_param_t *param,
		  pv_value_t *res)
{
#if 0
    str ret_val = {0, 0};
    int nitems;
    str callid = {0, 0};
    str from_tag = {0, 0};
    str to_tag = {0, 0};
    struct rtpp_node *node;
    struct iovec v[1 + 4 + 3 + 1] = {{NULL, 0}, {"Q", 1}, {" ", 1}, {NULL, 0},
		{" ", 1}, {NULL, 0}, {";1 ", 3}, {";1", }, {NULL, 0}};

    if (get_callid(msg, &callid) == -1 || callid.len == 0) {
        LM_ERR("can't get Call-Id field\n");
		return pv_get_null(msg, param, res);
    }
    if (get_to_tag(msg, &to_tag) == -1) {
        LM_ERR("can't get To tag\n");
		return pv_get_null(msg, param, res);
    }
    if (get_from_tag(msg, &from_tag) == -1 || from_tag.len == 0) {
        LM_ERR("can't get From tag\n");
		return pv_get_null(msg, param, res);
    }
    if(msg->id != current_msg_id){
        selected_rtpp_set = default_rtpp_set;
    }

    STR2IOVEC(callid, v[3]);
    STR2IOVEC(from_tag, v[5]);
    STR2IOVEC(to_tag, v[7]);
    node = select_rtpp_node(callid, 1);
    if (!node) {
        LM_ERR("no available proxies\n");
        return -1;
    }
    nitems = 8;
    if (msg->first_line.type == SIP_REPLY) {
        if (to_tag.len == 0)
            return -1;
        STR2IOVEC(to_tag, v[5]);
        STR2IOVEC(from_tag, v[7]);
    } else {
        STR2IOVEC(from_tag, v[5]);
        STR2IOVEC(to_tag, v[7]);
        if (to_tag.len <= 0)
            nitems = 6;
    }
    ret_val.s = send_rtpp_command(node, v, nitems);
	if(ret_val.s==NULL)
		return pv_get_null(msg, param, res);
    ret_val.len = strlen(ret_val.s);
    return pv_get_strval(msg, param, res, &ret_val);
#else
	return 1;
#endif
}

