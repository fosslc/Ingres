/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** WIN32S stuff
**
**  History:
**
**	30-aug-94 (fraser)
**	    Added definitions for Oracle OCI functions.
**	    Replaced defines with enum types to make maintenance easier.
**	31-aug-94 (fraser)
**	    Changed names associated with callback functions because it
**	    was confusing to see names of the form LIBQ_... associated
**	    with functions not in LIBQ; also changed MAXLIBQFUNC to
**	    MAX_CALLBK_FUNC.
**	    Added names for the two SQL Server callbacks.
**	8-sep-94 (fraser)
**	    Added names for the SQL Server dblib functions that we use.
**	8-sep-94 (fraser)
**	    Added names for dberrhandle and dbmsghandle.
**	21-sep-94 (fraser)
**	    Changed names for dberrhandle and dbmsghandle to point to
**	    cover routines IIQBdberrhandle and IIQBdbmsghandle.
**	23-sep-94 (fraser)
**	    Added names for the SQL Server functions dbdead and dbsetlname.
**	03-nov-94 (leighb)
**		Added IIgetpfnUTProc() prototype.
**	14-nov-94 (leighb)
**		Added IICB_win32s_hwnd.
**	14-nov-94 (leighb)
**		Added WIN32S_FREELIB.
**	22-nov-94 (fraser)
**		Added WIN32S_OR_IIQBTRA
**	23-nov-94 (fraser)
**		Added WIN32S_OR_IIQBGRI
**	27-nov-94 (fraser)
**		Added WIN32S_SS_DBEXIT
**	27-nov-94 (fraser)
**		Added WIN32S_SS_DBWINEXIT
**	28-nov-94 (leighb)
**		Added IICB_IIW4GL_SendUserEvent, IICB_IIW4GL_Open and IICB_IIW4GL_Close
**	09-dec-94 (leighb)
**		Defined UTloadIIw32sCB to a no-op.
**	09-jan-95 (leighb)
**		Added extern for hTask32bit.
**	10-jan-95 (leighb)
**		Added WIN32S_UTTERM; cleanup ifdef wgl_16s.
**	21-feb-95 (leighb)
**		Added WIN32S_SETWINDOWSHOOK & WIN32S_UNHOOKWINDOWSHOOK.
**	09-mar-95 (leighb)
**		Added WIN32S_XLATE32TO16 & WIN32S_XLATE16TO32.
**	7-may-95 (fraser)
**		Added definitions for the SQLBase API functions.
**	7-may-95 (fraser)
**		Removed definitions for SQLBase functions that we
**		won't use.
**	8-may-95 (fraser)
**		Removed functions which have prototypes in sql.h, but
**		are not documented in the SQLBase API Reference manual.
**		Execpt for the functions sqldii and sqlnii, which I am
**		retaining, the functions I am eliminating are (according
**		to Kent Laux) either obsolete or for internal Gupta use.
**	9-may-95 (fraser)
**		Removed WIN32S_SB_SQLSCL, WIN32S_SB_SQLSDN.
**	23-may-95 (fraser)
**		Added WIN32S_SB_IIQBCNC and WIN32S_SB_IIQBDIS.
**	2-jun-95 (fraser)
**		Added WIN32S_SB_IIQBCUP.
**	26-jun-95 (fraser)
**		Added WIN32S_SB_SQLXDP.
**	13-jul-95 (fraser)
**		Added WIN32S_SB_IIQBTRA.
**	21-feb-97 (mcgem01)
**		Cleaned up compile problem caused by the removal of
**		w32sut.h
**	09-apr-2007 (drivi01)
**	    	BUG 117501
**	    	Adding references to function IIcsRetScroll in order to fix
**	    	build on windows.
*/


#ifdef		wgl_16s
extern		HTASK	hTask32bit;	/* task handle of 32-bit task using this DLL */
/* comment out the following line to turn on the UTloadIIw32sCB() function */
# define		UTloadIIw32sCB()
# ifndef		UTloadIIw32sCB
long		UTloadIIw32sCB(void);
# endif
#else
typedef DWORD  ( WINAPI * UT32PROC)( LPVOID lpBuff,
                                     DWORD  dwUserDefined,
                                     LPVOID *lpTranslationList
                                   );
