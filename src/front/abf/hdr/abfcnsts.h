/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include <abrterr.h>

/**
** Name:	abfconsts.h -	ABF Constants Definition File.
**
**	Constants used throughout ABF
**
** History:
**	8/85 (jhw) -- Moved ABF RTS object table names definitions
**		and member sizes to "<abfrts.h>".
**	12-jan-87 (mgw)	Bug # 10079
**		Added ABNULLFRM - error - tried to run a frame with a null ptr.
**		Probably have a procedure with same name as frame.
**      16-Nov-89 (mrelac)
**              Added hp9_mpe defines for 6.2 port.
**      15-Jan-90 (mrelac) (hp9_mpe define)
**              Changed ABIMGEXT define from 'BIN' to '.BIN' for MPE/XL port.
**      24-May-1990 (fredv)
**              IBM RS/6000 C compiler doesn't like the .obj suffix. Changed
**              .obj to .o for ris_us5.
**      28-Jun-91 (mguthrie)
**              Bull compiler has same problem as ris_us5.
**	16-aug-91 (leighb)
**		On IBM/PC (PMFE), abextrac is written in C.  
**		IBM/PC (PMFE) gets .fmc forms.
**		IBM/PC (PMFE) image extension is .exe.
**	15-nov-91 (leighb) DeskTop Porting Change: Image Build
**		IBM/PC (PMFE) image build extension is .exe.
**		default image build extension is .rtt.
**	29-Jul-92 (fredb) hp9_mpe
**		Updated defines for MPE implementation.
**	10-feb-93 (davel)
**		Added INQUIRE_4GL & SET_4GL constants.
**	25-mar-93 (fraser - integrating leighb's 26-jan-93 changes)
**		Desktop porting change: "fmc" file extension used only for
**		DOS, and not for Windows; therefore, it is ifdef-ed on
**		"PMFEDOS" rather than "PMFE".
**      09-feb-95 (chech02)
**              Add rs4_us5 for AIX 4.1.
**	25-may-95 (emmag)
**		Desktop porting changes for OpenINGRES NT and OS/2 ports.
**	10-may-1999 (walro03)
**		Remove obsolete version string bu2_us5, bu3_us5.
**      02-Aug-99 (hweho01)
**              Added ris_u64 for AIX 64-bit platform.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

# define	ABBUFSIZE	256	/* size of buffers for path names, strings, etc. */
# define	ABBIGBUFSIZE	4096	/* size of buffer for create target
					   list done via 'define table' */
# define	ABLOCNUM	4	/* number of locations in buffers */
# define	ABNONE		0
# define	ABAPPNAM	12

/*
** On UNIX and DG, abextract is written in C.  VMS and CMS get macro. (daveb)
**   MPE is also written in C. (fredb)
*/
# ifdef UNIX
#	define	ABEXT_DSL	DSL_C
# else
# ifdef DGC_AOS
#	define	ABEXT_DSL	DSL_C
# else
# if defined(PMFE) || defined(NT_GENERIC)
#	define	ABEXT_DSL	DSL_C
# else
# ifdef hp9_mpe
#	define	ABEXT_DSL	DSL_C
# else
#	define	ABEXT_DSL	DSL_MACRO
# endif
# endif
# endif
# endif

/*
** VMS gets .mar forms.  PMFEDOS gets .fmc forms.  UNIX and CMS get C. (daveb)
**    Actually, all others get C the way this is written. (fredb)
*/

# ifdef VMS
#	define	ABFORM_EXT	"mar"
# else
# ifdef	PMFEDOS
#	define	ABFORM_EXT	"fmc"
# else
#	define	ABFORM_EXT	"c"
# endif /* PMFEDOS */
# endif /* VMS */

/*
** EXECUTE
**	codes for programs which abf knows about
*/
# define	ABVIFRED		1
# define	ABEDITOR		2
# define	ABPRINTER		3
# define	ABQBF			4
# define	ABCRDIR			5
# define	ABLIBCREATE		6
# define	ABLIBADD		7
# define	ABPURGE			8
# define	ABDIRSET		9
# define	ABRW			10
# define	ABSRW			11
# define	ABGBF			12
# define	ABRBF			13
# define	ABDELALL		14
# define	ABSHELL			15
# define	ABGRAPH			16
# define	ABPRINTDEL		17
# define	ABGBFMAN		18
# define	ABOS			19
# define	ABDIRDEL		20

/*
** INQUIRE_4GL/SET_4GL constants.
*/
# define	AB_INQ4GL_invalid		0
# define	AB_INQ4GL_clear_on_no_rows	1

/*
** OBJECT CONSTANTS moved to abfosl.h
*/

# define	ABSTAB		"abf.stb"

/*
** BUG 1546
** Option files were out of order.
**
** History:
**	16-oct-1986 (Joe)
**		Added ABOSLFILE and ABOSQFILE. These are used
**		in a STprintf statement to turn a frame name
**		into a file name for the source file.  On the
**		PC this size is 8.  This is not a good fix.  We
**		should build our own file names so we won't run into
**		OS limits, but this is equivalent to what we do now.
**	8-nov-89 (Mike S) Add listing extention (ABLISEXTENT)
*/
# ifdef CMS
# define	ABFDIR		"ING_ABFDIR"
# define	ABOBJEXTENT	".text"
# define	ABLISEXTENT	".lis"
# define	ABEXTSCR	"abextract.assemble"
# define	ABEXTOBJ	"abextract.text"
# define	ABIMGEXT	".module"
# define	ABCONFILE	"%.9s.con"
# endif

