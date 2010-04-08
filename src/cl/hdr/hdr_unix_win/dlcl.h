/*
** Copyright (c) 2004 Ingres Corporation
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
**	4-Mar-92 (daveb/jab)
**		Correct prototype for DLprepare.  Handle is a PTR *, not
**		a plain PTR.
**	5-nov-92 (peterw) 
**		Integrated change from ingres63p to added DL_LOADING_WORKS 
**		and DL_UNLOADING_WORKS for ICL DRS platforms
**	19-jan-93 (sweeney)
**		usl_us5 is the generic SVR4.2 from USL.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	9-aug-93 (robf)
**          Add su4_cmw
**      11-feb-94 (ajc)
**              Added hp8_bls specific entries based on hp8*
**	10-feb-95 (chech02)
**		Added rs4_us5 for AIX 4.1.
**	20-mar-95 (smiba01)
**		Added dr6_ues based on dr6_*
**	22-jun-95 (allmi01)
**		Added dgi_us5 support for DG-UX on Intel based platforms 
**		following dg8_us5.
**	03-jun-1996 (canor01)
**		Merge in changes that got put into the erroneously
**		resuscitated cl!hdr!hdr dl.h and again delete said file
**		per 7-jun-93 change above.
**	17-oct-1996 (canor01)
**		Remove 8-character filename limit on DL modules.  Add
**		default linker defines for use by DLcreate.
**	12-dec-1996 (donc)
**	    Set DL loading and unloading as "WORKS" for int_wnt
**	24-mar-1997 (canor01)
**	    Add DLdelete and DLdelete_loc.
**	07-apr-1997 (canor01)
**	    Add DLconstructloc.
**	13-aug-1997 (canor01)
**	    Add support for immediate or delayed reference resolution
**	    if supported.
**	10-feb-1998 (canor01)
**	    Add default value for DL_LINKCMD for NT.
**	13-nov-1998 (canor01)
**	    Add DL_MEM_ACCESS.
**	10-may-1999 (walro03)
**	    Remove obsolete version string dr6_ues, dr6_uv1, dra_us5.
**	27-jul-1999 (schte01)
**       Add DL_LOADING... for axp_osf.
**	10-dec-1999 (mosjo01)
**	    Add DL_LOADING_WORKS and DL_UNLOADING_WORKS for sqs_ptx
**	13-dec-1999 (hweho01)
**	    Add DL_LOADING_WORKS and DL_UNLOADING_WORKS for ris_u64. 
**	14-Jul-2000 (hanje04)
**	    Add DL_LOADING_WORKS and DL_UNLOADING_WORKS for int_lnx. 
**	15-aug-2000 (hanje04)
**	    Add DL_LOADING_WORKS and DL_UNLOADING_WORKS for ibm_lnx
**	08-Sep-2000 (hanje04)
**	    Add DL_LOADING_WORKS and DL_UNLOADING_WORKS for axp_lnx. 
**	08-feb-2001 (somsa01)
**	    Porting changes for i64_win.
**	28-Feb-2001 (musro02)
**	    Add DL_LOADING_WORKS and DL_UNLOADING_WORKS for nc4_us5
**	27-Feb-2001 (wansh01)
**	    Add DL_LOADING_WORKS and DL_UNLOADING_WORKS for sgi_us5. 
**	18-Apr-2001 (bonro01)
**	    Add DL_LOADING_WORKS and DL_UNLOADING_WORKS for rmx_us5. 
**      12-Jun-2001 (legru01)
**          Add DL_LOADING_WORKS and DL_UNLOADING_WORKS for sos_us5. 
**      22-Jun-2001 (linke01)
**          Add DL_LOADING_WORKS and DL_UNLOADING_WORKS for usl_us5 
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	25-oct-2001 (somsa01)
**	    Changed "int_wnt" to "NT_GENERIC".
**      04-Dec-2001 (hanje04)
**          Add DL_LOADING_WORKS and DL_UNLOADING_WORKS for i64_lnx
**	18-Jan-2002 (bonro01)
**	    Backout incorrect ingres25 change that was propagated to main
**	    in change 452764.
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**      19-Apr=2004 (loera01)
**          Added DLload().
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	22-Apr-2005 (bonro01)
**	    Added support for Solaris a64.sol
**	04-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with OSX and add support for Intel OSX.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

#define	DL_H
#define	DLnameavail	IIDLnameavail
#define	DLdelete	IIDLdelete
#define	DLprepare	IIDLprepare
#define	DLprepare_loc	IIDLprepare_loc
#define	DLload    	IIDLload
#define	DLosprepare	IIDLosprepare
#define	DLunload	IIDLunload
#define	DLbind	IIDLbind
#define	DLosunload	IIDLosunload
#define DLcreate	IIDLcreate
#define DLcreate_loc	IIDLcreate_loc
#define DLdelete	IIDLdelete
#define DLdelete_loc	IIDLdelete_loc
#define DLconstructloc  IIDLconstructloc

#define DL_DATE_NAME	ERx("!IIdldatecreated")
#define DL_IDENT_NAME	ERx("!IIdlident")
#define DL_IDENT_STRING	ERx("DL module version 2.0")

/*
** flags for DLprepare_loc --
**    how should symbol relocation be handled?
**    these are recommendations only, system may override
*/
#define DL_RELOC_DEFAULT	0
#define DL_RELOC_IMMEDIATE	1
#define DL_RELOC_DEFERRED	2

