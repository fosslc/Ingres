/*
**  Copyright (c) 2004 Ingres Corporation
**  All rights reserved.
*/

/*
** Name:    fe.h -	Front-End Utility Module Definitions.
**
** Description:
**	Symbols and type definitions needed to use the front-end utilities.
**
** History:
**	26-apr-1989 (mgw,eduardo) Added SEP definition for QA.
**	2/13/89 (tom) - added TAGID typedef and tag function decls.
**	Revision 6.0  87/07  wong
**	Added 'FEreqmem()', etc. declarations.
**	7/30/87 (peterk) - added nested include of erfe.h
**	4/14/87 (peterk) - incorporated fehdr.h
**	22-dec-1986 (peter)	Added FUNC_EXTERN definitions.
**
**	Revision 3.0  84/09/19  azad
**	Initial revision.
**
**	09/14/90 (dkh) - Put in definition of DATAVIEW for VMS so
**			 MACWORKSTATION stuff is enabled on VMS.
**	16-nov-90 (sandyd)
**		Activated DATAVIEW for UNIX now as well.
**	24-june-91 (blaise)
**		Added FE_VISIONVER, the current version of Vision.
**	30-May-92 (fredb)
**		Defined SEP for hp9_mpe, ifdef'd out DATAVIEW define.
**		There will be no MACWORKSTATION support on MPE/iX.
**	23-jul-1992 (rdrane)
**		Move the FE_RSLV_NAME typedef to ui.qsh so that ESQL
**		can access it directly (cf. FE_REL_SOURCE, etc.).
**		Modify declaration of FE_resolve() since it now returns a bool.
**		Add declaration of iiugReqMem().
**	31-jul-1992 (rdrane)
**		Modify declaration of FE_fullresolve() since it now returns a
**		bool.
**	1-oct-1992 (rdrane)
**		Add declaration of FExrt_tbl().  Protect this file from multiple
**		inclusions.
**	14-oct-1992 (rdrane)
**		Rename FExrt_tbl() to IIUIxrt_tbl().
** 	23-Feb-93 (mohan) 
**		Added tag to DATE structure.
**	12-mar-93 (fraser)
**		Changed tag name for DATE structure.
**	8-apr-1993 (rdrane)
**		Add IIUGgci_getcid() and its attendant data structure
**		definition (FE_GCI_IDENT).
**	14-jan-1994 (rdrane)
**		Add IIUIuea_unescapos() and IIUIea_escapos() declarations
**		and prototypes.
**	22-feb-1996 (lawst01)
**	   Windows 3.1 port changes - add some fcn protypes.
**	09-apr-1996 (chech02)
**	   Windows 3.1 port changes - add more function prototypes.
**	14-may-1996 (chech02)
**	   Windows 3.1 port changes - eliminate compiler warning messages.
**	23-sep-1996 (sarjo01)
**	   Bug 77944: added i4  arg to qroutalloc() proto.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-mar-2001 (somsa01)
**	    Added FE_FMTMAXCHARWIDTH.
**      03-May-2001 (fanra01)
**          Sir 104700
**          Add additional capabilities flag to IIMWiInit prototype.
**	04-Mar-2003 (hanje04)
**	    Add prototype for FEadfcb.
**	06-oct-2003 (somsa01)
**	    Added prototype for IIUFccConfirmChoice().
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	28-Mar-2005 (lakvi01)
**	    Added some more function prototypes.
**	06-Apr-2005 (lakvi01)
**	    Removed certain function prototypes that must be present in ug.h 
**	    and eqdef.h and not here. These were incorrectly added in my 
**	    previous change.
**	08-Apr-2005 (hanje04)
**	    Remove line break causing build errors on Linux/Unix
**	09-Apr-2005 (lakvi01)
**	    Moved some function declarations into NT_GENERIC and reverted back
**	    to old declarations since they were causing compilation problems on
** 	    non-windows compilers.
**	13-Apr-2005 (lakvi01) on behalf of (shaha03)
**	    Added prototypes for the function to avoid the compilation
**	    errors by ANSI-C/Native Compiler.
**  20-Sep-2005 (fanra01)
**      Removed conflicting function forward references and prototypes.
**	21-Aug-2009 (stephenb/kschendel) 121804
**	    Add some prototypes.
**	    Update some of the function declarations to fix gcc 4.3 problems.
**	1-Sep-2009 (bonro01)
**	    Last change for SIR 121804 caused compile problems on Windows
**	    because of incompatible prototypes for IIcsRetScroll()
**      03-Sep-2009 (frima01) 122490
**          Add more prototypes.
**      10-Sep-2009 (frima01) 122490
**          Add a couple more prototypes.
**      24-Nov-2009 (frima01) Bug 122490
**          Continued adding prototypes.
**	02-Dec-2009 (whiro01)
**	    Updated a couple of these prototypes because of
**	    conflicts with Windows headers.
**	24-Feb-2010 (frima01) Bug 122490
**	    Add function prototypes as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

#ifndef FE_HDR_INCLUDED
#define	FE_HDR_INCLUDED

#include	<erfe.h>

/*
** define SEP for QA testing on VMS, UNIX, and MPE/iX
*/
#ifdef VMS
#define SEP
#endif

#ifdef UNIX
#define SEP
#endif

#ifdef hp9_mpe
#define SEP
#endif

/*
**  Define DATAVIEW for MacWorkstation
*/
#ifndef hp9_mpe
# define	DATAVIEW
#endif

# define FE_NONUNIQUE	(-2)		/* search argument found > once */
# define FE_NOTEXIST	(-3)		/* search argument not found */

/*}
** Name:    FREELIST -	Front-End Node Allocation Free List Structure.
**
** Description:
**	Free list pointer structure for FE node allocation utilities.
**
** History:
**	19-feb-92 (leighb) DeskTop Porting Change:
**		added _FREELIST as tag for FREELIST struct.
*/

/* structure to avoid assigning pointer to integer - keeps lint happy */
struct fl_node
{
	struct fl_node *l_next;
};

typedef struct _FREELIST				 
{
	struct fl_node *l_free;	/* pointer to free list */
	i4 nd_size;		/* size of nodes on list */
	i2 l_tag;		/* tag associated with storage */
	i2 magic;		/* "magic number" for proper block format */
} FREELIST;

/*}
** Name:    TAGID -	typedef for a tag 
**
** Description:
**	UG functions FEgettag and others refer to and return
**	the TAGID typedef. Callers of these functions should
**	use this typedef in refering these items.
**
** History:
**	2/13/89  (tom) - taken from file ..ug/feloc.h 
*/

typedef i2 TAGID; 


/*
**	Constants Used by the FEchkenv routine.
*/

