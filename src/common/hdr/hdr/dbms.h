/*
** Copyright (c) 2004 Ingres Corporation
*/
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
/**
** Name: DBMS.H - Obsolete file, only used to include above files now
**
**	This file should eventually disappear once the above nested includes
**	have been propagated to all ingres files.
** History:
**	8-mar-93 (ed)
**	    split into iicommon.h and dbdbms.h, add gl.h at request of
**	    CL committee, 
**	    - sl.h was added since it will eventually be
**	    unnested also in order to eliminate compat.h cut and paste
**	    sl.h defines
**	5-aug-93 (ed)
**	    - remove dbdbms.h and move it backhdr
**/
/* do not add any new structures here
** - if structure is only visible to the dbms, then add it to dbdbms.h
** - if structure is also required by FE then add it to iicommon.h
*/
