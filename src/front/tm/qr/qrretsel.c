/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<st.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<eqrun.h>
# include	<qr.h>
# include	<itline.h>
# include	<er.h>
# include	"erqr.h"

/**
** Name:	qrretsel.c -	implement RETRIEVE/SELECT statements
**
** Description:
**	Implements RETRIEVE (QUEL) and SELECT (SQL) statements
**	for the Query Runner
**
**	routines:
**	    doretsel()		- cover routine, execute a RETRIEVE/SELECT
**	    IIqrfetchret()	- RETRIEVE/SELECT a row (or portion of)
**	    static qrubdhdlr()  - 'Data handler' for blob segments
**	    static qrlvcolret() - long varchar row retrieval
**	    static qrdefcolret() - 'default' (bounded) row retrieval
**
** History:
**	22-jul-88 (bruceb)	Fix for bugs 2537 and 2631.
**    	12-Jul-89 (bryanp)
**           	Backend sends us DB_MAXSTRING + DB_CNTSIZE + 1 bytes for
**            	a DB_MAXSTRING byte text column. I think this extra byte is
**            	for holding the 'NULL' flag (on if the value is NULL), but
**            	I can't cite any reference to prove it. The easiest solution
**            	seems to be to allocate a DB_MAXSTRING + DB_CNTSIZE + 1 byte
**            	buffer, rather than a DB_MAXSTRING+DB_CNTSIZE byte buffer.
**	01-aug-89 (teresal)
**		Precision info needs to eventually be passed to ADF (for
**		backend modifications); therefore, fetchret was modified so that
**		it retrieves the precision info and puts it in retval.db_prec.
**	3-oct-92 (rogerl)
**		Rework (6.5) to handle unbounded types.
**	12-nov-92 (rogerl)
**		Revert to allocating the retrieve buffer on the stack -
**		this code is reentrant at \i with a \g in the included file
**		so keeping a static pointer to the buffer is not ok
**	17-nov-92 (rogerl)
**		Removed printatt() from qrlvcolret at flushing buffers
**	11/25/92 (dkh) - Fixed code to pass the address of qriob->dbv rather
**			 than just qriob->dbv.  The latter form causes
**			 the DBV structure to be passed, which is not what
**			 is desired.
**	15-dec-92 (rogerl)
**		Short blob values need to be blank padded so format matches.
**	26-Aug-1993 (fredv)
*8		Included <st.h>.
**	23-apr-1996 (chech02)
**		added function prototypes for windows 3.1 port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-Jun-2009 (thich01)
**          Treat GEOM type the same as LBYTE.
**      20-Aug-2009 (thich01)
**          Treat all spatial types the same as LBYTE.
**	24-Aug-2009 (maspa05) SIR 122324, trac 397
**	    new "truly silent" mode - does not output 'box' lines etc
**          In silent mode don't output leading and trailing separators
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      14-apr-2010 (stial01)
**          Init col_maxname for select blob column
*/

GLOBALREF	bool	TrulySilent;
GLOBALREF	bool	Showtitles;
FUNC_EXTERN	i4	IInextget();
FUNC_EXTERN     VOID    printhor();
FUNC_EXTERN     VOID    printeol();
FUNC_EXTERN     VOID    qrnamfmt( QRB * qrb );
FUNC_EXTERN     VOID    qrputc();
FUNC_EXTERN     VOID	IIflush();
FUNC_EXTERN     VOID	IIretinit();
FUNC_EXTERN     VOID	IItm_retdesc();
FUNC_EXTERN     i4	IIerrtest();
FUNC_EXTERN     i4	IIretdom();

#ifdef WIN16
FUNC_EXTERN     VOID	IIflush(char *, nat);
FUNC_EXTERN     VOID	IIretinit(char *, nat);
FUNC_EXTERN     VOID	IItm_retdesc(PTR);
FUNC_EXTERN     i4	IIretdom(i4, i4, i4, char *);
#endif /* WIN16 */

    /* datahandler i/o package
    */
typedef struct _QRIOB
{
    QRB			* qrb;
    DB_DATA_VALUE	dbv;
} QRIOB;

