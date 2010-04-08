/*
** Copyright (c) 2004 Ingres Corporation
*/

#include        <compat.h>
#include        "iiapfrs.h"

/*
**  Name: iiacfrs.c - Provide FRS function prototype interface for ANSI C
**                      and C++ 3GL preprocessors.
**
**  Description:
**      The is the runtime layer for providing prototyped FRS functions
**      to ESQL/C and ESQL/C++. This file defines function interface
**      calls for all FRS routines. The functions defined here are
**      covers for the old-style non-prototyped FRS functions.
**
**      ESQL/C will call this layer only when a 3GL program is compiled
**      will the '-prototypes' preprocessor flag. This layer is
**	dependent on the 3GL source file being compiled with the
**	eqdefc.h and eqpname.h header files.
**
**      An ESQL/C++ program will always go through this layer. An
**	ESQL/C++ source files is compiled with the eqdefcc.h and eqpname.h
**	header files.
**
**	FYI: The LIBQ prototyped layer is in frontcl!libqsys!iiaclibq.c
**
**  History:
**      22-apr-94 (teresal)
**         Written for ESQL/C++ and ANSI ESQL/C support.
**	06-may-94 (geri)
**	   Bug 63166: Corrected IIpendfrm() to call IIendfrm (not itself!)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Jan-2007 (kiria01) b117277
**	    Add prototyped function support for IIset_random
**	23-Feb-2007 (kiria01) b117277
**	    Removed superflous return from IIpset_random as not needed
**	    and considered an error by some compilers.
*/

/*
** IMPORTANT NOTE:
**
**      When adding functions to this file, use Ingres typedefs
**      like i4  for data types with the following exceptions:
**
**              Use II_LQTYPE_PTR instead of Ingres's PTR. This allows
**              us to override PTR to use a generic pointer like "void *'.
**              For example, Unix defines PTR to be 'char *', but ANSI
**              C prototypes will object if we try to send/return a pointer
**              to something other than a char.
**
**              Use II_LQTYPE_GPTR in places where we are pointing to
**              a structure like FRAME or when we really mean to use a
**              generic pointer. Look at IIaddform() for an example.
*/

i4	
IIpactcomm(i4 commval,i4 val)
{
	return IIactcomm(commval,val);
}

i4	
IIpactscrl(char *tabname,i4 mode,i4 val)
{
	return IIactscrl(tabname,mode,val);
}

i4	
IIpaddform(II_LQTYPE_GPTR frame)
{
	return IIaddform(frame);
}

i4	
IIpchkfrm(void)
{
	return IIchkfrm();
}

i4	
IIpclrflds(void)
{
	return IIclrflds();
}

i4	
IIpclrscr(void)
{
	return IIclrscr();
}

i4	
IIpdispfrm(char *nm,char *md)
{
	return IIdispfrm(nm,md);
}

VOID	
IIpendforms(void)
{
	IIendforms();
}

i4	
IIpendfrm(void)
{
	return IIendfrm();
}

i4	
IIpendmu(void)
{
	return IIendmu();
}

VOID	
IIpendnest(void)
{
	IIendnest();
}

i4	
IIpfldclear(char *strvar)
{
	return IIfldclear(strvar);
}

i4	
IIpfnames(void)
{
	return IIfnames();
}

i4	
IIpforminit(char *nm)
{
	return IIforminit(nm);
}

i4	
IIpforms(i4 language)
{
	return IIforms(language);
}

i4	
IIpFRaeAlerterEvent(i4 intr_val)
{
	return IIFRaeAlerterEvent(intr_val);
}

i4	
IIpFRafActFld(char *strvar,i4 entry_act,i4 val)
{
	return IIFRafActFld(strvar,entry_act,val);
}

VOID	
IIpFRgotofld(i4 dir)
{
	IIFRgotofld(dir);
}

VOID	
IIpFRgpcontrol(i4 state,i4 alt)
{
	IIFRgpcontrol(state,alt);
}

VOID	
IIpFRgpsetio(i4 pid,i2 *nullind, bool isvar,i4 type,i4 len,
		II_LQTYPE_PTR val)
{
	IIFRgpsetio(pid,nullind,isvar,type,len,val);
}

VOID	
IIpFRInternal(i4 dummy)
{
	IIFRInternal(dummy);
}

