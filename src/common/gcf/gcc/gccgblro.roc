/*
** Copyright (c) 1988, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>

/**
**
**  Name: GCCGBLRO.ROC - Global read-only storage for GCC
**
**  Description:
**      This file declares global read-only storage required by GCC.  These are
**	certain global constants initialized at compile time, and used in a
**	read-only way by other GCC routines.  They are defined here in a
**	separate module to accommodate the way MVS does business in setting up
**	global static read-only references.
**
**
**  History:
**      21-Nov-1988 (jbowers)
**          Initial module creation.
**      08-Mar-1989 (ham)
**          Added C literal conversion table..
**	05-jul-89 (seiwald)
**	    NTS now ASCII.  Excepting 0x7f (DEL), all C literals map to
**	    ASCII equivalent, and control characters map to space.
**	    0x7f (DEL) maps to 0xff, because EBCDIC " is 0x7f.
**	15-Mar-90 (seiwald)
**	    Pass upper range bytes (128-255) through directly.
**	    This allows Kanji to indentify itself as ASCII (yuk).
**	30-May-90 (seiwald)
**	    Pass control characters (1-31) through directly.
**	    The only thing missing is 127.
**      30-Jul-90 (bxk)
**          Map 0x7f to itself so as to preserve one-to-one mapping.
**	28-Mar-91 (seiwald)
**	    Added ASCII_PC to NSCS table.
**	16-Aug-91 (seiwald)
**	    Updated IIGCc_rev_lvl.
**	16-Dec-91 (fraser)
**	    Add international support for IBMPC_ASCII.
**	10-Dec-92 (seiwald)
**	    6.5/beta GCF.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      10-nov-94 (tutto01)
**          changed message to reflect new revision. (bug #63570)
**	09-Feb-96 (rajus01)
**	    Defined  IIGCb_rev_lvl[] for Protocol Bridge.
**	28-jan-1997 (hanch04)
**	    changed to new version OI 2.0/00
**	18-jun-1997 (canor01)
**	    Add version for Jasmine.
**	20-jun-1997 (canor01)
**	    Jasmine version is now "Jasmine 1.1"
**	03-nov-1997 (reijo01)
**	    Make the GA genlevel 9711.
**	04-feb-1998 (hanch04)
**	    changed to new version OI 2.5/00
**	13-feb-1998 (johnny)
**	    Jasmine version is now "Jasmine 1.2"
**	    Make the genlevel 9804.
**	17-jun-1998 (canor01)
**	    Integrate ingres and jasmine versions to common base.
**	09-jul-98 (mcgem01)
**	    Change the server version string to II for the Ingres II builds.
**	15-jul-98 (mcgem01)
**	    Change INGRES_II to INGRESII
**	17-jun-1999 (mcgem01)
**	    Add product Version information.
**      02-Mar-2001 (hanch04)
**          Bumped release id to external "II 2.6" internal 860 or 8.6
**      07-Apr-2003 (hanch04)
**          Bumped release id to external "II 2.65" internal 865 or 8.65
**      13-Jan-2004 (sheco02)
**          Bumped release id to external "II 3.0" internal 90 or 9.0
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGCFLIBDATA
**	    in the Jamfile.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Removed LIBRARY jam hint.  This file is now included in the
**          GCC local library only.
**	17-Jan-2006 (hweho01)
**	    Changed version string to "ING 9.0". 
**	01-Feb-2006 (hweho01)
**	    Modified version string to "II 9.0", need to maintain the 
**          compatibility with current/previous releases. 
**	27-Apr-2006 (hweho01)
**	    Changed version string to "II 9.1" for new release. 
**	07-May-2007 (drivi01)
**	    Changed version string to "II 9.2".
**	14-Jun-2007 (bonro01)
**	    Changed version string to "II 9.3".
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	04-Jun-2009 (bonro01)
**	    Changed version string to "II 9.4" for new release.
**	10-Jun-2009 (hweho01)
**	    Changed version string to "II 10.0" for new release.
**	12-Jul-2010 (bonro01)
**	    Changed version string to "II 10.1" for new release.
**/


/*
** Definition of all global static read-only variables for GCC.
*/

/*
** GCC revision number designation.
*/

GLOBALDEF  const	char	IIGCc_rev_lvl[] = "II 10.1";

/*
** GCB revision number designation.
*/

GLOBALDEF  const	char	IIGCb_rev_lvl[] = "II 10.1";

/* Define C literal conversion table and constants */

# ifdef PMFE /* ASCII_PC */

GLOBALDEF  const	unsigned char	IIGCc_inxl_tbl[] =
    {
	'\0', 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	'\b', '\t', '\n', '\v', '\f', '\r', 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x00, 0x16, 0x17,		 
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	' ',  '!',  '"',  '#',  '$',  '%',  '&',  '\'',
	'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	'@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',
	'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
	'X',  'Y',  'Z',  '[',  '\\',  ']',  '^',  '_',
	'`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  '{',  '|',  '}',  '~',  0x7f,
	0x00, 0xed, 0xf6, 0xec, 0x00, 0x00, 0x00, 0x00,		 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xad, 0x9b, 0x9c, 0x00, 0x9d, 0x00, 0x15,
	0x00, 0x00, 0xa6, 0xae, 0xaa, 0xf0, 0x00, 0xee,
	0xf8, 0xf1, 0xfd, 0xfc, 0xef, 0xe6, 0x00, 0xfa,
	0x00, 0xfb, 0xa7, 0xaf, 0xac, 0xab, 0xf3, 0xa8,
	0xb7, 0xb5, 0xb6, 0xc7, 0x8e, 0x8f, 0x92, 0x80,
	0xd4, 0x90, 0xd2, 0xd3, 0xde, 0xd6, 0xd7, 0xd8,
	0xd1, 0xa5, 0xe0, 0xe3, 0xe2, 0xe5, 0x99, 0x00,
	0x00, 0xeb, 0xe9, 0xea, 0x9a, 0x00, 0xe8, 0xe1,
	0x85, 0xa0, 0x83, 0xc6, 0x84, 0x86, 0x91, 0x87,
	0x8a, 0x82, 0x88, 0x89, 0x8d, 0xa1, 0x8c, 0x8b,
	0xd0, 0xa4, 0x95, 0xa2, 0x93, 0xe4, 0x94, 0x00,
	0x00, 0x97, 0xa3, 0x96, 0x81, 0x98, 0xe7, 0x00		 
    };

# else /* PMFE */

GLOBALDEF  const	unsigned char	IIGCc_inxl_tbl[] =
    {
	'\0', 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	'\b', '\t', '\n', '\v', '\f', '\r', 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	' ',  '!',  '"',  '#',  '$',  '%',  '&',  '\'',
	'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	'@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',
	'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
	'X',  'Y',  'Z',  '[',  '\\',  ']',  '^',  '_',
	'`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  '{',  '|',  '}',  '~',  0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
	0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
	0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };

# endif /* PMFE */
