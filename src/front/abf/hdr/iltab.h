/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	iltab.h -	Intermediate Language Operator Look-up Table.
**
** Description:
**	Contains the IL operator look-up table.  (Note three separate
**	versions of this file are compiled:  for the IL interpreter, the
**	OSL translator, and the code generator.)  Defines:
**
**	iltab		IL interpreter table.
**	ilttab		IL translator table.
**	ilcgtab		IL code generator table.
**
** History:
**	Revision 5.1  86/09/18  12:20:09  wong
**	Modified from "interp/ilkw.c".
**
**	Revsion 6.0  87/06  wong
**	Added SQL operators.  87/04  wong
**	Added old-style SET/INQUIRE FRS operators.  87/04  wong
**
**	Revision 6.3  90/01  wong
**	Added entries for IL_SAVPT_SQL for JupBug #4567 and IL_COPY_SQL for
**	JupBug #9734.  (Also, for compatability, all intervening operator
**	entries as well.)
**
**	13-feb-1990 (Joe)
**        Added IL_QID.  It has 3 operands:
**            IL_QID strvalQueryName intvalQueryId1 intvalQueryId2
**        The 3 operands are the 3 parts of a query identifier.
**
**	Revision 6.3/03/00  90/09  wong
**	Added IL_CREATEUSER through IL_ALTERTABLE.  Replaced STAR LINK operators
**	(which are unused in rel. 6) with IL_DBVSTR and IL_EXIMMEDIATE.
**
**	Revision 6.4
**	3/91 (Mike S)
**		Add LOADTABLE
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Add CALLPL, LOCALPROC, MAINPROC.
**	04/22/91 (emerson)
**		Modifications for alerters (added IL_ACTALERT and IL_DBSTMT).
**	05/03/91 (emerson)
**		Modifications for alerters: handle GET EVENT properly.
**		(It needs a special call to LIBQ and thus a special IL op code).
**	05/07/91 (emerson)
**		Added IL_NEXTPROC statement (for codegen).
**	07/26/91 (emerson)
**		Change EVENT to DBEVENT (per LRC 7-3-91).
**
**	Revision 6.4/02
**	11/07/91 (emerson)
**		Added ARRAYREF, RELEASEOBJ, and ARRENDUNLD
**		(for bugs 39581, 41013, and 41014).
**
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Added QRYROWGET, QRYROWCHK, and CLRREC (for bug 39582).
**
**	Revision 6.5
**	24-sep-92 (davel)
**		Added CONNECT and DISCONNECT (for multi-session support).
**	28-sep-92 (davel)
**		Fixed bug 42015: DIRECT CONNECT and DIRECT DISCONNECT are SQL
**		statements, and hence should use the corresponding SQL
**		interpreter and codegen routines.
**	10-nov-92 (davel)
**		Added CHKCONNECT to check new connection - used after SET_SQL
**		switches sessions.
**	09-feb-93 (davel)
**		Added functions for IL_SET4GL and IL_INQ4GL.
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**	3-jul-1996 (angusm)
**		Add function for IL_DGTT (bug 75153)
**	30-oct-98 (i4jo01)
**		Had to change the IL_INDEX entry because it had the 'on'
**		already coded in it which would not work for the 
**		unique qualifier (b94049).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Jan-2007 (kiria01) b117277
**	    Added hooks for SET RANDOM_SEED
**       4-Jan-2007 (hanal04) Bug 118967
**          IL_EXPR and IL_LEXPR now make use of spare operands in order
**          to store an i4 size and i4 offset.
*/

/*
** Procedure Declarations.
*/
	/*
	** Routines for OSL interpreter.
	*/
