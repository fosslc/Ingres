/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<itline.h>
# include	<qr.h>
# include	<er.h>
# include	"erqr.h"
# include 	<me.h>
# include 	<cm.h>
# include 	<st.h>

/**
** Name:	qrformat.c - output formatting routines for QR module
**
** Description:
**	This file contains the formatting routines called by the
**	QR module for data output (RETRIEVE/SELECT results) returned
**	to the caller through the QRB output buffer.  These routines
**	were taken from code originally in the backend, in
**	ovqp/printup.c.	 They have been modified to use ADF style
**	arguments and ADF functions to determine default column
**	widths and to perform conversions to display output format.
**
**	Public (extern) routines defined:
**	    printatt(qrb, dv, segment)
**	    printeol(qrb, endchar)
**	    printhor(qrb, marker)
**	    printhdr(qrb, i, value)
**	    beginhdr(qrb)
**
**	History:
**		09-sep-1986 (peterk)
**			First working version.
**		26-aug-1987 (daver)
**			removed history older than 11 months; changed
**			printatt and printeol to not access qrb->outp directly
**		03-oct-88 (bruceb)
**			Shrink default buffer size in printatt to 2000.
**		07-feb-90 (bruceb)	Fix for bug 7407.
**			Translate any ADF error messge returned from adc_tmcvt
**			rather than stating 'internal error from adc_tmcvt'.
**		02/10/90 (dkh) - Changed to call qradd() (instead of
**				 qrprintf) in printatt() to speed up TM.
**		18-Mar-1992 (fredv)
**			In printhdr(), change the logic to check if value
**			is NULL or not before assigning
**				 p = value->rd_nmbuf
**			otherwise, tm/itm can SEGV if value is NULL.
**		08-oct-1992 (rogerl)
**			dynamically alloc adf output buffer
**		2-feb-1993 (rogerl)
**			Init auto i before first use, delete redundant
**			initialization; bug 45052
**		21-oct-1993 (mgw) Bug #54515
**			Allocate worstlen + 1, not just worstlen in sbuf
**			to accommodate trailing EOS.
**		13-mar-2001 (somsa01)
**			The worst width could potentially be bigger than
**			an i2 now.
**		11-aug-2009 (maspa05) trac 397, SIR 122324
**			in silent mode don't output a vertical separator
**                      at beginning and end of line for column header
**                      implement \trim (\nopadding) to strip white space
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/* Initial buffer size--assumed max width of a tuple; subject to growth */
# define	DFLT_BUFSIZ	2000

VOID            qrnamfmt( QRB   * qrb );

FUNC_EXTERN	i4	afe_errtostr();

static i4	Printlgth = 0;
static char	*sbuf = NULL;
static i4	buflen = DFLT_BUFSIZ;

GLOBALREF    bool    TrulySilent;
GLOBALREF    bool    Padding;

VOID
printatt(qrb, dv, segment)
QRB		*qrb;
DB_DATA_VALUE	*dv;
bool		segment;
{
	DB_DATA_VALUE	ltxtout;
	i4		outlen;
	DB_STATUS	status;
	i4		deflen, worstlen;
	i4		errlen;
	char		errbuf[ER_MAX_LEN + 1];
	char		*trimd_sbuf ;

	if ( ! segment )  	/* no separators - this is a column segment */
	    qrputc(qrb, DRCH_V);

	status = adc_tmlen(qrb->adfscb, dv, &deflen, &worstlen);
	if (status != E_DB_OK)
	{
	    errlen = ER_MAX_LEN + 1;
	    _VOID_ afe_errtostr(qrb->adfscb, errbuf, &errlen);
	    qrprintf(qrb, errbuf);
	    return;
	}
		/* we may end up allocating a larger buffer every time
		in (up to max worstlen); but this would be an unusual
		worst case.  so, allocate buffer, if it needs to get
		bigger, free the old one and allocate a new one
		*/
	if (worstlen > buflen)
	{
	    if (sbuf != NULL)  /* free up the old buffer */
		MEfree(sbuf);
				/* and get a bigger one */
	    sbuf = (char *)MEreqmem((u_i4)0, (u_i4)worstlen + 1,
	    				FALSE, &status);
	    buflen = worstlen;
	}
	else if (sbuf == NULL)
	{
		/* first time in, allocate a buffer */
	    sbuf = MEreqmem((u_i4)0, (u_i4)buflen + 1, FALSE, &status);
	    buflen++;
	}
	/* else, we've got at least as much as we need already */

	ltxtout.db_data = (PTR)sbuf;
	ltxtout.db_length = buflen;
	ltxtout.db_datatype = DB_LTXT_TYPE;
	status = adc_tmcvt(qrb->adfscb, dv, &ltxtout, &outlen);
	if (status != E_DB_OK || outlen > buflen)
	{
	    errlen = ER_MAX_LEN + 1;
	    _VOID_ afe_errtostr(qrb->adfscb, errbuf, &errlen);
	    qrprintf(qrb, errbuf);
	    return;
	}

	sbuf[outlen] = EOS;

	/* if in padding mode just output what we got from the server
        ** otherwise strip the leading and trailing spaces
        */
	if (Padding)
		qradd(qrb, sbuf,outlen);
	else
	{
		trimd_sbuf=sbuf;
		while (CMwhite(trimd_sbuf))
			trimd_sbuf++;
		qrtrimwhite(trimd_sbuf);
		outlen=STlength(trimd_sbuf);
		qradd(qrb, trimd_sbuf, outlen);
	}
	
}

VOID
printeol(qrb, endchar)
QRB	*qrb;
char	endchar;
{
	qrputc(qrb, endchar);
	qrputc(qrb, '\n');
}

