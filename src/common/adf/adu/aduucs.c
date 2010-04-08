/*
** Copyright (c) 2001, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cv.h>
#include    <me.h>
#include    <st.h>
#include    <lo.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ercl.h>
#include    <adfint.h>	
#include    <aduucol.h>
#include    <aduucoerce.h>

/*
** Name: ADUUCS.C - Unicode collation support
**
** Decsription:
**	Contains unicode collation support functions for creating in-memory
**	Collation element tables. For the time being, we'll just work from 
**	text files, but it may be more effecient and safer for the DBMS
**	to compile first (as we do for local collation). We'll fix this in the
**	next release.
**
** History:
**	13-mar-2001 (stephenb)
**	    Created.
**	19-sep-2001 (kinte01)
**	    Add me.h to pick up definition of mecopy & mereqmem on VMS
**	24-dec-2001 (somsa01)
**	    In aduucolinit(), modified such that the varsize which is placed
**	    in the Unicode table is now a SIZE_TYPE rather than an i4.
*/

/*
** Name: aduucolinit - Initialize Unicode collation
**
** Description:
**	This function reads a compiled collation element table and creates an
**	in-memory copy. The in-memory copy consists of two parts.
**	
**		Fixed part: Contains an array of MAXI2*2 ADUUCETAB elements.
**		Element number n will describe the collation for the Unicode
** 		character with value n.
**
**		Variable part: Contains a packed list of ADU_UCEFILE 
**		structures. This structure contains a union of the various
**		types of allowable entry, the entry provides some additional
**		collation information about a character and may contain
**	 	a variable sized array of values. The variable part of the
**		table is scanned in this function, and a pointer to each entry
**		is set up in the fixed table, in the element for the character
**		to which the information pertains. This allows the information
**		to be looked up directly from the fixed part of the table.
**
** Inputs:
**	col	collation name
**	alloc	memory allocation function
**
** Outputs:
**	tbl	collation element table
**	
**	syserr		operating system error
**
** History:
**	14-mar-2001 (stephenb)
**	    Created
**	2-apr-2001 (stephenb)
**	    Minor fixes to bugs in code logic.
**	9-apr-2001 (stephenb)
**	    Add code to read in decompositions
**	24-dec-2001 (somsa01)
**	    Modified such that the varsize which is placed in the Unicode
**	    table is now a SIZE_TYPE rather than an i4.
**	12-dec-2001 (devjo01)
**	    Make sure 1st overflow entry is read at correct alignment.
**      19-feb-2002 (stial01)
**          Changes for unaligned NCHR,NVCHR data (b107084)
**	04-Feb-2002 (jenjo02)
**	    Check for empty collation file name, return OK.
**	    (B109577)
**      12-feb-2002 (gupsh01)
**          Added code to handle the mapping file. Also close the file 
** 	    if exiting abnormaly.         
**	14-Jan-2004 (gupsh01)
**	    Included output location in CMopen_col & CMclose_col.
**	23-Jan-2004 (gupsh01)
**	    Fixed up to for enabling recombination table based compression. 
**	    of unicode values.
**	02-Feb-2004 (gupsh01)
**	    Fixed adu_set_recomb_tbl.
**	04-Feb-2004 (gupsh01)
**	    Added code to read the precalculated size of
**	    recomb_tbl from compiled collation file.
**	20-Feb-2007 (gupsh01)
**	    Added code to handle the case mapping for 
**	    nchar/nvarchar data types.
**	26-feb-2008 (gupsh01)
**	    During install if form based ingbuild is unable to open the
**	    default unicode collation file, then try to open it again from
**	    the install location.
**	24-nov-2008 (gupsh01)
**	    When allocating memory for recombination table and collation
**	    table, make sure to initialize the buffer. 
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	11-May-2009 (kschendel) b122041
**	    Change allocator routine signature to match MEreqmem.  Callers
**	    will pass MEreqmem or an equivalent.
*/
STATUS
aduucolinit(
	    char	*col,
	    PTR		(*alloc)(u_i2  tag,
                         SIZE_TYPE  size,
                         bool   clear,
                         STATUS *status ),
	    ADUUCETAB	**tbl,
	    char	**vartab,
	    CL_ERR_DESC	*syserr)
{
    char	colname[DB_MAXNAME+1];
    char	*buf = NULL;
    char	*tptr;
    ADUUCETAB	*tbase;
    char	*ipointer;
    STATUS	stat;
    SIZE_TYPE	*varsize;
    SIZE_TYPE	vsize_left;
    SIZE_TYPE	vtab_size;
    i4		left;
    i4		bytes_read;
    u_i2	**recomb_def2d;
    u_i2	*recomb_def1d;
    char	*workspace;
    i4		allocated = 0;
    SIZE_TYPE	*recomb_tbl_size = 0;
    SIZE_TYPE	recombsize= 0;

    STncpy(colname, col, DB_MAXNAME);
    colname[ DB_MAXNAME ] = EOS;
    STtrmwhite(colname);

    *tbl = (ADUUCETAB*)NULL;
    *vartab = NULL;

    if (colname[0] == EOS)
	return OK;

    /* open the file */

    if (CMopen_col(colname, syserr, CM_COLLATION_LOC) != OK)
    {
	/* During installation ingbuild might try to open
        ** the default unicode collation file, especially for
        ** UTF8 character set. Allow searching for the
        ** unicode collation file in the install location as
        ** well.
        */
        if (CMischarset_utf8())
        {
          if (CMopen_col(colname, syserr, CM_DEFAULTFILE_LOC) != OK)
            return FAIL;
        }
        else
          return FAIL;
    }

    /* grab memory */

    tptr = (*alloc)(0, ENTRY_SIZE, TRUE, &stat);
    if (tptr == NULL || stat != OK)
    {
	 _VOID_ CMclose_col(syserr, CM_COLLATION_LOC);
	return (FAIL);
    }
    *tbl = tbase = (ADUUCETAB *)tptr;
    /* must allocate, since it needs to be on a pointer boundary */
    buf = MEreqmem(0, COL_BLOCK, TRUE, &stat);
    if (buf == NULL || stat != OK)
    {
	_VOID_ CMclose_col(syserr, CM_COLLATION_LOC);
	return (FAIL);
    }
    /* read and parse table. first section contains the base table */
    for (bytes_read = 0; bytes_read < INSTRUCTION_SIZE + ENTRY_SIZE;)
    {
	/* read next buffer */
	stat = CMread_col(buf, syserr);
	if (stat != OK)
	{
	    /* @FIX_ME@ Free tptr? */
	    MEfree(buf);
	    _VOID_ CMclose_col(syserr, CM_COLLATION_LOC);
	    return(FAIL);
	}
	bytes_read += COL_BLOCK;

	if (bytes_read <= ENTRY_SIZE)
	{
	    /* reading basic table, copy into memory */
	    MEcopy(buf, COL_BLOCK, tptr);
	    tptr += COL_BLOCK;
	    continue;
	}
	else if (bytes_read <= ENTRY_SIZE + COL_BLOCK)
	{
	    /* the first read past end of basic table */
	    /* copy rest of basic */
	    MEcopy(buf, ENTRY_SIZE - (bytes_read - COL_BLOCK), tptr);
 
	    tptr = buf + (ENTRY_SIZE - (bytes_read - COL_BLOCK));
	    left = bytes_read - ENTRY_SIZE;

	    /* we are now into variable sized records, allocate memory */
	    if (left < sizeof(SIZE_TYPE))
	    {
		/* read some more */
		stat = CMread_col(buf, syserr);
		if (stat != OK)
		{
		    /* @FIX_ME@ Free tptr? */
		    MEfree(buf);
 		    _VOID_ CMclose_col(syserr, CM_COLLATION_LOC);
		    return(FAIL);
		}
		bytes_read += COL_BLOCK;
		tptr = buf;
	    }
	    varsize = (SIZE_TYPE *)(tptr);
	    if (*varsize)
	    {
	        tptr += sizeof(SIZE_TYPE);
	        tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
	        left = COL_BLOCK - (tptr - buf);

	        recomb_tbl_size = (SIZE_TYPE *)(tptr); 
		tptr += sizeof(SIZE_TYPE);
                tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
                left = COL_BLOCK - (tptr - buf);
		recombsize = *recomb_tbl_size;

		ipointer = (*alloc)(0, (u_i4)*varsize + (u_i4)*recomb_tbl_size, 
				TRUE, &stat);
		if (ipointer == NULL || stat != OK)
		{
		    /* @FIX_ME@ Free tptr? */
		    MEfree(buf);
		    _VOID_ CMclose_col(syserr, CM_COLLATION_LOC);
		    return (FAIL);
		}
		*vartab = ipointer;
	        workspace = ipointer + (u_i4 )(*varsize);
	    }
	    else
	    {
		/* nothing else to do */
		*vartab = NULL;
		MEfree(buf);
		_VOID_ CMclose_col(syserr, CM_COLLATION_LOC);
		return (OK);
	    }

	    /* copy the rest of current buffer into var table */
	    vsize_left = vtab_size = *varsize;
	    if (vsize_left < left)
	    {
		MEcopy(tptr, vsize_left, ipointer);
		/* we're done reading useful data */
		break;
	    }
	    else
	    {
		MEcopy(tptr, left, ipointer);
		ipointer += left;
		vsize_left -= left;
	    }
	}
	else
	{
	    if (vsize_left < COL_BLOCK)
	    {
		MEcopy(buf, vsize_left, ipointer);
		/* we're done reading useful data */
		break;
	    }
	    else
	    {
		MEcopy(buf, COL_BLOCK, ipointer);
		ipointer += COL_BLOCK;
		vsize_left -= COL_BLOCK;
	    }
	}
    }
    /*
    ** the fixed sized part of the buffer is now in the base memory table
    ** and the variable sized part is in the varaible sized table
    ** We are done reading the useful contents of the file, wrap that up
    */
    _VOID_ CMclose_col(syserr, CM_COLLATION_LOC);
    MEfree(buf);

    /* set up the combining character table for base table entries */
    adu_init_defaults (&workspace, tbase, &recomb_def2d, &recomb_def1d);

    /* re-set */
    ipointer = *vartab;
    vsize_left = vtab_size;
    /* 
    ** now we need to set up the base table entries and pointers
    ** from the variable sized table. This gives us a fast lookup.
    */

    for (;;)
    {
	char		*tptr;
	ADU_UCEFILE	*memrec = (ADU_UCEFILE *)ipointer;
	i4		num_vals;
	i4		entry_no;
	ADU_COMBENT	*elist;
	STATUS		stat;

	switch (memrec->type)
	{
	    case CE_COMBENT:
		num_vals = memrec->entry.adu_combent.num_values;
		entry_no = memrec->entry.adu_combent.values[0];
		/* set up base table */
		tbase[entry_no].flags |= CE_COMBINING;
		/*
		** check for linked list of combining entries, this
		** occurs when a character is the start character of
		** more than one combining entry
		*/
		if (elist = tbase[entry_no].combent)
		{
		    for(; elist->next_combent != NULL; 
			elist = elist->next_combent);
		    elist->next_combent = &memrec->entry.adu_combent;
		}
		else
		    tbase[entry_no].combent = &memrec->entry.adu_combent;

		/* re-set ipointer */
		tptr = ipointer + sizeof(ADU_UCEFILE) + 
		    (num_vals * sizeof(u_i2));
		tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
		vsize_left = vsize_left - (tptr - ipointer);
		ipointer = tptr;
		break;

	    case CE_OVERFLOW:
		/* set up entry */
		num_vals = memrec->entry.adu_ceoverflow.num_values;
		entry_no = memrec->entry.adu_ceoverflow.cval;
		tbase[entry_no].flags |= CE_HASOVERFLOW;
		/* can only be one overflow for each base character */
		tbase[entry_no].overflow = &memrec->entry.adu_ceoverflow;

		/* re-set ipointer */
		tptr = ipointer + sizeof(ADU_UCEFILE) + 
		    (num_vals * sizeof(memrec->entry.adu_ceoverflow.oce) 
		     * MAX_CE_LEVELS);
		tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
		vsize_left = vsize_left - (tptr - ipointer);
		ipointer = tptr;
		break;
	    case CE_CDECOMP:
		/* set up entry */
		num_vals = memrec->entry.adu_decomp.num_values;
		entry_no = memrec->entry.adu_ceoverflow.cval;
		tbase[entry_no].flags |= CE_HASCDECOMP;
		tbase[entry_no].decomp = &memrec->entry.adu_decomp;

		/* re-set ipointer */
		tptr = ipointer + sizeof(ADU_UCEFILE) + (num_vals * sizeof(u_i2));
		tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
		vsize_left = vsize_left - (tptr - ipointer);
		ipointer = tptr;

		/* set up the recom_tbl for the entry */
		/* FIXME: This code assumes that a decomposition 
		** in this table will have only one recombination 
		** character as specified in UTR15 Unicode 4.0
		*/
		stat = adu_set_recomb_tbl (tbase, entry_no, &recomb_def2d,
					&recomb_def1d, &allocated, &workspace);

		if (allocated > recombsize)
		  return (FAIL);

		break;

	    case CE_CASEMAP:
		/* set up entry */
		num_vals = memrec->entry.adu_casemap.num_lvals +  
			memrec->entry.adu_casemap.num_tvals +
			memrec->entry.adu_casemap.num_uvals - 1; 

		entry_no = memrec->entry.adu_casemap.cval;
		tbase[entry_no].flags |= CE_HASCASE;
		tbase[entry_no].casemap = &memrec->entry.adu_casemap;

		/* re-set ipointer */
		tptr = ipointer + sizeof(ADU_UCEFILE) + (num_vals * sizeof(u_i2));
		tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
		vsize_left = vsize_left - (tptr - ipointer);
		ipointer = tptr;
		break;

	    default:
		/* bad record! */
		return (FAIL);
	}
	if (vsize_left <= 0)
	    break;
    }
/*
    {
    STATUS db_stat = OK;
    char converter[MAX_LOC];
    if ((db_stat = adu_getconverter(converter)) == E_DB_OK)
      adu_readmap (converter);
    else
      adu_readmap ("default");
    }
*/

    return(OK);
}

