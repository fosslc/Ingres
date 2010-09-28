/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <cm.h>
#include    <me.h>
#include    <mh.h>
#include    <qu.h>
#include    <st.h>
#include    <iicommon.h>
#include    <usererror.h>
#include    <dbdbms.h>
#include    <tm.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefmain.h>
#include    <rdf.h>
#include    <scf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <sxf.h>
#include    <psyaudit.h>

/**
**
**  Name: pslseq.c - PSF sequence parser action routines
**
**  Description:
**	This module contains functions that execute semantic actions
**	associated with the sequence DDL statements. It is called from
**	the create & alter sequence productions in the SQL grammar.
**
**  Defines:
**	psl_csequence	- Semantic actions for create sequence
**	psl_asequence	- Semantic actions for alter sequence
**	psl_seq_parmcheck - validate/prepare numeric sequence parameters
**
**  History:
**	1-apr-02 (inkdo01)
**	    Written for sequences. 
**	4-june-2008 (dougi)
**	    Changed to support 64-bit integers.
**	15-june-2008 (dougi)
**	    Add support for unordered (random) sequences.
**	27-Jun-2010 (kschendel) b123986
**	    Make csequence and parmcheck aware of the fact that pss_object
**	    is not a PSY_CB during create table with identity columns.
**/

static u_char dec_mini8[] = {0, 0, 0, 0, 0, 0, 0x92, 0x23, 0x37, 0x20, 0x36,
	0x85, 0x47, 0x75, 0x80, 0x8c};


/*{
** Name: psl_csequence	- Perform create sequence semantic actions.
**
** Description:
** 	This routine performs the final validation of the parameters of a 
**	create sequence statement (including filling in default values for
**	missing parameters and resolving the need for decimal v.s. integer
**	parameters).
**
** Inputs:
**	sess_cb			Pointer to session control block.
**	seqp			Pointer to sequence tuple;  might
**				be either psy_tuple or the sequence
**				tuple in the yyvars area, depending
**				on context (create seq vs identity col).
**	err_blk			To be filled in if error.
**
** Outputs:
**	seqp			Sequence tuple to insert.
**	  .dbs_name		Name of sequence filled in by the grammar.
**	  .dbs_type		Sequence data type.
**	  .dbs_length		Size.
**	  .dbs_start		Start value.
**	  .dbs_incr		Increment value.
**	  .dbs_max		Maximum value.
**	  .dbs_min		Minimum value.
**	  .dbs_cache		Cache size.
**	  .dbs_flag		Various flag values.
**				Internal fields (create date, unique
**				sequence and text ids) are filled in just
**				before storage when the ids are
**				constructed. 
**				The owner of the sequence is filled in
**				this routine from pss_user before
**				storing.
**
**	Returns:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-apr-02 (inkdo01)
**	    Written for sequence support.
**	26-Mar-2003 (jenjo02)
**	    DEC "next" was being set to 1, not "min".
**	    Handle extreme ranges by casting DEC to float.
**	2-june-03 (inkdo01)
**	    Default start/next for negative decimal sequences was wrong
**	    (-9999.. instead of -1). Reverse effect of above change for
**	    negative sequences.
**	14-feb-2009 (dougi) bug 121657
**	    Make default max/min consistent with start values.
**	27-Jun-2010 (kschendel) b123986
**	    Don't ever reference psy_cb, because there isn't one if
**	    this is create table with identity columns.
*/