VOID
printhor(qrb, marker)
QRB	*qrb;
i4	marker;
{
	register i4	i;
	register char	begin, end;

	switch (marker)
	{
	    case DRCH_TT:
		begin = DRCH_UL;
		end = DRCH_UR;
		break;

	    case DRCH_BT:
		begin = DRCH_LL;
		end = DRCH_LR;
		break;

	    case DRCH_X:
		begin = DRCH_LT;
		end = DRCH_RT;
		break;

	    default:
		begin = end = DRCH_V;
	}

	for (i = 0; i < qrb->rd->rd_numcols; i++)
	{
	    register i4	j;
	    qrputc(qrb, i? marker: begin);
	    for (j = 0; j < qrb->dvlen[i].deflen; j++)
	    {
		qrputc(qrb, DRCH_H);
	    }
	}

	printeol(qrb, end);
}

VOID
printhdr(qrb, j, value)
QRB		*qrb;
register i4	j;
register ROW_COLNAME	*value;
{
	register i4	i = 0;
	register char	*p;

	if ( !TrulySilent || j != 0 )
		qrputc(qrb, DRCH_V);

	if (value != NULL)
	    {
	    p = value->rd_nmbuf;

	    /* if padding then make the title the column width
            ** othewise make it the full title length
            */
	    if (Padding)
	        for ( ; i < qrb->dvlen[j].deflen && i < value->rd_nmlen; i++)
		        qrputc(qrb, *p++);
            else
	        for ( ; i < value->rd_nmlen; i++)
		        qrputc(qrb, *p++);
	    }
	if (Padding)
		for ( ; i < qrb->dvlen[j].deflen; i++)
	    		qrputc(qrb, ' ');
}

/*{
** Name:	qrinitsz() - init the deflen and worstlen for each row val
**
** Description:
**	Depending upon the values in the row descriptor of this qrb row
**	descriptor (previously obtained by IIretinit()), init each of the
**	associated deflen and worstlen vals by calling adc_tmlen()
**
** Inputs:
**	QRB *qrb	- QueryRunner control block.
**
** Side Effects:
**	QRB *qrb	- QueryRunner control block updated with col names and
**			  column name box
**
** Returns:
**	VOID
**
** History:
**      30-oct-1992 (rogerl)
**		Renamed from beginhdr, moved formatting portion into qrnamfmt;
**		move from qrretsel.c to here.
**	21-sep-1993 (mgw)
**		Add other LONG datatypes to test for setting deflen and
**		worstlen. It would be nice if IIDT_LONGTYPE_MACRO() were
**		someplace handy like fe.h where we could get at it instead
**		of in embed!hdr!iilibq.h where we can't. Then we could use
**		that instead of listing all the datatypes separately.
**      01-oct-2001 (gupsh01)
**              Added support for long nvarchar types. [bug 105925].
*/

VOID
qrinitsz(
	QRB	* qrb
) {
	register i4	i;
	STATUS		status;
	i4		errlen;
	char		errbuf[ER_MAX_LEN + 1];
	DB_DT_ID	qrdatatype;

	for (i = 0; i < qrb->rd->rd_numcols; i++)
	{
			/* dblvch always is DB_ATT_MAXNAME */
	    qrdatatype = abs( qrb->rd->RD_DBVS_MACRO(i).db_datatype );
	    if ( qrdatatype == DB_LVCH_TYPE ||
	         qrdatatype == DB_LBYTE_TYPE ||
	         qrdatatype == DB_LBIT_TYPE  || 
		 qrdatatype == DB_LNVCHR_TYPE )
	    {
		if (qrb->rd->rd_names[i].rd_nmlen < DB_OLDMAXNAME_32)
		{
		    qrb->dvlen[i].deflen = DB_OLDMAXNAME_32; 
		    qrb->dvlen[i].worstlen = DB_OLDMAXNAME_32;
		}
		else
		{
		    qrb->dvlen[i].deflen = DB_ATT_MAXNAME; 
		    qrb->dvlen[i].worstlen = DB_ATT_MAXNAME; 
		}
	    }
	    else
	    {
		status = adc_tmlen(qrb->adfscb,&qrb->rd->RD_DBVS_MACRO(i),
		    &qrb->dvlen[i].deflen, &qrb->dvlen[i].worstlen);
		if ( status != E_DB_OK )
		{
		    errlen = ER_MAX_LEN + 1;
		    _VOID_ afe_errtostr(qrb->adfscb, errbuf, &errlen);
		    qrprintf(qrb, errbuf);
		    return;
		}
	    }
	}
	return;
}

/*{
** Name:	qrnamfmt() - format column names for header
**
** Description:
**	The result "names header" is formatted and added to the 
**	QRB output buffer.
**
** Inputs:
**	QRB *qrb	- QueryRunner control block.
**
** Side Effects:
**	QRB *qrb	- QueryRunner control block updated with col names and
**			  line formatting 
**
** Returns:
**	VOID
**
** History:
**      30-oct-1992 (rogerl)
**		Rework for unbounded types (renamed from preret2(), and moved
**		from qrretsel.c to here.
**      24-aug-2009 (maspa05)  SIR 122324, trac 397
**		In silent mode end the line with a space rather than the 
**		separator char
*/

VOID
qrnamfmt(
    QRB	* qrb
) {
    register i4		i;

    for ( i = 0; i < qrb->rd->rd_numcols; i++ )
	printhdr( qrb, i, ( qrb->rd->rd_flags & RD_NAMES
                       ? &qrb->rd->rd_names[i]
                       : NULL ) );

    if (!TrulySilent)
    	printeol(qrb, DRCH_V);
    else
    	printeol(qrb, ' ');
}