# define	FENV_TWRITE	0x0001		/* Temporary area writable */

/*
** Function prototypes.
*/

/* This section is to get gcc 4.3 off the ground, some should probably
** be redistributed to other .h's eventually (eg afe.h).
*/

FUNC_EXTERN i4     	isamatch();
FUNC_EXTERN i4     	getlockcat();
FUNC_EXTERN i4     	get_db_id();
FUNC_EXTERN i4     	trns_lnm_to_lktyp();
FUNC_EXTERN i4     	include();
FUNC_EXTERN i4     	quit();
FUNC_EXTERN i4     	IItmmonitor();
FUNC_EXTERN i4     	FScount();
#ifndef _INC_EH
FUNC_EXTERN VOID	terminate();
#endif
FUNC_EXTERN VOID	s_crack_cmd();
FUNC_EXTERN VOID	s_readin();
FUNC_EXTERN VOID	r_error();
FUNC_EXTERN VOID	s_copy_new();
FUNC_EXTERN VOID	s_del_old();
FUNC_EXTERN VOID	d_print();
FUNC_EXTERN VOID	st_nameinfo();
FUNC_EXTERN VOID	st_servinfo();
FUNC_EXTERN VOID	clean_up();
FUNC_EXTERN VOID	tr_dash_d();
FUNC_EXTERN VOID	srvsess();
FUNC_EXTERN VOID	refresh_status();
FUNC_EXTERN VOID	string_move();
FUNC_EXTERN VOID	FDrcvclose();
FUNC_EXTERN VOID	FDdmpcur();
FUNC_EXTERN VOID	FDdmpinit();
FUNC_EXTERN VOID	FDrcvalloc();
FUNC_EXTERN VOID	IIUFbot_Bottom();
FUNC_EXTERN VOID	qrprintf();
FUNC_EXTERN VOID	IIUFpan_Panic();
FUNC_EXTERN VOID	FEendforms();
FUNC_EXTERN VOID	IIUFtfrTblFldRegister();
FUNC_EXTERN VOID	IIUIck1stAutoCom();
FUNC_EXTERN VOID	IIUFdone();
FUNC_EXTERN VOID	fstestexit();
FUNC_EXTERN VOID	IIUIswc_SetWithClause();
FUNC_EXTERN VOID	macinit();
FUNC_EXTERN VOID	ipanic();
FUNC_EXTERN VOID	ITsetvl();
FUNC_EXTERN void	ip_forminit();
FUNC_EXTERN void	l_srvsort();
FUNC_EXTERN void	putprintf();
FUNC_EXTERN void	IIresync();
FUNC_EXTERN STATUS 	ip_chk_response_pkg();
FUNC_EXTERN STATUS 	my_help();
FUNC_EXTERN STATUS 	qrrelease();
FUNC_EXTERN STATUS 	FEtest();
FUNC_EXTERN STATUS 	FEhandler();
FUNC_EXTERN STATUS 	ITinit();
FUNC_EXTERN STATUS 	IIUIpsdPartialSessionData();
FUNC_EXTERN STATUS 	dl_delobj();
FUNC_EXTERN STATUS 	IIUGhlpname();
FUNC_EXTERN VOID	FEcopyright();
FUNC_EXTERN i4     	vifred();
FUNC_EXTERN VOID	abdbgnterp ();
FUNC_EXTERN VOID	aberrlog ();
FUNC_EXTERN VOID	abexcexit ();
FUNC_EXTERN STATUS	abexecompile ();
FUNC_EXTERN VOID 	abproerr();
FUNC_EXTERN VOID	abusrerr();
FUNC_EXTERN DB_STATUS	adh_dbcvtev();
FUNC_EXTERN DB_STATUS	adh_dbtoev();
FUNC_EXTERN i4		adh_dounicoerce ();
FUNC_EXTERN DB_STATUS	adh_evcvtdb();
FUNC_EXTERN DB_STATUS	adh_evtodb();
FUNC_EXTERN void	adh_losegcvt();
FUNC_EXTERN void	adjOverPos();
FUNC_EXTERN void	adjPosVert();
FUNC_EXTERN void	adjustPos();
FUNC_EXTERN DB_STATUS	afe_agfind();
FUNC_EXTERN STATUS	afe_clinstd();
FUNC_EXTERN DB_STATUS	afe_cvinto ();
FUNC_EXTERN STATUS	afe_dec_info();
FUNC_EXTERN DB_STATUS	afe_dplen();
FUNC_EXTERN DB_STATUS	afe_dpvalue();
FUNC_EXTERN STATUS	afe_dtrimlen();
FUNC_EXTERN DB_STATUS	afe_error ();
FUNC_EXTERN DB_STATUS	afe_errtostr ();
FUNC_EXTERN DB_STATUS	afe_fdesc ();
FUNC_EXTERN DB_STATUS	afe_ficoerce ();
FUNC_EXTERN DB_STATUS	afe_fitype ();
FUNC_EXTERN STATUS	afe_numaggs();
FUNC_EXTERN STATUS	afe_opid ();
FUNC_EXTERN DB_STATUS	afe_sqltrans ();
FUNC_EXTERN DB_STATUS	afe_tychk ( );
FUNC_EXTERN DB_STATUS	afe_tyoutput ();
FUNC_EXTERN DB_STATUS	afe_vtyoutput();
FUNC_EXTERN void	allocLast();
FUNC_EXTERN void	attrCom();
FUNC_EXTERN void	branch();
FUNC_EXTERN int		branchto();
FUNC_EXTERN void	bufclear();
FUNC_EXTERN void	bufflush();
FUNC_EXTERN void	buffree();
FUNC_EXTERN int		bufget();
FUNC_EXTERN void	bufpurge();
FUNC_EXTERN void	bufput();
FUNC_EXTERN void	buildField();
FUNC_EXTERN void	cgprompt();
FUNC_EXTERN STATUS	chk_priv( char *user_name, char *priv_name );
FUNC_EXTERN void	clearLine();
FUNC_EXTERN void	clrline();
FUNC_EXTERN void	copyappl();
FUNC_EXTERN void	IIbreak();
FUNC_EXTERN void	IIputdomio();
FUNC_EXTERN VOID	FEclr_exit();
FUNC_EXTERN VOID	FEset_exit();
FUNC_EXTERN DB_STATUS	IIUGlocal_decimal();
FUNC_EXTERN VOID	insnLines(i4 y, i4 numlines, bool initialized);
FUNC_EXTERN VOID	SectionsUpdate();
FUNC_EXTERN STATUS	delnLines();
FUNC_EXTERN VOID	fixField();
FUNC_EXTERN VOID	putEdTitle();
FUNC_EXTERN i4		IIerrnum();
FUNC_EXTERN i4		IIretdom();
FUNC_EXTERN i4		(*IIseterr())();
FUNC_EXTERN i4		ITxout();
FUNC_EXTERN VOID	IIwritedb();
FUNC_EXTERN STATUS	IImumatch();
FUNC_EXTERN i4  	IIcompfrm();
FUNC_EXTERN i4  	IIpopfrm();
FUNC_EXTERN i4  	IIUGdaDiffArch();
FUNC_EXTERN VOID  	IIVTlf();
FUNC_EXTERN i4  	FDgtfldno();
FUNC_EXTERN VOID	FDvfrbfdmp();
FUNC_EXTERN VOID	FEdml();
FUNC_EXTERN DB_STATUS	FErel_ffetch();
FUNC_EXTERN VOID	afe_nullstring();
FUNC_EXTERN VOID	FEgetvm();
FUNC_EXTERN VOID	FEinames();
FUNC_EXTERN VOID	FEtnames();
FUNC_EXTERN STATUS	FTtermsize();
FUNC_EXTERN VOID	FTterminfo();
FUNC_EXTERN VOID	FTrestore();
FUNC_EXTERN VOID	FTbell();
FUNC_EXTERN i4		FTputmenu();
FUNC_EXTERN VOID	FTclrfrsk();
FUNC_EXTERN VOID	FTclear();
FUNC_EXTERN i4		FTclose();
FUNC_EXTERN VOID	VTgetstring();
FUNC_EXTERN VOID	VThilite();
FUNC_EXTERN VOID	VTaddcomm();
FUNC_EXTERN VOID	VTgetScrCurs();
FUNC_EXTERN VOID	VTflashplus();
FUNC_EXTERN VOID	VTKillPreview();
FUNC_EXTERN VOID	VTdumpset();
FUNC_EXTERN VOID	VTerase();
FUNC_EXTERN VOID	VTdelstring();
FUNC_EXTERN VOID	VTputstring();
FUNC_EXTERN VOID	VTundosave();
FUNC_EXTERN VOID	VTsvimage();
FUNC_EXTERN VOID	VTundores();
FUNC_EXTERN VOID	VTundoalloc();
FUNC_EXTERN VOID	VTputvstr();
FUNC_EXTERN VOID	VTresimage();
FUNC_EXTERN VOID	VTswapimage();
FUNC_EXTERN VOID	VTwider();
FUNC_EXTERN void	VTextend();
FUNC_EXTERN i4  	VTcursor();
FUNC_EXTERN i4  	mqgetwidth();
FUNC_EXTERN STATUS	fmt_vfvalid();
FUNC_EXTERN VOID	syserr();
FUNC_EXTERN VOID	savePos();
FUNC_EXTERN VOID	spcRbld();
FUNC_EXTERN VOID	sec_list_unlink();
FUNC_EXTERN VOID	sec_list_insert();
FUNC_EXTERN VOID	IIUIcnb_CatNolockBegin();
FUNC_EXTERN VOID	IIUIcne_CatNolockEnd();
FUNC_EXTERN bool	scrmsg();
FUNC_EXTERN bool	wwinerr();
FUNC_EXTERN bool	winerr();
FUNC_EXTERN bool	swinerr();
FUNC_EXTERN STATUS	DSstore();
FUNC_EXTERN STATUS	s_w_action();
FUNC_EXTERN void	IItm_retdesc();