static	STATUS	qrdefcolret( QRIOB * qriob, i4  i, bool *flushed ,bool firstcol);
static	STATUS	qrlvcolret( QRIOB * qriob, bool *flushed  );
static	VOID	qrubdhdlr( QRIOB * qriob );
static	STATUS	qrrowdesc( QRB * qrb );
STATUS		IIqrfetchret( register QRB *qrb );
VOID		doretsel( QRB * qriob );

    /* token to output in non-tm tools
    */
static char	*blob_token	= NULL;

    /* size of dbv data buffer
    ** for IIretdom(), IILQlgd_LoGetData()... data retrieve
    */
# define QR_DRBSZ ((DB_MAXSTRING/sizeof(ALIGN_RESTRICT)) + 1 + DB_CNTSIZE)


/*{
** Name:	doretsel -  retrieve results, determine and output formatting
**
** Description:
**	Routine has 0-4 states; state maint in qrb->step:
**	state	purpose
**	---------------
**	0	- get output (row) descriptor
**	1	- output top of header
**	2	- output/format column names
**	3	- output final header line
**	4	- fetch a row; at last row, output terminating format
**		  and reset state to 0
**
**	State 4 is repeated as long as the DBMS is sending data.
**
** Inputs:
**	QRB *qrb	- QueryRunner control block.
**
** Side Effects:
**			May put results into qrb output buffer.  May flush
**			results from that buffer to the users device.
**			Eats data from LIBQ.  Flushes pipe when done.
**
** History:
**      30-oct-1992 (rogerl)
**	      rewrite for 6.5 (unbounded data types)
*/

VOID
doretsel(
    QRB	* qrb
) {
     switch( qrb->step )
     {
	case 0:				/* get row descriptor */
	    if ( qrrowdesc( qrb ) == OK )
	    {
		qrinitsz( qrb );
		qrb->step ++;
	    }
	    /* else- bad row, don't pass go (IIflush()?) */
	    break;
	case 1:				/* format top of header lien */
	    /* only show top line if not in silent mode  */
            if (!TrulySilent)
	    {
                qrputc( qrb, '\n' );
                printhor( qrb, DRCH_TT ); 
            }
	    qrb->step ++; 
	    break;
	case 2:				/* format column names  */
	    /* don't show titles if \notitles in effect */
            if (Showtitles)
	        qrnamfmt( qrb ); 
	    qrb->step ++;
	    break;
	case 3:				/* format final header line */
	    /* if no titles or silent mode don't put header line */
            if (Showtitles && !TrulySilent)
   	        printhor( qrb, DRCH_X ); 
	    qrb->step ++; 
	    break;
	case 4:				/* return and output a domain */
	    if ( IIqrfetchret( qrb ) != OK ) /* !OK -> done w/ retrieve */
	    {
	        /* no header for -S flag */
                if (!TrulySilent)
	           printhor( qrb, DRCH_BT );  /* put out final header line */ 

		IIflush( (char *)0, 0 );
		qrb->step = 0;
	    }
	    break;
    }
    return;
}

/*{
** Name:	qrrowdesc() - obtain row descriptor from LIBQ
**
** Description:
**	LIBQ is initialized and then qrb row descriptor is initialized
**
** Inputs:
**	QRB *qrb	- QueryRunner control block.
**
** Side Effects:
**	QRB *qrb	- QueryRunner control block, qrb->rd updated
**
** Returns:
**	FAIL		- Error initalizing LIBQ
**	OK		- Row descriptor retrieved OK
**
** History:
**      30-oct-1992 (rogerl)
**		renamed from preret1(), moved header formatting elsewhere
*/

STATUS
qrrowdesc(
    QRB * qrb
) {
    IIretinit( (char *)0, 0 );

    if ( IIerrtest() != 0 )
	return ( FAIL );

    IItm_retdesc( &qrb->rd );

    return( OK );
}

