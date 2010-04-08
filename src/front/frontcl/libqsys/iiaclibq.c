/*
** Copyright (c) 2004 Ingres Corporation
*/

#include 	<compat.h>
#include 	"iiaplibq.h"

/*
**  Name: iiaclibq.c - Provide LIBQ function prototype interface for ANSI C
**		        and C++ 3GL preprocessors.
**
**  Description:
**      The is the runtime layer for providing prototyped LIBQ functions
**	to ESQL/C and ESQL/C++. This file defines function interface
**	calls for all LIBQ routines. The functions defined here are 
**	covers for the old-style non-prototyped LIBQ functions.
**
**	ESQL/C will call this layer only when a 3GL program is compiled
**	will the '-prototypes' preprocessor flag.
**
**	An ESQL/C++ program will always go through this layer.
**
**	FYI: The FRS prototyped layer is in frontcl!runsys!iiacfrs.c
**
**  History:
**      22-apr-94 (teresal)
**         Written for ESQL/C++ and ANSI ESQL/C support.
**	05-may-94 (teresal)
**	   Fix funxtion prototypes for IIseterr and IILQldh_LoDataHandler.
**	   What we documented to users differed from the function definiton
**	   in LIBQ. Make the function prototypes consistent so ESQL 
**	   programs will link without errors (bugs 63295 and 63202).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-08-2002 (zhahu02)
**          Modified for IIpLQlgd_LoGetData (b108458/INGSRV1808).
**	6-Jun-2006 (kschendel)
**	    Added describe input.
**      09-apr-2007 (drivi01)
**          BUG 117501
**          Adding references to function IIcsRetScroll in order to fix
**          build on windows.
**      09-oct-2007 (huazh01)
**          Call IILQprsProcStatus() with correct number of parameter.
**          This fixes bug 119273.
**      09-oct-2007 (huazh01)
**          For functions that are not supposed to return void, add key
**          word 'return' to ensure it indeed returns the result to its 
**          caller function!
**          This fixes b119261. 
**      17-oct-2007 (huazh01)
**          update IIpLQprsProcStatus(), it nows takes one parameter. 
**          This fixes b119273. 
*/

/*
** IMPORTANT NOTE: 
**
**	When adding functions to this file, use Ingres typedefs 
**	like i4  for data types with the following exceptions:
**
**		Use II_LQTYPE_PTR instead of Ingres's PTR. This allows 
**		us to override PTR to use a generic pointer like "void *'. 
**		For example, Unix defines PTR to be 'char *', but ANSI 
**		C prototypes will object if we try to send/return a pointer 
**		to something other than a char like in IILQdbl(). 
**
**		Use II_LQTYPE_GPTR in places where we are pointing to 
**		a structure like SQLCA or when we really mean to use a 
**		generic pointer. Look at IIsqInit() for an example.
*/

void	
IIpbreak(void)
{
	IIbreak();
}

void	
IIpcsClose(char *cursor_name,i4 num1,i4 num2)
{
	IIcsClose(cursor_name,num1,num2);
}

void	
IIpcsDaGet(i4 lang,II_LQTYPE_GPTR sqd)
{
	IIcsDaGet(lang,sqd);
}

void	
IIpcsDelete(char *table_name,char *cursor_name,i4 num1,i4 num2)
{
	IIcsDelete(table_name,cursor_name,num1,num2);
}

void	
IIpcsERetrieve(void)
{
	IIcsERetrieve();
}

void	
IIpcsERplace(char *cursor_name,i4 num1,i4 num2)
{
	IIcsERplace(cursor_name,num1,num2);
}

void	
IIpcsGetio(i2 *indaddr,i4 isvar,i4 type,i4 length,II_LQTYPE_GPTR address)
{
	IIcsGetio(indaddr,isvar,type,length,address);
}

void	
IIpcsOpen(char *cursor_name,i4 num1,i4 num2)
{
	IIcsOpen(cursor_name,num1,num2);
}

void	
IIpcsParGet(char *target_list,char **argv,i4 (*transfunc)())
{
	IIcsParGet(target_list,argv,transfunc);
}

void	
IIpcsQuery(char *cursor_name,i4 num1,i4 num2)
{
	IIcsQuery(cursor_name,num1,num2);
}

void	
IIpcsRdO(i4 what,char *rdo)
{
	IIcsRdO(what,rdo);
}

void	
IIpcsReplace(char *cursor_name,i4 num1,i4 num2)
{
	IIcsReplace(cursor_name,num1,num2);
}

