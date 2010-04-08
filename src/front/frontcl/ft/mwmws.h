/*
**	MWmws.h
**	"@(#)mwmws.h	1.5"
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	MWmws.h - MacWorkStation protocol message interface.
**
** Usage:
**	INGRES FE system and MWhost on Macintosh.
**
** Description:
**	Contains the Pascal interface declarations for the MWhost module.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**/


/* Message Classes */
# define kCmdAlert		'A'
# define kCmdCursor		'C'
# define kCmdDialog		'D'
# define kCmdFile		'F'
# define kCmdGraphic		'G'
# define kCmdList		'L'
# define kCmdMenu		'M'
# define kCmdProcess		'P'
# define kCmdText		'T'
# define kCmdWindow		'W'
# define kCmdExec		'X'

/* Indexes into message array */
# define kMsgClass		0
# define kMsgID			1
# define kMsgParms		4
# define kMsgMaxData		550

/* Max. size of messages between host and MWS */
# define mwMAX_MSG_LEN		4095
/* Max size of each message packet */
# define mwPACKET_SZ		250

/* Message packet sequencing range */
# define mwPACKET_SEQ_LO	'A'
# define mwPACKET_SEQ_HI	'Z'
# define mwPACKET_SEQ_FORCE	'?'

/* Constants for getting the checksum */
# define mwPKTSUM_RSHIFT	2
# define mwPKTSUM_MASK		0x3f
# define mwPKTSUM_ADD		0x20

/* Communication framing characters */
# define mwSTART_CHAR		'\002'
# define mwSTOP_CHAR		'\001'
# define mwBLKMSG_DELIMIT	'\002'

/*
**	Message data structure used by parsing routines.
**	It is compatable with Apple's Pascal Test Host application.
*/
typedef struct
{
	i4	msgIndex;		/* Index into message */
	i4	msgLength;		/* Length of message */
	bool	msgValid;		/* TRUE if valid message */
	bool	filler;			/* to match Pascal declaration */
	char	msgData[mwMAX_MSG_LEN];
} TRMsg, *TPMsg;
