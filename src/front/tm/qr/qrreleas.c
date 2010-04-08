/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<qr.h>

/**
** Name:	qrrelease.c - close down QR and release resources.
**
** Description:
**
**	Contains the routine to close down QR module and release
**	any resources that have been allocated.  Currently this
**	includes only the statement echo buffer.
**
**	Public (extern) routines:
**		qrrelease(qrb)
**
** History:
**/

/*{
** Name:	qrrelease() -- close down QR and release resources
**
** Description:
**	Any QR resources, such as the statement echo buffer (qrb->stmt)
**	are de-allocated.
**
** Inputs:
**	QRB	*qrb;		// QR control block.
**
** Outputs:
**	QRB	*qrb;		// QR control block (updated).
**
** Returns:
**	STATUS
**		OK		// success
**		FAIL		// error freeing stmt or output buffer.
**
** Side Effects:
**	Memory (poinbted to by qrb->stmt) is freed, memory
**
** History:
**	10/86 (peterk) -- created.
**	26-aug-87 (daver) released the output buffer if needed.
**	02-01-1992 (kathryn)
**	    Add call to "unset" default database event and database procedure
**	    message handlers.  
**	8-oct-1992 (rogerl)
**	    output buffer always dynamically alloc'd, delete growbuf; if
**	    either free fails return FAIL.  Init the output buffers.
*/

STATUS
qrrelease(qrb)
register QRB	*qrb;
{
	STATUS	r1, r2;

	r1 = MEfree(qrb->stmt);
	r2 = MEfree(qrb->outbuf);

	qrb->stmt = qrb->s = NULL;
	qrb->outbuf = qrb->outp = qrb->outlin = NULL;
	qrb->outlen = 0;

	IItm(FALSE);
	IIQRsqhSetQrHandlers(FALSE);

	if ( r1 == FAIL || r2 == FAIL )
		return( FAIL );
	return( OK );
}
