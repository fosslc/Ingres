/*
**Copyright (c) 2004 Ingres Corporation
*/

# include   <compat.h>
# include   <gl.h>
# include   <cs.h>
# include   <me.h>
# include   <sl.h>
# include   <iicommon.h>
# include   <dbdbms.h>
# include   <scf.h>
# include   <ulf.h>
# include   <ulm.h>
# include   <ulh.h>
# include   <qsf.h>
# include   <qefmain.h>
# include   <qefscb.h>

/*
** Name:	qecdata.c
**
** Description:	Global data for qec facility.
**
** History:
**      19-Oct-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPQEFLIBDATA
**	    in the Jamfile.
**
*/

/* Jam hints
**
LIBRARY = IMPQEFLIBDATA
**
*/

/*
**  Data from qec.c
*/


GLOBALDEF QEF_S_CB   *Qef_s_cb  ZERO_FILL; /* The QEF server control block. */
