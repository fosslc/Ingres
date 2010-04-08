/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	impfrs.sc
**
** FRS routines for the new IMA based IPM lookalike tool.
**
** Basically - whenever a new form is added to the utility - it's name should 
** be added in here and an 'addform' done.
**
## History:
##
## 26-jan-94	(johna)
##		started on RIF day from pervious ad-hoc programs
## 04-may-94	(johna)
##		commented.
## 22-May-95 	(johna)
##		Added imp_server_diag - server internals diagram
##		Added ima_dmf_cache_stats -
## 21-Jun-95 	(johna)
##		Added ima_qsf_cache_stats
## 22-Jun-95 	(johna)
##		Added ima_qsf_dbp
## 07-Jul-95 	(johna)
##		Added ima_qsf_rqp and ima_rdf_cache_info ima_dio
## 08-Jul-95 	(johna)
##		Added ima_di_slave_info
##
## 02-Jul-2002 (fanra01)
##              Sir 108159
##              Added to distribution in the sig directory.
## 03-Sep-03 	(flomi02) mdf030903
##		Added imp_query_info
## 04-Sep-03 	(flomi02) mdf040903
##		Added imp_transaction_detail
##		Added imp_transaction_hexdump
##		Added imp_gcc_info
## 06-Apr-04    (srisu02) 
##              Sir 108159
##              Integrating changes made by flomi02  
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "imp.h"

exec sql include sqlca;

exec sql begin declare section;

extern int imp_client_info;
extern int imp_query_info;    
/* 
##mdf030903 
*/
extern int imp_lg_transactions;
extern int imp_lg_databases;
extern int imp_lg_processes;
extern int imp_lg_header;
extern int imp_main_menu;
extern int imp_server_list;
extern int imp_session_detail;
extern int imp_session_list;
extern int imp_server_popup;
extern int imp_server_parms;
extern int imp_lock_summary;
extern int imp_lg_menu;
extern int imp_lg_summary;
extern int imp_lk_menu;
extern int imp_lk_list;
extern int imp_lk_tx;
extern int imp_lk_locks;
extern int imp_res_locks;
extern int imp_res_list;
extern int imp_server_diag;
extern int imp_dmf_cache_stats;
extern int ima_qsf_cache_stats;
extern int ima_qsf_dbp;
extern int ima_qsf_rqp;
extern int ima_rdf_cache_info;
extern int ima_dio;
extern int ima_di_slave_info;
extern int imp_dmfcache;
extern int imp_transaction_detail;   
/*
##  mdf040903 
*/
extern int imp_transaction_hexdump;   
extern int imp_gcc_info;   

extern int inFormsMode;

exec sql end declare section;


int
setupFrs()
{
	exec frs forms;
	exec frs addform :imp_main_menu;
	exec frs addform :imp_server_list;
	exec frs addform :imp_session_list;
	exec frs addform :imp_server_popup;
	exec frs addform :imp_session_detail;
	exec frs addform :imp_server_parms;
	exec frs addform :imp_lock_summary;
	exec frs addform :imp_lk_menu;
	exec frs addform :imp_lk_list;
	exec frs addform :imp_lk_tx;
	exec frs addform :imp_lk_locks;
	exec frs addform :imp_res_locks;
	exec frs addform :imp_res_list;
	exec frs addform :imp_lg_menu;
	exec frs addform :imp_lg_summary;
	exec frs addform :imp_lg_header;
	exec frs addform :imp_lg_processes;
	exec frs addform :imp_lg_databases;
	exec frs addform :imp_lg_transactions;
	exec frs addform :imp_client_info;
	exec frs addform :imp_query_info;
	exec frs addform :imp_server_diag;
	exec frs addform :imp_dmf_cache_stats;
	exec frs addform :imp_dmfcache;
	exec frs addform :ima_qsf_cache_stats;
	exec frs addform :ima_qsf_dbp;
	exec frs addform :ima_qsf_rqp;
	exec frs addform :ima_rdf_cache_info;
	exec frs addform :ima_dio;
	exec frs addform :ima_di_slave_info;
	/*exec frs addform :imp_gcn_info;   */
	exec frs addform :imp_transaction_detail;     
/*
## mdf040903 
*/
	exec frs addform :imp_transaction_hexdump;     
	exec frs addform :imp_gcc_info;     
	inFormsMode = TRUE;
	return(TRUE);
}

int
closeFrs()
{
	if (inFormsMode) {
		exec frs clear screen;
		exec frs endforms;
		inFormsMode = FALSE;
	}
	return(TRUE);
}
