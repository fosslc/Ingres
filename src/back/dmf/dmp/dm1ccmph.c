/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <tr.h>
#include    <st.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dmppn.h>
#include    <dm1c.h>
#include    <dm1cn.h>
#include    <dm1ch.h>
#include    <dm1r.h> 
#include    <dm0m.h>
/*
**
**  Name:  DM1CCMPH.C - HIDATA tuple-level compression routines.
**
**  Description:
**      This file contains the routines needed to (de)compress tuples
**      using "HIDATA" compression.  They are usually called via the
**	dispatch vectors in the DMP_ROWACCESS tuple access descriptor.
**	While these routines don't care if the data is a base tuple or
**	something else, like an index key, Ingres doesn't at present
**	have syntax to specify HIDATA key compression.  (Nor does it
**	sound like a good idea, given how slow HIDATA is.)
**
**	HIDATA compression uses LZW compression against a cleaned-up
**	input row.  Since LZW is unlikely to be effective against short
**	rows, or rows with very random data, a leading byte is used to
**	indicate whether the result is compressed or as-is.
**
**	compress - compress a record
**	uncompress - uncompress a record
**	compexpand - how much can a tuple expand when compressed
**
**
**  History:
**	27-oct-1994 (shero03)
**	    Created;
**	13-dec-1994 (wolf)
**	    Change u_long's to u_i4's; the latter is defined
**	29-dec-1994 (shero03)
**	    B66061 use normal compression for index pages
**	    In compression, don't rely on output area size.
**	2-mar-1995 (shero03)
**	    Bug #B67267
**	    Improve Initialze by MEfilling the dictionary
**	11-mar-1995 (shero03)
**	    Make UNUSED constant portable 
**	29-apr-96 (nick)
**	    Improve compression performance ; we would attempt to compress
**	    garbage which not unsurprisingly gave poor results. #75579
**	17-jul-1996 (ramra01)
**	    Alter Table Project: For Uncompress with Add /Drop col
**	    an intermediate buffer needs to be provided to expand/
**	    extract valid columns. Avoid using stack for VPS.
**	    Would be better to alloc space on the rcb to be used
**	    as intermediate buffer with size of reltotwid.
**	13-sep-1996 (canor01)
**	    Add rcb to dmpp_uncompress call to pass temporary buffer
**	    for uncompression.  Use the temporary buffer for uncompression
**	    when available.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to dmpdata.c.
**	25-Feb-1998 (shero03)
**	    Account for a null coupon.
**	20-aug-99 (cucjo01)
**	    Fixed incorrect matchup of comment statement in order to
**	    fix build error.
**      20-jan-99 (stial01)
**          Another change for B98408, dm1ch_uncompress, record may be NULL.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	11-Apr-2008 (kschendel)
**	    RCB's adfcb now typed as such, minor fixes here.
**	21-Oct-2009 (kschendel) SIR 122739
**	    Integrate new rowaccessor scheme changes.
*/


/*
 **     Local Data structures
 */

#define BITS           11
#define MAX_CODE       ((1 << BITS) - (unsigned) 1)
#define TABLE_SIZE     2459L
#define END_OF_STREAM  256
#define BUMP_CODE      257
#define FLUSH_CODE     258
#define FIRST_CODE     259
#define UNUSED         0xFFFF     /* (1 << BITS) */
#define FIRSTBITS      9
#define FIRSTBUMPCODE  ((1 << FIRSTBITS) - (unsigned) 1)

typedef struct bit_file {
	u_char        *pDataPtr;
	u_i2          cbDataLen;
	u_i2          cResidual;
	u_i4        Residual;
	} BIT_FILE;

typedef struct dictionary {
	u_i2  CodeValue;
	u_i2  ParentCode;
	char  Character;
	} DICTIONARY;

typedef struct dictcontrol {
	u_i2  NextCode;
	u_i2  CurrentCodeBits;
	u_i2  NextBumpCode;
	} DICTCONTROL;


/*
 **	Forward function references
 */

static void dm1ch_InitDictionary(
	DICTIONARY     *pDict,
	DICTCONTROL    *pDictCtrl);

static u_i2 dm1ch_FindChildNode(
	DICTIONARY     *pDict,
	u_i2           ParentCode,
	u_i2           ChildCharacter);

static u_i2 dm1ch_DecodeString(
	DICTIONARY     *pDict,
	char *         pDecodeStack,
	u_i2           Count,
	u_i2           Code);