DB_STATUS
psl_csequence(
	PSS_SESBLK	*cb,
	DB_IISEQUENCE	*seqp,
	DB_ERROR   *err_blk)
{
    f8			f8temp;
    i4                err_code;
    i4			seqflag = seqp->dbs_flag;
    DB_STATUS	       status;
    PTR			bigerrtok;
    i4			bigerrtoksz;
    bool		bigerr = FALSE;
    bool		ascseq;
    bool		parmerr;
    char		dec0 = MH_PK_PLUS;
					/* dec(1) constant 0 */

    /* Make sure that the actual tuple won't end up with the temp
    ** parser flag in it!
    */
    seqp->dbs_flag &= ~DBS_DECSEEN;

    /* Check for sequence parameters whose values exceed the declared
    ** sequence type.
    ** NOTE: this usually happens with decimal parm values and 
    ** a technique used in this code to detect decimal values is to
    ** check the last byte for non-zero (as it must be, since it holds
    ** the sign). This is a bit kludgey, but should work on all 
    ** platforms, since decimal is a constructed type on all.
    */

    if (seqp->dbs_type == DB_INT_TYPE && seqflag & DBS_DECSEEN)
    {
	/* At least one parm is bigger than a INT8, search for it so
	** its name can be put in the message. */
	bigerr = TRUE;

	if (seqp->dbs_start.decval[15] != 0)
	{
	    bigerrtok = "START";
	    bigerrtoksz = sizeof("START");
	}
	else if (seqp->dbs_incr.decval[15] != 0)
	{
	    bigerrtok = "INCREMENT";
	    bigerrtoksz = sizeof("INCREMENT");
	}
	else if (seqp->dbs_max.decval[15] != 0)
	{
	    bigerrtok = "MAXVAL";
	    bigerrtoksz = sizeof("MAXVAL");
	}
	else	/* must be minval */
	{
	    bigerrtok = "MINVAL";
	    bigerrtoksz = sizeof("MINVAL");
	}
    }
    else if (seqflag & DBS_DECSEEN || seqp->dbs_type == DB_DEC_TYPE)
    {
	DB_DATA_VALUE minmaxdv;
	i4	decsize = sizeof(seqp->dbs_start.decval);
	i8	intval;
	char	maxdec[16];
	char	workdec[16];
	char	sign;
	bool	checkdec;

	/* Either type is declared decimal or at least one parameter
	** is big enough to require decimal. So coerce all to decimal. */
	if (seqp->dbs_type == 0)
	{
	    seqp->dbs_prec = 31;	/* default precision & length */
	    seqp->dbs_length = 16;
	}
	seqp->dbs_type = DB_DEC_TYPE;

	/* Prepare for possible generation of defaults. */
	minmaxdv.db_data = NULL;
	minmaxdv.db_datatype = DB_DEC_TYPE;
	minmaxdv.db_length = seqp->dbs_prec/2 + 1;
	minmaxdv.db_prec = DB_PS_ENCODE_MACRO(seqp->dbs_prec, 0);

	/* Prepare for value magnitude checking. */
	checkdec = (seqp->dbs_prec < 31);
	if (checkdec)
	{
	    /* Generate max value for precision. */
	    minmaxdv.db_data = &maxdec[0];
	    status = adc_minmaxdv(cb->pss_adfcb, NULL, &minmaxdv);
	    if (status != E_DB_OK)
		return(status);
	}

	/* Start with INCREMENT so we can determine ascseq. */
	if (!(seqp->dbs_flag & DBS_INCR))
	{
	    /* Set increment to 1 and show sequence is ascending. */
	    MEfill(decsize, 0, (PTR)&seqp->dbs_incr.decval[0]);
	    seqp->dbs_incr.decval[decsize-1] = MH_PK_PLUS | 0x10;
	}
	else if (seqp->dbs_incr.decval[15] == 0)
	{
	    /* Integer increment must be converted to decimal. */
	    intval = seqp->dbs_incr.intval;
	    if (CVl8pk(intval, seqp->dbs_prec, 0, 
		&seqp->dbs_incr.decval[16-seqp->dbs_length]) != OK)
	    {
		bigerrtok = "INCREMENT";
		bigerrtoksz = sizeof("INCREMENT");
		bigerr = TRUE;
		goto seq_bigerr;
	    }
	}
	else if (checkdec)
	{
	    /* Decimal increment value must be checked against max
	    ** for precision. */
	    MHpkabs(&seqp->dbs_incr.decval[0], 31, 0, &workdec[0]);
	    if (MHpkcmp(&maxdec[0], seqp->dbs_prec, 0, &workdec[0], 31, 0) < 0)
	    {
		bigerrtok = "INCREMENT";
		bigerrtoksz = sizeof("INCREMENT");
		bigerr = TRUE;
		goto seq_bigerr;
	    }
	}
	sign = seqp->dbs_incr.decval[15] & 0xf;
	ascseq = !(sign == MH_PK_MINUS || sign == MH_PK_AMINUS);
					/* determine order of sequence */

	/* Set up start value before max/min check. */
	if (seqp->dbs_flag & DBS_START && seqp->dbs_start.decval[15] == 0)
	{
	    /* Integer start must be converted to decimal. */
	    intval = seqp->dbs_start.intval;
	    if (CVl8pk(intval, seqp->dbs_prec, 0, 
		&seqp->dbs_start.decval[16-seqp->dbs_length]) != OK)
	    {
		bigerrtok = "START";
		bigerrtoksz = sizeof("START");
		bigerr = TRUE;
		goto seq_bigerr;
	    }
	}

	if (!(seqp->dbs_flag & DBS_MAXVAL))
	{
	    MEfill(decsize, 0, (PTR)&seqp->dbs_max.decval[0]);
	    if (ascseq)
	    {
		/* Compute max for supplied precision. */
		minmaxdv.db_data = (PTR)&seqp->dbs_max.decval[decsize-
						seqp->dbs_length];
		status = adc_minmaxdv(cb->pss_adfcb, NULL, &minmaxdv);
		if (status != E_DB_OK)
		return(status);
	    }
	    else 
	    {
		seqp->dbs_max.decval[decsize-1] = MH_PK_MINUS | 0x10;
					/* max is -1 for negative sequence */

		/* But if start is bigger, set max to start. */
		if (seqp->dbs_flag & DBS_START &&
		    MHpkcmp(&seqp->dbs_max.decval[0], 31, 0, 
				&seqp->dbs_start.decval[0], 31, 0) < 0)
		    MEcopy((PTR)&seqp->dbs_start.decval[0], decsize,
			(PTR)&seqp->dbs_max.decval[0]);
	    }
	}
	else if (seqp->dbs_max.decval[15] == 0)
	{
	    /* Integer maxvalue must be converted to decimal. */
	    intval = seqp->dbs_max.intval;
	    if (CVl8pk(intval, seqp->dbs_prec, 0, 
		&seqp->dbs_max.decval[16-seqp->dbs_length]) != OK)
	    {
		bigerrtok = "MAXVALUE";
		bigerrtoksz = sizeof("MAXVALUE");
		bigerr = TRUE;
		goto seq_bigerr;
	    }
	}
	else if (checkdec)
	{
	    /* Decimal max value must be checked against max
	    ** for precision. */
	    MHpkabs(&seqp->dbs_max.decval[0], 31, 0, &workdec[0]);
	    if (MHpkcmp(&maxdec[0], seqp->dbs_prec, 0, &workdec[0], 31, 0) < 0)
	    {
		bigerrtok = "MAXVALUE";
		bigerrtoksz = sizeof("MAXVALUE");
		bigerr = TRUE;
		goto seq_bigerr;
	    }
	}

	if (!(seqp->dbs_flag & DBS_MINVAL))
	{
	    MEfill(decsize, 0, (PTR)&seqp->dbs_min.decval[0]);
	    if (!ascseq)
	    {
		/* Compute min for supplied precision. */
		minmaxdv.db_data = (PTR)&seqp->dbs_min.decval[decsize-
						seqp->dbs_length];
		status = adc_minmaxdv(cb->pss_adfcb, &minmaxdv, NULL);
		if (status != E_DB_OK)
		    return(status);
	    }
	    else 
	    {
		seqp->dbs_min.decval[decsize-1] = MH_PK_PLUS | 0x10;
					/* min is 1 for positive sequence */

		/* But if start is smaller, set min to start. */
		if (seqp->dbs_flag & DBS_START &&
			MHpkcmp(&seqp->dbs_min.decval[0], 31, 0, 
				&seqp->dbs_start.decval[0], 31, 0) > 0)
		    MEcopy((PTR)&seqp->dbs_start.decval[0], decsize,
			(PTR)&seqp->dbs_min.decval[0]);
	    }
	}
	else if (seqp->dbs_min.decval[15] == 0)
	{
	    /* Integer minvalue must be converted to decimal. */
	    intval = seqp->dbs_min.intval;
	    if (CVl8pk(intval, seqp->dbs_prec, 0, 
		&seqp->dbs_min.decval[16-seqp->dbs_length]) != OK)
	    {
		bigerrtok = "MINVALUE";
		bigerrtoksz = sizeof("MINVALUE");
		bigerr = TRUE;
		goto seq_bigerr;
	    }
	}
	else if (checkdec)
	{
	    /* Decimal min value must be checked against max
	    ** for precision. */
	    MHpkabs(&seqp->dbs_min.decval[0], 31, 0, &workdec[0]);
	    if (MHpkcmp(&maxdec[0], seqp->dbs_prec, 0, &workdec[0], 31, 0) < 0)
	    {
		bigerrtok = "MINVALUE";
		bigerrtoksz = sizeof("MINVALUE");
		bigerr = TRUE;
		goto seq_bigerr;
	    }
	}

	if (!(seqp->dbs_flag & DBS_START))
	{
	    /* Set start to min value (or max, if negative sequence) */
	    if (ascseq)
		MEcopy((PTR)&seqp->dbs_min.decval[0], decsize,
			(PTR)&seqp->dbs_start.decval[0]);
	    else MEcopy((PTR)&seqp->dbs_max.decval[0], decsize,
			(PTR)&seqp->dbs_start.decval[0]);
	}
	else if (checkdec)
	{
	    /* Decimal start value must be checked against max
	    ** for precision. */
	    MHpkabs(&seqp->dbs_start.decval[0], 31, 0, &workdec[0]);
	    if (MHpkcmp(&maxdec[0], seqp->dbs_prec, 0, &workdec[0], 31, 0) < 0)
	    {
		bigerrtok = "START";
		bigerrtoksz = sizeof("START");
		bigerr = TRUE;
		goto seq_bigerr;
	    }
	}

	MEcopy((PTR)&seqp->dbs_start.decval[0], decsize,
			(PTR)&seqp->dbs_next.decval[0]);
					/* initial next is same as start */
	/* Set integer column values to -1. */
	seqp->dbs_start.intval = -1;
	seqp->dbs_incr.intval = -1;
	seqp->dbs_next.intval = -1;
	seqp->dbs_min.intval = -1;
	seqp->dbs_max.intval = -1;
    }
    else
    {
	i8	minival = MINI8;
	i8	maxival = MAXI8;

	/* Type is either declared integer or is integer by virtue
	** of parameter value magnitudes. Assign min/max values according 
	** to declared or default length. bigint is the default. */
	seqp->dbs_type = DB_INT_TYPE;
	if (seqp->dbs_length == 4)
	{
	    minival = MINI4;
	    maxival = MAXI4;
	}
	else seqp->dbs_length = 8;
	seqp->dbs_prec = 0;

	/* Check range of parameters against declared type. */
	if (seqp->dbs_flag & DBS_START && 
		(seqp->dbs_start.intval > maxival || 
				seqp->dbs_start.intval < minival))
	{
	    bigerrtok = "START";
	    bigerrtoksz = sizeof("START");
	    bigerr = TRUE;
	}
	else if (seqp->dbs_flag & DBS_INCR && 
		(seqp->dbs_incr.intval > maxival ||
				seqp->dbs_incr.intval < minival))
	{
	    bigerrtok = "INCREMENT";
	    bigerrtoksz = sizeof("INCREMENT");
	    bigerr = TRUE;
	}
	else if (seqp->dbs_flag & DBS_MINVAL &&
		(seqp->dbs_min.intval > maxival || 
				seqp->dbs_min.intval < minival))
	{
	    bigerrtok = "MINVAL";
	    bigerrtoksz = sizeof("MAXINL");
	    bigerr = TRUE;
	}
	else if (seqp->dbs_flag & DBS_MAXVAL && 
		(seqp->dbs_max.intval > maxival || 
				seqp->dbs_max.intval < minival))
	{
	    bigerrtok = "MAXVAL";
	    bigerrtoksz = sizeof("MAXVAL");
	    bigerr = TRUE;
	}
	if (bigerr) 
	    goto seq_bigerr;

	if (!(seqp->dbs_flag & DBS_INCR))
	    seqp->dbs_incr.intval = 1;
	ascseq = (seqp->dbs_incr.intval > 0);
	    
	/* Integer defaults. */
	if (!ascseq)
	{
	    if (!(seqp->dbs_flag & DBS_MAXVAL))
		seqp->dbs_max.intval = -1;
	    if (!(seqp->dbs_flag & DBS_MINVAL))
		seqp->dbs_min.intval = minival;
	    if (!(seqp->dbs_flag & DBS_START))
		seqp->dbs_start.intval = seqp->dbs_max.intval;

	    /* Max must be no less than start. */
	    if (!(seqp->dbs_flag & DBS_MAXVAL) && 
				seqp->dbs_start.intval > seqp->dbs_max.intval)
		seqp->dbs_max.intval = seqp->dbs_start.intval;
	}
	else
	{
	    if (!(seqp->dbs_flag & DBS_MAXVAL))
		seqp->dbs_max.intval = maxival;
	    if (!(seqp->dbs_flag & DBS_MINVAL))
		seqp->dbs_min.intval = 1;
	    if (!(seqp->dbs_flag & DBS_START))
		seqp->dbs_start.intval = seqp->dbs_min.intval;

	    /* Min must be no more than start. */
	    if (!(seqp->dbs_flag & DBS_MINVAL) && 
				seqp->dbs_start.intval < seqp->dbs_min.intval)
		seqp->dbs_min.intval = seqp->dbs_start.intval;
	}
	seqp->dbs_next.intval = seqp->dbs_start.intval;
				/* initial next is same as start */
	/* Set decimal column values to -1. */
	seqp->dbs_start.decval[15] = 0x10 | MH_PK_MINUS;
	seqp->dbs_incr.decval[15] = 0x10 | MH_PK_MINUS;
	seqp->dbs_next.decval[15] = 0x10 | MH_PK_MINUS;
	seqp->dbs_min.decval[15] = 0x10 | MH_PK_MINUS;
	seqp->dbs_max.decval[15] = 0x10 | MH_PK_MINUS;

    }

  seq_bigerr:
    if (bigerr)
    {
	/* At least one parameter value was too big for the declared 
	** sequence type. */
	(void)psf_error(6320L, 0L, PSF_USERERR,
	    &err_code, err_blk, 1,
	    bigerrtoksz, bigerrtok);
	return (E_DB_ERROR);    /* non-zero return means error */
    }

    /* Consistency checks: max >= start >= min. */
    if (seqp->dbs_type == DB_INT_TYPE)
	parmerr = (seqp->dbs_max.intval <= seqp->dbs_min.intval ||
		seqp->dbs_start.intval > seqp->dbs_max.intval ||
		seqp->dbs_start.intval < seqp->dbs_min.intval ||
		(f8temp = ((f8)seqp->dbs_max.intval - (f8)seqp->dbs_min.intval) /
		(f8)abs(seqp->dbs_incr.intval)) < 1 || 
		f8temp < (f8)seqp->dbs_cache);
    else /* if (seqp->dbs_type == DB_DEC_TYPE) */
    {
	/* Decimal checks are a bit more kludgey. */
	parmerr = FALSE;
	/* dbs_max <= dbs_min */
	if (MHpkcmp(&seqp->dbs_max.decval[0], 31, 0, 
		    &seqp->dbs_min.decval[0], 31, 0) <= 0)
	    parmerr = TRUE;
	/* dbs_start < dbs_min */
	else if (MHpkcmp(&seqp->dbs_start.decval[0], 31, 0, 
		    &seqp->dbs_min.decval[0], 31, 0) < 0)
	    parmerr = TRUE;
	/* dbs_start > dbs_max */
	else if (MHpkcmp(&seqp->dbs_max.decval[0], 31, 0, 
		    &seqp->dbs_start.decval[0], 31, 0) < 0)
	    parmerr = TRUE;
	else
	{
	    f8		f8max, f8min, f8incr;
	    char	decwk1[16];

	    /* dbs_max - dbs_min < abs(dbs_incr) */
	    MHpkabs(&seqp->dbs_incr.decval[0], 31, 0, &decwk1[0]);
	    CVpkf(decwk1, 31, 0, &f8incr);
	    CVpkf(seqp->dbs_max.decval, 31, 0, &f8max);
	    CVpkf(seqp->dbs_min.decval, 31, 0, &f8min);
	    if ( (f8temp = (f8max - f8min) / f8incr) < 1 ||
		    f8temp < (f8)seqp->dbs_cache )
		parmerr = TRUE;
	}
    }

    if (parmerr)
    {
	/* Inconsistent options - MAXVALUE, MINVALUE, START and
	** increment. */
	(void)psf_error(6317L, 0L, PSF_USERERR,
	    &err_code, err_blk, 1,
	    (i4) sizeof("MAXVALUE/MINVALUE/START/INCREMENT")-1, 
	    "MAXVALUE/MINVALUE/START/INCREMENT");
	return (E_DB_ERROR);    /* non-zero return means error */
    }

    /* Check for UNORDERED sequence inconsistencies. Must be int and 
    ** can't be MAX/MIN/INCR. */
    if (seqp->dbs_flag & DBS_UNORDERED && (seqp->dbs_type == DB_DEC_TYPE ||
		seqp->dbs_flag & (DBS_MAXVAL | DBS_MINVAL | DBS_INCR)))
    {
	(void)psf_error(6317L, 0L, PSF_USERERR,
	    &err_code, err_blk, 1,
	    (i4) sizeof("MAXVALUE/MINVALUE/INCREMENT/UNORDERED")-1, 
	    "MAXVALUE/MINVALUE/INCREMENT/UNORDERED");
	return (E_DB_ERROR);    /* non-zero return means error */
    }

    return(E_DB_OK);
}