UT32PROC	IIgetpfnUTProc(void);
#endif

enum thunk_function
{
	WIN32S_OLPCALL,
	WIN32S_LOADLIB,
	WIN32S_GETPROCADDR,
	WIN32S_PCCMDLINE,
	WIN32S_PCSLEEP,
	WIN32S_CREATEEDITCONTROL,
	WIN32S_GLOBALFREE,
	WIN32S_FREELIB,
	WIN32S_UTTERM,
	WIN32S_SETWINDOWSHOOK,
	WIN32S_UNHOOKWINDOWSHOOK,
	WIN32S_XLATE32TO16,
	WIN32S_XLATE16TO32,

	/* Oracle OCI functions */

	WIN32S_OR_OBNDRA,
	WIN32S_OR_OBNDRN,
	WIN32S_OR_OBNDRV,
	WIN32S_OR_OBREAK,
	WIN32S_OR_OCAN,
	WIN32S_OR_OCLOSE,
	WIN32S_OR_OCOF,
	WIN32S_OR_OCOM,
	WIN32S_OR_OCON,
	WIN32S_OR_ODEFIN,
	WIN32S_OR_ODESSP,
	WIN32S_OR_ODESCR,
	WIN32S_OR_OERHMS,
	WIN32S_OR_OEXEC,
	WIN32S_OR_OEXFET,
	WIN32S_OR_OEXN,
	WIN32S_OR_OFEN,
	WIN32S_OR_OFETCH,
	WIN32S_OR_OFLNG,
	WIN32S_OR_OLOGOF,
	WIN32S_OR_OOPEN,
	WIN32S_OR_OOPT,
	WIN32S_OR_OPARSE,
	WIN32S_OR_ORLON,
	WIN32S_OR_OROL,
	WIN32S_OR_IIQBTRA,
	WIN32S_OR_IIQBGRI,

	/* SQL Server dblib functions */

	WIN32S_SS_DBBIND,
	WIN32S_SS_DBCANCEL,
	WIN32S_SS_DBCANQUERY,
	WIN32S_SS_DBCLOSE,
	WIN32S_SS_DBCMD,
	WIN32S_SS_DBCOLLEN,
	WIN32S_SS_DBCOLNAME,
	WIN32S_SS_DBCOLTYPE,
	WIN32S_SS_DBCOUNT,
	WIN32S_SS_DBCURSOR,
	WIN32S_SS_DBCURSORBIND,
	WIN32S_SS_DBCURSORCLOSE,
	WIN32S_SS_DBCURSORCOLINFO,
	WIN32S_SS_DBCURSORFETCH,
	WIN32S_SS_DBCURSORINFO,
	WIN32S_SS_DBCURSOROPEN,
	WIN32S_SS_DBDATAREADY,
	WIN32S_SS_DBDATECRACK,
	WIN32S_SS_DBDATLEN,
	WIN32S_SS_DBDEAD,
	WIN32S_SS_DBEXIT,
	WIN32S_SS_DBFCMD,
	WIN32S_SS_DBFREEBUF,
	WIN32S_SS_DBGETUSERDATA,
	WIN32S_SS_DBHASRETSTAT,
	WIN32S_SS_DBINIT,
	WIN32S_SS_DBLOGIN,
	WIN32S_SS_DBNEXTROW,
	WIN32S_SS_DBNULLBIND,
	WIN32S_SS_DBNUMCOLS,
	WIN32S_SS_DBOPEN,
	WIN32S_SS_DBRESULTS,
	WIN32S_SS_DBRETSTATUS,
	WIN32S_SS_DBSETLNAME,
	WIN32S_SS_DBSETLOGINTIME,
	WIN32S_SS_DBSETUSERDATA,
	WIN32S_SS_DBSQLEXEC,
	WIN32S_SS_DBSTRCPY,
	WIN32S_SS_DBSTRLEN,
	WIN32S_SS_DBUSE,
	WIN32S_SS_DBLOCKLIB,
	WIN32S_SS_DBUNLOCKLIB,
	WIN32S_SS_DBWINEXIT,
	WIN32S_SS_IIQB_DBERRHANDLE,
	WIN32S_SS_IIQB_DBMSGHANDLE,

