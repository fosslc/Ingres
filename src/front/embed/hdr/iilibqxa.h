
/*--------------------------- iilibqxa.h -----------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iilibqxa.h - LIBQXA entrypoints, called from LIBQ.
**
**  History:
**      25-Aug-1992 (mani)
**          First written for Ingres65 DTP/XA project.
**	19-nov-1992 (larrym)
**	    Changed interface between libq and libqxa.  All calls from
**	    libq to libqxa now go through a handler, the pointer to which
**	    is maintained in the IIglbcb.  The function prototypes in this
**	    file have been converted to #defines.
**	09-mar-1993 (larrym)
**	    added connection name support; also support for rollback value.
**	24-mar-1993 (larrym)
**	    added iixa_set_ingres_state
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#ifndef  IILIBQXA_H
#define  IILIBQXA_H

/*
** Name:   IIXA_CHECK_IF_XA_MACRO
**
** Description:
**     Check if LIBXA is linked in, and XA support is turned on.
**
** Inputs:
**     Nothing.
**
** Outputs:
**     Returns: 
**         TRUE if XA support is turned on.
**         FALSE if this is a regular Ingres client.
**
** History:
**	25-Aug-1992 - First written (mani)
**	19-nov-1992 (larrym)
**	    Changed from function to MACRO.  It now checks the libq handler
**	    pointer in the global control block.  
*/

# define IIXA_CHECK_IF_XA_MACRO	(IIglbcb->ii_gl_xa_hdlr)

/*
+* Name:   IIxaHdl_libq - Handler that is always called by libq
**
** Description:
**      This handler is called by libq to access libqxa information.
**
** Inputs:
**      hflag - What condition triggered this.
**      h_arg_n   - The data associated with the condition - for passing nats.
**      h_arg_p*  - The data associated with the condition - for passing PTRs.
**      The following definitions describe the flag and the argument:
**
**        Flag                          Data
**      IIXA_CHECK_SET_INGRES_STATE     *fe_thread_cx_cb_p,
**					state_mask
**
**      IIXA_CHECK_INGRES_STATE         *fe_thread_cx_cb_p,
**					state_mask
**
**      IIXA_GET_AND_SET_XA_SID         *fe_thread_cx_cb_p,
**                                      connect_name,
**                                      *xa_sid
**
**      IIXA_SET_ROLLBACK_VALUE		*fe_thread_cx_cb_p
**
**      IIXA_SET_INGRES_STATE           *fe_thread_cx_cb_p,
**					new_ingres_state,
**					*old_ingres_state
**
** Outputs:
**     Returns:
**        DB_STATUS.
**
** History:
**      19-nov-1992 (larrym)
**          written.
**	02-dec-1992 (larrym)
**	    Added macros to simplify libq calling libqhandler
**	09-mar-1993 (larrym)
**	    added iixa_set_rollback_value.
**	24-mar-1993 (larrym)
**	    added iixa_set_ingres_state
*/

DB_STATUS
IIxaHdl_libq (
        i4      hflag,				/* Function to call */
# define IIXA_CODE_CHECK_SET_INGRES_STATE	01
# define IIXA_CODE_CHECK_INGRES_STATE		02
# define IIXA_CODE_GET_AND_SET_XA_SID		03
# define IIXA_CODE_SET_ROLLBACK_VALUE		04
# define IIXA_CODE_SET_INGRES_STATE		05

        i4      h_arg1_n,			/* for passing nats by value */
        PTR     h_arg1_p,			/* Generic pointer arguments */
        PTR     h_arg2_p,
        PTR     h_arg3_p,
        PTR     h_arg4_p);

/* Macros for LIBQ */
/* calls IIxa_check_set_ingres_state.  */
# define IIXA_CHECK_SET_INGRES_STATE(sm) \
	(*IIglbcb->ii_gl_xa_hdlr) ( IIXA_CODE_CHECK_SET_INGRES_STATE, \
	(i4) sm, (PTR) 0,  (PTR) 0, (PTR) 0, (PTR) 0)

/* calls IIxa_check_ingres_state.  */
# define IIXA_CHECK_INGRES_STATE(sm) \
	(*IIglbcb->ii_gl_xa_hdlr) ( IIXA_CODE_CHECK_INGRES_STATE, \
	(i4) sm, (PTR) 0, (PTR) 0, (PTR) 0, (PTR) 0)

/* calls IIxa_get_and_set_xa_sid.  */
# define IIXA_GET_AND_SET_XA_SID(connam, xasid) \
	(*IIglbcb->ii_gl_xa_hdlr) ( IIXA_CODE_GET_AND_SET_XA_SID, \
	(i4) 0, (PTR) 0, (PTR) connam, (i4 *) xasid, (PTR) 0) 

/* calls IIxa_set_rollback_value.  */
# define IIXA_SET_ROLLBACK_VALUE \
	(*IIglbcb->ii_gl_xa_hdlr) ( IIXA_CODE_SET_ROLLBACK_VALUE, \
	(i4) 0, (PTR) 0, (PTR) 0, (PTR) 0, (PTR) 0) 

/* calls IIxa_set_ingres_state.  */
# define IIXA_SET_INGRES_STATE(ns,os) \
	(*IIglbcb->ii_gl_xa_hdlr) ( IIXA_CODE_SET_INGRES_STATE, \
	(i4) ns, (PTR) 0, (i4 *) os, (PTR) 0, (PTR) 0)

#endif  /* ifndef IILIBQXA_H */
/*-------------------------- end of iilibqxa.h ----------------------------*/