/* End of getting-off-the-ground section */

# if defined(NT_GENERIC)
FUNC_EXTERN VOID	FEatt_dbdata(PTR, PTR);
FUNC_EXTERN DB_STATUS	FEatt_fopen(PTR, PTR);
FUNC_EXTERN DB_STATUS	FEatt_open(PTR, char *, char *);
FUNC_EXTERN DB_STATUS	FEatt_fetch(PTR, PTR);
FUNC_EXTERN DB_STATUS	FEatt_close(PTR);
FUNC_EXTERN STATUS	FEchkname(char *);
FUNC_EXTERN STATUS	FEchkenv(i4);
FUNC_EXTERN STATUS	FEcichk(i4, bool);
FUNC_EXTERN STATUS	FEcinfchk(i4, bool);
FUNC_EXTERN VOID	FE_decompose(PTR);
FUNC_EXTERN STATUS	FEforms();
FUNC_EXTERN bool	FE_fullresolve(PTR);
FUNC_EXTERN STATUS	FEingres(char *, ...);
FUNC_EXTERN STATUS	FEningres(char *, i4, char *, ...);
FUNC_EXTERN i4		FEinqrows();
FUNC_EXTERN i4		FEinqerr();
FUNC_EXTERN i4		FEinqferr();
FUNC_EXTERN FREELIST	*FElpcreate(i2);
FUNC_EXTERN PTR		FElpget(FREELIST *);
FUNC_EXTERN STATUS	FElpret(FREELIST *, i4 *);
FUNC_EXTERN STATUS	FElpdestroy(FREELIST *);
FUNC_EXTERN i4		FEprefix(register char *, char *, i4);
FUNC_EXTERN STATUS	FEprompt(char *, bool, i4, char *);
FUNC_EXTERN DB_STATUS	FErel_access(i4, char *, char *, PTR, bool, i4 *);
FUNC_EXTERN DB_STATUS	FErel_open(PTR, char *, char *, bool, bool);
FUNC_EXTERN DB_STATUS	FErel_fetch(register PTR, register PTR);
FUNC_EXTERN DB_STATUS	FErel_close(PTR);
FUNC_EXTERN STATUS	FErelexists(char *, ...);
FUNC_EXTERN bool	FE_resolve(PTR, char *, char *);
FUNC_EXTERN STATUS	FEtabfnd(char *, char *);
FUNC_EXTERN i4		FEtbsins(char *, char *, char *, char *, bool);
FUNC_EXTERN VOID	FEtfscrl(char *, char *, char *, i4);
FUNC_EXTERN STATUS	FEtstbegin(char *, char *, char *);
FUNC_EXTERN STATUS	FEtstend();
FUNC_EXTERN STATUS	FEtstmake();
FUNC_EXTERN STATUS	FEtstrun();
FUNC_EXTERN STATUS	FEt_open_bin(char *, char *, char *, PTR, FILE **);
FUNC_EXTERN VOID	FE_unresolve(PTR);
FUNC_EXTERN char	*FE_ur_partial(char *, char *, bool);
FUNC_EXTERN STATUS	FEutaget(char *, i4, i4, PTR, i4 *);
FUNC_EXTERN STATUS	FEutaopen(i4, char **, char *);
FUNC_EXTERN STATUS	FEutapget(i4, char **, i4 *, PTR);
FUNC_EXTERN bool	IIUIxrt_tbl(char *, char *, char *);	
FUNC_EXTERN bool	IIUGgci_getcid(char *, PTR, bool);
FUNC_EXTERN STATUS	IIUGcmInit();
FUNC_EXTERN VOID	FEutaclose();
FUNC_EXTERN VOID	IIUIea_escapos(char *, char *);
FUNC_EXTERN VOID	IIUIuea_unescapos(char *, char *);
#else /* NT_GENERIC */
VOID		FEatt_dbdata();
DB_STATUS	FEatt_fopen();
DB_STATUS	FEatt_open();
DB_STATUS	FEatt_fetch();
DB_STATUS	FEatt_close();
STATUS		FEchkname(/* char * */);
STATUS		FEchkenv();
VOID		FE_decompose();
STATUS		FEforms();
bool		FE_fullresolve();
STATUS		FEingres();
STATUS		FEningres();
i4		FEinqrows();
i4		FEinqerr();
i4		FEinqferr();
FREELIST	*FElpcreate();
PTR		FElpget();
STATUS		FElpret();
STATUS		FElpdestroy();
i4		FEprefix();
STATUS		FEprompt();
DB_STATUS	FErel_access();
DB_STATUS	FErel_open();
DB_STATUS	FErel_fetch();
DB_STATUS	FErel_close();
STATUS		FErelexists();
bool		FE_resolve();
STATUS		FEtabfnd();
STATUS		FEtstbegin();
STATUS		FEtstend();
STATUS		FEtstmake();
STATUS		FEtstrun();
STATUS		FEt_open_bin();
VOID		FE_unresolve();
char		*FE_ur_partial();
STATUS		FEutaget();
STATUS		FEutaopen();
STATUS		FEutapget();
bool		IIUIxrt_tbl();	
bool		IIUGgci_getcid();
STATUS   IIUGcmInit ();
VOID		FEutaclose();

