/*
** Copyright (c) 1987, 2008 Ingres Corporation
**	All rights reserved.
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ooclass.h>
#include	<cu.h>
#include	"ercu.h"
#include	"cuntr.h"

/**
** Name:	cuntr.roc -	Name/Record Type Map Table.
**
** Description:
**	Contains the definition of the copy record type to name mapping
**	table.  Defines:
**	
**	iiCUntrNameToRectype.
**
** History:
**	Revision 6.0  87/07/03  joe
**	Initial revision.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/*:
** Name:	iiCUntrNameToRecType[]
**
**	3-jul-1987 (Joe)
**		Initial Version
**/

GLOBALDEF const CUNTR iiCUntrNameToRectype[] =
{
    {CUC_ADEPEND,	ERx("CUC_ADEPEND"),	
		sizeof(ERx("CUC_ADEPEND"))-1, 0},
    {CUC_AODEPEND,	ERx("CUC_AODEPEND"),	
		sizeof(ERx("CUC_AODEPEND"))-1, 0},
    {CUC_FIELD,		ERx("CUC_FIELD"),	
		sizeof(ERx("CUC_FIELD"))-1, 0},
    {CUC_TRIM,		ERx("CUC_TRIM"),	
		sizeof(ERx("CUC_TRIM"))-1, 0},
    {CUC_RCOMMANDS,	ERx("CUC_RCOMMANDS"),	
		sizeof(ERx("CUC_RCOMMANDS"))-1, 0},
    {CUO_GBF,		ERx("CUO_GBF"),	
		sizeof(ERx("CUO_GBF"))-1, F_CU0001_GBF_GRAPH},
    {CUC_GCOMMANDS,	ERx("CUC_GCOMMANDS"),	
		sizeof(ERx("CUC_GCOMMANDS"))-1, 0},
    {CUC_VQTAB,	ERx("CUC_VQTAB"),	
		sizeof(ERx("CUC_VQTAB"))-1, 0},
    {CUC_VQTCOL,	ERx("CUC_VQTCOL"),	
		sizeof(ERx("CUC_VQTCOL"))-1, 0},
    {CUC_VQJOIN,	ERx("CUC_VQJOIN"),	
		sizeof(ERx("CUC_VQJOIN"))-1, 0},
    {CUC_MENUARG,	ERx("CUC_MENUARG"),	
		sizeof(ERx("CUC_MENUARG"))-1, 0},
    {CUC_FRMVAR,	ERx("CUC_FRMVAR"),	
		sizeof(ERx("CUC_FRMVAR"))-1, 0},
    {OC_APPL,		ERx("OC_APPL"),	
		sizeof(ERx("OC_APPL"))-1, F_CU0002_APPLICATION},
    {OC_HLPROC,		ERx("OC_HLPROC"),	
		sizeof(ERx("OC_HLPROC"))-1, F_CU0003_PROCEDURE},
    {OC_OSLPROC,	ERx("OC_OSLPROC"),	
		sizeof(ERx("OC_OSLPROC"))-1, F_CU0003_PROCEDURE},
    {OC_DBPROC,		ERx("OC_DBPROC"),	
		sizeof(ERx("OC_DBPROC"))-1, F_CU0003_PROCEDURE},
    {OC_UNPROC,		ERx("OC_UNPROC"),	
		sizeof(ERx("OC_UNPROC"))-1, F_CU0003_PROCEDURE},
    {OC_QBFNAME,	ERx("OC_QBFNAME"),	
		sizeof(ERx("OC_QBFNAME"))-1, F_CU0004_QBFNAME},
    {OC_OSLFRAME,	ERx("OC_OSLFRAME"),	
		sizeof(ERx("OC_OSLFRAME"))-1, F_CU0005_FRAME},
    {OC_RWFRAME,	ERx("OC_RWFRAME"),	
		sizeof(ERx("OC_RWFRAME"))-1, F_CU0005_FRAME},
    {OC_QBFFRAME,	ERx("OC_QBFFRAME"),	
		sizeof(ERx("OC_QBFFRAME"))-1, F_CU0005_FRAME},
    {OC_GRFRAME,	ERx("OC_GRFRAME"),	
		sizeof(ERx("OC_GRFRAME"))-1, F_CU0005_FRAME},
    {OC_GBFFRAME,	ERx("OC_GBFFRAME"),	
		sizeof(ERx("OC_GBFFRAME"))-1, F_CU0005_FRAME},
    {OC_MUFRAME,	ERx("OC_MUFRAME"),	
		sizeof(ERx("OC_MUFRAME"))-1, F_CU0005_FRAME},
    {OC_APPFRAME,	ERx("OC_APPFRAME"),	
		sizeof(ERx("OC_APPFRAME"))-1, F_CU0005_FRAME},
    {OC_UPDFRAME,	ERx("OC_UPDFRAME"),	
		sizeof(ERx("OC_UPDFRAME"))-1, F_CU0005_FRAME},
    {OC_BRWFRAME,	ERx("OC_BRWFRAME"),	
		sizeof(ERx("OC_BRWFRAME"))-1, F_CU0005_FRAME},
    {OC_UNFRAME,	ERx("OC_UNFRAME"),	
		sizeof(ERx("OC_UNFRAME"))-1, F_CU0005_FRAME},
    {OC_AFORMREF,	ERx("OC_AFORMREF"),	
		sizeof(ERx("OC_AFORMREF"))-1, 0},
    {OC_FORM,		ERx("OC_FORM"),	
		sizeof(ERx("OC_FORM"))-1, F_CU0006_FORM},
    {OC_JOINDEF,	ERx("OC_JOINDEF"),	
		sizeof(ERx("OC_JOINDEF"))-1, F_CU0007_JOINDEF},
    {OC_REPORT,		ERx("OC_REPORT"),	
		sizeof(ERx("OC_REPORT"))-1, F_CU0008_REPORT},
    {OC_RWREP,		ERx("OC_RWREP"),	
		sizeof(ERx("OC_RWREP"))-1, F_CU0008_REPORT},
    {OC_RBFREP,		ERx("OC_RBFREP"),	
		sizeof(ERx("OC_RBFREP"))-1, F_CU0008_REPORT},
    {OC_GRAPH,		ERx("OC_GRAPH"),	
		sizeof(ERx("OC_GRAPH"))-1, F_CU0009_GRAPH},
    {OC_RECORD,		ERx("OC_RECORD"),	
		sizeof(ERx("OC_RECORD"))-1, F_CU0010_CLASS},
    {OC_RECMEM,		ERx("OC_RECMEM"),	
		sizeof(ERx("OC_RECMEM"))-1, F_CU0011_ATTRIBUTE},
    {OC_GLOBAL,		ERx("OC_GLOBAL"),	
		sizeof(ERx("OC_GLOBAL"))-1, F_CU0012_GLOBAL},
    {OC_CONST,		ERx("OC_CONST"),	
		sizeof(ERx("OC_CONST"))-1, F_CU0013_CONSTANT},
    {CUE_GRAPH,		ERx("CUE_GRAPH"),	
		sizeof(ERx("CUE_GRAPH"))-1, F_CU0009_GRAPH},
    {CUC_ESCAPE,	ERx("CUC_ESCAPE"),	
		sizeof(ERx("CUC_ESCAPE"))-1, 0},
    {CU_BADTYPE,	ERx(""),		0}
};
