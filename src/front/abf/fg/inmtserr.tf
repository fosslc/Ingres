## -- inmtserr.tf: Error handling (master in table field, select operation)
##
INQUIRE_SQL (IIrowcount = ROWCOUNT, IIerrorno = ERRORNO);
IF (IIerrorno != 0) THEN
##  IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
    ROLLBACK WORK;
##  ENDIF
    IIretval = -1;
    CALLPROC beep();    /* 4gl built-in procedure */
    MESSAGE 'Error occurred SELECTing data. Details about the'
          + ' error were described by the message immediately'
          + ' preceding this one.'
##  IF $user_qualified_query THEN
          + ' Please correct the error and select "Go" again.'
##  ELSE
          + ' Returning to previous frame . . .'
##  ENDIF
          WITH STYLE = POPUP;
##  IF $user_qualified_query THEN
    RESUME;
##  ELSE
    ENDLOOP IIloop0;
##  ENDIF
ELSEIF (IIrowcount = 0) THEN
##  IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
    ROLLBACK WORK;
##  ENDIF
##  IF $user_qualified_query THEN
    MESSAGE 'No rows retrieved';
    SLEEP 2;
    RESUME;
##  ELSE
    MESSAGE 'No data found, returning to previous frame . . .'
        WITH STYLE = POPUP;
    ENDLOOP IIloop0;
##  ENDIF
ENDIF;