VOID		IIUIea_escapos(char *,char *);
VOID		IIUIuea_unescapos(char *,char *);
VOID          VTCornerHilite();
VOID          vfwinstr();
VOID          VTwide2();
VOID          adjtWin();
VOID          adjtTab();
VOID          VTwide1();
VOID          FTwerr();
VOID          FTtlxadd();
VOID          FTdispcol();
VOID          uerror();
VOID          mu_outmenu();
VOID          FTdisptf();
VOID          FKclrlbl();
VOID          TDbox();
VOID          IIFSend_qry();    
#endif /* NT_GENERIC */

VOID		IIUIea_escapos(char *,char *);
VOID		IIUIuea_unescapos(char *,char *);


FUNC_EXTERN struct _ADF_CB *FEadfcb(void);
FUNC_EXTERN VOID        FEexits(char *);
FUNC_EXTERN bool	FEfehelp(char *, char *, i4);
FUNC_EXTERN void	FEhelp(char *, char *);
FUNC_EXTERN void	FEhhelp(char *, char *);
FUNC_EXTERN void	FEing_exit(void);
FUNC_EXTERN bool	FEinMST(void);
FUNC_EXTERN PTR		FEreqmem(u_i4, SIZE_TYPE, bool, STATUS *);
FUNC_EXTERN PTR		FEreqlng(u_i4, SIZE_TYPE, bool, STATUS *);
FUNC_EXTERN char	*FEsalloc(char *);
FUNC_EXTERN char	*FEtsalloc(u_i2, char *);
FUNC_EXTERN STATUS      FEfree(TAGID);
FUNC_EXTERN TAGID	FEgettag(void);
FUNC_EXTERN TAGID	FEbegintag(void);
FUNC_EXTERN TAGID	FEendtag(void);
FUNC_EXTERN VOID	FElocktag(bool);
FUNC_EXTERN bool	FEtaglocked(void);
FUNC_EXTERN PTR		iiugReqMem(u_i4, SIZE_TYPE, bool, STATUS *);
FUNC_EXTERN i4          IIUFccConfirmChoice(i4,PTR,PTR,PTR,PTR,...);
FUNC_EXTERN void	IIUFfieldHelp(void);
FUNC_EXTERN VOID	IIUGtagFree(TAGID);
FUNC_EXTERN STATUS	IIUGfreetagblock(u_i4);

#ifdef NT_GENERIC
FUNC_EXTERN VOID	IIUFprompt(char *, i4, bool, char [], i4, i4, ...);

FUNC_EXTERN i4		IIFRrlpRunListPick(char *, char *, i4, i4,char *,char *,
					   i4 (*)(), PTR);
FUNC_EXTERN i4		IIFRrfpRunForcedPick(char *, char *, i4, i4, PTR,
					     i4 (*)(), PTR);
FUNC_EXTERN i4		IIFRclpCallListPick(PTR, i4 *);
FUNC_EXTERN i4		IIFDlpListPick(char *, char *, i4, i4, i4, char *,
				       char *, i4 (*)(), PTR);
FUNC_EXTERN i4		IIFDlpeListPickEnd(char *, char *, i4, i4, i4, char *,
					   char *, i4 (*)(), PTR);
FUNC_EXTERN STATUS	IIFDffFormatField(char *, i4, i4, i4, i4, char *, i4,
					  i4, PTR, i4, i4, PTR *);
