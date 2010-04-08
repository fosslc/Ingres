/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	keyparam.h -	ABF Run-time Keyword Parameter Descriptor
**					Definition File.
** Description:
**	Contains the definition of the internal ABF run-time system keyword
**	parameter descriptor.  Defines:
**
**	KEYWORD_PRM	keyword parameter descriptor.
**
** History:
**	Revision 8.0  89/07  wong
**	Initial revision.
*/

/*}
** Name:	KEYWORD_PRM -	Keyword Parameter Descriptor.
**
** Description:
**	Describes a keyword parameter passed to a 4GL frame or procedure.  This
**	is used internally to access the parameter value.
*/
typedef struct {
	char		*parent;	/* parent (frame or procedure) name */
	ER_MSGID	pclass;		/* parent class name */
	char		*name;		/* parameter name */
	DB_DATA_VALUE	*value;		/* parameter value */
} KEYWORD_PRM;
