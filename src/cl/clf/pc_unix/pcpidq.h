/*
**Copyright (c) 2004 Ingres Corporation
 *
 *	Name:
 *		PC.h
 *
 *	Function:
 *		None
 *
 *	Arguments:
 *		None
 *
 *	Result:
 *		defines symbols for PC module routines
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		3/82 (gb) written
 *		21-may-91 (seng)
 *			Add int declaration in front of Pidq_init.
 *			RS/6000 compiler chokes on statement.
 *
 */

/**
	Structure to store PID for spawned process and it's status
**/
typedef struct _pidque
{   
	QUEUE 	pidq;
	PID	pid;
	STATUS	stat;
} PIDQUE;

GLOBALREF QUEUE	Pidq;
GLOBALREF int Pidq_init;

