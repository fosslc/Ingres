/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	imp_gcn.sc
**
**
## History:
##
##	04-Sep-03	(flomi02)
##			created to display Name Server Info
##      06-Apr-04       (srisu02) 
##                      Sir 108159
##                      Integrating changes made by flomi02  
##	08-Oct-2007 (hanje04)
##	    SIR 114907
##	    Remove timebuf as it's not referenced and causes
##	    compiler errors on Mac OSX
*/

#include <stdio.h>                                 
#include <stdlib.h>                                
#include <ctype.h>                                 
#include <string.h>                                
#include "imp.h"                                   
                                                   
exec sql include sqlca;                            
exec sql include 'impcommon.sc';                   
                                                   
exec sql begin declare section;                    
                                                   
exec sql include  'imp_gcn_info.incl' ;    
/*
##mdf040903
*/  
                                                   
extern  int     imp_dmf_cache_stats;               
extern  int     inFormsMode;                       
extern  int     imaSession;                        
extern  int     iidbdbSession;                     
extern  int     userSession;                       
extern  char    *currentVnode;                     
                                                   
exec sql end declare section;                      

exec sql begin declare section;   
extern int      timeVal;          
exec sql end declare section;     

void displayGcnInfo();

/*
** More Info for GCN
**
**
## History:
##      04-Sep-03       (mdf040903)                     
*/
int impGcnInfo()
{
    exec sql begin declare section;
	int	i;
	char	buf[60];
    exec sql end declare section;

	exec frs display imp_gcn_info read;
	exec frs initialize;
	exec frs begin;
		exec frs set_frs frs (timeout = :timeVal);

		exec sql repeated select dbmsinfo('ima_vnode') into :buf;
		sqlerr_check();
		exec sql execute procedure imp_reset_domain;
		exec sql execute procedure imp_set_domain(entry=:currentVnode);
		exec sql commit;  
                    displayGcnInfo();

	exec frs end;


	exec frs activate menuitem 'Refresh';
	exec frs begin;
		displayGcnInfo();
	exec frs end;

	exec frs activate timeout;
	exec frs begin;
		displayGcnInfo();
	exec frs end;


	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
		exec frs set_frs frs (timeout = 0);
		exec frs breakdisplay;
	exec frs end;

	exec sql execute procedure imp_complete_vnode;
	exec sql commit;   
/*
##mdf040903
*/
	return(TRUE);
}

void displayGcnInfo()
{
    exec sql begin declare section;      
        char l_msgbuf[100];                    
    exec sql end declare section;        

    exec sql repeated select *
    into :gcninfo                                                   
    from imp_gcn_info;
    if (sqlerr_check() == NO_ROWS) {                              
        exec sql commit;    
        exec frs set_frs frs (timeout = 0);
        sprintf(l_msgbuf,"No information for Name Server ");
        exec frs message :l_msgbuf with style = popup;                  
    }
    else { 
        exec sql commit;                                           

	exec frs putform imp_gcn_info (name_server = :gcninfo.name_server);
	exec frs putform imp_gcn_info (default_class= :gcninfo.default_class);
	exec frs putform imp_gcn_info (local_vnode= :gcninfo.local_vnode);
	exec frs putform imp_gcn_info (remote_vnode= :gcninfo.remote_vnode);
	exec frs putform imp_gcn_info (cache_modified= :gcninfo.cache_modified);
	exec frs putform imp_gcn_info (bedcheck_interval= :gcninfo.bedcheck_interval);
	exec frs putform imp_gcn_info (compress_point= :gcninfo.compress_point);
	exec frs putform imp_gcn_info (expire_interval= :gcninfo.expire_interval);
	exec frs putform imp_gcn_info (hostname= :gcninfo.hostname);
	exec frs putform imp_gcn_info (installation_id = :gcninfo.installation_id);
	exec frs putform imp_gcn_info (remote_mechanism= :gcninfo.remote_mechanism);
	exec frs putform imp_gcn_info (ticket_cache_size= :gcninfo.ticket_cache_size);
	exec frs putform imp_gcn_info (ticket_expire= :gcninfo.ticket_expire);
	exec frs putform imp_gcn_info (ticket_lcl_cache_miss= :gcninfo.ticket_lcl_cache_miss);
	exec frs putform imp_gcn_info (ticket_lcl_created= :gcninfo.ticket_lcl_created);
	exec frs putform imp_gcn_info (ticket_lcl_expired= :gcninfo.ticket_lcl_expired);
	exec frs putform imp_gcn_info (ticket_lcl_used= :gcninfo.ticket_lcl_used);
	exec frs putform imp_gcn_info (ticket_rmt_cache_miss= :gcninfo.ticket_rmt_cache_miss);
	exec frs putform imp_gcn_info (ticket_rmt_created= :gcninfo.ticket_rmt_created);
	exec frs putform imp_gcn_info (ticket_rmt_expired= :gcninfo.ticket_rmt_expired);
	exec frs putform imp_gcn_info (ticket_rmt_used= :gcninfo.ticket_rmt_used);
	exec frs putform imp_gcn_info (timeout= :gcninfo.timeout);
	exec frs putform imp_gcn_info (trace_level= :gcninfo.trace_level);
    }
}