/*{
** Name: psl_asequence	- Perform alter sequence semantic actions.
**
** Description:
** 	This routine merges the new parameters of an altered sequence
**	with the values in the existing definition. It then performs 
**	validation of the resulting definition.
**
** Inputs:
**	sess_cb				Pointer to session control block.
**	    .pss_object			PSY_CB pointer.
**	oldseqp				Pointer to iisequence row contents for
**					existing sequence
**	err_blk				Pointer to PSQ_CB error block to be filled
**					in for errors.
**
** Outputs:
**      psy_cb
**	    .psy_tuple
**	       .psy_sequence		Modified sequence tuple for replacement.
**		  .dbs_name		Name of sequence filled in by the grammar.
**		  .dbs_type		Sequence data type.
**		  .dbs_length		Size.
**		  .dbs_start		Start value.
**		  .dbs_incr		Increment value.
**		  .dbs_max		Maximum value.
**		  .dbs_min		Minimum value.
**		  .dbs_cache		Cache size.
**		  .dbs_flag		Various flag values.
**					Internal fields (create date, unique
**					sequence and text ids) are filled in just
**					before storage when the ids are
**					constructed. 
**
**	Returns:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-apr-02 (inkdo01)
**	    Written for sequence support.
**	26-Mar-2003 (jenjo02)
**	    Handle extreme ranges by casting DEC to float.
**	    Added code to reset DBS_EXHAUSTED when
**	    appropriate.
**	26-mar-03 (inkdo01)
**	    Verify that the "next" value still lies between an altered max/min pair.
**	19-may-2007 (dougi)
**	    Increase precision for decimal ops, since max prec is > 31.
**	24-sep-2007 (dougi)
**	    Remove the above ill-thought change. The catalog row must be able
**	    to hold > 31 digit dec values and it can't without an upgrade.
**	5-oct-2007 (dougi)
**	    At least replace the bit that resizes the result of the NEXT-INCR
**	    computation which is still affected by the increase in decimal
**	    precision.
**	4-sept-2008 (dougi)
**	    Fix magnitude check of integer sequences in presence of int8s.
**	7-feb-2009 (dougi) Bug 121629
**	    Jon's f8 technique doesn't work with bigint because f8 mantissas
**	    don't have enough precision (actually, they don't for really big
**	    decimal, too).
*/

