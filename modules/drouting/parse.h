/*
 * $Id$
 *
 * Copyright (C) 2005-2009 Voice Sistem SRL
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * Kamailio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Kamailio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * History:
 * ---------
 *  2005-07-27  first version (bogdan)
 */


#ifndef dr_parse_h_
#define dr_parse_h_


#define SEP '|'
#define SEP1 ','
#define SEP_GRP ';'

#define IS_SPACE(s)\
	((s)==' ' || (s)=='\t' || (s)=='\r' || (s)=='\n')

#define EAT_SPACE(s)\
	while((s) && IS_SPACE(*(s))) (s)++


#endif
