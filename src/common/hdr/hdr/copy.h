/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: COPY.H - Common definitions used for COPY command.
**
** Description:
**      The file contains the definitions shared by the DBMS and FE during
**	execution of the COPY command.
**
** History:
**      01-dec-1986 (rogerk)
**          Written.
**	06-apr-1987 (rogerk)
**	    Added CPY_CONTINUE status which specifies that the user wants to
**	    continue with the copy when errors are encountered. 
**	06-apr-1987 (rogerk)
**	    Took out definition of CPY_VARROWS as it was a FE only concern.
**	29-apr-1991 (Derek)
**	    Added support for EMPTYTABLE clause to copy.
**	26-jul-1993 (rmuth)
**	    Added support for COPY to take a variable number of WITH
**	    options. These are passed around in a DMR_CB.
**	4-Dec-2009 (kschendel) SIR 122974
**	    Add special CSV delimiter.
**/

/*
**  Forward and/or External function references.
*/

/* 
** Defined Constants
*/

#define			CPY_INTO	0
#define			CPY_FROM	1

#define			CPY_MAXROW	(2 * DB_MAXTUP)
#define			CPY_MAXDOMAIN	DB_MAXTUP
#define			CPY_MAX_DELIM	2

#define			CPY_ESCAPE	'\\'
#define			CPY_DUMMY_TYPE	-1
#define			CPY_TEXT_TYPE	-2	/* not used */

/* Special faked-up delimiters for CSV and SSV forms.  These are passed as
** delimiters rather than some other sort of flag, to avoid having to
** change any structures in the copy map protocol.
** (csv = comma separated value, ssv = semicolon separated value;  both
** obey double-quote escaping with quote doubling.)
*/
#define			CPY_CSV_DELIM	'\001'
#define			CPY_SSV_DELIM	'\002'

/* These flags are used by both the front-end in II_CP_STRUCT cp_status,
** and the back-end in the QEU_COPY qeu_stat status.  The front-end
** status is initialized from a value sent by the back-end.
** I'm not clear on which flags are set and used where;  an
** investigation for someone else.  [kschendel]
*/

#define			CPY_OK			0x00000000
#define			CPY_FAIL		0x00000001
#define			CPY_ERROR		0x00000002
#define			CPY_WARN		0x00000004
/*	notused, was done			0x00000008 */
#define			CPY_NOTARG		0x00000010
#define			CPY_NOTBIN		0x00000020
/*	notused, was vartups			0x00000040 */
#define			CPY_PROGRAM		0x00000080
/*	notused, was network			0x00000100 */
/*	notused, was nosync			0x00000200 */
#define			CPY_NOABORT		0x00000400
#define			CPY_CONTINUE		0x00000800
/*	notused, was empty			0x00001000 */
#define			CPY_DMR_CB		0x00002000
