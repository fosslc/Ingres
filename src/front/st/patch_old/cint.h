/**
** Name:  cint.h - Define global cipher constants.
**
**
** Description:
**	This file contains global constants used by the compatiblility
**	library module, CI.  This module is used for encryption and
**	decryption, mapping binary byte streams to and from  printable
**	strings, and interpreting authorization strings for Ingres
**	Distribution keys.
**
** History:
**	12-dec-95 (tutto01)
**	    Stolen and stripped down from unix for the NT install.  Not
**	    pretty, but quick.
**/

/*
** Define the maximum length of a capability stream; note, this should be
** evenly divisible by 8 for use in encrytion routines.
*/
# define	MAX_CAP_LEN	128
/* jpk: real length has increased from 56 to 88, so 128 is still enough */

/*
** Define the maximum length of an authorization string.  This should be equal
** to the maximum length of an environment variable's equivalence string.
*/
# define	MAX_ASTR_LEN	256
/* jpk: real length remains 32, so 256 is still enough */

/**
**    Define the current Ingres Distribution Keys encryption key.
** This should be 8 chars. This key is used for encrypting the Capability
** block in the process of making the authorization string.  It should be
** changed only when and everytime we produce a new major release.  The last
** character should always change by a quantity of one unless it's equal to
** 'Z' in which case it should cycle back to '2'.
*/
# define	DIST_KEY	"DIST_KEZ"
/*
** Define the distribution version number.  This is used to determine is
** the authorization string has been decoded with the correct DIST_KEY, by
** embedding it in the capability block before it is encrypted.  Then if
** this value isn't the same after decryption of the authorization string,
** then the authorization string has been decrypted with the wrong key.
** Its value should increment by one every major INGRES release.
** E.g., If the eighth character in DIST_KEY is 'a' in 4.0, then it should
** be 'b' in 4.1.
*/
# define	DIST_VERSION	('Z' & 0x0ff)

/* define the number of iterations through the encryption algorithm. */
# define	ITER_NUM	8

/*
** Define a special value, which when passed to CIcapability to test
** whether the currently defined II_AUTH_STRING is a valid string.
*/
# define	CI_VALIDSTR	-1