static u_i4 dm1ch_InputBits(
	BIT_FILE *     pBitFile,
	u_i2           BitCount);

static i4  dm1ch_OutputBits(
	BIT_FILE *     pBitFile,       /* Ptr(BitFile) */
	u_i4           lCode,          /* Code to Emit */
	u_i2           cbCode);        /* Number of bits to Emit */

static int dm1ch_CloseBitFile(
	BIT_FILE *     pBitFile);


/*
**  Name:  dmpp_compress - Compress a record, given pointers to attributes
**
**  Description:
**     Compress the record into the work area provided
**
**  Compression Algorithms:
**     Compressed indicator - a one byte compressed indicator is written
**	to indicate whether the data is compressed or a copy of source
**
**	Nullable datatypes - a one byte null descriptor (indicating whether
**	the value is NULL) is written before any field values.  The field is
**	then set to 0x00.
**
**	C - the value is set to 0x00 after the last non-blank character.
**
**	CHAR - the value is set to 0x00 after the last valid character.
**
**	VARCHAR, TEXT - the values is set to 0x00 after the last valid
**	character - determined by the length field
**
**	The data is then compressed using an LZW 9-11bit variable code.
**
**	The compressor reads in new symbols one at-a-time from the input
**	buffer.  It then  checks to see if the combination of the current
**	symbol and the current code are already defined in the dictionary.
**	If they are not, they are added to the dictionary, and we start over
**	with a new symbol code.  If they are, the code for the combination
**	of the code and character becomes our new code.  Note that in this
**	enhanced version of LZW, the encoder needs to check the codes for
**	boundary conditions.
**
**  Inputs:
**	rac		Pointer to DMP_ROWACCESS info struct.
**	rec		Pointer to record to compress.
**	rec_size	Uncompressed record size.
**	crec		Pointer to an area to return compressed record.
**
**  Outputs:
**	crec_size	Pointer to an area where the compressed record size
**			is returned.
**
**  Returns:
**	E_DB_OK		Tuple compressed
**	E_DB_ERROR	Tuple could not be compressed
**
**
**  History:
**	27-oct-1994 (shero03)
**	    Created;
**	29-dec-1994 (shero03)
**	    In compression, don't rely on output area size.
**	29-apr-1996 (nick)
**	    For some yet to be adequately explained reason, the foreplay 
**	    before the compression described above wasn't being done ; this 
**	    typically meant that the compression results were poor.  We fix 
**	    this here by altering the input record to null out all those bits 
**	    which we can't actually see i.e. the 'value' part of a NULL 
**	    attribute and the bumf past the end of variable length attributes.
**	    We don't touch anything else as there is no need. #75579
**	25-Feb-1998 (shero03)
**	    Account for a null coupon.
**	21-sep-2004 (thaju02)
**	    If col previously dropped, do not compress dropped col. (B113110)
**	09-feb-2005 (gupsh01)
**	    Add case for nvarchar.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
*/

DB_STATUS
dm1ch_compress(
	DMP_ROWACCESS	*rac,
	char		*rec,
	i4		rec_size,
	char		*crec,
	i4		*crec_size)

