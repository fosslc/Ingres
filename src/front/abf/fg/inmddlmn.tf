##      -- template file: inmddlmn.tf   (Master/detail Delete menuitem)

'Delete' (VALIDATE = 0, ACTIVATE = 0,
    EXPLANATION = 'Delete current screen of data from Database'),
    KEY FRSKEY13 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
##  IF $insert_master_allowed THEN
    /* Must prevent AddNew followed by Save or Delete. Reasons:
    ** 1. User may have changed keys before selecting AddNew.
    ** Save/Delete assume hidden field/column versions of keys give
    ** true database identity of the displayed data.
    ** 2. Table field row _STATEs won't show just the changes
    ** made since AddNew.
    */
    IF (IIsaveasnew = 'y') THEN
        CALLPROC beep();        /* 4gl built-in procedure */
        MESSAGE 'Error: You cannot Delete data if you have'
              + ' previously selected AddNew on that data.'
              + ' Data not Deleted. To Delete this data you'
              + ' must reselect it and then press Delete.'
              WITH STYLE = POPUP;
        RESUME;
    ENDIF;
##
##  ENDIF
##
##  IF $delete_restricted THEN
    INQUIRE_FORMS TABLE ''
            (IIint = DATAROWS('$tblfld_name'));
##
##  IF $insert_detail_allowed THEN      -- tblfld in FILL mode
    IF (IIint = 0) OR
       (IIint = 1 AND $tblfld_name[1]._STATE = 0) THEN
        /* Table field is empty or has one Undefined row. */
##  ELSE
    IF (IIint = 0) THEN
        /* Table field is empty. */
##  ENDIF
##
    ELSE
        CALLPROC beep();
        MESSAGE
             'You are not allowed to "Delete" when the detail table'
           + ' field contains data.'
           WITH STYLE = POPUP; 
        RESUME;
    ENDIF;
##  ENDIF  -- $delete_restricted 

    IIchar1 = PROMPT
        'Delete this screen of data from the Database (y/n)?';
    IF (UPPERCASE(:IIchar1) != 'Y') THEN
        RESUME;
    ENDIF;
##
##  GENERATE USER_ESCAPE DELETE_START

    MESSAGE 'Deleting . . .';

    IImtries = 0;       /* deadlock retry counter (master table) */
    IIdtries = 0;       /* deadlock retry counter (detail table) */

    IIloop5:	/* loop to retry if deadlock is encountered */
    WHILE (IImtries <= $_deadlock_retry AND IIdtries <= $_deadlock_retry) DO
    IIloop6: WHILE (1=1) DO	/* dummy loop for branching */
##  DEFINE $_loopb 'IIloop6'

##  INCLUDE inmdel.tf   -- DELETE master

##  IF $delete_dbmsrule THEN
    /*
    ** The DBMS is expected to handle 
    ** the deletion of detail rows.
    */
##  ELSE
##  INCLUDE inddel.tf   -- unloadtable loop to DELETE details
##  ENDIF

##  GENERATE USER_ESCAPE DELETE_END
    COMMIT WORK;

    INQUIRE_SQL (IIerrorno = ERRORNO);
    IF (IIerrorno != 0) THEN
        ROLLBACK WORK;
        MESSAGE 'An error occurred while Deleting this data.'
              + ' Details about the error were described by'
              + ' the message immediately preceding this one.'
              + ' The data was not Deleted. Correct the error'
              + ' and select "Delete" again.'
              WITH STYLE = POPUP;
        RESUME;
    ENDIF;

    ENDLOOP IIloop5;
    ENDWHILE;				/* IIloop6 */
    ENDWHILE;				/* IIloop5 */

##  INCLUDE indeadlk.tf  -- Generate success/failure message after deadlock
##
##  IF $singleton_select THEN
##
    ENDLOOP;    /* exit submenu */
##
##  ELSE
##
    SET_FORMS FORM (CHANGE = 0);

    NEXT;       /* Data Deleted. Get Next screen of data */
##
##  GENERATE USER_ESCAPE QUERY_NEW_DATA
##
##  IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
    COMMIT WORK;    /* Release any locks acquired while selecting
                    ** data for the "NEXT" statement.
                    */
##  ENDIF
##
##  ENDIF       -- $singleton_select
##
END     /* end of 'Delete' menuitem */