i4	
IIpcsRetrieve(char *cursor_name,i4 num1,i4 num2)
{
	return IIcsRetrieve(cursor_name,num1,num2);
}

i4
IIpcsRetScroll(char *cursor_name, i4 num1, i4 num2, i4 fetcho, i4 fetchn)
{
	return IIcsRetScroll(cursor_name, num1, num2, fetcho, fetchn);
}

void	
IIpeqiqio(i2 *indaddr,i4 isvar,i4 type,i4 len,II_LQTYPE_PTR addr,char *name)
{
	IIeqiqio(indaddr,isvar,type,len,addr,name);
}

void	
IIpeqstio(char *name, i2 *indaddr,i4 isvar,i4 type,i4 len,II_LQTYPE_PTR data)
{
	IIeqstio(name,indaddr,isvar,type,len,data);
}

i4	
IIperrtest(void)
{
        return IIerrtest();
}

void	
IIpexDefine(i4 what,char *name,i4 num1,i4 num2)
{
	IIexDefine(what,name,num1,num2);
}

i4	
IIpexExec(i4 what,char *name,i4 num1,i4 num2)
{
	return IIexExec(what,name,num1,num2);
}

void	
IIpexit(void)
{
	IIexit();
}

void	
IIpflush(char *file_nm,i4 line_no)
{
	IIflush(file_nm,line_no);
}

i4	
IIpgetdomio(i2 *indaddr,i4 isvar,i4 hosttype,i4 hostlen,
		II_LQTYPE_GPTR addr)
{
	return IIgetdomio(indaddr,isvar,hosttype,hostlen,addr);
}

VOID	
IIpingopen(i4 lang,char *p1,char *p2,char *p3,char *p4,char *p5,char *p6,
	char *p7,char *p8,char *p9,char *p10,char *p11,char *p12,
	char *p13,char *p14,char *p15)
{
	IIingopen(lang,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,*p12,p13,p14,p15);
}

VOID	
IIpLQcnConName(char *con_name)
{
	IILQcnConName(con_name);
}

void *	
IIpLQdbl(double dblval)
{
	return (II_LQTYPE_PTR)IILQdbl(dblval);
}

VOID	
IIpLQesEvStat(i4 escall,i4 eswait)
{
	IILQesEvStat(escall,eswait);
}

void *
IIpLQint(i4 intval)
{
	return (II_LQTYPE_PTR)IILQint(intval);
}

void	
IIpLQisInqSqlio(i2 *indaddr,i4 isvar,i4 type,i4 len,II_LQTYPE_PTR addr,
	i4 attrib)
{
	IILQisInqSqlio(indaddr,isvar,type,len,addr,attrib);
}

void	
IIpLQldh_LoDataHandler(i4 type,i2 *indvar,i4 (*datahdlr)(void *),
	II_LQTYPE_PTR hdlr_arg)
{
	IILQldh_LoDataHandler(type,indvar,datahdlr,hdlr_arg);
}

i4	
IIpLQled_LoEndData(void)
{
	return IILQled_LoEndData();
}

void	
IIpLQlgd_LoGetData(i4 flags,i4 hosttype,i4 hostlen,char *addr,
		i4 maxlen,i4 *seglen, i4  *dataend)
{
	IILQlgd_LoGetData(flags,hosttype,hostlen,addr,maxlen,seglen,dataend);
}

void	
IIpLQlpd_LoPutData(i4 flags,i4 hosttype,i4 hostlen,char *addr,
	i4 seglen, i4  dataend)
{
	IILQlpd_LoPutData(flags,hosttype,hostlen,addr,seglen,dataend);
}

VOID	
IIpLQpriProcInit(i4 proc_type,char *proc_name)
{
	IILQpriProcInit(proc_type,proc_name);
}

i4	
IIpLQprsProcStatus(i4 resultCount)
{
	return IILQprsProcStatus(resultCount);
}

VOID	
IIpLQprvProcValio(char *param_name,i4 param_byref,i2 *indaddr,i4 isref,
	i4 type,i4 length,II_LQTYPE_PTR addr1, ...)
{
	IILQprvProcValio(param_name,param_byref,indaddr,isref,
		type,length,addr1);
}

void	
IIpLQshSetHandler(i4 hdlr,i4 (*funcptr)())
{
	IILQshSetHandler(hdlr,funcptr);
}

VOID	
IIpLQsidSessID(i4 session_id)
{
	IILQsidSessID(session_id);
}

void	
IIpLQssSetSqlio(i4 attrib,i2 *indaddr,i4 isvar,i4 type,i4 len,
		II_LQTYPE_PTR data)
{
	IILQssSetSqlio(attrib,indaddr,isvar,type,len,data);
}