/*
** BEGIN INGRES CAPABILITIES
*/
/*
**    Define Ingres capabilities and their corresponding bit positions in the
** Ingres capability vector.  The Ingres capability vector consists of 64 bits.
**
******************************************************************************
** DO NOT ALLOCATE NEW BITS WITHOUT GETTING THEM ASSIGNED BY THE CL COMMITTEE!
******************************************************************************
** The CI capability bit assignments are supposed to be the SAME across all
** platforms.  However, some capabilities may not be actively supported on
** every platform at a given time, so you might see some gaps in ci.h for a
** given platform.  Just because you see an available slot in this header
** file, DO NOT ASSUME that that bit is available for you to assign.  You
** must ONLY take bits assigned to you from the central assignment list
** maintained by Scott Tsugawa for the CL Committee.
*/
/*
** Define bit vectors that turn off portions of capability checking or in some
** other way alter the semantical interpretation of the authorization string.
** These vectors begin at the last bit in the capability bits of the capability
** block and grow towards the beginning of the capability bits; i.e. from
** the (CAP_BYTE_CNT * BITSPERBYTE - 1)th bit towards the 0th bit.
*/
/* Define the vector position of the last bit in the capability bits. */
/* jpk: CAP_BYTE_CNT increased from 7 to 11; increases cap bits from 56 to 88 */
# define	CAP_BYTE_CNT		11
# define	LAST_CAPABILITY_BIT	(CAP_BYTE_CNT * BITSPERBYTE - 1)
/* Define authorization string modifier bit vectors. */
# define	CI_NO_CHK_FLAG		LAST_CAPABILITY_BIT
# define	CI_NO_EXP_DATE		(CI_NO_CHK_FLAG - 1)
# define	CI_NO_SER_NUM_FLAG	(CI_NO_EXP_DATE - 1)
# define	CI_NO_CPU_MODEL_FLAG	(CI_NO_SER_NUM_FLAG - 1)
/*
**     Define bit vectors that turn on RTI product capabilities.  These bits begin
** at 0 and grow towards the last bit in the capability bits field.
*/
/* Define backend capabilities */
# define	CI_BACKEND		0
/* create tables via terminal monitor (only VAR licenses lack this) */
# define	CI_VAR_INGRES		1
# define	CI_CREATE_TABLE		1
# define	CI_INGRES_NET		2
/* Define terminal monitor capabilities */
# define	CI_QUEL_TM		3
# define	CI_SQL_TM		4
/* Define front-end capabilities */
# define	CI_ABF			5
# define	CI_GBF			6
# define	CI_NEW_GBF		7
# define	CI_MENU			8
# define	CI_OSLQUEL		9
# define	CI_OSLSQL		10
# define	CI_QBF			11
# define	CI_RBF			12
# define	CI_REPORT		13
# define	CI_VIFRED		14
/* Define equel language capabilities */
# define	CIeqADA			15
# define	CIeqBASIC		16
# define	CIeqC			17
# define	CIeqCOBOL		18
# define	CIeqFORTRAN		19
# define	CIeqPASCAL		20
# define	CIeqPL1			21
/* Define esql language capabilities */
# define	CIesqADA		22
# define	CIesqBASIC		23
# define	CIesqC			24
# define	CIesqCOBOL		25
# define	CIesqFORTRAN		26
# define	CIesqPASCAL		27
# define	CIesqPL1		28
/* Define PClink capability */
# define	CI_PCLINK		29
/* Define cluster option */
# define	CI_CLUSTER_FLAG		30
/* Define 3270 block mode capability */
# define	CI_BLOCK_3270		31
/* Define Distributed Backend Capability */
# define	CI_DBE			32
/* Define Serial protocol capability -- Protocal Support - Async */
# define	CI_CRGBLAST		33
/* Define Release 5 bits for sync w/ 6 & beyond (CI_SS_STAR->single node restr*/
# define	CI_RMS_GATEWAY		34
# define	CI_SS_STAR		35
# define	CI_RDB_GATEWAY		36
# define        CI_PM_GATEWAY           37
/* Define INGRES/TEAMWORK Case capability */
# define	CI_DBD_CASE		38
/* Define INGRES/VISION (Emerald) capability */
# define	CI_VISION		39
/* Define Terminator I optional packages (Knowledgemgmt & Objectmgmt) */
# define	CI_RESRC_CNTRL		40
# define	CI_USER_ADT		41
/* run ABFimage (only fully restricted VAR licenses lack this) */
# define	CI_ABF_IMAGE		42
/* UNUSED				43 */
/* Define placeholders for NET protocols */
# define        CI_INGTCP_NET           44
# define        CI_INGSNA_NET           45
/* Define Teamwork Reverse Engineering capability */
# define	CI_DBD_REV		46
/* Define Windows 4gl capability */
# define	CI_WINDOWS4GL_DEV	47
/* CI_WINDOWS4GL_RUNTIME set means user is restricted to runtime */
# define        CI_WINDOWS4GL_RUNTIME   48
# define        CI_CONVERSION_TOOLS     49
/* bit 50 is in use, ISOS thinks, for RUNTIME */
# define	CI_REPLICATOR		51
/* spatial data types */
# define        CI_CLASSOBJ_ADT         52
/* Support for Novell/USL's TUXEDO Open TP Monitor */
# define        CI_XA_TUXEDO            53
/* Support for IBM's CICS/6000, CICS/OEM Open TP Monitor */
# define        CI_XA_CICS              54
/* Support for Transarc's Encina Open TP Monitor */
# define        CI_XA_ENCINA            55
/* OpenINGRES/Enhanced Security 1.1 */
# define CI_B1_SECURE			56
/* Tech Support bit to enable use of potentially risky support tools */
# define CI_SUPPORT			57
/* embedded SQL C++ */
# define CI_esqCPLUSPLUS		58
/* per-user licensing */
# define CI_PER_USER			59