	/* SQLBase API functions */

	WIN32S_SB_SQLBLD,
	WIN32S_SB_SQLBLN,
	WIN32S_SB_SQLBND,
	WIN32S_SB_SQLBNN,
	WIN32S_SB_SQLCBV,
	WIN32S_SB_SQLCEX,
	WIN32S_SB_SQLCMT,
	WIN32S_SB_SQLCNC,
	WIN32S_SB_SQLCNR,
	WIN32S_SB_SQLCOM,
	WIN32S_SB_SQLCRS,
	WIN32S_SB_SQLCTY,
	WIN32S_SB_SQLDBN,
	WIN32S_SB_SQLDES,
	WIN32S_SB_SQLDII,
	WIN32S_SB_SQLDIR,
	WIN32S_SB_SQLDIS,
	WIN32S_SB_SQLDON,
	WIN32S_SB_SQLDRS,
	WIN32S_SB_SQLDSC,
	WIN32S_SB_SQLDST,
	WIN32S_SB_SQLELO,
	WIN32S_SB_SQLEPO,
	WIN32S_SB_SQLERR,
	WIN32S_SB_SQLETX,
	WIN32S_SB_SQLEXE,
	WIN32S_SB_SQLFER,
	WIN32S_SB_SQLFET,
	WIN32S_SB_SQLFQN,
	WIN32S_SB_SQLGDI,
	WIN32S_SB_SQLGET,
	WIN32S_SB_SQLGFI,
	WIN32S_SB_SQLGLS,
	WIN32S_SB_SQLIMS,
	WIN32S_SB_SQLINI,
	WIN32S_SB_SQLNBV,
	WIN32S_SB_SQLNII,
	WIN32S_SB_SQLNRR,
	WIN32S_SB_SQLNSI,
	WIN32S_SB_SQLOMS,
	WIN32S_SB_SQLPRS,
	WIN32S_SB_SQLRBF,
	WIN32S_SB_SQLRBK,
	WIN32S_SB_SQLRCD,
	WIN32S_SB_SQLRET,
	WIN32S_SB_SQLRLO,
	WIN32S_SB_SQLROW,
	WIN32S_SB_SQLRRS,
	WIN32S_SB_SQLSCN,
	WIN32S_SB_SQLSCP,
	WIN32S_SB_SQLSET,
	WIN32S_SB_SQLSIL,
	WIN32S_SB_SQLSPR,
	WIN32S_SB_SQLSRS,
	WIN32S_SB_SQLSSB,
	WIN32S_SB_SQLSTO,
	WIN32S_SB_SQLSTR,
	WIN32S_SB_SQLTEC,
	WIN32S_SB_SQLTEM,
	WIN32S_SB_SQLTIO,
	WIN32S_SB_SQLURS,
	WIN32S_SB_SQLWLO,
	WIN32S_SB_SQLXDP,
	WIN32S_SB_IIQBCNC,
	WIN32S_SB_IIQBDIS,
	WIN32S_SB_IIQBCUP,
	WIN32S_SB_IIQBTRA

};

/*
** Function numbers for callback functions
*/

enum thunk_callback
{
	/*
	**  IIG4 functions
	*/
	IICB_W4GL_IIG4acArrayClear,
	IICB_W4GL_IIG4bpByrefParam,
	IICB_W4GL_IIG4ccCallComp,
	IICB_W4GL_IIG4chkobj,
	IICB_W4GL_IIG4drDelRow,
	IICB_W4GL_IIG4fdFillDscr,
	IICB_W4GL_IIG4gaGetAttr,
	IICB_W4GL_IIG4ggGetGlobal,
	IICB_W4GL_IIG4grGetRow,
	IICB_W4GL_IIG4i4Inq4GL,
	IICB_W4GL_IIG4icInitCall,
	IICB_W4GL_IIG4irInsRow,
	IICB_W4GL_IIG4rrRemRow,
	IICB_W4GL_IIG4rvRetVal,
	IICB_W4GL_IIG4s4Set4GL,
	IICB_W4GL_IIG4saSetAttr,
	IICB_W4GL_IIG4seSendEvent,
	IICB_W4GL_IIG4sgSetGlobal,
	IICB_W4GL_IIG4srSetRow,
	IICB_W4GL_IIG4udUseDscr,
	IICB_W4GL_IIG4vpValParam,