{
	u_char        *pIn;
	u_char        *pOut;
	i4            iInSize;
	i4            iOutSize;
	BIT_FILE       BitFile;
	i2             Character;
	u_i2           StringCode;
	u_i2           index;
	i4            rc;
	DICTCONTROL    DictCtrl;
	DICTIONARY     Dict[TABLE_SIZE];
	i4        i;
	i2             att_ln, str_ln;
	i2             type;
	i2		abs_type;
    	i4		dt_bits;
	DB_ATTS        *att;
	DB_ATTS		**attpp;
        ADF_CB		adf_scb;

	MEfill(sizeof(ADF_CB),0,(PTR)&adf_scb);

        adf_scb.adf_maxstring = DB_MAXSTRING;

	/* preen the input record of garbage */
	pIn = (u_char *)rec;
	attpp = rac->att_ptrs;
	i = rac->att_count;

	while (--i >= 0)
	{
	    att = *attpp++;
	    att_ln = att->length;
	    type = att->type;

	    if (att->ver_dropped > att->ver_added)
		continue;

	    /* 
	    ** Obtain info about the data type. 
	    ** It's used to check for coupons then later for variable len
	    */
	    abs_type = (type < 0) ? -type: type;
	    adi_dtinfo(&adf_scb, abs_type, &dt_bits);

	    if ((type < 0) &&
	    	!(dt_bits & AD_PERIPHERAL))
	    {
		/* nullable */
		if (pIn[--att_ln] & ADF_NVL_BIT)
		{
		    /* it is null so zap the value */
		    MEfill(att_ln, 0x00, pIn);

		    pIn += att->length;
		    continue;
		}
		type = (-type);
	    }

	    /*
	    ** either not a nullable datatype or 
	    ** nullable but not null right now
	    */
	    switch (type)
	    {
	    /*
	    ** currently only interested in variable length 
	    ** attributes ; it is these which potentially have
	    ** garbage past their end - all others are either complete 
	    ** or blank padded already
	    */
	    case DB_VCH_TYPE:
	    case DB_TXT_TYPE:
	    case DB_VBYTE_TYPE:
		/* get length and adjust */
	        I2ASSIGN_MACRO(((DB_TEXT_STRING *)pIn)->db_t_count, str_ln);
		str_ln += DB_CNTSIZE;

		if (str_ln < att_ln)
		    MEfill((att_ln - str_ln), 0x00, (pIn + str_ln));

		break;

	    case DB_NVCHR_TYPE:
		/* get length and adjust */
	        I2ASSIGN_MACRO(((DB_TEXT_STRING *)pIn)->db_t_count, str_ln);
		str_ln = str_ln * sizeof(UCS2) + DB_CNTSIZE;

		if (str_ln < att_ln)
		    MEfill((att_ln - str_ln), 0x00, (pIn + str_ln));

		break;

	    default:
		break;
	    }
	    /* point to next attribute */
	    pIn += att->length;
	}

	/* now compress the preened record */
	pIn = (u_char*)rec;
	pOut = (u_char*)crec;
	iInSize = rec_size;

	/* write compression indicator */
	*pOut++ = (char)TRUE;

	dm1ch_InitDictionary(&Dict[0], &DictCtrl);

	BitFile.pDataPtr = pOut;
	BitFile.cbDataLen = (u_i2)(rec_size);
	BitFile.cResidual = 0;
	BitFile.Residual = 0L;

	StringCode = pIn[0];
	pIn++;
	iInSize--;

	while (iInSize > 0)
	{
	    Character = pIn[0];
	    pIn++;
	    iInSize--;
	    index = dm1ch_FindChildNode(&Dict[0], StringCode, Character);

	    if (Dict[index].CodeValue != UNUSED)
	    {
	        StringCode = Dict[index].CodeValue;
	    }
	    else
	    {
	        Dict[index].CodeValue = DictCtrl.NextCode++;
	        Dict[index].ParentCode = StringCode;
	        Dict[index].Character = (char) Character;
	        rc = dm1ch_OutputBits(&BitFile,
	                       (u_i4) StringCode,
	                       DictCtrl.CurrentCodeBits);
	        if (rc == -1)
	          break;	/* Compression didn't compress */

	        StringCode = Character;

	        if (DictCtrl.NextCode > (u_i2) MAX_CODE)
	        {

	             rc = dm1ch_OutputBits(&BitFile,
	                            (u_i4) FLUSH_CODE,
	                            DictCtrl.CurrentCodeBits);
	             if (rc == -1)
	               break;	/* Compression didn't compress */

	             dm1ch_InitDictionary(&Dict[0], &DictCtrl);
	        }	/* Dictionary has filled up */
	        else if (DictCtrl.NextCode > DictCtrl.NextBumpCode)
	        {

	             rc = dm1ch_OutputBits(&BitFile,
	                            (u_i4) BUMP_CODE,
	                            DictCtrl.CurrentCodeBits);
	             if (rc == -1)
	                 break;		/* Compression didn't compress */

	              DictCtrl.CurrentCodeBits++;
	              DictCtrl.NextBumpCode <<= 1;
	              DictCtrl.NextBumpCode |= 1;
	        }	/* Increase the number of code bits */
	    }
	}

	if (rc == 0)
	    rc = dm1ch_OutputBits(&BitFile,
	                   (u_i4) StringCode,
	                   DictCtrl.CurrentCodeBits);

	if (rc == 0)
	    rc = dm1ch_OutputBits(&BitFile,
			(u_i4) END_OF_STREAM,
			DictCtrl.CurrentCodeBits);

	if (rc == 0)
	    rc = dm1ch_CloseBitFile(&BitFile);

	if (rc == 0)
	{
	    iOutSize = rec_size - BitFile.cbDataLen + 1;
	}
	else  /* compression failed to reduce the size, so use original */
	{
	    /* write compression indicator */
	    pOut = (u_char*)crec;
	    *pOut++ = (char)FALSE;

	    /* write original data */
	    MEcopy(rec, rec_size, pOut);
	    iOutSize = rec_size + 1;
	}

	*crec_size = iOutSize;      /* Return the size of compressed data */
	return(E_DB_OK);

}  /* dm1ch_compress */

