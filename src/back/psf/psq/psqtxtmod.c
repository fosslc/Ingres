/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <cm.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/**
**
**  Name: PSQTXTMOD.C - Functions for modifying query text
**
**  Description:
**      When defining a view, permit, or integrity, INGRES puts the text of
**	the defining query in the iiqrytext relation.  The text can't be
**	copied verbatim, however.  It can contain column names and table names;
**	the rename table and rename column commands (not yet implemented)
**	would make the query text obsolete unless we change to some sort of
**	id instead.  This file contains functions for putting pieces of
**	query text into a linked list, substituting for pieces, and copying
**	the modified query text into QSF as a text object.
**
**          psq_topen	    - Open a query text chain
**	    psq_tadd	    - Add a piece of text to a chain
**	    psq_tclose	    - Close a query text chain
**	    psq_tsubs	    - Substitute one piece for another in a chain
**	    psq_tout	    - Put a query text chain in QSF
**	    psq_tcnvt	    - convert any db_data_value to text db_data_value
**	    psq_tinsert	    - Insert a piece of text in front of some piece in
**			      the chain
**	    psq_tbacktrack  - Return a pointer to the N-th previous piece from
**			      a given piece in the text chain
**	    psq_rptqry_text - add a piece of text to the text chain for a repeat
**			      query;
**	    psq_1rptqry_out - Put a repeat query text chain into a contiguous
**			      block of QSF memory
**	    psq_tmulti_out  - almost like psq_tout() except that it uses a
**			      preopened stream and returns a DB_TEXT_STRING
**			      pointer to the result
**	    psq_tdelete	    - delete specified element from the text chain.
**
**
**  History:
**      18-jul-86 (jeff)
**          written
**	 2-jun-88 (stec)
**	    Modified for DB procs (added psq_tb1add).
**	17-apr-89 (jrb)
**	    Modified to initialization prec field(s) correctly for decimal
**	    project.
**	04-jun-90 (andre)
**	    Wrote psq_tbacktrack(). Renamed psq_tb1add() to psq_tinsert().
**	18-nov-91 (rog)
**	    Fixed bug 40869 -- in psq_tcnvt, we weren't adding in the quotes
**	    in the total size of the text.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	03-dec-1992 (rog)
**	    Included ulf.h before qsf.h.
**      14-dec-92 (rblumer)
**          Added psq_tmulti_out routine and changed psq_1rptqry_out to use it.
**      23-mar-93 (smc)
**          Cast parameter of STlcopy() to match prototype.
**	03-may-93 (andre)
**	    defined psq_tdelete()
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	15-Aug-1997 (jenjo02)
** 	    New field, ulm_flags, added to ULM_RCB in which to specify the
**	    type of stream to be opened, shared or private.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psq_store_text(), psq_tout(),
**	    psq_tmulti_out().
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	11-Mar-1999 (shero03)
**	    support operand array in 0calclen
**      26-Oct-2009 (coomi01) b122714
**          Move psq_store_text() declarator to pshparse.h and make public.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_template@]...
**/

/* static and forward declarations */


/*}
** Name: PSQ_TEXT - Piece in query text chain
**
** Description:
**      This structure holds one piece in a query text chain.  It can contain
**	a piece of arbitrary length, even though it is declared for only on
**	character.
**
** History:
**      18-jul-86 (jeff)
**          written
*/
typedef struct _PSQ_TEXT
{
    struct _PSQ_TEXT *psq_prev;         /* Pointer to previous piece */
    struct _PSQ_TEXT *psq_next;		/* Pointer to next piece */
    i4              psq_psize;          /* Size of this piece */
    u_char	    psq_tval[1];	/* The text itself.  The 1 is bogus.
					** It is there only as a placeholder.
					** Actually, this element will hold
					** arrays of many different sizes.
					*/
} PSQ_TEXT;

/*}
** Name: PSQ_THEAD - Header for query text chain
**
** Description:
**      This structure heads a query text chain.
**
** History:
**      18-jul-86 (jeff)
**          written
*/
typedef struct _PSQ_THEAD
{
    PSQ_TEXT        *psq_first;         /* Pointer to first piece */
    PSQ_TEXT	    *psq_last;		/* Pointer to last piece */
    i4		    psq_tsize;		/* Total size of all text in pieces */
    ULM_RCB	    psq_tmem;		/* Control block for memory alloc. */
} PSQ_THEAD;

/*{
** Name: psq_topen	- Open a query text chain
**
** Description:
**      This function opens a query text chain by opening a memory stream,
**	allocating a header, and filling it in. 
**
** Inputs:
**      header                          Place to put pointer to header
**	memleft				Pointer to memory left
**	err_blk				Filled in if an error happens
**
** Outputs:
**      header                          Filled in with pointer to header
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**      18-jul-86 (jeff)
**          written
**      02-sep-86 (seputis)
**          err_blk becomes a DB_ERROR *
*/
DB_STATUS
psq_topen(
	PTR                *header,
	SIZE_TYPE	   *memleft,
	DB_ERROR	   *err_blk)
{
    ULM_RCB             ulm_rcb;
    DB_STATUS		status;
    i4		err_code;
    PSQ_THEAD		*hp;
    extern PSF_SERVBLK	*Psf_srvblk;

    /* Open the stream and allocate memory for the header */
    ulm_rcb.ulm_facility = DB_PSF_ID;
    ulm_rcb.ulm_poolid = Psf_srvblk->psf_poolid;
    ulm_rcb.ulm_blocksize = 512;
    ulm_rcb.ulm_memleft = memleft;
    ulm_rcb.ulm_streamid_p = &ulm_rcb.ulm_streamid;
    ulm_rcb.ulm_flags = ULM_SHARED_STREAM | ULM_OPEN_AND_PALLOC;
    ulm_rcb.ulm_psize = sizeof(PSQ_THEAD);
    if ((status = ulm_openstream(&ulm_rcb)) != E_DB_OK)
    {
	if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		&err_code, err_blk, 0);
	}
	else
	    (VOID) psf_error(E_PS0370_OPEN_TEXT_CHAIN,
		ulm_rcb.ulm_error.err_code, PSF_INTERR, &err_code, err_blk, 0);
	return (status);
    }

    *header = ulm_rcb.ulm_pptr;

    /* Fill in the header */
    hp		    = (PSQ_THEAD*) ulm_rcb.ulm_pptr;
    hp->psq_first   = (PSQ_TEXT *) NULL;
    hp->psq_last    = (PSQ_TEXT *) NULL;
    hp->psq_tsize   = 0;
    STRUCT_ASSIGN_MACRO(ulm_rcb, hp->psq_tmem);
    hp->psq_tmem.ulm_streamid_p = &hp->psq_tmem.ulm_streamid;

    return (E_DB_OK);
}

