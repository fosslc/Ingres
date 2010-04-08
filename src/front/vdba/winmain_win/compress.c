/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : CA/OpenIngres Visual DBA
**
**    Source : compress.c
**    data compression for .cfg files
** 9-Jul-2001 (hanje04)
**	Exclude hdr file if NOCOMPRESS is defined.
** 14-Feb-2002 (noifr01)
**  (sir 107121) now use code from the backend (dm1ccmph.c)
********************************************************************/
#include    <compat.h>
#include    <me.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/locking.h>
#include <share.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>


#include "dba.h"
#include "dbafile.h"
#include "compress.h"

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


//GLOBALREF DMPP_ACC_TLV dm1ch_tlv;


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
**	atts_array	Pointer to a list of pointers to attr descriptions.
**	atts_count	Number of entries in atts_array.
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
*/

BOOL
mycompress(
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
		unsigned long lstringlen = rec_size;
	    pOut = (u_char*)crec;
	    *pOut++ = (char)FALSE;
		// write lenght
		memcpy(pOut,(void *)&lstringlen,sizeof(unsigned long));
		pOut+=sizeof(long);

	    /* write original data */
	    MEcopy(rec, /*(u_i2)*/rec_size, pOut);
	    iOutSize = rec_size + 1 + sizeof(unsigned long);
	}

	*crec_size = iOutSize;      /* Return the size of compressed data */
	return TRUE;

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
**	atts_array	Pointer to a list of pointers to attr descriptions.
**	atts_count	Number of entries in atts_array.
**	src		Pointer to compressed record.
**	dst_maxlen      Size of the destination area.  If we determine that
**			we are exceeding the destination area size, we abort
**			the operation and return an error.
**	record_type	unused.
**	tid		unused.
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
*/

BOOL
myuncompress(
      char		*src_param,
      i4		dst_maxlen,
      i4		*dst_size,
      char		*record)		

{

	u_char         *pIn;
	u_char         *pOut;
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
	bool           local_recbuf = FALSE;

	pIn = (u_char*)src_param;

	if (*pIn++ == (char)FALSE)
	{
    	/* The data is in its original form */
		unsigned long llen;
		memcpy((char*)&llen,pIn,sizeof(long));
		if ((i4)llen>dst_maxlen)
			return FALSE;
		pIn += sizeof(long);
		memcpy(record, pIn, llen);
	    *dst_size = llen;
		return TRUE;
	}

	if (record)
	    record2 = record;
	else
	{
	    record2 = malloc(dst_maxlen);
		    if (record2 == NULL)
		return  FALSE;
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

    *dst_size = pOut - (u_char*)record2;

    //status = dm1r_cvt_row(atts_array, atts_count, record2, dst_param,
    //                      dst_maxlen, &iOutSize, row_version);

    /* If "record2" allocated locally, deallocate it */
    if (local_recbuf)
	free( record2 );

    return(TRUE);

}  /* dm1ch_uncompress */

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


void CompressFree(void)
{
}


#define BLOCKSIZE 2048
char CompBuf[2 * BLOCKSIZE];
int posinbuf4write = 0;
char resbuf[14 * BLOCKSIZE]; /* overdimensioned. saved blocks have normally a max size of */
							 /* BLOCKSIZE once restored */
int posinbuf4read = 0;
int availableinReadBuf = 0;

int Flush4Read  ( FILEIDENT fident)
{
	i4 rec_size;
	UINT uRead;
	uRead = fread(&rec_size, 1, sizeof(rec_size),fident);
	if (uRead !=sizeof(rec_size))
		return RES_ERR;
	if (rec_size >2*BLOCKSIZE)
		return RES_ERR;
	uRead = fread(CompBuf, 1, rec_size,fident);
	if (uRead != (UINT)rec_size)
		return RES_ERR;
	if (!myuncompress(CompBuf, sizeof (resbuf),&rec_size, resbuf))
		return RES_ERR;
	availableinReadBuf = (int) rec_size;
	posinbuf4read = 0;
	return RES_SUCCESS;
}

int Flush4Write ( FILEIDENT fident)
{
	i4 rec_size;
	UINT uWrite;
	if (! mycompress( CompBuf, (i4)  posinbuf4write, resbuf, &rec_size))
		return RES_ERR;
	uWrite = fwrite(&rec_size, 1, sizeof(rec_size),fident);
	if (uWrite !=sizeof(rec_size))
		return RES_ERR;
	uWrite = fwrite(resbuf, 1, rec_size,fident);
	if (uWrite < (UINT)rec_size)
		return RES_ERR;
	posinbuf4write = 0;
	return RES_SUCCESS;
}

int DBACompress4Save(FILEIDENT fident, void * buf, UINT cb)
{
	int ires;
	while (cb > 0) {
		UINT nremaininblock = BLOCKSIZE - posinbuf4write;
	    if ( cb > nremaininblock ) {
			char * ptmp = buf;
			memcpy(CompBuf + posinbuf4write, buf, nremaininblock);
			ptmp += nremaininblock;
			buf=ptmp;
			cb -= nremaininblock;
			posinbuf4write = BLOCKSIZE;
			ires = Flush4Write(fident);
			if (ires !=RES_SUCCESS)
				return ires;
		}
		else {
			memcpy(CompBuf + posinbuf4write, buf, cb);
			posinbuf4write += cb;
			return RES_SUCCESS;
		}
	}
	return RES_SUCCESS;
}

int DBAReadCompressedData(FILEIDENT fident, void * bufresu, size_t cb)
{
	int ires;
	while (cb > 0) {

		int remaininbuffer = availableinReadBuf - posinbuf4read;
		
		if (remaininbuffer == 0) {
			ires = Flush4Read(fident);
			if (ires !=RES_SUCCESS)
				return ires;
			continue;
		}

		if (cb > (size_t)remaininbuffer) {
			char * ptmp = bufresu;
			memcpy(bufresu, resbuf + posinbuf4read, remaininbuffer);
			posinbuf4read += remaininbuffer;
			ptmp += remaininbuffer;
			bufresu=ptmp;
			cb -= remaininbuffer;
			ires = Flush4Read(fident);
			if (ires !=RES_SUCCESS)
				return ires;
		}
		else {
			memcpy(bufresu, resbuf + posinbuf4read, cb);
			posinbuf4read += cb;
			return RES_SUCCESS;
		}
	}
	return RES_SUCCESS;
}

BOOL CompressFlush (FILEIDENT fident)
{
	if (availableinReadBuf) {
		availableinReadBuf = 0;
		posinbuf4read = 0;
	}
	
	if (posinbuf4write) {
		int ires = Flush4Write ( fident);
		if (ires !=RES_SUCCESS)
			return FALSE;
	}
	return RES_SUCCESS;

}



