/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <cm.h>
#include <er.h>
#include <si.h>
#include <cv.h>
#include <pc.h>
#include <lo.h>
#include <st.h>
#include <me.h>
#include <sl.h>
#include <nm.h>
#include <pc.h>
#include <iicommon.h>
#include <adf.h>
#include <ulf.h>
#include <adulcol.h>
#include <adfint.h>
#include <aduucol.h>
#include <aduucoerce.h>

/**
**
**  Name: ADUUMAPLKUP.C - Mapping file lookup routines.
**
**  Description:
**	This file contains routines to lookup unicode mapping tables
**	for coercion, and validation of a byte for a given codepage.
**	
**	This file defines the following routines:
**
**	adu_map_check_validity - checks the validity of a byte sequence against 
** 	adu_map_append_validity - adds the validity values to validity table.
** 	adu_map_init_charmap - Initializes a charmap array.
** 	adu_map_add_charmap - adds the charmap values to character mapping table.
** 	adu_map_get_chartouni - gets the unicode for a local character 
** 	adu_map_delete_charmap - cleans up a charmap table. 
** 	adu_map_init_unimap - Initializes a unimap array.
** 	adu_map_add_unimap - adds unimap values to unicode mapping table.
** 	adu_map_get_unitochar - gets the local character value from unicode 
** 	adu_map_delete_unimap - cleans up the unicode mapping table.
**	adu_map_delete_vldtbl - cleans up the memory for validity table. 
** 	adu_readmap - Reads and processes a coercion map table.  
** 	adu_initmap - Initializes the server map table in memory
** 	adu_deletemap - Cleans up the memory allocated for mapping tables. 
**
**  History:
**	04-Jan-2004 (gupsh01)
**	    Created.
**  16-Mar-2005 (fanra01)
**      Bug 114095
**      Scanning for the first non-zero lead value within the components of
**      the input value is limited to the maximum number of indexing
**      components of the Trie data structure.
**	02-Dec-2010 (gupsh01) SIR 124685
**	    Protype cleanup.
*/

/*
** Trie based Unicode lookup Design.
** 
** Mapping table is based on Trie Data Structure, to enable fast data look up 
** with optimum memory usage. The organization of the table is as follows:
**
** Design: Local characterset to Unicode Lookup
** --------------------------------------------
**
**	|----------Byte 1------------>|
**                                    |	
**      ------------------------------V----------------------------------------
** Base	|**	|	|	|**	|	|**u_i2|	|	|     |	
**       -------|------------------------------------|-------------------------
** 		|init   			     | 
** 	--<----         			     |  Assigned
** 	|	------------<-------------------------
** 	|	|						
** 	|	V	-------------------------------------------------
** 	|	BYTE2	|*u_i2	|	|	|	|*u_i2|	|	|
** 	|		----------------------|--------------------------
** 	|			Initial	      |          |	      | Assigned
** 	|    -----------------<---------------           |            v 
** 	|    |--------------<----------------------------             |
** 	|    |     -------------------------<-------------------------     
**      |    |    |     		Unassigned : probably illegal/missing 
** 	|    |    |                   byte sequence to this code page
** 	|    |	  V    -------------------------------------------v----------
** 	|    |  BYTE3	|unicode|	|unicode|unicode|0xfffd|unicode|    |	
** 	|    |		-----------------------------------------------------
** 	|    V		Substitution array
** 	|           -----------------------------------------------------------
** 	|   Default |0xfffd	|0xfffd	|0xfffd	|	|	|      |0xfffd|
** 	|	 ^  -----------------------------------------------------------
** 	|	 |				        
** 	|	 |------------<-----<---<----<-----------------<-------
** 	|	 |--------<-----    |   |   |                         | 
** 	v		 ---|---|---|---|---|------------------------------
** 	Default 2D	|*u_i2	|   |	|   |*u_i2|	|     |*u_i2|   | | 
**		        ---------------------------------------------------
** 
** This design ensures minimum test for fast lookup. The main tables are:
** Base		: An array of 256 pointers to two Dimensional Tables of u_i2's
**		  of size [256 x 256] for each byte.
** Default 2D	: An array of 256 pointers to u_i2's these pointers all 
**		  point to Default table.
** Default	: An array of u_i2's which all holds value 0xfffd. 
** BYTE2	: An arry of u_i2 pointers all elements of which point to the 
**		  Default array.
** BYTE3	: An arry of u_i2 values initialized as 0xfffd. 
**
** 1) Base points to Default2D or if a 2nd byte is known then to BYTE2.
** 2) When a 3rd Byte is known then the Byte2 cell value 
**    ("Pointer to Byte2 Array" + "2nd Byte value") will point to BYTE3 
**    instead of Default. 
** 3) The 3rd byte read determines the index in byte3 array which is assigned 
**    the unicode value. The unicode value for the third byte thus can be found 
**    at:  ("Pointer to Byte3 Array" + "3rd Byte value").
**	
** Design: Unicode to Local characterset Lookup
**		
**	     |------1st Unicode Byte ----------->|
**                                               |	
** 	     ------------------------------------V-----------------------
** UniBase   |*u_i2[3]|		|	 |	  |	  |	  |	  |
**	     ------------|---------|---------------------|----------------
**           		 |         |			 |assigned.
**  -----<-------------- |         |	                 |
** |	------------<---------------			 |
** |	|	      					 |
** |	|	      |-----------2nd Unicode Byte ----->|
** |	|             -----------------------------------v--------------
** |	|	UNI2  |subchar|   | 	|	|	|	|	|	
** |	|	      --------------------------------------------------
** v	v    -------------------------------------------------------------
** Default   |subchar|subchar|subchar|		|	|	|subchar  |
**	      -------------------------------------------------------------
**
** Handling Double Substitution:
** ----------------------------
** When converting from Unicode to a multibyte codepage an unassigned code 
** point is found and if a "subchar1" mapping is defined for the code point
** output the "subchar1" byte sequence otherwise output the regular 
** substitution character (subchar).
** ie:-  for all unicode code points for which subchar1 is declared set 
**     UniBase[byte1][byte2] = subchar1
**
** When converting from a multibyte codepage to Unicode if unassigned code
** point is found, then if the input sequence is of length 1 and a "subchar1" 
** value is specified for the codepage, output U+001a otherwise output U+fffd 
**  ie:- initialize Base[0][0][i] = subchar1
*/
/*
** Design: Validity Table - validates if a byte string belongs to the characterset 
**
**	Validity table is specified in form of a Statetable which has list of states.
**	Each state has 256 values.
**
**	name		 -----------------------------------------------
**	state 0		|	|	|P/X|	|	|	|	|
**	nextstate *	 ----------------------|------------------------
**		|			       |	
**		v    /--------------------------
**	name	    /	 -----------------------------------------------
**	state 1	<--/	|	|	|	|	|	|	|
**	nextstate *	 -----------------------------------------------
**		.
**		.
**		.
*/

