/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: ADUUCOL.H - defines for unicode collation
**
** Description:
**	This header contains the definitions to support Unicode collation
**	in ADF
**
** History:
**	12-mar-2001 (stephenb)
**	    Created.
**	2-apr-2001 (stephenb)
**	    MAXI2 (max of i2) contains max number of values in i2 -1 (to 
**	    account for the zero). Add 1 to fix defines.
**	25-Sep-2002 (jenjo02)
**	    Added defines, prototypes for LIKE processing.
**	23-Jan-2004 (gupsh01)
**	    Modified for Unicode coercion project.
**	02-Feb-2004 (gupsh01)
**	   Fixed adu_set_recomb_tbl, to set the default values correctly. 
**	04-Feb-2004 (gupsh01)
**	    Fixed memory allocation for recomb_tbl.
**	4-feb-05 (inkdo01)
**	    Add collID parms to adu_umakelces, adu_umakelpat functions.
**	9-mar-2007 (gupsh01)
**	    Added support for Unicode upper/lower case translation.
**	11-May-2009 (kschendel) b122041
**	    Fix proto for init allocator to match MEreqmem's.
*/

#define		MAX_CE_LEVELS		4
#define		MAX_UCOL_REC		2048 /* arbitrary max size of record
					     ** hopefully should be enough */

#define		USTRING_LESS		-1
#define		USTRING_EQUAL		0
#define		USTRING_GREATER		1
#define		USTRING_BAD_TAB		99

/* CE list separators used in LIKE processing */
#define		ULIKE_NORM	(u_i2)0xFFFF   /* CE list follows */
#define		ULIKE_ESC	(u_i2)0xFFFE   /* Escaped char follows */
#define		ULIKE_PAT	(u_i2)0xFFFD   /* Pattern char follows */

/* table size for unicode compiled table (file version):
**	max UCS2 values times size of entry (max of u_i2 = (MAXI2+1)*2)
** 	@FIX_ME@
**	Actually the instruction part is somewhat of a fudge. In theory, we could
**	blow the limit here if we tried to add some instruction to all 64,000 
**	characters. Not likely, but possible. We should probably code some 
**	safeguard into aducompile to check for this.
*/

#define INSTRUCTION_SIZE  ((MAXI2+1) * 2 * sizeof(ADU_UCEFILE))
#define	ENTRY_SIZE	((MAXI2+1) * 2 * sizeof(ADUUCETAB))
#define	RECOMBSIZE	(1000 * (sizeof (u_i2) + sizeof (u_i2 *)) * 256)

typedef struct 	_ADU_COMBENT	ADU_COMBENT;

/*
** Name: ADU_COMBENT - combining entry, has multiple character values
**
** Description:
**	Additional infomation for table entries that are combining, contains
**	exatra combining characters. Note that the assumption here is that a 
**      combining entry will have a single collation element list of MAX_CE_LEVELS.
**	The algorithm states that characters can be both expanding and contracting
**	which means that they could have multiple CE lists on combining entries.
**	We haven't come across this case yet.
**	Note that the CE list is repeated here, since there may be a distinct CE 
**	list for a single non-combining version of the character, which lives in 
**	the main table. We also need to note CE attributes in the flags
**
** History:
**	15-mar-2001 (stephenb)
**	    Created
*/
struct _ADU_COMBENT
{
    i4			num_values;
    i4			flags;
    ADU_COMBENT		*next_combent; /* @VALID FOR IN-MEMORY VERSION ONLY@
				       ** next combining list for this char */
    u_i2		ce[MAX_CE_LEVELS];
    u_i2		values[1]; /* place holder for value array */
};

/*
** Name: ADU_CEOVERFLOW - collation element overflow list
**
** Description:
**	overflow list of collation elements for characters that expand (have
**	more than one collation element list per character)
**
** History:
**	15-mar-2001 (stephenb)
**	    Created
*/
typedef struct _ADU_CEOVERFLOW
{
    i4		num_values;
    u_i2	cval;
    struct
    {
	u_i2	ce[MAX_CE_LEVELS];
	bool	variable;
    } oce[1];/* place holder for array of such */
} ADU_CEOVERFLOW;

