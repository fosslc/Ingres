/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <lc.h>
# include <cm.h>
# include <er.h>
# include <me.h>
# include <nm.h>
# include <lo.h>
# include <st.h>
# include <tr.h>
# include <ll.h>

/* 
** Name:	LC.c	- Locale specific functions. 
**
** Specification:
**	The locale specific functions help to find information about the
**	current locale. 
**
** Description:
**	Contains LC function 
**
** History:
**	06-May-2002 (gupsh01)
**	    initial creation.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**/


/*{
** Name:	LC_getStdLocale - gets the IANA equivalent of installation 
**				  characterset
**
** Description:
**	This function will return the IANA registered encoding for the Ingres 
**	character set value (II_CHARSETxx value). 
**	refer to the INANA website for more details.
**	http://www.iana.org/assignments/character-sets last updated 29 April 2002
**
**	Typical use of this function is to find the equivalent name of the 
**	character set to use in an xml file, to denote the encoding for 
**	ingres data.
**
** Inputs:
**	charset		optional. If this input parameter is provided, then 
**			this function returns the IANA equivalent 
**
** Outputs:
**	
** 	encoding	The pointer where the returned value will be placed.
**			Note this will return the source character set value
**			if no matching IANA charset found. 
**
** Returns:
**	OK		if succeeded
**	other		reportable error status.
**			E_LC_CHARSET_NOT_SET  : No characterset value is set 
**						 for installation.
**			E_LC_FORMAT_INCORRECT : Format of map file is incorrect.
**			E_LC_LOCALE_NOT_FOUND : No matching locale could be found. 
**
** History:
**	06-May-2002 (gupsh01)
**	    initial creation.
**	28-May-2002 (gupsh01)
**	    Fixed the comparison of the encoding names, to be done after we have
**	    processed the entry from the xmlnames.map file.
**	14-Jun-2004 (schka24)
**	    Ask CM for charset name, eliminate fooling with env vars.
*/

STATUS
LC_getStdLocale (charset, std_encoding, err_stat)
char *charset;
char *std_encoding;
CL_ERR_DESC	*err_stat;
{
  char	        source[GL_MAXNAME + CM_MAXATTRNAME + 1];  /* Guaranteed big enuf */
  STATUS	stat = OK;
  i4		i;
  LOCALE_MAP 	lmap;
  char          *separator;
  char          buf[256] = {0};
  i4            lno = 0;
  char          fname_buf[MAX_LOC];
  LOCATION      loc;
  FILE          *fp;
  i4            count_param = 0;
  char          *recptr;
  char          filename[] = "xmlname.map";

  if (err_stat)
    CL_CLEAR_ERR( err_stat);

  /* 
  ** If no charset is provided then find out 
  ** the installation character set 
  */
  if (charset == NULL)
  {
     CMget_charset_name(&source[0]);

     if (source[0] == '\0')
     {
         /* 
	 ** Give out error "cannot determine the 
	 ** installation character set 
	 */
         stat = E_LC_CHARSET_NOT_SET;
         err_stat->intern = 0;
         err_stat->errnum = E_LC_CHARSET_NOT_SET;
         return stat;
     }
  }
  else 
    STcopy (charset, source);

  STtrmwhite(source);

  /* Now we read in the xmlnames file for LOCALE_MAP information */
  NMloc( FILES, FILENAME, filename, &loc);
  STcopy (filename, fname_buf);
  LOcopy( &loc, fname_buf, &loc );

  if (SIopen(&loc, "r", &fp) != OK)
     return FAIL;

   while (SIgetrec(buf, sizeof(buf), fp) != ENDFILE)
   {
     lno++;
     count_param = 0;
  
      /* comment line */
      if (buf[0] == '#') 
	continue;
	
      /* blank line */
      (VOID)STtrmwhite(buf);
      if (STcompare(buf, "") == 0) 
	continue;

      recptr = buf;

      /* This file may contain three entries 
      ** 
      ** INGRES CHARACTER SET NAME , STD IANA charset name , # comment 
      */

      while ((separator = STindex(recptr, "\t", 0)) != NULL )
      {
         count_param++;
         (VOID)STtrmwhite(recptr);

         switch (count_param)
         {
           case 1: 
             STlcopy (recptr, lmap.ingloc, separator - recptr);
             break;

	   case 2:
             STlcopy (recptr, lmap.stdloc, separator - recptr);
             break;

           case 3:
             if (*recptr != '#')
	    {
	      /* This is a tab but not inside a comment so give out an error */
               err_stat->intern = 0;      
               err_stat->errnum = E_LC_FORMAT_INCORRECT ;   	
               err_stat->moreinfo[0].size = sizeof(i4);
               err_stat->moreinfo[0].data._i4 = lno;
               return FAIL;
             }	
             break;
           default:
	     err_stat->intern = 0;	
	     err_stat->errnum = E_LC_FORMAT_INCORRECT ;	
	     err_stat->moreinfo[0].size = sizeof(i4);
             err_stat->moreinfo[0].data._i4 = lno;
             return FAIL;
         }
         recptr += separator - recptr + 1;
         separator = 0;
    }	
       
    /* check if we have found the name for the standard 
    ** character set from map file 
    */
    if ( STbcompare(source, 0, lmap.ingloc, 0, 1)  == 0)
    {
        STcopy(lmap.stdloc, std_encoding);
        return stat;
    }
  }

  /* report error that no matching encoding value is found */ 
  stat = E_LC_LOCALE_NOT_FOUND;
  if (err_stat)
  {
    err_stat->intern = 0;
    err_stat->errnum = E_LC_LOCALE_NOT_FOUND;
    STcopy(source, std_encoding);
  }

 return stat;
}

