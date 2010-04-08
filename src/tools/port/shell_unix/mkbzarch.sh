:
#
# Copyright (c) 2004 Ingres Corporation
#
# This script builds UNIX versions of `bzarch.h,' a file containing
# all configuration dependent definitions and included by <compat.h>.
# It is only valid for INGRES 6.1 and above.  For earlier versions,
# bzarch.h should be created with the "mkhdr" program.
#
# This new version defines only public entities.  Another program,
# mkclconf.sh, builds the cl private cl/cl/hdr/secrets.h file that
# contains definitions that known only to the CL.
#
# The following things are defined in bzarch.h as needed.
#
# Config string machine identifier:
#
#	sun.u42 -> sun_u42
#	3bx_us5 -> x3bx_us5
#
# Obsolescent machine identifiers:
#
#	VAX
#	PYRAMID
#	Power6		(cci tahoe)
#	ELXSI
#	CONCEPT32	(gould PN, not NP)
#	SUN		(really wrong)
#	BURROUGHS
#	CT_MEGA
#	x3B5
#
# Machine/OS common flavors:
#
#	MEGAFRAME	(burroughs or CT)
#	UNIPLUS		(A/UX, BT, etc.)
#	SEQUENT		(Balance or Symmetry)
#	DOMAINOS	(apollo)
#	AIX		(ibm)
#
# Things that don't really change on Unix:
#
#	UNIX		(for all UNIX platforms)
#	BAD_BITFIELDS	(use masks for tid manipulation)
#	GLOBALREF	(extern)
#	GLOBALDEF	(null on non-VMS)
#	NODY		(DY not used)
#	II_DMF_MERGE	(use single server executable)
#
# Arch/compiler specific:
#
#	LITTLE_ENDIAN_INT  (Integers are little-endian)
#	BIG_ENDIAN_INT	(Integers are big-endian)
#	NO_64BIT_INT	(No 64 bit integer in hardware or compiler emulation)
#	IEEE_FLOAT	(might not need extra-CL visibility)
#	FLT_OPEQ_BUF	(common compiler problem)
#	INTERNAT	(obsolete?)
#	NEED_ZERO_FILL	(Must initialize all variables)
#	DUALUBSD	(Dual universe)
#	NODSPTRS	(Can't to 0->element offsetof(element) trick)
#	xHARDMATH	(EXCONTINUE from math exceptions won't work).
#	OS_THREADS_USED	(Operating system threads)
#	SYS_V_THREADS	(SystemV/Unix International/Solaris threads)
#	POSIX_THREADS	(POSIX threads)
#	HAS_VARIADIC_MACROS (Compiler supports variadic macro forms)
#
# Special types:
#
#	READONLY	"const" or ""
#	WSCREADONLY	"const" or ""	(obsolescent)
#	GLOBALCONSTDEF	"const" or ""
#	GLOBALCONSTREF	"extern const" or extern
#
# Alignment/Size related:
#
#	BITSPERBYTE	(better be 8!)
#	ALIGN_I2	(1)
#	ALIGN_I4	(1 | 3)
#	ALIGN_I8	(1 | 3 | 7)
#	ALIGN_F4	(1 | 3)
#	ALIGN_F8	(1 | 3 | 7)
#
#	ALIGN_RESTRICT	(char | short | int | double)
#
# Assignment Macros:
#
#	BA(a,i,b)	Byte aligned move of i bytes from a to b
#	I2_ASSIGN_MACRO
#	I4_ASSIGN_MACRO
#	I8_ASSIGN_MACRO
#	F4_ASSIGN_MACRO
#	F8_ASSIGN_MACRO
#
# Byte ordering, etc:
#
#	TID_SWAP	if "reverse" bitfield ordering
#
# Unsigned char manipulation
#
#	UNS_CHAR	if chars are unsigned only
#	I1_CHECK_MACRO	6.1 flavor of above
#
# Size of pointer, and equivalent sized scalar type.
#	
#	PTR_BITS_64	if pointers are 64 bits
#	SCALARP		scalar type same size as a pointer, usually int or long
#	SCALARP_IS_LONG	if SCALARP is defined to be "long"
#	LP64		if longs and pointers are 64 bits
#
## History:
##	10-oct-88 (daveb)
##		Created from mkhdr.sh
##	13-mar-89 (russ)
##		Added code to define xCL_038_BAD_SETJMP.
##       06-apr-89 (russ) 
##               Moving xCL_036_VTIMES_H_EXISTS to bzarch.h because
##               it is referenced in cl/hdr/hdr/tm.h. 
##	08-may-89 (russ)
##		Set LIBC correctly for AT&T COFF producing compiler on
##		odt_us5.
##	08-may-89 (daveb)
##		VTIMES is obsolete for R6. remove references.
##		No longer support 68010 on sun, so remove forced
##		alignment.
##	31-May-1989 (fredv)
##	     Moved the choice of character set from cm.h to bzarch.h because
##		we need this information in cm.h and dbms.h. This change
##		doesn't affect release 5.
##	6-jun-89 (daveb)
##	    Add xEX_HARDMATH.  Proposal in the works for this to be a 
##	    publically visible part of <ex.h>.  Since I can't figure out
##	    how to put it there, we'll put it here.
##       07-jun-89 (kimman)
##               Added DECstation 3100 (ds3_ulx) ALIGN information, because the
##               kernel fixes up unaligned data and doesn't allow our 'align'
##               tool to trap alignment errors.
##       13-jun-89 (arana)
##	    Define IEEE_FLOAT for Pyramid to take advantage of large exponent.
##	14-jun-89 (seng)
##		Took out "#define AIX" for ps2_us5 
##	21-jun-89 (russ)
##		Don't use "const" on READONLY or WSCREADONLY on ODT
##		because of compiler bug.
##	31-jul-1989 (boba)
##		Change conf to tools/port$noise/conf.
##	04-aug-1989 (boba)
##		Remove LIBCMALLOC - no choice in Release 6.
##	14-aug-1989 (harry)
##	        Add hard #defines for *READONLY for vax_ulx, since ANSI C 
##	        "const"is broken on VAX/ULTRIX 3.0 port.  const works in 
##	        simple C declarations, but has problems being declared as 
##	        an extern with unsigned shorts.
##	14-Aug-1989 (fredv)
##		Took out the define AIX from rtp_us5.
##		Define xCL_038_BAD_SETJMP for rtp_us5.
##	19-sept-89 (twong)
##		add IEEE_FLOAT for Convex architecture
##   	06-Oct-89  (harry)
##		Removed NO_ABFGO and added #define NODY to invariant
##		defines section.  Now that all UNIX releases will 
##		implement an interpretive ABF GO option, the old 
##		NO_ABFGO symbol is no longer valid.
##	27-oct-89 (blaise)
##		Added IEEE_FLOAT for bul_us5 (Bull XPS100)
##	03-nov-89 (rudyw)
##		Added IEEE_FLOAT and FLT_OPEQ_BUG for sur_u42 (Sun 386i)
##	21-nov-89 (pholman)
##		Added dr5_us5, which in its 5.0 guise was icb_us5, and
##		NO_VOID_ASSIGN test.  (For compilers which can't assign
##		pointers to void functions correctly).
##	22-nov-89 (pholman)
##		Added BAD_NEG_I2_CAST test.  This checks that a compiler
##		can handle the negation of a i2 (short) cast value, a
##		bug already seen on Ultrix and two forms of ICL machines.
##	23-nov-89 (swm)
##		Added dr6_us5: IEEE_FLOAT, ALIGN=SPARC and ensure that
##		'sparc' and 'UNICORN' are defined. Also, defined NO_ABFGO.
##	24-nov-89 (swm)
##		Removed reference to NO_ABFGO for dr6_us5 as this define is
##		no longer used.
##	13-dec-89 (seng)
##		Add CSIBM define to ps2_us5 to include IBM char extensions.
##		Add IEEE_FLOAT define for ps2_us5.
##	15-dec-1989 (boba)
##		PG_SIZE is not used by the CL and is redundantly defined
##		for the rest of the INGRES code.  It should no longer
##		exist in bzarch.h.
##	12-jan-90 (blaise)
##		Add new definition, NO_VIGRAPH. This is so that vigraph-
##		specific functions can be ifdef'd out of the source of
##		copyapp for ports which don't have vigraph.
##	12-jan-90 (blaise)
##		Back out previous change.
##	31-jan-90 (twong)
##		take out IEEE_FLOAT for cvx_u42; use native mode instead.
##	05-feb-1990 (swm)
##		Dont define xCL_038_BAD_SETJMP on dr6_us5.
##	07-feb-1990 (swm)
##		Backed out previous change for dr6_us5, also removed
##		defines for "sparc" and "UNICORN" as the associated
##		system compiler/include files have now been fixed.
##	13-mar-1990 (boba)
##		Add GLOBALCONSTDEF for 6.3 (always blank for UNIX).
##		Add II_DMF_MERGE for 6.3 (always defined for UNIX).
##		Add comments for invariant defines above.
##	16-may-1990 (boba)
##		Add comment for DOMAINOS.  Change comments for SPARC
##		and MIPS alignment to indicate it is to foce optimal
##		alignment.
##	23-jul-90 (chsieh)
##		Added IEEE_FLOAT for bu2_us5 (Bull DPX2/210)
##	30-aug-90 (jillb--DGC)
##		Integrating 6202p to 63p:
##		Added IEEE_FLOAT for dg8_us5.
##	30-oct-90 (rog)
##		Added support for the volatile keyword by creating the
##		VOLATILE #define, and made GLOBALCONSTDEF variant depending
##		on compiler support for the const keyword.
##	08-jan-91 (sandyd)
##		Added cases for sqs_ptx (including new "MCT" capability).
##	1-mar-91 (jab)
##		Changed [what might have been] typo for "volatile" test.
##		Now it doesn't need "." in $PATH to work.
##	12-mar-91 (seng)
##		Made changes specific for the RS/6000 (ris_us5).
##		Take const out of defines, it conflicts with a necessary
##		  compile time flag.
##		IBM RS/6000 is an IEEE_FLOAT machine.
##		Use the IBM character set (CSIBM).
##       19-mar-91 (dchan)
##               added check for ds3_ulx for use of "const".  Compiler
##               currently doesn't implement it.
##	21-mar-91 (rudyw)
##		Add odt_us5 to list using CSIBM international char set
##		Add IEEE_FLOAT define within odt_us5 variant case.
##		Add GLOBALCONSTDEF to the odt_us5 variant section to make
##		up for it getting lost in recent change to invariants.
##       21-mar-91 (szeng)
##               Update the test for volatile keyword to cover a more
##               general case, since the platform like vax_ulx do support
##               simple volatile keyword but do not support "typedef
##               volatile struct _bar bar". Also clean some redundant
##		test for "const" support. 
##	25-mar-91 (sandyd)
##		GCF64 is required for 6.3/04 (or is it now called 6.4?).
##		DBMS group had been editing bzarch.h directly.  Now let's
##		get the porting tools in sync.
##       29-apr-91 (dnorth)
##               Add cvx flags 
##       28-may-91 (szeng)
##               Added a missed declared statement in test of volatile
##               keyword left by my 25-mar-91's change.
##	02-jul-91 (johnr)
##		hp3_us5 accepts the const int as just int, but cannot handle
##		external const int.  (i.e. drop const for this box.)
##	18-aug-91 (ketan) 
##		Added nc5_us5. Defined _init as ncr_init as it clashes with 
##		a system variable at link time. Add IEEE_FLOAT and library
##		/usr/ccs/lib/libc.so.
##       12-dec-91 (Sanjive)
##             Add entry for dra_us5 (ICL DRS3000 running SVR4) - libc.a lives
##             in /usr/ccs/lib.
##	19-mar-92 (eileenm)
##		Removed MCT from sqs_ptx
##	06-apr-92 (johnr)
##               Added IEEE_FLOAT,ALIGN=MIPS and
##               READONLY,WSREADONLY GLOBALCONSTDEF,GLOBALCONSTREF extern for
##               ds3_osf..
##	25-mar-92 (sandyd)
##		Reconciled with 6.4 version (ingres63p).  Basically just took
##		the 6.4 version and removed "GCF64" and added "INGRES65".
##	16-oct-1992 (lauraw)
##		Uses new readvers to get config string.
##	30-oct-1992 (lauraw)
##		Changed the "trap '' 0" at the end of this script to "trap 0"
##		because on some platforms the former has no effect. In any
##		case, the latter is correct -- we are *unsetting* the trap
##		for 0, not setting it to be ignored.
##	04-nov-92 (peterw)
##		SIR 45887. Added IEEE_FLOAT for dra_us5. Change integrated 
##		from ingres63p
##	06-nov-92 (bonobo/mikem)
##		su4_us5 port.  Define IEEE_FLOAT and ALIGN=SPARC.
##	16-nov-92 (smc)
##		Added IEEE_FLOAT and defined ALIGN macro values for axp_osf.
##	10-feb-93 (dianeh)
##		Added #define GCF65 to list of invariants; changed History
##		comment block to double pound-signs.
##	16-feb-93 (sweeney)
##		usl_us5: libc is in /usr/ccs/lib. Use IEEE_FLOAT.
##	26-feb-93 (smc)
##		Added axp_osf to the list of boxes that exclude the const
##		declaration.
##	14-apr-93 (vijay)
##		Integrate from ingres63p:
##             take "pre" logic from mksecret.sh. This is the prefix for
##             symbols in the nm output from libc.a. Now, BAD_SETJMP will be
##             defined correctly for ris_us5 and some other boxes.
##	14-apr-93 (vijay)
##		add check to see if float.h exists. Used in clfloat.h.
##       05-mar-93 (jab)
##	   Added support for new VERS/CONFIG scheme for W4GL:  if
##	   "option=XXX"  is in VERS, then "#define conf_XXX" is omitted
##	   --- for the cases we know to look for (OL/X11R4/X11R5)
##	15-mar-93 (jab)
##		Integrated from 63p a change so that for any "option=XXX" in
##		the VERS file, "conf_XXX" appears in the bzarch.h file.
##		Period. (ccpp will notice this when it prepends
##		'bzarch.h' to CONFIG when it runs, also.)
##	14-sep-93 (dwilson)
##		Added "# define LOADDS", an invariant define that 
##		removes a PC-pertinent keyword which only serves
##		to louse up Unix compiles.
##	15-sep-93 (swm)
##		It can no longer be assumed that a pointer is 32 bits or
##		that is is the same same as an int. If pointer size is 64
##		bits define PTR_BITS_64. Define SCALARP, for use within
##		the CL only, to be a scalar type whose size is the same
##		as pointer size; normally, this is expected to be int or
##		long. If SCALARP is long define SCALARP_IS_LONG (useful for
##		defines in some hdr files); again this is for use only
##		within the CL .
##      04-jan-95 (chech02)
##              Added rs4_us5 for AIX 4.1. 
##	09-mar-95 (smiba01)
##		Added changes made in the ingres63p library for dr6_uv1
##		and dr6_ues (ICL secure platforms).
##		The changes from this file in 63p are:
##		(28-apr-92 (sbull))
##		(01-nov-93 (ajc))
##	 7-jun-95 (peeje01)
##		Add check for DoubleByte Character Set (DBCS) enabled port
##		(ie opt=DBL in VERS) and if required #define DOUBLEBYTE
##	15-jun-95 (popri01)
##		Add nc4_us5 entry for: libc, pre, and IEEE FLOAT
##		Add undef HIGHC for nc4, since it is unnecessary
##		for ncr and cause compiler problems in the front-ends
##	17-jul-95 (morayf)
##		Added sos_us5 entries - similiar to odt_us5.
##	25-jul-95 (allmi01)
##		Added support for dgi_us5 (DG-UX on Intel based platforms).
##	31-jul-95 (rambe99)
##		Integrate 6.4 change: add sqs_ptx to platforms not using
##		'const' for GLOBALCONSTREF.
##      26-jul-1995 (pursch)
##              Add changes by kirke from 6405 release 30-apr-92.
##              Add IEEE_FLOAT for pym_us5. libc.a is in a different
##              directory on Pyramid.
##              Need PYRAMID defined for pym_us5.
##              Add changes by erickson from 6405 release 3-dec-94.
##              pym_us5: align as MIPS processor, don't use "const"
##	02-nov-95 (wadag01)
##		SOLARIS x86 TRIAL
##		Temporaryly removed ALIGN=SPARC.
##	10-nov-1995 (murf)
##		Added sui_us5 along side su4_us5. With the above
##		change by wadag01, it may have been wiser to
##		align next to usl_us5, but for now it helps me
##		keep track.
##	18-nov-95 (allst01)
##		Added su4_cmw changes, they seem to have gone missing from
##		the previous port :( should be the same as su4_u42
##	04-Dec-1995 (walro03/mosjo01)
##		(1) added case logic around VOLATILE test because DG/UX 
##		    (dg8_us5, dgi_us5) platforms define __volatile__.
##		(2) added dg8_us5 to platforms where READONLY and its siblings
##		    should be __const__.
##      06-dec-95 (morayf)
##              Added support for SNI RMx00 machines with SINIX (rmx_us5).
##		Also ensured pym_us5 mirrored these changes.
##      23-feb-96 (morayf)
##		Added pym_us5 change to get rid of READONLY (const) stuff. 
##		System has const but protects const objects too rigorously !
##	03-jun-1996 (canor01)
##		Added OS_THREADS_USED for su4_us5 to support native operating
##		system threads.
##	11-nov-1996 (hanch04)
##		Added OS_THREADS_USED for sui_us5 to support native operating
##		system threads.
##	09-jan-1997 (hanch04)
##		Removed OS_THREADS_USED for su4_us5,su4_us5
##	18-feb-1997 (hanch04)
##		Added OS_THREADS_USED for su4_us5,sui_us5 again
##	13-feb-1997 (harpa06)
##		Added FILE_UNIX to build Netscape interface on UNIX
##	06-mar-1997 (toumi01)
##		Added OS_THREADS_USED and POSIX_THREADS for axp_osf.
##	07-apr-1997 (muhpa01)
##		Added OS_THREADS_USED & POSIX_THREADS for hp8_us5
##	29-Apr-1997 (merja01)
##      Added SIMULATE_PROCESS_SHARED for axp_osf.
##	24-apr-1997 (hweho01)
##		Added OS_THREADS_USED via SYS_V_THREADS for UnixWare (usl_us5).
##	07-may-1997 (muhpa01)
##		Added SIMULATE_PROCESS_SHARED & POSIX_DRAFT_4 for hp8_us5.  Also,
##	        added a define for HPUX which is needed in the build of web/insapi
##      21-may-1997 (musro02)
##              Added sqs_ptx to platforms not using 'const' for GLOBALCONSTREF.
##	28-jul-1997 (walro03)
##		Updated for Tandem (ts2_us5).
##      24-sep-1997/12-may-1997 (bonro01/mosjo01)
##		For dg8_us5 and dgi_us5, and for support of native os
##		threads we added OS_THREADS_USED, POSIX_THREADS and 
##		_POSIX4A_DRAFT6_SOURCE. The latter is needed for 
##		expanding file <sys/pthread.h>.
##	24-sep-1997 (kosma01)
##		Added OS_THREADS_USED for rs4_us5.
##      	Added SIMULATE_PROCESS_SHARED for rs4_us5.
##	04-Sep-1997 (bonro01)
##		Added OS_THREADS_USED, POSIX_THREADS for dg8_us5
##		Define _POSIX4A_DRAFT6_SOURCE is also required for
##		dg8_us5 compilation.
##	05-Nov-1997 (hanch04)
##	    Added LARGEFILE64 for su4_us5 if B64 is defined in the VERS file.
##	24-dec-1997 (canor01)
##	    Synchronize changes between Ingres and Jasmine to allow for a
##	    common source.
##	14-Jan-1998 (muhpa01)
##		Add hp8_us5 to list of platforms defining xCL_038_BAD_SETJMP
##	27-Jan-1998 (muhpa01)
##		Turning on POSIX_DRAFT_4 support for hp8_us5 - removed
##		defines for OS threads.
##	16-feb-1998 (toumi01)
##		For Linux (lnx_us5) port add IEEE_FLOAT and LIBC=.
##	26-Mar-1998 (muhpa01)
##		Added settings for hpb_us5: IEEE_FLOAT, OS_THREADS_USED,
##		POSIX_THREADS, HPUX.  Also define alias as hp8_us5.
##	01-Jun-1998 (hanch04)
##		Added INGRESII for Ingres II
##	15-Mar-1998 (allmi01)
##		Added entries for Silicon Graphics port (sgi_us5)
##	18-mar-1998 (fucch01)
##		Added OS_THREADS_USED, POSIX_THREADS, SIMULATE_PROCESS_
##		SHARED for sgi_us5.
##	07-jan-1999 (canor01)
##		Added TNG_EDITION for TNG version ("option=TNG").
##	22-Apr-1998 (hanch04)
##	    Changed name from OpenIngres to Ingres
##	27-Jan_1999 (schte01)
##		Added USE_IDLE_THREAD for axp_osf.
##      19-feb_1999 (matbe01)
##              Add #define USE_IDLE_THREAD for rs4_us5.
##	19-mar-1999 (walro03)
##		Since bitsin now talks about "long:" and "long long:" we must
##		awk for '^long:^ to find the length of long instead of the
##		word 'long'.
##	20-mar-1999 (popri01)
##		Add LARGE FILE support for Unixware (available as of UW 7).
##	26-mar-1999 (muhpa01)
##		Added xCL_NO_STACK_CACHING definition.
##	30-Mar-1999 (kosma01)
##		sgi_us5 supports 64 bit libs, new (elf) 32 bit libs,
##		and old (coff) 32 bit libs. Ingres uses the new 32 bit
##		libs /usr/lib32. For now use the older mips3 instead of
##		whatever is current.
##	14-apr-1999 (somsa01)
##		In the case of MAINWIN, don't define UNIX.
##      30-Apr-1999 (allmi01)
##              Added OS_THREADS_USED,POSIX_THREADS,POSIX_DRAFT_4 for rmx_us5.
##      07-May-1999 (allmi01)
##              Added SIMULATE_PROCESS_SHARED for rmx_us5.
##	10-may-1999 (walro03)
##		Remove obsolete version string apl_us5, bu2_us5,
##		bul_us5, cvx_u42, dr3_us5, dr4_us5, dr5_us5, dr6_ues, dr6_uv1,
##		dra_us5, ds3_osf, odt_us5, nc5_us5, ps2_us5, rtp_us5, vax_ulx,
##		3bx_us5.
##	19-may-1999 (schte01)
##       Added LP64 for axp-osf.
##	18-jun-1999 (schte01)
##       Added cast and assigning macros for axp used to avoid the unaligned
##       accesses that occur as a result of a pointer not be aligned to the
##       boundary of the largest member in the structure. This seems to 
##       be limited to axp_osf. The other 64-bit compilers seem to generate
##       fix-up code without "being told". Note: the reason there is a char
##       casting macro is so these types of extra casts are noted and easily
##       changed in the future.
##      22-jun-1999 (musro02)
##              For nc4_us5, remove undef __HIGHC__
##      18-May-1999 (linke01)
##              Added USE_IDLE_THREAD for sgi_us5.
##      03-jul-1999 (podni01)
##              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
##	06-oct-1999 (toumi01)
##		Change Linux config string from lnx_us5 to int_lnx.
##	03-nov-1999 (toumi01)
##		Add WORKGROUP_EDITION for VERS ("option=WORKGROUP_EDITION").
##		Add DEVELOPER_EDITION for VERS ("option=DEVELOPER_EDITION").
##      08-Dec-1999 (podni01)  
##          	Put back all POSIX_DRAFT_4 related changes; replaced 
##		POSIX_DRAFT_4 with DCE_THREADS to reflect the fact that 
##		Siemens (rux_us5) needs this code for some reason removed 
##		all over the place.
##      10-Jan-2000 (hanal04) Bug 99965 INGSRV1090.
##              Correct rux_us5 changes that were applied using the rmx_us5
##              label.
##	24-feb-2000 (somsa01)
##	    Added LARGEFILE64 for HP if B64 is defined in the VERS file.
##	29-feb-2000 (toumi01)
##		For int_lnx add defines for OS threading.
##	20-apr-2000 (somsa01)
##	    Added other products as alternate product builds.
##	26-apr-2000 (hayke02)
##		Removed TNG (embedded Ingres). This change fixes bug 101325.
##	18-jun-1999 (schte01)
##       Added cast and assigning macros for axp used to avoid the unaligned
##       accesses that occur as a result of a pointer not be aligned to the
##       boundary of the largest member in the structure. This seems to 
##       be limited to axp_osf. The other 64-bit compilers seem to generate
##       fix-up code without "being told". Note: the reason there is a char
##       casting macro is so these types of extra casts are noted and easily
##       changed in the future.
##	19-may-1999 (schte01)
##       Added LP64 for axp-osf.
##	18-jan-2001 (crido01)
##	 Look for the FUSEDLL option and if it exists add a #define FUSEDLL
##	23-feb-2001 (hayke02)
##	 Removed xCL_038_BAD_SETJMP define for hp8_us5 - this prevents
##	 a multitude of sigprocmask() calls from setjmp() and longjmp().
##	 This change fixes bug 103254.
##      23-May-2000 (hweho01)
##              Added changes for AIX 64-bit platform (ris_u64):
##              1) Enabled LARGEFILE64 support
##              2) Fixed the expr error when define ALIGN_F8 by   
##                 providing the value of long type.
##              3) Included long type (8-byte in 64-bit platform)
##                 into the check list when determine the software  
##                 imposed alignments.
##	24-may-2000 (toumi01)
##		Added USE_IDLE_THREAD for int_lnx.
##	15-aug-2000 (somsa01)
##		Added support for ibm_lnx.
##	11-Sep-2000 (hanje04)
##	    Added changes for Alpha Linux (axp_lnx). In addition to int_lnx
##	    stuff also added LARGEFILES64 support.
##      30-Oct-200 (hweho01)
##              Defined the processor alignment POWER64 for AIX PowerPC
##              RS64 platform (ris_u64). 
##    1-feb-99 (stephenb)
##            Add define for LP64 if LP64 is set in VERS.
##	01-dec-2000 (toumi01)
##		For now, remove threading defines for ibm_lnx, until POSIX
##		threading issues of dmfacp looping and stack limits being hit
##		during run_all xa testing are resolved.
##	11-dec-2000 (toumi01)
##		For now, remove threading defines for int_lnx, until POSIX
##		threading issues of lar12 hanging and stack limits being hit
##		during run_all xa testing are resolved:
##			echo "# define OS_THREADS_USED"
##			echo "# define POSIX_THREADS"
##			echo "# define SIMULATE_PROCESS_SHARED"
##			echo "# define USE_IDLE_THREAD"
##	19-Dec-2000 (bonro01)
##		Fixed usage of the LARGEFILE64 support option for all
##		platforms.  This option was intended to be turned on
##		as a VERS file option using the option=B64 flag.
##		NOTE: axp_osf never needs the Ingres LARGEFILE64 flag
##		to be set because it's file api's already use 64 bit
##		parameter by default.
##	22-dec-2000 (toumi01)
##		Replace defines that enable *_lnx OS threading.
##	30-Jan-2001 (wansh01)
##		added LARGEFILE64 support for dgi_us5. 
##              Defined _FILE_OFFSET_BITS == 64 
##      22-Dec-2000 (hweho01)
##              Defined LP64 for ris_u64 platform.
##	12-jun-2001 (somsa01)
##		For C++, define GLOBALREF to be 'extern "C"'.
##	29-may-2001 (devjo01)
##		Remove SIMULATE_PROCESS_SHARED for axp_osf.
##	18-jul-2001 (stephenb)
##		From allmi01 originals, add support for i64_aix
##	23-aug-2001 (toumi01))
##		add explicit ALIGN=IA64 to avoid mkbzarch expr eval problem
##		use xlc_rr as tested compiler instead of cc
##		change nm string prefix to '^.*\|'
##	03-Dec-2001 (hanje04)
##		Added support for IA64 Linux (i64_lnx), mimic int_lnx and
##		IA64
##	11-dec-2001 (somsa01)
##		Added hp2_us5, the 64-bit PA-RISC HP port.
##      28-dec-2001 (huazh01)
##       on DgUx, define _POSIX4_DRAFT_SOURCE in order to compile files invoke 
##       TMhrnow().
##      31-Jan-2002 (hweho01) 
##              Add UNSIGNED_CHAR_IN_HASH for rs4_us5 platform. 
##              It is used to maintain the compatibility with the 
##              existing databases prior to Ingres 2.6 release on AIX   
##              platform.   Otherwise, the upgrade process will fail to     
##              read the system catalog.
##	06-mar-2002 (somsa01)
##		Fixed up defines for su4_us5.
##      21-Mar-2002 (xu$we01)
##              Change "# define _POSIX4A_DRAFT6_SOURCE" to
##              "# define _USING_POSIX4A_DRAFT10" for dgi_us5
##	09-May-2002 (xu$we01)
##		We have problems to use "_USING_POSIX4A_DRAFT10",
##		so change back to use "_POSIX4A_DRAFT6_SOURCE" for
##		dgi_us5.
##	23-May-2002 (hanch04)
##		Run bitsin and bitsin64 for 32/64 hybrid build to have only
##		1 version of bzarch.h
##      25-Jun-2002 (hweho01)
##              Defined LARGEFILE64 for AIX platforms (rs4_us5 and
##              ris_u64).
##	27-Aug-2002 (bonro01)
##		Defined LARGEFILE64 for UnixWare (usl_us5)
##	17-Oct-2002 (bonro01)
##		For sgi_us5, removed SIMULATE_PROCESS_SHARED to use true
##		cross-process semaphores.  Also added LARGEFILE64 to 
##		support larger filesystems for SGI.  And added support for
##		Hybrid 32/64-bit builds in SGI.
##      01-Nov-2002 (hweho01)
##              Added change for AIX 32/64 hybrid build.
##	05-Nov-2002 (hanje04)
## 		Add support for AMD x86_64 Linux and genericise Linux defines
##		where apropriate.
##	11-nov-2002 (somsa01)
##		Implemented stack_caching for HP-UX; removed
##		xCL_NO_STACK_CACHING.
##	14-Nov-2002 (devjo01)
##		Undefined USE_IDLE_THREAD for axp_osf.  Should NEVER
##		be set for any platforms not setting SIMULATE_PROCESS_SHARED,
##		which should NEVER be set for OS'es supporting cross-
##		process mutexes & condition variables.
##      27-Nov-2002  (hweho01)
##              Removed "USE_IDLE_THREAD" for AIX platform.
##	01-May-2003 (yeema01) Sir 110158
##		Added back the #define UNIX for OpenROAD MAINWIN Unix port.
##	13-May-2003 (bonro01)
##		Added support for IA64 HP (i64_hpu).
##	12-Jun-2003 (hanje04) 
##		Define NEED_ZERO_FILL for all Linuxes
##      15-Jul-2003 (hweho01)
##              Moved R_MAINWIN_50 definition from VERS to bzarch.h.  
##	23-Sep-2003 (hanje04)
##		Add Large File Support for Linux
##	6-Jan-2004 (schka24)
##		Add i8 macros
##	20-Apr-04 (gordy)
##	    Added LITTLE_ENDIAN, BIG_ENDIAN and NO_64BIT_INT.
##      30-Apr-2004 (hanje04)
##          Replace BIG_ENDIAN and LITTLE_ENDIAN with BIG_ENDIAN_INT and
##          LITTLE_ENDIAN_INT because the former are already defined in
##          /usr/include/endian.h on Linux.
##	11-Jun-2004 (somsa01)
##	    Cleaned up code for Open Source.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	19-aug-04 (toumi01)
##	    For Linux kernel major versions > 2.4 assume NPTL.
##	30-aug-2004 (stephenb)
##	    bitsin64 is now $ING_TOOLS/bin/lp64/bitsin
##      07-Sep-2004 (devjo01)
##          - Set xCL_SUSPEND_USING_SEM_OPS for Linux platforms supporting
##	27-Oct-2004 (hweho01)
##	    salign64 is now $ING_TOOLS/bin/lp64/salign by using jam on AIX.
##      22-Dec-2004 (nansa02)
##             Changed all trap 0 to trap : 0 for bug (113670).
##	25-Dec-2004 (shaha03)
##	    SIR #113754 Added conditional flags for HP-UX itanium hybrid build.
##	05-Jan-2005 (hanje04)
##	    RedHat 2.4.21EL Kernel supports NPTL so use it if we can.
##	12-Jan-2005 (hanje04)
##	    Make sure kernel is detected properly on Linux
##      26-Jan-2004 (hanje04)
##          FIXME!!!!
##          Hardcode defines usually generated by mathsigs for AMD Linux.
##          mathsigs is failing on SuSE 9.2 pro for AMD64 and just exiting
##          with a SIG_FPE. Looks like glibc bug as it works on other AMD64
##          Linuxes
##	01-Mar-2005 (hanje04)
## 	    SIR 114034
##	    Add support for reverse hybrid build (ADD_ON32)
##	    Add support for any hybrid build on Linux.
##	15-Mar-2005 (bonro01)
##	    Add support for Solaris AMD64 a64_sol
##	28-Mar-2005 (shah03)
##	    Add support for hybrid build on HP-UX itanium. Reverted back the 
##	    changes done for hybrid build.	
##	12-Apr-2005 (bonro01)
##	    Set ALIGN_RESTRICT properly for Hybrid platforms which need
##	    different alignment for 32 and 64-bit executables.
##      18-Apr-2005 (hanje04)
##          Add support for Max OS X (mg5_osx).
##          Based on initial changes by utlia01 and monda07.
##	11-May-2005 (lazro01)
##	    Fix to set ALIGN_RESTRICT in case of Non-Hybrid platforms.
##	30-Jun-2005 (hweho01)
##	    Add I8CAST_MACRO for axp_osf platform.
##	20-Jul-2005 (hanje04)
##	    bash doesn't like quotes around arguments to integer operators.
##	    Fix if statement so that $pad and $hbpad are only checked if
##	    $HB is set.
##	23-Aug-2005 (schka24)
##	    Fix libc defn for Solaris 10, works on earlier ones too.
##	    (So, why do we do this same thing in 4 different places???)
##	26-Aug-2005 (hweho01)
##	    Set ALIGN_RESTRIC for AIX, use single alignment. 
##	14-Sep-2005 (hanje04)
##	    Ammend setting of ALIGN_RESTRIC on AIX, causes shell errors
##	    on Mac and Linux
##	26-Sep-2005 (hanje04)
##	    Correctly define pre for linux so that it picks up xCL_ABS_EXISTS.
##	27-Sep-2005 (sheco02)
##	    Same fix for Sun Solaris so that it picks up xCL_ABS_EXISTS.
##	9-Sep-2005 (schka24)
##	    Rearrange conditionals in alignment code to eliminate shell
##	    warnings in non-hybrid builds.
##	03-Oct-2005 (bonro01)
##	    Define pre for UnixWare so that it greps for abs() properly.
##	13-Oct-2005 (hanje04)
##	    Remove extra 'fi' left by X-integration.
##	25-Oct-2005 (hanje04)
##	    float.h not being picked up on Linux platforms. Make sure it is.
##	07-Nov-2005 (hanje04)
##	    Don't define LARGEFILE64 for Linux, because we already build
##	    with -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE which enables
##	    large file support transparently. Defining LARGEFILE64 can 
##	    confuse things (and does on SuSE SLES 8)
##	15-Aug-2006 (kschendel)
##	    Pick up integer-valued build params from VERS.
##	06-Nov-2006 (hanje04)
##	    SIR 116877
##	    Need to be able to NOT build the GTK code if it's not available
##	    on a given platform. Currently only supported on Linux.
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.
##	21-Mar-2007 (hanje04)
##	    SIR 117044
##	    Don't build GTK or RPM stuff on RPL Linux.
##	23-Mar-2007 (hanje04)
##	    SIR 117985
##	    Add support for 64bit PowerPC Linux (ppc_lnx). As no versions
##	    of Linux currently support internal threads, define 
##	    NO_INTERNAL_THREADS for all Linux platforms
##	30-May-2007 (hanje04)
##	    BUG 119214
##	    Replace continue with : in case statement to stop warnings.
##	16-Jan-2008 (kschendel)
##	    Emit a conf_all_defined preprocessor variable, for default.h
##	    to see.  This will eliminate the massive amounts of
##	    "duplicate definition" messages from ccpp'ed targets, which
##	    first see bzarch.h and then default.h.
##	08-Feb-2008 (hanje04)
##	    SIR S119978
##	    Add suport for Intel Mac OS X
##	23-Feb-2008 (hanje04)
##	    SIR 119978
##	    Replace mac_osx with mg5_osx
##      24-Sep-2008 (macde01)
##          BUG 120944
##          Disable GTK support for Debian.
##	28-Oct-2008 (jonj)
##	    SIR 120874: Add test for variadic macro support (ISO 99), emit
##	    HAS_VARIADIC_MACROS.
##	31-Oct-2008 (jonj)
##	    Fix typos in compiled variadic macro test code.
##      12-dec-2008 (joea)
##          Remove BYTE_SWAP (use BIG/LITTLE_ENDIAN_INT instead).
##	11-May-2009 (kschendel) b122041
##	    Remove notice about SCALARP only being allowed in the CL.
##	    That notion is anti-portability, because SCALARP (intptr_t)
##	    is needed to cast between pointers and i4's on LP64 without
##	    compiler size-mismatch warnings.
##      15-May-2009 (frima01)
##          Enable const type for rs4_us5.
##	17-Jun-2009 (kschendel) SIR 122138
##	    Mods for new hybrid scheme (build_arch).  Eliminate the trolling
##	    through libc for symbols, use trial compilation instead.
##	    (Actually, I'm sure that most of it could just be ditched.)
##	28-Jul-2009 (hweho01) SIR 122138
##	    Fix the error that was introduced by the change on 17-Jun-2009,    
##	    the RAAT_SUPPORT should be defined if BUILD_ARCH32 is set on
##	    Solaris platforms.
##	16-Sep-2009 (hanje04)
##	   Remove UNSIGNED_CHAR_IN_HASH for OSX as it was added for PPC and
##	   is only relavent in upgrades from earlier Ingres releases which
##	   didn't exist on this platform
##	09-Okt-2009 (frima01) SIR 122138
##	   Replace non-existent BUILD_BITS32 by BUILD_ARCH32 to enable raat
##	   support for 32bit installations on AIX and HP-UX.

