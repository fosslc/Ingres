
/*--------------------------- iixagbl.h -----------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iixagbl.h - LIBQXA global constant and variable definitions
**
**  History:
**      01-Oct-93(mhughes)
**          First written for Ingres65 DTP/XA TUXEDO project.
**	03-Nov-93 (larrym)
**	    moved tuxedo-only stuff to iitxgbl.h
**	08-Nov-93 (larrym)
**	    added IIxa_do_2_phase_operation prototype as that is now
**	    called by tuxedo as well (and most tuxedo files include
** 	    this header).  It may not totally belong here, but for now
**	    this is the best place for it.
**	11-Nov-93 (larrym)
**	    modified IIxa_do_2phase_operation to accept new parameters
**	    to support TUXEDO
**	17-nov-93 (larrym)
**	    moved IIXA_CHECK_IF_TUXEDO_MACRO from iitx.h to here.
**	31-jan-94 (larrym)
**	    added IIXA_XID_STRING_LENGTH for retrieving XID's in ascii form
**	    from the lgmo_xa_dis_tran_ids view.
**	28-mar-1994 (larrym)
**	    Moved IIxa_do_2phase_operation into iixamain.h
*/

#ifndef  IIXAGBL_H
#define  IIXAGBL_H


/*
**  The XA call made.  This is used to check via a state table, whether
**  the TP library is making the XA calls in the proper sequence, for any
**  specific application thread.  DON'T FORGET to update IIxa_trace_xa_call
**  if you update this list !!!
*/

#define  IIXA_MIN_XA_CALL         1
#define  IIXA_OPEN_CALL           1
#define  IIXA_CLOSE_CALL          2
#define  IIXA_START_CALL          3
#define  IIXA_END_CALL            4
#define  IIXA_ROLLBACK_CALL       5
#define  IIXA_PREPARE_CALL        6
#define  IIXA_COMMIT_CALL         7
#define  IIXA_RECOVER_CALL        8
#define  IIXA_FORGET_CALL         9
#define  IIXA_COMPLETE_CALL       10
#define  IIXA_START_RB_CALL       11
#define  IIXA_START_RES_CALL      12
#define  IIXA_START_JOIN_CALL     13
#define  IIXA_START_RES_RB_CALL   14
#define  IIXA_END_RB_CALL         15
#define  IIXA_END_SUSP_CALL       16   
#define  IIXA_END_SUSP_RB_CALL    17
#define  IIXA_END_FAIL_CALL       18
#define  IIXA_PREPARE_RMERR_CALL  19
#define  IIXA_MAX_XA_CALL         19

#define  IIXA_NUM_RMI_XA_CALLS    (IIXA_CLOSE_CALL)
#define  IIXA_NUM_TOTAL_XA_CALLS  (IIXA_MAX_XA_CALL)
#define  IIXA_NUM_MAIN_XA_CALLS   (IIXA_COMPLETE_CALL)

#define  IIXA_XID_STRING_LENGTH	  351

/*
** TUXEDO definitions
*/

/*
** CHECK for tuxedo switch pointer defined in fe_thread_main control block
** Will only be set if TUXEDO was defined during compile of ..... one of the
** startup routines....
*/

# define IIXA_CHECK_IF_TUXEDO_MACRO \
	(IIcx_fe_thread_main_cb->fe_main_tuxedo_switch)

/*
** Define constants for actions for the IIXA_CHECK_IF_TUXEDO_MACRO
*/

#define  IITUX_MIN_ACTION    1
#define  IITUX_ACTION_ONE    1
#define  IITUX_ACTION_TWO    2
#define  IITUX_ACTION_THREE  3
#define  IITUX_ACTION_FOUR   4
#define  IITUX_ACTION_FIVE   5
#define  IITUX_MAX_ACTION    5

/*
** Need this one here so lock_parse_open_string knows how to parse
** tuxedo open string
*/
#define	IITUX_MIN_SGID_NAME_LEN		4
#define	IITUX_MAX_SGID_NAME_LEN		24
#define IITUX_SGID_SIZE			IITUX_MAX_SGID_NAME_LEN + 1

#endif /* ifndef IIXAGBL_H */

/*------------------------------end of iixagbl.h ----------------------------*/