FUNC_EXTERN STATUS	IIFDaefAddExistFld(PTR, PTR, i4, i4);
FUNC_EXTERN VOID	IIMTlf(i4);
FUNC_EXTERN bool	IIMWguvGetUsrVer(char *);
FUNC_EXTERN STATUS	IIMWomwOverrideMW();
FUNC_EXTERN i4		ITlnprint(register char *, register char *, i4);
FUNC_EXTERN bool	IIUFmro_MoreOutput(register PTR, register i4, PTR);
FUNC_EXTERN i4		MTview(PTR, PTR, PTR, bool, i4 *, i4 *);
FUNC_EXTERN bool	IIUFadd_AddRec(PTR, char *, bool);
FUNC_EXTERN VOID	IIUFclb_ClearBcb(PTR);
FUNC_EXTERN STATUS	FTaddfrs(i4, i4, char *, i4, i4);
FUNC_EXTERN VOID	FTdisptf(PTR, PTR, register PTR);
FUNC_EXTERN i4		FTgetmenu(PTR, PTR);
FUNC_EXTERN VOID	FTtlxadd(PTR, char *);
FUNC_EXTERN VOID	mu_outmenu(char *);
FUNC_EXTERN VOID	FTdispcol(PTR, PTR, register PTR, register PTR);
FUNC_EXTERN bool	TDtgetflag(char *);
FUNC_EXTERN i4		TDtgetnum(char *);
FUNC_EXTERN char	*FTflfrsmu(i4);
FUNC_EXTERN STATUS	IIMWsmeSetMapEntry(char, i4);
FUNC_EXTERN VOID	IITDhotspot(i4, i4, i4, i4, char *, i4);
FUNC_EXTERN VOID	TDsetattr(PTR, i4);
FUNC_EXTERN STATUS	IIMWdmDisplayMsg(char *, i4, PTR);
FUNC_EXTERN STATUS	TDonewin(PTR *, PTR *, PTR *, i4, i4);
FUNC_EXTERN i4		TDxaddstr(PTR, i4, u_char *);
FUNC_EXTERN STATUS	IIMWguiGetUsrInp(char *, i4, PTR, char *);
FUNC_EXTERN i4		TDmove(register PTR, register i4, register i4);
FUNC_EXTERN VOID	TDtputs(char *, i4, i4(*)());
FUNC_EXTERN STATUS	IIMWscfSetCurFld(PTR, i4);
FUNC_EXTERN STATUS	IIMWscmSetCurMode(i4, i4, bool, bool, bool);
FUNC_EXTERN STATUS	IIMWdlkDoLastKey(bool, bool *);
FUNC_EXTERN VOID	IIFTmlsMvLeftScroll(PTR, PTR, bool);
FUNC_EXTERN VOID	IIFTmrsMvRightScroll(PTR, PTR, bool);
FUNC_EXTERN VOID	IIFTsfrScrollFldReset(PTR, PTR, bool);
FUNC_EXTERN STATUS	IIMWgscGetSkipCnt(PTR, i4, i4 *);
FUNC_EXTERN STATUS	IIMWtvTblVis(PTR, i4, i4, i4);
FUNC_EXTERN i4		FTprmthdlr(char *, u_char *, i4, PTR);
FUNC_EXTERN VOID	FTmessage(char *, bool, bool, ...);
FUNC_EXTERN VOID	TDoverlay(PTR, i4, i4, PTR, i4, i4);
FUNC_EXTERN VOID	IITDnwscrn(char *, i4, i4, i4, i4, i4, i4);
FUNC_EXTERN VOID	TDsbox(PTR, i4, i4, i4, i4, char, char, char, i4, i4,
			       bool);
FUNC_EXTERN VOID	TDputattr(PTR, i4);
FUNC_EXTERN STATUS	TDmvstrwadd(PTR, i4, i4, char *);
FUNC_EXTERN VOID	IITDnwtbl(i4, PTR);
FUNC_EXTERN STATUS	FTtblbld(PTR, PTR, bool, PTR, bool);
FUNC_EXTERN VOID	IITDfield(i4, i4, i4, i4, i4, i4, i4, i4);
FUNC_EXTERN VOID	IITDfldtitle(i4, i4, char *);
FUNC_EXTERN VOID	TDbox(PTR, char, char, char);
FUNC_EXTERN VOID	TDboxhdlr(register PTR, register i4, register i4,
				  register i4);
FUNC_EXTERN VOID	TDtfdraw(register PTR, register i4, register i4,
				 register char);
FUNC_EXTERN VOID	IIMWemEnableMws(i4);
FUNC_EXTERN VOID	FTsetda(PTR, i4, i4, i4, i4, i4, i4);
FUNC_EXTERN i4		TDmvcrsr(register i4, register PTR, register i4, bool);
FUNC_EXTERN STATUS	IIMWdsmDoSkipMove(PTR, i4 *, i4, i4(*)(), i4(*)());
FUNC_EXTERN VOID	FTscrrng(PTR, i4, i4, i4, PTR, PTR *);
FUNC_EXTERN STATUS	FTlastpos(PTR, i4, i4, u_char *);
FUNC_EXTERN i4		TDaddchar(PTR, u_char *, bool);
FUNC_EXTERN STATUS	FTbuild(PTR, bool, PTR, bool);
FUNC_EXTERN VOID	FTvi(register PTR, register i4, register PTR);
FUNC_EXTERN VOID	TDdumpwin(PTR, PTR, bool);
FUNC_EXTERN STATUS	IIMWfkmFKMode(i4);
FUNC_EXTERN VOID	TDfkmode(i4);
FUNC_EXTERN i4		TDrmove(register PTR, register i4, register i4);
FUNC_EXTERN VOID	IIFTcsfClrScrollFld(PTR, bool);
FUNC_EXTERN VOID	IIFTsfaScrollFldAddstr(PTR, i4, char *);
FUNC_EXTERN VOID	IIFTpwsPrevWordScroll(PTR, PTR, bool);
FUNC_EXTERN VOID	IIFTnwsNextWordScroll(PTR, PTR, bool);
FUNC_EXTERN STATUS	IIMWufUpdFld(PTR, i4, i4, i4, i4);
FUNC_EXTERN i4		mu_trace(PTR);
FUNC_EXTERN VOID	FTshiftmenu(PTR, u_char);
FUNC_EXTERN VOID	uerror(PTR, u_i4, char *);
FUNC_EXTERN STATUS	IIMWiInit(i4, i4);
FUNC_EXTERN STATUS	IIMWkmiKeyMapInit(PTR, i4, PTR, i4, i2 *, i4, i1 *);
FUNC_EXTERN VOID	FKclrlbl(i4);
FUNC_EXTERN VOID	FK40parse(PTR, i4);
FUNC_EXTERN STATUS	IIMWkfnKeyFileName(char *);
FUNC_EXTERN VOID	FKxtcmd(i4, i4 *, i4 *);
FUNC_EXTERN VOID	IIMWrmeRstMapEntry(char, i4);
FUNC_EXTERN VOID	TDnomacro(i4);
FUNC_EXTERN STATUS	IIMWfmFlashMsg(char *, i4);
FUNC_EXTERN STATUS	IIFTpumPopUpMessage(char *, i4, i4, i4, i4, i4, PTR,
					    i4);
FUNC_EXTERN STATUS	IIFTpupPopUpPrompt(char *, i4, i4, i4, i4, i4, PTR,
					   char *);
