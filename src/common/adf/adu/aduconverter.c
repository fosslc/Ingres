/*
** Copyright (c) 2004 Ingres Corporation
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
#include    <aduucoerce.h>

/*
** Name: ADUUCNVTR.C - Convertor mapping file related functions for  
**		       for Unicode coercion support.
**
** Decsription:
**      Contains unicode coersion support functions for identifying the 
**      mapping file to user for conversion.
**
** Following functions are supported:
**	
**	adu_getconverter - Obtains the name of the converter file to use
**			   for unicode coersion.
**
**	adu_csnormalize - normalizes a string which represents charset
**			   to a normalized form as dictated by unicode 
**			   technical report UTF22.
** History:
**      22-Jan-2004 (gupsh01)
**          Created.
**	29-Jan-2004 (gupsh01)
**	    Fixed the spelling of converter in symbol table.
*/

/* Name: adu_getconverter - Obtains the name of converter mapping file
**			    to use for unicode coercion. 
** Description:
**
** To obtain the mapping file to be used for carrying out unicode-local
** character conversion. The following mechanism is followed:
**
** 1. Check symbol table for user defined converter setting
**    II_UNICODE_CONVERTER. If set then return this setting 
** 2. If the variable is not set then 
**	2.a Get the platform character set
**	2.b Read the aliasmaptbl file. 
** 	2.c Search the alias file for platform charset.
** 3. If still not found then find the II_CHARSETxx value
**    for ingres installation and search the alias file for 
**    this value. 
** 4. If none of these attempts succeed then return default 
**    with a warning to the errorlog if this happens 
** 
** Input:
**	converter -	Place holder for the output string, 
**			It is assumed that the area is at least MAX_LOC
**			chars in size.
** Output:
**	converter -	Pointer to string where the output
**			converter name is stored.
** History:
**
**	22-jan-2004 (gupsh01)
**	    Added.
**	14-Jun-2004 (schka24)
**	    Safe charset name handling.
*/
STATUS
adu_getconverter(
    char	*converter)
{
    STATUS		stat;
    char		*tptr;
    char		*env = 0;
    char		chset[CM_MAXATTRNAME+1];
    char		pcs[CM_MAXLOCALE+1]; /* platform character set */
    char		norm_pcs[CM_MAXLOCALE+1];
    CL_ERR_DESC         syserr;
    char                *alias_buffer = NULL;
    char                *bufptr = NULL;
    char                *buf = NULL;
    ADU_ALIAS_MAPPING   *aliasmapping;
    ADU_ALIAS_DATA	*aliasdata;	
    char 		*datasize;
    SIZE_TYPE		filesize = 0;
    SIZE_TYPE		sizemap = 0;
    SIZE_TYPE		sizedata = 0;
    i4			bytes_read;
    char		*abufptr;
    i4			i = 0;
    i4			index = 0;


/* STEP 1 */
    NMgtAt(ERx("II_UNICODE_CONVERTER"), &env);
    if (env && *env)
    {
        STlcopy(env, converter, MAX_LOC-1);
	return OK;
    }

/* STEP 2 */
    stat = CM_getcharset(pcs);
    
    if (CMopen_col("aliasmaptbl", &syserr, CM_UCHARMAPS_LOC) != OK)
    {
      /* return an ERROR if we are unable to open the file */
      return FAIL;
    }

    /* initialize buf to help read from the aliasmaptbl file */
    buf = MEreqmem(0, COL_BLOCK, TRUE, &stat);
    if (buf == NULL || stat != OK)
    {
       CMclose_col(&syserr, CM_UCHARMAPS_LOC);
       return (FAIL);
    }

    /* First file buffer has size information. */
    stat = CMread_col(buf, &syserr);
    if (stat != OK)
    {
        MEfree((char *)buf);
        CMclose_col(&syserr, CM_UCHARMAPS_LOC);
       return (FAIL);
    }

    tptr = buf;
    bytes_read = COL_BLOCK;

    /* filesize is the first entry of the map file */
    filesize = *(SIZE_TYPE *) buf;
    tptr += sizeof(SIZE_TYPE);
    tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));

    /* allocate working space for the data */
    alias_buffer = (char *)MEreqmem(0, filesize, TRUE, &stat);
    if (alias_buffer == NULL || stat != OK)
    {
       CMclose_col(&syserr, CM_UCHARMAPS_LOC);
       return (FAIL);
    }
    abufptr = alias_buffer;
    MEcopy (buf, COL_BLOCK, abufptr);
    abufptr += COL_BLOCK;

    /* Read the file till it is read completely */
    for ( ;bytes_read < filesize;)
    {
        stat = CMread_col(buf, &syserr);
        if (stat != OK)
        {
          MEfree((char *)buf);
          MEfree((char *)alias_buffer);
          CMclose_col(&syserr, CM_UCHARMAPS_LOC);
          return (FAIL);
	}
	bytes_read += COL_BLOCK;
        MEcopy (buf, COL_BLOCK, abufptr);
        abufptr += COL_BLOCK;
    }

    if (bytes_read < filesize)
    {
      /* we had to exit for some unknown reason */
      MEfree((char *)buf);
      MEfree((char *)alias_buffer);
      CMclose_col(&syserr, CM_UCHARMAPS_LOC);
      return (FAIL);
    }
    tptr = alias_buffer;
    tptr += sizeof(SIZE_TYPE);
    tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));

    /* Read the size of the MappingArray nodes */
    sizemap = *(SIZE_TYPE *) tptr;
    tptr += sizeof(SIZE_TYPE);
    tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));

    /* initialize buffer for ADU_ALIAS_MAPPING buffer */
    aliasmapping = (ADU_ALIAS_MAPPING *) MEreqmem(0, sizemap, TRUE, &stat);
    if (aliasmapping == NULL)
    {
      MEfree((char *)buf);
      MEfree((char *)alias_buffer);
      CMclose_col(&syserr, CM_UCHARMAPS_LOC);
      return (FAIL);
    }

    /* Copy data for ADU_ALIAS_MAPPING array */
    MEcopy(tptr, sizemap, aliasmapping);
    tptr += sizemap;
    tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));

    /* Get size for the aliasdata */
    sizedata = *(SIZE_TYPE *) tptr;
    tptr += sizeof(SIZE_TYPE);
    tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));

     /* Initialize buffer for ADU_ALIAS_MAPPING buffer */
    aliasdata = (ADU_ALIAS_DATA *) MEreqmem(0, sizedata, TRUE, &stat);
    if (aliasdata == NULL)
    {
      MEfree((char *)buf);
      MEfree((char *)alias_buffer);
      MEfree((char *)aliasmapping);
      CMclose_col(&syserr, CM_UCHARMAPS_LOC);
      return (FAIL);
    }

    /* Copy the ADU_ALIAS_DATA array */
    MEcopy(tptr, sizedata, aliasdata);
    tptr += sizedata;
    tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));

    /* Close the "aliasmaptbl" file */
    CMclose_col(&syserr, CM_UCHARMAPS_LOC);

    /* Normalize pcs */

    adu_csnormalize (pcs, STlength(pcs), norm_pcs);

    /* Retrieve the pcs value */
    for (i=0; i < sizedata/sizeof(ADU_ALIAS_DATA); i++)
    {
      if ((STcompare (aliasdata[i].aliasNameNorm, norm_pcs)) == 0)
      {
	index = aliasdata[i].aliasMapId; 
	/* found */
	STcopy (aliasmapping[index].mapping_id, 
		  converter);

        /* cleanup */
        MEfree((char *)buf);
        MEfree((char *)alias_buffer);
        MEfree((char *)aliasmapping);
        MEfree((char *)aliasdata);

        return (OK);
      } 
    }

