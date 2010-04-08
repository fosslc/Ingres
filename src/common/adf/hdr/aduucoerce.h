/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: ADUUCOERCE.H - defines for unicode collation
**
** Description:
**      Header file to support Unicode coercion in ADF.
**
** History:
**      12-feb-2002 (gupsh01)
**          Created.
**	02-jan-2004 (gupsh01)
**	    Modified to support the xml format for mapping files.
**	24-Aug-2009 (kschendel) 121804
**	    Add more prototypes for gcc 4.3.
*/

/*
** Name: ADU_MAP_HEADER - Header information for the mapping table.
**
** Decsription:
**      This node contains the header information about the mapping
**      table.
**
** History:
**    28-May-2002 (gupsh01)
**      Created.
**    19-Nov-2002 (gupsh01)
**      Comments added.
**	06-Jan-2004 (gupsh01)
**	    Added more stuff for supporting UTR22. 
**
*/
typedef struct _ADU_MAP_HEADER{
    SIZE_TYPE	validity_size;		/* size of the validity cells */
    SIZE_TYPE	assignment_size;	/* size of the assignments */
    i4 		flags;			/* This flag defines whether 
					** the map table is multibyte */
#define ADU_ISMBCS      0x0001;
    char	id[30];
    char	version[25]; 		/* version of code page */
    char	description[50];	 
    char	contact[50];
    char	registrationAuthority[25];
    char	registrationName[25];
    char	copyright[30];
    char	bidiOrder[11];		/* logical, logicalRTL, logicalLTR */
    char	combiningOrder[7];	/* before|after */
    char	normalization[15];	/* NFC|NFD|NFC_NFD|undertermined or 
					** neither */
    i4      	totalcodes;		/* Total # of codes in code page */
    u_i4    	subchar;		/* The replacement character to be 
					** used for untransliterable unicode 
					** characters 
				 	*/
    u_i2        subchar1;               /* The alternate replacement
                                        ** characterto be used for
                                        ** untransliterable unicode characters.
                                        */
} ADU_MAP_HEADER;

# define        ST_VALID        0x0001  /* The byte sequence is valid,
                                        ** search for unicode value.
                                        */

# define        ST_INVALID      0x0010  /* The byte sequence is
                                        ** illegal return error.
                                        */

# define ST_EXP_UNASSIGNED      0x1000  /* The sequence is incomplete
                                        ** i.e the final state is not
                                        ** recheched but all input bytes
                                        ** are exhausted return error.
                                        */

# define        ST_UNASSIGNED   0x0000  /* Byte sequence is valid but
                                        ** unassigned return Substitute
                                        ** character if so requested.
                                        */

# define        ST_INCOMPLETE   0x0100  /* The sequence is incomplete
                                        ** i.e we have not reached the
                                        ** final state return an error.
                                        */

# define        ST_AMBIGUOUS    0x0100 /** A situation where xxyyzz is
                                        ** input and xxyy is a
                                        ** valid/invalid sequence
                                        ** This is ambiguous as codepage
                                        ** could declare xxyyzz also valid
                                        ** but we can never map to it.
                                        */

# define        MAXBYTES        3       /* Max Number of bytes in a code point
                                        ** byte sequence for the code page
                                        */

typedef struct _ADU_MAP_VALIDITY{
    char	state_type[15];
    char	state_next[15];
    u_i2	state_start;
    u_i2	state_end;
    u_i2	state_max;
} ADU_MAP_VALIDITY;

typedef struct _ADU_MAP_VLDSTATE ADU_MAP_VLDSTATE;

struct _ADU_MAP_VLDSTATE 
{
    u_i2		property;
    ADU_MAP_VLDSTATE	**next;
};

typedef struct _ADU_MAP_STATETABLE ADU_MAP_STATETABLE;

struct _ADU_MAP_STATETABLE
{
    char		name[15];
    ADU_MAP_VLDSTATE	**state_table;
    ADU_MAP_STATETABLE	*next;
};

typedef struct _ADU_MAP_CHARMAP{
    u_i4		***charmap;
    u_i4		**chardefault2d;
    u_i4		*chardefault1d;
} ADU_MAP_CHARMAP;

typedef struct _ADU_MAP_UNIMAP{
    u_i2		**unimap;
    u_i2		*unidefault;
} ADU_MAP_UNIMAP;

/* Validation state table Defines */
/*
** Name: ADU_MAP_INFO - Information about the mapping table.
**
** Decsription:
**              The main structure which holds all
**              the information about mapping table
**              and the coercion mapping table itself.
**
** History:
**    28-May-2002 (gupsh01)
**      Created.
**    19-Nov-2002 (gupsh01)
**      Comments added.
**
*/
typedef struct _ADU_MAP_INFO{
    ADU_MAP_HEADER 	*header;	/* header information */
    ADU_MAP_STATETABLE  *validity_tbl;	/* validity table for the mapping */ 
    ADU_MAP_CHARMAP	*charmapptr;
    ADU_MAP_UNIMAP	*unimapptr;
} ADU_MAP_INFO;

/* Name: ADU_MAP_ASSIGNMENT - assignment values read from mapping file.
** The values for c UTF8 bytes and v version are ignored initially
*/
typedef struct _ADU_MAP_ASSIGNMENT
{
    u_i4	bval;	/* byte value */
    u_i2	uval;	/* unicode value */ 
    u_i2	uhsur;	/* unicode high surrogate 
			** usually NULL */
} ADU_MAP_ASSIGNMENT;

/* ALIAS TABLE Definitions */
typedef struct _ADU_ALIAS_MAPPING
{
    char	mapping_id[50];
    char	display_name[50];      
    char	display_xmllang[3];    /* Two letter language code */
    int		mappingIndex;
} ADU_ALIAS_MAPPING;

typedef struct _ADU_ALIAS_DATA
{
    char	aliasName[50];         
    char	aliasNameNorm[50];     
    char	aliasPrefBy[50];       
    int		aliasMapId;
} ADU_ALIAS_DATA;

  
/* Name: ADU_CHARACTER_MAPPING - The structure of the compiled 
**	 unicode coercion mapping file. 
*/
typedef struct _ADU_CHARACTER_MAPPING
{
  ADU_MAP_HEADER 	*header;	 /* mapping file header */
  i4 			validity_size;	 /* size of validity entries */
  ADU_MAP_VALIDITY 	*validity;	 /* validity table cells */	
  i4 			assignment_size; /* size of assignment entries */
  ADU_MAP_ASSIGNMENT	*assignment; 	 /* assignments */
} ADU_CHARACTER_MAPPING;

/* External Function Declarations */
FUNC_EXTERN DB_STATUS adu_getconverter(char *converter);
FUNC_EXTERN VOID adu_csnormalize(char        *instring,
    				i4          inlength,
    				char        *outstring);

FUNC_EXTERN u_i4 adu_map_get_chartouni(u_i4 ***, u_i4 *);
FUNC_EXTERN u_i2 adu_map_get_unitochar(u_i2 **, u_i2 *);
