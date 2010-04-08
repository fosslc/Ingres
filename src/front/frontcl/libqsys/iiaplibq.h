/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	IIAPLIBQ.H - ANSI C Funtion Prototypes for the runtime layer
**		    & function declarations for the existing LIBQ
**		    calls.
**
**		This header file differs from eqdefc.h because it
**		uses the data types defined in compat.h. Except for
**
**		PTR is represented by II_LQTYPE_PTR.
**		II_LQTYPE_GPTR replaces char * where appropriate.
**
**		This file contains the IIpxxx routine names.
**
**		Porting notes:
**
**			Ingres PTR is translated into II_LQTYPE_PTR.
**
**	History:
**		20-apr-1994	(teresal)
**				Written for adding ESQL/C++ and '-prototypes'
**				for ESQL/C.
**		06-may-1994	(teresal)
**				Correct function prototypes for IIpseterr
**				and IIpLQldh_LoDataHandler - bugs 63205 &
**				63202.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-08-2002 (zhahu02)
**          Modified for IIpLQlgd_LoGetData (b108458/INGSRV1808).
**	16-jun-2005 (gupsh01)
**	    Added IILQucolinit(). 
**	6-Jun-2006 (kschendel)
**	    Add IIsqDescInput.
**      09-apr-2007 (drivi01)
**          BUG 117501
**          Adding references to function IIcsRetScroll in order to fix
**          build on windows.
**      09-oct-2007 (huazh01)
**          update the parameter list of IILQprsProcStatus(). It now takes
**          one parameter. This fixes bug 119273. 
**      17-oct-2007 (huazh01)
**          update the parameter list of IIpLQprsProcStatus(). It nows takes
**          one parameter. This fixes bug 119273. 
*/

# define II_LQTYPE_PTR		void *
# define II_LQTYPE_GPTR         void *

void	IIpbreak(void);
void	IIpcsClose(char *cursor_name,i4 num1,i4 num2);
void	IIpcsDaGet(i4 lang,void *sqd);
void	IIpcsDelete(char *table_name,char *cursor_name,i4 num1,
		i4 num2);
void	IIpcsERetrieve(void);
void	IIpcsERplace(char *cursor_name,i4 num1,i4 num2);
void	IIpcsGetio(i2 *indaddr,i4 isvar,i4 type,
		 i4  length,II_LQTYPE_GPTR address);
void	IIpcsOpen(char *cursor_name,i4 num1,i4 num2);
void	IIpcsParGet(char *target_list,char **argv,
		i4 (*transfunc)());
void	IIpcsQuery(char *cursor_name,i4 num1,i4 num2);
void	IIpcsRdO(i4 what,char *rdo);
void	IIpcsReplace(char *cursor_name,i4 num1,i4 num2);
i4	IIpcsRetrieve(char *cursor_name,i4 num1,
		i4 num2);
i4	IIpcsRetScroll(char *cursor_name, i4 num1, i4 num2, 
		i4 fetcho, i4 fetchn);
void	IIpeqiqio(i2 *indaddr,i4 isvar,i4 type,
		 i4  len,II_LQTYPE_PTR addr,char *name);
void	IIpeqstio(char *name, i2 *indaddr,i4 isvar,
		 i4  type,i4 len,II_LQTYPE_PTR data);
i4	IIperrtest(void);
void	IIpexDefine(i4 what,char *name,i4 num1,
		i4 num2);
i4	IIpexExec(i4 what,char *name,i4 num1,
		i4 num2);
void	IIpexit(void);
void	IIpflush(char *file_nm,i4 line_no);
i4	IIpgetdomio(i2 *indaddr,i4 isvar,
		i4 hosttype,i4 hostlen,
		II_LQTYPE_GPTR addr);
VOID	IIpingopen(i4 lang,char *p1,char *p2,char *p3,
		char *p4,char *p5,char *p6,char *p7,char *p8,char *p9,
		char *p10,char *p11,char *p12,char *p13,char *p14,char *p15);
VOID	IIpLQcnConName(char *con_name);
II_LQTYPE_PTR	IIpLQdbl(double dblval);
VOID	IIpLQesEvStat(i4 escall,i4 eswait);
II_LQTYPE_PTR	IIpLQint(i4 intval);
void	IIpLQisInqSqlio(i2 *indaddr,i4 isvar,
		i4 type,i4 len,II_LQTYPE_PTR addr,
		i4 attrib);