/*{
** Name:	IIqrfetchret() - fetch 1 tuple of RETRIEVE result
**
** Description:
**	A single tuple of the RETRIEVE (or SELECT) result is fetched
**	from pipeblocks by calling LIBQ routines directly.  The single
**	tuple (if any) is formatted for display and added to the output
**	buffer of the active QRB.  For unbounded types, the QRB output
**	buffers are output at each segment of column to avoid overwhelming
**	the buffers.
**
** Inputs:
**	QRB	*qrb;		// QR control block.
**
** Outputs:
**	QRB	*qrb;		// QR control block (updated).
**
** Returns:
**	STATUS
**		OK	// tuple fetched, RETRIEVE still active.
**		FAIL	// end-of-data reached, no more tuples.
**
** Side Effects:
**	LIBQ routines called read data in pipeblocks.
**
** History:
**   	12-Jul-89 (bryanp)
**            	Backend sends us DB_MAXSTRING + DB_CNTSIZE + 1 bytes for
**            	a DB_MAXSTRING byte text column. I think this extra byte is
**            	for holding the 'NULL' flag (on if the value is NULL), but
**            	I can't cite any reference to prove it. The easiest solution
**            	seems to be to allocate a DB_MAXSTRING + DB_CNTSIZE + 1 byte
**            	buffer, rather than a DB_MAXSTRING+DB_CNTSIZE byte buffer.
**	01-aug-89 (teresal)
**		Modified fetchret to get precision info.
**	12-apr-90 (mgw) Bug # 21121
**		retbuf will contain a DB_DATA_VALUE and must be byte aligned
**		on Sun4.
**	10-oct-92 (rogerl)
**		Rewrite to handle unbounded columns
**	21-sep-1993 (mgw)
**		Add DB_LBYTE_TYPE and DB_LBIT_TYPE cases to switch stmt
**		to trigger a call to qrlvcolret().
**	02-nov-1993 (mgw)
**		Fix ansi C compiler warning about not reaching end of loop
**		by changing while(IInextget()...) to if(IInextget()...).
**		We return out of the loop after the first time through anyway.
**	01-oct-2001 (gupsh01)
** 		Added support for long nvarchar types. [bug 105925].
**
*/


STATUS
IIqrfetchret(
    QRB	* qrb
) {
bool firstcol=TRUE;
register i4	i;
bool		flushed = TRUE;	/* QRB output buffers empty */
ALIGN_RESTRICT	retbuf[ QR_DRBSZ ];
QRIOB		qriob	= {
			(QRB *)NULL,
			{ (PTR)NULL, (i4)0, (DB_DT_ID)0, (i2)0 }
		};

    qriob.qrb = qrb;	/* package up for data handler */
    qriob.dbv.db_data = (PTR) retbuf;
			/* other dbv values will be set by  qr*colret() */

    /* Normally only a single tuple of data is collected; On error or ok
    ** completion, return FAIL clears BE output.  If an unbounded-type column
    ** is found within a tuple, loop on 2k segments of the result, formatting
    ** and flushing for each segment.  (tm only- other callers get token
    ** representing ub data).
    */

    if ( IInextget() != 0 ) 		/* if there's a row to get */
    {
	for (i = 0; i < qrb->rd->rd_numcols; i++)  /* for each column */
	{
			/* get and output the col value.  default is to format
			** into qrb output buffer. type LVCH has output
			** buffer flushed to out at each segment 
			** (II) of col
			*/

	    /* Init col_maxname for for this column */
	    qrb->col_maxname = qrb->dvlen[i].worstlen;

	    switch( abs( qrb->rd->RD_DBVS_MACRO(i).db_datatype ) )
	    {
			    /* return FAIL at err or finish,
			    ** IIflush in doretsel() will clear the pipe
			    */
		case DB_LVCH_TYPE:
		case DB_LBYTE_TYPE:
		case DB_GEOM_TYPE:
                case DB_POINT_TYPE:
                case DB_MPOINT_TYPE:
                case DB_LINE_TYPE:
                case DB_MLINE_TYPE:
                case DB_POLY_TYPE:
                case DB_MPOLY_TYPE:
		case DB_LBIT_TYPE:
		case DB_LNVCHR_TYPE:
		    if ( qrlvcolret( &qriob, &flushed ) == FAIL )
			return( FAIL );
		    break;
		default:
		    /* format column, if first column don't add vertical
		    ** separator if in silent mode
                    */
		    if (i == 0 && TrulySilent)
			firstcol=TRUE;
		    else
			firstcol=FALSE;

		    if ( qrdefcolret( &qriob, i, &flushed, firstcol ) == FAIL )
			return( FAIL );
		    break;
	    }
	}
	/* don't output final vertical separator in silent mode
        ** printeol needs a non-null char so use space
        */
	if (!TrulySilent)
		printeol( qrb, DRCH_V );
	else
		printeol( qrb, ' ');

	return( OK );		/* indicate in active fetch loop */
    }
    return( FAIL );		/* indicate end-of-data */
}