/*{
** Name: psq_tadd	- Add a piece of query text to a chain
**
** Description:
**      This function adds a piece of query text to an existing chain.
**
** Inputs:
**      header                          Pointer to chain header
**	piece				Pointer to piece of text
**	size				Size of piece
**	result				Place to put pointer to new piece
**	err_blk				Filled in if an error happens
**
** Outputs:
**      result                          Filled in with pointer to chain element
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**      18-jul-86 (jeff)
**          written
*/
DB_STATUS
psq_tadd(
	PTR                header,
	u_char		   *piece,
	i4		   size,
	PTR		   *result,
	DB_ERROR	   *err_blk)
{
    PSQ_THEAD           *hp = (PSQ_THEAD *) header;
    PSQ_TEXT		*tp;
    i4		err_code;
    DB_STATUS		status;

    /* Allocate enough space for PSQ_TEXT structure containing piece */
    hp->psq_tmem.ulm_psize = size + sizeof(PSQ_TEXT) - 1;
    status = ulm_palloc(&hp->psq_tmem);
    if (status != E_DB_OK)
    {
	if (hp->psq_tmem.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		&err_code, err_blk, 0);
	}
	else
	    (VOID) psf_error(E_PS0371_ALLOC_TEXT_CHAIN,
		hp->psq_tmem.ulm_error.err_code, PSF_INTERR, &err_code,err_blk, 0);
	return (status);
    }
    *result = hp->psq_tmem.ulm_pptr;
    tp	    = (PSQ_TEXT*) *result;

    /* Fill in text piece */
    MEcopy((char *) piece, size, (char *) tp->psq_tval);
    tp->psq_psize = size;

    /* Hook it up to the chain */
    tp->psq_next = (PSQ_TEXT *) NULL;
    if (hp->psq_last != (PSQ_TEXT *) NULL)
    {
	hp->psq_last->psq_next = tp;
	tp->psq_prev = hp->psq_last;
    }
    else
    {
	tp->psq_prev = NULL;
    }
    hp->psq_last = tp;
    if (hp->psq_first == (PSQ_TEXT *) NULL)
	hp->psq_first = tp;

    /* Add in the length to the total for the chain */
    hp->psq_tsize += size;

    return (E_DB_OK);
}

