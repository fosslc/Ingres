/*
** Copyright (c) 2004 Ingres Corporation
**
** Name: iomon.sc
**
** Simple monitor for DIO in servers in the current installation. 
**
** This utility uses two IMA tables and a database procedure defined in the
** iomon.sql file to provide a simple barchart display of relative disk I/O
** usage by sessions in servers in the current installation.
**
** The monitor is inspired by the VMS 'MONITOR PROCESS/TOPDIO' screen.
**
** As the refresh rate is configurable and the maximum I/O per sample period 
** is unknown, the most resource-hungry session is shown with a full
** bar, all other sessions being shown relative to that session.
**
** The database is assumed to be 'imadb' unless overridden by a command
** line argument.
**
** THIS PROGRAM IS PROVIDED AS AN EXAMPLE OF USING IMA TABLES ONLY. ERROR
** CHECKING IS DELIBERATELY SIMPLISTIC.
** 
** History:
**
** 23-Dec-93 (johna)	
**	Written.
** 31-May-95 (johna)
**	Reworked as an example program for the IMA sig directory.
*/

#include <stdio.h>
#include <stdlib.h>

#define TRUE 	1
#define FALSE 	0

EXEC SQL INCLUDE SQLCA;

EXEC SQL BEGIN DECLARE SECTION;
extern 	int dio_top;	/* the form */
char 	bar_chart[51];
int	in_forms_mode = FALSE;
EXEC SQL END DECLARE SECTION;

int
main(argc,argv)
int	argc;
char	*argv[];
{
	EXEC SQL BEGIN DECLARE SECTION;
	int 	sleeptime = 5;
	char 	db_name[20];
	char 	prompt_buf[20];
	char 	message_buf[1024];
	int 	newtime;
	exec sql end declare section;

	strcpy(db_name,"imadb");

	while (--argc > 0) {
		if (strncmp(argv[argc],"-r",2) == 0) {
			if ((newtime = atoi((char *) &argv[argc][2])) != 0) {
				sleeptime = newtime;
			}
		} else {
			strcpy(db_name,argv[argc]);
		}
	}
	printf("Using database %s with refresh at %d second%s\n",
		db_name,
		sleeptime,
		(sleeptime == 1) ? "." : "s.");
	sleep(2);

	EXEC SQL WHENEVER SQLERROR CALL sqlerr_check;
	EXEC SQL CONNECT :db_name;
	EXEC SQL EXECUTE PROCEDURE ima_set_vnode_domain;
	EXEC SQL DECLARE 
		GLOBAL TEMPORARY TABLE session.dio_table (
		l_server	varchar(25),
		l_session	varchar(25),
		l_terminal	varchar(50),
		l_user		varchar(50),
		l_inuse		integer,
		l_dio		integer,
		l_old_dio	integer
	) ON COMMIT PRESERVE ROWS WITH NORECOVERY;

	EXEC FRS FORMS;
	in_forms_mode = TRUE;
	EXEC FRS ADDFORM :dio_top;

	EXEC FRS DISPLAY dio_top READ;
	EXEC FRS INITIALIZE;
	EXEC FRS BEGIN;
		EXEC SQL SELECT DBMSINFO('IMA_VNODE') INTO :prompt_buf;
		EXEC FRS PUTFORM dio_top (vnode = :prompt_buf);
		EXEC FRS INITTABLE dio_top dio_tbl;
		display_graph();
		EXEC FRS SET_FRS FRS (TIMEOUT = :sleeptime);
	EXEC FRS END;

	EXEC FRS ACTIVATE TIMEOUT;
	EXEC FRS BEGIN;
		display_graph();
	EXEC FRS END;

	EXEC FRS ACTIVATE MENUITEM 'Change Timeout';
	EXEC FRS BEGIN;
		EXEC FRS SET_FRS FRS (TIMEOUT = 0);
		EXEC FRS PROMPT ('Enter the new timeout value', :prompt_buf) 
			WITH STYLE = POPUP;
		if ((newtime = atoi(prompt_buf)) == 0) {
			sprintf(message_buf,"%s is not a valid timeout value",
				prompt_buf);
			EXEC FRS MESSAGE :message_buf WITH STYLE = POPUP;
		} else {
			sleeptime = newtime;
		}
		EXEC FRS SET_FRS FRS (TIMEOUT = :sleeptime);
	EXEC FRS END;

	EXEC FRS ACTIVATE MENUITEM 'End', FRSKEY3;
	EXEC FRS BEGIN;
		EXEC FRS BREAKDISPLAY;
	EXEC FRS END;

	EXEC FRS FINALIZE;
	EXEC SQL DISCONNECT;
	EXEC FRS CLEAR SCREEN;
	EXEC FRS ENDFORMS;
	exit(0);

}