	/*
	** External User Event Functions
	*/
	IICB_IIW4GL_SendUserEvent,
	IICB_IIW4GL_Open,
	IICB_IIW4GL_Close,

	/*
	**  SQL Server callbacks
	*/
	IICB_SS_err_handler,
	IICB_SS_msg_handler,

	/*
	**  LIBQ functions
	*/
	IICB_LIBQ_IILQdbl,
	IICB_LIBQ_IILQesEvStat,
	IICB_LIBQ_IILQint,
	IICB_LIBQ_IILQisInqSqlio,
	IICB_LIBQ_IILQpriProcInit,
	IICB_LIBQ_IILQprsProcStatus,
	IICB_LIBQ_IILQprvProcValio,
	IICB_LIBQ_IILQshSetHandler,
	IICB_LIBQ_IILQsidSessID,
	IICB_LIBQ_IILQssSetSqlio,
	IICB_LIBQ_IIbreak,
	IICB_LIBQ_IIcsClose,
	IICB_LIBQ_IIcsDaGet,
	IICB_LIBQ_IIcsDelete,
	IICB_LIBQ_IIcsERetrieve,
	IICB_LIBQ_IIcsRetScroll,
	IICB_LIBQ_IIcsERplace,
	IICB_LIBQ_IIcsGetio,
	IICB_LIBQ_IIcsOpen,
	IICB_LIBQ_IIcsQuery,
	IICB_LIBQ_IIcsRdO,
	IICB_LIBQ_IIcsReplace,
	IICB_LIBQ_IIcsRetrieve,
	IICB_LIBQ_IIeqiqio,
	IICB_LIBQ_IIeqstio,
	IICB_LIBQ_IIerrtest,
	IICB_LIBQ_IIexDefine,
	IICB_LIBQ_IIexExec,
	IICB_LIBQ_IIexit,
	IICB_LIBQ_IIflush,
	IICB_LIBQ_IIgetdomio,
	IICB_LIBQ_IIingopen,
	IICB_LIBQ_IInexec,
	IICB_LIBQ_IInextget,
	IICB_LIBQ_IIparret,
	IICB_LIBQ_IIparset,
	IICB_LIBQ_IIputctrl,
	IICB_LIBQ_IIputdomio,
	IICB_LIBQ_IIretinit,
	IICB_LIBQ_IIsexec,
	IICB_LIBQ_IIsqConnect,
	IICB_LIBQ_IIsqDaIn,
	IICB_LIBQ_IIsqDescribe,
	IICB_LIBQ_IIsqDisconnect,
	IICB_LIBQ_IIsqExImmed,
	IICB_LIBQ_IIsqExStmt,
	IICB_LIBQ_IIsqFlush,
	IICB_LIBQ_IIsqInit,
	IICB_LIBQ_IIsqMods,
	IICB_LIBQ_IIsqParms,
	IICB_LIBQ_IIsqPrepare,
	IICB_LIBQ_IIsqPrint,
	IICB_LIBQ_IIsqStop,
	IICB_LIBQ_IIsqTPC,
	IICB_LIBQ_IIsqUser,
	IICB_LIBQ_IIsyncup,
	IICB_LIBQ_IItupget,
	IICB_LIBQ_IIutsys,
	IICB_LIBQ_IIvarstrio,
	IICB_LIBQ_IIwritio,
	IICB_LIBQ_IIxact,
	IICB_win32s_hwnd,
	
/*
**	VERY IMPORTANT!
**
**	MAX_CALLBK_FUNC must be the last item in this list.
*/

	MAX_CALLBK_FUNC
};
