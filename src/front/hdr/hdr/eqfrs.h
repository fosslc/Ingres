/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: eqfrs.h - EQUEL Set/Inquire FRS Module Definitions.
**
** Description:
**	Contains definitions for the Set/Inquire FRS module of EQUEL.  These
**	definitions provide an interface between the interpretive and code
**	generation routines of the module.
**
** History:
**	    Initial revision.  (Moved from "eqfrs.c".)
**	20-feb-92 (sandyd)
**	    Moved "extern FRS__  frs;" out of here, and into the only file
**	    which really references it -- embed!equel eqgenfrs.c.  This was
**	    done because the W4GL OSLGRAM.MY includes this hdr file and the 
**	    "frs" reference was causing most of equel to get pulled into the 
**	    W4GL link on VMS.
**	07-oct-92 (lan)
**	    Added constant _fMEN for menu object type.
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** FRS_OBJ - Define the structure to store a master object-type.  The mapping 
**	     information includes a flag to tell whether it was supported
** under the old syntax.
*/
typedef struct {
    char	*fo_name;		/* Name of object-type */
    i4		fo_def;			/* FRS defintion from fsiconsts.h */
    i4		fo_args;		/* Number of parent-names required */
    i4		fo_flag;		/* Flag for use with frs-constants */
    bool	fo_old;			/* Was it supported under old EQUEL */
} FRS_OBJ;

/* Constants for the fo_flag field to map to frs-constant possiblities */
# define	_fFLD	0001
# define	_fFRM	0002
# define	_fTAB	0004
# define	_fCOL	0010
# define	_fFRS	0020
# define	_fROW	0040
# define	_fRCL	0100
# define	_fMEN	0200

/*
** FRS_NAME - 	Parent-names of an FRS object consist of a name and possibly
**	      	a symbol table entry if there was a variable used.
**  	      	Object-names are the same.
*/
typedef struct {
    char	*fn_name;
    PTR		fn_var;
} FRS_NAME;

/* Maximum number of parent-names for an object */
# define	FRSmaxNAMES		3

/*
** FRS_CONST - Define the structure to store an frs-constant.  The mapping 
**	       information includes a flag to tell under which FRS object-types
** this constant is allowed.
*/
typedef struct {
    char	*fc_name;		/* Name of frs-constant */
    i4		fc_def;			/* FRS define in fsiconsts.h */
    i4		fc_type[2];		/* Types for INQUIRE and SET (if set) */
    i4		fc_oname;		/* Is an object-name required ? */
    i4		fc_oflag;		/* Flag which object-types allow this */
} FRS_CONST;

/* Types for fc_oname */
# define	_fOBNUL		0	/* No object-name */
# define	_fOBREQ		1	/* Object-name required */
# define	_fOBOPT		2	/* Object-name optional */

# define	fc_canset(fc)	(fc->fc_type[FRSset] != T_NONE)

/* Is numbered object within range ? */
# define	fc_keyrange(v)	(v >= 1 && v <= Fsi_KEY_PF_MAX)
# define	fc_murange(v)	(v >= 1 && v <= Fsi_MU_MAX)


/*
** FRS_VAR - Variables used with an INQUIRE.
*/
typedef struct {
    char	*fv_name;
    i4		fv_type;
} FRS_VAR;

/*
** FRS__ - Global FRS information stored on a per-statement basis.
*/
typedef struct {
    i4		f_mode;			/* Set or Inquire */
    bool	f_err;			/* Statement level error */
    bool	f_old;			/* Supported under old EQUEL ? */
				/* Info for the statement head */
    FRS_OBJ	*f_obj;			/* Current object-type */
    FRS_NAME	f_row;			/* Row information (usually default) */
    FRS_NAME	f_pnames[FRSmaxNAMES];	/* Object parent-names */
    i4		f_argcnt;		/* Number of parent-names */
				/* Info for the current frs-constant */
    FRS_CONST	*f_const;		/* Current frs-constant in use */
    i4		f_consterr;		/* frs-constant level error */
    FRS_VAR	f_var;			/* Inquire var or Set value */
} FRS__;

# define 	FRSrowDEF	"0"	/* Default for current row */