/*{
** Name: psq_rptqry_text - add a piece of text to the text chain for a repeat
**			   query
**
** Description: Add a piece of text to the text chain for a repeat query.  It is
**		imperative that text of all repeat queries be stored in a
**		uniform fashion so that comparing two stored query texts would
**		serve as a reliable indicator of their sameness.  Each piece
**		will be preceeded with a blank except for a PERIOD.  Neither the
**		piece consisting of PERIOD nor the following piece will be
**		preceeded with a blank.
**
** Inputs:
**      header                          Pointer to chain header
**	piece				Pointer to piece of text
**	size				Size of piece
**	result				Place to put pointer to new piece
**	err_blk				Filled in if an error happens
**
** Outputs:
**      result                          Filled in with pointer to chain element
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**      24-jan-90 (andre)
**          Plagiarized from psq_tadd().
**	28-jan-91 (andre)
**	    Do not insert a space if a piece is immediately following a piece
**	    consisting of a $.
*/
DB_STATUS
psq_rptqry_text(
	PTR                header,
	u_char		   *piece,
	i4		   size,
	PTR		   *result,
	DB_ERROR	   *err_blk)
{
    PSQ_THEAD           *hp = (PSQ_THEAD *) header;
    PSQ_TEXT		*tp;
    i4		err_code;
    bool		leading_blank;
    char		*txt;
    DB_STATUS		status;

    /*
    ** Allocate enough space for PSQ_TEXT structure containing piece:
    ** all pieces will be preceeded with a blank with the following exceptions:
    **	- piece consisting of PERIOD will not be preceeeded with a blank;
    **	- piece which immediately follows a piece consisting of PERIOD;
    **	- piece starting with a "white" character will not be preceeded with a
    **	  blank;
    **	- piece which immediately follows a piece consisting of $ (preceeded by
    **	  a blank which was inserted by this function)
    */

    if (   size == CMbytecnt(".") && !CMcmpcase(piece, ".")
	|| CMwhite(piece))
    {
	/*
	** piece consists of a period or starts with a "white" character - no
	** leading blanks will be added
	*/
	leading_blank = FALSE;
    }
    else if (   hp->psq_last != (PSQ_TEXT *) NULL
	     && ((   hp->psq_last->psq_psize == CMbytecnt(".")
		  && !CMcmpcase(hp->psq_last->psq_tval, ".")
		 )
		 ||
		 (   hp->psq_last->psq_psize == CMbytecnt(" ") + CMbytecnt("$")
		  && !CMcmpcase(hp->psq_last->psq_tval, " ")
		  && !CMcmpcase((hp->psq_last->psq_tval + CMbytecnt(" ")), "$")
		 )
	        )
	    )
    {
	/*
	** previous piece consists of a period or of a $ preceeded by a blank
	** inserted by this function - no leading blanks will be added
	*/
	leading_blank = FALSE;
    }
    else
    {
	/* insert a blank before the piece */
	leading_blank = TRUE;
    }

    hp->psq_tmem.ulm_psize = (leading_blank)
	? size + sizeof(PSQ_TEXT) - 1 + CMbytecnt(" ")
	: size + sizeof(PSQ_TEXT) - 1;

    if ((status = ulm_palloc(&hp->psq_tmem)) != E_DB_OK)
    {
	if (hp->psq_tmem.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    (VOID) psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		&err_code, err_blk, 0);
	}
	else
	{
	    (VOID) psf_error(E_PS0371_ALLOC_TEXT_CHAIN,
		hp->psq_tmem.ulm_error.err_code, PSF_INTERR, &err_code, err_blk,
		0);
	}

	return (status);
    }

    *result = hp->psq_tmem.ulm_pptr;
    tp	    = (PSQ_TEXT*) *result;

    /* Fill in text piece */
    txt = (char *) tp->psq_tval;

    /* insert a leading blank if necessary */
    if (leading_blank)
    {
	CMcpychar(" ", txt);
	txt += CMbytecnt(" ");
    }
	
    MEcopy((char *) piece, size, txt);
    tp->psq_psize = (leading_blank) ? size + CMbytecnt(" ") : size;

    /* Hook it up to the chain */
    tp->psq_next = (PSQ_TEXT *) NULL;
    if (hp->psq_last != (PSQ_TEXT *) NULL)
    {
	hp->psq_last->psq_next = tp;
	tp->psq_prev = hp->psq_last;
    }
    else
    {
	tp->psq_prev = NULL;
    }
    hp->psq_last = tp;
    if (hp->psq_first == (PSQ_TEXT *) NULL)
	hp->psq_first = tp;

    /* Add in the length to the total for the chain */
    hp->psq_tsize += tp->psq_psize;

    return (E_DB_OK);
}

/*{
** Name: psq_tclose	- Close the memory stream for a text chain
**
** Description:
**      This function closes the memory stream for a text chain, deallocating
**	the memory.
**
** Inputs:
**      header                          Pointer to header of text chain
**	err_blk				Filled in if an error happens
**
** Outputs:
**      err_blk                         Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Deallocates memory
**
** History:
**      18-jul-86 (jeff)
**          written
*/
DB_STATUS
psq_tclose(
	PTR		   header,
	DB_ERROR           *err_blk)
{
    DB_STATUS           status;
    ULM_RCB		ulm_rcb;
    i4		err_code;

    STRUCT_ASSIGN_MACRO(((PSQ_THEAD*)header)->psq_tmem, ulm_rcb);
    status = ulm_closestream(&ulm_rcb);
    if (status != E_DB_OK)
    {
	(VOID) psf_error(E_PS0372_CLOSE_TEXT_CHAIN,
	    ulm_rcb.ulm_error.err_code, PSF_INTERR, &err_code, err_blk, 0);
	return (status);
    }

    return (E_DB_OK);
}

/*{
** Name: psq_tsubs	- Substitute one piece of text for another
**
** Description:
**      This function substitutes one piece of query text for another in the
**	given text chain.  It will try to re-use the current piece before
**	allocating another.
**
** Inputs:
**      header                          Pointer to header
**	piece				Piece we want to substitute for
**	newtext				Pointer to new piece of text
**	size				Size of new piece
**	newpiece			Place to put pointer to new piece
**	err_blk				Filled in if an error happens
**
** Outputs:
**      newpiece                        Filled in with pointer to new piece
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory
**
** History:
**      18-jul-86 (jeff)
**          written
*/
DB_STATUS
psq_tsubs(
	PTR                header,
	PTR		   piece,
	u_char		   *newtext,
	i4		   size,
	PTR		   *newpiece,
	DB_ERROR	   *err_blk)
{
    PSQ_THEAD           *hp = (PSQ_THEAD *) header;
    PSQ_TEXT		*tp = (PSQ_TEXT *) piece;
    PSQ_TEXT		*np;
    DB_STATUS		status;
    i4		err_code;
    i4			oldsize;

    /* Remember the size of the original piece */
    oldsize = tp->psq_psize;

    /* If there isn't enough room in the current piece, allocate another one */
    if (tp->psq_psize < size)
    {
	/* Allocate enough for PSQ_TEXT struct with size bytes of text */
	hp->psq_tmem.ulm_psize = size + sizeof(PSQ_TEXT) - 1;
	status = ulm_palloc(&hp->psq_tmem);
	if (status != E_DB_OK)
	{
	    if (hp->psq_tmem.ulm_error.err_code == E_UL0005_NOMEM)
	    {
		psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		    &err_code, err_blk, 0);
	    }
	    else
		(VOID) psf_error(E_PS0371_ALLOC_TEXT_CHAIN,
		    hp->psq_tmem.ulm_error.err_code,
		    PSF_INTERR,&err_code, err_blk, 0);
	    return (status);
	}
	np = (PSQ_TEXT*) hp->psq_tmem.ulm_pptr;

	/* Hook up the new piece into the chain */
	if (np->psq_prev = tp->psq_prev)
	{
	    np->psq_prev->psq_next = np;
	}
	
	if (np->psq_next = tp->psq_next)
	{
	    np->psq_next->psq_prev = np;
	}

	if (hp->psq_first == tp)
	    hp->psq_first = np;
	if (hp->psq_last == tp)
	    hp->psq_last = np;
	tp = np;
    }

    /* Copy the new text into the piece and set the size */
    MEcopy((char *) newtext, size, (char *) tp->psq_tval);
    tp->psq_psize = size;

    /* Adjust the total size of the chain */
    hp->psq_tsize -= oldsize - size;

    /* Give the caller a pointer to the new piece */
    *newpiece = (PTR) tp;

    return (E_DB_OK);    
}