i4	
IIpnexec(void)
{
	return IInexec();
}

i4	
IIpnextget(void)
{
	return IInextget();
}

i4	
IIpparret(char *tlist,char **argv,i4 (*xfunc)())
{
	return IIparret(tlist,argv,xfunc);
}

i4	
IIpparset(char *string,char **argv,char *(*transfunc)())
{
	return IIparset(string,argv,transfunc);
}

VOID	
IIpputctrl(i4 ctrl)
{
	IIputctrl(ctrl);
}
	
void	
IIpputdomio(i2 *indaddr,i4 isvar,i4 type,i4 length,II_LQTYPE_GPTR addr)
{
	IIputdomio(indaddr,isvar,type,length,addr);
}

void	
IIpretinit(char *file_nm,i4 line_no)
{
	IIretinit(file_nm,line_no);
}

/* IIseterr() in LIBQ is really defined with the error handler function
** returning a long with an argument that is a pointer to a long, but we have 
** always documented to users that they should define their error handler
** to return an int.
**
** Bug fix 63205 - make the prototyped version return an int so
** we don't get an argument mismatch at compile time.
*/
i4 (*IIpseterr(i4 (*proc)(i4 *errno)))()
{
	return IIseterr(proc);
}

void	
IIpsexec(void)
{
	IIsexec();
}

void	
IIpsqConnect(i4 lang,char *db,char *a1,char *a2,char *a3,
		char *a4,char *a5,char *a6,char *a7,char *a8,char *a9,
		char *a10,char *a11, char *a12,char *a13)
{
	IIsqConnect(lang,db,a1,a2,a3,a4,a5,a6,a7,a8,a9,
		a10,a11,a12,a13);
}

void	
IIpsqDaIn(i4 lang, II_LQTYPE_GPTR sqd)
{
	IIsqDaIn(lang, sqd);
}

void 	
IIpsqDescInput(i4 lang,char *stmt_name,II_LQTYPE_GPTR sqd)
{
	IIsqDescInput(lang,stmt_name,sqd);
}

void 	
IIpsqDescribe(i4 lang,char *stmt_name,II_LQTYPE_GPTR sqd,i4 using_flag)
{
	IIsqDescribe(lang,stmt_name,sqd,using_flag);
}

void	
IIpsqDisconnect(void)
{
	IIsqDisconnect();
}

void	
IIpsqExImmed(char *query)
{
	IIsqExImmed(query);
}

void	
IIpsqExStmt(char *stmt_name,i4 using_vars)
{
	IIsqExStmt(stmt_name,using_vars);
}

void	
IIpsqFlush(char *filename,i4 linenum)
{
	IIsqFlush(filename,linenum);
}

VOID	
IIpsqGdInit(i2 sqlsttype,char *sqlstdata)
{
	IIsqGdInit(sqlsttype,sqlstdata);
}

void	
IIpsqInit(II_LQTYPE_GPTR sqc)
{
	IIsqInit(sqc);
}

void	
IIpsqMods(i4 mod_flag)
{
	IIsqMods(mod_flag);
}

VOID	
IIpsqParms(i4 flag,char * name_spec,i4 val_type,char *str_val, i4  int_val)
{
	IIsqParms(flag,name_spec,val_type,str_val,int_val);
}

void	
IIpsqPrepare(i4 lang,char *stmt_name,II_LQTYPE_GPTR sqd,
		i4 using_flag,char *query)
{
	IIsqPrepare(lang,stmt_name,sqd,using_flag,query);
}

VOID	
IIpsqPrint(II_LQTYPE_GPTR sqlca)
{
	IIsqPrint(sqlca);
}

void	
IIpsqStop(II_LQTYPE_GPTR sqc)
{
	IIsqStop(sqc);
}

void	
IIpsqTPC(i4 highdxid,i4 lowdxid)
{
	IIsqTPC(highdxid,lowdxid);
}

void	
IIpsqUser(char * usename)
{
	IIsqUser(usename);
}

VOID	
IIpsqlcdInit(II_LQTYPE_GPTR sqc,i4 *sqlcd)
{
	IIsqlcdInit(sqc,sqlcd);
}

void	
IIpsyncup(char *file_nm,i4 line_no)
{
	IIsyncup(file_nm,line_no);
}

i4	
IIptupget(void)
{
	return IItupget();
}

void	
IIputsys(i4 uflag,char *uname,char *uval)
{
	IIutsys(uflag,uname,uval);
}

