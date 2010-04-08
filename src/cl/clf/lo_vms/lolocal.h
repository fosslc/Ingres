/*
**    Copyright (c) 1985, 2008 Ingres Corporation
*/

/**
** Name: LOLOCAL.H - Local definitions for the LO compatibility library.
**
** Description:
**      The file contains the local types used by LO and the local 
**      definition of the LO functions.  These are used for manipulating
**      location names.
**
** History:
**      03-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	26-oct-2001 (kinte01)
**	    Update itemlist - retlen is unsigned int (u_i4)
**	06-apr-2003 (abbjo03)
**	    Correct 10/26/01 change above. retlen is the "address of a word",
**	    i.e., pointer to unsigned i2.
**  05-Jul-2005 (fanra01)
**      Bug 114804
**      Consolidate LO status codes into lo.h.
**	27-feb-08 (smeke01) b120003
**	    Add definition of BACKSLASHSTRING for lotoes() function.
**	09-oct-2008 (stegr01/joea)
**	    Replace ITEMLIST by ILE3.
**/

/*
**  Forward and/or External function references to internal LO routines.
*/

FUNC_EXTERN VOID 	LOclean_fab();
FUNC_EXTERN STATUS 	LOcombine_paths();
FUNC_EXTERN VOID 	LOdev_to_root();
FUNC_EXTERN VOID 	LOdir_to_file();
FUNC_EXTERN VOID 	LOfile_to_dir();
FUNC_EXTERN i4		LOparse();
FUNC_EXTERN VOID	LOsearchpath();


/* 
** Defined Constants
*/

/* Constants */

#define		MAXFNAMELEN	9

#define		DOT		ERx(".")
#define		DASH		ERx("-")
#define		CBRK		ERx("]")
#define		OBRK		ERx("[")
#define		CABRK		ERx(">")
#define		OABRK		ERx("<")
#define		COLON		ERx(":")
#define		SEMI		ERx(";")

#define		WILD_V		ERx(";*")
#define		WILD_NTV 	ERx("*.*;*")

#define		BACKSLASHSTRING	ERx("\\")
