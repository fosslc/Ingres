## -- inmsserr.tf: Error handling - master table (simple field), select
## --             operation
##
##IF $singleton_select THEN
INQUIRE_SQL (IIrowcount = ROWCOUNT, IIerrorno = ERRORNO);
##ELSE
INQUIRE_SQL (IIerrorno = ERRORNO);
##  ENDIF
IF (IIerrorno != 0) THEN
##  IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
    ROLLBACK WORK;
##  ENDIF
    IIretval = -1;
    CALLPROC beep();/* 4gl built-in procedure */
    MESSAGE 'Error occurred SELECTing master data. Details about'
          + ' the error were described by the message immediately'
          + ' preceding this one.'
##  IF $user_qualified_query THEN
          + ' Please correct the error and select "Go" again.'
##  ELSE
          + ' Returning to previous frame . . .'
##  ENDIF  -- $user_qualified_query
          WITH STYLE = POPUP;
##  IF $user_qualified_query THEN
    RESUME;
##  ELSE
    ENDLOOP IIloop0;
##  ENDIF
##IF $singleton_select THEN
ELSEIF (IIrowcount = 0) THEN
##  IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
    ROLLBACK WORK;
##  ENDIF
##  IF $user_qualified_query THEN
    MESSAGE 'No rows retrieved';
    SLEEP 2;
    RESUME;
##  ELSE	-- $user_qualified_query
##  IF $insert_master_allowed THEN
    MESSAGE 'No data found, returning to previous frame . . .'
        WITH STYLE = POPUP;
##  ENDIF	-- $insert_master_allowed
    ENDLOOP IIloop0;
##  ENDIF	-- $user_qualified_query	
##  ENDIF  -- singleton_select
ENDIF;