void	
IIpvarstrio(i2 *indnull,i4 isvar,
		i4 type,i4 length,II_LQTYPE_GPTR addr)
{
	IIvarstrio(indnull,isvar,type,length,addr);
}

void	
IIpwritio(i4 trim,i2 *ind_unused,i4 isref_unused,i4 type,
		i4 length,char *qry)
{
	IIwritio(trim,ind_unused,isref_unused,type,length,qry);
}

void	
IIpxact(i4 what)
{
	IIxact(what);
}



/* 4GL Runtime System Calls */

i4      
IIG4pacArrayClear(i4 array)
{
         return IIG4acArrayClear(array);

}


i4      
IIG4pbpByrefParam(i2 *ind, i4  isval, i4  type, i4  length,
         II_LQTYPE_PTR   data, char *name)
{

        return  IIG4bpByrefParam(ind, isval, type, length, data, name);
}


i4      
IIG4pccCallComp()
{

         return IIG4ccCallComp();
}


i4      
IIG4pchkobj(i4 object, i4  access, i4  index, i4  caller)
{
        return  IIG4chkobj(object, access, index, caller);
}

i4      
IIG4pdrDelRow(i4 array, i4  index)
{
         return IIG4drDelRow(array, index);
}

i4      
IIG4pfdFillDscr(i4 object, i4  language, II_LQTYPE_GPTR *sqd)
{
         return IIG4fdFillDscr(object, language, sqd);
}

i4      
IIG4pgaGetAttr(i2 *ind, i4  isvar, i4  type, i4  length,
         II_LQTYPE_PTR  data, char *name)
{
         return IIG4gaGetAttr(ind, isvar, type, length, data, name);
}

i4      
IIG4pggGetGlobal(i2 *ind, i4  isvar, i4  type, i4  length,
         II_LQTYPE_PTR  data, char *name, i4  gtype)
{
         return IIG4ggGetGlobal(ind, isvar, type, length, data, name, gtype);
}

i4      
IIG4pgrGetRow(i2 *rowind, i4  isvar, i4  rowtype, i4  rowlen,
         II_LQTYPE_PTR  rowptr, i4 array, i4  index)
{
         return IIG4grGetRow(rowind, isvar, rowtype, rowlen, rowptr, array, index);
}

i4      
IIG4pi4Inq4GL(i2 *ind, i4  isvar, i4  type, i4  length,
         II_LQTYPE_PTR  data, i4 object, i4  code)
{
         return IIG4i4Inq4GL(ind, isvar, type, length, data, object, code);
}

i4      
IIG4picInitCall(char *name, i4  type)
{
         return IIG4icInitCall(name, type);
}

i4      
IIG4pirInsRow(i4 array, i4  index, i4 row, i4  state, i4  which)
{
         return IIG4irInsRow(array, index, row, state, which);
}

i4      
IIG4prrRemRow(i4 array, i4  index)
{
         return IIG4rrRemRow(array, index);
}

i4      
IIG4prvRetVal(i2 *ind, i4  isval, i4  type, i4  length,
         II_LQTYPE_PTR  data)
{
         return IIG4rvRetVal(ind, isval, type, length, data);
}

i4      
IIG4ps4Set4GL(i4 object, i4  code, i2 *ind, i4  isvar, i4  type,
         i4  length, II_LQTYPE_PTR  data)
{
         return IIG4s4Set4GL(object, code, ind, isvar, type, length, data);
}

i4      
IIG4psaSetAttr(char *name, i2 *ind, i4  isvar, i4  type,
         i4  length, II_LQTYPE_PTR  data)
{
         return IIG4saSetAttr(name, ind, isvar, type, length, data);
}

i4      
IIG4pseSendEvent(i4 frame)
{
         return IIG4seSendEvent(frame);
}

i4      
IIG4psgSetGlobal(char *name, i2 *ind, i4  isvar, i4  type,
         i4  length, II_LQTYPE_PTR  data)
{
         return IIG4sgSetGlobal(name, ind, isvar, type, length, data);
}

i4      
IIG4psrSetRow(i4 array, i4  index, i4 row)
{
         return IIG4srSetRow(array, index, row);
}

i4      
IIG4pudUseDscr(i4 language, i4  direction, II_LQTYPE_GPTR *sqd)
{
         return IIG4udUseDscr(language, direction, sqd);
}

i4      
IIG4pvpValParam(char *name, i2 *ind, i4  isval, i4  type,
         i4  length, II_LQTYPE_PTR  data)
{
         return IIG4vpValParam(name, ind, isval, type, length, data);
}