i4	IIITis3InqSet30(),
	IIITreResEntryExec(),
	IIITfoeFrsOptExec(),
	IIITmseModeStmtExec(),
	IIITsteScrollTableExec(),
	IIITsdeSqlDbExec(),
	IIITqdeQuelDbExec(),
	IIITsreSqlRepeatExec(),
	IIITqreQuelRepeatExec(),
	IIITaTimeoutActEx(),
	IIOasAssignExec(),
	IIITmBeginMenuEx(),
	IIITmdBegDispMenuEx(),
	IIITmsBegSubMenuEx(),
	IIITmSubMenuEx(),
	IIIT4gCallEx(),
	IIITsuCalSubSysEx(),
	IIITsyCalSysEx(),
	IIOcaClrallExec(),
	IIOclClrfldExec(),
	IIOcwClearrowExec(),
	IIOcsClrscrExec(),
	IIITaColActEx(),
	IIOdrDeleterowExec(),
	IIITdisplayEx(),
	IIITdlDispLpEx(),
	IIOduDumpExec(),
	IIOdaDycAllocExec(),
	IIOdtDycCatExec(),
	IIOdfDycFreeExec(),
	IIITmEndMenuEx(),
	IIOeuEndUnldExec(),
	IIITeofEx(),
	IIITexitEx(),
	IIOepExprExec(),
	IIITaFldActEx(),
	IIOgeGenericExec(),
	IIOgfGetformExec(),
	IIOgrGetrowExec(),
	IIOgtGoToExec(),
	IIOhfHelpfileExec(),
	IIOhsHelpformsExec(),
	IIOieIfExec(),
	IIITinitializeEx(),
	IIOitInittableExec(),
	IIOifInqFormsExec(),
	IIITiqInqINGex(),
	IIITstINGsetEx(),
	IIOirInsertrowExec(),
	IIITaKeyActEx(),
	IIITaMenuItemEx(),
	IIITmlMenuLpEx(),
	IIOmeMessageExec(),
	IIOmcMovConstExec(),
	IIITprBuildPrmEx(),
	IIITmPopSubmuEx(),
	IIOpnPrintScrExec(),
	IIOpmPromptExec(),
	IIOpfPutformExec(),
	IIOprPutrowExec(),
	IIOryRedisplayExec(),
	IIOrcResColumnExec(),
	IIOrdResFieldExec(),
	IIOrmResMenuExec(),
	IIOrnResNextExec(),
	IIOruResumeExec(),
	IIObqBuildQryExec(),
	IIOqbQryBrkExec(),
	IIOqcQryChildExec(),
	IIOqfQryFreeExec(),
	IIOqgQryGenExec(),
	IIITreturnEx(),
	IIOqnQryNextExec(),
	IIOqsQrySingleExec(),
	IIOsfSetFormsExec(),
	IIITstmtHdrEx(),
	IIOspSleepExec(),
	IIITstopEx(),
	IIOu1Unloadtbl1Exec(),
	IIOu2Unloadtbl2Exec(),
	IIOvfValFieldExec(),
	IIOvaValidateExec(),
	IIOvrValidrowExec(),
	IIITeColEntEx(),
	IIITeFldEntEx(),
	IIITdotDotExec(),
	IIITarrArrayExec(),
	IIOcawClearArrRowExec(),
	IIOdarDeleteArrRowExec(),
	IIOiarInsertArrRowExec(),
	IIOua1UnloadArr1Exec(),
	IIOua2UnloadArr2Exec(),
	IIITltfLoadTFExec(),
	IIITlpLocalProc(),
	IIITmpMainProc(),
	IIITaAlertActEx(),
	IIITgeeGetEventExec(),
	IIITroeReleaseObjExec(),
	IIOuaeUnloadArrEndExec(),
	IIOqrQryRowchkExec(),
	IIOcrClearRecExec(),
	IIOspSetRndExec(),
	IIITconDBConnect(),
	IIITdscDBDisconnect(),
	IIITchsCheckSession(),
	IIITs4gSet4GL(),
	IIITi4gInquire4GL(),
	IIITrnfResNxtFld(),
	IIITrpfResPrvFld(),
	IIITptPurgeTbl(),
	/*
	** Routines for code generator.
	*/
	IICGrseResEntryGen(),
	IICGasnAssignGen(),
	IICGaColAct(),
	IICGaFldAct(),
	IICGaKeyAct(),
	IICGaMenuItem(),
	IICGaTimeoutAct(),
	IICG4glCallGen(),
	IICGsuCalSubSys(),
	IICGsyCalSys(),
	IICGclaClrallGen(),
	IICGclfClrfldGen(),
	IICGcrwClearrowGen(),
	IICGclsClrscrGen(),
	IICGdbcDbconstGen(),
	IICGdbsDbstmtGen(),
	IICGdbrDbRepeatGen(),
	IICGdblDbvalGen(),
	IICGdbtDbvstrGen(),
	IICGdbvDbvarGen(),
	IICGdrwDeleterowGen(),
	IICGdisplay(),
	IICGdlDispLoop(),
	IICGdyaDycAllocGen(),
	IICGdyfDycFreeGen(),
	IICGeudEndUnldGen(),
	IICGeof(),
	IICGexitGen(),
	IICGexpExprGen(),
	IICGfopFrsOptGen(),
	IICGnullOp(),
	IICGgtfGetformGen(),
	IICGgrwGetrowGen(),
	IICGgotGotoGen(),
	IICGhflHelpfileGen(),
	IICGhfsHelpformsGen(),
	IICGifgIfGen(),
	IICGinitialize(),
	IICGitbInittableGen(),
	IICGiqfInqFormsGen(),
	IICGif3InqFrs30Gen(),
	IICGmBeginMenu(),
	IICGmEndMenu(),
	IICGmodeGen(),
	IICGmPopSubmu(),
	IICGmdBegDispMenu(),
	IICGmsBegSubMenu(),
	IICGmSubMenu(),
	IICGiqInqINGgen(),
	IICGstINGsetGen(),
	IICGirwInsertrowGen(),
	IICGlstmtHdr(),
	IICGmsgMessageGen(),
	IICGmlpMenuLoop(),
	IICGmvcMovconstGen(),
	IICGprBuildPrm(),
	IICGprsPrintscrGen(),
	IICGprmPromptGen(),
	IICGptfPutformGen(),
	IICGprwPutrowGen(),
	IICGqrbQryBrkGen(),
	IICGqrcQryChildGen(),
	IICGqrfQryFreeGen(),
	IICGqrgQrygenGen(),
	IICGqrnQryNextGen(),
	IICGqrsQrySingleGen(),
	IICGqryBuildQryGen(),
	IICGrdyRedisplayGen(),
	IICGrscResColumnGen(),
	IICGrsfResFieldGen(),
	IICGrsmResMenuGen(),
	IICGrsnResNextGen(),
	IICGrsuResumeGen(),
	IICGreturnGen(),
	IICGscrollGen(),
	IICGsdbSqldbGen(),
	IICGsdrSqldbRepeatGen(),
	IICGsf3SetFrs30Gen(),
	IICGslpSleepGen(),
	IICGstfSetFormsGen(),
	IICGstmtHdr(),
	IICGul1Unldtbl1Gen(),
	IICGul2Unldtbl2Gen(),
	IICGvldValidateGen(),
	IICGvlfValFieldGen(),
	IICGvrwValidrowGen(),
	IICGeColEnt(),
	IICGdotDotGen(),
	IICGarrArrayGen(),
	IICGcarClearArrRowGen(),
	IICGdarDeleteArrRowGen(),
	IICGiarInsertArrRowGen(),
	IICGua1UnldArr1Gen(),
	IICGua2UnldArr2Gen(),
	IICGltfLoadTFGen(),
	IICGpgProcGen(),
	IICGaAlertAct(),
	IICGgegGetEventGen(),
	IICGrogReleaseObjGen(),
	IICGuaeUnldArrEndGen(),
	IICGqrrQryRowchkGen(),
	IICGcrClearRecGen(),
	IICGslpSetRndGen(),
	IICGeFldEnt(),
	IICGconDBConnect(),
	IICGdscDBDisconnect(),
	IICGchsCheckSession(),
	IICGs4gSet4GL(),
	IICGi4gInquire4GL(),
	IICGrnfResNxtFld(),
	IICGrpfResPrvFld(),
	IICGptPurgeTbl();