DB_STATUS
psl_asequence(
	PSS_SESBLK *cb,
	DB_IISEQUENCE *oldseqp,
	DB_ERROR   *err_blk)
{
    f8			f8temp;
    i4                err_code;
    DB_STATUS	      status;
    PSY_CB	      *psy_cb;
    DB_IISEQUENCE     *newseqp;
    PTR			bigerrtok;
    i4			bigerrtoksz;
    i4			seqflag;
    bool			bigerr = FALSE;
    bool	      ascseq, parmerr;


    psy_cb = (PSY_CB *) cb->pss_object;

    newseqp = &psy_cb->psy_tuple.psy_sequence;
    seqflag = newseqp->dbs_flag;
    /* Make sure that the actual tuple won't end up with the temp
    ** parser flag in it!
    */
    newseqp->dbs_flag &= ~DBS_DECSEEN;

    /* Replacement tuple is addr'ed by newseqp. It also contains the 
    ** new values. Now we merge values from old tuple with new. First,
    ** copy constant values and old numeric parameter values to new tuple. 
    */
    newseqp->dbs_create = oldseqp->dbs_create;
    STRUCT_ASSIGN_MACRO(oldseqp->dbs_uniqueid, newseqp->dbs_uniqueid);
    newseqp->dbs_type = oldseqp->dbs_type;
    newseqp->dbs_length = oldseqp->dbs_length;
    newseqp->dbs_prec = oldseqp->dbs_prec;
    MEcopy((PTR)&oldseqp->dbs_next, sizeof(oldseqp->dbs_next), 
	(PTR)&newseqp->dbs_next);

    /* Check flags - reset corresponding flag in old tuple for later 
    ** merge. */
    if (newseqp->dbs_flag & DBS_CYCLE)
	oldseqp->dbs_flag &= ~DBS_NOCYCLE;
    if (newseqp->dbs_flag & DBS_NOCYCLE)
	oldseqp->dbs_flag &= ~DBS_CYCLE;
    if (newseqp->dbs_flag & DBS_ORDER)
	oldseqp->dbs_flag &= ~DBS_NOORDER;
    if (newseqp->dbs_flag & DBS_NOORDER)
	oldseqp->dbs_flag &= ~DBS_ORDER;
    if (newseqp->dbs_flag & DBS_CACHE)
	oldseqp->dbs_flag &= ~DBS_NOCACHE;
    else if (newseqp->dbs_flag & DBS_NOCACHE)
    {
	oldseqp->dbs_flag &= ~DBS_CACHE;
	oldseqp->dbs_cache = 0;		/* zero cache, too */
    }
    else newseqp->dbs_cache = oldseqp->dbs_cache;
    if (newseqp->dbs_flag & DBS_MAXVAL)
	oldseqp->dbs_flag &= ~DBS_NOMAXVAL;
    else MEcopy((PTR)&oldseqp->dbs_max, sizeof(oldseqp->dbs_max), 
	(PTR)&newseqp->dbs_max);
    if (newseqp->dbs_flag & DBS_MINVAL)
	oldseqp->dbs_flag &= ~DBS_NOMINVAL;
    else MEcopy((PTR)&oldseqp->dbs_min, sizeof(oldseqp->dbs_min), 
	(PTR)&newseqp->dbs_min);

    /* Now check numeric values in alter. Values must be checked
    ** for magnitude and consistency. */
    if (newseqp->dbs_flag & (DBS_RESTART | DBS_INCR | DBS_MAXVAL | 
	DBS_MINVAL | DBS_NOMAXVAL | DBS_NOMINVAL))
     if (newseqp->dbs_type == DB_INT_TYPE )
      if (seqflag & DBS_DECSEEN)
      {
	/* At least one parm is bigger than a INT4 (otherwise, DECOPT wouldn't
	** be set) - search for it so its name can be put in the message. 
	**
	** NOTE: this happens with decimal parm values and 
	** a technique used in this code to detect decimal values is to
	** check the last byte for non-zero (as it must be, since it holds
	** the sign). This is a bit kludgey, but should work on all 
	** platforms, since decimal is a constructed type on all.
	*/
	bigerr = TRUE;

	if (newseqp->dbs_start.decval[15] != 0)
	{
	    bigerrtok = "RESTART";
	    bigerrtoksz = sizeof("RESTART");
	}
	else if (newseqp->dbs_incr.decval[15] != 0)
	{
	    bigerrtok = "INCREMENT";
	    bigerrtoksz = sizeof("INCREMENT");
	}
	else if (newseqp->dbs_max.decval[15] != 0)
	{
	    bigerrtok = "MAXVAL";
	    bigerrtoksz = sizeof("MAXVAL");
	}
	else	/* must be minval */
	{
	    bigerrtok = "MINVAL";
	    bigerrtoksz = sizeof("MINVAL");
	}
     }
     else /* DB_INT_TYPE but no DBS_DECSEEN */
     {
	/* Integer parameters. */
	i8	maxival = MAXI8;
	i8	minival = MINI8;

	if (newseqp->dbs_length == 4)
	{
	    maxival = MAXI4;
	    minival = MINI4;
	}

	/* Check range of parameters against declared type. */
	if (newseqp->dbs_flag & DBS_RESTART && 
		(newseqp->dbs_start.intval > maxival || 
				newseqp->dbs_start.intval < minival))
	{
	    bigerrtok = "START";
	    bigerrtoksz = sizeof("START");
	    bigerr = TRUE;
	}
	else if (newseqp->dbs_flag & DBS_INCR && 
		(newseqp->dbs_incr.intval > maxival ||
				newseqp->dbs_incr.intval < minival))
	{
	    bigerrtok = "INCREMENT";
	    bigerrtoksz = sizeof("INCREMENT");
	    bigerr = TRUE;
	}
	else if (newseqp->dbs_flag & DBS_MINVAL &&
		(newseqp->dbs_min.intval > maxival || 
				newseqp->dbs_min.intval < minival))
	{
	    bigerrtok = "MINVAL";
	    bigerrtoksz = sizeof("MAXINL");
	    bigerr = TRUE;
	}
	else if (newseqp->dbs_flag & DBS_MAXVAL && 
		(newseqp->dbs_max.intval > maxival || 
				newseqp->dbs_max.intval < minival))
	{
	    bigerrtok = "MAXVAL";
	    bigerrtoksz = sizeof("MAXVAL");
	    bigerr = TRUE;
	}
	if (bigerr) 
	    goto seq_bigerr;

	if (newseqp->dbs_flag & DBS_INCR)
	{
	    /* New increment value. 
	    **
	    ** If the sequence value has been drawn from and
	    ** the new increment is greater than the old, then
	    ** adjust the next value down by the old increment
	    ** then up by the new increment. This yields the
	    ** correct next starting range of cached values.
	    **
	    ** If the absolute value of the new increment is 
	    ** less than the old, leave
	    ** the next value untouched as we can't allow the
	    ** new next value to fall below the "current"
	    ** next value, as one or more servers may have
	    ** already cached that range of values.
	    **
	    ** If the new increment is in the reverse direction of
	    ** the old (e.g., new increment is -1, old increment
	    ** is 1), the next value is again altered as above.
	    **
	    ** Finally, if a restart value is specified, nothing
	    ** is done here, since the next value is set to
	    ** the restart value. 
	    */

	    if (!(newseqp->dbs_flag & DBS_RESTART) &&
		oldseqp->dbs_start.intval != oldseqp->dbs_next.intval &&
		(newseqp->dbs_incr.intval > oldseqp->dbs_incr.intval &&
		newseqp->dbs_incr.intval > 0 && oldseqp->dbs_incr.intval > 0 ||
		newseqp->dbs_incr.intval < oldseqp->dbs_incr.intval &&
		newseqp->dbs_incr.intval < 0 && oldseqp->dbs_incr.intval < 0 ||
		newseqp->dbs_incr.intval > 0 && oldseqp->dbs_incr.intval < 0 ||
		newseqp->dbs_incr.intval < 0 && newseqp->dbs_incr.intval > 0))
		newseqp->dbs_next.intval = oldseqp->dbs_next.intval -
			oldseqp->dbs_incr.intval + newseqp->dbs_incr.intval;
	}
	if (newseqp->dbs_flag & DBS_RESTART)
	    newseqp->dbs_next.intval  = newseqp->dbs_start.intval;
	else newseqp->dbs_start.intval = oldseqp->dbs_start.intval;
	if (!(newseqp->dbs_flag & DBS_INCR))
	    newseqp->dbs_incr.intval = oldseqp->dbs_incr.intval;

	ascseq = (newseqp->dbs_incr.intval > 0);

	if (newseqp->dbs_flag & DBS_NOMAXVAL)
	{
	    /* Change (or set) max value. */
	    oldseqp->dbs_flag &= ~DBS_MAXVAL;
	    newseqp->dbs_max.intval = (ascseq) ? maxival : -1;
	}
	if (newseqp->dbs_flag & DBS_NOMINVAL)
	{
	    /* Change (or set) min value. */
	    oldseqp->dbs_flag &= ~DBS_MINVAL;
	    newseqp->dbs_min.intval = (ascseq) ? 1 : minival;
	}
     }
     else /* DB_DEC_TYPE */
     {
	DB_DATA_VALUE minmaxdv;
	i4	decsize = sizeof(newseqp->dbs_start.decval);
	i8	intval;
	i4	p1, s1, p2, s2;
	i4	cmpres;
	char	maxdec[16];
	char	workdec[16];
	char	workdec1[20], workdec2[20];
	char	sign, sign1;
	bool	checkdec;

	/* Type of sequence is decimal, so values must be coerced
	** (if integer constants) or moved and checked for magnitude. */

	/* Prepare for possible generation of defaults. */
	minmaxdv.db_data = NULL;
	minmaxdv.db_datatype = DB_DEC_TYPE;
	minmaxdv.db_length = newseqp->dbs_prec/2 + 1;
	minmaxdv.db_prec = DB_PS_ENCODE_MACRO(newseqp->dbs_prec, 0);

	/* Prepare for value magnitude checking. */
	checkdec = (newseqp->dbs_prec < 31);
	if (checkdec)
	{
	    /* Generate max value for precision. */
	    minmaxdv.db_data = &maxdec[0];
	    status = adc_minmaxdv(cb->pss_adfcb, NULL, &minmaxdv);
	    if (status != E_DB_OK)
		return(status);
	}

	if (newseqp->dbs_flag & DBS_INCR)
	{
	    if (newseqp->dbs_incr.decval[15] == 0)
	    {
		/* Integer increment must be converted to decimal. */
		intval = newseqp->dbs_incr.intval;
		newseqp->dbs_incr.intval = -1;
		if (CVl8pk(intval, newseqp->dbs_prec, 0, 
		   &newseqp->dbs_incr.decval[16-newseqp->dbs_length]) != OK)
		{
		   bigerrtok = "INCREMENT";
		   bigerrtoksz = sizeof("INCREMENT");
		   bigerr = TRUE;
		   goto seq_bigerr;
		}
	    }
	    else if (checkdec)
	    {
		/* Decimal increment value must be checked against max
		** for precision. */
		MHpkabs(&newseqp->dbs_incr.decval[0], 31, 0, &workdec[0]);
		if (MHpkcmp(&maxdec[0], newseqp->dbs_prec, 0, &workdec[0], 31, 0) < 0)
		{
		   bigerrtok = "INCREMENT";
		   bigerrtoksz = sizeof("INCREMENT");
		   bigerr = TRUE;
		   goto seq_bigerr;
		}
	    }
	}
	else MEcopy((PTR)&oldseqp->dbs_incr.decval[0], decsize,
				(PTR)&newseqp->dbs_incr.decval[0]);
					/* copy unchanged from old tuple */
	sign = newseqp->dbs_incr.decval[15] & 0xf;
	ascseq = !(sign == MH_PK_MINUS || sign == MH_PK_AMINUS);
					/* determine order of sequence */

	if (newseqp->dbs_flag & DBS_NOMAXVAL)
	{
	    /* [Re]set to default maximum. */
	    oldseqp->dbs_flag &= ~DBS_MAXVAL;
	    MEfill(decsize, 0, (PTR)&newseqp->dbs_max.decval[0]);
	    if (ascseq)
	    {
		/* Compute max for supplied precision. */
		minmaxdv.db_data = (PTR)&newseqp->dbs_max.decval[decsize-
						newseqp->dbs_length];
		status = adc_minmaxdv(cb->pss_adfcb, NULL, &minmaxdv);
		if (status != E_DB_OK)
		return(status);
	    }
	    else newseqp->dbs_max.decval[decsize-1] = MH_PK_MINUS | 0x10;
					/* max is -1 for negative sequence */
	}
	else if (newseqp->dbs_flag & DBS_MAXVAL)
	{
	    if (newseqp->dbs_max.decval[15] == 0)
	    {
		/* Integer maxvalue must be converted to decimal. */
		intval = newseqp->dbs_max.intval;
		newseqp->dbs_max.intval = -1;
		if (CVl8pk(intval, newseqp->dbs_prec, 0, 
		   &newseqp->dbs_max.decval[16-newseqp->dbs_length]) != OK)
		{
		   bigerrtok = "MAXVALUE";
		   bigerrtoksz = sizeof("MAXVALUE");
		   bigerr = TRUE;
		   goto seq_bigerr;
		}
	    }
	    else if (checkdec)
	    {
		/* Decimal max value must be checked against max
		** for precision. */
		MHpkabs(&newseqp->dbs_max.decval[0], 31, 0, &workdec[0]);
		if (MHpkcmp(&maxdec[0], newseqp->dbs_prec, 0, &workdec[0], 31, 0) < 0)
		{
		   bigerrtok = "MAXVALUE";
		   bigerrtoksz = sizeof("MAXVALUE");
		   bigerr = TRUE;
		   goto seq_bigerr;
		}
	    }
	}
	else MEcopy((PTR)&oldseqp->dbs_max.decval[0], decsize,
				(PTR)&newseqp->dbs_max.decval[0]);
					/* copy unchanged from old tuple */

	if (newseqp->dbs_flag & DBS_NOMINVAL)
	{
	    /* [Re]set to default minimum. */
	    oldseqp->dbs_flag &= ~DBS_MINVAL;
	    MEfill(decsize, 0, (PTR)&newseqp->dbs_min.decval[0]);
	    if (!ascseq)
	    {
		/* Compute min for supplied precision. */
		minmaxdv.db_data = (PTR)&newseqp->dbs_min.decval[decsize-
						newseqp->dbs_length];
		status = adc_minmaxdv(cb->pss_adfcb, &minmaxdv, NULL);
		if (status != E_DB_OK)
		    return(status);
	    }
	    else newseqp->dbs_min.decval[decsize-1] = MH_PK_PLUS | 0x10;
					/* min is 1 for positive sequence */
	}
	else if (newseqp->dbs_flag & DBS_MINVAL)
	{
	    if (newseqp->dbs_min.decval[15] == 0)
	    {
		/* Integer minvalue must be converted to decimal. */
		intval = newseqp->dbs_min.intval;
		newseqp->dbs_min.intval = -1;
		if (CVl8pk(intval, newseqp->dbs_prec, 0, 
		   &newseqp->dbs_min.decval[16-newseqp->dbs_length]) != OK)
		{
		   bigerrtok = "MINVALUE";
		   bigerrtoksz = sizeof("MINVALUE");
		   bigerr = TRUE;
		   goto seq_bigerr;
		}
	    }
	    else if (checkdec)
	    {
		/* Decimal min value must be checked against max
		** for precision. */
		MHpkabs(&newseqp->dbs_min.decval[0], 31, 0, &workdec[0]);
		if (MHpkcmp(&maxdec[0], newseqp->dbs_prec, 0, &workdec[0], 31, 0) < 0)
		{
		   bigerrtok = "MINVALUE";
		   bigerrtoksz = sizeof("MINVALUE");
		   bigerr = TRUE;
		   goto seq_bigerr;
		}
	    }
	}
	else MEcopy((PTR)&oldseqp->dbs_min.decval[0], decsize,
				(PTR)&newseqp->dbs_min.decval[0]);
					/* copy unchanged from old tuple */

	if (newseqp->dbs_flag & DBS_RESTART)
	{
	    if (newseqp->dbs_start.decval[15] == 0)
	    {
		/* Integer restart value must be converted to decimal. */
		intval = newseqp->dbs_start.intval;
		newseqp->dbs_start.intval = -1;
		if (CVl8pk(intval, newseqp->dbs_prec, 0, 
		   &newseqp->dbs_start.decval[16-newseqp->dbs_length]) != OK)
		{
		   bigerrtok = "RESTART"; 
		   bigerrtoksz = sizeof("RESTART");
		   bigerr = TRUE;
		   goto seq_bigerr;
		}
	    }
	    else if (checkdec)
	    {
		/* Decimal restart value must be checked against max
		** for precision. */
		MHpkabs(&newseqp->dbs_start.decval[0], 31, 0, &workdec[0]);
		if (MHpkcmp(&maxdec[0], newseqp->dbs_prec, 0, &workdec[0], 31, 0) < 0)
		{
		   bigerrtok = "RESTART";
		   bigerrtoksz = sizeof("RESTART");
		   bigerr = TRUE;
		   goto seq_bigerr;
		}
	    }
	}
	else MEcopy((PTR)&oldseqp->dbs_start.decval[0], decsize,
				(PTR)&newseqp->dbs_start.decval[0]);
					/* copy unchanged from old tuple */

	/* Reset nextval in sequence, if necessary. */
	sign1 = oldseqp->dbs_incr.decval[15] & 0xf;
	if (newseqp->dbs_flag & DBS_RESTART)
	    MEcopy((PTR)&newseqp->dbs_start.decval[0], decsize,
				(PTR)&newseqp->dbs_next.decval[0]);
					/* copy new start point */
	else if (newseqp->dbs_flag & DBS_INCR &&
		MHpkcmp(&oldseqp->dbs_start.decval[0], 31, 0, 
			&oldseqp->dbs_next.decval[0], 31, 0) != 0 &&
		(sign == sign1 &&
		((cmpres = MHpkcmp(&newseqp->dbs_incr.decval[0], 31, 0, 
			&oldseqp->dbs_incr.decval[0], 31, 0)) > 0 &&
		ascseq || cmpres < 0) || sign != sign1))
	{
	    /* If increment has been altered to a larger abs value and the
	    ** sequence has already been used (oldstart != oldcurr) or
	    ** if the increment has changed in direction, must
	    ** recompute next to assure nextvalue doesn't overlap one
	    ** in use by current transaction. */
	    MHpksub(&oldseqp->dbs_next.decval[0], 31, 0, 
		&oldseqp->dbs_incr.decval[0], 31, 0, 
		&workdec1[0], &p1, &s1);
	    MHpkadd(&workdec1[0], p1, s1,
		&newseqp->dbs_incr.decval[0], 31, 0, 
		&workdec2[0], &p2, &s2);
	    CVpkpk(&workdec2[0], p2, s2, 31, 0, &newseqp->dbs_next.decval[0]);
	}
	else MEcopy((PTR)&oldseqp->dbs_next.decval[0], decsize,
				(PTR)&newseqp->dbs_next.decval[0]);
					/* copy unchanged from old tuple */
     }
    else
    {
	/* No changes to start or incr - copy 'em all from oldseqp. */
	MEcopy((PTR)&oldseqp->dbs_start, sizeof(oldseqp->dbs_start),
			(PTR)&newseqp->dbs_start);
	MEcopy((PTR)&oldseqp->dbs_incr, sizeof(oldseqp->dbs_incr),
			(PTR)&newseqp->dbs_incr);
	MEcopy((PTR)&oldseqp->dbs_next, sizeof(oldseqp->dbs_next),
			(PTR)&newseqp->dbs_next);
    }

  seq_bigerr:
    if (bigerr)
    {
	/* At least one altered parameter value was too big for 
	** the sequence data type. */
	(void)psf_error(6320L, 0L, PSF_USERERR,
	    &err_code, err_blk, 1,
	    bigerrtoksz, bigerrtok);
	return (E_DB_ERROR);    /* non-zero return means error */
    }

    /* Finally, validate all the values: max >= start >= min. */
    if (newseqp->dbs_type == DB_INT_TYPE)
	parmerr = (newseqp->dbs_max.intval <= newseqp->dbs_min.intval ||
	    newseqp->dbs_start.intval > newseqp->dbs_max.intval ||
	    newseqp->dbs_start.intval < newseqp->dbs_min.intval ||
	    newseqp->dbs_next.intval > newseqp->dbs_max.intval ||
	    newseqp->dbs_next.intval < newseqp->dbs_min.intval ||
	    (f8temp = ((f8)newseqp->dbs_max.intval - (f8)newseqp->dbs_min.intval) /
	    (f8)abs(newseqp->dbs_incr.intval)) < 1 || 
	    f8temp < (f8)newseqp->dbs_cache);
    else /* if (newseqp->dbs_type == DB_DEC_TYPE) */
    {
	/* Decimal checks are a bit more kludgey. */
	parmerr = FALSE;
	/* dbs_max <= dbs_min */
	if (MHpkcmp(&newseqp->dbs_max.decval[0], 31, 0, 
		&newseqp->dbs_min.decval[0], 31, 0) <= 0)
	    parmerr = TRUE;
	/* dbs_start < dbs_min */
	else if (MHpkcmp(&newseqp->dbs_start.decval[0], 31, 0, 
		&newseqp->dbs_min.decval[0], 31, 0) < 0)
	    parmerr = TRUE;
	/* dbs_max < dbs_start */
	else if (MHpkcmp(&newseqp->dbs_max.decval[0], 31, 0, 
		&newseqp->dbs_start.decval[0], 31, 0) < 0)
	    parmerr = TRUE;
	/* dbs_next < dbs_min */
	else if (MHpkcmp(&newseqp->dbs_next.decval[0], 31, 0, 
		&newseqp->dbs_min.decval[0], 31, 0) < 0)
	    parmerr = TRUE;
	/* dbx_max < dbs_next */
	else if (MHpkcmp(&newseqp->dbs_max.decval[0], 31, 0, 
		&newseqp->dbs_next.decval[0], 31, 0) < 0)
	    parmerr = TRUE;
	else
	{
	    f8		f8max, f8min, f8incr;
	    char	decwk1[16];

	    /* dbs_max - dbs_min < abs(dbs_incr) */
	    MHpkabs(&newseqp->dbs_incr.decval[0], 31, 0, &decwk1[0]);
	    CVpkf(decwk1, 31, 0, &f8incr);
	    CVpkf(newseqp->dbs_max.decval, 31, 0, &f8max);
	    CVpkf(newseqp->dbs_min.decval, 31, 0, &f8min);
	    if ( (f8temp = (f8max - f8min) / f8incr) < 1 ||
		    f8temp < (f8)newseqp->dbs_cache )
		parmerr = TRUE;
	}
    }

    if (parmerr)
    {
	/* Inconsistent options - MAXVALUE, MINVALUE, START and
	** increment. */
	(void)psf_error(6317L, 0L, PSF_USERERR,
	    &err_code, err_blk, 1,
	    (i4) sizeof("MAXVALUE/MINVALUE/START/INCREMENT")-1, 
	    "MAXVALUE/MINVALUE/START/INCREMENT");
	return (E_DB_ERROR);    /* non-zero return means error */
    }

    /* Check for UNORDERED sequence inconsistencies. Must be int and 
    ** can't be MAX/MIN/INCR. */
    if (newseqp->dbs_flag & DBS_UNORDERED && (newseqp->dbs_type == DB_DEC_TYPE ||
		newseqp->dbs_flag & (DBS_MAXVAL | DBS_MINVAL | DBS_INCR)))
    {
	(void)psf_error(6317L, 0L, PSF_USERERR,
	    &err_code, err_blk, 1,
	    (i4) sizeof("MAXVALUE/MINVALUE/INCREMENT/UNORDERED")-1, 
	    "MAXVALUE/MINVALUE/INCREMENT/UNORDERED");
	return (E_DB_ERROR);    /* non-zero return means error */
    }

    /* Combine flag settings sans "EXHAUSTED". */
    newseqp->dbs_flag |= (oldseqp->dbs_flag & ~DBS_EXHAUSTED);

    /*
    ** If the sequence is exhausted (all possible values
    ** have been drawn into cache), then check if the
    ** altering of the sequence will released more values.
    **
    ** When the sequence was flagged "exhausted" (dmsiiseq.c), 
    ** the next value was set to the last cached value.
    **
    ** NB: exhaustion can only happen with non-Cyclable sequences.
    */
    if ( oldseqp->dbs_flag & DBS_EXHAUSTED )
    {
	if (newseqp->dbs_type == DB_INT_TYPE)
	{
	    i8		i8next = newseqp->dbs_next.intval;

	    /* If "next" is still the last value, increment it */
	    if ( i8next == oldseqp->dbs_next.intval )
	    {
		/* 1st "if" avoids overflow during the check. */
		if (!(ascseq && (i8next < 0 || newseqp->dbs_incr.intval > 
				MAXI8-i8next) ||
			!ascseq && (i8next > 0 || newseqp->dbs_incr.intval < 
				MINI8-i8next)))		/* overflow check */
		    i8next += newseqp->dbs_incr.intval;
		/* 2nd "if" tidies up after 1st. */
		else if (ascseq && i8next < 0 || !ascseq && i8next > 0)
		    i8next += newseqp->dbs_incr.intval;
		else if (newseqp->dbs_flag & DBS_CYCLE)
		    newseqp->dbs_flag |= DBS_EXHAUSTED;
	    }

	    /* Out of range, non-Cyclable (but at least no overflow)? */
	    if ( !(newseqp->dbs_flag & DBS_CYCLE) &&
		(i8next > newseqp->dbs_max.intval ||
		 i8next < newseqp->dbs_min.intval) )
	    {
		/* then the sequence is still exhausted */
		newseqp->dbs_flag |= DBS_EXHAUSTED;
		/* Retain last value */
		newseqp->dbs_next.intval = oldseqp->dbs_next.intval;
	    }
	    else
		newseqp->dbs_next.intval = i8next;
	}
	else
	{   /* Ditto in decimal... */
	    f8		f8onext, f8next, f8incr, f8max, f8min;

	    /* Coerce to float to avoid overflow problems */
	    CVpkf(oldseqp->dbs_next.decval, 31, 0 , &f8onext);
	    CVpkf(newseqp->dbs_next.decval, 31, 0 , &f8next);
	    CVpkf(newseqp->dbs_incr.decval, 31, 0 , &f8incr);
	    CVpkf(newseqp->dbs_min.decval, 31, 0 , &f8min);
	    CVpkf(newseqp->dbs_max.decval, 31, 0 , &f8max);

	    /* If "next" is still the last value, increment it */
	    if ( f8next == f8onext )
		 f8next += f8incr;

	    /* Out of range and non-Cyclable? */
	    if ( !(newseqp->dbs_flag & DBS_CYCLE) &&
		(f8next > f8max || f8next < f8min) )
	    {
		/* then the sequence is still exhausted */
		newseqp->dbs_flag |= DBS_EXHAUSTED;
		/* Retain last value */
		MEcopy((PTR)&oldseqp->dbs_next, sizeof(newseqp->dbs_next),
			    (PTR)&newseqp->dbs_next);
	    }
	    else
		CVfpk(f8next, 31, 0, (PTR)&newseqp->dbs_next.decval);
	}
    }

    return(E_DB_OK);
}


