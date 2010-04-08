/*
** 1991, 2000 Ingres Corporation
*/
#ifndef DL_H

/*
** dl.h -- Header file for #defines that DL clients will need.
**
**	This header file defines important things (i.e. typedefs and
**	#define's) that DL clients will need to get to the DL interfaces.
**
** History:
**	25-jul-91 (jab)
**		Written
**	16-sep-91 (jefff)
**		Added support for en8_us5
**	10-jab-92 (jefff)
**		Changing to conform to 6.4 CL spec.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	13-feb-2001 (kinte01)
**	    Add DL_RELOC*, DL_MOD_NOT_READABLE & DL_MEM_ACCESS definitions.
**      13-mar-2002 (loera01)
**          Eliminated Unix #ifdef's and set DL_UNLOADING_WORKS to TRUE (1).
**	12-apr-2002 (kinte01)
**	    Correct previous integration - defome should be define
**	    DL_UNLOADING_WORKS was first set to 0 and then set to 1.  It
**	    only needs to be defined once and defined to 1
**      08-jun-2004 (loera01)
**          Added DLload().
**
*/

#define	DL_H
#define	DLnameavail	IIDLnameavail
#define DLcreate        IIDLcreate
#define	DLdelete	IIDLdelete
#define	DLload  	IIDLload
#define	DLprepare	IIDLprepare
#define	DLprepare_loc	IIDLprepare_loc
#define	DLosprepare	IIDLosprepare
#define	DLunload	IIDLunload
#define	DLbind		IIDLbind
#define	DLosunload	IIDLosunload
#define DLcreate_loc	IIDLcreate_loc
#define DLdelete_loc	IIDLdelete_loc

#define DL_DATE_NAME	ERx("!IIdldatecreated")
#define DL_IDENT_NAME	ERx("!IIdlident")
#define DL_IDENT_STRING	ERx("DL module version 2.0")

#define	DL_MAX_MOD_NAME		8

#define DL_LINKCMD	"link"
#define DL_LINKFLAGS    ""
# define DL_LINKCMD_CPP DL_LINKCMD

/*
** flags for DLprepare_loc --
**    how should symbol relocation be handled?
**    these are recommendations only, system may override
*/
#define DL_RELOC_DEFAULT        0
#define DL_RELOC_IMMEDIATE      1
#define DL_RELOC_DEFERRED       2

#define havelinker
#define	DL_LOADING_WORKS	1

#define	DL_UNLOADING_WORKS	1


#define DL_OSLOAD_FAIL  		(E_CL_MASK + E_DL_MASK + 0x12)
#define DL_VERSION_WRONG        	(E_CL_MASK + E_DL_MASK + 0x13)
#define DL_OSUNLOAD_FAIL        	(E_CL_MASK + E_DL_MASK + 0x15)
#define DL_NOT_IMPLEMENTED      	(E_CL_MASK + E_DL_MASK + 0x16)
#define DL_NOMEM      			(E_CL_MASK + E_DL_MASK + 0x1B)
#define	DL_BAD_LOOKUPFCN		(E_CL_MASK + E_DL_MASK + 0x1E)
/* Start new errors here */
#define	DL_NOT_DIR			(E_CL_MASK + E_DL_MASK + 0x20)
#define	DL_DIRNOT_FOUND			(E_CL_MASK + E_DL_MASK + 0x21)
#define	DL_MODULE_PRESENT		(E_CL_MASK + E_DL_MASK + 0x22)
#define	DL_MOD_COUNT_EXCEEDED		(E_CL_MASK + E_DL_MASK + 0x23)
#define	DL_MOD_NOT_FOUND		(E_CL_MASK + E_DL_MASK + 0x24)
#define	DL_LOOKUP_BAD_HANDLE		(E_CL_MASK + E_DL_MASK + 0x25)
#define	DL_FUNC_NOT_FOUND		(E_CL_MASK + E_DL_MASK + 0x26)
#define	DL_MALFORMED_LINE		(E_CL_MASK + E_DL_MASK + 0x27)
#define	DL_UNRECOGNIZED_LINE		(E_CL_MASK + E_DL_MASK + 0x28)
#define DL_MOD_NOT_READABLE		(E_CL_MASK + E_DL_MASK + 0x29)
#define DL_MEM_ACCESS			(E_CL_MASK + E_DL_MASK + 0x2B)

#endif	/* DL_H */
