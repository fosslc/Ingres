#include	<compat.h>
#include        <gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include        <adf.h>
#include        <ft.h>
#include        <fmt.h>
#include        <frame.h>
#include        <iisqlda.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

#if defined(UNIX) || defined(NT_GENERIC)

/**
**  Name: iimffrs.c - Provide FRS interface to MF COBOL processor.
**
**  Description:
**	The routines defined in this file provide the FRS interface to the MF
**	COBOL processor.  This file is very dependent on the content and
**	notes of the files libqsys/iimflibq.c and libqsys/iimfdata.c.
**
**  Defines:
**      IIMFfrs		- Load up all FRS calls for MF COBOL.
**
**  Function Interfaces (see notes):
**	IIXactclm
**	IIXactcomm
**	IIXactfld
**	IIXnmuact
**	IIXactscrl
**	IIXchkfrm
**	IIXdispfrm
**	IIXendmu
**	IIXfnames
**	IIXFRtoact
**	IIXnfrskact
**	IIXfsetio
**	IIXinitmu
**	IIXiqset
**	IIXinqset
**	IIXretval
**	IIXrunform
**	IIXvalfld
**	IIXnestmu
**	IIXrunnest
**	IIXendnest
**	IIXtbact
**	IIXtbinit
**	IIXtbsetio
**	IIXtdata
**	IIXtdelrow
**	IIXtinsert
**	IIXtscroll
**	IIXtunload
**	IIXtvalval
**	IIXTBcaClmAct
**	IIXFRafActFld
**
**  Long-name Interfaces (see notes):
**	IIXFRitIsTimeo[t       ]
**	IIXFRaeAlerter[vent    ]	
**
**  Notes:
**	0. Files in MF COBOL run-time layer:
**		iimfdata.c	- Data processing interface (libqsys)
**		iimflibq.c	- Interface to LIBQ (libqsys)
**		iimffrs.c	- Interface to FRS (runsys)
**
**	1. When adding new entries read the file header notes in
**	   libqsys/iimflibq.c.  There are 3 considerations explained:
**		- Regular subroutine calls.
**		- Functions calls.
**		- Long-named external calls.
**
**
**  History:
** 	27-nov-89 (neil)
**	   Written for MF COBOL FRS interface.
**	14-aug-1990 (barbara)
**	    Added (long-name) interface for IIFRsaSetAttrio to support setting
**	    per value attributes on LOADTABLE and INSERTROW (via WITH clause).
**	26-oct-1990 (barbara)
**	    Put in declaration of count argument and a v_args array in
**	    to IIiqset and IIinqset.  EQL/COBOL does generate a count
**	    argument.
**	26-jul-1991 (kathryn)
**	    Remove FRS_64 macro.  FRS functions new to 6.3/03 were incorrectly
**          ifdef'd out by this.  This macro should have been used for pre
**	    6.3/03 development testing only.
**	10-apr-1993 (kathryn)
**          Remove references to obsolete 3.0 routine: IIfmdatio. We no longer
**          support rel. 3.0 calls.
**	08/19/93 (dkh) - Added entry IIFRgotofld to support resume
**			 nextfield/previousfield.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	15-feb-96 (tutto01)
**	    Added Windows NT to the platforms which use this file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-dec-2003 (rigka01)
**	    include sl.h for inclusion of systypes.h which includes
**	    sys\types.h as required by the fix to bug 111096, change 465421
**	    for ade.h use of off_t. 
**	24-Aug-2009 (kschendel) 121804
**	    Prototype a couple of function calls to keep aux-info straight.
*/

FUNC_EXTERN bool IIfrshelp();
FUNC_EXTERN bool IIhelpfile();
FUNC_EXTERN bool IItunload();


/*{
** Name: IIMFfrs 	- Cause loader to see all of FRS entry points.
**
** Description:
**	This routine must never actually be called.  All the external FRS
**	routines are "pretended to be called" by this routine so that the
**	MF COBOL interpreter/loader will pull them in.  This routine can be
**	considered as a load vector.
**
**	An equivalent file exists for the LIBQ calls.
**
** Notes:
**	1. See notes for IIMFlibq on adding a new entry.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** History:
**	27-nov-89 (neil)
**	   Written for MF COBOL interface.
**	25-apr-1991 (teresal)
**	   Added acitvate event function call.
**	12-aug-1992 (kevinm)
**		If dg8_us5 we don't reference IIfmdatio().  IIfmdatio() is
**		a run30 function and dg8_us5 doesn't use run30 because there
**		wasn't a release prior to 6 of INGRES ever shipped.
**      10-apr-1993 (kathryn)
**          Remove references to old 3.0 routine IIfmdatio, it is obsolete.
*/