/* Static functions declarations */

static DB_STATUS
adu_map_append_validity(
    ADU_MAP_STATETABLE  **vtabptr,
    ADU_MAP_VALIDITY    *validity);

static DB_STATUS
adu_map_init_charmap(
    u_i4        ****Base,
    u_i4        ***Default2D,
    u_i4        **Default);

static void
adu_map_add_charmap(
    u_i4        ***Base,
    u_i4        **Default2D,
    u_i4        *Default,
    u_i4        bval,
    u_i2        unicode,
    u_i2        uhsur);

static void
adu_map_delete_charmap(
    u_i4        ****Base,
    u_i4        ***Default2D,
    u_i4        **Default);

static DB_STATUS
adu_initmap(
    ADU_MAP_HEADER      *header,
    ADU_MAP_VALIDITY    *validities,
    ADU_MAP_ASSIGNMENT  *assignments
);

static void
adu_map_delete_vldtbl (
    ADU_MAP_STATETABLE *validity_table);

static DB_STATUS
adu_map_init_unimap(
    u_i2 	***UniBase, 
    u_i2 	**Unidefault, 
    u_i2 	subchar);

static void 
adu_map_add_unimap (
    u_i2 	**UniBase, 
    u_i2 	*Unidefault, 
    u_i2 	local, 
    u_i2 	unicode, 
    u_i2 	uhsur,
    u_i2 	subchar);

static void 
adu_map_delete_unimap(
    u_i2	***UniBase, 
    u_i2	**Unidefault);

static DB_STATUS
adu_initmap(
    ADU_MAP_HEADER	*header,
    ADU_MAP_VALIDITY	*validities,
    ADU_MAP_ASSIGNMENT	*assignments);

static void 
adu_map_delete_vldtbl (
    ADU_MAP_STATETABLE *validity_table);


/* Name: adu_map_check_validity - checks the validity of a byte sequence against 
** 				  the validity table.
** Description:
**	This routines, checks a byte value against the validity table to identify
**	if the byte value is valid for this code page. Validity data is present 
**	in form of a statetable in the Mapping files.
**
** Input:
**		validitytable	pointer to validity table.
**		bval		pointer to byte values.
** Output:
**	 	E_DB_OK		byte is valid	
**
**	 	E_DB_ERROR	byte is invalid
**
**	FIXME:
**	There are several states which can be identified 
**	from the byte sequence, ie, whether it is 
**	unassigned, incomplete, ambiguous or illegal. 
**	Fix this routine to return appropriate error for 
**	each case.
**
** History:
** 	23-Jan-2004 (gupsh01)
**	    Added.
**	04-Feb-2004 (gupsh01)
**	    Modified strcmp to STbcompare.
**      16-Apr-2004 (loera01) Bug 112162 
**          In adu_deletemap, removed redundant MEfree of 
**          Adf_globs->Adu_maptbl.  
**  16-Mar-2005 (fanra01)
**      Ensure that the scan through the array inbyte is limited to the bounds
**      of the array.
*/
DB_STATUS 
adu_map_check_validity(
    ADU_MAP_STATETABLE 	*validitytable, 
    u_i4  		*bval)
{
    int 		i = 0;
    u_i2 		inbyte[MAXBYTES]; 
    ADU_MAP_VLDSTATE 	**currstate;
    ADU_MAP_VLDSTATE 	**nextstate;
    u_i2		property = 0;
    u_i2 		current_byte; 
    ADU_MAP_STATETABLE 	*ptr;

    if ((validitytable == NULL) || (bval == NULL))
      return (E_DB_ERROR); 

    /* search validity table for first state */
    for (ptr=validitytable; ptr != NULL; ptr = ptr->next)
    {
      if (STbcompare (ptr->name, 0, "FIRST", 0, 1) == 0)
        break;
    }

    inbyte[0] = (*bval & 0xFFFFFF) >> 16;
    inbyte[1] = (*bval & 0xFFFF) >> 8; 
    inbyte[2] = (*bval & 0xFF);

    current_byte = inbyte[i++];

    if (!(validitytable && ptr))
    {
      /* ERROR No validity table found */
      return (E_DB_ERROR);
    }
    else
    {
      /* Get to a non zero leading byte */
      while ((current_byte == 0) && (i < MAXBYTES))
        current_byte = inbyte[i++];

      currstate = ptr->state_table;
      property = ST_UNASSIGNED;
      nextstate = NULL;

      if (currstate[current_byte])
      {
        property = currstate[current_byte]->property;	// initialize property
        nextstate = currstate[current_byte]->next;	//initialize next state

        while(nextstate && i<MAXBYTES)
        {
    	current_byte = inbyte[i++];
    	currstate = nextstate;	
    	if (currstate[current_byte])
    	{
    	  nextstate = currstate[current_byte]->next;
    	  property = currstate[current_byte]->property;
    	}
    	else 
    	{
    	  property = ST_UNASSIGNED;
    	  nextstate = NULL;
    	  break;
    	}
        }
      }
      if (property & ST_VALID)
	return (E_DB_OK);	
      else 
        return (E_DB_ERROR);
    }
}

