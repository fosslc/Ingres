/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <qu.h>
#include    <st.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSYPRINT.C - Print queries as stored in iiqrytext relation.
**
**  Description:
**      This file contains the functions for printing queries as they are
**	stored in the iiqrytext relation.  These functions are useful for
**	the "help permit", "help integrity", and "help view" commands.
**
**          psy_print - Decode query text into output blocks
**	    psy_put   - Put string into output block
**
**
**  History:    $Log-for RCS$
**      15-jul-86 (jeff)
**          written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	08-dec-92 (anitap)
**	    Changed the order of #includes of headr files for CREATE SCHEMA.
**	15-dec-92 (wolf) 
**	    ulf.h must be included before qsf.h
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	    Changed psf_sesscb() prototype to return PSS_SESBLK* instead
**	    of DB_STATUS.
**	04-sep-2003 (abbjo03)
**	    Add missing declaration and initialization of sess_cb in xDEBUG
**	    block in psy_put.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*{
** Name: psy_print	- Format query text to send to user
**
** Description:
**      This function decodes query text that comes from the iiqrytext relation
**	and formats it to send to the user.  It is useful for the "help permit",
**	"help integrity", and "help view" commands.
**
**	Query text stored in the iiqrytext relation consists of human-readable
**	text and special symbols.  Some of these special symbols are numbers
**	and strings sent from EQUEL or ESQL programs.  Others stand for table
**	names, range variables, and column numbers.  This functio decodes all
**	this stuff, and puts the result into a chain of PSY_QTEXT blocks to be
**	sent back to the user.
**
** Inputs:
**	mstream				Memory stream to allocate blocks from
**	map				map of range var numbers to those
**					in rngtab.
**	block				Current query text block
**      text                            Pointer to the query text
**	length				Length of the query text
**	rngtab				The range table (used for decoding the
**					special symbol standing for range
**					variable numbers, and column numbers).
**					The result range variable should stand
**					for the table we're getting help on.
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
**	    Sends query text to user
**
** History:
**      15-jul-86 (jeff)
**          written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/
DB_STATUS
psy_print(
	PSF_MSTREAM	   *mstream,
	i4		    map[],
	PSY_QTEXT	   **block,
	u_char             *text,
	i4		   length,
	PSS_USRRANGE	   *rngtab,
	DB_ERROR	   *err_blk)
{
    char                buf[1024 + DB_TAB_MAXNAME];
    i4                  i4align;
    i2                  i2align;
    f8                  f8align;
    register u_char	*p;
    register i4	j;
    register i4	i;
    PSS_RNGTAB		*lastvar = (PSS_RNGTAB *) NULL;
    DB_STATUS		status;
    i4			slength;

    /* Put out range statements */
    for (i = 0; i < PST_NUMVARS; i++)
    {
	/* Only look at range vars that are being used */
	if (!rngtab->pss_rngtab[i].pss_used ||
	    rngtab->pss_rngtab[i].pss_rgno < 0)
	{
	    continue;
	}

	status = psy_put(mstream, (char*) "range of ", 
	    (i4) sizeof("range of ") - 1, block, 
	    err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* NULL terminate and trim blanks from range variable name */
	MEcopy(rngtab->pss_rngtab[i].pss_rgname, DB_TAB_MAXNAME, buf);
	buf[DB_TAB_MAXNAME] = '\0';
	slength = STtrmwhite(buf);
	status = psy_put(mstream, buf, slength, block, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	status = psy_put(mstream, " is ", (i4) sizeof(" is ") - 1,
	     block, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* NULL terminate and trim blanks from table name */
	MEcopy((char *) &rngtab->pss_rngtab[i].pss_tabname, DB_TAB_MAXNAME, buf);
	buf[DB_TAB_MAXNAME] = '\0';
	slength = STtrmwhite(buf);
	status = psy_put(mstream, buf, slength, block, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* Newline after every range statement */
	status = psy_put(mstream, (char*) "\n", (i4) 1, block, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
    }

    for (p = text; p < (u_char*) text + length;)
    {
	switch (*p++)
	{
	case PSQ_HVSTR:	    /* String sent from user program */
	    slength = STlength((char*) p);

	    /* Emit opening quote */
	    status = psy_put(mstream, (char *) "\"", 1, block, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    for (j = slength; j > 0; j--, p++)
	    {
		if (*p == '"')
		{
		    /* Escape any quote characters */
		    status = psy_put(mstream, "\\\"", 2, block, err_blk);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		}
		else if (!CMprint(p))
		{
		    /* Non-printing characters show up as escape sequence */
		    STprintf(buf, "\\%o", *p);
		    status = psy_put(mstream, buf, (i4) STlength(buf),
			block, err_blk);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		}
		else
		{
		    status = psy_put(mstream, (char*) p, 1, block, err_blk);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		}
	    }
	    break;

	case PSQ_HVF8:	    /* f8 sent from user program */
	    MEcopy((char *) p, sizeof(f8align), (char *) &f8align);
	    STprintf(buf, "%f", f8align);
	    status = psy_put(mstream, buf, (i4) STlength(buf), block, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    p += sizeof(f8align);
	    break;

	case PSQ_HVI4:	    /* i4 sent from user program */
	    MEcopy((char *) p, sizeof(i4align), (char *) &i4align);
	    CVla(i4align, buf);
	    status = psy_put(mstream, buf, (i4) STlength(buf), block, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    p += sizeof(i4align);
	    break;

	case PSQ_HVI2:	    /* i2 sent from user program */
	    MEcopy((char *) p, sizeof(i2align), (char *) &i2align);
	    CVla((i4) i2align, buf);
	    status = psy_put(mstream, buf, (i4) STlength(buf), block, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    p += sizeof(i2align);
	    break;

	case DB_INTRO_CHAR: /* Intro char for special sequence */
	    switch (*p++)
	    {
	    case DB_RNG_VAR:
		/* Put a blank before the table name */
		status = psy_put(mstream, " ", 1, block, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return (status);

		/* Get the range variable number */
		MEcopy((char *) p, sizeof(i4align), (char *) &i4align);
		p += sizeof(i4align);

		i4align = map[i4align];
		/* Look up range variable number */
		for (j = 0; j < PST_NUMVARS; j++)
		{
		    if (rngtab->pss_rngtab[j].pss_used &&
			rngtab->pss_rngtab[j].pss_rgno == i4align)
		    {
			break;
		    }
		}

		/* If found, give variable name, otherwise question marks */
		if (j < PST_NUMVARS)
		{
		    /* trim trailing blanks and NULL terminate */
		    MEcopy(rngtab->pss_rngtab[j].pss_rgname, DB_TAB_MAXNAME, buf);
		    buf[DB_TAB_MAXNAME] = '\0';
		    slength = STtrmwhite(buf);
		    status = psy_put(mstream, buf, slength, block, err_blk);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		    /* Remember the range variable for when we get the colnum */
		    lastvar = &rngtab->pss_rngtab[j];
		}
		else
		{
		    lastvar = (PSS_RNGTAB*) 0;
		    /* Put question marks if not found */
		    status = psy_put(mstream, "???", 3, block, err_blk);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		}
		break;

	    case DB_COL_NUM:	    /* Column number for a range var */

		/* Get the column number */
		MEcopy((char *) p, sizeof(i4align), (char *) &i4align);
		p += sizeof(i4align);

		/* If there was no range variable, put question marks */
		if (lastvar != (PSS_RNGTAB *) NULL)
		{
		    /* NULL terminate and trim blanks from column name */
		    MEcopy((char *) &lastvar->pss_attdesc[i4align]->att_name,
			DB_ATT_MAXNAME, buf);
		    buf[DB_ATT_MAXNAME] = '\0';
		    slength = STtrmwhite(buf);
		    status = psy_put(mstream, buf, slength, block, err_blk);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		}
		else
		{
		    /* Don't know column name, just give question marks */
		    status = psy_put(mstream, "???", 3, block, err_blk);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		}

		/* Put a blank after the column name */
		status = psy_put(mstream, " ", 1, block, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return (status);

		break;

	    case DB_TBL_NM:
		/* The table name is in the result slot of the range table */

		/* Put a blank before the table name */
		status = psy_put(mstream, " ", 1, block, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return (status);

		/* NULL terminate and trim blanks from the table name */
		MEcopy((char *) &rngtab->pss_rsrng.pss_tabname,
		    sizeof(DB_TAB_NAME), buf);
		buf[DB_TAB_MAXNAME] = '\0';
		slength = STtrmwhite(buf);
		status = psy_put(mstream, buf, slength, block, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return (status);

		/* Put a blank after the table name */
		status = psy_put(mstream, " ", 1, block, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return (status);

		break;

	    case DB_RES_COL:	/* Result column: column in result table */
		/* Put a blank before the column name */
		status = psy_put(mstream, " ", 1, block, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return (status);

		/* Get the column number for the result column */
		MEcopy((char *) p, sizeof(i4align), (char *) &i4align);
		p += sizeof(i4align);

		/* Get the column name from the result range variable */
		lastvar = &rngtab->pss_rsrng;
		MEcopy((char *) &lastvar->pss_attdesc[i4align]->att_name,
		    DB_ATT_MAXNAME, buf);
		buf[DB_ATT_MAXNAME] = '\0';
		slength = STtrmwhite(buf);
		status = psy_put(mstream, buf, slength, block, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return (status);

		/* Put a blank after the column name */
		status = psy_put(mstream, " ", 1, block, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return (status);
		break;

	    default:
		/* Unknown special sequence: just put out as it came in */
		status = psy_put(mstream, (char*) p - 1, 2, block, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return (status);
		break;
	    }
	    break;
	default:
	    /* No special case, just put the char */
	    status = psy_put(mstream, (char*) p - 1, 1, block, err_blk);
	    break;
	}
    }

    /* Put a newline just after the statement */
    status = psy_put(mstream, "\n", 1, block, err_blk);
    return (status);
}

/*{
** Name: psy_put	- Put characters in blocks for output
**
** Description:
**      This function buffers up characters to be sent to the user by SCF.
**	It puts as much of the current request in the current block as
**	possible.  If there is any overflow, it allocates another block and
**	puts the excess there.  It keeps doing this until all the input is
**	used up.
**
** Inputs:
**      mstream                         Memory stream to use for allocating
**					blocks.
**	inbuf				Characters to send to user
**	len				Number of chars to send to user
**	block				Pointer to pointer to block to use
**	err_blk				Filled in if an error happens
**
** Outputs:
**      block                           Filled in with string to send to user.
**					A new block may be allocated and this
**					pointer modified to point to new one.
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_FATAL			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory
**
** History:
**      15-jul-86 (jeff)
**          written
*/
DB_STATUS
psy_put(
	PSF_MSTREAM        *mstream,
	register char	   *inbuf,
	register i4	   len,
	PSY_QTEXT	   **block,
	DB_ERROR	   *err_blk)
{
    i4                  tocopy;
    i4			left;
    char		*outplace;
    PSY_QTEXT		*newblock;
    DB_STATUS		status;
#ifdef xDEBUG
    i4		val1, val2;	    /* tracing temps */
    PSS_SESBLK	   	*sess_cb;

    sess_cb = psf_sesscb();
#endif

    while (len > 0)
    {
	/* Copy as much of input as will fit in current block */
	left	= PSY_QTSIZE - (*block)->psy_qcount;
	tocopy = len > left ? left : len;
	outplace = (*block)->psy_qtext + (*block)->psy_qcount;
#ifdef xDEBUG
	if (ult_check_macro(&sess_cb->pss_trace, 71, &val1, &val2))
	{
	    TRdisplay("%.#s", tocopy, inbuf);
	}
#endif
	MEcopy(inbuf, tocopy, outplace);
	len			-= tocopy;
	(*block)->psy_qcount	+= tocopy;
	inbuf			+= tocopy;

	/* If there is more input, create another block and link it up */
	if (len > 0)
	{
	    status = psy_txtalloc(mstream, &newblock, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    (*block)->psy_qnext = newblock;

	    /* Use the new block until a new one is allocated */
	    *block = newblock;
	}
    }

    return (E_DB_OK);
}

/*{
** Name: psy_txtalloc	- Allocate a new text block
**
** Description:
**      This function allocates a new text block in a memory stream,
**	fills it in, and returns a pointer to it.
**
** Inputs:
**      mstream                         Pointer to memory stream
**	newblock			Place to put pointer to new block
**	err_blk				Filled in if error happens
**
** Outputs:
**      newblock                        Filled in with pointer to new block
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**      15-jul-86 (jeff)
**          written
*/
DB_STATUS
psy_txtalloc(
	PSF_MSTREAM        *mstream,
	PSY_QTEXT	   **newblock,
	DB_ERROR	   *err_blk)
{
    DB_STATUS           status;
    PSS_SESBLK	   	*sess_cb;

    sess_cb = psf_sesscb();

    /* Allocate the block */
    status = psf_malloc(sess_cb, mstream, sizeof(PSY_QTEXT), (PTR *) newblock, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* Fill in the block */
    (*newblock)->psy_qnext = (PSY_QTEXT *) NULL;
    (*newblock)->psy_qcount = 0;

    return (E_DB_OK);
}
