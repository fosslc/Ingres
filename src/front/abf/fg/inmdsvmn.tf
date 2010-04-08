##      -- template file: inmdsvmn.tf (Master/detail save menuitem)

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
##      IF $join_field_used THEN
##      GENERATE CHECK_CHANGE JOIN_FIELDS       -- sets IIupdrules

##      IF $update_restricted THEN
        INQUIRE_FORMS TABLE $form_name
                (IIint = DATAROWS($tblfld_name));
        IF (IIupdrules = 1) THEN
            /* Displayed join field(s) changed. */
##
##          IF $insert_detail_allowed THEN      -- tblfld in FILL mode
            IF (IIint = 0) OR
               (IIint = 1 AND $tblfld_name[1]._STATE = 0) THEN
                /* Table field is empty or has one Undefined row. */
##          ELSE
            IF (IIint = 0) THEN
                /* Table field is empty. */
##          ENDIF  -- insert_detail_allowed
##
            ELSE
                /* Displayed join field changed + detail rows exist. 
                ** Disallowed by update-restricted update rule.
                */
                CALLPROC beep();        /* 4gl built-in procedure */
                MESSAGE 'Error: join fields changed when detail data'
                  + ' exists. The update rules between these 2 tables'
                  + ' do not allow you to change the join field(s) when'
                  + ' detail data exists. The "Save" operation'
                  + ' was not performed. The join field(s) will be'
                  + ' restored to their original value.'
                  WITH STYLE = POPUP;
##              GENERATE COPY_HIDDEN_TO_VISIBLE JOIN_FIELDS
                RESUME;
            ENDIF;
        ENDIF;
##
##      ENDIF  -- update_restricted 
##      ELSE
        IIupdrules = 0;
##      ENDIF  -- join_field_used
##
##      GENERATE USER_ESCAPE UPDATE_START
##
        MESSAGE 'Saving changes . . .';

        IImtries = 0;	/* deadlock retry counter (master table) */
        IIdtries = 0;	/* deadlock retry counter (detail table) */

        IIloop1:	/* loop to retry if deadlock is encountered */
        WHILE (IImtries <= $_deadlock_retry AND IIdtries <= $_deadlock_retry) DO
        IIloop2: WHILE (1=1) DO		/* dummy loop for branching */
##      DEFINE $_loopb 'IIloop2'

        /* Do detail updates first in case of rules or referential
        ** integrities on master data. If master updates done first
        ** then detail updates may fail. For example if master update
        ** changes value of join field, and that fires a rule that
        ** changes the join key value for all matching detail data,
        ** then that could cause detail updates to fail (rowcount=0).
        */
##      INCLUDE indupd.tf   -- unloadtable loop to UPDATE details.

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

        NEXT;   /* Changes Saved. Get Next screen of data. */
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
END                /* end of 'Save' menuitem */