VOID
IIMFfrs()
{
    static	i4	never_ever_call = 0;

    if (never_ever_call == 0 || never_ever_call != 0)
    {
	IIaddform();
	IIclrflds();
	IIclrscr();
	IIprmptio();
	IIendforms();
	IIendfrm();
	IIfldclear();
	IIforminit();
	IIforms();
	IIfrshelp();
	IIfsinqio();
	IIfssetio();
	IIgetoper();
	IIgtqryio();
	IIhelpfile();
	IIiqfsio();
	IImessage();
	IImuonly();
	IIprnscr();
	IIputoper();
	IIredisp();
	IIrescol();
	IIresfld();
	IIFRgotofld();
	IIresmu();
	IIresnext();
	IIgetfldio();
	IIrunmu();
	IIputfldio();
	IIsleep();
	IIstfsio();
	IIFRgpcontrol();
	IIFRgpsetio();
	IItbsmode();
	IItclrcol();
	IItclrrow();
	IItcogetio();
	IItcoputio();
	IItcolval();
	IItdatend();
	IItfill();
	IIthidecol();
	IItunend();
	IItvalrow();
	IIFRsqDescribe();
	IIFRsqExecute();
	IITBceColEnd();
	IIFRreResEntry();
    } /* If never ever */
    never_ever_call = 0;
} /* IIMFfrs */

/*
** Function Interfaces
*/
VOID
IIXactclm(ret_val, table, column, intr)
i4	*ret_val;
char	*table, *column;
i4	intr;
{
   *ret_val = IIactclm(table, column, intr);
}


VOID
IIXactcomm(ret_val, ctrl, intr)
i4  	*ret_val;
i4	ctrl, intr;
{
    *ret_val = IIactcomm(ctrl, intr);
}

VOID
IIXactfld(ret_val,  fld_name, intr)
i4      *ret_val;
char    *fld_name;
i4	intr;
{
    *ret_val = IIactfld(fld_name, intr);
}

VOID
IIXnmuact(ret_val, menu_name, exp, val, act, intr)
i4      *ret_val;
char	*menu_name, *exp;
i4	val, act, intr;
{
    *ret_val = IInmuact(menu_name, exp, val, act, intr);
}

VOID
IIXactscrl(ret_val, tab_name, mode, intr)
i4      *ret_val;
char	*tab_name;
i4	mode, intr;
{
    *ret_val = IIactscrl(tab_name, mode, intr);
}

VOID
IIXchkfrm(ret_val)
i4  	*ret_val;
{
    *ret_val = IIchkfrm();
}

VOID
IIXdispfrm(ret_val, form_name, mode)
i4      *ret_val;
char    *form_name, *mode;
{
    *ret_val = IIdispfrm(form_name, mode);
}

VOID
IIXendmu(ret_val)
i4	*ret_val;
{
    *ret_val = IIendmu();
}

VOID
IIXfnames(ret_val)
i4	*ret_val;
{
    *ret_val = IIfnames();
}

VOID
IIXFRtoact(ret_val, val, intr)
i4      *ret_val;
i4	val;
i4	intr;
{
	*ret_val = IIFRtoact(val, intr);
}

VOID
IIXnfrskact(ret_val, frsnum, exp, val, act, intr)
i4      *ret_val;
i4	frsnum;
char	*exp;
i4	val, act, intr;
{
    	*ret_val = IInfrskact(frsnum, exp, val, act, intr);
}

VOID
IIXfsetio(ret_val, name)
i4	*ret_val;
char    *name;
{
   *ret_val = IIfsetio(name);
}


VOID
IIXinitmu(ret_val)
i4  	*ret_val;
{
    *ret_val = IIinitmu();
}

VOID
IIXiqset(ret_val, count, objtype, row, arg1, arg2, arg3, arg4)
i4	*ret_val;
i4	count;
i4	objtype, row;
char    *arg1, *arg2, *arg3, *arg4;	    
{
    char	*v_args[4];
    i4		i;

    for (i = 0; i < 4; i++)		/* Initialize varying arg array */
    v_args[i] = (char *)0;

    /* Portable fall-through switch to pick up correct # of args */

    switch(count -2)	/* First two args are actually fixed */
    {
	case 4:	v_args[3] = arg4;
	case 3: v_args[2] = arg3;
	case 2: v_args[1] = arg2;
	case 1: v_args[0] = arg1;
    }

    *ret_val = IIiqset(objtype, row, v_args[0], v_args[1], v_args[2],
			v_args[3]);
}

