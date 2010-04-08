/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <gca.h>
# include       <iicgca.h>
# include	<generr.h>
# include	<iirowdsc.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>

/**
+*  Name: iiretdom.c - Routine to fetch data out of a RETRIEVE.
**
**  Defines:
**	IIgetdomio	- Extract data from RETRIEVE into user's variable.
**	IIretdom	- Cover routine for backward compatibility
-*
**  History:
**	16-oct-1986	- Modified to use ULC. (neil)
**	25-feb-1986     - Changed name and interface of the routine (bjb)
**	04-apr-1996     - Added support for compressed variable-length 
**   			  datatypes. (loera01) 
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
+*  Name: IIgetdomio - Get next column in a RETRIEVE.
**
**  Description:
**	Gets the next column in a RETRIEVE from the data area.  At this point
**	we use the current DB value descriptor in ii_rd_desc.  Using this 
**	DB value we load the host variable and execute any conversions needed.
**	If an error occurred previously in the tuple, will not load the host
**	language variable with the value of the domain.
**
**	Note that for string variables, if the data returned is longer
**	than the space allocated to the variable this routine will overwrite
**	the following variables.
**
**  Inputs:
**	indaddr - Address of 2-byte int or NULL
**	isvar   - Better be set to flag this is by reference.
**	hosttyp - Type of variable
**	hostlen - Length of variable (0 or length for strings)
**	addr    - Address of variable
**
**  Outputs:
**	indaddr - Indicates nullness of retrieved data.  If it is a
**		    non-null pointer, the variable it points at will
**		    contain DB_EMB_NULL if the data pointed to by addr
**		    is nul-valued; otherwise it will contain 0.
**	addr	- New value retrieved.
**	Returns:
**	    i4  - 0 - Fail
**		  1 - Success
**	Errors:
**	    Conversion errors.
**
**  Example:
**	## int	 i;
**	## short ind;
**	## float f;
**	## char *c;
**	## retrieve (i = t.icol, f = t.fcol, c = t.ccol);
**
**  Generates (in part):
**	int i;
**	short ind;
**	float f;
**	char *c;
**
**	while (IInextget() != 0) {
**	    IIgetdomio(&ind,1,DB_INT_TYPE,sizeof(i4),&i);
**	    IIgetdomio((short *)0,1,DB_FLT_TYPE,sizeof(f4),&f);
**	    IIgetdomio((short *)0,1,DB_CHR_TYPE,0,c);
**	}
-*
**  Side Effects:
**	1. Increments to the next column descriptor in ii_rd_desc.
**	
**  History:
**	06-feb-1985 - Rewritten (ncg)
**	23-aug-1985 - Modified to also support 4.0 pipeblock descs (ncg)
**	27-sep-1985 - Added conversion type parameter to pb_get calls
**		      to support inter-architecture networking. (mmb)
**	29-oct-1985 - Changed way TEXT works for 4.0 (ncg)
**	16-apr-1986 - Made all calls to pb_get go thru pb_geterr to 
**		      allow errors on column boundaries (ncg)
**	16-oct-1986 - Modified to use LIBQ globals. (ncg)
**	27-oct-1986 - Added calls to support user null indicators. (bjb)
**	26-feb-1987 - Modified to support new null indicator interface (bjb)
**	12-dec-1988 - Generic error processing. (ncg)
**	01-nov-1992 (kathryn)
**	    Added support for Large Objects. Large objects are transmitted
**	    in segments which will be put together and deposited in the
**	    users variable by IILQlvg_LoValueGet.
**	04-apr-1996 - Added support for compressed variable-length datatypes. 
**		      (loera01)
*/

/*VARARGS4*/
i4
IIgetdomio(indaddr, isvar, hosttype, hostlen, addr)
i2		*indaddr;
i4		isvar;
i4		hosttype;
i4		hostlen;
char		*addr;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    DB_EMBEDDED_DATA	edv;
    II_RET_DESC		*rd;
    char		ebuf1[20], ebuf2[20];	/* Error buffers */
    i4			stat;			/* Result of getting data */
    DB_DATA_VALUE	*dbv;
    i4                  flags;

    /* If error then skip this RETRIEVE query */
    if ((isvar==0) || (IIlbqcb->ii_lq_curqry & II_Q_QERR))
	return 0;

    rd = &IIlbqcb->ii_lq_retdesc;
    if (rd->ii_rd_colnum >= rd->ii_rd_desc.rd_numcols)
    {
	CVna(rd->ii_rd_desc.rd_numcols, ebuf1);		/* i4  */
	CVna(rd->ii_rd_colnum+1, ebuf2);		/* i4  */
	IIlocerr(GE_CARDINALITY, E_LQ003C_RETCOLS, II_ERR, 2, ebuf1, ebuf2);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	if (IISQ_SQLCA_MACRO(IIlbqcb))
	    IIsqWarn( thr_cb, 3 );	/* Status information for SQLCA */
	return 0;
    }
    dbv = &rd->ii_rd_desc.RD_DBVS_MACRO(rd->ii_rd_colnum);
    if (IIDB_LONGTYPE_MACRO(dbv))
    {
	stat = IILQlvg_LoValueGet( thr_cb, indaddr, isvar, hosttype, hostlen, 
				   (PTR)addr, dbv, rd->ii_rd_colnum + 1 );
    }
    else
    {
        edv.ed_type = hosttype;
    	edv.ed_length = hostlen;
    	edv.ed_data = (PTR)addr;
    	edv.ed_null = indaddr;

        flags = (rd->ii_rd_desc.rd_flags & RD_CVCH) ? IICGC_VVCH : 0;
    	stat = IIgetdata( thr_cb, 
			  &rd->ii_rd_desc.RD_DBVS_MACRO( rd->ii_rd_colnum ),
			  &edv, rd->ii_rd_colnum +1,flags );
    }
    rd->ii_rd_colnum++;
    if (stat == 0)
	return 0;
    else
	return 1;
}

/*{
+*  Name: IIretdom - Cover routine for IIgetdomio
**
**  Description:
**	This routine is here for backward compatibility, so that old
**	programs may be relinked and still work.  It merely calls
**	the newer data-fetching routine, IIgetdomio.
**
**  Inputs:
**	isvar	- Better b y set to flag this is by reference.
**	hosttyp - Type of variable
**	hostlen - Length of variable
**	addr	- Address of variable
**
**  Outputs:
**	Returns:
**	    Return value of IIgetdomio()
**	Errors:
**	    None
-*
**  History:
**	26-feb-1987 - rewrote as a cover routine (bjb)
*/
i4
IIretdom( isvar, hosttype, hostlen, addr )
i4		isvar;
i4		hosttype;
i4		hostlen;
char		*addr;
{
    return IIgetdomio( (i2 *)0, isvar, hosttype, hostlen, addr );
}