/*
**  Name:  dmpp_uncompress - Uncompress a record, given pointers to attributes
**
**  Description:
**     Uncompress the record into the workarea provided and return the size
**
**  Uncompression Algorithms:
**     Compressed indicator - a one byte compressed indicator is written
**	to indicate whether the data is compressed or a copy of source
**
**	Nullable datatypes - a one byte null descriptor (indicating whether
**	the value is NULL) is written before any field values.  The field is
**     then set to 0x00.
**
**	It reads in the codes and converts the codes to a string of
**	characters.  The only catch occurs when the encoder encounters a
**	CHAR+STRING+CHAR+STRING+CHAR sequence.  When this occurs, the encoder
**	outputs a code that in not presently defined in the table.  This is
**	handled as an exception.  All of the special input codes are
**	handled in various ways.
**
**  Inputs:
**	rac		Pointer to DMP_ROWACCESS info struct.
**	src		Pointer to compressed record.
**	dst_maxlen      Size of the destination area.  If we determine that
**			we are exceeding the destination area size, we abort
**			the operation and return an error.
**
**  Outputs:
**	dst		Pointer to an area to return the uncompressed record
**	dst_size	Size of the uncompressed record.
**
**  Returns:
**	E_DB_OK	        Tuple expanded
**	E_DB_ERROR	Tuple could not be expanded or size exceeded
**
**
**  History:
**	27_oct_94 (shero03)
**	    Created;
**      17-jul-1996 (ramra01)
**          Alter Table Project: For Uncompress with Add /Drop col
**          an intermediate buffer needs to be provided to expand/
**          extract valid columns. Avoid using stack for VPS.
**          Would be better to alloc space on the rcb to be used
**          as intermediate buffer with size of reltotwid.
**	13-sep-1996 (canor01)
**	    Use the temporary buffer passed with the rcb for uncompression
**	    when available.
**      10-mar-1997 (stial01)
**          Delete unused tuple buffer from stack
**          Pass record size to dm0m_tballoc()
**	15-jan-1999 (nanpr01)
**	    Pass ptr to ptr to dm0m_tbdealloc function.
**	12-aug-99 (stephenb)
**	    Alter parameters, buffer is now char** since it may be 
**	    freed by called function.
**	20-aug-99 (cucjo01)
**	    Fixed incorrect matchup of comment statement in order to
**	    fix build error.
**	30-Aug-1999 (jenjo02)
**	    Only deallocate "record" if allocated locally.
**      16-mar-2004 (gupsh01)
**          Modified dm1ch_uncompress to include adf control block 
**	    in parameter list.
*/

DB_STATUS
dm1ch_uncompress(
      DMP_ROWACCESS	*rac,
      char		*src_param,
      char		*dst_param,
      i4		dst_maxlen,
      i4		*dst_size,
      char		**record,
      i4		row_version, 
      ADF_CB		*rcb_adf_cb)		

