/*
** Copyright (c) 1983, 2004 Ingres Corporation
** All rights reserved.
*/

/*
** Name:	bzarch.h  - 	used to define symbols needed to compile system
**				on differing architectures
*/

/*
** 	This header file is used to select architectural features needed
**	by the processor/OS/machine INGRES is running on.
**
** 	AS MANY of the FEATURES below AS NEEDED are to be SELECTED.
**
**	To select a feature, put a comment at the of the line beginning
**	"To select ...",  ABOVE the feature you want to select.
**
**	History:
**		3/12/85 (kooi) -- added xCACHE (used to be in trace.h)
**		3/19/85 (cfr) -- added in TID_SWAP
**    16-mar-92 (kevinq) NT
**       Microsoft NT migration.  NOTE:  under NT, bzarch is NOT created
**       via mkbzarch.sh!
**    11-jul-95 (emmag)
**	 Removed define for MCT
**    19-jul-95 (emmag)
**	 Added define for MCT on instruction from Pierre Bouchard.
**    12-Sep-95 (fanra01)
**       Added preprocessor statements to modify the GLOBALDEF and GLOBALREF
**       type specifiers.  These vary according where they are used.
**       +---------+-------------------+------------------------------------+
**       |         |GLOBALDEF          |               GLOBALREF            |
**       |         |                   +-------------------+----------------+
**       |         |                   |    import data    | facility global|
**       +---------+-------------------+-------------------+----------------+
**       | DLL     |declspec(dllexport)|declspec(dllimport)|   extern       |
**       +---------+-------------------+-------------------+----------------+
**       | PROGRAM |                   |declspec(dllimport)|   extern       |
**       +---------+-------------------+-------------------+----------------+
**    17-oct-95 (emmag)
**	 Added define for int_wnt (Intel - Windows NT)
**    25-oct-96 (mcgem01)
**	 Add define for OS_THREADS_USED for 2.0
**	04-aug-1997 (canor01)
**	    Add "extern" to GLOBALCONSTREF.
**	31-jul-1998 (canor01)
**	    include "process.h" and "windowsx.h"
**	10-aug-1998 (canor01)
**	    include "malloc.h" for alloca() definition.
**	18-nov-1998 (canor01)
**	    Add definition of DLLEXPORT and DLLIMPORT.
**	19-nov-1998 (canor01)
**	    Add include of rpc directories for UUID definitions. Remove
**	    historical OS/2 definitions.
**      16-dec-1998 (canor01)
**          Remove include of "windowsx.h", which was included a little too
**          readily.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	08-feb-2001 (somsa01)
**	    Porting changes for i64_win.
**	12-jun-2001 (somsa01)
**	    In the case of C++, GLOBALREF should be 'extern "C"'.
**	25-oct-2001 (somsa01)
**	    Set the appropriate platform identifier in the case of Windows.
**	15-jan-2002 (somsa01)
**	    Changed Win64's ALIGN_RESTRICT from longlong to double.
**	03-jul-2002 (somsa01)
**	    All Windows platforms should have LARGEFILE64 defined.
**	27-jan-2003 (devjo01)
**	    Add RAAT_SUPPORT to enable Record at a Time interface for
**	    32 bit windows.
**	02-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
**	27-Jan-2004 (fanra01)
**	    Add I8ASSIGN_MACRO and ALIGN_I8.
**	20-Apr-04 (gordy)
**	    Added symbols LITTLE_ENDIAN, BIG_ENDIAN, NO_64BIT_INT.
**	14-May-04 (penga03)
**	    Modified symbols BIG_ENDIAN to BIG_ENDIAN_INT, LITTLE_ENDIAN to 
**	    LITTLE_ENDIAN_INT.
**	29-Oct-2008 (jonj)
**	    SIR 120874: Check for variadic macro support, emit
**	    HAS_VARIADIC_MACROS if _MSC_VER >= 1400 (VS 2005 or better).
**      12-dec-2008 (joea)
**          Remove BYTE_SWAP (use BIG/LITTLE_ENDIAN_INT instead).
*/

# ifdef WIN64
# ifdef NT_AMD64
# define	a64_win
# else
# define	i64_win
# endif  /* NT_AMD64 || NT_IA64 */
# else
# define	int_w32
# endif  /* WIN64 */

# define	BITSPERBYTE	8

/*
**	IMPORTANT NOTE *****  This constant (PG_SIZE) is used by the
**	front-ends to set up the appropriate size buffers for info they
**	receive from INGRES.  SO, please don't change this without
**	notifying the front-end.
*/

#define DYNALINK

# define	PG_SIZE		2048
					/* so far this hasn't changed */

# define MAX_FILE_HANDLES 16535	/* should be more than enough! */

/*	To select MATH_EX_HANDLE, put an end of comment at the end of this line
*/
# define	MATH_EX_HANDLE
				/*
				** For machines that can continue from
				** math exceptions like divide by zero.
				** VMS can do this as well as some
				** UNIX systems.  This define is used
				** in iutil!mathhandl.c
				*/

