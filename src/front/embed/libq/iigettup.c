/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<pc.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<iisqlca.h>
# include	<iilibq.h>

/**
+*  Name: iigettup.c - Act like IInextget and IIgetdomio for PARAM RETRIEVE.
**
**  Defines:
**	IItupget	- Process top part of RETRIEVE for PARAM.
**
**  Example:
**	## int	*argv[10];
**	## char *tlist;
**	
**	## retrieve (param(tlist, argv))
**	## {
**		Code;
**	## }
**
**  Generates:
**	int *argv[10];
**	char *tlist;
**
**	IIwritedb("retrieve(");
**	IIparret(tlist, argv,(int *)0);
**	IIwritedb(")");
**	IIretinit((char *)0,0);
**	if (IIerrtest() != 0) goto IIrtE1;
**   IIrtB1:
**	while (IItupget() != 0) {
**	  if (IIerrtest() != 0) goto IIrtB1;
**	  {
**		Code;
**	  }
**	}
**      IIflush((char *)0,0);
**   IIrtE1:;
-*
**  History:
**	09-oct-1986	- Modified to use 6.0. (ncg)
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* External data */
FUNC_EXTERN	i4	IIgetdomio();


/*{
+*  Name: IItupget - Retrieve one PARAM row into host language variables.
**
**  Description:
**	Based on the above example, IItupget works like the top part of
**	a hard coded RETRIEVE - the IInextget call followed by a number of
**	IIgetdomios.  IIretinit has checked for too few or too many columns.
**
**	IIparret set "ii_rd_ptrans" to its functional argument.  If this is not
** 	null then it should point at a function that can convert from a C data
**	type to whatever is needed.  Otherwise the default is used.
**
**  Inputs:
**	Uses static variable array descriptor returned from IIparret.
**
**  Outputs:
**	Returns:
**	   i4  - Return value from IInextget, or IIgetdomio (the latter, only
**		 if failure).
**	Errors:
**	    Not the right number
-*
**  Side Effects:
**	
**  History:
**	06-may-1983	- Put stuff in to get text columns. (lichtman)
**	08-aug-1986	- Added error block processing. (lichtman)
**	26-sep-1984 	- Added length info for C strings for OSL support. (jrc)
**	21-feb-1985 	- Rewritten to use I/O routines. (ncg)
**	23-aug-1985	- Modified to support INGRES 4.0. (ncg)
**	09-oct-1986	- Modified to use 6.0. (ncg)
**	26-feb-1987 	- Modified to support null indicators (bjb)
**	28-feb-92 (leighb) DeskTop Porting Change: cast args to func ptr.
*/

i4
IItupget()
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    i2			nuldata;	/* For use by transfunc */
    i2			*nulptr;
    i4			data[DB_GW4_MAXSTRING/sizeof(i4) +1]; 
    i4			cols;		/* Counter for result columns */
    II_RET_DESC		*ret;		/* RETRIEVE descriptor */
    DB_EMBEDDED_DATA	*ped;		/* Pointer into host vars */

    /* First test entry into RETRIEVE loop */
    if (IInextget() == 0)
	return 0;

    ret = &IIlbqcb->ii_lq_retdesc;

    /* 
    ** Walk through each embedded data value one at a time, picking off the 
    ** calls to make to IIgetdomio and (if necessary) transforming result data 
    ** and null indicator information into host variables. 
    */
    for (ped = ret->ii_rd_param, cols = 0;
	 cols < ret->ii_rd_patts; ped++, cols++)
    {
	if (ret->ii_rd_ptrans)
	{
	    if ((nulptr = ped->ed_null) != (i2 *)0)  /* Need an indicator */
		nulptr = &nuldata;		     /* May be translated */

	    if (IIgetdomio(nulptr, 1, ped->ed_type, ped->ed_length, 
			(char *)data) == 0)
		return 0;

	    /* May need to translate indicator and/or data */
	    if (nulptr)			/* Have an indicator */
		(*ret->ii_rd_ptrans)((i4)DB_INT_TYPE, (i4)sizeof(i2), nulptr,		 
				     ped->ed_null);
	    if (nulptr == (i2 *)0 || *nulptr != DB_EMB_NULL)
		(*ret->ii_rd_ptrans)((i4)(ped->ed_type),(i4)(ped->ed_length),		 
				     data, ped->ed_data);				 
	}
	else
	{
	    if (IIgetdomio(ped->ed_null, 1, ped->ed_type, ped->ed_length, 
		ped->ed_data) == 0)
		return 0;
	}
    }
    return 1;
}