FUNC_EXTERN bool	FTputgui(PTR, i2, i2);
FUNC_EXTERN VOID	IITDmnline(i4, i4, char *);
FUNC_EXTERN VOID	FTringShift(i4);
FUNC_EXTERN VOID	FTguiShift(i4);
FUNC_EXTERN VOID	FTrngshft(PTR, i4, PTR, bool);
FUNC_EXTERN VOID	FTrngdcp(PTR, i4, PTR, i4);
FUNC_EXTERN STATUS	IIMWgrvGetRngVal(PTR, i4, PTR, u_char *);
FUNC_EXTERN STATUS	IIMWsttSetTbcellText(i4, i4, i4, i4, u_i4, u_char *);
FUNC_EXTERN STATUS	IIMWsaSetAttr(PTR, i4, i4, i4, i4, i4);
FUNC_EXTERN VOID	TDwin_rstr(PTR, PTR, bool);
FUNC_EXTERN STATUS	IIMWrucRngUpCp(PTR, i4, i4);
FUNC_EXTERN STATUS	IIMWrdcRngDnCp(PTR, i4, i4);
FUNC_EXTERN VOID	TDrstcur(i4, i4, i4, i4);
FUNC_EXTERN STATUS	FTwtrv(PTR, i4);
FUNC_EXTERN VOID	FTsetmmap(i4);
FUNC_EXTERN VOID	TDrestore(i4, i4);
FUNC_EXTERN VOID	IITDgvmode(i4);
FUNC_EXTERN STATUS	FTforcebrowse(PTR, i4);
FUNC_EXTERN i4		FTbrowse(PTR, i4, PTR);

FUNC_EXTERN i4		FTinsert(PTR, i4, PTR);
FUNC_EXTERN VOID	IIFTsnfSetNbrFields(i4);
FUNC_EXTERN VOID	IITDposPrtObjSelected(char *);
FUNC_EXTERN VOID	IIFTdcaDecodeCellAttr(i4, i4 *);
FUNC_EXTERN VOID	FKumapcmd(i4, i4);
FUNC_EXTERN VOID	IIFTsmShiftMenu(i4);
FUNC_EXTERN VOID	TDrowhilite(PTR, i4, u_char);
FUNC_EXTERN VOID	FTwerr(char *, bool, bool, ...);
FUNC_EXTERN VOID	FTwinerr(char *, bool, ...);
FUNC_EXTERN VOID	TDhilite(PTR, i4, i4, u_char);
FUNC_EXTERN VOID	FK30parse(u_char *, i4);
FUNC_EXTERN VOID	FKprserr(char *, i4, u_char *);
FUNC_EXTERN STATUS	FKdomap(i4, i4, i4, i4, i4, i4, char *, i4, char *);
FUNC_EXTERN STATUS	IIMWrfRstFld(PTR, i4, PTR);
FUNC_EXTERN VOID	FTrgtag(i2);
FUNC_EXTERN STATUS	FTrowtrv (PTR, i4, bool);
FUNC_EXTERN VOID	TDclrbot(PTR, bool);
FUNC_EXTERN VOID	FTrngfldres(PTR, PTR, bool);
FUNC_EXTERN i4		FTmove(i4, PTR, i4 *, PTR *, i4, PTR);
FUNC_EXTERN i4		FTrmvright(PTR, PTR, bool);
FUNC_EXTERN i4		FTrmvleft(PTR, PTR, bool);
FUNC_EXTERN i4		FTrgscrup(PTR, i4, PTR, i4);
FUNC_EXTERN STATUS	FTtbltrv(PTR, i4);
FUNC_EXTERN STATUS	FTtrv(PTR, i4, bool);
FUNC_EXTERN STATUS	FTrngxpand(PTR, i4);
FUNC_EXTERN VOID	IIFTrbsRuboutScroll(PTR, bool);
FUNC_EXTERN VOID	WNrunNoFld(i2);
FUNC_EXTERN i4		FTattrxlate(i4);
FUNC_EXTERN VOID	TDupvfsl(PTR, i4);
FUNC_EXTERN STATUS	TDltvfsl(PTR, i4, bool);
FUNC_EXTERN VOID	VTClearArea(PTR, i4, i4, i4, i4);
FUNC_EXTERN VOID	TDcorners(PTR, i4);
FUNC_EXTERN VOID	VTwide1(PTR, i4);
FUNC_EXTERN VOID	VTwide2(PTR, i4);
FUNC_EXTERN VOID	TDflashplus(PTR, i4, i4, i4);
FUNC_EXTERN VOID	VTxydraw(PTR, i4, i4);
FUNC_EXTERN VOID	FTword(PTR, i4, PTR, i4);
FUNC_EXTERN VOID	adjtTab(PTR, char, i4, i4);
FUNC_EXTERN VOID	adjtWin(PTR, i4, i4);
FUNC_EXTERN STATUS	TDmvchwadd (PTR, i4, i4, char);
FUNC_EXTERN VOID	VTdispbox(PTR, i4, i4, i4, i4, i4, bool, bool, bool);
FUNC_EXTERN VOID	VTCornerHilite(PTR, i4, i4, i4, i4, bool);
FUNC_EXTERN VOID	TDsetdispattr(i4);
FUNC_EXTERN VOID	FSdiag (char *, ...);
FUNC_EXTERN VOID	FTswinerr(char *, bool, ...);
FUNC_EXTERN STATUS	FSsetup(PTR, char *, char *, char *, i4, PTR, PTR);
FUNC_EXTERN bool	IIUFdsp_Display(PTR, PTR, char *, char *, bool,
					char *, char *, bool);
FUNC_EXTERN STATUS	FSonError(i4);
FUNC_EXTERN VOID	IIFSend_qry(PTR, PTR);
FUNC_EXTERN VOID	IItm_status(i4 *, i4 *, i4 *, bool *);
FUNC_EXTERN STATUS	FSargs(i4, char **, PTR, char **, char *, i4);
FUNC_EXTERN VOID	fstest (char *, char *);
FUNC_EXTERN i4		IIAFdsDmlSet(i4);
FUNC_EXTERN STATUS	FSrun (PTR, char *, i4);
FUNC_EXTERN VOID	IIQRmdh_MultiDetailHelp(PTR, char *, i4);
FUNC_EXTERN STATUS	IIQR_relexists (i4, char *);
FUNC_EXTERN STATUS	IIQRaln_AddListNode(PTR, char *, char *, char * i4);
FUNC_EXTERN VOID	IItm_dml(i4);
FUNC_EXTERN VOID	IIQRsqhSetQrHandlers(bool);
FUNC_EXTERN VOID	IItm(bool);
FUNC_EXTERN VOID	IItm_adfucolset(PTR);
FUNC_EXTERN VOID	qradd(PTR, char *, i4);
FUNC_EXTERN STATUS	FSqbinit(PTR);
FUNC_EXTERN STATUS	FSqbfull(PTR);
FUNC_EXTERN STATUS	IIQRtbo_TableOwner(PTR, PTR, char, PTR, PTR);
FUNC_EXTERN STATUS	IIQRtot_TblOwnerType(PTR, PTR, PTR, PTR, PTR);
FUNC_EXTERN STATUS	qroutalloc(PTR, i4);
FUNC_EXTERN i4		IIFRfind_row(PTR, PTR, bool); 
FUNC_EXTERN VOID	FEgeterrmsg(u_i4, i4, char [], ...);
FUNC_EXTERN VOID	FEmsg(char *, bool, ...);
FUNC_EXTERN i4		FEpresearch(register i4, char *[], register char *);
FUNC_EXTERN VOID	IIUGprompt(char *, i4, bool, char [], i4, i4, ...);
FUNC_EXTERN VOID	FEutaerr(u_i4, i4, ...);
FUNC_EXTERN char	*FEutapfx(i4);
FUNC_EXTERN VOID	IIUGatoi(char *, i4 *, i4);
FUNC_EXTERN VOID	IIUGclCopyLong(register PTR, register u_i4,
				       register PTR);