/* Name: adu_map_append_validity - adds the validity values to validity table.
** Description:
**      This routine, appends a validity information into the validity table.
** Input:
**              vtabptr 	pointer to validity table.
**              validity 	pointer to vaidity value to add.
** Output:
**
**      None.
**
** History:
**      23-Jan-2004 (gupsh01)
**          Added.
**      04-Feb-2004 (gupsh01)
**	    Modified strcmp to STbcompare.
*/
static DB_STATUS
adu_map_append_validity(
    ADU_MAP_STATETABLE 	**vtabptr, 
    ADU_MAP_VALIDITY 	*validity)
{
    /* default values */
    ADU_MAP_STATETABLE 	*validitytable = *vtabptr;
    ADU_MAP_STATETABLE 	*tmp_begin = NULL;
    ADU_MAP_STATETABLE 	*tmp_end = NULL;			
    ADU_MAP_STATETABLE 	*ptr = NULL;
    u_i2 		property = ST_UNASSIGNED; 
    int 		i = 0;

    /* FIXME: if *validity is NULL return error */
    if (validity == NULL) 
      return (E_DB_ERROR); /* EMPTY_VALIDITY_DATA being added */

    /* Based on the end value set next state. */
    if (STbcompare (validity->state_next, 0, "VALID", 0, 1) == 0)
      property = ST_VALID;		  
    else if (STbcompare (validity->state_next, 0,  "INVALID", 0, 1) ==0)
      property = ST_INVALID;
    else if (STbcompare (validity->state_next, 0, "UNASSIGNED", 0, 1) ==0)
      property = ST_EXP_UNASSIGNED;
    else
    {
      /* Updating the end pointers */
      if (validitytable) 
      {
        for (ptr=validitytable; ptr != NULL; ptr = ptr->next)
        {
          if (STbcompare (ptr->name, 0, validity->state_next, 0, 1) == 0)
            break;			
        }
      }

      if (ptr == NULL)
      {  
        tmp_end = (ADU_MAP_STATETABLE *)MEreqmem(0, 
			sizeof(ADU_MAP_STATETABLE), FALSE, NULL);

        if (validity) 
        {
          STcopy(validity->state_next, tmp_end->name); 
          tmp_end->next = NULL;
    	  tmp_end->state_table = (ADU_MAP_VLDSTATE **)MEreqmem(0, 
			sizeof(ADU_MAP_VLDSTATE *) * 256, FALSE, NULL);
          for (i=0;i<256;i++)
            tmp_end->state_table[i] = NULL;
        }	

        /* add to the list */
        if (validitytable == NULL)
          *vtabptr = validitytable = tmp_end;
        else
        {
          tmp_end->next = validitytable->next;
          validitytable->next=tmp_end;
        }
      }
      else 
        tmp_end = ptr;
    }
    
    /* Updating the start pointers */
    for (ptr=validitytable; ptr != NULL; ptr = ptr->next)
    {
    	if (STbcompare (ptr->name, 0, validity->state_type, 0, 1) == 0)
    	break;
    }
    
    if (ptr == NULL)	
    {
       /* NOT FOUND */
    	tmp_begin = (ADU_MAP_STATETABLE *)MEreqmem(0, 
			sizeof(ADU_MAP_STATETABLE), FALSE, NULL);
    	ptr = tmp_begin;
    	if (validity)
    	{
    	  STcopy(validity->state_type, tmp_begin->name); 
    	  tmp_begin->next = NULL;
    	  tmp_begin->state_table = (ADU_MAP_VLDSTATE **)MEreqmem(0, 
			sizeof(ADU_MAP_VLDSTATE *) * 256, FALSE, NULL);
              for (i=0;i<256;i++)
                tmp_begin->state_table[i] = NULL;
    	}
    	/* add this to the validation list */
    	if (validitytable == NULL)
    	{
    	  *vtabptr = validitytable = tmp_begin;
    	}
    	else
    	{
    	  tmp_begin->next = validitytable->next;
    	  validitytable->next=tmp_begin;
    	}
    }

    for (i=validity->state_start; i <= validity->state_end; i++)
    {
    	ptr->state_table[i] = (ADU_MAP_VLDSTATE *)MEreqmem(0, 
			sizeof(ADU_MAP_VLDSTATE), FALSE, NULL);
    	(ptr->state_table[i])->property = property;

    	if (tmp_end)
    	  (ptr->state_table[i])->next = tmp_end->state_table;
    	else 
    	  (ptr->state_table[i])->next = NULL;
    }

    return (E_DB_OK); 
}