/*{
** Name: psq_tout	- Copy a text chain to QSF memory
**
** Description:
**      This function copies a text chain to QSF memory, as a text object.
**	It allocates it as a contiguous piece.
**
** Inputs:
**	rngtab			if non-NULL, range statements will be
**				generated for all entries of the range table
**				that are active (pss_used && pss_rgno >= 0);
**				should be non-NULL only for QUEL queries
**      header                  Pointer to chain header
**	mstream			Pointer to unopened memory stream
**	err_blk			Filled in if an error happens
**
** Outputs:
**      mstream                 Filled with object id for new text
**				object
**	err_blk			Filled in if an error happened
**	Returns:
**	    E_DB_OK		Success
**	    E_DB_ERROR		Non-catastrophic failure
**	    E_DB_FATAL		Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**      18-jul-86 (jeff)
**          written
**      2-sep-86 (seputis)
**          changed MEcopy routine parameter to be &hp->psq_tsize
**	6-may-87 (daved)
**	    clear mstream lock id after unlocking memory stream
**	29-jun-87 (daved)
**	    emit the range table entries if a range table is provided.
**	10-aug-93 (andre)
**	    removed declaration of qsf_call()
*/
DB_STATUS
psq_tout(
	PSS_SESBLK	   *sess_cb,
	PSS_USRRANGE	   *rngtab,
	PTR                header,
	PSF_MSTREAM	   *mstream,
	DB_ERROR	   *err_blk)
{
    DB_STATUS           status;
    i4		err_code;
    u_char		*buf;
    QSF_RCB		qsf_rb;

    /* Open the QSF memory stream for the text object */
    status = psf_mopen(sess_cb, QSO_QTEXT_OBJ, mstream, err_blk);
    if (status != E_DB_OK)
	return (status);

    status = psq_store_text(sess_cb, rngtab, header, mstream, 
	(PTR *) &buf, (bool) FALSE, err_blk);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* Set the root of the object */
    status = psf_mroot(sess_cb, mstream, (PTR) buf, err_blk);
    if (status != E_DB_OK)
	return (status);

    /* Unlock the object */
    qsf_rb.qsf_type = QSFRB_CB;
    qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
    qsf_rb.qsf_length = sizeof(qsf_rb);
    qsf_rb.qsf_owner = (PTR)DB_PSF_ID;
    qsf_rb.qsf_sid = sess_cb->pss_sessid;
    STRUCT_ASSIGN_MACRO(mstream->psf_mstream, qsf_rb.qsf_obj_id);
    qsf_rb.qsf_lk_id = mstream->psf_mlock;
    status = qsf_call(QSO_UNLOCK, &qsf_rb);
    if (status != E_DB_OK)
    {
	(VOID) psf_error(E_PS0375_UNLOCK_QSF_TEXT,
	    qsf_rb.qsf_error.err_code, PSF_INTERR, &err_code, err_blk, 0);
	return (status);
    }
    mstream->psf_mlock = 0;

    return (E_DB_OK);
}

/*{
** Name: psq_1rptqry_out - Put a repeat query text chain into a contiguous
**			   block of QSF memory
**
** Description: This function puts a repeat query text chain into a contiguous
**		block of QSF memory 
**      
**
** Inputs:
**      header                          Pointer to chain header
**	mstream				Pointer to preopened memory stream
**	err_blk				Filled in if an error happens
**
** Outputs:
**	txt_len				length of text copied into QSF memory
**	txt				pointer to a QSF memory buffer
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**      24-jan-91 (andre)
**          Adapted from psq_tout()
**	18-jan-93 (andre)
**	    replace with a call to psq_store_text()
*/
DB_STATUS
psq_1rptqry_out(
	PSS_SESBLK	*sess_cb,
	PTR             header,
	PSF_MSTREAM	*mstream,
	i4		*txt_len,
	u_char		**txt,
	DB_ERROR	*err_blk)
{
    DB_STATUS           status;
    DB_TEXT_STRING	*out_str;

    status = psq_store_text(sess_cb, (PSS_USRRANGE *) NULL, header, 
	mstream, (PTR *) &out_str, (bool) TRUE, err_blk);
    if (status != E_DB_OK)
	return (status);

    *txt_len = out_str->db_t_count;
    *txt     = out_str->db_t_text;

    return (E_DB_OK);
}

/*{
** Name: psq_tqrylen 	- Return total length of text accumulated so far.
**
** Description:
**      This function copies the total length of text chain accumulated
**	so far to callers variable. Used by GRANT stmnt.
**
** Inputs:
**      header                          Pointer to chain header
**	length				Address of user's variable.
**
** Outputs:
**	length				Address of user's variable.
**	Returns:
**	    E_DB_OK			Success
**	Exceptions:
**	    none
**
** Side Effects:
**	    none.
**
** History:
**      29-apr-87 (stec)
**          written
*/
DB_STATUS
psq_tqrylen(
	PTR                header,
	i4		   *length)
{
    PSQ_THEAD		*hp = (PSQ_THEAD *) header;

    *length = hp->psq_tsize;
    return (E_DB_OK);
}