header=bzarch.h
date=`date`

# create header with correct permissions
touch $header
chmod 664 $header

# stdout into newly created header

exec > $header

# don't leave it around if we have trouble

trap 'rm $header' 0

# version file generated by running the source transfer script
# generated by mkreq.sh.  GV_VER and GV_ENV used in gver.roc.

. readvers
vers=$config
CC=cc

product="Ingres"
machine=`machine`
whatunix=`whatunix`
 
# create a new target:

cat << !
/*
** $product $header created on $date
** by the $0 shell script
*/

!

if [ -n "$build_arch" ] ; then
    # define config32 or config64 depending on command line supplied
    # BUILD_ARCH32 or BUILD_ARCH64, which the platform jamdefs must
    # always arrange for if hybrid capable.  If we don't see either,
    # it's probably ccpp or something, and assume the primary build arch.
    # Also define conf_BUILD_ARCH_32_64 or similar for c/ccpp stuff that
    # needs to know whether it was a hybrid build
    echo '#if ! (defined(BUILD_ARCH32) || defined(BUILD_ARCH64))'
    if [ "$build_arch" = '32' -o "$build_arch" = '32+64' ] ; then
	echo '# define BUILD_ARCH32'
    else
	echo '# define BUILD_ARCH64'
    fi
    echo '#endif'
    echo '#if defined(BUILD_ARCH32)'
    echo "# ifndef $config32"
    echo "# define $config32"
    echo "# endif"
    echo "#else"
    echo "# ifndef $config64"
    echo "# define $config64"
    echo "# endif"
    echo "#endif"
    x=`echo $build_arch | tr '+' '_'`
    echo "#define conf_BUILD_ARCH_$x"
