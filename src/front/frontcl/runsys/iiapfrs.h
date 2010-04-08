/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**      IIAPFRS.H - ANSI C Funtion Prototypes for the FRS runtime layer
**                  & function declarations for the existing FRS
**                  calls.
**
**              This header file differs from eqdefc.h because it
**              uses the data types defined in compat.h. Except for:
**
**              	o PTR is represented by II_LQTYPE_PTR.
**              	o II_LQTYPE_GPTR replaces char * where appropriate.
**
**              This file contains the IIpxxx() routine names.
**
**              Porting notes:
**
**                      Ingres PTR is translated into II_LQTYPE_PTR.
**
**      History:
**              20-apr-1994     (teresal)
**                              Written for adding ESQL/C++ and '-prototypes'
**                              for ESQL/C.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Jan-2007 (kiria01) b117277
**	    Add prototyped function support for IIset_random
*/

# define II_LQTYPE_PTR          void *
# define II_LQTYPE_GPTR         void *

i4	IIpactcomm(i4 commval,i4 val);
i4	IIpactscrl(char *tabname,i4 mode,i4 val);
i4	IIpaddform(II_LQTYPE_GPTR frame);
i4	IIpchkfrm(void);
i4	IIpclrflds(void);
i4	IIpclrscr(void);
i4	IIpdispfrm(char *nm,char *md);
VOID	IIpendforms(void);
i4	IIpendfrm(void);
i4	IIpendmu(void);
VOID	IIpendnest(void);
i4	IIpfldclear(char *strvar);
i4	IIpfnames(void);
i4	IIpforminit(char *nm);
i4	IIpforms(i4 language);
i4	IIpFRaeAlerterEvent(i4 intr_val);
i4	IIpFRafActFld(char *strvar,i4 entry_act,
		i4 val);
VOID	IIpFRgotofld(i4 dir);
VOID	IIpFRgpcontrol(i4 state,i4 alt);
VOID	IIpFRgpsetio(i4 pid,i2 *nullind,
		bool isvar,i4 type,i4 len,
		II_LQTYPE_PTR val);
VOID	IIpFRInternal(i4 dummy);
i4	IIpFRitIsTimeout(void);
i4	IIpFRreResEntry(void);
VOID	IIpFRsaSetAttrio(i4 fsitype,char *cname, 
		i2 *nullind,i4 isvar,
		i4 datatype,i4 datalen,
		II_LQTYPE_PTR value);
bool	IIpfrshelp(i4 type,char *subj,char *flnm);
VOID	IIpFRsqDescribe(i4 lang,i4 is_form,
		char *form_name,char *table_name,char *mode,
		II_LQTYPE_GPTR sqd);
VOID	IIpFRsqExecute(i4 lang,i4 is_form,
		i4 is_input,II_LQTYPE_GPTR sqd);
i4	IIpFRtoact(i4 timeout,i4 intr_val);
i4	IIpfsetio(char *nm);
i4	IIpfsinqio(i2 *nullind,i4 variable,
		i4 type,i4 len,II_LQTYPE_GPTR data,
		char *name);
i4	IIpfssetio(char *name,i2 *nullind,
		i4 isvar,i4 type,i4 len,
		II_LQTYPE_PTR data);
i4	IIpgetfldio(i2 *ind,i4 variable,
		i4 type,i4 len,II_LQTYPE_PTR data,
		char *name);
i4	IIpgetoper(i4 set);
i4	IIpgtqryio(i2 *ind,i4 isvar,
		i4 type,i4 len,i4 *variable,
		char *name);
bool	IIphelpfile(char *subj,char *flnm);
i4	IIpinitmu(void);
i4	IIpinqset(char *object,char *p0,char *p1,char *p2,char *p3);
i4	IIpiqfsio(i2 *nullind,i4 isvar,
		i4 type,i4 length,II_LQTYPE_PTR data,
		i4 attr,char *name,i4 row);
i4	IIpiqset(i4 objtype,i4 row,
		char *formname,char *fieldname,char *term);
i4	IIpmessage(char *buf);
i4	IIpmuonly(void);
i4	IIpnestmu(void);
i4	IIpnfrskact(i4 frsknum,char *exp,i4 val,
		i4 act,i4 intrval);
i4	IIpnmuact(char *strvar,char *exp,i4 val,
		i4 act,i4 intrp);
VOID	IIpprmptio(i4 noecho,char *prstring,
		i2 *nullind,i4 isvar,i4 type,
		i4 length,II_LQTYPE_PTR data);
VOID	IIpprnscr(char *filename);
i4	IIpputfldio(char *s1,i2 *ind,i4 isvar,
		i4 type,i4 len,II_LQTYPE_PTR data);