/*{
** Name: PSQ_TCNVT	- convert db_data_value to text form
**
** Description:
**      This routine takes any user supplied db_data_value and converts
**  it to the value's textual representation.  This is required for the
**  iiqrytext catalog.  It is assumed that all datavalues can be written
**  into a character representation. 
**
** Inputs:
**	sess_cb				session control block
**	    .pss_adfcb			adf session control block for output
**					arguments.
**      header                          Pointer to chain header
**	dbval				db_data_value
**	result				Place to put pointer to new piece
**	err_blk				Filled in if an error happens
**
** Outputs:
**      result                          Filled in with pointer to chain element
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**      29-jun-87 (daved)
**          written
**	28-jan-91 (andre)
**	    fix bug 35446: size of a new piece should include quotes, if they
**	    were added.
**	18-nov-91 (rog)
**	    Fixed bug 40869, et alia: the above fix missed adding the quotes
**	    to the total size and not just the piece size.
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
*/
DB_STATUS
psq_tcnvt(
	PSS_SESBLK	   *sess_cb,
	PTR                header,
	DB_DATA_VALUE	   *dbval,
	PTR		   *result,
	DB_ERROR	   *err_blk)
{
    PSQ_THEAD           *hp;
    PSQ_TEXT		*tp;
    i4		err_code;
    DB_STATUS		status;
    ADF_CB		*adf_cb;
    ADF_FN_BLK		adffn;
    i4			dv_size;
    i4			count;
    char		*cptr;
    char		*quote_char;
    DB_TEXT_STRING      *string;
    ADI_DT_NAME		dt_fname;
    ADI_DT_NAME		dt_tname;
    char		f4_style;
    char		f8_style;
    i4			f4_width;
    i4			f8_width;
    i4			f4_prec;
    i4			f8_prec;
    i4			totype;
    i4			is_string;

    hp	    = (PSQ_THEAD *) header;
    adf_cb  = (ADF_CB *) sess_cb->pss_adfcb;
    status  = E_DB_OK;
    totype  = (DB_DT_ID) DB_LTXT_TYPE;

    if (dbval->db_datatype == totype)
	dv_size = dbval->db_length;
    else
	dv_size = 0;

    /* JRBCMT -- PSF is not allowed to know this!!  First of all, date is   */
    /* missing from the list below and decimal will soon need to be on it.  */
    /* But also, we need to fix this code so that it doesn't make	    */
    /* assumptions about what types exist.  I think the proper approach is  */
    /* to quote all non-intrinsic types, but this should be investigated.   */

    /* are we dealing with a string type (incoming). */
    if (abs(dbval->db_datatype) == DB_INT_TYPE ||
        abs(dbval->db_datatype) == DB_FLT_TYPE ||
	abs(dbval->db_datatype) == DB_MNY_TYPE ||
	abs(dbval->db_datatype) == DB_BOO_TYPE)
    {
	is_string = FALSE;
	quote_char = (char *) NULL;
    }
    else
    {
	is_string = TRUE;
	quote_char = (sess_cb->pss_lang == DB_SQL) ? "\'" : "\"";
    }
    

    /* set the floating point conversion display */
    f4_style = adf_cb->adf_outarg.ad_f4style;
    f8_style = adf_cb->adf_outarg.ad_f8style;
    f4_width = adf_cb->adf_outarg.ad_f4width;
    f8_width = adf_cb->adf_outarg.ad_f8width;
    f4_prec  = adf_cb->adf_outarg.ad_f4prec;
    f8_prec  = adf_cb->adf_outarg.ad_f8prec;

    adf_cb->adf_outarg.ad_f4style = 'n';
    adf_cb->adf_outarg.ad_f8style = 'n';
    adf_cb->adf_outarg.ad_f4width = 20;
    adf_cb->adf_outarg.ad_f8width = 20;
    adf_cb->adf_outarg.ad_f4prec  = 10;
    adf_cb->adf_outarg.ad_f8prec  = 10;

    /* get the function instance id for this conversion */
    status = adi_ficoerce(adf_cb, dbval->db_datatype, totype, &adffn.adf_fi_id);
    if (status != E_DB_OK)
    {
	goto exit;
    }         

    /* determine the result size. */
    if (!dv_size)
    {
	/* Now lets fill in the datatype length info and allocate space for the 
	** data.
	*/
	status = adi_fidesc(adf_cb, adffn.adf_fi_id, &adffn.adf_fi_desc);
	if (status != E_DB_OK)
	{
	    goto exit;
	}
	status = adi_0calclen(adf_cb, &adffn.adf_fi_desc->adi_lenspec, 1, &dbval, 
		&adffn.adf_r_dv);
	dv_size = adffn.adf_r_dv.db_length;

	if (status != E_DB_OK)
	{
	     goto exit;
	}
    }
    /* if string, add room for quotes */
    if (is_string)
	dv_size += 2 * CMbytecnt(quote_char);

    /* Allocate enough space for PSQ_TEXT structure containing piece */
    hp->psq_tmem.ulm_psize = dv_size + sizeof(PSQ_TEXT) - 1;
    status = ulm_palloc(&hp->psq_tmem);
    if (status != E_DB_OK)
    {
	if (hp->psq_tmem.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		&err_code, err_blk, 0);
	}
	else
	    (VOID) psf_error(E_PS0371_ALLOC_TEXT_CHAIN,
		hp->psq_tmem.ulm_error.err_code, PSF_INTERR,
		&err_code, err_blk, 0);
	return (status);
    }
    *result = hp->psq_tmem.ulm_pptr;
    tp	    = (PSQ_TEXT*) *result;

    string	  = (DB_TEXT_STRING*) tp->psq_tval;
    /* Fill in text piece */
    adffn.adf_r_dv.db_datatype	= totype;
    adffn.adf_r_dv.db_length	= dv_size;
    adffn.adf_r_dv.db_data	= (PTR) string;
    adffn.adf_dv_n		= 1;
    STRUCT_ASSIGN_MACRO(*dbval, adffn.adf_1_dv);
    adffn.adf_isescape		= FALSE;
    if ((status = adf_func(adf_cb, &adffn)) != E_DB_OK)
    {
	goto exit;
    }
    /* CAUTION: entering tricky code.
    ** string is a variable containing a text datatype. We want to convert
    ** to a C datatype.  We also want to add quote characters if the datatype
    ** was a string type.  We grab the count from the string variable first.
    ** we can then use the 2 byte count for character data.
    */
    count = string->db_t_count;
    cptr = (char *)  string;
    if (is_string)
    {
	/*
	** for strings, copy the opening quote (" or ', depending on language)
	*/ 
	CMcpychar(quote_char, cptr);
	cptr += CMbytecnt(quote_char);
    }
    MEcopy((PTR) string->db_t_text, count, (PTR) cptr);
    cptr += count;
    if (is_string)
    {
	/*
	** for strings, copy the closing quote (" or ', depending on language)
	*/ 
	CMcpychar(quote_char, cptr);
    }

    /* if storing a string, do not forget to account for quotes (bug 35446) */
    tp->psq_psize = (is_string) ? count + 2 * CMbytecnt(quote_char) : count;

    /* Hook it up to the chain */
    tp->psq_next = (PSQ_TEXT *) NULL;
    if (hp->psq_last != (PSQ_TEXT *) NULL)
    {
	hp->psq_last->psq_next = tp;
	tp->psq_prev = hp->psq_last;
    }
    else
    {
	tp->psq_prev = NULL;
    }
    hp->psq_last = tp;
    if (hp->psq_first == (PSQ_TEXT *) NULL)
	hp->psq_first = tp;

    /* Add in the length to the total for the chain */
    hp->psq_tsize += tp->psq_psize;