/* Name: adu_init_defaults - Initializes the recombination table in unicode
**			     collation table.
** Description:
**	This routine initializes the default tables used with recombination
**	of base and combining tables. 
** Input: 
**
**	workspace - Memory area where to place the default values 
**    	ucetab	     - Pointer to unicode collation element table.
**    	recomb_def2d - Pointer to default 2D array.
**    	recomb_def1d - Pointer to default array.
**
** Output: 
**	
**	E_DB_OK	if operation successful.
**
** History:
**	23-Jan-2004 (gupsh01)
**	    Added.
**	04-Feb-2004 (gupsh01)
**	    Fixed memory allocation here.
*/
STATUS
adu_init_defaults(
    char	**workspace,
    ADUUCETAB	*ucetab,
    u_i2	***recomb_def2d,
    u_i2	**recomb_def1d)
{
    i4		i=0;
    char	*iptr;
    STATUS	status;

    iptr = *workspace; 
    *recomb_def1d = (u_i2 *) iptr;
    iptr += sizeof(u_i2) * 256; 
    iptr = ME_ALIGN_MACRO(iptr, sizeof(PTR));

    *recomb_def2d = (u_i2 **)iptr; 
    iptr += sizeof(u_i2 *) * 256; 
    iptr = ME_ALIGN_MACRO(iptr, sizeof(PTR));

    for ( i=0; i < 256; i++)
      (*recomb_def2d)[i] = *recomb_def1d;

    for ( i=0; i < ENTRY_SIZE/sizeof(ADUUCETAB); i++)
      ucetab[i].recomb_tab = *recomb_def2d;

    *workspace = iptr;

    return E_DB_OK;
}