/* Name: adu_map_init_charmap - Initializes a charmap array.
** Description:
**      This routine, initializes a charmap array which holds information to 
**	convert from local characterset to unicode. 
**
** Input:
**              Base            pointer to charmap table.
**              Default2D       pointer to 2D default array.
**              Default         pointer to default array.
** Output:
**
**      None.
**
** History:
**      23-Jan-2004 (gupsh01)
**          Added.
*/
static DB_STATUS 
adu_map_init_charmap(
    u_i4 	****Base, 
    u_i4	***Default2D, 
    u_i4 	**Default)
{
    i4 i = 0;
    if ((Base == NULL) || (Default2D == NULL) || (Default == NULL))
      return (E_DB_ERROR); /* AN ERROR CONDITION */

    *Base = (u_i4 ***)MEreqmem(0, sizeof(u_i4 **) * 256, FALSE, NULL);

    /* array of unicode values (default for conversion) */
    *Default2D = (u_i4 **)MEreqmem(0, sizeof(u_i4 *) * 256, FALSE, NULL);

    /* saves if ptr = NULL return DEFAULT test. */
    *Default = (u_i4 *)MEreqmem(0, sizeof(u_i4) * 256, FALSE, NULL);

    for (i=0;i<256;i++)
    {
      (*Default)[i] = 0xFFFD;
      (*Default2D)[i] = *Default;
      (*Base)[i] = *Default2D;
    }
    return (E_DB_OK);
}

/* Name: adu_map_add_charmap - adds the charmap values to character mapping table.
** Description:
**      This routines, inserts a unicode mapping value
**      to the character map table. 
**
** Input:
**              Base		pointer to validity table.
**              Default2D	pointer to 2D default array.
**              Default		pointer to default array. 
**		bval		byte value to add.
**		unicode		unicode value to add.
**		uhsur		higher surrogate value for unicode if any.
**
** Output:
**		None.
**
** History:
**      23-Jan-2004 (gupsh01)
**          Added.
*/
static void
adu_map_add_charmap(
    u_i4 	***Base, 
    u_i4 	**Default2D, 
    u_i4 	*Default, 
    u_i4 	bval, 
    u_i2 	unicode,
    u_i2	uhsur)
{
    i4 i = 0;
    u_i2 inbyte[3]; 
    inbyte[0] = (bval & 0xFFFFFF) >> 16; 
    inbyte[1] = (bval & 0xFFFF) >> 8;
    inbyte[2] = (bval & 0xFF);

    if (Base[inbyte[0]] == Default2D)
    {
      Base[inbyte[0]] = (u_i4 **)MEreqmem(0, 
		sizeof(u_i4 *) * 256, FALSE, NULL);
      for (i=0;i<256;i++)
        Base[inbyte[0]][i] = Default;
    }
    
    if (Base[inbyte[0]][inbyte[1]] == Default)
    {
      Base[inbyte[0]][inbyte[1]] = (u_i4 *)MEreqmem(0, 
		sizeof(u_i4) * 256, FALSE, NULL);

      for (i=0;i<256;i++)
        Base[inbyte[0]][inbyte[1]][i] = 0xFFFD;
    }

    Base[inbyte[0]][inbyte[1]][inbyte[2]] = unicode;
}

/* Name: adu_map_get_chartouni - gets the unicode for a local character 
**
** Description:
**      This routines, obtains the unicode value inserts a unicode mapping value
**      to the character map table.
**
** Input:
**              Base            pointer to charmap table.
**              bytes           pointer to byte value to lookup.
**
** Output:
**              pointer to unicode value, returns a u_i4. 
** History:
**      23-Jan-2004 (gupsh01)
**          Added.
*/
u_i4 
adu_map_get_chartouni(
    u_i4 	***Base, 
    u_i4 	*bytes)
{
   u_i4	bt = *bytes;
    return Base[(bt & 0xFFFFFF) >> 16][(bt & 0xFFFF) >> 8][bt & 0xFF];
}