else
    # Not hybrid capable, just define config
    echo "# ifndef $config"
    echo "# define $config"
    echo "# endif"
fi

# Generic OS-related symbols to make conditional compilation easier.

# Any HP/UX (hp8, hpb, hp2, i64_hpu)
# (OS-)Threaded HP/UX (excludes hp8)
if [ "$config" = 'hp8_us5' -o "$config32" = 'hpb_us5' -o "$config32" = 'i64_hpu' ] ; then
    echo "#define any_hpux"
    if [ "$config" != 'hp8_us5' ] ; then
	echo "#define thr_hpux"
    fi
fi

# Any SPARC Solaris.  Note that this does not include AMD/Intel Solaris.
# It's possible that there should be a generic "solaris" symbol, we'll see.
if [ "$config64" = 'su9_us5' ] ; then
    echo "#define sparc_sol"
fi

# Any AIX.  At present (Jun 2009) that pretty much means IBM Power.
# AIX Itanium never got off the ground.
if [ "$config32" = 'rs4_u5' ] ; then
    echo "#define any_aix"
fi

#
# obsolete machine identifiers (predate use of config string)
#
# These defines are mostly rehashing of the machine ID defined by the
# cpp.  They should eventually expire as the references to them in the
# code are changed to #ifdef's keyed either on some capability determined
# automatically below or on the config string.
#

