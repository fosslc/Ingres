/*
**	Copyright (c) 2004 Ingres Corporation
*/


#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>

/*
** Name:	adgdata.c
**
** Description:	Global data for ADG facility.
**
** History:
**      12-Dec-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**	11-jun-2001 (somsa01)
**	    To save stack space, we now dynamically allocate space for
**	    the min / max arrays in adg_startup().
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPADFLIBDATA
**	    in the Jamfile.
*/

/*
**
LIBRARY = IMPADFLIBDATA
**
*/

/*
**  Data from adginit.c
*/

GLOBALDEF	ADF_SERVER_CB	*Adf_globs = NULL;  /* ADF's global server CB.*/


/*
**  Data from adgstartup.c
*/

GLOBALDEF   u_char	*Chr_min ZERO_FILL;
GLOBALDEF   u_char	*Chr_max ZERO_FILL;
GLOBALDEF   u_char	*Cha_min ZERO_FILL;
GLOBALDEF   u_char	*Cha_max ZERO_FILL;
GLOBALDEF   u_char	*Txt_max ZERO_FILL;
GLOBALDEF   u_char	*Vch_min ZERO_FILL;
GLOBALDEF   u_char	*Vch_max ZERO_FILL;
GLOBALDEF   u_char	*Lke_min ZERO_FILL;
GLOBALDEF   u_char	*Lke_max ZERO_FILL;
GLOBALDEF   u_char	*Bit_min ZERO_FILL;
GLOBALDEF   u_char	*Bit_max ZERO_FILL;
