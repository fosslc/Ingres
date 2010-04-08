/*
** Copyright (c) 1996, 2004 Ingres Corporation
*/

#include    <compat.h>
# include	<gl.h>
# include	<er.h>
# include	<lo.h>
# include	<nm.h>
# include	<pc.h>
# include	<si.h>
# include	<me.h>
# include	<st.h>
# include	<cs.h>
# include       "nmlocal.h"

/*
** Name:	nmdata.c
**
** Description:	Global data for NM facility.
**
LIBRARY = IMPCOMPATLIBDATA
**
** History:
**      12-Sep-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**	12-dec-2003 (somsa01)
**	    Added symbol.bak, symbol.log, and symbol.lck locations.
**	03-feb-2004 (somsa01)
**	    Backed out last changes for now.
**	06-apr-2004 (somsa01)
**	    Added symbol.bak and symbol.log locations.
**      17-Nov-2004 (rigka01) bug 112953/INGSRV2955, cross Unix change 472415
**	    Added symbol.lck location to enable locking the symbol table 
**          during update.
**
*/

GLOBALDEF SYM		*s_list ZERO_FILL;	/* lst of symbols */
GLOBALDEF SYSTIME	NMtime ZERO_FILL;	/* modification time */
GLOBALDEF LOCATION	NMSymloc ZERO_FILL;	/* location of symbol.tbl */
GLOBALDEF II_NM_STATIC  NM_static ZERO_FILL;
GLOBALDEF LOCATION      NMLckSymloc ZERO_FILL;  /* location of symbol.lck */
GLOBALDEF LOCATION	NMBakSymloc ZERO_FILL;	/* location of symbol.bak */
GLOBALDEF LOCATION	NMLogSymloc ZERO_FILL;	/* location of symbol.log */

