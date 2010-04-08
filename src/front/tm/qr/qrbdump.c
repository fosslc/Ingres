
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
# include	<er.h>

/**
** Name:	qrbdump.c - QR debug output
**
** Description:
**
**	Contains routine for dumping QRB (Query Runner control Block)
**	structure.
**
**	Public (extern) routines:
**		qrbdump(qrb)
**
** History:	
**	11-aug-1987 (daver)
**	Added ERx calls to string extraction.
**	15-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**
 * Revision 60.24  87/04/08  01:39:41  joe
 * Added compat, dbms and fe.h
 * 
 * Revision 60.23  87/04/02  14:16:08  peterk
 * changed QRB to use ROW_DESC structure for tuple descriptor.
 * 
 * Revision 60.22  86/12/31  11:24:47  peterk
 * change over to 6.0 CL headers
 * 
 * Revision 60.21  86/11/06  10:00:48  peterk
 * mods reflecting changes in QRB struct for 6.0 LIBQ.
 * 
 * Revision 60.20  86/10/14  13:07:39  peterk
 * working version 2.
 * modified to reflect changes in QRB struct.
 * 
 * Revision 60.1  86/10/01  10:33:01  peterk
 * initial version in RCS.
 * 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	qrbdump - debug printout a QRB structure
**
** Description:
**	dump a printout of a QRB strucutre to stderr.
**
** Inputs:
**	QRB *qrb	- QueryRunner control block.
**
** Outputs:
**	none
**
** Returns:
**	nothing
**
** Side effects:
**	formatted printout sent to stdout.
**
** Exceptions:
**	nothing
**
** History:
**	8-25-86 (peterk) - created.
**	8-02-89 (teresal) - added precision info to be displayed.
*/

qrbdump(q)
register QRB	*q;
{
	SIfprintf(stderr, 
		ERx("QRB @ x%p\n"), q);
	SIfprintf(stderr, 
		ERx("  lang		-> %d\n"), q->lang);
	SIfprintf(stderr, 
		ERx("  inbuf		-> x%p\n"), q->inbuf);
	if (q->inbuf)
	    SIfprintf(stderr, 
		ERx("    -> %s\n"), q->inbuf);
	SIfprintf(stderr, 
		ERx("  infunc	-> x%p\n"), q->infunc);
	SIfprintf(stderr, 
		ERx("  contfunc	-> x%p\n"), q->contfunc);
	SIfprintf(stderr, 
		ERx("  errfunc	-> x%p\n"), q->errfunc);
	SIfprintf(stderr, 
		ERx("  script	-> x%p\n"), q->script);
	SIfprintf(stderr, 
		ERx("  echo		-> %d\n"), q->echo);
	SIfprintf(stderr, 
		ERx("  nosemi	-> %d\n"), q->nosemi);
	SIfprintf(stderr, 
		ERx("  saveerr	-> x%p\n"), q->saveerr);
	SIfprintf(stderr, 
		ERx("  savedisp	-> x%p\n"), q->savedisp);
	SIfprintf(stderr, 
		ERx("  token	-> %s\n"), q->token);
	SIfprintf(stderr, 
		ERx("  t		-> x%p\n"), q->t);
	SIfprintf(stderr, 
		ERx("  stmt		-> %s\n"), q->stmt);
	SIfprintf(stderr, 
		ERx("  stmtbufsiz	-> %d\n"), q->stmtbufsiz);
	SIfprintf(stderr, 
		ERx("  s		-> x%p\n"), q->s);
	SIfprintf(stderr, 
		ERx("  sno		-> %d\n"), q->sno);
	SIfprintf(stderr, 
		ERx("  stmtdesc	-> x%p\n"), q->stmtdesc);
	SIfprintf(stderr, 
		ERx("  nextdesc	-> x%p\n"), q->nextdesc);
	SIfprintf(stderr, 
		ERx("  lines		-> %d\n"), q->lines);
	SIfprintf(stderr, 
		ERx("  stmtoff	-> %d\n"), q->stmtoff);
	SIfprintf(stderr, 
		ERx("  step		-> %d\n"), q->step);
	SIfprintf(stderr, 
		ERx("  peek		-> %d '%c'\n"), q->peek, q->peek);
	SIfprintf(stderr, 
		ERx("  nonnull	-> %d\n"), q->nonnull);
	SIfprintf(stderr, 
		ERx("  eoi		-> %d\n"), q->eoi);
	SIfprintf(stderr, 
		ERx("  rd		-> x%p\n"), q->rd);
	if (q->rd)
	{
	    register i4	i;

	    SIfprintf(stderr, ERx("    rd_numcols	-> %d\n"), q->rd->rd_numcols);
	    for (i = 0; i < q->rd->rd_numcols; i++)
	    {
		SIfprintf(stderr, ERx("    [%d]	-> %d %d %d x%p]\n"),
		    i, q->rd->RD_DBVS_MACRO(i).db_datatype,
		    q->rd->RD_DBVS_MACRO(i).db_length,
		    q->rd->RD_DBVS_MACRO(i).db_prec,
		    q->rd->RD_DBVS_MACRO(i).db_data);
	    }
	}
	SIfprintf(stderr, 
		ERx("  adfscb	-> x%p\n"), q->adfscb);
	SIfprintf(stderr, 
		ERx("  error		-> %d\n"), q->error);
	SIfprintf(stderr, 
		ERx("  severity	-> %d\n"), q->severity);
	SIfprintf(stderr, 
		ERx("  errmsg	-> %s\n"), q->errmsg);
	SIfprintf(stderr, 
		ERx("  outbuf	-> x%p \"%s\n"), q->outbuf, q->outbuf);
	SIfprintf(stderr, 
		ERx("  outlen	-> %d\n"), q->outlen);
	SIfprintf(stderr, 
		ERx("  outlin	-> x%p \"%s\n"), q->outlin, q->outlin);
	SIfprintf(stderr, 
		ERx("  outp		-> x%p\n"), q->outp);
}