/* Name: adu_set_recomb_tbl - Set recombination table in memory. 
**
** Description:
**      This routine sets the base characer in recombination table. 
** Input:
**
**    ucetab 	- Unicode collation table. 
**    basechar 	- The entry in the collation table 
**    alloc 	- Memory allocation function.
**    recomb_def2d - default 2d table.
**    recomb_def1d - default table.
**
** Output:
**
**      E_DB_OK if operation successful.
**
** History:
**      23-Jan-2004 (gupsh01)
**          Added.
**	02-Feb-2004 (gupsh01)
**	    Fixed for correctly initiating the recombination table.
**	04-Feb-2004 (gupsh01)
**	    Fixed memory allocation. recombination table memory is
**	    now allocated beforehand. 
*/
STATUS
adu_set_recomb_tbl(
    ADUUCETAB	*ucetab,
    u_i2	basechar,
    u_i2	***recomb_def2d,
    u_i2	**recomb_def1d,
    i4		*allocated,
    char	**workspace
)
{
    u_i2 	firstchar; 
    u_i2 	secondchar; 
    STATUS	stat;
    i4		i;

    if (ucetab[basechar].decomp && *workspace) 
    {
      firstchar = ucetab[basechar].decomp->dvalues[0];
      secondchar = ucetab[basechar].decomp->dvalues[1];

      if (ucetab[firstchar].recomb_tab == *recomb_def2d)
      {
	ucetab[firstchar].recomb_tab = (u_i2 **) (*workspace);
	*workspace += sizeof (u_i2 *) * 256;
	*workspace = ME_ALIGN_MACRO (*workspace, sizeof(PTR));
	*allocated += sizeof (u_i2 *) * 256;	
	for (i=0; i < 256; i++)
        {
	  if (i == ((secondchar & 0xFFFF) >> 8))
	  {
	    ucetab[firstchar].recomb_tab[i] = (u_i2 *) (*workspace);
	    *workspace += sizeof(u_i2) * 256;
	    *workspace = ME_ALIGN_MACRO (*workspace, sizeof(PTR));
	    *allocated += sizeof(u_i2) * 256;	
	  }
	  else 
	    ucetab[firstchar].recomb_tab[i] = *recomb_def1d;
        }
      }
      else if (ucetab[firstchar].recomb_tab[(secondchar & 0xFFFF) >> 8] == 
		*recomb_def1d)
      {
	ucetab[firstchar].recomb_tab[(secondchar & 0xFFFF) >> 8] = 
		(u_i2 *) (*workspace);
	*workspace += sizeof(u_i2) * 256;
	*workspace = ME_ALIGN_MACRO (*workspace, sizeof(PTR));
	*allocated += sizeof(u_i2)*256;	
      }
      ucetab[firstchar].recomb_tab[
	(secondchar & 0xFFFF) >> 8][secondchar & 0xFF] = basechar;
    }
}