case $vers in
vax_*)		echo "# define VAX"		;;
rmx_us5|rux_us5)		echo "# define PYRAMID"		;;
esac

for o in $opts
do
	echo "# define conf_$o"
#
#       Define FUSEDLL if this is a fused OpenROAD build
#
	if [ "$o" = "FUSEDLL" ]
        then
    	    echo '# define FUSEDLL'
	fi
done

for o in $conf_params
do
	eval x='$conf_'$o
	echo "# define conf_$o $x" 
done
# Define a symbol so that ccpp / default.h won't try to
# define all the conf_xxx stuff again.  That annoys the
# preprocessor.
echo "# define conf_all_defined"

HB=""
# Hybrid build?
if [ "$build_arch" = '32+64' ]
then
    HB=64
    RB=32
    LPHB=lp64
elif  [ "$build_arch" = '64+32' ]
then
    HB=32
    RB=64
    LPHB=lp32
fi

# Define sizing things:
#
#	LP64		Longs-Pointers-64-bit model
#	PTR_BITS_64	If pointers are 64 bits
#	SCALARP		integer type with pointer size
#	SCALARP_IS_LONG	an old way of saying LP64
#
# wrapped in command line defined BUILD_ARCH test if [reverse] hybrid build.

if [ -n "$HB" ] ; then
    echo "# if defined(BUILD_ARCH${HB})"
    $ING_TOOLS/bin/lp${HB}/bitsin | awk '