i4	
IIpFRitIsTimeout(void)
{
	return IIFRitIsTimeout();
}

i4	
IIpFRreResEntry(void)
{
	return IIFRreResEntry();
}

VOID	
IIpFRsaSetAttrio(i4 fsitype,char *cname,i2 *nullind,i4 isvar,
		i4 datatype,i4 datalen,II_LQTYPE_PTR value)
{
	IIFRsaSetAttrio(fsitype,cname,nullind,isvar,datatype,datalen,value);
}

bool	
IIpfrshelp(i4 type,char *subj,char *flnm)
{
	return IIfrshelp(type,subj,flnm);
}

VOID	
IIpFRsqDescribe(i4 lang,i4 is_form, char *form_name,char *table_name,
		char *mode, II_LQTYPE_GPTR sqd)
{
	IIFRsqDescribe(lang,is_form,form_name,table_name,mode,sqd);
}

VOID	
IIpFRsqExecute(i4 lang,i4 is_form, i4  is_input,II_LQTYPE_GPTR sqd)
{
	IIFRsqExecute(lang,is_form,is_input,sqd);
}

i4	
IIpFRtoact(i4 timeout,i4 intr_val)
{
	return IIFRtoact(timeout,intr_val);
}

i4	
IIpfsetio(char *nm)
{
	return IIfsetio(nm);
}

i4	
IIpfsinqio(i2 *nullind,i4 variable, i4  type,i4 len,II_LQTYPE_GPTR data,
		char *name)
{
	return IIfsinqio(nullind,variable,type,len,data,name);
}

i4	
IIpfssetio(char *name,i2 *nullind, i4  isvar,i4 type,i4 len,
		II_LQTYPE_PTR data)
{
	return IIfssetio(name,nullind,isvar,type,len,data);
}

i4	
IIpgetfldio(i2 *ind,i4 variable, i4  type,i4 len,II_LQTYPE_PTR data,
		char *name)
{
	return IIgetfldio(ind,variable,type,len,data,name);
}

i4	
IIpgetoper(i4 set)
{
	return IIgetoper(set);
}

i4	
IIpgtqryio(i2 *ind,i4 isvar,i4 type,i4 len,i4 *variable,char *name)
{
	return IIgtqryio(ind,isvar,type,len,variable,name);
}

bool	
IIphelpfile(char *subj,char *flnm)
{
	return IIhelpfile(subj,flnm);
}

i4	
IIpinitmu(void)
{
	return IIinitmu();
}

i4	
IIpinqset(char *object,char *p0,char *p1,char *p2,char *p3)
{
	return IIinqset(object,p0,p1,p2,p3);
}

i4	
IIpiqfsio(i2 *nullind,i4 isvar,i4 type,i4 length,II_LQTYPE_PTR data,
		i4 attr,char *name,i4 row)
{
	return IIiqfsio(nullind,isvar,type,length,data,attr,name,row);
}

i4	
IIpiqset(i4 objtype,i4 row,char *formname,char *fieldname,char *term)
{
	return IIiqset(objtype,row,formname,fieldname,term);
}

i4	
IIpmessage(char *buf)
{
	return IImessage(buf);
}

i4	
IIpmuonly(void)
{
	return IImuonly();
}

i4	
IIpnestmu(void)
{
	return IInestmu();
}

i4	
IIpnfrskact(i4 frsknum,char *exp,i4 val, i4  act,i4 intrval)
{
	return IInfrskact(frsknum,exp,val,act,intrval);
}

i4	
IIpnmuact(char *strvar,char *exp,i4 val,i4 act,i4 intrp)
{
	return IInmuact(strvar,exp,val,act,intrp);
}

VOID	
IIpprmptio(i4 noecho,char *prstring,i2 *nullind,i4 isvar,i4 type,
		i4 length,II_LQTYPE_PTR data)
{
	IIprmptio(noecho,prstring,nullind,isvar,type,length,data);
}

VOID	
IIpprnscr(char *filename)
{
	IIprnscr(filename);
}

i4	
IIpputfldio(char *s1,i2 *ind,i4 isvar,i4 type,i4 len,II_LQTYPE_PTR data)
{
	return IIputfldio(s1,ind,isvar,type,len,data);
}