/*
** Name: ADU_DECOMP - character decomposition values
**
** Description:
**	This structure contains a variable sized list of decomposition
**	values for a character
**
** History:
**	5-apr-2001 (stephenb)
**	    Created
*/
typedef struct ADU_DECOMP
{
    i4		num_values;
    u_i2	cval;
    u_i2	dvalues[1]; /* holder for array */
} ADU_DECOMP;

/*
** Name: ADU_CASEMAP - character case mapping values
**
** Description:
**	This structure contains a variable sized list to hold  
**	the case mapping for acharacter.
**	The values[] is a variable list which has 2 byte code 
**	points in the following order 
**	  num_uvalues Upper case code points, followed by
**	  num_lvalues Lower case code points, followed by
**	  num_tvalues Title case code points.
** History:
**	08-feb-2007 (gupsh01)
**	    Created
*/
typedef struct ADU_CASEMAP
{
    i2		num_lvals; /* number of lower case code points */
    i2		num_tvals; /* number of title case code points */
    i2		num_uvals; /* number of upper case code points */
    u_i2	cval;	   /* code point value */
    u_i2	csvals[1]; /* values ordered lower, title, upper*/
} ADU_CASEMAP;

/* These defines are used in association with the case mapping */
#define ADU_CASEMAP_UPPER 1
#define ADU_CASEMAP_LOWER 2
#define ADU_CASEMAP_TITLE 3

/*
** Name: ADUUCETAB - Definition of ADF Unicode collation element table
**
** Decsription:
**	This table resides in memory, there will one entry for each
**	value in the UCS2 set (even the ones that have no character
**	or collation). The table contains the character's collation element
**	list.
**
** History:
**	12-mar-2001 (stephenb)
**     	    Created
**	5-apr-2001 (stephenb)
**	    Add support for canonical decomposition for normalization.
**	17-may-2001 (stephenb)
**	    Add combining class
**      25-apr-2002 (gupsh01)
**          Added coercion elements to this table to coerce from unicode
**          to / from local code page. This table may now very well be
**          called Unicode collation and coercion element table.
**	08-jan-2002 (gupsh01)
**	    Added recomb_tab to this structure, which holds the list of
**	    combining characters that combine with this entry to form 
**	    a composite characters, this will enable normal form C 
**	    implementation as given in UTF15.
**	17-mar-2007 (gupsh01)
**	    Added french collation flag. This is optional. If a user has
**	    tailered the udefault.uce file with @backward entry at the
**	    begining of the file we set this here.
*/
typedef struct _ADUUCETAB
{
    u_i2		ce[MAX_CE_LEVELS];
    i4			comb_class;
    i4			flags;
#define		CE_VARIABLE	0x00001 /* entry is variable */
#define		CE_COMBINING	0x00002 /* entry has corresponding 
					** combing characters */
#define		CE_HASOVERFLOW	0x00004 /* there is more than one CE list for
					** this character */
#define		CE_REARRANGE	0x00008 /* character is subject to rearranging */
#define		CE_HASMATCH	0x00010 /* the character actually has a collation
					** element list (not synthesized). This
					** is only set if the single character
					** (not combined) has a CE list */
#define		CE_HASCDECOMP	0x00020 /* character has a canonical */
#define		CE_HASCASE	0x00040 /* character has a case map entry */
#define		CE_CASEIGNORABLE 0x00080 /* This character has a case ignorable property 
					** We are overloading flags to store some
					** of the properties of the character like cased
					** caseignorable is a property where a 
					** character is either of types
					** U+0027 APOSTROPHE, U+00AD SOFT HYPHEN (SHY)
					** U+2019 RIGHT SINGLE QUOTATION MARK or of
					** class
					*/
#define		CE_COLLATEFRENCH 0x00100 /* This is a french collation, really
					 ** a collation level property.
					 */ 
    ADU_COMBENT		*combent;	/* @VALID FOR IN-MEMORY VERSION ONLY@
					** pointer to combinig entry list */
    ADU_CEOVERFLOW 	*overflow;	/* @VALID FOR IN-MEMORY VERSION ONLY@
					** pointer to overflow list */
    ADU_DECOMP		*decomp;	/* @VALID FOR IN_MEMORY VERSION ONLY@
					** pointer to canonical decomposition */
    ADU_CASEMAP		*casemap;	/* @VALID FOR IN_MEMORY VERSION ONLY@
					** pointer to case mapping */
    u_i2		**recomb_tab;	/* Valid if the present character is a 
					** base for composite characters. 
					** This is a list of combining 
					** characters and corresponding 
					** composite character, it forms with 
					** this entry. 0 indicates no 
					** combination is possible for that 
					** combining character. */
} ADUUCETAB;