/*{
** Name: psl_seq_parmcheck - validate sequence numeric parameters. 
**
** Description:
** 	This routine performs the validation of the numeric parameters of 
**	a sequence definition - start value, increment, max/min value. Checks
**	are performed for both integer and decimal values.
**
** Inputs:
**	psq_cb				Parse ctl block ptr (for error handling)
**	sess_cb				Pointer to session control block.
**	    .pss_object			PSY_CB pointer.
**	pdv				Pointer to DB_DATA_VALUE defining parm.
**	seqp				Pointer to DB_IISEQUENCE tuple.
**	flag				Flag value indicating parameter type.
**
** Outputs:
**      psy_cb
**	    .psy_tuple
**	       .psy_sequence		Sequence tuple to insert.
**		  .dbs_start		Start value.
**		  .dbs_incr		Increment value.
**		  .dbs_max		Maximum value.
**		  .dbs_min		Minimum value.
**		  .dbs_flag		Various flag values.
**	    .psy_error.err_code		Filled in if an error happens:
**
**	Returns:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	2-apr-02 (inkdo01)
**	    Written for sequence support.
**	24-feb-03 (inkdo01)
**	    Detect MINI4 and convert back to integer (arrives as decimal
**	    constant with minus flag ).
**	25-feb-03 (inkdo01)
**	    Check for 0-valued increment.
**	18-may-04 (inkdo01)
**	    -MAXI4 comes in as a i8.
**	14-july-04 (inkdo01)
**	    Lexical scanner now produces bigints. Add code to possibly
**	    convert bigints back to decimal.
**	24-sep-2007 (dougi)
**	    Prohibit decimal constants > 31 digits.
**	6-june-2008 (dougi)
**	    Pass ptr to DB_IISEQUENCE for identity column support.
**	09-Sep-2008 (jonj)
**	    Number of parameters (0) missing from psf_error().
**	2-dec-2008 (dougi) BUG 121302
**	    Fix syntax error on "start with" option in identity column def.
**	27-Jun-2010 (kschendel) b123986
**	    Don't ever reference psy_cb, because there isn't one if
**	    this is create table with identity columns.
*/