exit:
    /* set the floating point conversion display */
    adf_cb->adf_outarg.ad_f4style = f4_style;
    adf_cb->adf_outarg.ad_f8style = f8_style;
    adf_cb->adf_outarg.ad_f4width = f4_width;
    adf_cb->adf_outarg.ad_f8width = f8_width;
    adf_cb->adf_outarg.ad_f4prec  = f4_prec;
    adf_cb->adf_outarg.ad_f8prec  = f8_prec;

    if (status != E_DB_OK)
    {
	(VOID) adi_tyname(adf_cb, dbval->db_datatype, &dt_fname);
	(VOID) adi_tyname(adf_cb, totype, &dt_tname);
	(VOID) psf_error(2911L, 0L, PSF_USERERR,
	    &err_code, err_blk, 3, sizeof (sess_cb->pss_lineno),
	    &sess_cb->pss_lineno, 
	    psf_trmwhite(sizeof(dt_fname), (char *) &dt_fname), &dt_fname, 
	    psf_trmwhite(sizeof (dt_tname), (char *) &dt_tname), &dt_tname);
        return (E_DB_ERROR);    
    }
    return (status);
}

/*{
** Name: psq_tinsert 	- Add a piece of query text to a chain in front
**			  of some piece.
**
** Description:
**      This function inserts a piece of query text to an existing chain.
**	New text is added in front of the existing piece specified by the
**	caller .
**	A need for the routine arises from the fact that the owner names need
**	to qualify database object names in case of database procedures.
**	Another use for the function is when synonym_name has to be
**	replaced with owner.actual_table_name in the text stored for views,
**	integrities, and permits.
**	These names become known when the scanner has already 'seen' the
**	object name (and therefore has already added it to the text chain).
**
** Inputs:
**      header                          Pointer to chain header
**	piece				Pointer to piece of text
**	size				Size of piece
**	result				Place to put pointer to new piece
**	oldpiece			Piece in front of which the new piece
**					will be inserted (should not be NULL).
**	err_blk				Filled in if an error happens
**
** Outputs:
**      result                          Filled in with pointer to chain element
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**      02-jun-88 (stec)
**          written
**	04-jun-90 (andre)
**	    renamed from psq_tb1add() to psq_tinsert();
**	    changed the description to describe use of this function in
**	    conjunction with synonyms.
*/
DB_STATUS
psq_tinsert(
	PTR                header,
	u_char		   *piece,
	i4		   size,
	PTR		   *result,
	PTR		   oldpiece,
	DB_ERROR	   *err_blk)
{
    PSQ_THEAD           *hp = (PSQ_THEAD *) header;
    PSQ_TEXT		*tp, *op = (PSQ_TEXT *) oldpiece;
    i4		err_code;
    DB_STATUS		status;

    if (op == (PSQ_TEXT *) NULL)
    {
	/* Should not be reached */
	return (E_DB_ERROR);
    }

    /* Allocate enough space for PSQ_TEXT structure containing piece */
    hp->psq_tmem.ulm_psize = size + sizeof(PSQ_TEXT) - 1;
    status = ulm_palloc(&hp->psq_tmem);
    if (DB_FAILURE_MACRO(status))
    {
	if (hp->psq_tmem.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		&err_code, err_blk, 0);
	}
	else
	    (VOID) psf_error(E_PS0371_ALLOC_TEXT_CHAIN,
		hp->psq_tmem.ulm_error.err_code, PSF_INTERR, &err_code,err_blk, 0);
	return (status);
    }
    *result = hp->psq_tmem.ulm_pptr;
    tp	    = (PSQ_TEXT*) *result;

    /* Fill in text piece */
    MEcopy((char *) piece, size, (char *) tp->psq_tval);
    tp->psq_psize = size;

    /* Hook it up to the chain */
    if (tp->psq_prev = op->psq_prev)
	tp->psq_prev->psq_next = tp;
    tp->psq_next = op;
    op->psq_prev = tp;

    if (hp->psq_first == op)
	hp->psq_first = tp;

    /* Add in the length to the total for the chain */
    hp->psq_tsize += size;

    return (E_DB_OK);
}