/^ptr/ {
    ptrsize = $2
}
/^long:/ {
    longsize = $2
}
/^int/ {
    intsize = $2
}
END {
    if (ptrsize == 64)
    {
	print "# define PTR_BITS_64"
	if (ptrsize == longsize)
	    print "# define LP64"
    }
    print "# undef SCALARP"
    if (ptrsize == intsize)
    {
	print "# define SCALARP int"
    }
    else
    {
	if (ptrsize == longsize)
	{
	    print "# define SCALARP long"
	    print "# define SCALARP_IS_LONG"
	}
	else
	{
	    print "/* SCALARP MUST be defined, please fix in mkbzarch.sh"
	}
    }
}'
    echo "# else /* BUILD_ARCH${RB} */"
fi

#
# It can no longer be assumed that the size of a pointer is the same size
# as an int. SCALARP is a scalar type whose size is the same as the size of
# a pointer; it is expected that SCALARP will usually be int or long. If it
# is long, define SCALARP_IS_LONG (useful for #defines in hdr files).
#
# It can no longer be assumed that the size of a pointer is 32 bits as this
# can affect the size of messages (eg. in GCA). If pointer size is 64 bits
# define PTR_BITS_64.
#
bitsin | awk '
/^ptr/ {
    ptrsize = $2
}
/^long:/ {
    longsize = $2
}
/^int/ {
    intsize = $2
}
END {
    if (ptrsize == 64)
    {
	print "# define PTR_BITS_64"
	if (ptrsize == longsize)
	    print "# define LP64"
    }
    if (ptrsize == intsize)
    {
	print "# define SCALARP int"
    }
    else
    {
	if (ptrsize == longsize)
	{
	    print "# define SCALARP long"
	    print "# define SCALARP_IS_LONG"
	}
	else
	{
	    print "/* SCALARP MUST be defined, please fix in mkbzarch.sh"
	}
    }
}'

