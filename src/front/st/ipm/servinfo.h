/*
**      Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
*/

/*
** servinfo.h
** This file contains operation codes which lockinfo specifies to get
** the appropriate info from either the INGRES name server or the DMBS
** server.  Also specifies operation codes to shutdown servers, sessions
**
** History
**	21-mar-89	tomt	written
**	27-mar-89	tomt	created new operation codes and changed others
**	29-mar-89	tomt	created new operation code (FORMAT SESSION)
**	28-jun-89	tomt	moved external/for ref into this file
**	23-aug-89	tomt	added operations for load_servinfo()
**	23-aug-89	tomt	added operations for dispsrvinfo()
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    23-Sep-2009 (hanal04) bug 115316
**        Add GET_SERVER_CAP to retrieve server capabilities.
*/

/*
** External and forward references
*/
i4	get_name_info(); /* entry point into routines that get nmsrv info */
STATUS	dosrvoper();	/* retrieve info from DBMS server */
i4	dispsrv();	/* main server display routine */
i4      load_servinfo(); /* gets server list and info from gcn and server */
VOID	dispsrvinfo();	/* displays info on appropriate form */
VOID	servdetail();
STATUS	load_servinfo();

/*
** define different operation codes for get_name_info and get_serv_info
*/
#define GC_NAME_INIT		1	/* initialize connection to GCA for nm*/
#define GC_NAME_CONNECT		2	/* connect to nm serv to get info */
#define GC_NAME_DISCONNECT	3	/* disassoc conn to nm serv and 
					** terminate GCA association
					*/

#define NAME_SHOW		4	/* get server names from name server */

#define GC_SERV_INIT		5	/* init GCA to connect to DBMS server */
#define GC_SERV_CONNECT		6	/* connect to a specified DBMS server */
#define GC_SERV_DISCONNECT	7	/* disconnect from a DBMS server */
#define GC_SERV_TERMINATE	8	/* terminate GCA association */

#define GET_SERVER_INFO		9	/* get server info from DBMS server(s)*/
#define GET_SESSION_INFO	10	/* get info on sessions in the server */
#define GET_FORMAT_SESSION	11	/* get session detail */
#define SET_SERVER_SHUT		12	/* spin down current DBMS server nice */
#define STOP_SERVER		13	/* stop current DBMS server hard */
#define SUSPEND_SESSION		14	/* suspend current session in current
					** DBMS server
					*/
#define RESUME_SESSION		15	/* resume current sess in curr serv */
#define REMOVE_SESSION		16	/* stop current session in curr serv */

#define GET_SERVER_CAP          17      /* get server capabilities from DBMS */

/*
** Define operation codes for load_servinfo()
*/
#define GET_SERVLIST 		0	/* get list of servers only */
#define GET_SERVINFO		1	/* get info about one or all servers */
#define GET_ALLINFO		2	/* get list of servers and info */

/*
** Define operation codes for dispsrvinfo()
*/
#define DISP_SERV_LIST		0	/* A list of servers */
#define DISP_SERV_INFO  	1	/* Info on a specified server i.e. quantum etc. */
#define DISP_SESS_LIST  	2	/* Session list for a specified server */
#define DISP_SESS_INFO		3	/* Info on a particular session */