/*
** psq_tbacktrack   - return a pointer to the N-th previous piece from some
**		      piece in the text chain
**
** Description:	    This function will return a pointer to the N-th previous
**		    piece from some piece P in the text chain.  If N is greater
**		    than the number of pieces ahead of P in the chain, NULL will
**		    be returned.
** Input:
**	piece 	    pointer to some piece P in the text chain
**	n	    number of the piece (starting from P) desired
**
** Output:
**	None
**
** Returns:
**	Pointer to the N-th previous piece from some piece P in the text chain.
**	if N > # pieces in the chain ahead of P, NULL will be returned.
**
** Sideeffects, exceptions:
**	None
**
** History:
**	4-jun-90 (andre)
**	    Written
*/
PTR
psq_tbacktrack(
	PTR         piece,
	i4	    n)
{
    PSQ_TEXT	*p = (PSQ_TEXT *) piece;

    if (n < 0)
    {
	return(NULL);
    }

    for (; p && n; p = p->psq_prev, n--)
    ;

    return((PTR) p);
}

/*
** psq_tbackfromlast   - return a pointer to the N-th previous piece before
**		         the last piece in the text chain
**
** Description:	    This function will return a pointer to the N-th previous
**		    piece from the last piece in the text chain.
**
**		    If N is greater than the number of pieces ahead of P in the
**		    chain, NULL will be returned.  If N is 0, the last piece
**		    will be returned.
** Input:
**	header 	    pointer to head of the text chain
**	n	    number of the piece (starting from the end) desired
**
** Output:
**	None
**
** Returns:
**	Pointer to the N-th previous piece from some piece P in the text chain.
**	if N > # pieces in the chain ahead of P, NULL will be returned.
**	If N is 0, the last piece will be returned.
**
** Sideeffects, exceptions:
**	If N < 0, NULL will be returned.
**
** History:
**	04-jan-93 (rblumer)
**	    Written
*/
PTR
psq_tbackfromlast(
	PTR         header,
	i4	    n)
{
    PSQ_THEAD	*headp = (PSQ_THEAD *) header;
    PTR		p;

    p = psq_tbacktrack( (PTR) headp->psq_last, n);
    
    return(p);

}  /* end psq_tbackfromlast */	

/*{
** Name: psq_tmulti_out - Put a query text chain into a contiguous block
**			  of QSF memory, and return a DB_TEXT_STRING ptr to
**			  it
**
** Description: 
**	This function puts a query text chain into a DB_TEXT_STRING
**      structure allocated from a contiguous block of QSF memory.
**
**	Unlike psq_tout, it doesn't open and close the memory stream,
**      so multiple text chains can be copied into the same memory stream.
**
** Inputs:
**	rngtab			if non-NULL, range statements will be
**				generated for all entries of the range table
**				that are active (pss_used && pss_rgno >= 0);
**				should be non-NULL only for QUEL queries
**      header			Pointer to chain header
**	mstream			Pointer to preopened memory stream
**	err_blk			Filled in if an error happens
**
** Outputs:
**	txt			address of a DB_TEXT_STRING stored
**				in QSF memory buffer
**	err_blk			Filled in if an error happened
**
** Returns:
**	    E_DB_OK		Success
**	    E_DB_ERROR		Non-catastrophic failure
**	    E_DB_FATAL		Catastrophic failure
** Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**      19-nov-92 (rblumer)
**          Adapted from psq_tout() and psq_1rptqry_out.
**	09-jan-93 (andre)
**	    added rngtab and modified to use psq_store_text();
**	    in it previous form, this function could not correctly handle
**	    generation of text for QUEL (did not try to generate RANGE
**	    statements)
*/
DB_STATUS
psq_tmulti_out(
	       PSS_SESBLK	    *sess_cb,
	       PSS_USRRANGE         *rngtab,
	       PTR		    header,
	       PSF_MSTREAM	    *mstream,
	       DB_TEXT_STRING	    **txtp,
	       DB_ERROR		    *err_blk)
{
    return(psq_store_text(sess_cb, rngtab, header, mstream, 
		(PTR *) txtp, (bool) TRUE, err_blk));
} /* end psq_tmulti_out */