/*	To select NO_64BIT_INT, put an end of comment at the end of this line

# define	NO_64BIT_INT
				/*
				** For machines without 64 bit integer
				** support either in hardware or by
				** compiler emulation.  Used by cvgcc.h
				*/

/*	To select BIG_ENDIAN, put an end of comment at the end of this line

# define	BIG_ENDIAN_INT
				/*
				** For big-endian integer machines.
				** Used by cvgcc.h
				*/

/*	To select LITTLE_ENDIAN, put an end of comment at the end of this line
*/
# define	LITTLE_ENDIAN_INT
				/*
				** For little-endian integer machines.
				** Used by cvgcc.h
				*/

/*	To select IEEE_FLOAT, put an end of comment at the end of this line
*/
# define	IEEE_FLOAT
				/*
				** For machines whose floating point
				** rep. follows the IEEE standard.
				** This reduces the range of money types
				** over the VAX.  Used in cvgcc.h
				*/

/*	To select TID_SWAP, put an end of comment at the end of this line.

# define 	TID_SWAP
				/* for machines/compilers whose internal
				** storage of bit fields is the opposite
				** of the vax.  That is the MSB is in
				** the low addr and the LSB is in the
				** hi addr. (Currently these are the
				** 68000, pyramid, and 3b5 processors)
				*/

/*	To select DUMB_ACCESS, put an end of comment at the end of this line.

/* # define	DUMB_ACCESS */
				/* for UNIX OS variants (UNIX5.0,
				   BSD4.1a) where access run from
				   a setuid program runs as the
				   real rather than the effective
				   user.  This presents problems
				   in protecting the dbs so a
				   special process, chowndir, has
				   been created for createdb and
				   some error checking is sacrificed.
				*/

/*	To select ALIGN_RESTRICT, put an end of comment at the end of this line
*/

# ifdef WIN64
# define	ALIGN_RESTRICT	double
#else
# define	ALIGN_RESTRICT	long
# define	RAAT_SUPPORT
# endif
				/*
				** Specifies the byte alignment
				** properties desired for this
				** machine.  No data type used should
				** require more restrictive alignment
				** than this type.
				*/

/*	To select GLOBAL PAGE CACHE, put an end of comment at the end of
	this line.

# define	xCACHE
				/*
				** For machines, like VAX/VMS, where
				** we have a global page cache
				*/


# define	INCL_DOS
# define	INCL_DOSERRORS
#include <stdio.h>
#include <process.h>
#include <windows.h>
# include <rpc.h>
# include <rpcndr.h>
# include <rpcdce.h>
#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include <stddef.h>
#include <limits.h>
#include <clsecret.h>

/*	To choose NO_QRYMOD configuration, end this line with a comment

# define	NO_QRYMOD
				/* qrymod isn't part of this backend
				** (no views, integrities, permissions)
				** For PCINGRES.
				*/

/*	To select ATTCURSES, put an end of comment at the end of this line.

# define	ATTCURSES
				/*
				** To use the CURSES screen
				** manipulation package pro-
				** vided by AT&T for SYSTEM V.
				*/

/*	To select ATTLOCK, put an end of comment at the end of this line.

# define	ATTLOCK
				/*
				** To use the lock driver that
				** uses semaphores in SYSTEM V.
				*/

/*	To select TOWER, put an end of comment at the end of this line.

# define	TOWER
				/*
				** To use code changes created
				** for the NCR TOWER (usually
				** due to compiler bugs.)
				*/


#if defined(IMPORT_DLL_DATA)
#define readonly		const
#define READONLY		const
#define WSCREADONLY		const		/* for backend folks */
#define GLOBALCONSTREF		extern const
#define GLOBALCONSTDEF		const
#define globalref		extern
#ifdef __cplusplus
#define GLOBALREF		extern "C"
#else  /* __cplusplus */
#define GLOBALREF		extern
#endif  /* __cplusplus */
#define GLOBALDEF
#define globaldef
# define GLOBALDLLREF           __declspec(dllimport)
#else               /* IMPORT_DLL_DATA */

#define readonly		const
#define READONLY		const
#define WSCREADONLY		const		/* for backend folks */
#define GLOBALCONSTREF		extern __declspec(dllimport) const
#define FACILITYCONSTREF        extern const
#define globalref		__declspec(dllimport)
#define GLOBALREF               __declspec(dllimport)
#define FACILITYREF             extern
#define GLOBALCONSTDEF		__declspec(dllexport) const
#define FACILITYCONSTDEF        const
#define GLOBALDEF               __declspec(dllexport)
#define globaldef               __declspec(dllexport)
#define FACILITYDEF
#endif              /* IMPORT_DLL_DATA */
#define DLLEXPORT		__declspec(dllexport)
#define DLLIMPORT		__declspec(dllimport)

#	define	VOLATILE		volatile
#	define	ZERO_FILL 		={0}