/* Name: adu_get_recomb - Recombines a base and a combining character.
** 
** Description:	  
**	This routine uses the recombination table to combines a base 
**	character and a combining character to get a new base character. 
**
** Input:
**	adf_scb	- ADF session control block.
**	basechar - base character to combine with.
**	combining - combing character.
**
** Output:
**	Recombined character if recombination possible, FAILS otherwise.
**
** History:
**	23-Jan-2004 (gupsh01)
**	   Added.
**      12-Sep-2007 (hanal04) Bug 119061
**          Disallow collation table access if db isn't Unicode enabled.
*/
u_i2
adu_get_recomb(
    ADF_CB   *adf_scb,
    u_i2      basechar,
    u_i2      combining)
{
    ADUUCETAB   *ucetab = (ADUUCETAB *)adf_scb->adf_ucollation; 
    u_i2 	newbase;

    if (ucetab == (ADUUCETAB *) NULL)
        return (adu_error(adf_scb, E_AD5082_NON_UNICODE_DB, 0));
                                /* better be Unicode enabled */
    newbase = 
     (ucetab[basechar].recomb_tab[(combining & 0xFFFF) >> 8][combining & 0xFF]);

    if (newbase)
      return newbase;
    else 
      return FAIL; /* cannot combine any more */
}

