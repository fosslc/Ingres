/*	
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<qr.h>
/**
** Name:	qrinit.c - QR initialization routine
**
** Description:
**
**	Contains initialization routine for QR module which fills
**	in the QRB control block structure.
**
**	Setting/saving/restoring of LIBQ error/display handlers is
**	done in qrrun().
**
**	Public (extern) routines:
**		qrinit(qrb, semi, buf, infunc, contfunc, errfunc, script, 
**				echo, outbuf, adfscb, flfunc, flparam)
**
** History:
** 	08-sep-86 (peterk) Initial version
**	13-may-87 (daver) added flfunc, flparam parameters to qrinit.
**	12-aug-88 (bruceb)
**		Added putfunc parameter to qrinit.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      02-Apr-2003 (hweho01) 
**          Modified the parameter list in qrinit(), make it match with 
**          the prototype declaration in qr.h file. Avoid compiler error 
**          "Redeclaration of qrinit differs from previous declaration" 
**          on AIX.
**	11-Nov-2009 (wanfr01) b122875
**	    Added Outisterm paramter 
**/
/*{
** Name:	qrinit() -- initialize QR module control block.
**
** Description:
**	The Query Runner Control Block structure (QRB) is initialized
**	and filled in with passed parameters.
**
** Inputs:
**	QRB	*qrb;		// ptr to allocated control block.
**	char	*inbuf;		// if != 0, the query input string.
**	char	(*infunc)();	// if != NULL, an input function
**				   to be called with no args to
**				   return a single input character.
**	STATUS	(*contfunc)();	// if != NULL, a "continue-function"
**				   which will be invoked with the
**				   QRB as argument after each query
**				   is executed.
**	i4	(*errfunc)();	// if != NULL, a LIBQ error handler
**				   function, which will be established
**				   as the LIBQ error handler during
**				   QR module calls;
**				   if errfunc == NULL a standard QR
**				   error function will be used.
**	FILE	*script;	// if != NULL, a descriptor for a
**				   scripting file to record query
**				   input and output.
**	i4	echo;		// (boolean) flags whether to echo
**				   query statements to standard output
**				   as they are recognized by QR.
**	DB_DATA_VALUE *outbuf;	// represents the output buffer for
**				   this QR session; outbuf->db_data
**				   should point to a character buffer
**				   whose length is given by 
**				   outbuf->db_length.
**	ADF_CB	*adfscb;	// an initialized ADF control block
**				   language setting is taken from .adf_qlang.
**
**	VOID	(*flfunc)();	// if != NULL, a function which will handle
**				   printing of error messages. it will be
**				   passed the block of memory in flparam,
**				   below, and the error message passed to 
**				   qrerror.
**
**	PTR	flparam;	// the block of memory which contains 
**				   whatever context needed for printing an
**				   error message. this is passed to the
**				   routine specified by flfunc, above.
**
**	VOID	(*putfunc)();	// if != NULL, a function which will handle
**				   put'ing out (writing) normal data.
**
**	bool	tm;		// true if caller is tm, false otherwise
**				   IIqrfetchret() needs to know the difference
**
**	bool	Outisterm;	// true if output/stdout is to terminal
**
** Outputs:
**	QRB	*qrb;		// members initialized and filled in.
**
** Returns:
**	STATUS
**		OK		// success
**		FAIL		// both inbuf and infunc were NULL
**
** Side Effects:
**	IItm(TRUE) is called to cause ULC to set the "column-names" bit.
**	IIQRsqhSetQrHandlers called to set dbevent and message handlers.
**
** History:
**	9/86 (peterk) -- created.
**	3/31/87 (peterk) -- added call to IItm();
**	12-feb-88 (neil) -- semis always required in sql (logic was wrong).
**	20-apr-89 (kathryn) -- No longer initialize qrb->severity.  This
**		will be set in [monitor]go.qsc  [fstm]fsrun.qsc.
**		User may now change this via the [no]continue command.
**	01-aug-91 (kathryn)
**		Initialize new qrb field qrb->striptok.
**	02-01-1992 (kathryn)
**	    Add call to IIQRsqhSetQrHandlers, this registers default  database 
**	    event and database procedure message handlers.
**	1-oct-1992 (rogerl)
**	    Add 'tm' since IIqrfetchret() needs to know if caller is
**	    TM for blobs display.  Delete growbuf.
**	22-jan-1993 (rogerl)
**	    contfunc returns a STATUS.
**      25-sep-96 (mcgem01)
**          Global data moved to qrdata.c
*/

GLOBALREF QRSD	noStmtDesc;

STATUS
qrinit(
register QRB	*qrb,
bool	semi,
char	*inbuf,
char	(*infunc)(),
STATUS	(*contfunc)(),
i4	(*errfunc)(),
FILE	*script,
bool	echo,
register DB_DATA_VALUE 	*outbuf,
ADF_CB	*adfscb,
VOID	(*flfunc)(),
PTR	flparam,
VOID	(*putfunc)(),
bool	tm, 
bool	Outisterm )
{
	i4		qrerror();
	/* one of inbuf or infunc must be non-null */
	if (!inbuf && !infunc)
		return FAIL;
	/* initializations */
	qrb->s = qrb->stmt = NULL;
	qrb->stmtbufsiz = 0;
	qrb->sfirst = -1;
	*qrb->token = qrb->peek = '\0';
	qrb->t = qrb->token;
	qrb->striptok = FALSE;
	qrb->sno = 0;
	qrb->nonnull = FALSE;
	qrb->nobesend = FALSE;
	qrb->norowmsg = FALSE;
	qrb->stmtdesc = qrb->nextdesc = &noStmtDesc;
	qrb->rd = NULL;
	qrb->step = qrb->lines = qrb->stmtoff = 0;
	qrb->comment = qrb->eoi = FALSE;
	qrb->error = 0;
	qrb->errmsg = NULL;
	qrb->saveerr = qrb->savedisp = NULL;
	/* set parameters */
	qrb->lang = adfscb->adf_qlang;
	qrb->nosemi = qrb->lang == DB_SQL? FALSE: TRUE;
	qrb->inbuf = inbuf;
	qrb->infunc = infunc;
	qrb->contfunc = contfunc;
	qrb->errfunc = errfunc? errfunc: qrerror;
	qrb->flushfunc = flfunc;
	qrb->flushparam = flparam;
	qrb->putfunc = putfunc;
	qrb->script = script;
	qrb->echo = echo;
	qrb->endtrans = FALSE;
	qrb->outbuf = outbuf->db_data;
	qrb->outlin = qrb->outp = qrb->outbuf;
	qrb->outlen = outbuf->db_length;
	qrb->adfscb = adfscb;
	qrb->tm = tm;
	qrb->Outisterm = Outisterm;
	IIQRsqhSetQrHandlers(TRUE);
	IItm(TRUE);
	return OK;
}