if [ -n "$HB" ]
then
    echo "# endif /* BUILD_ARCH */"
fi

#
#	Essentially invariant defines.
#

cat << end_of_invariants
# if !defined(MAINWIN) || defined(conf_W4GL)
# define UNIX
# endif
# define BAD_BITFIELDS
# ifdef __cplusplus
# define GLOBALREF extern "C"
# else  /* __cplusplus */
# define GLOBALREF extern
# endif  /* __cplusplus */
# define GLOBALDEF
# define NODY
# define II_DMF_MERGE
# define INGRES65
# define GCF65
# define LOADDS
# define FILE_UNIX
end_of_invariants

#
# other variants
#
# Currently, these capabilities are not determined automatically.
#
# If you make a special case for your machine, be sure to duplicate
# all the defines from the generic *) case.
#
#	NO_64BIT_INT	No 64 bit integer in hardware or compiler emulation)
#	IEEE_FLOAT	has ieee conforming floating point
#	FLT_OPEQ_BUG	can't say "float1 op= float2"
#	INTERNAT	internationalization code
#	NEED_ZERO_FILL	ZERO_FILL must be "= {0}"
#	DUALUBSD	Dual universe with most of the port built in BSD and
#			a veneer of Sys V
#	MCT		Multiple Concurrent Threads (sqs_ptx only, for now)
#	OS_THREADS_USED	Operating System Threads (posix or sysV)
#	SYS_V_THREADS	SystemV/Unix International/Solaris threads
#	POSIX_THREADS	POSIX threads
#	SIMULATE_PROCESS_SHARED 
#			Simulate the process-shared attribute for POSIX
#			THREAD machines that do not support sharing
#			mutexes and condition variables between processes.
#			POSIX_THREADS_PROCESS_SHARED. missing on rs4_us5
#	LARGEFILE64	Support 64 bit file system access.
#	RAAT_SUPPORT	Support for obsolescent Record at a Time interface.
#			Restricted to 32 bit HP, SUN, AIX, NT, and Tru64.
#	NO_INTERNAL_THREADS
#			Ingres Internal threading is not supported or has
#			not been enabled for this platform

case $vers in
sqs_ptx)	echo "# define IEEE_FLOAT"
		;;
sqs_*)		echo "# define IEEE_FLOAT"
		;;
sui_us5|\
su4_us5|\
su9_us5|\
a64_sol)	echo "# define IEEE_FLOAT"
                echo "# define OS_THREADS_USED"
                echo "# define SYS_V_THREADS"
		echo "# define LARGEFILE64"
		if [ "$vers" = 'a64_sol' ] ; then
		    echo "# define NO_INTERNAL_THREADS"
		fi
		if [ "$vers" = 'sui_us5' ] ; then
		    echo "# define RAAT_SUPPORT"
		elif [ "$config32" = 'su4_us5' ] ; then
		    echo "# if defined(BUILD_ARCH32)"
		    echo "# define RAAT_SUPPORT"
		    echo "# endif"
		fi
		;;
i64_hpu)	echo "# define IEEE_FLOAT"
		echo "# define HPUX"
		echo "# define OS_THREADS_USED"
		echo "# define POSIX_THREADS"
		;;
hpb_us5|\
hp2_us5)	echo "# define IEEE_FLOAT"
		echo "# define HPUX"
		echo "# define OS_THREADS_USED"
		echo "# define POSIX_THREADS"
		## hybrid capable so build-bits will be defined
		echo "# if defined(BUILD_ARCH32)"
		echo "# define RAAT_SUPPORT"
		echo "# endif"
		;;
hp8_us5)	echo "# define IEEE_FLOAT"
		echo "# define HPUX"
		echo "# define RAAT_SUPPORT"
		;;
sos_us5)	NEED_ZERO_FILL=true
		echo "# define IEEE_FLOAT"
		;;
rmx_us5)        echo "# define IEEE_FLOAT"
                ;;
rux_us5)        echo "# define IEEE_FLOAT"
                echo "# define OS_THREADS_USED"
                echo "# define POSIX_THREADS"
                echo "# define DCE_THREADS"
                echo "# define SIMULATE_PROCESS_SHARED"
		echo "# define USE_IDLE_THREAD"
		;;
sgi_us5)	echo "# define IEEE_FLOAT"
            	echo "# define OS_THREADS_USED"
            	echo "# define POSIX_THREADS"
                echo "# define LARGEFILE64"
		;;
ds3_ulx)        echo "# define IEEE_FLOAT"
		;;
dr6_us5)	echo "# define IEEE_FLOAT"
		;;
dg8_us5 | dgi_us5)	echo "# define IEEE_FLOAT"
                echo "# define OS_THREADS_USED"
                echo "# define POSIX_THREADS"
                echo "# define _POSIX4A_DRAFT6_SOURCE"
	        echo "# define _POSIX4_DRAFT_SOURCE"	
                ;;
r64_us5|\
rs4_us5)	echo "# define IEEE_FLOAT"
                echo "# define OS_THREADS_USED"
                echo "# define POSIX_THREADS"
        	echo "# define UNSIGNED_CHAR_IN_HASH"
		echo "# define LARGEFILE64"
		## hybrid capable so build-bits will be defined
		echo "# if defined(BUILD_ARCH32)"
		echo "# define RAAT_SUPPORT"
		echo "# endif"
		## R_MAINWIN_50 not needed any more??
		;;
ris_u64)	echo "# define IEEE_FLOAT"
                echo "# define OS_THREADS_USED"
                echo "# define POSIX_THREADS"
        	echo "# define UNSIGNED_CHAR_IN_HASH"
		echo "# define LARGEFILE64"
		;;
ris_us5)	echo "# define IEEE_FLOAT"
		;;
nc4_us5)        echo "# define IEEE_FLOAT"
		;;
axp_osf)        echo "# define IEEE_FLOAT"
		echo "# define OS_THREADS_USED"
		echo "# define POSIX_THREADS"
		echo "# define RAAT_SUPPORT"
    echo "# define CHCAST_MACRO(a) (((char *)(a))[0])"
    echo "# define ASSIGN_CH_MACRO(s,d) (CHCAST_MACRO(d) = CHCAST_MACRO(s))"
    echo "# define I2CAST_MACRO(a) (*(i2*)(a))"
    echo "# define ASSIGN_I2_MACRO(s,d) (I2CAST_MACRO(d) = I2CAST_MACRO(s))"
    echo "# define I4CAST_MACRO(a) (*(i4*)(a))"
    echo "# define ASSIGN_I4_MACRO(s,d) (I4CAST_MACRO(d) = I4CAST_MACRO(s))"
    echo "# define F4CAST_MACRO(a) (*(f4*)(a))"
    echo "# define F8CAST_MACRO(a) (*(f8*)(a))"
    echo "# define I8CAST_MACRO(a) (*(i8*)(a))"
                ;;
ts2_us5)        echo "# define IEEE_FLOAT"
                ;;
usl_us5)        echo "# define IEEE_FLOAT"
                echo "# define OS_THREADS_USED"
                echo "# define SYS_V_THREADS"
                echo "# define LARGEFILE64"
		;;
int_lnx|\
int_rpl|\
ibm_lnx|\
axp_lnx|\
i64_lnx|\
ppc_lnx|\
a64_lnx)        NEED_ZERO_FILL=true
		NPTL=false
		kmaj=`uname -r | awk -F. '{ print $1 $2 }'`
		kmin=`uname -r | cut -d. -f3 | cut -d- -f1`
		if [ $kmaj -gt 24 ] || \
		     ( [ $kmaj -eq 24 ] && [ "`uname -r | grep EL`" ] )
		then
		    NPTL=true
		fi
		echo "# define IEEE_FLOAT"
		echo "# define OS_THREADS_USED"
		echo "# define POSIX_THREADS"
		$NPTL || echo "# define SIMULATE_PROCESS_SHARED" 
		$NPTL || echo "# define USE_IDLE_THREAD"
		echo "# define NO_INTERNAL_THREADS"
		echo "# define LNX"
		;;
  *_osx)	echo "# define OSX"
		echo "# define IEEE_FLOAT"
		echo "# define OS_THREADS_USED"
		echo "# define POSIX_THREADS"
		echo "# define NO_INTERNAL_THREADS"
	   	# xprocsem=`getconf POSIX_THREAD_PROCESS_SHARED`
		# disable X process semaphores for now, they don't work
		xprocsem=""
		if [ X"$xprocsem" = X ] || [ "$xprocsem" = "undefined" ]
		then
		    echo "# define SIMULATE_PROCESS_SHARED"
		    echo "# define USE_IDLE_THREAD"
		fi
		;;

esac

if [ -n "$NEED_ZERO_FILL" ]
then echo "# define ZERO_FILL = {0}"
else echo "# define ZERO_FILL"
fi