/* Name: adu_map_delete_charmap - cleans up a charmap table. 
**
** Description:
**      This routines, deletes the inmemory charmap table and default arrays.
**
** Input:
**              Base            pointer to charmap table.
**              Default2D       pointer to 2D default array.
**              Default         pointer to default array.
**
** Output:
**		None.
** History:
**      23-Jan-2004 (gupsh01)
**          Added.
**	10-Aug-2005 (gupsh01)
**	    Free Base and Default2D. (Bug 115022) 
*/
static void 
adu_map_delete_charmap(
    u_i4 	****Base, 
    u_i4 	***Default2D, 
    u_i4 	**Default)
{
    i4 i,j,k;
    if (*Base)
    {
      for (j=0; j<256; j++)
      {
        if ((*Base)[j] && ((*Base)[j] != *Default2D))
        {
    	for (k=0; k<256; k++)
    	{
     	  if ((*Base)[j][k] && ((*Base)[j][k] != *Default))
    	  {
     	    /* delete ((*Base)[j][k]); */
	    MEfree ((char *)(*Base)[j][k]);
    	    (*Base)[j][k] = NULL;
    	  }
    	}
    	/* delete (*Base)[j]; */ 
    	MEfree ((char *)(*Base)[j]); 
    	(*Base)[j] = NULL;
        }
      }
      MEfree (*Base);	  
      *Base = NULL;    
    }

    if (*Default2D)
    {
      for (i=0; i<256; i++)
      {
        if ((*Default2D)[i] && ((*Default2D)[i] != *Default))
        {
    	/* delete ((*Default2D)[i]); */
	MEfree ((char *)(*Default2D)[i]);
    	(*Default2D)[i] = NULL;
        }
      }
      MEfree ((char *)*Default2D);
      *Default2D=NULL;
    }
    if (*Default)
    {
      /* delete *Default; */
      MEfree ((char *)*Default);	  
      *Default=NULL;
    }
}

/* Name: adu_map_init_unimap - Initializes a unimap array.
**
** Description:
**      This routine, initializes a unimap array which holds information to
**      convert from unicode to local characterset.
**	
**	A substitution character can be provided which will be used to 
**	initialize the default arrays.
** Input:
**              UniBase            pointer to unimap table.
**              Unidefault         pointer to default array.
**              subchar		   substitution character for initialization.
** Output:
**
**      None.
**
** History:
**      23-Jan-2004 (gupsh01)
**          Added.
*/
static DB_STATUS
adu_map_init_unimap(
    u_i2 	***UniBase, 
    u_i2 	**Unidefault, 
    u_i2 	subchar)
{
    i4 		i=0;
    DB_STATUS 	stat = E_DB_OK;
    if ((UniBase == NULL) || (Unidefault == NULL))
      return (E_DB_ERROR);	/*Error Condition*/

    *UniBase = (u_i2 **)MEreqmem(0, sizeof(u_i2 *) * 256, FALSE, NULL);

    *Unidefault = (u_i2 *)MEreqmem(0, sizeof(u_i2) * 256, FALSE, NULL);
    for (i=0;i<256;i++)
    {
      (*Unidefault)[i] = subchar; 
      (*UniBase)[i] = *Unidefault;
    }
    return stat;
}

/* Name: adu_map_add_unimap - adds unimap values to unicode mapping table.
**
** Description:
**
**      This routines, inserts a unicode mapping value to the unicode map table.
**
** Input:
**              UniBase         pointer to unitochar table.
**              Unidefault      pointer to default array.
**              local           byte value to add.
**              unicode         unicode value to add.
**              uhsur           higher surrogate value for unicode if any.
**              subchar		substitution character for initialization.
**
** Output:
**              None.
**
** History:
**      23-Jan-2004 (gupsh01)
**          Added.
*/
static void 
adu_map_add_unimap (
    u_i2 	**UniBase, 
    u_i2 	*Unidefault, 
    u_i2 	local, 
    u_i2 	unicode, 
    u_i2 	uhsur,
    u_i2 	subchar)
{
    int i = 0;
    u_i2 unibytes[2]; 
    unibytes[0] = (unicode & 0xFFFF) >> 8;
    unibytes[1] = unicode & 0xFF;
		
    if (UniBase[unibytes[0]] == Unidefault)
    {
      UniBase[unibytes[0]] = (u_i2 *)MEreqmem(0, sizeof(u_i2) * 256, FALSE, NULL);
      for (i=0;i<256;i++)
        UniBase[unibytes[0]][i] = subchar;
    }
    UniBase[unibytes[0]][unibytes[1]] = local;
}

/* Name: adu_map_get_unitochar - gets the local character value from unicode 
**
** Description:
**      This routines, obtains the local value from unicode.
**
** Input:
**              Unibase         pointer to unimap table.
**              unicode		pointer to unicode value to lookup.
**
** Output:
**              pointer to byte value, returns a u_i2.
**
** History:
**      23-Jan-2004 (gupsh01)
**          Added.
*/
u_i2 
adu_map_get_unitochar(
    u_i2 	**UniBase, 
    u_i2 	*unicode)
{
    u_i2 ucode = *unicode;
    return UniBase[(ucode & 0xFFFF) >> 8][ucode & 0xFF];
}

/* Name: adu_map_delete_unimap - cleans up the unicode mapping table.
**
** Description:
**      This routines, releases the memory allocated for unicode
**	mapping table
**
** Input:
**              Unibase         pointer to unimap table.
**              unidefault 	pointer to default table.
**
** Output:
**              None. 
**
** History:
**      23-Jan-2004 (gupsh01)
**          Added.
**	10-Aug-2005 (gupsh01)
**	    Modify this routine to pass in the address
**	    of UniBase and Unidefault, so we can free them
**	    (Bug 115022) 
*/
static void 
adu_map_delete_unimap(
    u_i2	***UniBase, 
    u_i2	**Unidefault)
{
    int i;
    if (*UniBase)
    {
      for (i=0; i<256; i++)
      {
        if ((*UniBase)[i] && ((*UniBase)[i] != *Unidefault))
        {
	  /* delete (UniBase)[i]; */
	  MEfree ((char *)(*UniBase)[i]);
	  (*UniBase)[i] = NULL;
        }
      }
      MEfree ((char *)(*UniBase));
      *UniBase = NULL;      
    }
    if (*Unidefault)
    {
      /* delete Unidefault; */
      MEfree ((char *)(*Unidefault));
      *Unidefault = NULL;
    }   
}