void	IIpLQldh_LoDataHandler(i4 type,i2 *indvar, 
		i4 (*datahdlr)(void *),II_LQTYPE_PTR hdlr_arg);
i4	IIpLQled_LoEndData(void);
void	IIpLQlgd_LoGetData(i4 flags,i4 hosttype, 
		i4 hostlen,char *addr,
		i4 maxlen,i4 *seglen, 
		i4 *dataend);
void	IIpLQlpd_LoPutData(i4 flags,i4 hosttype,
		i4 hostlen,char *addr,i4 seglen,
		i4 dataend);
VOID	IIpLQpriProcInit(i4 proc_type,char *proc_name);
i4	IIpLQprsProcStatus(i4 resultCount);
VOID	IIpLQprvProcValio(char *param_name,i4 param_byref,
		i2 *indaddr,i4 isref,i4 type,
		i4 length,II_LQTYPE_PTR addr1, ...);
void	IIpLQshSetHandler(i4 hdlr,i4 (*funcptr)());
VOID	IIpLQsidSessID(i4 session_id);
void	IIpLQssSetSqlio(i4 attrib,i2 *indaddr,
		i4 isvar,i4 type,i4 len,
		II_LQTYPE_PTR data);
i4	IIpnexec(void);
i4	IIpnextget(void);
i4	IIpparret(char *tlist,char **argv,i4 (*xfunc)());
i4	IIpparset(char *string,char **argv,
		char *(*transfunc)());
VOID	IIpputctrl(i4 ctrl);
void	IIpputdomio(i2 *indaddr,i4 isvar,
		i4 type,i4 length,
		II_LQTYPE_GPTR addr);
void	IIpretinit(char *file_nm,i4 line_no);
i4	(* IIpseterr(i4 (*proc)(i4 *errno)))();
void	IIpsexec(void);
void	IIpsqConnect(i4 lang,char *db,char *a1,char *a2,char *a3,
		char *a4,char *a5,char *a6,char *a7,char *a8,char *a9,
		char *a10,char *a11, char *a12,char *a13);
void	IIpsqDaIn(i4 lang, II_LQTYPE_GPTR sqd);
void 	IIpsqDescInput(i4 lang,char *stmt_name,II_LQTYPE_GPTR sqd);
void 	IIpsqDescribe(i4 lang,char *stmt_name,II_LQTYPE_GPTR sqd,
		i4 using_flag);
void	IIpsqDisconnect(void);
void	IIpsqExImmed(char *query);
void	IIpsqExStmt(char *stmt_name,i4 using_vars);
void	IIpsqFlush(char *filename,i4 linenum);
VOID	IIpsqGdInit(i2 sqlsttype,char *sqlstdata);
void	IIpsqInit(II_LQTYPE_GPTR sqc);
void	IIpsqMods(i4 mod_flag);
VOID	IIpsqParms(i4 flag,char * name_spec,
		i4 val_type,char *str_val, i4  int_val);
void	IIpsqPrepare(i4 lang,char *stmt_name,II_LQTYPE_GPTR sqd,
		i4 using_flag,char *query);
VOID	IIpsqPrint(II_LQTYPE_GPTR sqlca);
void	IIpsqStop(II_LQTYPE_GPTR sqc);
void	IIpsqTPC(i4 highdxid,i4 lowdxid);
void	IIpsqUser(char * usename);
VOID	IIpsqlcdInit(II_LQTYPE_GPTR sqc,i4 *sqlcd);
void	IIpsyncup(char *file_nm,i4 line_no);
i4	IIptupget(void);
void	IIputsys(i4 uflag,char *uname,char *uval);
void	IIpvarstrio(i2 *indnull,i4 isvar,
		i4 type,i4 length,II_LQTYPE_GPTR addr);
void	IIpwritio(i4 trim,i2 *ind_unused,
		i4 isref_unused,i4 type,
		i4 length,char *qry);
void	IIpxact(i4 what);

/* Existing LIBQ function declarations */