FUNC_EXTERN char	*IIUGdmlStr(DB_LANG);
FUNC_EXTERN VOID	ugstderr(PTR, char *, bool);
FUNC_EXTERN VOID	IIUGeeExprErr(u_i4, i4, i4, ...);
FUNC_EXTERN bool	IIUGtcTypeCompat(i4, i4);
FUNC_EXTERN i4		IIUGhsHtabScan(PTR, bool, char **, PTR *);
FUNC_EXTERN STATUS	IIUGihiIntHtabInit(i4, VOID (*)(), i4 (*)(), i4 (*)(),
					   PTR *);
FUNC_EXTERN STATUS	IIUGihfIntHtabFind(PTR, i4, PTR *);
FUNC_EXTERN STATUS	IIUGihdIntHtabDel(PTR, i4);
FUNC_EXTERN STATUS	IIUGiheIntHtabEnter(PTR, i4, PTR);
FUNC_EXTERN i4		IIUGihsIntHtabScan(PTR, bool, i4 *, PTR *);
FUNC_EXTERN VOID	IIUGitoa(i4, char *, i4);
FUNC_EXTERN char	*iiugOpRep(i4);
FUNC_EXTERN i4		IIUGnpNextPrime(register i4);
FUNC_EXTERN VOID	IIUGqsort(char *, i4, i4, i4 (*)());
FUNC_EXTERN STATUS	IIUGvfVerifyFile(char *, i4, bool, bool, i4);
FUNC_EXTERN STATUS	IIFDaetAddExistTrim(PTR, PTR, i4, i4);
FUNC_EXTERN STATUS	IIFDaefAddExistFld(PTR, PTR, i4, i4);
FUNC_EXTERN STATUS	IIFDaexAddExistTf(PTR, PTR, i4, i4);
FUNC_EXTERN STATUS	IIFDaecAddExistCol(PTR, PTR, PTR);
FUNC_EXTERN bool	IIUFatsAdjustTfSize(char *, char *, i4, i4);
FUNC_EXTERN char	*FEtrunc(char *, i4);
FUNC_EXTERN STATUS	IIFDfcFormatColumn(char *, i4, char *, PTR, i4, PTR *);
FUNC_EXTERN STATUS	IIFDmcMakeColumn(char *, char *, PTR, bool, i4, PTR,
					 i4 *);
FUNC_EXTERN STATUS	IIFDgcGenColumn(PTR, i4, PTR, PTR *);
FUNC_EXTERN bool	IIRTfrshelp(char *, char *, i4, VOID (*)());
FUNC_EXTERN VOID	IIUFask(char *, bool, char [], i4, ...);
FUNC_EXTERN bool	IIUFver(char *, i4, ...);
FUNC_EXTERN VOID	iiufBrExit(PTR, u_i4, i4, PTR, ...);
FUNC_EXTERN VOID	IIUFrtmRestoreTerminalMode(i4);
FUNC_EXTERN VOID	IIFTrfoRegFormObj(PTR, i4, i4, i4, i4, i4, i4, i4, i4);
FUNC_EXTERN VOID	vfwinstr(register PTR, u_char *);
#else
FUNC_EXTERN bool	IIUFver();
FUNC_EXTERN void	FEutaerr();
FUNC_EXTERN VOID	iiufBrExit();
FUNC_EXTERN VOID	IItm_status(i4 *, i4 *, i4 *, bool *);
FUNC_EXTERN void	IItm(bool);
FUNC_EXTERN VOID	IItm_adfucolset();
FUNC_EXTERN STATUS	IIMWomwOverrideMW();
FUNC_EXTERN STATUS	FSrun();
FUNC_EXTERN STATUS	FSargs();
FUNC_EXTERN STATUS	FSqbinit();
FUNC_EXTERN STATUS	FSqbfull();
FUNC_EXTERN STATUS	FSonError();
FUNC_EXTERN i4		IIAFdsDmlSet();
FUNC_EXTERN VOID	IIUFclb_ClearBcb();
FUNC_EXTERN VOID	FTswinerr();
FUNC_EXTERN VOID	FSdiag (char *str, ...);
FUNC_EXTERN STATUS 	FSsetup();
FUNC_EXTERN VOID	FEafeerr(struct _ADF_CB  *cb);
FUNC_EXTERN VOID	fstest();
FUNC_EXTERN i4		IIFDlpListPick();
FUNC_EXTERN i4		IIFRrlpRunListPick();
FUNC_EXTERN i4		FTgetmenu();
FUNC_EXTERN STATUS	FTaddfrs();
FUNC_EXTERN VOID	FTsetda();
FUNC_EXTERN VOID	VTdispbox();
FUNC_EXTERN VOID	VTClearArea();
FUNC_EXTERN VOID	VTxydraw();
FUNC_EXTERN VOID	FEmsg();
#endif

#ifdef NOT_YET
/*{
** Name:    FEalloc() -	Front-End Memory Allocation.
**
** Description:
**	Allocates memory out of the current tag block.
**
** Input:
**	size	{u_i4}  The size in bytes of the requested memory block.
**	status	{STATUS *}  Status result.
**
** Returns:
**	{PTR}  Address of the allocated memory (NULL if error.)
**
** History:
**	07/87 (jhw) -- Defined as cover for 'FEreqmem()'.
*/
#define FEalloc(size, status) FEreqmem(0, (size), FALSE, (status))

/*{
** Name:    FEcalloc() -	Front-End Cleared Memory Allocation.
**
** Description:
**	Allocates cleared memory out of the current tag block.
**
** Input:
**	size	{u_i4}  The size in bytes of the requested memory block.
**	status	{STATUS *}  Status result.
**
** Returns:
**	{PTR}  Address of the allocated memory (NULL if error.)
**
** History:
**	07/87 (jhw) -- Defined as cover for 'FEreqmem()'.
*/
#define FEcalloc(size, status) FEreqmem(0, (size), TRUE, (status))