i4	
IIpputoper(i4 set)
{
	return IIputoper(set);
}

i4	
IIpredisp(void)
{
	return IIredisp();
}

i4	
IIprescol(char *tabname,char *colname)
{
	return IIrescol(tabname,colname);
}

i4	
IIpresfld(char *strvar)
{
	return IIresfld(strvar);
}

i4	
IIpresmu(void)
{
	return IIresmu();
}

i4	
IIpresnext(void)
{
	return IIresnext();
}

i4	
IIpretval(void)
{
	return IIretval();
}

i4	
IIprf_param(char *infmt,char **inargv, i4  (*transfunc)())
{
	return IIrf_param(infmt,inargv,transfunc);
}

i4	
IIprunform(void)
{
	return IIrunform();
}

i4	
IIprunmu(i4 dispflg)
{
	return IIrunmu(dispflg);
}

i4	
IIprunnest(void)
{
	return IIrunnest();
}

i4	
IIpsf_param(char *infmt,char **inargv, i4  (*transfunc)())
{
IIpsf_param(infmt,inargv,transfunc);
}

VOID
IIpset_random(i4 seed)
{
	IIset_random(seed);
}

i4	
IIpsleep(i4 sec)
{
	return IIsleep(sec);
}

i4	
IIpstfsio(i4 attr,char *name,i4 row, i2 *nullind,i4 isvar,i4 type,
		i4 length,II_LQTYPE_PTR tdata)
{
	return IIstfsio(attr,name,row,nullind,isvar,type,length,tdata);
}

i4	
IIptbact (char *frmnm,char *tabnm,i4 loading)
{
	return IItbact (frmnm,tabnm,loading);
}

i4	
IIpTBcaClmAct(char *tabname,char *colname,i4 entry_act,i4 val)
{
	return IITBcaClmAct(tabname,colname,entry_act,val);
}

VOID	
IIpTBceColEnd(void)
{
	IITBceColEnd();
}

i4	
IIptbinit (char *formname,char *tabname,char *tabmode)
{
	return IItbinit (formname,tabname,tabmode);
}

i4	
IIptbsetio(i4 cmnd,char *formname,char *tabname,i4 rownum)
{
	return IItbsetio(cmnd,formname,tabname,rownum);
}

i4	
IIptbsmode(char *modestr)
{
	return IItbsmode(modestr);
}

i4	
IIptclrcol(char *colname)
{
	return IItclrcol(colname);
}

i4	
IIptclrrow(void)
{
	return IItclrrow();
}

i4	
IIptcogetio(i2 *ind,i4 variable,i4 type,i4 len,II_LQTYPE_PTR data,
		char *colname)
{
	return IItcogetio(ind,variable,type,len,data,colname);
}

i4	
IIptcolval(char *colname)
{
	return IItcolval(colname);
}

i4	
IIptcoputio(char *colname,i2 *ind, i4  variable,i4 type,
		i4 len,II_LQTYPE_PTR data)
{
	return IItcoputio(colname,ind,variable,type,len,data);
}

i4	
IIptdata(void)
{
	return IItdata();
}

i4	
IIptdatend(void)
{
	return IItdatend();
}

i4	
IIptdelrow(i4 tofill)
{
	return IItdelrow(tofill);
}

i4	
IIptfill(void)
{
	return IItfill();
}

i4	
IIpthidecol(char *colname,char *coltype)
{
	return IIthidecol(colname,coltype);
}

i4	
IIptinsert(void)
{
	return IItinsert();
}

i4	
IIptrc_param(char *infmt,char **inargv,i4 (*transfunc)())
{
	return IItrc_param(infmt,inargv,transfunc);
}

i4	
IIptscroll(i4 tofill,i4 recnum)
{
	return IItscroll(tofill,recnum);
}

i4	
IIptsc_param(char *infmt,char **inargv,i4 (*transfunc)())
{
	return IItsc_param(infmt,inargv,transfunc);
}

i4	
IIptunend(void)
{
	return IItunend();
}

bool	
IIptunload(void)
{
	return IItunload();
}

i4	
IIptvalrow(void)
{
	return IItvalrow();
}

i4	
IIptvalval(i4 set)
{
	return IItvalval(set);
}

i4	
IIpvalfld(char *strvar)
{
	return IIvalfld(strvar);
}
