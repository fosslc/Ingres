## -- inmins.tf         INSERT statement for master table.
##GENERATE QUERY INSERT MASTER REPEATED

INQUIRE_SQL (IIerrorno = ERRORNO);
IF (IIerrorno = $_deadlock_error) THEN
    IImtries = IImtries + 1;
    MESSAGE 'Retrying . . .';
    ENDLOOP $_loopb;
ELSEIF IIerrorno != 0 THEN
    ROLLBACK WORK;
    MESSAGE 'An error occurred while saving changes to this (master)'
          + ' data. Details about the error were described'
          + ' by the message immediately preceding this one.'
          + ' The "AddNew" operation was not performed.'
          + ' Please correct the error and select "AddNew"'
          + ' again.'
          WITH STYLE = POPUP;
    RESUME;
ENDIF;
