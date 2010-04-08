## -- indserr.tf: Error handling (detail table, select operation)
##
INQUIRE_SQL (IIerrorno = ERRORNO);
IF (IIerrorno != 0) THEN
##  IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
    ROLLBACK WORK;
##  ENDIF
    IIretval = -1;
    CALLPROC beep();    /* 4gl built-in procedure */
    MESSAGE 'Error occurred SELECTing detail data. Details about'
          + ' the error were described by the message immediately'
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
ENDIF;