/*{
** Name:	qrdefcolret	- collect and format output for default column
**
** Description:
**	Initialize local dbv with qrb->rd values describing column to retrieve.
**	Collect the column and format into QRB output buffers.
**
** Inputs:
**	QRIOB	*qriob		- static QRIOB data handle
**	i4	i		- column indicator
**	bool	flushed		- qrb output buffers flushed to user or not
**
** Side Effects:
**	Collect column value from LIBQ, formats value into output buffer,
**	flushed set to false.
**
** Returns:
**	STATUS		FAIL	- error on IIretdom()
**			OK	- data successfully formatted into buffer
**
** History:
**	10-oct-92 (rogerl)	written
**	24-aug-2009 (maspa05)	trac 397, SIR 122324
**	    added firstcol parameter - if true then this is the first column
**          don't output a vertical separator in silent mode
*/

static STATUS
qrdefcolret(
    QRIOB	* qriob,
    i4		i,
    bool	* flushed,
    bool        firstcol
) {
		/* init dbv with column attributes */

    qriob->dbv.db_length	= qriob->qrb->rd->RD_DBVS_MACRO(i).db_length;
    qriob->dbv.db_datatype	= qriob->qrb->rd->RD_DBVS_MACRO(i).db_datatype;
    qriob->dbv.db_prec 		= qriob->qrb->rd->RD_DBVS_MACRO(i).db_prec;

		/* pick up fixed length column value */
    IIretdom( 1, DB_DBV_TYPE, 0, &(qriob->dbv) );

    if (IIerrtest() != 0)
	return( FAIL );    /* error, fake end-data to caller */

    printatt( qriob->qrb, &(qriob->dbv), firstcol );

    *flushed = FALSE;

    return( OK );
}

/*{
** Name:	qrlvcolret	- collect, format, and output DB_LVCH_TYPE 
**
** Description:
**	Set local dbv values for LVCH formatting; flush output buffers;
**	call data handler (through LIBQ)
**
** Inputs:
**	QRIOB	*qriob		- static QRIOB data handle
**	bool	flushed		- qrb output buffers flushed to user or not
**
** Side Effects:
**	Flushes QRB output buffers to user.  Sets flushed to TRUE.
**	Initializes output dbv values.  Calls ub data handler.
**
** Returns:
**	STATUS		OK	- data successfully formatted into buffer
**			(currently never returns FAIL)
**
** History:
**	10-oct-92 (rogerl)	written
*/

static STATUS
qrlvcolret(
    QRIOB	* qriob,
    bool	* flushed
) {
short		indvar;

	/* if this isn't TM (others don`t handle blobs) these
	** assignments will be overwritten, but that's OK
	*/
    qriob->dbv.db_datatype	= DB_CHA_TYPE;
    qriob->dbv.db_prec		= 0;
	/* qriob->dbv.db_length filled in by qrubdhldr */

	/* dump cruft output from last column */
    if ( ( ! *flushed ) && ( qriob->qrb->putfunc != NULL ) )
    {
	(*(qriob->qrb->putfunc))( qriob->qrb );      	/* output to user */
	*flushed = TRUE;		 		/* buffers empty */
    }
    IILQldh_LoDataHandler( 1, &indvar, qrubdhdlr, qriob );
   						/* to align formatting,
						** pad field with ' '
    						*/
    if ( indvar == DB_EMB_NULL )	
    {
	qrputc( qriob->qrb, DRCH_V );
	for( indvar = DB_MAXNAME ; indvar > 0; indvar -- )
	    qrputc( qriob->qrb, ' ' );
    }

    return( OK );
}

