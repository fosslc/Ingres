/*
** Copyright (c) 2004, 2008 Ingres Corporation
*/

/*
** Filename:	eqfgen.h
** Purpose:	Contains all the OS and machine dependent information 
**		used by the code generator (see forgen.c).
** History:
**		15-jun-1987 (cmr) - Written for 5.0 UNIX FORTRAN 
**		17-aug-89 (sylviap) 
**			Added DG support.
**		05-jan-90 (barbara)
**			Integrated Terryr's changes for f77 and DEC FORTRAN
**			support on ULTRIX machines and his ps2_us5 changes.
**		05-sep-90 (kathryn) 
**			Integration of Porting changes from 6202p code line:
**		21-feb-1990 (terryr)(rudyw integrates from ug)
**			made changes to allow ps2_us5 to support vsfortran
**			compiling in mr1 (necessary) and mvx (optional) modes
**		01-Mar-1990 (fredv)
**			Both rtp_us5 and hp3_us5 allow tab, added entries
**			for them.
**		02-apr-90 (griffin)
**			Added some machine specific code for hp3_us5.
**			Need to define STRICT_F77_STD for hp3_us5.
**		19-apr-90 (davep)
**			Need to define STRICT_F77_STD for hp8_us5
**			and make the length of the line 72 chars.
**		19-apr-90 (kathryn)
**			Added define for MIXEDCASE_FORTRAN for ISC 386/ix.
**		05-jun-90 (bchin)
**			apu_us5, set STRICT_F77_STD and line length to 72.
**		06-jun-90 (thomasm)
**			Need to define STRICT_F77_STD for ris_us5 and
**			make length of line = 72
**		06-jul-90 (fredb)
**			Added cases for HP3000 MPE/XL (hp9_mpe).
**              23-Oct-91 (dchan)
**                      Added ds3_ulx to list of machines that
**                      adhere to the "standard" f77 comment semantics.
**              26-sep-91 (jillb/jroy--DGC) (from 6.3)
**                      Added case for LEN_AFTER_VAR for LPI Fortran
**                      support for dg8_us5.  Also added case for
**                      MIXEDCASE_FORTRAN to fix a problem with
**                      multiply defined symbols when using mixed
**                      case compiles.
**		29-Jul-1992 (fredv)
**			Defined STRICT_F77_STD and MIXEDCASE_FORTRAN for
**			m88_us5 (Motorola Delta 88k) f77 compiler.
**              31-jul-92 (kevinm)
**                      Removed dg8_us5 entry for LEN_AFTER_VAR because Data
**                      General is no longer supporting LPI fortan.
**		22-jul-92 (sweeney)
**			apl_us5 needs STRICT_F77 and G_LINELEN 72
**		02-sep-92 (ricka)
**			VMS compiler cannot handle lines that are continued
**			by using a " \ ".
**              10-Sep-93 (kchin)
**                      Added axp_osf to list of machines that
**                      adhere to the "standard" f77 comment semantics.
**              10-feb-95 (chech02)
**                      Added rs4_us5 for AIX 4.1.
**              18-may-95 (wolf) 
**                      Re-format if statements that were modified for rs4_us5 
**			to avoid use of a continuation character, which causes
**			compiler warnings on VMS.
**		01-feb-96 (morayf)
**			Added rmx_us5 to those needing STRICT_F77 and 72
**			columns defined for Siemens-Nixdorf RMx00 port.
**			Also want MIXEDCASE_FORTRAN (also in iiufsys.h !).
**		02-Nov-98 (hweho01)
**       		Need to define STRICT_F77_STD and make the length of  
**      		the line 72 chars. for EPC F77 compiler (Ver. 2.6.7)  
**                      support on dg8_us5.         
**		10-may-1999 (walro03)
**			Remove obsolete version string apl_us5, apu_us5,
**			ix3_us5, ps2_us5, rtp_us5, vax_ulx.
**              03-jul-1999 (podni01)
**                      Added support for ReliantUNIX V 5.44 - rux_us5
**                      (same as rmx_us5)
**		16-apr-2001 (mcgem01)
**			Added support for Windows 2000.
**		23-apr-2001 (mcgem01)
**			Fortran behaviour on NT more closely resembles
**			VMS than it does UNIX.
**		25-Jun-2008 (hweho01)
**			Defined INC_FILE64 for 64-bit code on hybrid build. 
**              18-Jul-2008 (hanal04) Bug 120531
**                      Define STRICT_F77_STD for LNX and su9. f90 and f95
**                      compilers insist on the continuation character
**                      being in column 6. Without STRICT_F77_STD esqlf
**                      puts the continuation character in column 1.
**              21-Oct-2008 (hanal04) Bug 120531
**                      su9_us5 is not being defined in bzarch.h so
**                      check for su4_us5. Leaving the su9_us5 in place
**                      in case we decide to define su9_us5 on a su9_us5
**                      build.
**		05-Jan-2009 (toumi01) Bug 120531
**			Along with the above change for STRICT_F77_STD for
**			LNX and Solaris we must also set G_LINELEN to 72 to
**			avoid output layouts that fail g77 compilation
**			(e.g. the glf00 and glf01 sep tests).
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**			
*/

