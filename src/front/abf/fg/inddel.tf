## -- inddel.tf         UNLOADTABLE loop to DELETE details
UNLOADTABLE $tblfld_name
    (IIrowstate = _STATE, IIrownumber = _RECORD)
BEGIN
    /* delete UNCHANGED, CHANGED & DELETED rows */
    IF (IIrowstate = 2) OR (IIrowstate = 3)
                        OR (IIrowstate = 4) THEN
##
##      IF $nullable_detail_key THEN
##--
##--    If table has nullable key, then can't simply generate a WHERE
##--    clause with "keycolumn = :keyfield_name", because ":keyfield_name" may
##--    contain a NULL value, and "keycolumn = NULL" never matches anything!
##--    Must generate a more complicated WHERE clause for such columns and
##--    use Flag variables to indicate when "keyfield_name" contains NULL.

##      GENERATE SET_NULL_KEY_FLAGS DETAIL
##      ENDIF

##      GENERATE QUERY DELETE DETAIL REPEATED

##  IF ('$locks_held' = 'optimistic' OR '$locks_held' = 'dbms') THEN
    INQUIRE_SQL (IIrowcount = ROWCOUNT, IIerrorno = ERRORNO);
##  ELSE
    INQUIRE_SQL (IIerrorno = ERRORNO);
##  ENDIF
    IF (IIerrorno = $_deadlock_error) THEN
        IIdtries = IIdtries + 1;
        MESSAGE 'Retrying . . .';
        ENDLOOP $_loopb;
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
##  IF ('$locks_held' = 'optimistic' OR '$locks_held' = 'dbms') THEN
    ELSEIF (IIrowcount = 0) THEN    /* another user has performed an update */
        ROLLBACK WORK;
##  IF ('$locks_held' = 'optimistic') THEN
        MESSAGE 'The "Delete" operation was not performed,'
              + ' because detail record(s) have been updated'
              + ' or deleted by another user since you selected'
              + ' them.'
              + ' You must re-select and repeat your delete.'
              WITH STYLE = POPUP;
##  ELSE
        MESSAGE 'The "Delete" operation was not performed,'
              + ' because key column(s) of detail record(s) have'
              + ' been changed by another user since you selected'
              + ' them.'
              + ' You must re-select and repeat your delete.'
              WITH STYLE = POPUP;
##  ENDIF
        RESUME;
##  ENDIF
    ENDIF;  /* IIerrorno */
    ENDIF;  /* IIrowstate */
END;/* unloadtable */
