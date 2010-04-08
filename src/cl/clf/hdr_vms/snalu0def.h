/********************************************************************************************************************************/
/* Created 11-DEC-1987 17:12:45 by VAX-11 SDL V3.0-2      Source: 19-SEP-1987 10:34:15 SNA$ENVIR:[AI.LU0.SRC]SNALU0DEF.SDL;14 */
/********************************************************************************************************************************/
 
/*** MODULE LU0LIBDEF IDENT V01.01 ***/
/*                                                                          */
/***************************************************************************** */
/**									     * */
/**  Copyright (C) 1987							     * */
/**  by Digital Equipment Corporation, Maynard, Mass.			     * */
/** 									     * */
/**  This software is furnished under a license and may be used and  copied  * */
/**  only  in  accordance  with  the  terms  of  such  license and with the  * */
/**  inclusion of the above copyright notice.  This software or  any  other  * */
/**  copies  thereof may not be provided or otherwise made available to any  * */
/**  other person.  No title to and ownership of  the  software  is  hereby  * */
/**  transferred.							     * */
/** 									     * */
/**  The information in this software is subject to change  without  notice  * */
/**  and  should  not  be  construed  as  a commitment by digital equipment  * */
/**  corporation.							     * */
/** 									     * */
/**  Digital assumes no responsibility for the use or  reliability  of  its  * */
/**  software on equipment which is not supplied by digital.		     * */
/**									     * */
/***************************************************************************** */
/**++                                                                       */
/** FACILITY:	SNA LU0 Subroutine Library                                  */
/**                                                                         */
/** ABSTRACT:	Data structure definitions that are external to the LU0     */
/**		modules. These definitions are public and must remain       */
/**		backwards compatible.                                       */
/**                                                                         */
/** ENVIRONMENT:                                                            */
/**                                                                         */
/**	VAX/VMS Operating System                                            */
/**                                                                         */
/** AUTHOR:  Mike Cambria, Distributed Systems Software Engineering         */
/**                                                                         */
/** CREATION DATE:  20-July-1984                                            */
/**                                                                         */
/** Modified by:                                                            */
/**                                                                         */
/** 1.00  20-Jul-1984  Mike Cambria                                         */
/**	  Initially written.                                                */
/**                                                                         */
/** 1.01  15-Sep-1985  Jonathan D. Belanger                                 */
/**	  Re-formatted entire .SDL file and cleaned up entry point definitions. */
/**                                                                         */
/**--                                                                       */
#define snalu0$k_positive_rsp 0
#define snalu0$k_negative_rsp 1
#define snalu0$k_normal_flow 0
#define snalu0$k_expedited_flow 1
#define snalu0$k_passive 0
#define snalu0$k_active 1
#define snalu0$k_request 0
#define snalu0$k_response 1
#define snalu0$k_mtype_lustat 4
#define snalu0$k_mtype_rtr 5
#define snalu0$k_mtype_bis 112
#define snalu0$k_mtype_sbi 113
#define snalu0$k_mtype_qec 128
#define snalu0$k_mtype_qc 129
#define snalu0$k_mtype_relq 130
#define snalu0$k_mtype_cancel 131
#define snalu0$k_mtype_chase 132
#define snalu0$k_mtype_sdt 160
#define snalu0$k_mtype_clear 161
#define snalu0$k_mtype_stsn 162
#define snalu0$k_mtype_rqr 163
#define snalu0$k_mtype_shutd 192
#define snalu0$k_mtype_shutc 193
#define snalu0$k_mtype_rshutd 194
#define snalu0$k_mtype_bid 200
#define snalu0$k_mtype_sig 201
#define snalu0$k_rsp_none 0
#define snalu0$k_rsp_rqd1 8
#define snalu0$k_rsp_rqd2 2
#define snalu0$k_rsp_rqd3 10
#define snalu0$k_rsp_rqe1 9
#define snalu0$k_rsp_rqe2 3
#define snalu0$k_rsp_rqe3 11
#define snalu0$k_st_min 1
#define snalu0$k_st_ses_reset 1
#define snalu0$k_st_ses_p_active 2
#define snalu0$k_st_ses_active 3
#define snalu0$k_st_ses_p_reset 4
#define snalu0$k_st_ses_max 4
#define snalu0$k_st_bsm_betb 1
#define snalu0$k_st_bsm_inb 2
#define snalu0$k_st_bsm_p_bb 3
#define snalu0$k_st_bsm_p_inb 4
#define snalu0$k_st_bsm_p_term_s 5
#define snalu0$k_st_bsm_p_term_r 6
#define snalu0$k_st_bsm_max 6
#define snalu0$k_st_chain_betc 1
#define snalu0$k_st_chain_inc 2
#define snalu0$k_st_chain_purge 3
#define snalu0$k_st_chain_max 3
#define snalu0$k_st_turn_cont 1
#define snalu0$k_st_turn_cont_s 2
#define snalu0$k_st_turn_cont_r 3
#define snalu0$k_st_turn_send 4
#define snalu0$k_st_turn_rcv 5
#define snalu0$k_st_turn_rcv_81b 6
#define snalu0$k_st_turn_erps 7
#define snalu0$k_st_turn_erpr 8
#define snalu0$k_st_turn_max 8
#define snalu0$k_st_qec_reset 1
#define snalu0$k_st_qec_pend 2
#define snalu0$k_st_qec_qeiesced 3
#define snalu0$k_st_qec_max 3
#define snalu0$k_min 0
#define snalu0$k_mclass_formatted_fm 0
#define snalu0$k_mclass_network_control 1
#define snalu0$k_mclass_dfc 2
#define snalu0$k_mclass_session_control 3
#define snalu0$k_mclass_unformatted_fm 4
#define snalu0$k_max 4
#define snalu0$k_min_status_vector 64
#define snalu0$k_min_notify_vector 64
struct constants {
    unsigned long int snalu0$l_dummy;   /* Keep SDL happy                   */
/* Initialize argument counter                                              */
    } ;
