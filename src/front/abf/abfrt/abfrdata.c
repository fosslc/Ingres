/*
** Copyright (c) 1996, 2008 Ingres Corporation
*/
 
#include        <compat.h>
# include       <gl.h>    
# include       <sl.h>
# include       <lo.h>
# include       <er.h>
# include       <iicommon.h>
#include        <fe.h>
#include        <rtsdata.h>
#include        <fdesc.h>
#include	<abfrts.h>

/*
** Name:        abfrdata.c
**
** Description: Global data for abfrt facility.
**
** History:
**
**      24-sep-96 (hanch04)
**          Created.
**      10=mar-98 (rosga02)
**              Added IIParamPassing.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

/*
**	abfdbg.c
*/

GLOBALDEF char  *IIarTest = NULL;       /* Names for testing files (-Z) */
GLOBALDEF char  *IIarDebug = NULL;      /* Name of debug output file (-D) */
GLOBALDEF char  *IIarDump = NULL;       /* Name of keystroke output file (-I) */
/*
**	abrtcall.qsc
*/
GLOBALDEF bool   IIParamPassing = FALSE;

/*
** ABF RunTime Frame and Procedure Call Stack.
*/
GLOBALDEF ABRTSSTK      *IIarRtsStack = NULL;

/*
**	abrthelp.qc
*/

GLOBALDEF LOCATION      iiARmsgDir;

/*
**	abrtrw.qsc
*/

GLOBALDEF const char IIAR_Printer[]  = ERx("printer");
GLOBALDEF const char IIAR_Terminal[] = ERx("terminal");

/*
**	ildata.c
*/

GLOBALDEF   PTR    IIarRtsprm = NULL;

/*
**	rtsdata.c
*/

/*
**      The names below (IIarBatch and IIarNoDB) are only usable by the abfrt
**      directory.  No other directory requires access to them.
*/
GLOBALDEF bool          IIarBatch = FALSE;
GLOBALDEF bool          IIarNoDB  = FALSE;
GLOBALDEF bool          IIarUserApp  = FALSE;
/*
**      The names below (IIarDbname and IIarStatus) are only usable by
**      the abfrt directory.  Access from outside the abfrt directory
**      (such as abf and abfimage) must be accomplished by handles    
**      obtained by calling IIARgadGetArDbname() and IIargasGetArStatus().
*/
 
/*
** The database
*/
GLOBALDEF char          *IIarDbname = NULL;
  
/*
**  ABF runtime status
*/
GLOBALDEF STATUS        IIarStatus = OK;        /* exit status */