/*
** Name: psq_store_text - store query text in a contiguous block of QSF memory
**
** Description:
**	Copy contents of a query text chain (prepended, if necessary with RANGE
**	statements) into a contiguous block of QSF memory.  Caller may specify
**	that the text be stored in DB_TEXT_STRING format by setting
**	return_db_text_string to TRUE; otherwise the function will return a i4
**	followed by query text.
**
** Input:
**	rngtab			if non-NULL, range statements will be
**				generated for all entries of the range table
**				that are active (pss_used && pss_rgno >= 0);
**				should be non-NULL only for QUEL queries
**      header			Pointer to chain header
**	mstream			Pointer to opened memory stream
**	return_db_text_string	if TRUE, function will store text in
**				DB_TEXT_STRING format; otherwise it will store
**				it a a i4  (length) followed by text
**
** Output:
**	result			query text in specified format
**	err_blk			Filled in if an error happens
**
** Side efects:
**	allocates memory
**
** Returns:
**	E_DB_{OK,ERROR}
**
** History:
**	09-jan-93 (andre)
**	    written
**	29-jul-2001 (toumi01)
**	    problem found doing i64_aix port:
**	    (u_char *)'\n' should be (uchar)'\n'
**	    (u_char *)'\0' should be (uchar)'\0'
**      26-Oct-2009 (coomi01) b122714
**          Move psq_store_text() declarator to pshparse.h and make it public here.
**	24-Jun-2010 (kschendel) b123775
**	    Correct a call to trim-whitespace.
*/
DB_STATUS
psq_store_text(
	PSS_SESBLK	    *sess_cb,
	PSS_USRRANGE	    *rngtab,
	PTR		    header,
	PSF_MSTREAM	    *mstream,
	PTR		    *result,
	bool		    return_db_text_string,
	DB_ERROR	    *err_blk)
{
    DB_STATUS		status;
    i4			i;
    PSQ_THEAD		*hp = (PSQ_THEAD *) header;
    i4			size = hp->psq_tsize;
    PSQ_TEXT		*tp;
    PSS_RNGTAB		*rngvar;
    u_char		*out;
    
    if (rngtab)
    {
	/*
	** allocate enough space for range statements. each range statement
	** looks like range of 'rngname' is 'tabname'\n.
	** Thus, max space is 14+2*DB_MAX_NAME.
	*/
	for (i = 0, rngvar = rngtab->pss_rngtab; i < PST_NUMVARS; i++, rngvar++)
	{
	    /* Only look at range vars that are being used */
	    if (rngvar->pss_used && rngvar->pss_rgno >= 0)
	    {
		size += (  14   /* "range of  is \n" */
			 + psf_trmwhite(DB_TAB_MAXNAME, rngvar->pss_rgname)
			 + psf_trmwhite(sizeof(DB_TAB_NAME),
			       (char *) &rngvar->pss_tabname));
	    }
	}
    }
    
    if (return_db_text_string)
    {
	DB_TEXT_STRING	    *str;

	status = psf_malloc(sess_cb, mstream, size + sizeof(*str) - sizeof(u_char),
	    result, err_blk);
	if (status != E_DB_OK)
	    return (status);

	str = (DB_TEXT_STRING *) *result;

	/*
	** store the total length of query text 
	*/
	str->db_t_count = size;

	out = str->db_t_text;

    }
    else
    {
	/* Allocate a piece large enough for all the text + a i4  (count) */
	status = psf_malloc(sess_cb, mstream, size + sizeof(i4), result, err_blk);
	if (status != E_DB_OK)
	    return (status);

	out = (u_char *) *result;
	
	/* Copy the length into the buffer */
	MEcopy((char *) &size, sizeof(size), (char *) out);

	out += sizeof(size);
    }
    
    /* Copy the pieces into the buffer; first put the range statements */
    if (rngtab)
    {
	for (i = 0, rngvar = rngtab->pss_rngtab; i < PST_NUMVARS; i++, rngvar++)
	{
	    /* Only look at range vars that are being used */
	    if (rngvar->pss_used && rngvar->pss_rgno >= 0)
	    {
		i4 plen;

		STncpy( (char *)out, "range of ", 9);
		out += 9;

		/* add in range name */
		plen = psf_trmwhite(DB_TAB_MAXNAME, rngvar->pss_rgname);
		STncpy( (char *)out, rngvar->pss_rgname, plen);
		out += plen;

		STncpy( (char *)out, " is ", 4);
		out += 4;

		plen = psf_trmwhite(DB_TAB_MAXNAME, rngvar->pss_tabname.db_tab_name);
		STncpy( (char *)out, (char *)&rngvar->pss_tabname, plen);
		out += plen;

		*out = (u_char)'\n';
		out++;
		*out = (u_char)'\0';
	    }
	}
    }

    for (tp = hp->psq_first; tp != (PSQ_TEXT *) NULL; tp = tp->psq_next)
    {
	MEcopy((char *) tp->psq_tval, tp->psq_psize, (char *) out);
	out += tp->psq_psize;
    }

    return(E_DB_OK);
}

/*
** Name: psq_tdelete - delete specified element of a text chain
**
** Description:
**	delete specified element of a text chain and adjust pointers of next and
**	previous elements.  In addition, if the deleted element was first/last
**	element of the chain, adjust first/last pointers accordingly
**
** Input:
**	header	    chain header
**	piece	    pointer to the element to be deleted
**
** Output:
**	none
**
** Returns:
**	none
**
** History:
**	03-may-93 (andre)
**	    written
*/
void
psq_tdelete(
	PTR	    header,
	PTR         piece)
{
    PSQ_THEAD	*h = (PSQ_THEAD *) header;
    PSQ_TEXT	*p = (PSQ_TEXT *) piece;

    /*
    ** if element to be deleted was the first element of the chain, adjust
    ** "first" ptr accordingly; otherwise adjust "next" ptr of the element
    ** preceeding the one being deleted
    */
    if (h->psq_first == p)
	h->psq_first = p->psq_next;
    else
	p->psq_prev->psq_next = p->psq_next;

    /*
    ** if element to be deleted was the last element of the chain, adjust
    ** "last" ptr accordingly; otherwise adjust "previous" ptr of the element
    ** following the one being deleted
    */
    if (h->psq_last == p)
	h->psq_last = p->psq_prev;
    else
	p->psq_next->psq_prev = p->psq_prev;

    /*
    ** adjust size of the string represented by the chain downward to account
    ** for deleted piece
    */
    h->psq_tsize -= p->psq_psize;
    
    return;
}