/*	To select BYTE_ALIGN, put an end of comment at the end of this line
*/
# define	I2ALIGNCHECK	0x1
# define	I4ALIGNCHECK	0x3
# define 	BYTE_ALIGN
				/* for machines needing ints and floats
				** aligned on 2, 4, and/or 8-byte
				** boundaries.  (Currently these are the
				** 68000, pyramid, and 3b5 processors)
				*/

/*	To select UNS_CHAR, put an end of comment at the end of this line.

# define 	UNS_CHAR
				/* for machines where chars are always
				** UNSIGNED objects. (Currently this is
				** the 3b5)
				*/

/*	To select UNS_I4, put an end of comment at the end of this line. */

# define 	UNS_I4
				/* for machines where i4's can be
				** UNSIGNED objects. (Currently this
				** is not true for Lattice).
				*/

/*	To select UPPER_FILENAME, put an end of comment at the end of this line
 */
# define 	UPPER_FILENAME
				/* for machines who do NOT distinguish
				** case for filenames. (Currently this
				** is VMS )
				*/

/*	To select NO_VTIMES, put an end of comment at the end of this line.
*/

# define	NO_VTIMES
				/*
				** For operating systems that don't
				** have a UNIX vtimes function.
				*/

/*	To select NO_FTIME, put an end of comment at the end of this line.
*/

# define	NO_FTIME
				/*
				** For operating systems that don't
				** have a UNIX ftime function.
				*/

/*	To select CPU100THS, put an end of comment at the end of this line.

# define	CPU100THS
				/*
				** For machines with clock ticks every
				** 100th of a second.  This is currently
				** the 3b5.
				** (The VAX and 68ks tick every 60th
				** of a sec.)
				*/

/*	To select x3b_BITFIELD_BUG, put an end of comment at the end
	of this line
# define	x3b_BITFIELD_BUG
				/*
				** This selects altered code to handle
				** machines that require bit fields to
				** be four byte aligned.
				*/

/*	To use compiler bit-field capability, put an end-of-comment
	at the end of this line.
*/

# define	HASBITFLDS
				/* whaddaya know, this compiler
				** accepts BITFLDS
				*/

/*	To use compiler structure-assignment capability, terminate
	this comment here:
*/
# define 	STRUCTASSN
				/* we can assign whole structures */

/*{
** Name:	I1_CHECK() -	Convert I1 to Nat (sign-extended).
**
** Description:
**	This macro converts an I1 to a i4, sign-extending on unsigned
**	character machines.
*/
# ifdef UNS_CHAR
# define	I1_CHECK(x)	((i4)(x) <= MAXI1 ? (i4)(x) : ((i4)(x) - (MAXI1 + 1) * 2))
# else
# define	I1_CHECK(x)	(i4)(x)
# endif

# define	FIXCHAR(x)		I1_CHECK(x)
# define	I1_CHECK_MACRO(x)	I1_CHECK(x)


/* Microsoft compiler supports "pragma" preprocessor statements
   which can be used to tell the compiler to turn on and off the code
   generation for stack checking code */

# ifdef MICROSOFT

# define STACKCHECKING \#pragma check_stack(on)
# define NOSTACKCHECKING \#pragma check_stack(off)

# else

# define STACKCHECKING
# define NOSTACKCHECKING

# endif

#ifndef __IBMC__
#define register
#endif

# define _BA(a,i,b)		(((char *)&(b))[i] = ((char *)&(a))[i])
# define I2ASSIGN_MACRO(a,b)	(_BA(a,0,b),_BA(a,1,b))
# define I4ASSIGN_MACRO(a,b)	(_BA(a,0,b),_BA(a,1,b),_BA(a,2,b),_BA(a,3,b))
# define I8ASSIGN_MACRO(a,b) (_BA(a,0,b), _BA(a,1,b), _BA(a,2,b), \
                _BA(a,3,b), _BA(a,4,b), _BA(a,5,b), _BA(a,6,b), _BA(a,7,b))
# define F4ASSIGN_MACRO(a,b)	(_BA(a,0,b),_BA(a,1,b),_BA(a,2,b),_BA(a,3,b))
# define F8ASSIGN_MACRO(a,b)	(_BA(a,0,b),_BA(a,1,b),_BA(a,2,b),_BA(a,3,b), \
				 _BA(a,4,b),_BA(a,5,b),_BA(a,6,b),_BA(a,7,b))

# define LARGEFILE64
# ifdef WIN64
# define PTR_BITS_64
# define SCALARP __int64
# define LP64
# else
# define SCALARP int
# endif

# define        ALIGN_I2        0
# define        ALIGN_I4        0
# define        ALIGN_I8        7
# define        ALIGN_F4        0
# define        ALIGN_F8        3

# define        MCT
# define        OS_THREADS_USED

/* If Visual Studio 2005 or better, should support variadic macros */
#if defined(_MSC_VER) && ( _MSC_VER >= 1400 )
# define	HAS_VARIADIC_MACROS
#endif