DB_STATUS
psl_seq_parmcheck(
	PSS_SESBLK *cb,
	PSQ_CB	   *psq_cb,
	DB_DATA_VALUE *pdv,
	DB_IISEQUENCE *seqp,
	i4	    flag,
	bool	    minus)
{
    i4                err_code;
    DB_STATUS	       status;
    i8			intval;
    i8			*iparmp;
    PTR			dparmp;
    PTR			etokptr;
    i4			etoklen;
    char		workbig[22];	/* to convert bigint back to dec */
	
    if ((flag & DBS_START) && psq_cb->psq_mode != PSQ_CSEQUENCE &&
		psq_cb->psq_mode != PSQ_CREATE
	|| (flag & DBS_RESTART) && psq_cb->psq_mode != PSQ_ASEQUENCE)
    {
	/* START WITH only allowed on CREATE SEQUENCE or TABLE (for identity
	** sequences), RESTART only allowed on ALTER SEQUENCE. */
	psl_yerror(3, cb, psq_cb);
	return(E_DB_ERROR);
    }

    /* Set a few locals. */
    switch(flag) {
      case DBS_START:
      case DBS_RESTART:
	dparmp = (PTR)&seqp->dbs_start.decval[0];
	iparmp = (i8 *)&seqp->dbs_start.intval;
	etokptr = "[RE]START WITH";
	etoklen = sizeof("[RE]START WITH")-1;
	break;
      case DBS_INCR:
	dparmp = (PTR)&seqp->dbs_incr.decval[0];
	iparmp = (i8 *)&seqp->dbs_incr.intval;
	etokptr = "INCREMENT BY";
	etoklen = sizeof("INCREMENT BY")-1;
	break;
      case DBS_MINVAL:
	dparmp = (PTR)&seqp->dbs_min.decval[0];
	iparmp = (i8 *)&seqp->dbs_min.intval;
	etokptr = "MINVALUE";
	etoklen = sizeof("MINVALUE")-1;
	break;
      case DBS_MAXVAL:
	dparmp = (PTR)&seqp->dbs_max.decval[0];
	iparmp = (i8 *)&seqp->dbs_max.intval;
	etokptr = "MAXVALUE";
	etoklen = sizeof("MAXVALUE")-1;
	break;
    }

    if (pdv->db_datatype != DB_INT_TYPE &&
	(pdv->db_datatype != DB_DEC_TYPE ||
		DB_S_DECODE_MACRO(pdv->db_prec) != 0))
    {
	/* Start value must be integer or decimal constant. */
	(void)psf_error(6316L, 0L, PSF_USERERR,
		&err_code, &psq_cb->psq_error, 1,
		etoklen, etokptr);
	return (E_DB_ERROR);    /* non-zero return means error */
    }

    if (pdv->db_datatype == DB_INT_TYPE)
    {
	switch (pdv->db_length) {
	  case 1:
	    intval = *(i1 *)pdv->db_data;
	    break;
	  case 2:
	    intval = *(i2 *)pdv->db_data;
	    break;
	  case 4:
	    intval = *(i4 *)pdv->db_data;
	    break;
	  case 8:
	    intval = *(i8 *)pdv->db_data;
	    break;
	}

	if (minus && pdv->db_datatype != DB_DEC_TYPE)
	    intval = -intval;		/* negative start */
	*iparmp = intval;
    }
    else
    {
	if (pdv->db_length > 16)
	{
	    /* Decimal constants > 31 digits aren't supported yet. This is
	    ** likely a temporary limitation since Ingres supports up to 
	    ** 39 digit decimal - hence the use of an explicit constant. */
	    (void)psf_error(6320L, 0L, PSF_USERERR,
		&err_code, &psq_cb->psq_error, 1,
		etoklen, etokptr);
	    return (E_DB_ERROR);    /* non-zero return means error */
	}
	MEcopy(pdv->db_data, pdv->db_length, dparmp+16-pdv->db_length);

	if (minus)
	{
	    char	tempdec[16];

	    /* Test first for MINI8, in which case it is still an integer. */
	    if (MHpkcmp(dparmp, 31, 0, (char *)&dec_mini8[0], 31, 0) == 0)
	    {
		MEfill(16, 0, dparmp);
				/* clear result of decimal copy */
		*iparmp = MINI8;	/* assign integer value */
		goto mini8_exit;
	    }

	    /* Negative decimal value - copy to temp, negate & copy back. */
	    MEcopy(dparmp, sizeof(tempdec), &tempdec);
	    MHpkneg(tempdec, 31, 0, dparmp);
	}
	seqp->dbs_flag |= DBS_DECSEEN;
				/* flag at least 1 decimal constant */
    }

    /* Simple check for 0-valued increment. */
    if (flag == DBS_INCR && 
	pdv->db_datatype == DB_INT_TYPE && intval == 0)
    {
	(void)psf_error(6322L, 0L, PSF_USERERR,
		&err_code, &psq_cb->psq_error, 0);
	return (E_DB_ERROR);    /* non-zero return means error */
    }

mini8_exit:
    seqp->dbs_flag |= flag;

    return(E_DB_OK);
}