/*                                                                          */
/* Define all the SNALU0$xxx entry points                                   */
/*                                                                          */
/*                                                                          */
/* SNALU0$EXAMINE_STATE[_W]                                                 */
/*                                                                          */
long int SNALU0$EXAMINE_STATE() ;
long int SNALU0$EXAMINE_STATE_W() ;
/*                                                                          */
/* SNALU0$RECEIVE_MESSAGE[_W]                                               */
/*                                                                          */
long int SNALU0$RECEIVE_MESSAGE() ;
long int SNALU0$RECEIVE_MESSAGE_W() ;
/*                                                                          */
/* SNALU0$REQUEST_CONNECT[_W]                                               */
/*                                                                          */
long int SNALU0$REQUEST_CONNECT() ;
long int SNALU0$REQUEST_CONNECT_W() ;
/*                                                                          */
/* SNALU0$REQUEST_DISCONNECT[_W]                                            */
/*                                                                          */
long int SNALU0$REQUEST_DISCONNECT() ;
long int SNALU0$REQUEST_DISCONNECT_W() ;
/*                                                                          */
/* SNALU0$REQUEST_RECONNECT[_W]                                             */
/*                                                                          */
long int SNALU0$REQUEST_RECONNECT() ;
long int SNALU0$REQUEST_RECONNECT_W() ;
/*                                                                          */
/* SNALU0$TRANSMIT_MESSAGE[_W]                                              */
/*                                                                          */
long int SNALU0$TRANSMIT_MESSAGE() ;
long int SNALU0$TRANSMIT_MESSAGE_W() ;
/*                                                                          */
/* SNALU0$TRANSMIT_RESPONSE[_W]                                             */
/*                                                                          */
long int SNALU0$TRANSMIT_RESPONSE() ;
long int SNALU0$TRANSMIT_RESPONSE_W() ;
 
/* File:  LU0TXT.H    Created:  11-DEC-1987 17:13:46.87 */
 
#define SNALU0$_FACILITY	0X00000400
#define SNALU0$_BUGCHK	0X0400800C
#define SNALU0$_FREELU0VM	0X04008014
#define SNALU0$_UNFLEF	0X0400801C
#define SNALU0$_AEVTER	0X04008024
#define SNALU0$_ILEFWT	0X0400802C
#define SNALU0$_UNQAST	0X04008034
#define SNALU0$_UNDAST	0X0400803C
#define SNALU0$_INRCVER	0X04008044
#define SNALU0$_DFCERR	0X0400804C
#define SNALU0$_SETEVT	0X04008054
#define SNALU0$_INSRES	0X0400805A
#define SNALU0$_UNALEF	0X04008062
#define SNALU0$_EVTCLR	0X0400806A
#define SNALU0$_TOPOSRSP	0X04008072
#define SNALU0$_TONEGRSP	0X0400807A
#define SNALU0$_GETLU0VM	0X04008082
#define SNALU0$_PARERR	0X0400808A
#define SNALU0$_RCVBFSM	0X04008092
#define SNALU0$_ACQLU	0X0400809A
#define SNALU0$_FREELU	0X040080A2
#define SNALU0$_DISCFAIL	0X040080AA
#define SNALU0$_NOSESN	0X040080B2
#define SNALU0$_RCVFAIL	0X040080BA
#define SNALU0$_RCNFAIL	0X040080C2
#define SNALU0$_XMTFAIL	0X040080CA
#define SNALU0$_INVSESID	0X040080D2
#define SNALU0$_EXIT	0X040080DA
#define SNALU0$_NETSHUT	0X040080E2
#define SNALU0$_INVBUF	0X040080EA
#define SNALU0$_NOTVECTSM	0X040080F2
#define SNALU0$_NORMAL	0X040080F9
#define SNALU0$_INVFMBIND	0X04008102