/*{
** Name:	IIILtab -	Intermediate Language Operator Look-Up Table.
**
** Description:
**	The IL operator look-up table.
**
*/
GLOBALDEF IL_OPTAB	IIILtab[] = 
{
    IL_EOF,	"eof",		1,	il_func(IIITeofEx, IICGeof),
    IL_ABORT,	"abort",	1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_APPEND,	"append",	2,	il_func(IIITqreQuelRepeatExec, IICGdbrDbRepeatGen),
    IL_ASSIGN,	"assign",	4,	il_func(IIOasAssignExec, IICGasnAssignGen),
    IL_BEGLIST,	"beginlist",	1,	il_func(IIOgeGenericExec, IICGnullOp),
    IL_BEGMENU,	"beginmenu",	2,	il_func(IIITmBeginMenuEx, IICGmBeginMenu),
    IL_BEGDISPMU,"begin display menu",2,il_func(IIITmdBegDispMenuEx, IICGmdBegDispMenu),
    IL_BEGTRANS,"begin transaction", 1, il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_CALLF,	"callframe",	2,	il_func(IIIT4gCallEx, IICG4glCallGen),
    IL_CALLP,	"callproc",	2,	il_func(IIIT4gCallEx, IICG4glCallGen),
    IL_CALSUBSYS,"call subsys",	4,	il_func(IIITsuCalSubSysEx,IICGsuCalSubSys),
    IL_CALSYS,	"call system",	2,	il_func(IIITsyCalSysEx, IICGsyCalSys),
    IL_DBVAL,	"dbval",	2,	il_func(IIOgeGenericExec, IICGdblDbvalGen),
    IL_CLRALL,	"clrall",	1,	il_func(IIOcaClrallExec, IICGclaClrallGen),
    IL_CLRFLD,	"clrfld",	2,	il_func(IIOclClrfldExec, IICGclfClrfldGen),
    IL_CLRROW,	"clrrow",	3,	il_func(IIOcwClearrowExec, IICGcrwClearrowGen),
    IL_CLRSCR,	"clrscr",	1,	il_func(IIOcsClrscrExec, IICGclsClrscrGen),
    IL_COLACT,	"colact",	4,	il_func(IIITaColActEx, IICGaColAct),
    IL_COPY,	"copy",		1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_CREATE,	"create",	1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_DBCONST,	"dbconst",	2,	il_func(IIOgeGenericExec, IICGdbcDbconstGen),
    IL_DBVAR,	"dbvar",	2,	il_func(IIOgeGenericExec, IICGdbvDbvarGen),
    IL_DELETE,	"delete",	2,	il_func(IIITqreQuelRepeatExec, IICGdbrDbRepeatGen),
    IL_DELROW,	"delrow",	3,	il_func(IIOdrDeleterowExec, IICGdrwDeleterowGen),
    IL_DESINTEG,"destroy integrity", 1, il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_DESPERM,	"destroy permit", 1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_DESTROY,	"destroy",	1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_DISPLAY,	"display",	2,	il_func(IIITdisplayEx, IICGdisplay),
    IL_DISPLOOP,"disploop",	1,	il_func(IIITdlDispLpEx, IICGdlDispLoop),
    IL_DUMP,	"dump",		3,	il_func(IIOduDumpExec, IICGnullOp),
    IL_DYCALLOC,"dycalloc",	2,	il_func(IIOdaDycAllocExec, IICGdyaDycAllocGen),
    IL_DYCCAT,	"dyccat",	3,	il_func(IIOgeGenericExec, IICGnullOp),
    IL_DYCFREE,	"dycfree",	2,	il_func(IIOdfDycFreeExec, IICGdyfDycFreeGen),
    IL_ENDLIST,	"endlist",	1,	il_func(IIOgeGenericExec, IICGnullOp),
    IL_ENDMENU,	"endmenu",	2,	il_func(IIITmEndMenuEx, IICGmEndMenu),
    IL_ENDTRANS,"end transaction", 1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_ENDUNLD,	"endunld",	1,	il_func(IIOeuEndUnldExec, IICGeudEndUnldGen),
    IL_EXIT,	"exit",		1,	il_func(IIITexitEx, IICGexitGen),
    IL_EXPR,	"expr",		5,	il_func(IIOepExprExec, IICGexpExprGen),
    IL_FLDACT,	"fldact",	3,	il_func(IIITaFldActEx, IICGaFldAct),
    IL_FLDENT,	"fldent",	3,	il_func(IIITeFldEntEx, IICGeFldEnt),
    IL_GETFORM,	"getform",	3,	il_func(IIOgfGetformExec, IICGgtfGetformGen),
    IL_GETROW,	"getrow",	3,	il_func(IIOgrGetrowExec, IICGgrwGetrowGen),
    IL_GOTO,	"goto",		2,	il_func(IIOgtGoToExec, IICGgotGotoGen),
    IL_OINQFRS,	"oinqfrs",	5,	il_func(IIITis3InqSet30, IICGif3InqFrs30Gen),
    IL_OSETFRS,	"osetfrs",	5,	il_func(IIITis3InqSet30, IICGsf3SetFrs30Gen),
    IL_COLENT, "colent",	4,	il_func(IIITeColEntEx, IICGeColEnt),
    IL_MODE,	"mode",		2,	il_func(IIITmseModeStmtExec, IICGmodeGen),
    IL_HLPFILE,	"helpfile",	3,	il_func(IIOhfHelpfileExec, IICGhflHelpfileGen),
    IL_HLPFORMS,"helpforms",	3,	il_func(IIOhsHelpformsExec, IICGhfsHelpformsGen),
    IL_IF,	"if",		3,	il_func(IIOieIfExec, IICGifgIfGen),
    IL_INDEX,	"index",	1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_INITIALIZE,"initialize",	1,	il_func(IIITinitializeEx,IICGinitialize),
    IL_INITTAB,	"inittable",	3,	il_func(IIOitInittableExec, IICGitbInittableGen),
    IL_INQELM,	"inqelm",	5,	il_func(IIOgeGenericExec, IICGnullOp),
    IL_INQFRS,	"inqfrs",	5,	il_func(IIOifInqFormsExec, IICGiqfInqFormsGen),
    IL_INQING,	"inqing",	1,	il_func(IIITiqInqINGex, IICGiqInqINGgen),
    IL_INSROW,	"insertrow",	3,	il_func(IIOirInsertrowExec, IICGirwInsertrowGen),
    IL_TL3ELM,	"tl3elm",	4,	il_func(IIOgeGenericExec, IICGnullOp),
    IL_INTEGRITY,"define integrity", 1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_KEYACT,	"keyact",	6,	il_func(IIITaKeyActEx, IICGaKeyAct),
    IL_LEXPR,	"lexpr",	6,	il_func(IIOepExprExec, IICGexpExprGen),
    IL_MENUITEM,"menuitem",	6,	il_func(IIITaMenuItemEx, IICGaMenuItem),
    IL_MENULOOP,"menuloop",	1,	il_func(IIITmlMenuLpEx,IICGmlpMenuLoop),
    IL_MESSAGE,	"message",	2,	il_func(IIOmeMessageExec, IICGmsgMessageGen),
    IL_MODIFY,	"modify",	1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_MOVCONST,"movconst",	3,	il_func(IIOmcMovConstExec, IICGmvcMovconstGen),
    IL_NEPROMPT,"neprompt",	3,	il_func(IIOpmPromptExec, IICGprmPromptGen),
    IL_PERMIT,	"define permit", 1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_POPSUBMU,"popsubmu",	1,	il_func(IIITmPopSubmuEx, IICGmPopSubmu),
    IL_LSTHD,	"lsthd",	3,	il_func(IIITstmtHdrEx, IICGlstmtHdr),
    IL_PARAM,	"param",	6,	il_func(IIITprBuildPrmEx,IICGprBuildPrm),
    IL_PRNTSCR,	"printscreen",	2,	il_func(IIOpnPrintScrExec, IICGprsPrintscrGen),
    IL_PROMPT,	"prompt",	3,	il_func(IIOpmPromptExec, IICGprmPromptGen),
    IL_PUTFORM,	"putform",	3,	il_func(IIOpfPutformExec, IICGptfPutformGen),
    IL_PUTROW,	"putrow",	3,	il_func(IIOprPutrowExec, IICGprwPutrowGen),
    IL_PVELM,	"pvelm",	3,	il_func(IIOgeGenericExec, IICGnullOp),
    IL_QRY,	"qry",		4,	il_func(IIObqBuildQryExec, IICGqryBuildQryGen),
    IL_QRYBRK,	"qrybrk",	2,	il_func(IIOqbQryBrkExec, IICGqrbQryBrkGen),
    IL_QRYCHILD,"qrychild",	3,	il_func(IIOqcQryChildExec, IICGqrcQryChildGen),
    IL_QRYEND,	"qryend",	4,	il_func(IIOgeGenericExec, IICGnullOp),
    IL_QRYFREE,	"qryfree",	2,	il_func(IIOqfQryFreeExec, IICGqrfQryFreeGen),
    IL_QRYGEN,	"qrygen",	4,	il_func(IIOqgQryGenExec, IICGqrgQrygenGen),
    IL_QRYNEXT,	"qrynext",	4,	il_func(IIOqnQryNextExec, IICGqrnQryNextGen),
    IL_QRYPRED,	"qrypred",	3,	il_func(IIOgeGenericExec, IICGnullOp),
    IL_QRYSINGLE,"qrysingle",	3,	il_func(IIOqsQrySingleExec, IICGqrsQrySingleGen),
    IL_QRYSZE,	"qrysze",	3,	il_func(IIOgeGenericExec, IICGnullOp),
    IL_FRSOPT,	"frsopt",	1,	il_func(IIITfoeFrsOptExec, IICGfopFrsOptGen),
    IL_RANGE,	"range",	1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_REDISPLAY,"redisplay",	1,	il_func(IIOryRedisplayExec, IICGrdyRedisplayGen),
    IL_RELOCATE,"relocate",	1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_REPLACE,	"replace",	2,	il_func(IIITqreQuelRepeatExec, IICGdbrDbRepeatGen),
    IL_RESCOL,	"rescol",	3,	il_func(IIOrcResColumnExec, IICGrscResColumnGen),
    IL_RESFLD,	"resfld",	2,	il_func(IIOrdResFieldExec, IICGrsfResFieldGen),
    IL_RESMENU,	"resmenu",	1,	il_func(IIOrmResMenuExec, IICGrsmResMenuGen),
    IL_RESNEXT,	"resnext",	1,	il_func(IIOrnResNextExec, IICGrsnResNextGen),
    IL_RESUME,	"resume",	1,	il_func(IIOruResumeExec, IICGrsuResumeGen),
    IL_RETFRM,	"retfrm",	2,	il_func(IIITreturnEx, IICGreturnGen),
    IL_RETINTO,	"retrieve into", 1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_RETPROC,	"retproc",	2,	il_func(IIITreturnEx, IICGreturnGen),
    IL_SAVE,	"save",		1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_SAVEPOINT,"savepoint",	1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_SET,	"set",		1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_SETFRS,	"setfrs",	5,	il_func(IIOsfSetFormsExec, IICGstfSetFormsGen),
    IL_SETSQL,	"set",		1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_SLEEP,	"sleep",	2,	il_func(IIOspSleepExec, IICGslpSleepGen),
    IL_START,	"start",	1,	il_func(IIITmpMainProc, IICGpgProcGen),
    IL_STHD,	"sthd",		2,	il_func(IIITstmtHdrEx, IICGstmtHdr),
    IL_STOP,	"stop",		1,	il_func(IIITstopEx, IICGnullOp),
    IL_TL1ELM,	"tl1elm",	2,	il_func(IIOgeGenericExec, IICGnullOp),
    IL_TL2ELM,	"tl2elm",	3,	il_func(IIOgeGenericExec, IICGnullOp),
    IL_UNLD1,	"unld1",	3,	il_func(IIOu1Unloadtbl1Exec, IICGul1Unldtbl1Gen),
    IL_UNLD2,	"unld2",	2,	il_func(IIOu2Unloadtbl2Exec, IICGul2Unldtbl2Gen),
    IL_VALFLD,	"valfld",	3,	il_func(IIOvfValFieldExec, IICGvlfValFieldGen),
    IL_VALIDATE,"validate",	2,	il_func(IIOvaValidateExec, IICGvldValidateGen),
    IL_VALROW,	"valrow",	4,	il_func(IIOvrValidrowExec, IICGvrwValidrowGen),
    IL_VIEW,	"define view",	1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_DBVSTR,	"dbvstr",	2,	il_func(IIOgeGenericExec, IICGdbtDbvstrGen),
    IL_EXIMMEDIATE, "execute immediate", 1, il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    0,	"unused",	0, 	NULL,
    0,	"unused",	0,	NULL,
    IL_DIREXIMM,"direct execute immediate", 1, il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    0,	"unused",	0,	NULL,
    IL_INSERT,	"insert into",	2,	il_func(IIITsreSqlRepeatExec, IICGsdrSqldbRepeatGen),
    IL_DELETEFRM,"delete from",	2,	il_func(IIITsreSqlRepeatExec, IICGsdrSqldbRepeatGen),
    IL_UPDATE,	"update",	2,	il_func(IIITsreSqlRepeatExec, IICGsdrSqldbRepeatGen),
    IL_COMMIT,	"commit",	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_ROLLBACK,"rollback",	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_DRPTABLE,"drop table",	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_DRPINDEX,"drop index",	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_DRPVIEW,	"drop view",	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_DROP,	"drop",		1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_DRPINTEG,"drop integrity on",1,il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_DRPPERMIT,"drop permit on",1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_CRTTABLE,"create table", 1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_CRTINDEX,"create index", 1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_CRTUINDEX,"create unique index",1,il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_CRTVIEW,	"create view",	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_CRTPERMIT,"create permit",1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_CRTINTEG,"create integrity on", 1,il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_GRANT,	"grant",	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_SQLMODIFY,"modify",	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),

    IL_SCROLL,	"scroll",	4,	il_func(IIITsteScrollTableExec, IICGscrollGen),

    /*
    ** These next four all use IICGsdbDbstmtGen, even though they are 
    ** legal in both SQL and QUEL.  This may be a problem sometime.
    */
    IL_REGISTER,"register", 	1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_REMOVE,	"remove", 	1,	il_func(IIITqdeQuelDbExec, IICGdbsDbstmtGen),
    IL_DIRCONN,	"direct connect", 1, 	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_DIRDIS,	"direct disconnect", 1, il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_ACTIMOUT,"actimeout",	3,	il_func(IIITaTimeoutActEx, IICGaTimeoutAct),
    IL_BEGSUBMU,"begin submenu",2,	il_func(IIITmsBegSubMenuEx, IICGmsBegSubMenu),
    IL_SUBMENU,	"submenu",	1,	il_func(IIITmSubMenuEx, IICGmSubMenu),
    IL_RESENTRY,"resentry",     1,	il_func(IIITreResEntryExec, IICGrseResEntryGen),
    IL_DOT,	"dot", 		4,	il_func(IIITdotDotExec, IICGdotDotGen),
    IL_ARRAY,	"array",	5,	il_func(IIITarrArrayExec, IICGarrArrayGen),
    IL_SETING,	"set ingres",	1,	il_func(IIITstINGsetEx, IICGstINGsetGen),
    IL_ALTERGROUP,"alter group",1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_ALTERROLE,"alter role",	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_CREATEGROUP,"create group",1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_CREATEROLE,"create role",1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_CREATERULE,"create rule",1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_DRPRULE,"drop rule",     1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_DRPPROC,"drop proc",     1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_DRPGROUP,"drop group",   1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_DRPROLE,"drop role",     1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_REVOKE,"revoke",		1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),

    IL_ARRCLRROW,"arrclrrow",	3,	il_func(IIOcawClearArrRowExec, IICGcarClearArrRowGen),
    IL_ARRDELROW,"arrdelrow",	3,	il_func(IIOdarDeleteArrRowExec, IICGdarDeleteArrRowGen),
    IL_ARRINSROW,"arrinsrow",	3,	il_func(IIOiarInsertArrRowExec, IICGiarInsertArrRowGen),
    IL_ARR1UNLD,"arr1unld",	4,	il_func(IIOua1UnloadArr1Exec, IICGua1UnldArr1Gen),
    IL_ARR2UNLD,"arr2unld",	3,	il_func(IIOua2UnloadArr2Exec, IICGua2UnldArr2Gen),
    IL_SAVPT_SQL,"savepoint",	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_COPY_SQL,"copy table",	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_QID,   "Query ID",	4,	NULL,

    IL_CREATEUSER,"create user", 1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_ALTERUSER,"alter user",	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_DRPUSER,"drop user",	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_ALTERTABLE,"alter table", 1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_LOADTABLE,"loadtable", 3,	il_func(IIITltfLoadTFExec, IICGltfLoadTFGen),
    IL_CALLPL,	"callproc local", 3,	il_func(IIIT4gCallEx, IICG4glCallGen),
    IL_LOCALPROC, "local proc",	9,	il_func(IIITlpLocalProc, IICGpgProcGen),
    IL_MAINPROC, "main proc",	5,	il_func(IIITmpMainProc, IICGpgProcGen),
    IL_ACTALERT,"actalert",	2,	il_func(IIITaAlertActEx, IICGaAlertAct),
    IL_DBSTMT,"db statement", 	1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_GETEVENT,"get dbevent", 	2,	il_func(IIITgeeGetEventExec, IICGgegGetEventGen),
    IL_NEXTPROC, "next proc",	2,	il_func(IIOgeGenericExec, IICGnullOp),
    IL_ARRAYREF, "arrayref",	4,	il_func(IIITarrArrayExec, IICGarrArrayGen),
    IL_RELEASEOBJ, "releaseobj", 2,	il_func(IIITroeReleaseObjExec, IICGrogReleaseObjGen),
    IL_ARRENDUNLD,"arrendunld",	2,	il_func(IIOuaeUnloadArrEndExec, IICGuaeUnldArrEndGen),
    IL_QRYROWGET,"qryrowget",	3,	il_func(IIOqsQrySingleExec, IICGqrsQrySingleGen),
    IL_QRYROWCHK,"qryrowchk",	3,	il_func(IIOqrQryRowchkExec, IICGqrrQryRowchkGen),
    IL_CLRREC	,"clrrec",	2,	il_func(IIOcrClearRecExec, IICGcrClearRecGen),
    IL_CONNECT, "connect",	2,	il_func(IIITconDBConnect, IICGconDBConnect),
    IL_DISCONNECT, "disconnect", 1,	il_func(IIITdscDBDisconnect, IICGdscDBDisconnect),
    IL_CHKCONNECT, "chkconnect", 1,	il_func(IIITchsCheckSession, IICGchsCheckSession),
    IL_SET4GL,	"set 4gl",	1,	il_func(IIITs4gSet4GL, IICGs4gSet4GL),
    IL_INQ4GL,	"inquire 4gl",	1,	il_func(IIITi4gInquire4GL, IICGi4gInquire4GL),
    IL_RESNFLD,	"resume nextfield", 1, il_func(IIITrnfResNxtFld, IICGrnfResNxtFld),
    IL_RESPFLD, "resume previousfield", 1, il_func(IIITrpfResPrvFld, IICGrpfResPrvFld),
    IL_PURGETBL, "purgetable", 3, il_func(IIITptPurgeTbl, IICGptPurgeTbl),

    IL_DGTT,	"declare global temporary table", 1,	il_func(IIITsdeSqlDbExec, IICGsdbSqldbGen),
    IL_SET_RANDOM, "set random_seed",2,	il_func(IIOspSetRndExec, IICGslpSetRndGen),
    0,		NULL,		0,	NULL
};
