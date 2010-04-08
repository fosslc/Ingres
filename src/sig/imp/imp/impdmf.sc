/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	impdmf.sc
**
** This file contains all of the routines needed to support the "DMF" 
** Subcomponent of the IMA based IMP lookalike tools.
**
## History:
##
##	08-Jul-95	(johna)
##		Incorporated from earlier DMF tools
##
##	15-mar-2002	(mdf150302)
##		The existing ima_dmf_cache_stats doesn't contain the necessary
##		columns for this routine to work.
##		This routine now looks at a different table imp_dmf_cache_stats.
##
##		Now caters for different page sizes.
*/

/*
##      10-Apr-02       (mdf100402)
##              Expand domain to handle looking at a different DMF.
##              Ideas for further enhancements.
##              -  Shared cache handling.
##
##      24-Apr-02       (mdf240402)
##              Commit after calling imp_complete_vnode
##
##      02-Jul-2002 (fanra01)
##          Sir 108159
##          Added to distribution in the sig directory.
##
##      04-Sep-03       (mdf040903)
##              Commit after selecting data
##              Add extra error handling.
##              Added extra dmf cache information onto form
##      06-Apr-04    (srisu02) 
##              Sir 108159
##              Integrating changes made by flomi02  
##      05-Oct-04    (srisu02) 
##              Removed fatal error message and instead added a popup message 
##              whenever the cache information was unavailable in the DMF when
##              the user selected the 'Internals' option
##      05-Oct-04    (srisu02) 
##              Removed unnecessary comments, corrected spelling mistakes 
##		in the code comments
##
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "imp.h"
#define ERRLEN 100
exec sql include sqlca;
exec sql include 'impcommon.sc';

void getDmfCache();

exec sql begin declare section;

exec sql include  'imp_dmf.incl' ;    
/*
## mdf150302 
*/

extern 	int 	imp_dmf_cache_stats;
extern 	int 	inFormsMode;
extern 	int 	imaSession;
extern 	int 	iidbdbSession;
extern 	int 	userSession;
extern 	char	*currentVnode;

exec sql end declare section;

/*
** Name:	impdmfsum.sc
**
** Display the DMF cache statistics summary
**
*/

exec sql begin declare section;
extern int 	timeVal;
char		timebuf[20];
exec sql end declare section;