/* Name: adu_map_conv_normc - combines the unicode characters based on
**                            normal form C rules mentioned in UTR15.
** Description:
**      This routines, combines the unicode characters, in normal form D
**      to normal form c.
**
** Input:
**    adf_scb   - Adf session control block.
**    ucetab    - Pointer to the unicode collation table.
**    basechar  - Base character to lookup for.
**    comblist  - List of combining characters.
**    combcount - Count of combing characters.
**    result    - Pointer for for resultant Normal Form C characters.
**    outbuflen - Size of output buffer available.
**    resultcnt - Count of the resultant Normal Form C characters.
**
** Output:
**
**    result    - Normal Form C character string.
**    resultcnt - Count of the resultant Normal Form C characters.
**
** History:
**      06-jan-2004 (gupsh01)
**          Created.
**	10-jun-2005 (gupsh01)
**	    Fixed this routine to handle errors
**	    better.
*/
STATUS
adu_map_conv_normc(
    ADF_CB      *adf_scb,
    ADUUCETAB   *ucetab,
    u_i2        basechar,
    u_i2        *comblist,
    i4          combcount,
    u_i2        *result,
    i4          *resultcnt
)
{
    u_i4        baseIterant;
    u_i4        savedIterant;
    i4          cmbcnt = 0;
    i4          rescnt = 0;
    i4          i = 0;
    STATUS      stat = E_DB_OK;
    u_i2	resultbuf[1024];
    u_i2	*bufptr;

    baseIterant = basechar;
    cmbcnt = combcount;

    while (cmbcnt-- > 0)
    {
      savedIterant = baseIterant;
      if ((baseIterant = adu_get_recomb (adf_scb,
                           baseIterant, comblist[i++])) == FAIL)
      {
        /* done here */
        /* It will be nice to return an error */
        baseIterant = savedIterant;
	cmbcnt++;
	i--;
        break;
      }
    }

    /* Put remaining characters in result */
    if (cmbcnt > 0 )
    {
	if (cmbcnt > 1024)
	  bufptr = (UCS2 *) MEreqmem (0, sizeof(UCS2) * cmbcnt, TRUE, &stat);
	else 
	  bufptr = resultbuf;
	
	bufptr[rescnt++] = baseIterant;				
	for (; i < cmbcnt; i++)
	  bufptr[rescnt++] = comblist[i];			
		
	MEcopy (bufptr, sizeof(UCS2) * rescnt, result);

	if (cmbcnt > 1024)
	  MEfree(bufptr);
    }
    else
    {
	*result = baseIterant;
	rescnt++;
    }

    *resultcnt = rescnt;
    return E_DB_OK;
}
