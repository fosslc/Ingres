/*
** Copyright (c) 1985, Ingres Corporation
*/

# include	<si.h>

/**
** Name: DS.H - Global definitions for the DS compatibility library.
**
** Description:
**      The file contains the type used by DS and the definition of the
**      DS functions.  These are used by the data structure definition 
**      facility.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**	9-sep-1986 (Joe)
**	   Updated to 5.0 spec.
**	23-feb-1987 (Joe)
**	   Added DSD_FRADR, DSD_VRADR, DSD_FNADR, and DSD_MENU.
**	   Gotten from 5.0 code.  Needed by FE code.
**	10-jun-1987 (Joe)
**	   Adding include of si.h and new DSD_ constants:
**		DSD_PTR, DSD_NULL and DSD_FORP
**	14-jun-1988 (greg)
**	    Changed DSD_CADDR to DSD_IADDR (integrating "forgotten" UNIX change)
**	03-apr-1990 (fpang)
**	    With CL committee approval, added the new DS datatype DSD_NATARY
**	    which will be used to initialize fields to (nat *). On vms DSD_NATARY
**	    is the same as DSD_ARYADR, but on UNIX compilers it will be (nat *).
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN VOID   DSbegin();          /* Begin file. */
FUNC_EXTERN STATUS DSload();          /* Load encoded data. */
FUNC_EXTERN STATUS DSstore();          /* Encode data structure. */
FUNC_EXTERN STATUS DSdecode();
FUNC_EXTERN i4     DSencode();


/* 
** Defined Constants
*/

/* DS return status codes. */

#define                 DS_OK           0
#define			DSN_RECUR	(E_CL_MASK + E_DS_MASK + 0x01)
                                	/* Can't be recursively called */
#define			DSN_LANG	(E_CL_MASK + E_DS_MASK + 0x02)
                                        /* A bad language parameter */
#define			DSN_ALIGN	(E_CL_MASK + E_DS_MASK + 0x03)
                                        /* A bad Alignment parameter */
#define			DSN_VIS		(E_CL_MASK + E_DS_MASK + 0x04)
                                        /* Bad visiblity parameter */
#define			DSU_NKNOWN	(E_CL_MASK + E_DS_MASK + 0x05)
                                        /* Don't know how to do */


/*
** LANGUAGES
*/
# define	DSL_LOW		1
# define	DSL_MACRO	1		/* macro */
# define	DSL_C		2		/* C */
# define	DSL_HI		2

/*
** ALIGNMENT TYPES
*/
# define	DSA_LOW		1
# define	DSA_UN		1		/* unaligned */
# define	DSA_C		2		/* As C would */
# define	DSA_HI		2

/*
** VISIBILITY
*/
# define	DSV_LOW		1
# define	DSV_GLOB	1		/* GLOBAL */
# define	DSV_LOCAL	2		/* Local */
# define	DSV_HI		2

/*
** DATA TYPES
*/
# define	DSD_LOW		1
# define	DSD_I4		1		/* Long word, 4 bytes */
# define	DSD_NAT		1		/* Long word, 4 bytes */
# define	DSD_I2		2		/* a short */
# define	DSD_I1		3		/* a byte */
# define	DSD_F4		4		/* FLOAT */
# define	DSD_F8		5		/* a double */
# define	DSD_CHAR	6		/* a character */
# define	DSD_STR		7		/* A STRING, must have length*/
# define	DSD_SPTR	8		/* A string pointer */
# define	DSD_ADDR	9		/* AN ADDRESS */
# define	DSD_IADDR	(i4) 10		/* Address cast to (char *) */
# define	DSD_ARYADR	(i4) 11		/* Address of an array */
# define	DSD_PTADR	(i4) 12		/* */
# define	DSD_PLADR	(i4) 13
# define	DSD_VRADR	9		/* AN ADDRESS */
# define	DSD_FRADR	9		/* AN ADDRESS */
# define	DSD_MENU	9		/* AN ADDRESS */
# define	DSD_FNADR	9		/* AN ADDRESS */
# define	DSD_PTR		9		/* A cast to PTR (an address) */
# define	DSD_NULL	1		/* Generate the NULL constant */
# define	DSD_FORP	9		/* A cast to ABRTSFO (addr) */
# define	DSD_HI		13
# define	DSD_NATARY	(i4) 11 	/* Address of a nat array */

/*
**	Data structure Id's for standard types
*/

/*
**	DS_id values 0 and 1 have been reserved for
**	DS_REGFLD and DS_TBLFLD for compatability
**	with exisiting compiled forms.  (See front/hdr/feds.h.)
*/

