/*
**static char	Sccsid[] = "@(#)mq_eglobs.h	30.1	11/14/84";
*/

/*
**	MQ_QEXECGLOBS.h  - This file contains the GLOBAL VARIABLES
**		     used in execution phase of QBF.
**
**	Copyright (c) 2004 Ingres Corporation
*/


/* Global variables */
extern	i4	mq_mwhereend;		/* ptr to end of master where clause */
extern	i4	mq_dwhereend;		/* ptr to end of detail where clause */
extern	char	*mqmrec;		/* buffer for master rec for retrieves*/
extern	char	*mqdrec;		/* buffer for detail rec for retrieves*/
extern 	char	*mqumrec;		/* buffer for master rec for updates */
extern	char	*mqudrec;		/* buffer for detail rec for updates */


/*
** Globals for parameterized getform statements
** to get _RECORD and _STATE info and store tids
*/
extern	i4	mq_record;
extern	i4	mqrstate;
extern	char	mq_tids[];
extern	bool	mq_second;

/* for recovery (kira) */
extern FILE 	*mqb_file;