sqlerr_check()
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	msg_buf[1024];
	int	currentConnection;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL INQUIRE_SQL (:msg_buf = ERRORTEXT);
	EXEC SQL INQUIRE_SQL (:currentConnection = SESSION);

	if (in_forms_mode) {
		EXEC FRS SET_FRS FRS (TIMEOUT = 0);
		EXEC FRS MESSAGE :msg_buf WITH STYLE = POPUP;
		EXEC FRS CLEAR SCREEN;
		EXEC FRS ENDFORMS;
	} else {
		printf("\n%s\n",msg_buf);
	}
	if (currentConnection) {
		EXEC SQL DISCONNECT;
	}
	exit(0);
}

display_graph()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int i;
	int cnt;
	int blks;
	int j;
	float scale;
	char	 msg_buf[100];
	char	local_server[50];
	char	local_terminal[50];
	char	local_user[50];
	char	local_session[25];
	char	local_euser[50];
	int	local_cpu;
	int	local_new_dio;
	int	local_old_dio;
	int	local_dio;
	int	local_bio;
	exec sql end declare section;



	EXEC SQL UPDATE session.dio_table SET l_inuse = 0;
	EXEC SQL COMMIT;

	EXEC SQL DECLARE dio_csr CURSOR FOR SELECT 
		s.server, s.session_id, session_terminal, real_user, dio
	FROM 
		ima_server_sessions s, ima_unix_sessions c
	WHERE
		c.server = s.server 
	AND 	s.session_id = c.session_id;

	EXEC SQL OPEN dio_csr;
	EXEC SQL FETCH dio_csr INTO 
		:local_server,
		:local_session,
		:local_terminal,
		:local_user,
		:local_dio;

	while (sqlca.sqlcode == 0) {
		/*
		** look for the row in the session.dio_table
		** - if it exists, calculate the new cpu
		** otherwise - insert a new row
		*/
		EXEC SQL UPDATE session.dio_table SET 
			l_old_dio = l_dio,
			l_dio = :local_dio,
			l_inuse = 1
		WHERE
			l_server = :local_server 
		AND	l_session = :local_session 
		AND	l_terminal = :local_terminal;
		EXEC SQL INQUIRE_SQL (:cnt = ROWCOUNT);
		if (cnt == 0) {
			EXEC SQL INSERT INTO session.dio_table VALUES
				(	:local_server,
					:local_session,
					:local_terminal,
					:local_user,
					1,
					:local_dio,
					0);
		}

		EXEC SQL FETCH dio_csr INTO 
			:local_server,
			:local_session,
			:local_terminal,
			:local_user,
			:local_dio;
	}
	EXEC SQL CLOSE dio_csr;

	EXEC SQL DELETE FROM session.dio_table WHERE l_inuse = 0;
	EXEC SQL COMMIT;

	EXEC FRS INITTABLE dio_top dio_tbl;
	i = 1;
	EXEC SQL SELECT 
		l_server,
		l_session,
		l_terminal,
		l_user,
		l_dio,
		l_old_dio,
		(l_dio - l_old_dio) as l_new_dio
	INTO
		:local_server,
		:local_session,
		:local_terminal,
		:local_user,
		:local_dio,
		:local_old_dio,
		:local_new_dio
	FROM 
		session.dio_table
	ORDER BY
		l_new_dio desc;

	EXEC SQL BEGIN;
		if ((local_new_dio != 0) && (local_old_dio != 0)) {
		/*
		** calc the barchart
		*/
			if (i == 1) {
				cnt = local_new_dio;
			}
			/*
			** fill the barchart - top row is always full
			*/
			scale = ((float) local_new_dio / cnt);
			blks = (scale * 20) ;
			sprintf(msg_buf,"Scale %f, blks %d",scale,blks); 
			for (j = 0; j < blks; j++) {
				bar_chart[j] = '%';
			}
			bar_chart[j] = '\0';
		EXEC FRS INSERTROW dio_top dio_tbl (
			server = :local_server,
			session_id = :local_session,
			terminal = :local_terminal,
			username = :local_user,
			barchart = :bar_chart,
			new_io = :local_new_dio);
			i++;
		}
	EXEC SQL END;

}