/*{
** Name:    FEtalloc() -	Front-End Tagged Memory Allocation.
**
** Description:
**	Allocates memory out of the input tag block.
**
** Input:
**	tag	{u_i4}  The tag for memory allocation.
**	size	{u_i4}  The size in bytes of the requested memory block.
**	status	{STATUS *}  Status result.
**
** Returns:
**	{PTR}  Address of the allocated memory (NULL if error.)
**
** History:
**	07/87 (jhw) -- Defined as cover for 'FEreqmem()'.
*/
#define FEtalloc(tag, size, status) FEreqmem((tag), (size), FALSE, (status))

/*{
** Name:    FEtcalloc() -	Front-End Tagged Cleared Memory Allocation.
**
** Description:
**	Allocates cleared memory out of the input tag block.
**
** Input:
**	tag	{u_i4}  The tag for memory allocation.
**	size	{u_i4}  The size in bytes of the requested memory block.
**	status	{STATUS *}  Status result.
**
** Returns:
**	{PTR}  Address of the allocated memory (NULL if error.)
**
** History:
**	08/88 (jhw) -- Defined as cover for 'FEreqmem()'.
*/
#define FEtcalloc(tag, size, status) FEreqmem((tag), (size), TRUE, (status))

#endif

/*}
** Name:	DATE - internal date format
**
** Description:
**	Variables declared as DATE must be manipulated through the set of
**	routines provided.  It is used as a convenience to pass date fields
**	to and from the database.
**
** History:
**	31-Mar-1987 (rdesmond)
**		first introduced
**      15-Mar-95 (fanra01)
**              Modified DATE definition to avoid key word clash with Microsoft
**              compiler.
*/
#ifdef WIN32
typedef struct
{
        i4      date[3];
} ii_DATE;
#define DATE    ii_DATE
#else
typedef struct	s_DATE
{
	i4	date[3];
} DATE;
#endif



/**
** Name:    fehdr.h - Header file constants that Front-Ends need.
**
** Description:
**	This file contains constant definition that all front ends need.
**
** History:
**	@(#)fehdr.h	1.1	1/24/85
**	02/20/87 (dkh) - Added definition of FE_MAXNAME.
**	03/06/87 (dkh) - Renamed BUFSIZ to FE_BUFSIZ to eliminate
**			 conflict with similar declaration in "si.h".
**	03/11/87 (dkh) - Deleted FE_BUFSIZE as no one is using it.
**	03/21/87 (bobm)  Delete EXTYPUSER
**	12/1/89  (pete)  Add FE_MAXTABNAME.
**	08/90 (jhw) -- Removed now un-used PG_SIZE.
**	09-jul-92 (rdrane)
**		Redefine FE_MAXTABNAME to accomodate both owner and tablename
**		as delimited identifiers.  This increases the size from
**			(FE_MAXNAME*2 +4 +1)
**		to
**			((((FE_MAXNAME * 2) + 2) * 2) + 1 + 1),
**		which handles a name composed entirely of escaped double quotes
**		plus the delimiting double quotes, times both owner and
**		tablename, plus the delimiting period, plus the terminating
**		NULL.
**	20-jul-92 (rdrane)
**		Add definition of FE_UNRML_MAXNAME (FE name max length when
**		in un-normalized form).  Replace the ((FE_MAXNAME * 2) + 2)
**		portion of FE_MAXTABNAME with the new constant.
**	08-mar-2001 (somsa01)
**	    Added FE_FMTMAXCHARWIDTH.
**/

/*
**  New front end constant that defines the maximum name
**  length of front end objects (e.g., form/field names).
**  This is needed since front end object names may
**  need to be bigger on certain environments (such as DG).
**  That's why FE_MAXNAME, instead of DB_MAXNAME, should be used. (dkh)
**  This length assumes normalized form.
*/
#define		FE_MAXNAME	32

/*
**  New front end constant that defines the maximum name
**  length of front end objects (e.g., form/field names)
**  when in un-normalized form.
*/
#define		FE_UNRML_MAXNAME ((FE_MAXNAME * 2) + 2)
/*
**  This constant defines the maximum size of a table reference.
**  Table references can be "table", or a fully qualified "owner.table",
**  or (later, after 6.4) a table reference using delimited identifiers, for
**  example: "mytab"."column".  This length assumes un-normalized form for both
**  components.
*/
#define		FE_MAXTABNAME	((FE_UNRML_MAXNAME * 2) + 1 + 1)

/*
**  Front end constant defining the maximum number of
**  characters in a database name.  This is needed because 
**  a database name can be an absolute pathname in some 
**  environments (DG for instance). (danielt)
*/
#define 	FE_MAXDBNM	256

/*
**  Maximum number of characters which will be returned in an FRS prompt
**  response, no matter how wide the screen is.  In practice, this is wider
**  than any screens we know of thus far.  Useful in allocating response
**  buffers.  Does not count the terminating EOS.
*/
#define		FE_PROMPTSIZE	200

/*
** Current version of Vision.
*/
#define		FE_VISIONVER	2

/*
**  Historically, fmt_ideffmt() was initially invoked by front-end code
**  with a maxcharwidth of 10000. This was done to initially avoid
**  wrap format for form display. 10000 was just a huge number, used
**  so that other default formats were compatible. After the inception
**  of 32K char lengths, thus "huge number" was no longer huge. Thus,
**  it has been bumped up to 40000, which came about like this:
**
**      10000 - 2000 (old max char length) + 32000 (new max char length)
**
**  Note that if the max char length is ever increased, this number WILL
**  need to change.
*/
#define         FE_FMTMAXCHARWIDTH      40000


/*
** Structure used by IIUGgci_getcid() to parse compound identifier constructs
** such as owner.table, owner.procedure, and/or correlationname.column.
** Since it depends on FE_UNRML_MAXNAME, it must follow the fehdr.h
** inclusion/copy.
*/
typedef struct
{
	char	name[(((2 * FE_UNRML_MAXNAME) + 1) + 1)];
	i4	c_oset;
	i4	b_oset;
	i4	e_code;
# define	FE_GCI_NO_ERROR		0
# define	FE_GCI_EMPTY		1	/* No identifier found  */
# define	FE_GCI_BARE_CID		2	/*
						** Compound delimiter before or
						** after single identifier -
						**	.["]ident["]
						**	["]ident["].
						*/
# define	FE_GCI_DEGEN_DID	3	/*
						** Degenerate delimited
						** identifier component(s)
						**	"
						**	""
						*/
# define	FE_GCI_UNBAL_Q		4	/*
						** Unbalanced delimiting or
						** embedded quotes
						**	"abc
						**	"ab"""c"
						**	a"b
						*/
	char	owner[(FE_UNRML_MAXNAME + 1)];
	char	object[(FE_UNRML_MAXNAME + 1)];
}       FE_GCI_IDENT;


#endif