i4	IIpputoper(i4 set);
i4	IIpredisp(void);
i4	IIprescol(char *tabname,char *colname);
i4	IIpresfld(char *strvar);
i4	IIpresmu(void);
i4	IIpresnext(void);
i4	IIpretval(void);
i4	IIprf_param(char *infmt,char **inargv,
		i4 (*transfunc)());
i4	IIprunform(void);
i4	IIprunmu(i4 dispflg);
i4	IIprunnest(void);
i4	IIpsf_param(char *infmt,char **inargv,
		i4 (*transfunc)());
i4	IIpsleep(i4 sec);
i4	IIpstfsio(i4 attr,char *name,i4 row,
		i2 *nullind,i4 isvar,i4 type,
		i4 length,II_LQTYPE_PTR tdata);
i4	IIptbact (char *frmnm,char *tabnm,i4 loading);
i4	IIpTBcaClmAct(char *tabname,char *colname,
		i4 entry_act,i4 val);
VOID	IIpTBceColEnd(void);
i4	IIptbinit (char *formname,char *tabname,char *tabmode);
i4	IIptbsetio(i4 cmnd,char *formname,char *tabname,
		i4 rownum);
i4	IIptbsmode(char *modestr);
i4	IIptclrcol(char *colname);
i4	IIptclrrow(void);
i4	IIptcogetio(i2 *ind,i4 variable,
		i4 type,i4 len,II_LQTYPE_PTR data,
		char *colname);
i4	IIptcolval(char *colname);
i4	IIptcoputio(char *colname,i2 *ind,
		i4 variable,i4 type,
		i4 len,II_LQTYPE_PTR data);
i4	IIptdata(void);
i4	IIptdatend(void);
i4	IIptdelrow(i4 tofill);
i4	IIptfill(void);
i4	IIpthidecol(char *colname,char *coltype);
i4	IIptinsert(void);
i4	IIptrc_param(char *infmt,char **inargv,
		i4 (*transfunc)());
i4	IIptscroll(i4 tofill,i4 recnum);
i4	IIptsc_param(char *infmt,char **inargv,
		i4 (*transfunc)());
i4	IIptunend(void);
bool	IIptunload(void);
i4	IIptvalrow(void);
i4	IIptvalval(i4 set);
i4	IIpvalfld(char *strvar);

/* Existing FRS function declarations */

i4	IIactcomm();
i4	IIactscrl();
i4	IIaddform();
i4	IIchkfrm();
i4	IIclrflds();
i4	IIclrscr();
i4	IIdispfrm();
VOID	IIendforms();
i4	IIendfrm();
i4	IIendmu();
VOID	IIendnest();
i4	IIfldclear();
i4	IIfnames();
i4	IIforminit();
i4	IIforms();
i4	IIFRaeAlerterEvent();
i4	IIFRafActFld();
VOID	IIFRgotofld();
VOID	IIFRgpcontrol();
VOID	IIFRgpsetio();
VOID	IIFRInternal();
i4	IIFRitIsTimeout();
i4	IIFRreResEntry();
VOID	IIFRsaSetAttrio();
bool	IIfrshelp();
VOID	IIFRsqDescribe();
VOID	IIFRsqExecute();
i4	IIFRtoact();
i4	IIfsetio();
i4	IIfsinqio();
i4	IIfssetio();
i4	IIgetfldio();
i4	IIgetoper();
i4	IIgtqryio();
bool	IIhelpfile();
i4	IIinitmu();
i4	IIinqset();
i4	IIiqfsio();
i4	IIiqset();
i4	IImessage();
i4	IImuonly();
i4	IInestmu();
i4	IInfrskact();
i4	IInmuact();
VOID	IIprmptio();
VOID	IIprnscr();
i4	IIputfldio();
i4	IIputoper();
i4	IIredisp();
i4	IIrescol();
i4	IIresfld();
i4	IIresmu();
i4	IIresnext();
i4	IIretval();
i4	IIrf_param();
i4	IIrunform();
i4	IIrunmu();
i4	IIrunnest();
i4	IIsf_param();
VOID	IIset_random();
i4	IIsleep();
i4	IIstfsio();
i4	IItbact();
i4	IITBcaClmAct();
VOID	IITBceColEnd();
i4	IItbinit();
i4	IItbsetio();
i4	IItbsmode();
i4	IItclrcol();
i4	IItclrrow();
i4	IItcogetio();
i4	IItcolval();
i4	IItcoputio();
i4	IItdata();
i4	IItdatend();
i4	IItdelrow();
i4	IItfill();
i4	IIthidecol();
i4	IItinsert();
i4	IItrc_param();
i4	IItscroll();
i4	IItsc_param();
i4	IItunend();
bool	IItunload();
i4	IItvalrow();
i4	IItvalval();
i4	IIvalfld();