# ifdef MVS
# define	ABFDIR		"ING_ABFDIR"
# define	ABOBJEXTENT	".obj"
# define	ABLISEXTENT	".lis"
# define	ABEXTSCR	"abextrac.asm"
# define	ABEXTOBJ	"abextrac.obj"
# define	ABIMGEXT	".module"
# define	ABCONFILE	"%.9s.con"
# endif

# ifdef UNIX
# define	ABFDIR		"ING_ABFDIR"
#  if defined(any_aix)
# define        ABOBJEXTENT     ".o"
# define        ABEXTOBJ        "abextract.o"
#  else
# define	ABOBJEXTENT	".obj"
# define	ABEXTOBJ	"abextract.obj"
#  endif
# define	ABLISEXTENT	".lis"
# define	ABEXTSCR	"abextract.c"
# define	ABIMGEXT	""
# define	ABCONFILE	"%.9s.con"
# endif

# ifdef MSDOS
# define	ABFDIR		"ING_ABFDIR"
# define	ABOBJEXTENT	".obj"
# define	ABLISEXTENT	".lis"
# define	ABEXTSCR	"abextrac.c"
# define	ABEXTOBJ	"abextrac.obj"
# define	ABIMGEXT	".exe"
# define	ABCONFILE	"%.8s.con"
# define	IMGBLDEXT	"exe"			 
# endif

# ifdef DESKTOP
# define	ABFDIR		"ING_ABFDIR"
# define	ABOBJEXTENT	".obj"
# define	ABLISEXTENT	".lis"
# define	ABEXTSCR	"abextract.c"
# define	ABEXTOBJ	"abextract.obj"
# define	ABIMGEXT	".exe"
# define	ABCONFILE	"%.8s.con"
# define	IMGBLDEXT	"exe"			 
# endif

# ifdef VMS
# define	ABFDIR		"ING_ABFDIR"
# define	ABOBJEXTENT	".obj"
# define	ABLISEXTENT	".lis"
# define	ABEXTSCR	"abextract.mar"
# define	ABEXTOBJ	"abextract.obj"
# define	ABIMGEXT	".exe"
# define	ABCONFILE	"%.31s.con"
# endif

# ifdef DGC_AOS
# define        ABFDIR          "ING_ABFDIR"
# define        ABOBJEXTENT     ".ob"
# define	ABLISEXTENT	".lis"
# define        ABEXTSCR        "abextract.c"
# define        ABEXTOBJ        "abextract.ob"
# define        ABIMGEXT        ".pr"
# define        ABOSLFILE       "%.9s.osl"
# define        ABOSQFILE       "%.9s.osq"
# define        ABCONFILE       "%.9s.con"
# endif

# ifdef hp9_mpe
# define        ABOPTS          "ING_ABFOPT1/OPT,ING_ABFOPT/OPT"
# define        ABFDIR          "ING_ABFDIR"
# define        ABOBJEXTENT     ".O"
# define        ABEXTNAM        "ABEX"
# define        ABEXTSCR        "ABEX.C"
# define        ABEXTOBJ        "ABEX.O"
# define        ABIMGEXT        ".BIN"
# define        ABOSLFILE       "%.8s.OSL"
# define        ABOSQFILE       "%.8s.OSQ"
# define	ABLISEXTENT	".LIS"
# define	ABCONFILE	"%.8s.con"	/* may not be right - frb */
# endif

# ifndef	IMGBLDEXT				 
# define	IMGBLDEXT	"rtt"			 
# endif							 

# ifndef ABFDIR
	Acckkk_Pfffffffft!!!!!! ABFDIR not defined when it should!!!
# endif


/*
** GLOBAL OBJECTS
** in runtime.   Should be unique in 7 characters.
*/
# define	ABHASHSIZE	71
#ifdef CMS
# define	ABEXTNAME	"$IIAB@EXTRACT"
# define	ABEXTCNAME	$IIAB@EXTRACT
# define	ABCTOLANG	"$ABCTOLN"	/* CMS only */
# define	ABCTOCLANG	abctolng	/* CMS only */
#else
# define	ABEXTNAME	"IIAB_Extract"
# define	ABEXTCNAME	IIAB_Extract
#endif

/*
** STATUS VALUES
*/
# define	ABSCOMP		1
# define	ABSLNK		2
# define	ABSLIB		3
# define	ABSINTLNK	4

/*
** HELP
** Help files for all the menus
*/
# define	hlpUNFRM	"abhufrm.hlp"
# define	hlpUNPROC	"abhuproc.hlp"
# define	hlpREPMU	"abhrepmu.hlp"
# define	hlpGBFMU	"abhgbfmu.hlp"
# define	hlpVIGMU	"abhvigmu.hlp"

/*
** Menus
** Constants for all menu operations
*/
# define	ABMUHELP	1
# define	ABMUCREATE	2
# define	ABMUDEFINE	3
# define	ABMUDESTROY	4
# define	ABMUIMAGE	5
# define	ABMUOPTIONS	6
# define	ABMURUN	7
# define	ABMUQBF	8
# define	ABMUQUIT	9
# define	ABMUAPPL	10
# define	ABMUFRAME	11
# define	ABMUPROC	12
# define	ABMURETURN	13
# define	ABMUCHECK	14
# define	ABMUEDIT	15
# define	ABMUPRINT	16
# define	ABMUVIFRED	17
# define	ABMURBF	18
# define	ABMUSREP	19
# define	ABMUGBF	20
# define	ABMUGRAPH	21
# define	ABMUGO	22
# define	ABMUYES	23
# define	ABMUNO	24
# define	ABMUCALL	25
# define	ABMUREP	26
# define	ABMUREL		27
