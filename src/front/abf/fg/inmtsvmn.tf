##      -- template file: inmtsvmn.tf (Master in a table field save menuitem)

'Save' (ACTIVATE = 1,
        EXPLANATION = 'Update database with current screen data'),
        KEY FRSKEY8 (ACTIVATE = 1) =
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
        MESSAGE 'Error: You cannot Save changes to data if you'
              + ' have previously selected AddNew on that data.'
              + ' Changes not Saved. To change this data you must'
              + ' reselect it, make changes and then press Save.'
              WITH STYLE = POPUP;
        RESUME;
    ENDIF;
##  ENDIF

    INQUIRE_FORMS FORM (IIint = CHANGE);
    IF (IIint = 1) THEN
        VALIDATE;        /* validate all fields on form */
##
##      GENERATE USER_ESCAPE UPDATE_START
##
        MESSAGE 'Saving changes . . .';

        IImtries = 0;			/* deadlock retry counter */

        IIloop1: WHILE (IImtries <= $_deadlock_retry) DO /*deadlock retry loop*/
        IIloop2: WHILE (1=1) DO		/* dummy loop for branching */
##      DEFINE $_loopb 'IIloop2'

##      IF $delete_master_allowed THEN
        /* Process the deleted rows first.
        ** If we process deleted rows last, which is the order
        ** that Unloadtable delivers them in, then we will lose data 
        ** if a row with an identical key is deleted and then inserted 
        ** into the table field before the user selects Save.
        ** Process Deleted rows only:
        */
        UNLOADTABLE $tblfld_name
                (IIrowstate = _STATE, IIrownumber = _RECORD)
        BEGIN

            IF (IIrowstate = 4) THEN        /* deleted */
                /* delete row using hidden field keys in where clause. */
##
##          IF $nullable_master_key THEN
##
##              GENERATE SET_NULL_KEY_FLAGS MASTER 
##              ENDIF

##              GENERATE QUERY DELETE MASTER REPEATED
            ENDIF;

##          INCLUDE inmtuerr.tf  -- Error handling, including deadlock

        END;        /* end of first unloadtable */
##      ENDIF -- $delete_master_allowed

        /* process all but Deleted rows */
        UNLOADTABLE $tblfld_name
                (IIrowstate = _STATE, IIrownumber = _RECORD)
        BEGIN
            IF (IIrowstate = 1) THEN             /* new */

##              GENERATE QUERY INSERT MASTER REPEATED 
##              INCLUDE inmtuerr.tf  -- Error handling, including deadlock

            ELSEIF (IIrowstate = 3) THEN         /* changed */
                /*
                ** Update row using hidden version of key fields
                ** in where clause.
                */

##              IF $nullable_master_key THEN
##              GENERATE SET_NULL_KEY_FLAGS MASTER
##              ENDIF

##              GENERATE QUERY UPDATE MASTER REPEATED 
##          INCLUDE inmtuerr.tf  -- Error handling, including deadlock
            ENDIF;

        END;        /* end of second unloadtable */

##      GENERATE USER_ESCAPE UPDATE_END
        COMMIT WORK;

        INQUIRE_SQL (IIerrorno = ERRORNO);
        IF (IIerrorno != 0) THEN
            ROLLBACK WORK;
            MESSAGE 'An error occurred while Saving this data.'
                  + ' Details about the error were described by'
                  + ' the message immediately preceding this one.'
                  + ' The "Save" operation was not performed.'
                  + ' Please correct the error and select "Save"'
                  + ' again.'
                  WITH STYLE = POPUP;
            RESUME;
        ENDIF;
        ENDLOOP IIloop1;
        ENDWHILE;			/* IIloop2 */
        ENDWHILE;			/* IIloop1 */

##      INCLUDE indeadlk.tf  -- Generate success/failure message after deadlock

        ENDLOOP;
    ELSE
        MESSAGE 'No changes to Save.';
        SLEEP 2;
    ENDIF;
END                /* end of 'Save' menuitem */
