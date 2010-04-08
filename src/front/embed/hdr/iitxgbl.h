/*--------------------------- iitxgbl.h -----------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iitxgbl.h - LIBQTXXA global constant and variable definitions
**
**  History:
**      03-Nov-93 (larrym)
**          First written for Ingres65 DTP/XA TUXEDO project.
**      06-Nov-93 (mhughes)
**	    Added some more macros
**	10-Nov-93 (larrym)
**	    added IITUX_TMS_AS_GLOBAL_CB typedef.
**	16-Nov-93 (larrym)
**	    Held code review, modified to incorporate feedback.
**	23-Nov-1993	(mhughes)
**	    Added more tp_ret_value error codes.
**	02-Dec-1993 (mhughes)
**	    Added PUT_INDEXED_XID macro and added icas status services
**	03-Dec-1993 (mhughes)
**	    Added userlog definitions & field in tuxedo_global structure.
**	    Added fields for AS service names to contain names of services
**	    advertised by the current as process (if applicable).
**	    Got rid of tuxedo_test field. 
**	13-Dec-1993 (mhughes)
**	    Sorry, created more macros. I want to log tpadvertise/unadvertise
**	    activity. I'll use the same style as we've got for now. But:
**	    CHECK: Get rid of these tp macros and write iitp wrapper functions.
**	16-Dec-1993 (mhughes)
**	    Tweak file names for directory checking. 
**	    Add delays for repeat service calls on AS startup.
**	22-Dec-1993 (mhughes)
**	    More error statuses & logging for the icas services
**	08-Jan-1993 (mhughes)
**	    Add tpcall flags to tuxedo global cb.
**	    Modify arguments to IItux_userlog.
**	12-Jan-1993 (mhughes)
**	    Modified IITUX_TPCALL_MACRO to break tpcall into a tpacall and
**	    tpgetrply. This fixes a bug where services making further
**	    tpcalls would hang. Also cuts down atmi overhead a tad.
**	01-Feb-1994 (mhughes)
**	    Added field to tuxedo_global_cb for user-spcified alternate
**	    icas log file directory.
**	01-Feb-1994 (larrym)
**	    Fixed bug in IITUX_TPCALL_MACRO where we weren't returning error 
**	    status is tpacal failed
**	09-mar-1994 (larrym)
**	    fixed bug 59639.  Modified service name templates to allow
**	    service names less than four characters in length.
**	18-mar-1994 (larrym)
**	    fixed bug 60203, AS doesn't die when ICAS tells it to.  Modified
**	    IITUX_TPCALL_MACRO to handle the TPNOREPLY case.
**	08-Apr-1994 (mhughes)
**	    Added Recovery Testing field to tuxedo_global_cb ifdef'd for
**	    recovery testing compiles. Field holds location of failure point.
**      16-Jan-1995 (stial01)
**          Changes for new INGRES-TUXEDO architecture.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#ifndef  IITXGBL_H
#define  IITXGBL_H


/*
** Definitions for managing tuxedo userlog writing
*/
#define IITUX_ULOG_MIN_LEVEL	 0
#define IITUX_ULOG_LEVEL_ZERO	 0
#define IITUX_ULOG_LEVEL_ONE	 1
#define IITUX_ULOG_LEVEL_TWO	 2
#define IITUX_ULOG_LEVEL_THREE	 3
#define IITUX_ULOG_LEVEL_FOUR	 4
#define IITUX_ULOG_MAX_LEVEL	 4

#define IITUX_ULOG_BOOT		 1
#define IITUX_ULOG_SHUTDOWN    	 2
#define IITUX_ULOG_TPCALL    	 3
#define IITUX_ULOG_TPRETURN    	 4
#define IITUX_ULOG_TPADVERTISE   5
#define IITUX_ULOG_TPUNADVERTISE 6
#define IITUX_ULOG_TPCALLED    	 7
#define IITUX_ULOG_ERRORTXT    	 8


/*
** TUXEDO work directory and icas server log filename root
*/

#define IITUX_END_OF_SERVER_LIST	0
#define IITUX_SGID_NAME_UNIQUE_LEN 4
#define TUXEDO_DIR "/files/tuxedo"

/*
** TUXEDO maximum allowable number of AS's (FF hex == 255 Decimal )
*/

#define IITUX_SERVER_ID_STRLEN	2
#define IITUX_MAX_APP_SERVERS	0xFF

/*
** Global TUXEDO structure for AS and TMS processes.  
** IITUX_MAX_SGID_NAME_LEN and IITUX_SGID_SIZE are defined in iixagbl.h
**
*/

typedef struct s_IITUX_TMS_AS_GLOBAL_CB
{
    i4		SVN;
    i4		process_type;
    char        server_group_id[IITUX_SGID_SIZE];
                                            /* Tuxedo server group id     */
                                            /* extracted from open string */
                                            /* as a tag and used when     */
					    /* identifying the group      */
    i4			userlog_level;
#define IITUX_MAX_LOG_DIR_NAME 256          /* MAX_LOC, but I don't want */
                                            /* to include lo.h in every */
					    /* libqtxxa module */

#ifdef TUXEDO_RECOVERY_TEST
    i4			failure_point;	    /* Numeric code representing */
    					    /* location of process failure */
#define IITUX_NO_FAILURE		0
#define IITUX_FAIL_MAIN_TOP		1
#define IITUX_FAIL_AS_BEFORE_PREPARE	10
#define IITUX_FAIL_AS_AFTER_PREPARE 	11  
#define IITUX_FAIL_AS_BEFORE_COMMIT 	12
#define IITUX_FAIL_AS_AFTER_COMMIT 	13
#define IITUX_FAIL_AS_BEFORE_ROLLBACK 	14
#define IITUX_FAIL_AS_AFTER_ROLLBACK 	15
#define IITUX_FAIL_TMS_BEFORE_PREPARE	20
#define IITUX_FAIL_TMS_AFTER_PREPARE 	21  
#define IITUX_FAIL_TMS_BEFORE_COMMIT 	22
#define IITUX_FAIL_TMS_AFTER_COMMIT 	23
#define IITUX_FAIL_TMS_BEFORE_ROLLBACK 	24
#define IITUX_FAIL_TMS_AFTER_ROLLBACK 	25
#define IITUX_FAIL_TMS_BEFORE_REATTACH	30

#endif /* TUXEDO_RECOVERY_TEST */
} IITUX_TMS_AS_GLOBAL_CB;

GLOBALREF	IITUX_TMS_AS_GLOBAL_CB	*IItux_tms_as_global_cb;

/*
** Top level switching routine for tuxedo bridge functions.
*/
FUNC_EXTERN int
IItux_main( i4  xa_call_type, i4  xa_action,
            PTR arg1, PTR arg2, PTR arg3, PTR arg4,
            PTR arg5, PTR arg6, PTR arg7, PTR arg8
          );

#endif /* ifndef IITXGBL_H */
/*------------------------------end of iitxgbl.h ----------------------------*/