/* Name: adu_readmap - Reads and processes a coercion map table.  
**
** Description:
** 	This routine reads a compiled coercion map table and
**	initializes the in memory table structures for the map  
**	table.
**
** Input:
** 	chset:  	character set to read from.
**
** Output:
**	E_DB_OK		if the maptable is opened and initialized.
**	E_DB_ERROR	if it fails.
**
** History:
**      23-Jan-2004 (gupsh01)
**          Created.
**	18-Feb-2004 (gupsh01)
**	    Added handling of CM_DEFAULTFILE_LOC, so that ingbuld 
**	    at install time when no characterset info is avaliable
**	    reads the default mapping file from $II_CONFIG location.
**	22-Oct-2004 (gupsh01)
**	    Fixed for correctly reading the mapping files.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**
*/
DB_STATUS
adu_readmap(char *charset)
{
    CL_ERR_DESC         syserr;
    char                *buf = NULL;
    char                *bufptr = NULL;
    i4                  bytes_read = 0;
    DB_STATUS              stat = OK;
    ADU_MAP_HEADER      *header;
    ADU_MAP_ASSIGNMENT	*assignments;
    char		*aptr;
    ADU_MAP_VALIDITY 	*validities;
    char		*vptr;
    i4 			buf_rem;
    i4 			val_rem;
    i4 			assn_rem;

    if (CMopen_col(charset, &syserr, CM_UCHARMAPS_LOC) != OK)
    {
      /* If we are opening the default file then check and return 
      ** an ERROR if we are unable to open the file */
      if (STbcompare (charset, 0, "default", 0, 1) == 0)
      {
	  if (CMopen_col(charset, &syserr, CM_DEFAULTFILE_LOC) != OK)
            return (FAIL);
      }
      else 
	return (FAIL);
    }

    /* allocate memory for buffer */
    buf = MEreqmem(0, COL_BLOCK, TRUE, &stat);
    if (buf == NULL || stat != OK)
    {
       CMclose_col(&syserr, CM_UCHARMAPS_LOC);
       return (FAIL);
    }

    /* First file buffer has header information. */
    stat = CMread_col(buf, &syserr);
    if (stat != OK)
    {
        MEfree((char *)buf);
        CMclose_col(&syserr, CM_UCHARMAPS_LOC);
       return (FAIL);
    }
    bytes_read = COL_BLOCK;

    header = (ADU_MAP_HEADER *) MEreqmem(0,
                    sizeof(ADU_MAP_HEADER), TRUE, &stat);
    if (stat != OK)
    {
        MEfree((char *)buf);
        CMclose_col(&syserr, CM_UCHARMAPS_LOC);
        return FAIL;
    }
    MEcopy (buf, sizeof(ADU_MAP_HEADER), header);
    bufptr = buf + sizeof(ADU_MAP_HEADER);
    bufptr = ME_ALIGN_MACRO (bufptr, sizeof(PTR));

    if (header->validity_size > 0)
    {
      validities = (ADU_MAP_VALIDITY *)MEreqmem(0,
                        (u_i4) header->validity_size, FALSE, &stat);
      if (validities == NULL || stat != OK)
      {
           MEfree((char *)buf);
           CMclose_col(&syserr, CM_UCHARMAPS_LOC);
           return FAIL;
      }
      vptr = (char *) validities;
    }
    else
    {
      /* ERROR: validity information is required */
      CMclose_col(&syserr, CM_UCHARMAPS_LOC);
      return (FAIL);
    }

    if (header->assignment_size > 0)
    {
      assignments = (ADU_MAP_ASSIGNMENT *)MEreqmem (0,
                        (u_i4) header->assignment_size , FALSE, &stat);
      if (assignments == NULL || stat != OK)
      {
        MEfree((char *)buf);
        CMclose_col(&syserr, CM_UCHARMAPS_LOC);
        return FAIL;
      }
      aptr = (char *) assignments;
    }
    else
    {
        /* ERROR: assignment information is required */
        CMclose_col(&syserr, CM_UCHARMAPS_LOC);
        return (FAIL);
    }

    buf_rem =  COL_BLOCK - sizeof(ADU_MAP_HEADER);
    val_rem = header->validity_size;
    assn_rem = header->assignment_size;
    
    for ( ;(bytes_read < (header->validity_size + 
		         header->assignment_size + 
		         sizeof(ADU_MAP_HEADER))) || 
			(assn_rem > 0); )
    {
      if (val_rem > 0)
      {
        if (buf_rem >= val_rem) 
        {
          /* Validation table was read completely */
          MEcopy (bufptr, val_rem, vptr);
	  bufptr += val_rem;
	  bufptr = ME_ALIGN_MACRO (bufptr, sizeof(PTR));
          buf_rem -= val_rem;
          val_rem = 0;

          if ((assn_rem > 0) && (buf_rem <= assn_rem))
          {
            MEcopy (bufptr, buf_rem, aptr);
	    bufptr = ME_ALIGN_MACRO (bufptr, sizeof(PTR));
	    bufptr += buf_rem;
	    aptr +=buf_rem;
            assn_rem -= buf_rem;
	    buf_rem = 0;
          }
          else 
          {
            MEcopy (bufptr, assn_rem, aptr);
	    bufptr += assn_rem;
	    bufptr = ME_ALIGN_MACRO (bufptr, sizeof(PTR));
	    aptr +=assn_rem;
            buf_rem -= assn_rem;
	    assn_rem = 0;
          }
        }
        else 
        {
          /* read more validities before proceeding  */
          MEcopy (bufptr, buf_rem, vptr);
	  bufptr = ME_ALIGN_MACRO (bufptr, sizeof(PTR));
	  vptr += buf_rem;
          val_rem -= buf_rem;  
          buf_rem = 0 ;
        }
      }
      else if (assn_rem > 0)
      {
        if (buf_rem <= assn_rem)
        {
          MEcopy (bufptr, buf_rem, aptr);
	  aptr += buf_rem;
          assn_rem -= buf_rem;
	  buf_rem = 0;
        }
        else 
        {
          MEcopy (bufptr, assn_rem, aptr);
	  aptr += assn_rem;
          buf_rem -= assn_rem;
	  assn_rem = 0;
        }
      }	
      /* read next buffer if more data is needed to be read */
      if (assn_rem > 0)
      {
        stat = CMread_col(buf, &syserr);
        if (stat != OK)
        {
          /* FIXME: either we are have got an error or we have
          ** found the end of the file in either case
          ** free the buf and exit. should report
          ** ERROR condition.
          */
          break;
        }
	bufptr = buf; /* initialize bufptr */
        bytes_read += COL_BLOCK;
        buf_rem = COL_BLOCK;
      }
    } 

    /* check if we have read the whole file */
    if (bytes_read < (header->validity_size +
                      header->assignment_size + 
                      sizeof(ADU_MAP_HEADER)))
    {
      /* we had to exit for some unknown reason */
      MEfree((char *)buf);
      CMclose_col(&syserr, CM_UCHARMAPS_LOC);
      return FAIL;
    }

    /* done with the file so close it now */
    if (CMclose_col(&syserr, CM_UCHARMAPS_LOC) != OK)
    {
        MEfree ((char *)buf);   /* free the buffer */
        return FAIL;
    }

    /* set up the pointers in the ADF memory 
    ** from the information obtained from mapfile. 
    */
    stat =  adu_initmap (header, validities, assignments);
    if (stat)
      return (E_DB_ERROR);

    if (header)
      MEfree ((char *)header);

    if (validities)
      MEfree ((char *)validities);

    if (assignments)
      MEfree ((char *)assignments);

    if (buf)
      MEfree ((char *)buf);
    
    return (E_DB_OK);
}

