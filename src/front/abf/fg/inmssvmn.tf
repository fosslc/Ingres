##      -- template file: inmssvmn.tf (master/simple-fields Save menuitem)

'Save' (ACTIVATE = 1,
        EXPLANATION = 'Update database with current screen data'),
        KEY FRSKEY8 (ACTIVATE = 1) =
BEGIN
##  IF $insert_master_allowed THEN
    /* Must prevent AddNew followed by Save or Delete. Reason:
    ** 1. User may have changed keys before selecting AddNew.
    ** Save/Delete assume hidden field/column versions of keys give
    ** true database identity of the displayed data.
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
        VALIDATE;   /* validate all fields on form */

##      GENERATE USER_ESCAPE UPDATE_START
##
        MESSAGE 'Saving changes . . .';

        IImtries = 0;		/* deadlock retry counter */

        IIloop1: WHILE (IImtries <= $_deadlock_retry) DO /*deadlock retry loop*/
        IIloop2: WHILE (1=1) DO		/* dummy loop for branching */
##      DEFINE $_loopb 'IIloop2'

##      INCLUDE inmupd.tf   -- UPDATE statement for master

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
##
##      IF $singleton_select THEN
##
        ENDLOOP;
##
##      ELSE
##
        SET_FORMS FORM (CHANGE = 0);

        NEXT;
##
##      GENERATE USER_ESCAPE QUERY_NEW_DATA
##
##      IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
        COMMIT WORK;    /* Release any locks acquired while selecting
                        ** data for the "NEXT" statement.
                        */
##      ENDIF
##
##      ENDIF   -- $singleton_select
    ELSE
        MESSAGE 'No changes to Save.';
        SLEEP 2;
    ENDIF;
END                     /* end of 'Save' menuitem */