/*
** Determine string to use for an include statement.
*/

# ifdef apo_u42
#     define	INCLUDE_STRING	"%INCLUDE '"
# else
	/* VMS and all other UNIX F77 compilers */
#     define	INCLUDE_STRING	"\tinclude '"
# endif

# if defined(LNX) || defined(sparc_sol)
# define STRICT_F77_STD
# endif
# if defined(hp3_us5) || defined(any_hpux)
# define STRICT_F77_STD
# endif
# if defined(hp9_mpe) || defined(ds3_ulx) 
# define STRICT_F77_STD
# endif
# if defined(m88_us5) || defined(axp_osf) 
# define STRICT_F77_STD
# endif
# if defined (rmx_us5) || defined(rux_us5)
# define STRICT_F77_STD
# endif
# if defined (any_aix)
# define STRICT_F77_STD
# endif
# if defined (dg8_us5)
# define STRICT_F77_STD
# endif

# if defined(rmx_us5) || defined(rux_us5)
# define MIXEDCASE_FORTRAN
# endif

/*
** Determine string to use to begin a comment.
** Determine the name of the equel/fortran include file.
*/

# if defined(VMS) || defined(NT_GENERIC)
#     define	BEGIN_COMMENT 	ERx("! ")
#     define 	INC_FILE	ERx("eqdef.for")
# else
#     define	BEGIN_COMMENT	ERx("C ")
#     define 	INC_FILE	ERx("eqdef.f")
#     define 	INC_FILE64	ERx("eqdef64.f")
# endif

/*
** Determine the start of continuation lines for string constants and
** for regular statements.
*/

# ifdef hp9_mpe
#	 define CONT_STR        ERx("\n     1")
#	 define CONT_STATE      ERx("\n     1")
# else
#   ifdef UNIX
#     ifdef STRICT_F77_STD
	 /* ANSI style continuations */
#	 define CONT_STR	ERx("\n     &")
#	 define CONT_STATE	ERx("\n     &")
#     else
	      /* UNIX f77 style continuations */
#	      define CONT_STR	ERx("\n&") 
#	      define CONT_STATE	ERx("\n&\t")
#     endif /* STRICT_F77_STD */
#   else
#     ifdef DGC_AOS
#        define CONT_STR	ERx("\n     1")
#        define CONT_STATE	ERx("\n     1\t") 
#     else
         /* VMS style continuations */
#        define CONT_STR	ERx("\n\t1")
#        define CONT_STATE	ERx("\n\t1") 
#     endif /* DGC_AOS */
#   endif /* UNIX */
# endif  /* hp9_mpe */

/*
** Control breaking of string constants.
** Allow STR_FUDGE extra spaces for possible or '); as the call may require 
*/

# if defined(UNIX) || defined(hp9_mpe)
#     define STR_FUDGE	0
# else
#     define STR_FUDGE	5
# endif

/* Max line length for FORTRAN code, with space for closing calls, etc */


# ifdef hp9_mpe       /* hp3000 mpe/xl */
#     define  G_LINELEN       72
# else
#     ifdef UNIX
#         if defined(hp3_us5) || defined(any_hpux) || defined(any_aix) || \
	     defined(rmx_us5) || defined(dg8_us5) || \
	     defined(rux_us5) || defined(LNX)     || defined(sparc_sol)
#           define   G_LINELEN       72
#         else
#           define   G_LINELEN       71      /* no tabs allowed */
#         endif
#     else
#       define    G_LINELEN       65      /* tab at start only counted as one */
#     endif  /* UNIX */
# endif  /* hp9_mpe */

/*
** Some F77 compilers treat backslash as an escape character, like
** the C compiler.  We must compensate for this when generating
** string constants.  This is a problem area, because may compilers
** behave differently version to version with backslash escapes.
** You're better off avoiding them altogether if you can. (daveb).
*/

# ifdef UNIX
#     ifndef hp3_us5
#         ifndef SEQUENT

#             define F77ESCAPE

#         endif /* SEQUENT */
#     endif /* hp3 */
# endif /* UNIX */

/* Since user's string vars are not null terminated in Fortran, the compiler
** supplies the length of the string var to the called function.  Most com-
** pilers put this length at the end of the arg list.  But some compilers
** (hp 9000/840) put the length immediately following the string var.  
** For this special case, we must generate a "dummy" len for non-string
** vars so that the run-time routines will have a consistent arg list.
*/
# if defined(any_hpux)
#     define LEN_AFTER_VAR
# endif

# ifdef m88_us5
# define MIXEDCASE_FORTRAN
# endif