/* Name: adu_initmap - Initializes the server map table in memory
**
** Description:
**	This routine initializes the server maptable with the 
**	assignment and validity information read from the compiled
**	unicode coercion mapping table.
** Input:
**	header		- pointer to mapping header information
**	validities	- pointer to validity data	
** 	assignments	- pointer to actual assignment data.	
** Output:
**      E_DB_OK         if the maptable is opened and initialized.
**      E_DB_ERROR      if it fails.
**
** History:
**	23-Jan-2004 (gupsh01)
**	    Created.
**	17-Feb-2004 (gupsh01)
**	    Fixed handling of header. Always set substitution 
**	    character to 0, as substitution by default is not
**	    allowed, we restore to substitution only if users
**	    require it at retrieval. This only effercts unitochar
**	    chartouni conversion always store unicode substitution
**	    character 0xFFFD if the unicode code point is not found
**	    for the char in question in the table.
*/
static DB_STATUS
adu_initmap(
    ADU_MAP_HEADER	*header,
    ADU_MAP_VALIDITY	*validities,
    ADU_MAP_ASSIGNMENT	*assignments
)
{
    ADU_MAP_INFO         *mapinfo;
    DB_STATUS		 stat;
    u_i4 		***cbase; 
    u_i4		**default2d; 
    u_i4 		*default1d;
    u_i2 		**ubase; 
    u_i2		*udefault; 
    u_i4		subchar = 0;
    i4			i;

    i4	validity_cnt = (header->validity_size / sizeof (ADU_MAP_VALIDITY));
    i4	assignment_cnt =(header->assignment_size / sizeof (ADU_MAP_ASSIGNMENT));

    /* Initialize space for the lookup tables */  
    mapinfo = (ADU_MAP_INFO *)(MEreqmem(0,
                  sizeof(ADU_MAP_INFO), FALSE, &stat));
    if (stat != OK)
        return (E_DB_ERROR);

    mapinfo->header = (ADU_MAP_HEADER *)(MEreqmem(0,
                  sizeof(ADU_MAP_HEADER), TRUE, &stat));

    mapinfo->charmapptr = (ADU_MAP_CHARMAP *)(MEreqmem(0,
                  sizeof(ADU_MAP_CHARMAP), TRUE, &stat));

    mapinfo->unimapptr = (ADU_MAP_UNIMAP *)(MEreqmem(0,
                  sizeof(ADU_MAP_UNIMAP), TRUE, &stat));

    stat = adu_map_init_charmap (&cbase, &default2d, &default1d); 
    if (stat)
      return (E_DB_ERROR);

    MEcopy (header, sizeof(ADU_MAP_HEADER), mapinfo->header);

    (mapinfo->charmapptr)->charmap = cbase; 
    (mapinfo->charmapptr)->chardefault2d = default2d; 
    (mapinfo->charmapptr)->chardefault1d = default1d;

    stat = adu_map_init_unimap (&ubase, &udefault, subchar); 
    if (stat)
      return (E_DB_ERROR);

    (mapinfo->unimapptr)->unimap = ubase; 
    (mapinfo->unimapptr)->unidefault = udefault;

    mapinfo->validity_tbl = NULL;

    /* Append states to validity state table */
    for (i=0; i < validity_cnt; i++)
    {
      stat = adu_map_append_validity (&(mapinfo->validity_tbl), &validities[i]);
      if (stat)
      {
	/* cannot append the state information return error */
        return (E_DB_ERROR);
      }
    }

    for (i=0; i < assignment_cnt; i++)
    {
      /* Insert assignments to the charmap table */
      adu_map_add_charmap (((mapinfo->charmapptr)->charmap), 
			   ((mapinfo->charmapptr)->chardefault2d), 
			   ((mapinfo->charmapptr)->chardefault1d),
			   assignments[i].bval, 
			   assignments[i].uval, 
			   assignments[i].uhsur);

      /* Insert assignments to the unimap table */
      adu_map_add_unimap (((mapinfo->unimapptr)->unimap), 
			  ((mapinfo->unimapptr)->unidefault), 
			   assignments[i].bval, 
			   assignments[i].uval, 
			   assignments[i].uhsur,
			   subchar);
    }
    Adf_globs->Adu_maptbl = (char *)mapinfo;

    return (E_DB_OK);
}

