/*
** Copyright (c) 2004 Ingres Corporation
*/
#ifndef	DS_H_def
#define	DS_H_def	1

# include	<compat.h>
# include	<si.h>

/*
** some constants used in the data structure definition
** facility
**
** History:
**	85/10/01  22:29:05  daveb
**		Integrate 3.0/23ibs changes
**		Added defines for DS_NAT, DSD_NAT, IS_RACC, etc.
**	86/02/12  22:26:25  daveb
**		Add several new types needed to support C initialization
**		of the abextract file used by ABF.  This eliminates the need
**		to support DSL_MACRO on UNIX.
**	86/04/10  16:06:46  boba
**		change dstemp to dstmp due to Burroughs name space problems
**	86/04/25  10:55:45  daveb
**		3.0 porting changes:
**		- variable names changed for name space problems.
**		- More DS types for elimination of DSL_MACRO on UNIX.
**	86/12/02  09:08:16  joe
**		Initial60 edit of files
**	87/01/28  16:36:27  arthur
**		Brought over changes in B07 release of PC Ingres.
**	87/06/04  13:58:04  bobm
**		add DSD_PTR, DSD_FORP
**	87/06/08  11:28:51  bobm
**		DSD_NULL
**	20-may-88 (bruceb)
**		Changed DSD_CADDR to DSD_IADDR for venus (which will
**		now cast pointers to reg/tbl fields to (i4 *).
**      22-may-90 (blaise)
**              Comment out string following #endif.
**	4-june-90 (blaise)
**	    Integrated changes from termcl code line:
**		Add the new DS datatype DS_NATARY, which will be used to
**		generate initialisations of fields to (i4 *).
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	16-dec-93 (swm)
**	    Bug #58642
**	    Changed type of addr in SH_MEM struct from i4 to PTR; on
**	    Alpha AXP OSF an i4 cannot accomodate a pointer.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** ERROR codes
*/
# define	DSN_RECUR	-1	/* Can't be recursively called */
# define	DSN_LANG	-2	/* A bad language parameter */
# define	DSN_ALIGN	-3	/* A bad Alignment parameter */
# define	DSN_VIS		-4	/* Bad visiblity parameter */
# define	DSU_NKNOWN	-5	/* Don't know how to do */

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
# define	DSD_I2		2		/* a short */
# define	DSD_I1		3		/* a byte */
# define	DSD_F4		4		/* FLOAT */
# define	DSD_F8		5		/* a double */
# define	DSD_CHAR	6		/* a character */
# define	DSD_STR		7		/* A STRING, must have length*/
# define	DSD_SPTR	8		/* A string pointer */
# define	DSD_ADDR	9		/* AN ADDRESS */
# define	DSD_IADDR	10		/* Address cast to (int *) */
# define	DSD_ARYADR	11		/* Address for an array */
# define	DSD_PTADR	12		/* Address cast to (vtree*) */
# define	DSD_PLADR	13		/* Addr cast to (valist *) */
# define	DSD_VRADR	14		/* Addr cast to (ABRTSVAR*) */
# define	DSD_FRADR	15		/* Addr cast to (FRAME **) */
# define	DSD_MENU	16		/* Addr cast to (MENU) */
# define	DSD_FNADR	17		/* Addr cast to (funcptr) */

# define	DSD_HI		17
# define	DSD_NAT		18		/* size depends on machine */
# define	DSD_FORP	19		/* Addr cast to (ABRTSFO*) */
# define	DSD_PTR		20		/* Addr cast to (PTR) */
# define	DSD_NULL	21		/* produce NULL pointer */
# define        DSD_NATARY      22              /* generate (i4 *) */

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
				** size.
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
				** increment ds_off is in terms of.
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
	i4	dstmp_type;	/* type of data structure template */
	DSPTRS	*ds_ptrs;		/* array of DSPTRS.  Each DSPTRS 
	contains descriptions of
					** pointer/union fields of the data structure
					*/	
	char	*dstmp_file;		/* name of the file that contains descriptions of
					** pointer and union fields of the data structure
					*/
	i4	(*dstmp_func)();	/* pointer to the function that
					** processes pointer and union fields of the data structure
					*/
} DSTEMPLATE;


# define	DS_IN_CORE	1
# define	DS_IN_FILE	2
# define	DS_IN_FUNC	3


/* definitions of shared data region descriptor */

		typedef struct {
		PTR	addr;		/* data segment addr */
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
		u_i2	sh_tag;		/* memory tag, 0 for default */
		bool	sh_dstore;	/* flag for destructive store */
		} SH_DESC;


# define	KEEP	1

# define	IS_FILE 1 
# define 	IS_SH_MEM 2 
# define	IS_MEM	3 
# define	IS_RACC 4 

FUNC_EXTERN i4 DSencode( SH_DESC *sh_desc, char *str, char ch );
#endif	/* DS_H_def */

/* ----------------------------------------------------------------------- */