/*{
** Name:	qrubdhdlr	- handle multiple segment retrieve of ub col
**
** Description:
**	If caller is 'tm', retrieve segments of ub col, format segments into
**	qrb output buffer, flush qrb output buffer to user (for each segment).
**	Other callers get only a token representing the blob value formatted
**	into the qrb output buffers, tell libq to throw data from col
**	away. Assume buffers have been flushed upon entry.
**
**
** Inputs:
**	QRIOB	*qriob		- static QRIOB data handle
**
** Side Effects:
**	Retrieves ub data in segments, treats those segments as if they
**	were rows for purpose of formatting and outputting.
**
** Returns:
**	VOID
**
** History:
**	10-oct-92 (rogerl)	written
**	15-may-93 (kathryn)	Bugs 51841 and 51998
**          Set the db_length field of the DB_DATA_VALUE of the blob segment 
**	    to the actual buffer size before calling IILQlgd_LoGetData so that
**	    LIBQ knows the the length of the buffer.  The segment buffer will
**	    be blank padded by LIBQ, so large objects shorter than GL_MAXNAME
**	    can be formatted correctly by printatt by setting db_length.
**      01-oct-1993 (kathryn)
**          Change value of first parameter in call to IILQlgd_LoGetData().
**          That routine now expects a bit mask. Valid values are defined in
**          eqrun.h (currently II_DATSEG, and II_DATLEN).
*/

static VOID
qrubdhdlr(
    QRIOB	* qriob
) {
i4	seglen = 0;
i4	data_end = 0;
i4	count = 0;

    /* check for NULL data somewhere here and take appropriate action if
    ** found  (will data_end be 1 in this case?)
    */

    if ( qriob->qrb->tm )
    {
	qrputc( qriob->qrb, DRCH_V );  /* begin with a | */
	while ( data_end != 1 )  /* get and put to user 2k or less segs */
	{
	    qriob->dbv.db_length = QR_DRBSZ;
	    IILQlgd_LoGetData((i4)(II_DATLEN|II_DATSEG), DB_DBV_TYPE, QR_DRBSZ,
	    			&(qriob->dbv), QR_DRBSZ, &seglen, &data_end );

	    if (IIerrtest() != 0)
	    {
		IILQled_LoEndData();
		return; 
	    }
	    count++;

	    /* blob col names are formatted into GL_MAXNAME size boxes.
	    /* if entire blob is shorter than column header size, set 
	    ** db_length to column header size (GL_MAXNAME) so that printatt 
	    ** will blank pad correctly.
	    */

	    if (count == 1 && data_end == 1 && seglen < qriob->qrb->col_maxname)
		qriob->dbv.db_length = qriob->qrb->col_maxname;
	    else
	    	qriob->dbv.db_length = seglen;
	    printatt( qriob->qrb, &(qriob->dbv), TRUE );
	    (*(qriob->qrb->putfunc))(qriob->qrb);
	}
    }
    else /* don't output the blobs, this is not 'tm'. token only. */
    {
	    /* don't need to go through printatt in this case,
	    ** no translation or conversion is necessary
	    */
	IILQled_LoEndData();

	    /* first time through, retrieve the token from er file */
	if ( blob_token == NULL )
	    blob_token	= ERget(F_QR011E_Unbounded_data);

	qrputc(qriob->qrb, DRCH_V);
	qradd(qriob->qrb, blob_token, seglen = STlength( blob_token ));
					/* blob col names are formatted into
					** DB_MAXNAME size boxes. the token
					** must be padded or truncated to fit.
					** data_end is not mnemonic here!!
					*/
	for( data_end = ( qriob->qrb->col_maxname - seglen );
			data_end > 0; data_end -- )
	    qrputc(qriob->qrb, ' ');
    }
    return;
}