/* Name: adu_map_delete_vldtbl - cleans up the memory for validity table. 
**
** Description:
**      This routines, releases the memory allocated for 
**	state table for validity check on valid code points.
**
** Input:
**              validity_table - pointer to the validity table.
**
** Output:
**              None. 
**
** History:
**      02-Sep-2004 (gupsh01)
**          Added.
*/
static void 
adu_map_delete_vldtbl (
    ADU_MAP_STATETABLE *validity_table)
{
    int i;
    ADU_MAP_STATETABLE  *ptr = NULL;

    if (validity_table)
    {
      for (ptr=validity_table; ptr != NULL; ptr = ptr->next)
      {
	/* For every row in the state table */

	/* Delete all arrays entries in the state table */
        for (i=0; i<256; i++)
        {
    	   if (ptr->state_table[i])
           {
	     MEfree ((char *)(ptr->state_table[i]));
	     ptr->state_table[i] = NULL;
	   }
        }

	/* Delete the statetable entries itself. */
    	if (ptr->state_table)
	{
	  MEfree ((char *)ptr->state_table);
	  ptr->state_table = NULL;
	}
      }

      if (validity_table)
      {
	MEfree ((char *)validity_table);
        validity_table = NULL;
      }
    }
}

/* Name: adu_deletemap - Cleans up the memory allocated for mapping tables. 
**
** Description:
**      Releases memory allocated for coercion mapping tables.
** Input:
**	None 
** Output:
**	None 
**
** History:
**      23-Jan-2004 (gupsh01)
**          Created.
**	17-Feb-2004 (gupsh01)
**	    Fixed handling the map header.
**      16-Apr-2004 (loera01)
**          Removed redundant MEfree of Adf_globs->Adu_maptbl.
**	02-Sep-2004 (gupsh01)
**	    Add cleaning up of validity table memory.
**	10-Aug-2005 (gupsh01)
**	    Free charmapptr and unimapptr.(Bug 115022) 
**
*/
DB_STATUS
adu_deletemap(void)
{
   DB_STATUS		status;
   ADU_MAP_HEADER 	*header;
   ADU_MAP_CHARMAP	*charmapptr;
   ADU_MAP_UNIMAP	*unimapptr;
   ADU_MAP_STATETABLE   *validity_tbl;

   header = ((ADU_MAP_INFO *)(Adf_globs->Adu_maptbl))->header;
   charmapptr = ((ADU_MAP_INFO *)(Adf_globs->Adu_maptbl))->charmapptr;
   unimapptr = ((ADU_MAP_INFO *)(Adf_globs->Adu_maptbl))->unimapptr;
   validity_tbl = ((ADU_MAP_INFO *)(Adf_globs->Adu_maptbl))->validity_tbl;

   if (header)
     MEfree ((char *)header);

   adu_map_delete_charmap (
		&charmapptr->charmap, 
		&charmapptr->chardefault2d, 
		&charmapptr->chardefault1d);

   adu_map_delete_unimap (
		&unimapptr->unimap, 
		&unimapptr->unidefault);

   adu_map_delete_vldtbl (validity_tbl);

   MEfree ((char *)charmapptr);
   MEfree ((char *)unimapptr);

   if (Adf_globs->Adu_maptbl)
     MEfree ((char *)Adf_globs->Adu_maptbl);

   Adf_globs->Adu_maptbl = NULL;

   return (E_DB_OK);
}
