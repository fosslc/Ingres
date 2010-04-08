## -- inmdel.tf         DELETE statement for master
##
##IF $nullable_master_key THEN
##
##-- If table has nullable key, then can't simply generate a WHERE
##-- clause with "keycolumn = :keyfield_name", because ":keyfield_name" may
##-- contain a NULL value, and "keycolumn = NULL" never matches anything!
##-- Must generate a more complicated WHERE clause for such columns and
##-- use Flag variables to indicate when "keyfield_name" contains NULL.
##
##GENERATE SET_NULL_KEY_FLAGS MASTER

##ENDIF
##
##GENERATE QUERY DELETE MASTER REPEATED

##IF ('$locks_held' = 'optimistic' OR '$locks_held' = 'dbms') THEN
INQUIRE_SQL (IIrowcount = ROWCOUNT, IIerrorno = ERRORNO);
##ELSE
INQUIRE_SQL (IIerrorno = ERRORNO);
##ENDIF
IF (IIerrorno = $_deadlock_error) THEN
    IImtries = IImtries + 1;
    MESSAGE 'Retrying . . .';
    ENDLOOP $_loopb;
ELSEIF (IIerrorno != 0) THEN
    ROLLBACK WORK;
    MESSAGE 'An error occurred while Deleting this (master) data.'
          + ' Details about the error were described by the'
          + ' message immediately preceding this one.'
          + ' This data was not Deleted.'
          + ' Correct the error and select "Delete" again.'
          WITH STYLE = POPUP;
    RESUME;
##IF ('$locks_held' = 'optimistic' OR '$locks_held' = 'dbms') THEN
ELSEIF (IIrowcount = 0) THEN    /* another user has performed an update */
    ROLLBACK WORK;
##IF ('$locks_held' = 'optimistic') THEN 
    MESSAGE 'The "Delete" operation was not performed,'
          + ' because the master record has been updated'
          + ' or deleted by another user since you selected it.'
          + ' You must re-select and repeat your delete.'
          WITH STYLE = POPUP;
##ELSE
    MESSAGE 'The "Delete" operation was not performed,'
          + ' because the key column(s) of the master record has'
          + ' been changed by another user since you selected it.'
          + ' You must re-select and repeat your delete.'
          WITH STYLE = POPUP;
##ENDIF
    RESUME;
##ENDIF
ENDIF;