# define	DS_I2		2		/* a short */
# define	DS_SHORT	2
# define	DS_I1		3		/* a byte */
# define	DS_F4		4		/* FLOAT */
# define	DS_FLOAT	4
# define	DS_F8		5		/* a double */
# define	DS_DOUBLE	5
# define	DS_CHAR		6		/* a character */
# define	DS_STR		7		/* A STRING, must have length*/
# define	DS_SPTR		8		/* A string pointer */
# define	DS_STRING	8
# define	DS_ADDR		9		/* AN ADDRESS */
# define	DS_LOCATION	10
# define	DS_BOOL		11
# define	DS_DATENTRNL 	12		/* date data type */
# define	DS_I4		13		/* Long word, 4 bytes */
# define	DS_INT		13
# define	DS_LONG		13
# define	DS_NAT		14

typedef struct {
	int	dsId;
	int	dsControl;	/* This is a bit mask, and for future use */
#define			dstFILE		01
#define			dstFUNC		02
#define			dstSTRUCT	04
	char	*dsLocation;	/* file containing template description*/
	int	(*dsFunc)();	/* function interpreting data structure */
	/* more stuff (like the layout description )goes in here */
} DsTemplate;


/* definitions of data structure template */


/* DSTEMPLATE is a descriptor of data structure */

typedef struct {
	i4	ds_off;		/* offset of the pointer/union field from the 
				** beginning of the data structure
				** for DSVSIZE, DS2VSIZE, this is the offset
				** of the element giving the real structure
                                ** size
				*/
	i4	ds_kind;	/* type of pointer/union field */
			
	i4	ds_sizeoff;	/* if ds_kind is DSARRAY, ds_sizeoff will have 
				** the offset of the array count field from
				** the beginning of the data structure;
				** if ds_kind is DSUNION, ds_sizeoff will have 
				** the offset of the union tag(data structure id) field
				** from the beginning of the data structure;
				** if ds_kind is DSPTRARY, ds_sizeoff will have
				** the constant size of the array.
				** for DSVSIZE, DS2VSIZE this is the size
				** increment ds_off is in terms of
				*/

	i4	ds_temp;	/* if ds_kind is DSARRAY or DSNORMAL, ds_temp 
				** will have the data structure id of the 
				** array element or struct. If array element is
				** a pointer, then use negative data structure
				** id to indicate the id of the pointed data 
				** structure; if ds_kind is DSPTRARY, ds_temp 
				** will have the data structure id of the
				** pointed data structure.
				*/

} DSPTRS;

# define	DSNORMAL	1	/* pointer to struct */
# define	DSSTRP		2	/* char string */
# define	DSARRAY		3	/* array where size of array is not known
					** at compiled time. e.g. i4 *a and i4  **a 
					*/
# define	DSUNION		4	/* union */
# define	DSPTRARY	5	/* array of pointers where size of array is known 
					** at compiled time. e.g. i4 *a[12] 
					*/
# define	DSVSIZE		6	/* actual structure size */
# define	DS2VSIZE	7	/* actual structure size (2 byte) */

typedef struct {
	i4	ds_id;		/* data structure id */
	i4	ds_size;	/* total size of the data structure */
	i4	ds_cnt;		/* total number of pointer and union fields in the data structure
				*/
	i4	dstemp_type;	/* type of data structure template */
	DSPTRS	*ds_ptrs;		/* array of DSPTRS.  Each DSPTRS 
	contains descriptions of
					** pointer/union fields of the data structure
					*/	
	char	*dstemp_file;		/* name of the file that contains descriptions of
					** pointer and union fields of the data structure
					*/
	i4	(*dstemp_func)();	/* pointer to the function that
					** processes pointer and union fields of the data structure
					*/
} DSTEMPLATE;


# define	DS_IN_CORE	1
# define	DS_IN_FILE	2
# define	DS_IN_FUNC	3


/* definitions of shared data region descriptor */

		typedef struct {
		i4	addr;		/* data segment addr */
		i4	key;		/* key of the shared memory segment which will be used by
					** the called module to get the shared memory segment 
					*/
		i4	id;		/* shared memory id associated with the key */
		} SH_MEM;


		typedef struct {
		FILE	*fp;		/* file pointer of the file. 
					** Initialize to NULL if file 
					** needs to be opened
					*/
		char	*name;		/* Unique file name of the file, in the
					** form of ING_TEMP/XXXXXX.ds 
					** where XXXXXX is the process
					** id of the current process and
					** an unique letter
					*/
		} SH_FILE;

		typedef struct {
		i2	sh_type;	/* type of shared data region */

		i2	sh_keep;	/* a flag for keeping data region around after relocation */
		i4	size;		/* size of the shared data region */
		union {
			SH_MEM	mem;	/* shared memory */
			SH_FILE	file;	/* file */
		} sh_reg;
		i2 sh_tag;		/* memory tag for use in allocation */
		bool sh_dstore;		/* destructive store flag */
		} SH_DESC;


# define	KEEP	1

# define	IS_FILE 1 
# define 	IS_SH_MEM 2 
# define	IS_MEM	3 
# define	IS_RACC 4
