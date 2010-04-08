/*
** Copyright (c) 1995, 2008 Ingres Corporation
*/


#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <uld.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/*
** Name:	optdata.c
**
** Description:	Global data for opt facility.
**
** History:
**      19-Oct-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**	10-Jan-2001 (jenjo02)
**	    Added OPG_CB* Opg_cb GLOBALDEF.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPOPFLIBDATA
**	    in the Jamfile.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

/*
**
LIBRARY	=	IMPOPFLIBDATA 
**
*/


/* Pointer to the server-wide OPF control block */
GLOBALDEF	OPG_CB	*Opg_cb	ZERO_FILL;

/*
**  Data from optcotree.c
*/

struct	tab
{
	i4	t_opcode;
	char	*t_string;
};
#define OPT_TEND (-99)
GLOBALDEF const	struct tab	opf_sjtyps_tab[] =
{
        DB_ORIG,        "Orig",
        DB_PR,          "Proj-rest",
        DB_SJ,          "Join",     
	DB_EXCH,	"Exchange",
        DB_RSORT,       "Sort",     
        DB_RISAM,       "Ref-isam", 
        DB_RHASH,       "Ref-hash", 
        DB_RBTREE,      "Ref-Btree",
        OPT_TEND,       "???"
};

