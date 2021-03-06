/*
 * $Id$
 *
 * This file is part of SIP-router, a free SIP server.
 *
 * SIP-router is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * SIP-router is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*!
 * \file
 * \brief Debugger :: Configuration
 * \ingroup debugger
 */


#ifndef _DEBUGGER_CONFIG_H
#define _DEBUGGER_CONFIG_H

#include "../../cfg/cfg.h"
#include "../../str.h"

struct cfg_group_dbg {
	unsigned int mod_level_mode;
	unsigned int mod_hash_size;
};

extern struct cfg_group_dbg	default_dbg_cfg;
extern void	*dbg_cfg;
extern cfg_def_t dbg_cfg_def[];

extern int dbg_level_mode_fixup(void *temp_handle,
	str *group_name, str *var_name, void **value);
#endif
