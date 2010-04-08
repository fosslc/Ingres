/*
** Copyright (c) 2003, 2004 Computer Associates Intl. Inc. All Rights Reserved.
**
** This is an unpublished work containing confidential and proprietary
** information of Computer Associates.  Use, disclosure, reproduction,
** or transfer of this work without the express written consent of
** Computer Associates is prohibited.
*/

/* Name: gcadmint.h
**
** Description:
**	Global definitions for GCADM  
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
**	 19-Sept-2004 (wansh01)
**	    added gca_rg_parm in GCADM_SCB.parms.
*/

#ifndef _GCADMINT_INCLUDED_
#define _GCADMINT_INCLUDED_

#include <qu.h>

#define	GCADM_LOG_ID		"GCADM"

/* FSM state external input  */ 
#define  GCADM_RQST_NEW_SESSION	  30
#define  GCADM_RQST_ERROR	  31
#define  GCADM_RQST_DONE	  32
#define  GCADM_RQST_RSLT	  33
#define  GCADM_RQST_RSLTLST       34
#define  GCADM_RQST_DISC          35

#define  GCADM_MIN_BUFLEN	1024
/*
** Name: GCADM_GLOBAL
**
** Description:
**	GCADM global information.
**
** History:
**	09-Sept-03 (wansh01)
**	    created.
*/

typedef struct
{
    QUEUE 		scb_q;
    QUEUE 		rcb_q;
    i4         		gcadm_trace_level;
    i4 			gcadm_state;		
#define GCADM_IDLE	0x0000 
#define GCADM_ACTIVE	0x0001 
#define GCADM_TERM	0x0002 
    u_i4       		id_tdesc;
    i4			gcadm_buff_len;
    GCADM_ALLOC_FUNC 	alloc_rtn;
    GCADM_DEALLOC_FUNC 	dealloc_rtn;
    GCADM_ERRLOG_FUNC 	errlog_rtn;
}GCADM_GLOBAL;



GLOBALREF  GCADM_GLOBAL  GCADM_global;
 
      
/*
** Name: GCADM_SCB - session control block.
**
** Description:
**	Information describing a session  
**	
**
** History:
**	09-Sept-03 (wansh01)
*/  

typedef struct
{
    QUEUE		q;
    STATUS		status;
    u_i2		sequence;       /* current state    */
    i4			aid;		/* client association id */ 
    PTR			gca_cb;
    i4			admin_flag;		
#define GCADM_COMP_MASK	0x0001 
#define GCADM_DISC	0x0002 
#define GCADM_CALLBACK	0x0004 
#define GCADM_ERROR	0x0008 
    GCADM_RQST_FUNC	rqst_cb;
    PTR			rqst_cb_parms;
    GCADM_CMPL_FUNC	rslt_cb;
    PTR			rslt_cb_parms;
    QUEUE		*rsltlst_end;
    GCADM_RSLT		*rsltlst_entry;
    GCA_TD_DATA		tdesc_buf; 	 		
    union
    { 
      GCA_RV_PARMS	gca_rv_parm;
      GCA_SD_PARMS	gca_sd_parm;
      GCA_DA_PARMS	gca_da_parm;
      GCA_RG_PARMS	gca_rg_parm;
      GCA_ALL_PARMS	gca_all_parm;
    } parms;
    i4			msg_type;
    i4			row_count;
    i4			row_len;
    int			buf_len; 
    char		*buffer; 	 		
   	
}GCADM_SCB;

/*
** Name: GCADM_RCB - request control block.
**
** Description:
**	Information describing a request  
**	
**
** History:
**	09-Sept-03 (wansh01)
*/  

typedef struct
{
    QUEUE		q;
    STATUS		status;
    u_i1		event;		/* input event 		*/
    i4			operation;	/* GCA operation	*/
    GCADM_SCB		*scb;
}GCADM_RCB;


FUNC_EXTERN void	gcadm_smt_init();
FUNC_EXTERN STATUS 	gcadm_request( u_i1, GCADM_SCB * );
FUNC_EXTERN GCADM_SCB * gcadm_new_scb( i4, PTR );
FUNC_EXTERN void	gcadm_free_scb( GCADM_SCB * );
FUNC_EXTERN void	gcadm_errlog( STATUS );
FUNC_EXTERN int 	gcadm_format_err_data( char *, char *, i4, i4, char *);


#endif /* _GCADMINT_INCLUDED */