{

	u_char         *pIn;
	u_char         *pOut;
	u_char         *pOutMaxAddr;
	i4            iOutSize = 0;
	DICTCONTROL    DictCtrl;
	DICTIONARY     Dict[TABLE_SIZE];
	BIT_FILE       BitFile;
	u_i2           NewCode = FLUSH_CODE;
	u_i2           OldCode;
	i4            Character;
	i4            Count;
	char	       *record2;
	char           DecodeStack[TABLE_SIZE];
	i4            rc = 0;
	DB_STATUS      status;				
	bool           local_recbuf = FALSE;

	pIn = (u_char*)src_param;
	pOut = (u_char*)dst_param;
	pOutMaxAddr = pOut + dst_maxlen;

	if (*pIn++ == (char)FALSE)
	{
    	/* The data is in its original form */
    	/* compute the source length, */
    	/* then copy the data to the output area */

    	    status = dm1r_cvt_row(rac->att_ptrs, rac->att_count, pIn, pOut,
				  dst_maxlen, dst_size, row_version, 
				  rcb_adf_cb);
	    return(status);
	}

	if (record && *record != NULL)
	    record2 = *record;
	else
	{
	    record2 = dm0m_tballoc(dst_maxlen);
	    if (record2 == NULL)
		return (E_DB_ERROR);
	    local_recbuf = TRUE;
	}

	pOut = (u_char*)record2;

	BitFile.pDataPtr  = pIn;
	BitFile.cResidual = 0;
	BitFile.Residual  = 0;

	while ( (NewCode == FLUSH_CODE) &&
	        (rc == 0) )
	{

	    dm1ch_InitDictionary(&Dict[0], &DictCtrl);
	    OldCode = (u_i2) dm1ch_InputBits(&BitFile, DictCtrl.CurrentCodeBits);
	    if (OldCode == END_OF_STREAM)
	    {
	        rc = 0;
	        break;
	    }

	    Character = OldCode;
	    *pOut++ = (char)OldCode;

	    for (; ;)
	    {

	    	NewCode = (u_i2) dm1ch_InputBits(&BitFile, DictCtrl.CurrentCodeBits);
	    	if (NewCode == END_OF_STREAM)
	    	{
		   rc = 0;
		   break;
	    	}

	    	if (NewCode == FLUSH_CODE)
		   break;

	    	if (NewCode == BUMP_CODE)
	    	{
		   DictCtrl.CurrentCodeBits++;
		   continue;
	    	}

	    	if (NewCode >= (u_i2)DictCtrl.NextCode)
	    	{
		   DecodeStack[0] = (char) Character;
		   Count = dm1ch_DecodeString(&Dict[0], DecodeStack, 1, OldCode);
	    	}
	    	else
	    	{
		   Count = dm1ch_DecodeString(&Dict[0], DecodeStack, 0, NewCode);
	    	}

	    	Character = DecodeStack[Count - 1];

	    	while (Count > 0)
	    	{
		   *pOut++ = DecodeStack[--Count];
	    	}

	    	Dict[DictCtrl.NextCode].ParentCode = OldCode;
	    	Dict[DictCtrl.NextCode].Character = (char) Character;
	    	DictCtrl.NextCode++;
	    	OldCode = NewCode;
	    }  /* Expand this dictionary set */

    }  /* Expand buffer */

    iOutSize = pOut - (u_char*)record2;

    status = dm1r_cvt_row(rac->att_ptrs, rac->att_count, record2, dst_param,
                          dst_maxlen, &iOutSize, row_version, 
			  rcb_adf_cb);
    if (status == E_DB_OK)
    {
       *dst_size = iOutSize;      
    }

    /* If "record2" allocated locally, deallocate it */
    if (local_recbuf)
	dm0m_tbdealloc( &record2 );

    return(status);

}  /* dm1ch_uncompress */

/*
** Name: dmpp_compexpand -- how much can a tuple expand when compressed?
**
** Description:
**	Though the obvious goal of compression is to shrink actual tuple
**	size, in all our compression algorithms there is potential growth
**	in the worst case.  Given the list of attributes for a relation,
**	this accessor returns the amount by which a tuple may expand in the
**	worst case.
**
**	The hidata compressor keeps track of the compressed row width,
**	and rejects the compression (uses the original row) if compression
**	fails to compress.  We need one leading indicator byte to
**	show whether a particular row was compressed or not, so the
**	worst-case expansion is that one byte.  The att array and count
**	are passed since this is called via a standard dispatch, but
**	they are not used.
**
** Inputs:
**	compression_type		The compression type code
**      atts_array                      Pointer to an array of attribute
**                                      descriptors.
**      atts_count                      Number of entries in atts_array.
**
**
** Outputs:
**	Returns:
**
**	              			Number of bytes the tuple may increase
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	27_oct_94 (shero03)
**	Created
**	9-Jul-2010 (kschendel) SIR 123450
**	    Compression type is now passed too.
*/

i4
dm1ch_compexpand(	i4		compression_type,
			DB_ATTS		**atts,
			i4		att_cnt)
{

	return 1;

}  /* dm1ch_compexpand */


/*
**
** dm1ch_InitDictionary
**
** This routine is used to initialize the dictionary, both when the
** routine first starts up, and also when a flush code comes in
**
*/
static void dm1ch_InitDictionary(
	DICTIONARY		*pDict,
	DICTCONTROL		*pDictCtrl)
{

	MEfill (sizeof(DICTIONARY) * TABLE_SIZE,
		0xff,
		(PTR)pDict);

	pDictCtrl->NextCode = FIRST_CODE;
	pDictCtrl->CurrentCodeBits = FIRSTBITS;
	pDictCtrl->NextBumpCode = FIRSTBUMPCODE;
	return;

}  /* dm1ch_InitDictionary */

