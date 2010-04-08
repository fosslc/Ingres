
/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: raatint.h - Items used internally by the RAAT interface.
**
**  Description:
**	This file contains definitions for those items used internally by 
** 	the RAAT interface that are not to be exposed to the end user
**	by being placed in raat.h
**
** History:
**	11-feb-97 (cohmi01)
**	    Created.
**      01-dec-97 (stial01)
**          Added function prototypes.
**      18-jun-99 (shust01)
**          Change send_bkey() prototype, pass tid. To fix problem
**          with repositioning on secondary index when RAAT_BAS_RECDATA.
**          bug #97496.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
** Name: CHECK_RAAT_GCA_RET 
**
** Description: 
**	Macro to check return codes from GCA and
**	set the err_code field of the RAAT_CB accordingly.
**
** Inputs:
**	raat		pointer to RAAT_CB
**	gstat		status item passed as parm to IIGCa_call
**	gparm		pointer to GCA_PARMLIST block
**
** Outputs:
**	Sets the err_code field of RAAT_CB
** Returns:
**	Non-zero if error occured.
*/

#define CHECK_RAAT_GCA_RET(raat,gstat,gparm)	\
 ((raat)->err_code = ((gstat) != E_GC0000_OK) ? (gstat) :	\
 ((gparm)->gca_status != E_GC0000_OK) ? (gparm)->gca_status : 0  )
  


/*
** Name: CHECK_RAAT_GCA_RESPONSE 
**
** Description: 
** 	Macro to verify that a GCA_RESPONSE message type was received 
**	from GCA, and to set the err_code field of the RAAT_CB if not.
**
** Inputs:
**	raat		pointer to RAAT_CB
**	gi		pointer to GCA_IT_PARMS block
**
** Outputs:
**	Sets the err_code field of RAAT_CB if not a GCA_RESPONSE msg.
** Returns:
**	Non-zero if error occured.
*/

#define CHECK_RAAT_GCA_RESPONSE(raat,gi)  ( \
 ((gi)->gca_message_type == GCA_RESPONSE) ? 0  : ((raat)->err_code =  \
 ((gi)->gca_message_type == GCA_RELEASE) ? E_LQ002D_ASSOCFAIL : E_LQ0009_GCFMT))
 

FUNC_EXTERN VOID send_bkey(
			RAAT_CB      *raat_cb,
			char         *tup_ptr,
			char         *buf_ptr,
			i4      tid,
			i4          *msg_size);