# if defined(sparc_sol)
# define havelinker
# define DL_LINKCMD	"ld"
# define DL_LINKFLAGS	"-G"
# define DL_LINKCMD_CPP "CC"
# define DL_LINKCPPLIB "-lC"
# endif /* su4_us5 */

#if defined(any_hpux)
#define havelinker
#define DL_LINKCMD	"ld"
#define DL_LINKFLAGS	"-b"
#endif /* hp8_us5 */

#if defined(NT_GENERIC)
#define havelinker
#define DL_LINKCMD	"link"
#define DL_LINKFLAGS    ""
#endif /* NT_GENERIC */

#ifndef havelinker
#define DL_LINKCMD	"ld"
#define DL_LINKFLAGS	""
#endif

# ifndef DL_LINKCMD_CPP
# define DL_LINKCMD_CPP DL_LINKCMD
# endif

# ifndef DL_LINKCPPLIB
# define DL_LINKCPPLIB ""
# endif

#if defined(any_hpux) || defined(hp3_us5) || defined(hp8_bls)
#define	DL_LOADING_WORKS	1
#define	DL_UNLOADING_WORKS	0
#endif

#if defined(su4_u42) || defined(any_aix) || defined(m88_us5) || \
    defined(nc4_us5) || defined(dg8_us5) || defined(sparc_sol) || \
    defined(sui_us5) || defined(dgi_us5) || \
    defined(su4_cmw) || defined(NT_GENERIC) || defined(axp_osf) || \
    defined(sqs_ptx) || defined(nc4_us5) || \
    defined(rmx_us5) || defined(sgi_us5) || defined(sos_us5) || \
    defined(usl_us5) || defined(LNX) || \
    defined(OSX) || defined(a64_sol)
#define	DL_LOADING_WORKS	1
#define	DL_UNLOADING_WORKS	1
#endif

#if defined(dr6_us5) 
#define DL_LOADING_WORKS        1
#define DL_UNLOADING_WORKS      0 
#endif

#if defined(VMS)
#define	DL_LOADING_WORKS	1
#define	DL_UNLOADING_WORKS	0
#endif

#ifndef DL_UNLOADING_WORKS
#define	DL_UNLOADING_WORKS	0
#endif

#ifndef DL_LOADING_WORKS
#define	DL_LOADING_WORKS	0
#endif


#define DL_OSLOAD_FAIL  (E_CL_MASK + E_DL_MASK + 0x12)
#define DL_VERSION_WRONG        (E_CL_MASK + E_DL_MASK + 0x13)
#define DL_OSUNLOAD_FAIL        (E_CL_MASK + E_DL_MASK + 0x15)
#define DL_NOT_IMPLEMENTED      (E_CL_MASK + E_DL_MASK + 0x16)
#define DL_NOMEM      (E_CL_MASK + E_DL_MASK + 0x1B)
#define	DL_BAD_LOOKUPFCN	(E_CL_MASK + E_DL_MASK + 0x1E)
/* Start new errors here */
#define	DL_NOT_DIR		(E_CL_MASK + E_DL_MASK + 0x20)
#define	DL_DIRNOT_FOUND		(E_CL_MASK + E_DL_MASK + 0x21)
#define	DL_MODULE_PRESENT		(E_CL_MASK + E_DL_MASK + 0x22)
#define	DL_MOD_COUNT_EXCEEDED		(E_CL_MASK + E_DL_MASK + 0x23)
#define	DL_MOD_NOT_FOUND		(E_CL_MASK + E_DL_MASK + 0x24)
#define	DL_LOOKUP_BAD_HANDLE		(E_CL_MASK + E_DL_MASK + 0x25)
#define	DL_FUNC_NOT_FOUND		(E_CL_MASK + E_DL_MASK + 0x26)
#define	DL_MALFORMED_LINE		(E_CL_MASK + E_DL_MASK + 0x27)
#define	DL_UNRECOGNIZED_LINE		(E_CL_MASK + E_DL_MASK + 0x28)
#define	DL_MOD_NOT_READABLE 		(E_CL_MASK + E_DL_MASK + 0x29)
#define	DL_MEM_ACCESS       		(E_CL_MASK + E_DL_MASK + 0x2B)

#endif	/* DL_H */
