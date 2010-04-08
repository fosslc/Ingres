## -- induerr.tf: Error handling (detail table, insert & update operations)
##
##IF ('$locks_held' = 'optimistic' OR '$locks_held' = 'dbms') THEN
INQUIRE_SQL (IIrowcount = ROWCOUNT, IIerrorno = ERRORNO);
##ELSE
INQUIRE_SQL (IIerrorno = ERRORNO);
##ENDIF
IF (IIerrorno = $_deadlock_error) THEN		/* deadlock */
    IIdtries = IIdtries + 1;
    MESSAGE 'Retrying . . .';
    ENDLOOP $_loopb;
ELSEIF (IIerrorno != 0) THEN
##  IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
    ROLLBACK WORK;
##  ENDIF
    IIretval = -1;
    CALLPROC beep();    /* 4gl built-in procedure */
    MESSAGE 'An error occurred while saving the table field'
          + ' data. Details about the error were described'
          + ' by the message immediately preceding this'
          + ' one. The "Save" operation was not performed.'
          + ' Please correct the error and select "Save"'
          + ' again. If possible, the cursor will be placed on the row'
          + ' where the error occurred.'
          WITH STYLE = POPUP;

    /* Only scroll if row not deleted from table field */
    IF (IIrownumber > 0) THEN
          SCROLL $tblfld_name TO :IIrownumber;
    ENDIF;

    RESUME FIELD $tblfld_name;
##IF ('$locks_held' = 'optimistic' OR '$locks_held' = 'dbms') THEN
ELSEIF (IIrowcount = 0) THEN	/* another user has performed an update */
    ROLLBACK WORK;
##IF ('$locks_held' = 'optimistic') THEN
    MESSAGE 'The "Save" operation was not performed,'
          + ' because detail record(s) have been updated'
          + ' or deleted by another user since you selected'
	  + ' them.'
          + ' You must re-select and repeat your update.'
          WITH STYLE = POPUP;
##ELSE
/* If a deadlock occured and we are retrying the xact there is a potential
 * for the key columns to have been updated by another user.
*/
    MESSAGE 'The "Save" operation was not performed,'
          + ' because the key column(s) for detail record(s)'
          + ' have been changed since you selected them.'
          WITH STYLE = POPUP;
##ENDIF
    RESUME;
##ENDIF
ENDIF;
