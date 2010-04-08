##      -- template file: inmtdlmn.tf   (Master in table field Delete menuitem)

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
    IIchar1 = PROMPT
        'Delete this screen of data from the Database (y/n)?';
    IF (UPPERCASE(:IIchar1) != 'Y') THEN
        RESUME;
    ENDIF;
##
##  GENERATE USER_ESCAPE DELETE_START

    MESSAGE 'Deleting . . .';

    IImtries = 0;			/* deadlock retry counter */

    IIloop5: WHILE (IImtries <= $_deadlock_retry) DO /* deadlk retry loop */
    IIloop6: WHILE (1=1) DO     /* dummy loop for branching */

    UNLOADTABLE $tblfld_name
        (IIrowstate = _STATE, IIrownumber = _RECORD)
    BEGIN
        /* delete UNCHANGED, CHANGED & DELETED rows */
        IF (IIrowstate = 2) 
        OR (IIrowstate = 3)
        OR (IIrowstate = 4) THEN
##
##          IF $nullable_master_key THEN
##          GENERATE SET_NULL_KEY_FLAGS MASTER 
##          ENDIF

##          GENERATE QUERY DELETE MASTER REPEATED
        ENDIF;

##      IF ('$locks_held' = 'optimistic' OR '$locks_held' = 'dbms') THEN
        INQUIRE_SQL (IIrowcount = ROWCOUNT, IIerrorno = ERRORNO);
##      ELSE
        INQUIRE_SQL (IIerrorno = ERRORNO);
##      ENDIF
        IF (IIerrorno = $_deadlock_error) THEN
            IImtries = IImtries + 1;
            MESSAGE 'Retrying . . .';
            ENDLOOP IIloop6;
        ELSEIF (IIerrorno != 0) THEN
            ROLLBACK WORK;
            MESSAGE 'An error occurred while Deleting the table'
                  + ' field data. Details about the error were'
                  + ' described by the message immediately preceding'
                  + ' this one. This data was not Deleted.'
                  + ' Correct the error and select "Delete" again.'
                  + ' If possible, the cursor will be placed on the row'
                  + ' where the error occurred.'
                  WITH STYLE = POPUP;

            /* Only scroll if row not deleted from table field */
            IF (IIrownumber > 0) THEN
                  SCROLL $tblfld_name TO :IIrownumber;
            ENDIF;

            RESUME FIELD $tblfld_name;
##      IF ('$locks_held' = 'optimistic' OR '$locks_held' = 'dbms') THEN
        ELSEIF (IIrowcount = 0) THEN /* another user has performed an update */
            ROLLBACK WORK;
##	IF ('$locks_held' = 'optimistic') THEN
            MESSAGE 'The "Delete" operation was not performed,'
                  + ' because master record(s) have been updated'
                  + ' or deleted by another user since you selected'
                  + ' them.' 
                  + ' You must re-select and repeat your delete.'
                  WITH STYLE = POPUP;
##	ELSE
/* If a deadlock occured and we are retrying the xact there is a potential
 * for the key columns to have been updated by another user.
 */
            MESSAGE 'The "Delete" operation was not performed,'
                  + ' because the key column(s) of one of the master'
                  + ' records has changed since you selected it.'
                  + ' You must re-select and repeat your delete.'
                  WITH STYLE = POPUP;
##	ENDIF
            RESUME;
##      ENDIF
        ENDIF;

    END;	/* Unloadtable */


##  GENERATE USER_ESCAPE DELETE_END
##
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
    ENDWHILE;			/* IIloop6 */
    ENDWHILE;			/* IIloop5 */
##  DEFINE $_retry_max $_deadlock_retry+1
    IF (IImtries > $_deadlock_retry) THEN
        ROLLBACK WORK;
        MESSAGE 'Deadlock was detected during all $_retry_max' 
              + ' attempts to delete this data. The "Delete" operation'
              + ' was not performed. Please correct the error and'
              + ' select "Delete" again.'
              WITH STYLE = POPUP;
        RESUME;
    ELSEIF (IImtries > 0) THEN
        /* Save succeeded on a reattempt following deadlock */
        MESSAGE 'Transaction completed successfully.'
              WITH STYLE = POPUP;
    ENDIF;


    ENDLOOP;    /* exit submenu */
##
END     /* end of 'Delete' menuitem */