#
# NODSPTRS - can we deference a null structure pointer at compile time?
#
(
	cd /tmp
	echo '
		struct a { int a,b; };
		int c = (int)&(((struct a *)0)->b);
	' > npr.c
	$CC -c npr.c > /dev/null 2>&1
) || echo '# define NODSPTRS'
rm -f /tmp/npr.[co]
#
# BAD_NEG_I2_CAST - check that the compiler can handle the negation
#	of a i2 (short) cast value.  (This is known to effect more
#	compilers than one would imagine!).
#
(
	cd /tmp
	echo '
# define ONE    (i2)1
typedef short i2;
i2 minusone = -ONE;
' > nocast.c
	$CC -c nocast.c > /dev/null 2>&1
) || echo '# define BAD_NEG_I2_CAST'
rm -f /tmp/nocast.[co]
#
# NO_VOID_ASSIGN - can we assign pointers to functions of type 'void' ?
#
(
	cd /tmp
	echo '
void
func()
{
    int nothing;
}

main()
{
    void(*assign)();
    assign = func;
}
' > novoid.c
	$CC -c novoid.c > /dev/null 2>&1
) || echo '# define NO_VOID_ASSIGN'
rm -f /tmp/novoid.[co]
#
# READONLY - can we use const type with the compiler?
# Need to have a special case for ris_us5 because of compiler flag
# conflicts per fredv. (seng)
#
case $vers in
ds3_ulx|ris_us5|axp_osf|sqs_ptx|ris_u64)
	echo "# define READONLY"
	echo "# define WSCREADONLY"
	echo "# define GLOBALCONSTDEF"
	echo "# define GLOBALCONSTREF extern"
	;;
dg8_us5|dgi_us5)
        echo "# define READONLY __const__"
        echo "# define WSCREADONLY __const__"
        echo "# define GLOBALCONSTDEF __const__"
        echo "# define GLOBALCONSTREF extern __const__"
        ;;
*)
	if (
		cd /tmp
		echo '
			const int x = 5;
		' > const.c
		$CC -c const.c > /dev/null 2>&1
	)
	then
		echo "# define READONLY	const"
		echo "# define WSCREADONLY const"
		echo "# define GLOBALCONSTDEF const"
		echo "# define GLOBALCONSTREF extern const"
	else
		echo "# define READONLY"
		echo "# define WSCREADONLY"
		echo "# define GLOBALCONSTDEF"
		echo "# define GLOBALCONSTREF extern"
	fi
	rm -f /tmp/const.[co]
	;;
esac

# The following tests whether the volatile keyword is supported
# by the compiler.  The volatile keyword prevents some code from
# being moved when it shouldn't be.
case $vers in
dg8_us5|dgi_us5)
        echo "# define VOLATILE __volatile__"
        ;;
*)
	if (
		cd /tmp
		echo '
main()

{
      typedef volatile struct _bar bar;
      struct _bar
      {
              int test;
      };
        bar barvar;
        barvar.test = 0;
        return (barvar.test);
}
' > volatile.c
($CC -o volatile volatile.c) > /dev/null 2>&1 && `./volatile`
	)
	then
		echo "# define VOLATILE volatile"
	else
		echo "# define VOLATILE"
	fi
	rm -f /tmp/volatile*
	;;
esac

#
# Check to see if __setjmp exists.  We check here because the define
# is needed in the header file ex.h.
#

if [ "$vers" = "hp8_us5" ] || [ "$vers" = "rtp_us5" ]
then
    echo "# define xCL_038_BAD_SETJMP"
else
    tmpfile=/tmp/trial$$
    rm -f ${tmpfile}*
    echo 'main(){_setjmp();}' >${tmpfile}.c
    $CC ${tmpfile}.c -o ${tmpfile} >/dev/null 2>/dev/null  || \
	echo "# define xCL_038_BAD_SETJMP"
    rm -f ${tmpfile}*
fi

#
# Determine the _SC_PAGESIZE from sysconf(). Used to define ME_MPAGESIZE
# so that valloc() used instead of brk()/sbrk() on systems which don't
# support memalign. 
#
if [ "$vers" = "mg5_osx" ] || [ "$vers" = "int_osx" ] ; then
    tmpfile=/tmp/scpgsz$$
    cat << EOF > ${tmpfile}.c
# include <unistd.h>
# include <stdio.h>
# include <errno.h>

int
main()
{
    int scpgsz = 0L;

    scpgsz=(int)sysconf(_SC_PAGESIZE);

    if ( errno == 0 )
	return(printf("%dL",scpgsz));
    else
    {
	printf("sysconf() failed, errno = %d",errno);
	return(-1);
    }
}
EOF
    $CC ${tmpfile}.c -o ${tmpfile} >/dev/null 2>&1 && \
	echo "# define SC_PAGESIZE `${tmpfile}`"
fi
#
# heterogeneous processors and alignment override
#
# This section is used to overide automatic determination of capabilities
# on machines distributed with different processors (e.g. processor may be
# either 68010 or 68020) or where our "align" tool is not able to
# correctly determine the optimal alignment requirements.
#
#	ALIGN=68010	Force 68010 alignment.
#	ALIGN=SPARC	Force Sun SPARC optimal alignment.
#	ALIGN=MIPS	Force MIPS optimal alignment.
#	ALIGN=AXP	Force Alpha optimal alignment.
#       ALIGN=POWER64   Force AIX PowerPC 64-bit optimal alignment.
#

case $vers in
brm_us5|alt_u42|btf_us5|dgc_us5|apo_u42)	ALIGN=68010	;;
su4_u42|su4_cmw|dr6_us5|su4_us5|su9_us5)	ALIGN=SPARC	;;
ds3_ulx|rmx_us5|ts2_us5|sgi_us5|rux_us5)	ALIGN=MIPS      ;;
axp_osf)						ALIGN=AXP	;;
ris_u64|rs4_us5)                                ALIGN=POWER64   ;;
i64_lnx)                                                ALIGN=IA64   ;;
esac

#	Evaluation section.  Automatically determined defines.
#

#
# Integer storage format
#
endian_type=`endian`
case $endian_type in
little)	echo "# define LITTLE_ENDIAN_INT"
	;;
big)	echo "# define BIG_ENDIAN_INT"
	;;
esac

#
# bits in a byte
#
if [ -f $ING_SRC/bin/bitsin ]
then
echo "# define BITSPERBYTE	"`bitsin char | awk '{print $2}'`
else
echo "# define BITSPERBYTE 8"
fi

#
# If specified above, use overriding alignment.
# Otherwise, determine alignment by trial & error, capturing faults:
#
case $ALIGN in
SPARC)	ALIGN_I2=2 ALIGN_I4=4 ALIGN_I8=8 ALIGN_F4=4 ALIGN_F8=8 ;;
68010)	ALIGN_I2=2 ALIGN_I4=4 ALIGN_I8=4 ALIGN_F4=4 ALIGN_F8=4 ;;
MIPS)   ALIGN_I2=2 ALIGN_I4=4 ALIGN_I8=8 ALIGN_F4=4 ALIGN_F8=8 ;;
AXP)    ALIGN_I2=2 ALIGN_I4=4 ALIGN_I8=8 ALIGN_F4=4 ALIGN_F8=8 ;;
POWER64)    ALIGN_I2=2 ALIGN_I4=4 ALIGN_I8=8 ALIGN_F4=4 ALIGN_F8=8 ;;
IA64)   ALIGN_I2=2 ALIGN_I4=4 ALIGN_I8=8 ALIGN_F4=4 ALIGN_F8=8 ;;
*)      eval `align | awk 'BEGIN{
            v["char"]=1;
	    v["long"]=4;
            n["short"]="I2"; v["short"]=2;
            n["int"]="I4"; v["int"]=4;
            n["float"]="F4"; v["float"]=4;
            n["double"]="F8"; v["double"]=8;
        }
        n[$1] != "" {printf "ALIGN_%s=%s ", n[$1], v[$4];}
	END {printf "ALIGN_I8=8"}'`
        ;;
esac

echo "# define	ALIGN_I2	`expr $ALIGN_I2 - 1`"
echo "# define	ALIGN_I4	`expr $ALIGN_I4 - 1`"
echo "# define	ALIGN_I8	`expr $ALIGN_I8 - 1`"
echo "# define	ALIGN_F4	`expr $ALIGN_F4 - 1`"
echo "# define	ALIGN_F8	`expr $ALIGN_F8 - 1`"

#
# Determine the software imposed alignments
#
eval `salign | awk 'BEGIN{
        n["short:"]="I2";
        n["int:"]="I4";
        n["float:"]="F4";
        n["double:"]="F8";
        n["long:"]="I8";
        }
n[$1] != "" {printf "SALIGN_%s=%s ", n[$1], $5;}'`
if [ -n "$HB" ] ; then
eval `$ING_TOOLS/bin/$LPHB/salign | awk 'BEGIN{
        n["short:"]="I2";
        n["int:"]="I4";
        n["float:"]="F4";
        n["double:"]="F8";
        n["long:"]="I8";
        }
n[$1] != "" {printf "SALIGN_HB_%s=%s ", n[$1], $5;}'`
fi

