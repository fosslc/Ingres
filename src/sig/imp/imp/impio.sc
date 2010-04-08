/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	impio.sc
**
** This file contains all of the routines needed to support the "DI" 
** Subcomponent of the IMA based IMP lookalike tools.
**
## History:
##
##	07-Jul-95	(johna)
##		Created.
##      02-Jul-2002 (fanra01)
##          Sir 108159
##          Added to distribution in the sig directory.
##	04-Sep-03   (flomi02) mdf040903
##		Added link to Session Info
##      06-Apr-04    (srisu02) 
##              Sir 108159
##              Integrating changes made by flomi02  
##	08-Oct-2007 (hanje04)
##	    SIR 114907
##	    Remove timebuf as it's not referenced and causes compilation
##	    errors on Mac OSX
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "imp.h"

exec sql include sqlca;
exec sql include 'impcommon.sc';

SessEntry *newSessEntry();

exec sql begin declare section;


extern 	int 	ima_dio;
extern 	int 	inFormsMode;
extern 	int 	imaSession;
extern 	int 	iidbdbSession;
extern 	int 	userSession;
extern 	char	*currentVnode;

extern char    bar_chart[51];
extern float   pct_used;

exec sql end declare section;

/*
** Name:	impiomon.sc
**
** Display the relative I/O rates for this server
**
*/

exec sql begin declare section;
extern int 	timeVal;
static int	highVal = 0;
exec sql end declare section;

int
processIoMonitor(server)
exec sql begin declare section;
GcnEntry	*server;
exec sql end declare section;
{
	exec sql begin declare section;
	char	server_name[SERVER_NAME_LENGTH];
        char            thread[20];          
/*
##mdf040903 
*/
        int             scb_pid;   
/*
##mdf240402
*/
        char            msgbuf[40];          
/*
##mdf040903 
*/
        SessEntry        *current_sess;   
        SessEntry        *SessListHead;
	exec sql end declare section;

	exec sql set_sql (printtrace = 1);
        strcpy(server_name,currentVnode);   
	strcat(server_name,"::/@");
	strcat(server_name,server->serverAddress);

	exec sql declare global temporary table session.dio_table (
	        l_server        varchar(25),
                l_session       varchar(25),
                l_terminal      varchar(50),
                l_user          varchar(50),
                l_inuse         integer,
                l_dio           integer,
                l_old_dio       integer
        ) on commit preserve rows
          with norecovery;
	
	exec frs display ima_dio read;
	exec frs initialize;
	exec frs begin;
		exec frs putform ima_dio (server = :server_name);
	exec frs clear field highval;                
	highVal = 0;                                
 /*
## mdf040903
*/
		displayIoSummary(server_name);
		exec frs set_frs frs (timeout = :timeVal);

	exec frs end;

	exec frs activate timeout;
	exec frs begin;
		displayIoSummary(server_name);
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		displayIoSummary(server_name);
	exec frs end;

        exec frs activate menuitem 'Session_Info';
        exec frs begin; 
            exec frs getrow ima_dio ima_dio
                        (:thread = session_id);                       

            /* current_sess = SessListHead; */

            if (current_sess = newSessEntry()) {                    
                strcpy(current_sess->sessionId,thread );            
                /*current_sess->scb_pid = scb_pid;     
/*
##mdf240402
*/  
                }                                                   
                                                        

            if (current_sess) {                                   
                    displaySessionDetail(current_sess,server_name);  
            }                                                     
        exec frs end;                                                         

	exec frs activate menuitem 'Reset';
	exec frs begin;
	    highVal = 0;
	    exec frs clear field highval;                
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	exec frs set_frs frs (timeout = 0);
	exec sql drop table session.dio_table;
	exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

displayIoSummary(server_name)
exec sql begin declare section;
    char    *server_name;
exec sql end declare section;
{
    exec sql begin declare section;
        int i;
        int cnt;
        int blks;
        int j;
        float scale;
        char     msgBuf[100];
        char    local_server[50];
        char    local_terminal[50];
        char    local_user[50];
        char    local_session[25];
        char    local_euser[50];
        int    local_cpu;
        int    local_new_dio;
        int    local_old_dio;
        int    local_dio;
        int    local_bio;
    exec sql end declare section;

    exec sql update session.dio_table set l_inuse = 0;
    exec sql commit;
    exec frs set_frs menu '' (active('Session_Info') = off); 

    exec sql declare dio_csr cursor for select 
        s.server, s.session_id, terminal, username, dio
    from 
        imp_scs_sessions s, imp_cs_sessions c
    where
        c.server = s.server and s.session_id = c.session_id
        and s.server = :server_name  
/*
##mdf040903
*/
        ;

    exec sql open dio_csr for readonly;
    exec sql fetch dio_csr into 
        :local_server,
        :local_session,
        :local_terminal,
        :local_user,
        :local_dio;

    while (sqlca.sqlcode == 0) 
    {
        /*
        ** look for the row in the session.dio_table
        ** - if it exists, calculate the new cpu
        ** otherwise - insert a new row
        */
        exec sql update session.dio_table set 
            l_old_dio = l_dio,
            l_dio = :local_dio,
            l_inuse = 1
        where
            l_server = :local_server 
        and    l_session = :local_session 
        and    l_terminal = :local_terminal;
        exec sql inquire_sql (:cnt = rowcount);
        if (cnt == 0) {
            exec sql insert into session.dio_table values
            (       :local_server,
                    :local_session,
                    :local_terminal,
                    :local_user,
                    1,
                    :local_dio,
                    0);
        }

        exec sql fetch dio_csr into 
            :local_server,
            :local_session,
            :local_terminal,
            :local_user,
            :local_dio;
    }
    exec sql close dio_csr;

    exec sql delete from session.dio_table where l_inuse = 0;
    sqlerr_check();
    exec sql commit;

    exec frs inittable ima_dio ima_dio;
    i = 1;
    exec sql select 
        l_server,
        l_session,
        l_terminal,
        l_user,
        l_dio,
        l_old_dio,
        (l_dio - l_old_dio) as l_new_dio
    into
        :local_server,
        :local_session,
        :local_terminal,
        :local_user,
        :local_dio,
        :local_old_dio,
        :local_new_dio
    from 
        session.dio_table
    order by
        l_new_dio desc;

    exec sql begin;
        if ((local_new_dio != 0) && (local_old_dio != 0)) 
        {
            /*
            ** calc the barchart
            */
            if (i == 1) {
                cnt = local_new_dio;
                if (cnt > highVal) {
                    highVal = cnt;
                } else {
                    cnt = highVal;
                }
            }
            /*
            ** fill the barchart - top row is always full
            */
            scale = ((float) local_new_dio / cnt);
            blks = (scale * 40) ;
            sprintf(msgBuf,"Scale %f, blks %d",scale,blks); 
            for (j = 0; j < blks; j++) {
                bar_chart[j] = '%';
            }
            bar_chart[j] = '\0';
            exec frs insertrow ima_dio ima_dio (
                session_id = :local_session,
                terminal = :local_terminal,
                username = :local_user,
                barchart = :bar_chart,
                io_rate = :local_new_dio);
            i++;
            exec frs set_frs menu '' (active('Session_Info') = on);
        }
    exec sql end;
    sqlerr_check();
    exec frs putform ima_dio (highval = :highVal);
    exec frs scroll ima_dio ima_dio to 1; 
/*
##mdf040903
*/ 
}