/*
** Name: ADU_UCEFILE - Description of the compiled unicode collation element file
**
** Description:
**	Contains a definition of the contents of a compiled unicode
**	collation element file. This describes the second part fo the file, which
**	contains various modifying options
**
** History:
**	14-mar-2001 (stephenb)
**	    Created.
**	08-feb-2007 (gupsh01)
**	    Case Mapping Added.
**	07-nov-2007 (gupsh01)
**	    Added adu_umakeqpat.
*/
typedef struct _ADU_UCEFILE
{
    i4		type;
#define		CE_COMBENT	2 /* combining entry */
#define		CE_OVERFLOW	3 /* overflow collation elements */
#define		CE_CDECOMP	4 /* canonical decomposition */
#define		CE_CASEMAP	5 /* case mapping */

    /*
    ** NOTE: The following union should be the last entry in this structure
    ** since the structures it contains are variable in size
    */
    union
    {
	ADU_COMBENT	adu_combent;
	ADU_CEOVERFLOW	adu_ceoverflow;
	ADU_DECOMP	adu_decomp;
	ADU_CASEMAP	adu_casemap;
    } entry;
} ADU_UCEFILE;


FUNC_EXTERN STATUS
aduucolinit(
	    char		*col,
	    PTR		(*alloc)(u_i2  tag,
                         SIZE_TYPE  size,
                         bool   clear,
                         STATUS *status ),
	    ADUUCETAB	**tbl,
	    char	**vartab,
	    CL_ERR_DESC	*syserr);

FUNC_EXTERN DB_STATUS
aduucmp(
	ADF_CB          *adf_scb,
	i4		flags,
	u_i2		*bsa,
	u_i2		*esa,
	u_i2		*bsb,
	u_i2		*esb,
	i4		*rcmp,
	i4		collID);

FUNC_EXTERN DB_STATUS	adu_umakeskey(
	ADF_CB		*adf_scb,
	u_i2		*bst,
	u_i2		*est,
	u_i2		**listp,
	i2		collID);

FUNC_EXTERN DB_STATUS	adu_umakelces(
	ADF_CB		*adf_scb,
	u_i2		*bst,
	u_i2		*est,
	u_i2		**listp,
	i4		collID);

FUNC_EXTERN DB_STATUS	adu_umakelpat(
	ADF_CB		*adf_scb,
	u_i2		*bst,
	u_i2		*est,
	u_i2		*esc,
	u_i2		**listp,
	i4		collID);

FUNC_EXTERN DB_STATUS	adu_umakeqpat(
	ADF_CB		*adf_scb,
	u_i2		*bst,
	u_i2		*est,
	u_i2		*esc,
	u_i2		**listp,
	i4		collID);

FUNC_EXTERN STATUS	adu_set_recomb_tbl(
	ADUUCETAB	*ucetab,
	u_i2		basechar,
	u_i2		***recomb_def2d,
	u_i2		**recomb_def1d,
	i4		*allocated,
	char		**workspace);

FUNC_EXTERN STATUS	adu_init_defaults(
	char		**workspace,
	ADUUCETAB	*ucetab,
	u_i2		***recomb_def2d,
	u_i2		**recomb_def1d);

FUNC_EXTERN u_i2	adu_get_recomb(
	ADF_CB		*adf_scb,
	u_i2		basechar,
	u_i2		combining);

FUNC_EXTERN STATUS	adu_map_conv_normc(
	ADF_CB		*adf_scb,
	ADUUCETAB	*ucetab,
	u_i2		basechar,
	u_i2		*comblist,
	i4		combcount,
	u_i2		*result,
	i4		*resultcnt);