/* STEP 3 */
    /* Obtain Ingres characterset */
    STcopy("default", converter);
    CMget_charset_name(&chset[0]);
    if (STcasecmp(chset, "UTF8") != 0)
    { 
	    /* search maptable for env  */ 
	for (i=0; i < sizedata/sizeof(ADU_ALIAS_DATA); i++)
	{
	    if ((STcompare (aliasdata[i].aliasNameNorm, norm_pcs)) == 0)
	    {
		index = aliasdata[i].aliasMapId; 
		/* found */
		STcopy (aliasmapping[index].mapping_id, converter);
		break;
	    }
	}
    }

    /* cleanup */
    MEfree((char *)buf);
    MEfree((char *)alias_buffer);
    MEfree((char *)aliasmapping);
    MEfree((char *)aliasdata);

    /* FIXME warning or error if still "default" ? */

    return (OK);
}

/* Name: adu_csnormalize - Normalizes a platform characterset 
**				 string in the alias table or as obtained
**				 by CM_getcharset.
** Description:
**
**	       As per the Unicode technical report UTR22. Ref: 
**	http://www.unicode.org/reports/tr22/ section 1.4, recommended 
**	method to compare the alias values in an alias table to a character 
**	set is to normalize the string in following manner.
**
** 	1. Delete all characters except a-z, A-Z, and 0-9. 
**	2. Map uppercase A-Z to the corresponding lowercase a-z. 
**	3. From left to right, delete each 0 that is not preceded by a digit.
**
** 	For example, the following names should match: "UTF-8", "utf8",
** 	"u.t.f-008", but not "utf-80" or "ut8".
**
** Input:
**      instring -     Unnormalized converter name string.
**
** Output:
**      instring -     Normalized converter name string. 
**
** History:
**      23-Jan-2004 (gupsh01)
**          Added.
*/
void
adu_csnormalize(
    char        *instring,
    i4          inlength,
    char        *outstring)
{
    char *iptr = instring;
    char *endinput = instring + inlength;
    char *optr = outstring;
    char nopreceding = '0';

    while (iptr < endinput)
    {
      if (CMdigit(iptr) || CMalpha(iptr))
      {
        if (CMalpha(iptr))
          nopreceding = *iptr;
        else
          nopreceding = '0';

        CMtolower(iptr,iptr);

        if (nopreceding != '0'&& *iptr == '0')
        {
          *iptr++;
          continue;
        }

        CMcpyinc(iptr, optr);
      }
      else
        iptr++;
    }
    *optr = '\0';
}