int
processDmfSummary(server)
exec sql begin declare section;
GcnEntry	*server;
exec sql end declare section;
{
	exec sql begin declare section;
	char	server_name[SERVER_NAME_LENGTH];
            int page_size;
            int multicache;
            int cachechosen;
	exec sql end declare section;

	exec sql set_sql (printtrace = 1);
 
        exec sql execute procedure imp_complete_vnode;  
/*
## mdf100402 
*/
        exec sql commit;  
/*
## mdf240402 
*/
	strcpy(server_name,currentVnode);    
/*
## mdf040903 
*/
	strcat(server_name,"::/@");
	strcat(server_name,server->serverAddress);

	exec frs display imp_dmf_cache_stats update;
	exec frs initialize;
	exec frs begin;
		exec frs putform imp_dmf_cache_stats (server = :server_name);
		getDmfCache(server_name, &page_size, &multicache, &cachechosen);
                if (cachechosen == TRUE)
                {
                    /* Only show the SelectCache menuitem when relevant */
                    if (multicache == TRUE) 
                        {
                        exec frs set_frs menu '' (active('SelectCache') = on); 
                        }
	            else
                        {
                        exec frs set_frs menu '' (active('SelectCache') = off);
                        }
	            exec frs putform imp_dmf_cache_stats (page_size = :page_size);
		    getDmfSummary(server_name);
		    displayDmfSummary();
		    exec frs set_frs frs (timeout = :timeVal);
                }
                else
                {
	            exec frs set_frs frs (timeout = 0);
	            exec sql set_sql (printtrace = 0);
	            exec frs breakdisplay;
                }
	exec frs end;

	exec frs activate timeout;
	exec frs begin;
		getDmfSummary(server_name);
		displayDmfSummary();
	exec frs end;

	exec frs activate menuitem 'Flush';
	exec frs begin;
		clear_dmf();
		getDmfSummary(server_name);
		displayDmfSummary();
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		getDmfSummary(server_name);
		displayDmfSummary();
	exec frs end;

	exec frs activate menuitem 'SelectCache';
	exec frs begin;
		getDmfCache(server_name, &page_size, &multicache, &cachechosen);
                if (cachechosen == TRUE)
                {
	            exec frs putform imp_dmf_cache_stats (page_size = :page_size);
		    getDmfSummary(server_name);
		    displayDmfSummary();
                }
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	    exec frs set_frs frs (timeout = 0);
	    exec sql set_sql (printtrace = 0);
	    exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

clear_dmf()
{
	exec sql set trace point dm421;
}

getDmfSummary(server_name)
exec sql begin declare section;
char    *server_name;
exec sql end declare section;
{
    int i;
    int blks;
    exec sql begin declare section;
        int    page_size;
    exec sql end declare section;
    exec frs getform imp_dmf_cache_stats (:page_size = page_size);

    exec sql repeated select *
    into :dmfbuf
    from imp_dmf_cache_stats
    where server = :server_name
    and  page_size = :page_size;
    if (sqlerr_check() == NO_ROWS) {
        fatal("getDmfSummary: No information found for this cache");
    }


}

displayDmfSummary()
{
	exec sql begin declare section;
	char	bar[100];
	int pct;
	float	flt_pct;
	int	blks;
	int	i;
	int	free;
	int	dirty;
	int	modified;
	exec sql end declare section;
	exec frs putform imp_dmf_cache_stats (
		server = :dmfbuf.server,
		page_buffer_count = :dmfbuf.page_buffer_count,
		group_buffer_count = :dmfbuf.group_buffer_count,
		group_buffer_size = :dmfbuf.group_buffer_size,
		read_count = :dmfbuf.read_count,
		group_buffer_read_count = :dmfbuf.group_buffer_read_count,
		write_count = :dmfbuf.write_count,
		group_buffer_write_count = :dmfbuf.group_buffer_write_count,
		free_buffer_count = :dmfbuf.free_buffer_count,
		cache_status = :dmfbuf.cache_status,   
/* 
##mdf040903
##*/
		pages_still_valid = :dmfbuf.pages_still_valid,   
		pages_invalid = :dmfbuf.pages_invalid,   
		buffer_count = :dmfbuf.buffer_count,    
		free_buffer_waiters = :dmfbuf.free_buffer_waiters, 
		free_group_buffer_count = :dmfbuf.free_group_buffer_count,
		fixed_buffer_count = :dmfbuf.fixed_buffer_count,
		fixed_group_buffer_count = :dmfbuf.fixed_group_buffer_count,
		modified_buffer_count = :dmfbuf.modified_buffer_count,
		modified_group_buffer_count = :dmfbuf.modified_group_buffer_count,
		mlimit = :dmfbuf.mlimit,
		flimit = :dmfbuf.flimit,
		wbstart = :dmfbuf.wbstart,
		wbend = :dmfbuf.wbend,
		fix_count = :dmfbuf.fix_count,
		hit_count = :dmfbuf.hit_count,
		unfix_count = :dmfbuf.unfix_count,
		dirty_unfix_count = :dmfbuf.dirty_unfix_count,
		io_wait_count = :dmfbuf.io_wait_count,
		fc_wait = :dmfbuf.fc_wait,              
/*
##mdf040903
*/
		bm_gwait = :dmfbuf.bm_gwait,            
		bm_mwait = :dmfbuf.bm_mwait,           
		force_count = :dmfbuf.force_count);

	/* calculate the buffer stuff */

	if (dmfbuf.fix_count == 0) 
	    flt_pct = 0;
        else  
	    flt_pct = (100 * dmfbuf.hit_count) / dmfbuf.fix_count;
	pct = (int) flt_pct;
	blks = (pct * 45) / 100;
	for (i = 0; i < blks; i++) {
		bar[i] = '%';
	}
	bar[i] = '\0';
	exec frs putform imp_dmf_cache_stats (hit_ratio_pct = :flt_pct,
						hit_ratio = :bar);

	/* modified pages */
	pct = (100 * dmfbuf.modified_buffer_count) / dmfbuf.page_buffer_count;
	blks = (pct * 45) / 100;
	for ( i = 0;(i < blks) && (i < 45); i++) {
		bar[i] = 'D';
	}
	/* fixed but not modified */
	pct = (100 * (dmfbuf.page_buffer_count - (dmfbuf.free_buffer_count +
		dmfbuf.modified_buffer_count))) / dmfbuf.page_buffer_count;
	blks = (pct * 45) / 100;
	blks = blks + i;
	for (/* no start */; i < blks; i++) {
		bar[i] = 'u';
	}
	pct = (100 * dmfbuf.free_buffer_count) / dmfbuf.page_buffer_count;
	blks = (pct * 45) / 100;
	blks = blks + i;
	for (/* no start */; i < blks; i++) {
		bar[i] = 'f';
	}
	free = blks;

	bar[i] = '\0';
	exec frs putform imp_dmf_cache_stats (cache = :bar);

}

void getDmfCache(server_name , p_page_size , p_multicache, p_cachechosen)
exec sql begin declare section;
    char    *server_name;          
    int    *p_page_size;          
    int    *p_multicache;          
    int    *p_cachechosen;          
exec sql end declare section;  
{
    exec sql begin declare section;
        int     cnt;
        int     l_page_size;
        char    l_msgbuf[ERRLEN];
    exec sql end declare section;
    exec frs display imp_dmfcache read;
    exec frs initialize;
    exec frs begin;
        exec frs inittable imp_dmfcache cache ;
        exec sql select page_size 
                         into :l_page_size
                         from imp_dmf_cache_stats
        where server = :server_name  ;
        exec sql begin;                                                   
            exec frs insertrow imp_dmfcache cache (  
                         page_size = :l_page_size);
        exec sql end;                                                   
        sqlerr_check();    
/* 
##mdf040903 
*/
        exec sql inquire_sql (:cnt = rowcount);
        exec sql commit;    
/*
##mdf040903
*/
        if (cnt == 0) {
/*** A popup error message is displayed when no cache information is found ***/ 
            sprintf(l_msgbuf,"No information found for this cache");
            exec frs message :l_msgbuf with style = popup;
            exec frs breakdisplay;
                }                                   
        else if (cnt == 1) {                        
             *p_multicache = FALSE;
             *p_page_size = l_page_size;
             *p_cachechosen = TRUE;
             exec frs breakdisplay;
                }                                   
        else {                        
             *p_multicache = TRUE;
             exec frs scroll imp_dmfcache cache to 1; 
/*
##mdf040903
*/
                }                                   
    exec frs end;

    exec frs activate menuitem 'Select', frskey4;   
    exec frs begin;
        exec frs getrow imp_dmfcache cache (:l_page_size = page_size);
        *p_page_size = l_page_size;
        *p_cachechosen = TRUE;
        exec frs breakdisplay;
    exec frs end;

    exec frs activate menuitem 'Cancel', frskey3;
    exec frs begin;
        *p_cachechosen = FALSE;
        exec frs breakdisplay;
    exec frs end;

}
