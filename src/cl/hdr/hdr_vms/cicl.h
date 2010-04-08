/*
**	Copyright (c) 1985, 2002 Ingres Corporation
*/

/**CL_SPEC
** Name:	cicl.h	- Define global cipher constants.
**
** Specification:
**
** Description:
**	  This file contains global constants used by the compatiblility
**	library module, CI.  This module is used for encryption and
**	decryption, mapping binary byte streams to and from  printable
**	strings, and interpreting authorization strings for Ingres
**	Distribution keys.
**
** History:
**	15-Sep-1986 (fred)
**	    Added to Jupiter CL.
**	<automatically entered by code control>
**	22-jun-89 (jrb)
**	    Added capability bits 40 and 41 for terminator phase I.
**	31-jul-1989 (sandyh)
**	    Added NET protocol enabling bits & gateways
**      15-aug-89 (jennifer)
**          Added C2 capability bit 43
**      19-sep-1989 (sandyh)
**          Modified bit order to conform with release 5 strings
**      16-jan-90 (sandyd)
**          Added CI_PART_RUNTIME, CI_FULL_RUNTIME, CI_VISION, for 6.4.
**          Also added comment about central control of CI bit allocation,
**	    and removed the unauthorized ingresnet "placeholders" CI_ING1_NET,
**	    CI_ING2_NET, CI_ING3_NET, since they conflict with bit numbers
**	    already properly assigned for other uses.
**	27-apr-90 (marian)
**	    Updated so 6.3 and 6.4 vms/unix are consistent with what the
**	    'official' list maintained by the product code controllers
**	    contains.  DO NOT update this file without notifying the
**	    appropriate people.  There should only be ONE copy of the ci.h
**	    file for all releases or the authorization strings will break.
**	07-may-90 (marian)
**	    Add bit 47 for windows 4gl.
**	22-mar-91 (erickson)
**	    Add bits 48,49.
**	07-dec-1992 (pholman)
**	   C2: freed up bit 43 (was security auditing, which is now part 
**	   of KME)
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**     26-aug-1993 (stevet)
**          Added CI_CLASSOBJ_ADT for INGRES class library objects.
**	22-dec-1993 (jpk)
**	    Remove cbsid from capability block; increase CAP_BYTE_CNT
**	    from 7 to 11.  Thus sizeof(CI_CAP_BLOCK) remains the same,
**	    20 bytes, so the printable auth string remains the same length
**	    as well, 32 printable characters.  Net result is get rid of a
**	    nearly-useless processor ID check, a no-op in most cases, in
**	    return for 32 more capability bits.
**	28-March-1994 (jpk)
**	    Add new capability bits: CI_B1_SECURE, CI_SUPPORT,
**	    and CI_esqCPLUSPLUS.
**	29-March-1994 (jpk)
**	    Fix bug 60494: change name of CI_FULL_RUNTIME to CI_CREATE_TABLE,
**	    and CI_PART_RUNTIME to CI_ABF_IMAGE, to better reflect their
**	    new sense of enabling capability, not disabling.
**	30-jan-1995 (wolf) 
**	    CI_PER_USER was added to the Unix and VAX CLs, needs to be here too.
**	08-nov-1996 (chech02)
**	    CI_INGRES_BRIDGE and CI_OPAL need to be here too.
**	19-may-1998 (kinte01)
**	    Cross integrate Unix change 435147 & 435463
**	    25-mar-1998 (canor01)
**            Add capabilities for gateways.
**    	    06-may-1998 (canor01)
**            Add capability bit for DB2 gateway.
**	24-jun-1998 (kinte01)
**	    Changed DIST_KEY for 2.0
**      04-sep-1998 (kinte01)
**          Add CI_ERR_MAX_LEN, the maximum length of an error message
**          returned from the CA-Licensing software. (Unix change 437510)
**	11-sep-1998 (kinte01)
**	    Add new bit for CI_ICE (unix change 437663)
**	22-nov-2002 (mcgem01)
**	    Add a bit for Advantage OpenROAD Transformation Runtime.
**	11-Jun-2004 (hanch04)
**	    Removed reference to CI for the open source release.
*/


/*
** Define the maximum length of a capability stream; note, this should be
** evenly divisible by 8 for use in encrytion routines.
*/
# define	MAX_CAP_LEN	128
/* jpk: real length has increased from 56 to 88, so 128 is still enough */

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
# define	CI_XA_TUXEDO		53
/* Support for IBM's CICS/6000, CICS/OEM Open TP Monitor */
# define	CI_XA_CICS		54
/* Support for Transarc's Encina Open TP Monitor */
# define	CI_XA_ENCINA		55
/* OpenINGRES/Enhanced Security 1.1 */
# define CI_B1_SECURE			56
/* Tech Support bit to enable use of potentially risky support tools */
# define CI_SUPPORT			57
/* embedded SQL C++ */
# define CI_esqCPLUSPLUS		58
/* per-user licensing */
# define CI_PER_USER			59
/* OpenINGRES Protocol Bridge */
# define	CI_INGRES_BRIDGE	60
/* OPAL Support */
# define CI_OPAL			61
/* Oracle Gateway */
# define CI_ORACLE_GATEWAY              62
/* Informix Gateway */
# define CI_INFORMIX_GATEWAY            63
/* Sybase Gateway */
# define CI_SYBASE_GATEWAY              64
/* MS SQL Server Gateway */
# define CI_MSSQL_GATEWAY               65
/* ODBC Gateway */
# define CI_ODBC_GATEWAY                66
/* ALLBASE Gateway */
# define CI_ALB_GATEWAY                 67
/* CA-Datacom Gateway */
# define CI_DCOM_GATEWAY                68
/* CA-IDMS Gateway */
# define CI_IDMS_GATEWAY                69
/* IMS Gateway */
# define CI_IMS_GATEWAY                 70
/* VSAM Gateway */
# define CI_VSAM_GATEWAY                71
/* RMS Gateway */
# define CI_NEW_RMS_GATEWAY             72
/* RDB Gateway */
# define CI_NEW_RDB_GATEWAY             73
/* DB2 Gateway */
# define CI_DB2_GATEWAY                 74
/* ICE */
# define CI_ICE                         75
/* OpenROAD Transformation Runtime */
# define CI_WINDOWS4GL_AOTR		76
 
/*
**  These special bits 84-87 are used for special authorization strings 
** and are declared at the top of this section.  It is also placed here
** so the bits are not reused.
**
**# define	CI_NO_CHK_FLAG		LAST_CAPABILITY_BIT	 ==> 87
**# define	CI_NO_EXP_DATE		(CI_NO_CHK_FLAG - 1)	 ==> 86
**# define	CI_NO_SER_NUM_FLAG	(CI_NO_EXP_DATE - 1)	 ==> 85
**# define	CI_NO_CPU_MODEL_FLAG	(CI_NO_SER_NUM_FLAG - 1) ==> 84
*/

/*
**	END INGRES CAPABILITIES
**/


/*}
** Name:	CI_KS	- encryption key schedule.
**
** Description:
**	  This is a key schedule datatype.  It is used by the encryption algorithm.
**	Before it is used, it must be initialized using the routine, CIsetkey().
**
** History:
**	27-Jan-86 (ericj) - written.
*/
/* define the number of iterations through the encryption algorithm. */
# define	ITER_NUM	8

typedef	char	CI_KS[ITER_NUM][48];

# define CI_ERR_MAX_LEN                 512