if [ $ALIGN_I4 != 1 ] && echo "# define BYTE_ALIGN"
then
echo "# define BA(a,i,b)	(((char *)&(b))[i] = ((char *)&(a))[i])"
echo "# define I2ASSIGN_MACRO(a,b) (BA(a,0,b), BA(a,1,b))"
echo "# define I4ASSIGN_MACRO(a,b) (BA(a,0,b), BA(a,1,b), BA(a,2,b), BA(a,3,b))"
echo "# define I8ASSIGN_MACRO(a,b) (BA(a,0,b), BA(a,1,b), BA(a,2,b), \\"
echo "		BA(a,3,b), BA(a,4,b), BA(a,5,b), BA(a,6,b), BA(a,7,b))"
echo "# define F4ASSIGN_MACRO(a,b) (BA(a,0,b), BA(a,1,b), BA(a,2,b), BA(a,3,b))"
echo "# define F8ASSIGN_MACRO(a,b) (BA(a,0,b), BA(a,1,b), BA(a,2,b), \\"
echo "		BA(a,3,b), BA(a,4,b), BA(a,5,b), BA(a,6,b), BA(a,7,b))"
else
echo "# define I2ASSIGN_MACRO(a,b) ((*(i2 *)&b) = (*(i2 *)&a))"
echo "# define I4ASSIGN_MACRO(a,b) ((*(i4 *)&b) = (*(i4 *)&a))"
echo "# define I8ASSIGN_MACRO(a,b) ((*(i8 *)&b) = (*(i8 *)&a))"
echo "# define F4ASSIGN_MACRO(a,b) ((*(f4 *)&b) = (*(f4 *)&a))"
echo "# define F8ASSIGN_MACRO(a,b) ((*(f8 *)&b) = (*(f8 *)&a))"
fi

#  Align to larger of F8 or I8 padding
pad=$SALIGN_F8
if [ $SALIGN_I8 -gt $SALIGN_F8 ] ; then
   pad=$SALIGN_I8
fi
padhb=$SALIGN_HB_F8
if [ $SALIGN_HB_I8 -gt $SALIGN_HB_F8 ] ; then
   padhb=$SALIGN_HB_I8
fi
# For AIX platform, set $padhb to $pad, so it has a single alignment only.
if [ -n "$HB" ] && [ "$config32" = "rs4_us5" ] ; then
  pad=$padhb
fi
# Define separate alignment for 32 and 64 bit code if necessary
if [ -n "$HB" ] && [ $pad -ne $padhb ] ; then
   echo "# if defined(BUILD_ARCH${HB})"
   case $padhb in
      7)      echo "# define ALIGN_RESTRICT   double" ;;
      1)      echo "# define ALIGN_RESTRICT   short" ;;
      0)      echo "# define ALIGN_RESTRICT   char" ;;
      *)      echo "# define ALIGN_RESTRICT   int" ;;
    esac
    echo "# else"
fi
case $pad in
   7)      echo "# define ALIGN_RESTRICT   double" ;;
   1)      echo "# define ALIGN_RESTRICT   short" ;;
   0)      echo "# define ALIGN_RESTRICT   char" ;;
   *)      echo "# define ALIGN_RESTRICT   int" ;;
esac
if [ -n "$HB" ] && [ $pad -ne $padhb ] ; then
   echo "# endif"
fi

#
# If Compiler does not produce bit-fields ordered as on VAX
#
[ "`tidswap | tr -d '\012'`" = "TID_SWAPPED" ]&& \
		echo "# define TID_SWAP"

#
# If chars are unsigned only:
#
#	The macro, ((i4)(x) <= MAXI1 ? (i4)(x) : ((i4)(x) - (MAXI1 + 1) * 2)),
#	is valid for twos complement machines.
#
if [ "unsigned" = "`unschar`" ]
then
	echo "# define UNS_CHAR"
	echo "# define I1_CHECK(x) ((i4)(x)<=MAXI1?(i4)(x):((i4)(x)-(MAXI1+1)*2))"
else
	echo "# define I1_CHECK	(i4)"
fi
echo "/* 6.1 renamed this... */"
echo "# define I1_CHECK_MACRO( x )	I1_CHECK( (x) )"

#
#	Define the name of the character set being used.  This is
#	used by cmtype.c to determine which table of values will be
#	used to classify characters.  Choose one and only one of:
#
#	CSASCII		Standard ASCII
#	CSDECMULTI	DEC Multinational
#	CSDECKANJI	DEC kanji (double-byte CS)
#	CSJIS		JIS kanji (double-byte CS)
#	CSBURROUGHS 	Burroughs kanji (double-byte CS)
#	CSIBM		IBM's Code Page 0 8 bit character set.
#

if [ "$ING_BUILD" ]
then
case $config32 in
rs4_us5) echo '# define CSIBM'
	 ;;
*)	 echo '# define CSASCII'
	 ;;
esac
fi

#
# 	Define LARGEFILE64 if this is a Large File enabled port.
#	axp_osf does not need the conf_B64 option because it already
#	supports large files with the standard file api's
#
if [ "$conf_B64" ]
then
case $vers in
dgi_us5)
        echo "# define _FILE_OFFSET_BITS  64"  
	;;
axp_osf|\
  *_lnx|\
int_rpl)
        :
        ;;
*)	echo "# define LARGEFILE64"
	;;
esac
fi

#
# 	Define DOUBLEBYTE if this is a DBCS enabled port.
#
if [ "$conf_DBL" ]
then
    echo '# define DOUBLEBYTE'
fi

#
# 	Define INGRESII for Ingres II
#
    echo '# define INGRESII'

#
#	Define WORKGROUP_EDITION for Workgroup Edition
#
if [ "$conf_WORKGROUP_EDITION" ]
then
    echo '# define WORKGROUP_EDITION'
fi

#
#	Define DEVELOPER_EDITION for Developer Edition
#
if [ "$conf_DEVELOPER_EDITION" ]
then
    echo '# define DEVELOPER_EDITION'
fi
#
#	Define xCL_SUSPEND_USING_SEM_OPS for use of sem_wait/sem_post
#
if [ "$conf_CLUSTER_BUILD" ]
then
case $vers in
  *_lnx|\
int_rpl)
    echo '# define xCL_SUSPEND_USING_SEM_OPS'
    ;;
esac
fi
# 
# If any math exceptions can't be continued, then we may need to
# put in more handlers lower than we would if we could continue.
# As declaring and deleting them isn't cheap, create an ifdef flag
# to use around them.
#

mathsigs | grep "HARD" > /dev/null && echo "# define xEX_HARDMATH"

#
#  see if <float.h> exists.
#

# location not guaranteed on linux
case "$vers" in
   *lnx|\
int_rpl)
	floattst=/tmp/flttst$$
	cat << EOF > ${floattst}.c
# include <float.h>

int
main()
{
    return(0);
}

EOF
	$CC -o $floattst ${floattst}.c > /dev/null 2>&1 && \
		 echo "# define xCL_FLOAT_H_EXISTS"
	rm -f ${floattst}*
	;;
     *)
	[ -f /usr/include/float.h ] && \
		echo "# define xCL_FLOAT_H_EXISTS"
	;;
esac

# 
# in 2009 we expect that abs() is defined...
#
echo "# define xCL_ABS_EXISTS"

# 
# see if finite() is defined
#
tmpfile=/tmp/trial$$
rm -f ${tmpfile}*
echo 'main(){float x;finite(x);}' >${tmpfile}.c
$CC ${tmpfile}.c -o ${tmpfile} >/dev/null 2>&1 && echo "# define xCL_FINITE_EXISTS"
rm -f ${tmpfile}*

#
# GTK support
#
case "$vers" in
    *lnx)
#
# except for Debian - because bldenv doesn't set GTK Header path stuff
# for Debian on the basis that Debian doesn't have RPM. 
# At present (28-mar-2008) GTK support implies RPM support
#
        if [ ! -f /etc/debian_version ]
        then
	  echo "# define xCL_GTK_EXISTS"
        fi
	;;
esac

#
# RPM support
#
case "$vers" in
    *lnx)
	echo "# define xCL_RPM_EXISTS"
	;;
esac

#
#  see if variadic macros are supported
#
tmpfile=/tmp/varmac$$
rm -f ${tmpfile}*
cat >${tmpfile}.c <<'!EOF!'
# include <stdarg.h>

void
vfunc(int x, ...)
{
    int nothing;
}

int
main()
{
    int x = 0;

#define	vmac(x, ...) vfunc(x, __VA_ARGS__)

    vmac(x,1,2,3);
    return(0);
}
!EOF!
$CC -c -o ${tmpfile}.o ${tmpfile}.c > /dev/null 2>&1 \
	&& echo '# define HAS_VARIADIC_MACROS'
rm -f ${tmpfile}*

#
# Some OS_THREADS platforms cannot successfully support
# thread stack caching.  It can be turned off here by
# defining xCL_NO_STACK_CACHING for your platform.
# It's defined here, rather than in clsecret.h, so that
# it can be picked up in st/crsfiles for processing dbms.crs.
#


echo ""
echo "/* End of $header */"

trap : 0
