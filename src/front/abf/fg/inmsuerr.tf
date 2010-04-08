## -- inmsuerr.tf: Error handling - master table (simple field), insert &
## --              update operations. Used for append frames as well.
##
##IF ('$locks_held' = 'optimistic' OR '$locks_held' = 'dbms') THEN
INQUIRE_SQL (IIrowcount = ROWCOUNT, IIerrorno = ERRORNO);
##ELSE
INQUIRE_SQL (IIerrorno = ERRORNO);
##ENDIF
IF (IIerrorno = $_deadlock_error) THEN		/* deadlock */
    IImtries = IImtries + 1;
    MESSAGE 'Retrying . . .';
    ENDLOOP $_loopb;
ELSEIF (IIerrorno != 0) THEN
##  IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
    ROLLBACK WORK;
##  ENDIF
    IIretval = -1;
    CALLPROC beep();    /* 4gl built-in procedure */
    MESSAGE 'An error occurred while saving changes to the'
          + ' master data. Details about the error were'
          + ' described by the message immediately preceding'
          + ' this one. The "Save" operation was not'
          + ' performed. Please correct the error and select'
          + ' "Save" again.'
          WITH STYLE = POPUP;
##  IF ('$frame_type' = 'Append' OR $user_qualified_query) THEN
    RESUME;
##  ELSE
    ENDLOOP IIloop0;
##  ENDIF
##IF ('$locks_held' = 'optimistic' OR '$locks_held' = 'dbms') THEN
ELSEIF (IIrowcount = 0) THEN    /* another user has performed an update */
    ROLLBACK WORK;
##IF ('$locks_held' = 'optimistic') THEN
    MESSAGE 'The "Save" operation was not performed,'
          + ' because the master record has been updated'
          + ' or deleted by another user since you selected it.'
          + ' You must re-select and repeat your update.'
          WITH STYLE = POPUP;
##ELSE
/* If a deadlock occured and we are retrying the xact there is a potential
 * for the key columns to have been updated by another user.
 */
    MESSAGE 'The "Save" operation was not performed,'
          + ' because the key column(s) for the master'
          + ' record have changed since you selected it.'
          + ' You must re-select and repeat your update.'
          WITH STYLE = POPUP;
##ENDIF
    RESUME;
##ENDIF
ENDIF;