/*
**
** dm1ch_FindChildNode
**
** This hashing routine is responsible for finding the table location for a
** string/character combination.  The table index is created by using an
** exclusive OR combination of the prefix and character.  This code also has
** to check for collisions, and handles them by jumping around in the table.
**
*/
static u_i2 dm1ch_FindChildNode(
	DICTIONARY     *pDict,
	u_i2           ParentCode,
	u_i2           ChildCharacter)
{
	u_i2           index;
	u_i2           offset;

	index = ((ChildCharacter << (BITS - BITSPERBYTE)) ^ ParentCode)
		& MAX_CODE;
	if (index == 0)
	    offset = 1;
	else
	    offset = (u_i2)TABLE_SIZE - index;

	for (; ;)
	{

	if (pDict[index].CodeValue == UNUSED)
	    return(index);

	if (pDict[index].ParentCode == ParentCode &&
	    pDict[index].Character == (char) ChildCharacter)

	return(index);

	if (index >= offset)
	    index -= offset;
	else
	    index += (int)TABLE_SIZE - offset;
	}
}  /* Find Child Node */

/*
** dm1ch_DecodeString
**
** This routine decodes a string from the dictionary, and stores it in the
** decode_stack data structure.  It returns a count to the calling program
** of how many characters were placed in the stack.
**
*/
static u_i2 dm1ch_DecodeString(
	DICTIONARY     *pDict,
	char *         pDecodeStack,
	u_i2           Count,
	u_i2           Code)
{

	while (Code > 255)
	{
	    pDecodeStack[Count++] = pDict[Code].Character;
	    Code = pDict[Code].ParentCode;
	}

	pDecodeStack[Count++] = (char) Code;
	return(Count);
}  /* dm1ch_Decode String */


/*
**  dm1ch_InputBits
**    Read a bit from a bit file.
**
*/
static u_i4 dm1ch_InputBits(
	BIT_FILE *     pBitFile,       /* BitFile */
	u_i2           BitCount)       /* Bits to read */

{
	u_i4           lReturnValue = 0L;

	while (pBitFile->cResidual <= 24)
	{
	     pBitFile->Residual |= (u_i4)*pBitFile->pDataPtr <<
		                   (24 - pBitFile->cResidual);
	     pBitFile->pDataPtr++;

	     pBitFile->cResidual += BITSPERBYTE;
	}

	lReturnValue = pBitFile->Residual >> (32 - BitCount);
	pBitFile->Residual <<= BitCount;
	pBitFile->cResidual -= BitCount;
	return(lReturnValue);

}  /* dm1ch_InputBits */

/*
**  dm1ch_OutputBits
**  Write multiple bits to a bit file that has been opened for output.
**
*/

static i4  dm1ch_OutputBits(

	BIT_FILE *     pBitFile,       /* BitFile */
	u_i4           lCode,          /* Code to Emit */
	u_i2           cbCode)         /* Number of bits to Emit */

{

	pBitFile->cResidual += cbCode;
	pBitFile->Residual |= lCode << (32 - pBitFile->cResidual);

	while (pBitFile->cResidual >= BITSPERBYTE)
	{

	    if (pBitFile->cbDataLen > 0)
	    {
		*pBitFile->pDataPtr = (u_char)(pBitFile->Residual >> 24);
		pBitFile->pDataPtr++;
		pBitFile->cbDataLen--;
	    }
        else
	{
	    return -1;
	}

	pBitFile->cResidual -= BITSPERBYTE;
	pBitFile->Residual <<= BITSPERBYTE;
    }

    return 0;
}  /* dm1ch_OutputBits */


/*
**  dm1ch_CloseBitFile
**  Write any residue to the open file
**
*/

static int dm1ch_CloseBitFile(

	BIT_FILE *     pBitFile)       /* BitFile */

{

	if (pBitFile->cResidual > 0)
	{

	    if (pBitFile->cbDataLen > 0)
	    {
		*pBitFile->pDataPtr = (u_char)(pBitFile->Residual >> 24);
		pBitFile->pDataPtr++;
		pBitFile->cbDataLen--;
	    }
	    else
	    {
		return -1;
	    }
	}

    return 0;
}  /* dm1ch_CloseBitFile */
