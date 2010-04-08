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
#include	"curtc.h"

/**
** Name:	curtc.roc	-  Table to convert from rectype to CURECORD
**
** Description:
**	This files defines:
**	
**	IICUrtcRectypeToCurecord
**
** Note:  THIS IS NOT SHAREABLE ON VAX/VMS.
**
** History:
**	Revision 6.2  89/06  wong
**	Added entry for DB procedures.  JupBug #6466.
**
**	Revision 6.0  87/07/05  joe
**	Initial revision.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/
extern CURECORD IICUaprAPplRecord;
extern CURECORD IICUapdAPplDepend;
extern CURECORD IICUaorAbfObjectRecord;
extern CURECORD IICUadrAbfDependRecord;
extern CURECORD IICUforFOrmRecord;
extern CURECORD IICUfirFIeldRecord;
extern CURECORD IICUtrrTRimRecord;
extern CURECORD IICUgbrGBfRecord;
extern CURECORD IICUgcrGCommandsRecord;
extern CURECORD IICUjdrJoinDefRecord;
extern CURECORD IICUqnrQbfNameRecord;
extern CURECORD IICUrwrReportWriterRecord;
extern CURECORD IICUrcrRCommandsRecord;
extern CURECORD IICUvirVIgraphRecord;
extern CURECORD IICUenrENcodedRecord;
extern CURECORD IICUatrATtributeRecord;
extern CURECORD IICUvqjVQJoinRecord;
extern CURECORD IICUvqtVQTabRecord;
extern CURECORD IICUvqcVQColRecord;
extern CURECORD IICUmaMenuArgRecord;
extern CURECORD IICUfvFrameVarRecord;
extern CURECORD IICUecrEscapeCodeRecord;

GLOBALDEF const CURTC	iiCUrtcRectypeToCurecord[] =
{
    {CUC_ADEPEND,	&IICUapdAPplDepend},
    {CUC_AODEPEND,	&IICUadrAbfDependRecord},
    {CUC_FIELD,		&IICUfirFIeldRecord},
    {CUC_TRIM,		&IICUtrrTRimRecord},
    {CUC_RCOMMANDS,	&IICUrcrRCommandsRecord},
    {CUO_GBF,		&IICUgbrGBfRecord},
    {CUC_GCOMMANDS,	&IICUgcrGCommandsRecord},
    {CUC_VQJOIN,	&IICUvqjVQJoinRecord},
    {CUC_VQTAB,		&IICUvqtVQTabRecord},
    {CUC_VQTCOL,	&IICUvqcVQColRecord},
    {CUC_MENUARG,	&IICUmaMenuArgRecord},
    {CUC_FRMVAR,	&IICUfvFrameVarRecord},
    {OC_APPL,		&IICUaprAPplRecord},
    {OC_HLPROC,		&IICUaorAbfObjectRecord},
    {OC_OSLPROC,	&IICUaorAbfObjectRecord},
    {OC_DBPROC,		&IICUaorAbfObjectRecord},
    {OC_UNPROC,		&IICUaorAbfObjectRecord},
    {OC_QBFNAME,	&IICUqnrQbfNameRecord},
    {OC_OSLFRAME,	&IICUaorAbfObjectRecord},
    {OC_RWFRAME,	&IICUaorAbfObjectRecord},
    {OC_QBFFRAME,	&IICUaorAbfObjectRecord},
    {OC_GRFRAME,	&IICUaorAbfObjectRecord},
    {OC_GBFFRAME,	&IICUaorAbfObjectRecord},
    {OC_MUFRAME,	&IICUaorAbfObjectRecord},
    {OC_APPFRAME,	&IICUaorAbfObjectRecord},
    {OC_BRWFRAME,	&IICUaorAbfObjectRecord},
    {OC_UPDFRAME,	&IICUaorAbfObjectRecord},
    {OC_UNFRAME,	&IICUaorAbfObjectRecord},
    {OC_AFORMREF,	&IICUaorAbfObjectRecord},
    {OC_RECORD,		&IICUaorAbfObjectRecord},
    {OC_RECMEM,		&IICUatrATtributeRecord},
    {OC_GLOBAL,		&IICUaorAbfObjectRecord},
    {OC_CONST,		&IICUaorAbfObjectRecord},
    {OC_FORM,		&IICUforFOrmRecord},
    {OC_JOINDEF,	&IICUjdrJoinDefRecord},
    {OC_REPORT,		&IICUrwrReportWriterRecord},
    {OC_RWREP,		&IICUrwrReportWriterRecord},
    {OC_RBFREP,		&IICUrwrReportWriterRecord},
    {OC_GRAPH,		&IICUvirVIgraphRecord},
    {CUE_GRAPH,		&IICUenrENcodedRecord},
    {CUC_ESCAPE,	&IICUecrEscapeCodeRecord},
    {CU_BADTYPE,	NULL}
};