VOID
IIXinqset(ret_val, count, arg1, arg2, arg3, arg4, arg5)
i4	*ret_val;
i4	count;
char    *arg1, *arg2, *arg3, *arg4, *arg5;	    
{
    char	*v_args[5];
    i4		i;

    for (i = 0; i < 5; i++);		/* Initialize varying arg array */
    v_args[i] = (char *)0;

    /* Portable fall-through switch to pick up corect # of args */
    switch(count)
    {
	case 5:	v_args[4] = arg4;
	case 4:	v_args[3] = arg4;
	case 3:	v_args[2] = arg3;
	case 2:	v_args[1] = arg2;
	case 1:	v_args[0] = arg1;
    }
    *ret_val = IIinqset(v_args[0], v_args[1], v_args[2], v_args[3], 
		v_args[4]);
}

VOID
IIXretval(ret_val)
i4	*ret_val;
{
    *ret_val = IIretval();
}

VOID
IIXrunform(ret_val)
i4	*ret_val;
{
    *ret_val = IIrunform();
}

VOID
IIXvalfld(ret_val, fld_name)
i4	*ret_val;
char    *fld_name;
{
    *ret_val = IIvalfld(fld_name);
}

VOID
IIXnestmu(ret_val)
i4	*ret_val;
{
    *ret_val = IInestmu();
}

VOID
IIXrunnest(ret_val)
i4	*ret_val;
{
    *ret_val = IIrunnest();
}

VOID
IIXendnest(ret_val)
i4	*ret_val;
{
    *ret_val = IIendnest();
}

VOID
IIXtbact(ret_val, form_name, tbl_fld_name, load_flg)
i4	*ret_val;
char	*form_name, *tbl_fld_name;
i4	load_flg;
{
    *ret_val = IItbact(form_name, tbl_fld_name, load_flg);
}

VOID
IIXtbinit(ret_val, form_name, tbl_fld_name, tf_mode)
i4	*ret_val;
char	*form_name, *tbl_fld_name;
char    *tf_mode;
{
    *ret_val = IItbinit(form_name, tbl_fld_name, tf_mode);
}

VOID
IIXtbsetio(ret_val, mode, form_name, tbl_fld_name, row_num)
i4	*ret_val;
i4	mode;
char	*form_name, *tbl_fld_name;
i4	row_num;
{
    *ret_val = IItbsetio(mode, form_name, tbl_fld_name, row_num);
}

VOID
IIXtdata(ret_val)
i4	*ret_val;
{
    *ret_val = IItdata();
}

VOID
IIXtdelrow(ret_val, in_list)
i4	*ret_val;
i4	in_list;
{
    *ret_val = IItdelrow(in_list);
}

VOID
IIXtinsert(ret_val)
i4	*ret_val;
{
    *ret_val = IItinsert();
}

VOID
IIXtscroll(ret_val, in_list, rec_num)
i4	*ret_val;
i4	in_list, rec_num;
{
    *ret_val = IItscroll(in_list, rec_num);
}

VOID
IIXtunload(ret_val)
i4	*ret_val;
{
    *ret_val = IItunload();
}

VOID
IIXtvalval(ret_val, one)
i4	*ret_val;
i4	one;
{
    *ret_val = IItvalval(one);
}

VOID
IIXTBcaClmAct(ret_val, table, column, entry_act, intr)
i4	*ret_val;
char	*table, *column;
i4	entry_act, intr;
{
   *ret_val = IITBcaClmAct(table, column, entry_act, intr);
}

VOID
IIXFRafActFld(ret_val,  fld_name, entry_act, intr)
i4      *ret_val;
char    *fld_name;
i4	entry_act, intr;
{
    *ret_val = IIFRafActFld(fld_name, entry_act, intr);
}

/*
** Long-name Interfaces
12345678901234|
*/

VOID
IIXFRitIsTimeo(ret_val)
i4	*ret_val;
{
    *ret_val = IIFRitIsTimeout();
}

VOID
IIFRsaSetAttri( attr, name, ind, isref, type, len, var_ptr )
i4	attr;
char	*name;
i2	*ind;
i4	isref;
i4	type;
i4	len;
char	*var_ptr;
{
    IIFRsaSetAttrio( attr, name, ind, isref, type, len, var_ptr );
}

VOID
IIXFRaeAlerter(ret_val, intr)
i4      *ret_val;
i4	intr;
{
	*ret_val = IIFRaeAlerterEvent( intr );
}
# endif /* UNIX */