void    IIbreak();
void    IIcsClose();
void    IIcsDaGet();
void    IIcsDelete();
void    IIcsERetrieve();
void    IIcsERplace();
void    IIcsGetio();
void    IIcsOpen();
void    IIcsParGet();
void    IIcsQuery();
void    IIcsRdO();
void    IIcsReplace();
i4      IIcsRetrieve();
void    IIeqiqio();
void    IIeqstio();
i4      IIerrtest();
void    IIexDefine();
i4      IIexExec();
void    IIexit();
void    IIflush();
i4      IIgetdomio();
VOID    IIingopen();
VOID    IILQcnConName();
II_LQTYPE_PTR   IILQdbl();
VOID    IILQesEvStat();
II_LQTYPE_PTR   IILQint();
void    IILQisInqSqlio();
void    IILQldh_LoDataHandler();
i4 IILQled_LoEndData();
void    IILQlgd_LoGetData();
void    IILQlpd_LoPutData();
VOID    IILQpriProcInit();
i4      IILQprsProcStatus(int resultCount);
VOID    IILQprvProcValio();
void    IILQshSetHandler();
VOID    IILQsidSessID();
i4	IILQucolinit();
void    IILQssSetSqlio();
i4      IInexec();
i4      IInextget();
i4      IIparret();
i4      IIparset();
VOID    IIputctrl();
void    IIputdomio();
void    IIretinit();
i4 (* IIseterr())();
void    IIsexec();
void    IIsqConnect();
void    IIsqDaIn();
void    IIsqDescribe();
void    IIsqDisconnect();
void    IIsqExImmed();
void    IIsqExStmt();
void    IIsqFlush();
VOID    IIsqGdInit();
void    IIsqInit();
void    IIsqMods();
VOID    IIsqParms();
void    IIsqPrepare();
VOID    IIsqPrint();
void    IIsqStop();
void    IIsqTPC();
void    IIsqUser();
VOID    IIsqlcdInit();
void    IIsyncup();
i4      IItupget();
void    IIutsys();
void    IIvarstrio();
void    IIwritio();
void    IIxact();
i4	IIcsRetScroll();



/* 4GL Runtime System Calls */

i4      IIG4pacArrayClear(i4 array);
i4      IIG4pbpByrefParam(i2 *ind, i4  isval, i4  type, i4  length, 
         II_LQTYPE_PTR   data, char *name);
i4      IIG4pccCallComp();
i4      IIG4pchkobj(i4 object, i4  access, i4  index, i4  caller);
i4      IIG4pdrDelRow(i4 array, i4  index);
i4      IIG4pfdFillDscr(i4 object, i4  language, II_LQTYPE_GPTR *sqd);
i4      IIG4pgaGetAttr(i2 *ind, i4  isvar, i4  type, i4  length, 
         II_LQTYPE_PTR  data, char *name);
i4      IIG4pggGetGlobal(i2 *ind, i4  isvar, i4  type, i4  length, 
         II_LQTYPE_PTR  data, char *name, i4  gtype);
i4      IIG4pgrGetRow(i2 *rowind, i4  isvar, i4  rowtype, i4  rowlen, 
         II_LQTYPE_PTR  rowptr, i4 array, i4  index);
i4      IIG4pi4Inq4GL(i2 *ind, i4  isvar, i4  type, i4  length, 
         II_LQTYPE_PTR  data, i4 object, i4  code);
i4      IIG4picInitCall(char *name, i4  type);
i4      IIG4pirInsRow(i4 array, i4  index, i4 row, i4  state, i4  which);
i4      IIG4prrRemRow(i4 array, i4  index);
i4      IIG4prvRetVal(i2 *ind, i4  isval, i4  type, i4  length, 
         II_LQTYPE_PTR  data);
i4      IIG4ps4Set4GL(i4 object, i4  code, i2 *ind, i4  isvar, i4  type, 
         i4  length, II_LQTYPE_PTR  data);
i4      IIG4psaSetAttr(char *name, i2 *ind, i4  isvar, i4  type, 
         i4  length, II_LQTYPE_PTR  data);
i4      IIG4pseSendEvent(i4 frame);
i4      IIG4psgSetGlobal(char *name, i2 *ind, i4  isvar, i4  type, 
         i4  length, II_LQTYPE_PTR  data);
i4      IIG4psrSetRow(i4 array, i4  index, i4 row);
i4      IIG4pudUseDscr(i4 language, i4  direction, II_LQTYPE_GPTR *sqd);
i4      IIG4pvpValParam(char *name, i2 *ind, i4  isval, i4  type, 
         i4  length, II_LQTYPE_PTR  data);
