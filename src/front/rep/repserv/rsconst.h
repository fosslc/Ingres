/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef RSCONST_H_INCLUDED
# define RSCONST_H_INCLUDED

/**
** Name:	rsconst.h - Replicator Server constants
**
** Description:
**	Constants used by the Replicator Server.
**
** History:
**	16-dec-96 (joea)
**		Created based on rsconst.h in replicator library.
**	04-feb-97 (joea)
**		Eliminate QRY_*, TX_* and INGDATE_LENGTH defines.
**	23-jul-98 (abbjo03)
**		Eliminate INGERRTEXT_LENGTH.
**/

/* Quieting of target databases and cdds's */

#define NOT_QUIET	0			/* not quiet */
#define SERVER_QUIET	1			/* quieted by server */
#define USER_QUIET	2			/* quieted by user */


/* Collision mode */

# define COLLMODE_PASSIVE	0	/* detect simple coll.; no resolve */
# define COLLMODE_ACTIVE	1	/* detect benign coll.; no resolve */
# define COLLMODE_BENIGN	2	/* detect benign coll.; resolve */
# define COLLMODE_PRIORITY	3	/* resolve by priority lookup */
# define COLLMODE_TIME		4	/* resolve by last timestamp */


/* Error modes */

# define EM_SKIP_TRANS		0	/* continue replicating; skip trans. */
# define EM_SKIP_ROW		1	/* continue replicating; skip row */
# define EM_QUIET_CDDS		2	/* quiet target CDDS */
# define EM_QUIET_DB		3	/* quiet target database */
# define EM_QUIET_SERVER	4	/* quiet server */


/* error processing codes */

# define ERR_NEXT_TRANS		0	/* go to the next transaction */
# define ERR_WAIT_EVENT		1	/* go wait for an event */
# define ERR_REREAD		2	/* re-read the distrib queue */
# define ERR_NEXT_ROW		3	/* go to the next row */

#endif
