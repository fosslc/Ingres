## -- indins.tf         UNLOADTABLE loop to INSERT details
UNLOADTABLE $tblfld_name
    (IIrowstate = _STATE, IIrownumber = _RECORD)
BEGIN
    /* insert NEW, UNCHANGED & CHANGED rows */
    IF ((IIrowstate = 1) OR (IIrowstate = 2)
                         OR (IIrowstate = 3)) THEN

##      GENERATE QUERY INSERT DETAIL REPEATED

        INQUIRE_SQL (IIerrorno = ERRORNO);
        IF (IIerrorno = $_deadlock_error) THEN
            IIdtries = IIdtries + 1;
            MESSAGE 'Retrying . . .';
            ENDLOOP $_loopb;
        ELSEIF (IIerrorno != 0) THEN
            ROLLBACK WORK;
            MESSAGE 'An error occurred while saving the table field'
                  + ' data. Details about the error were'
                  + ' described by the message immediately'
                  + ' preceding this one. The "AddNew"'
                  + ' operation was not performed. Please correct'
                  + ' the error and select "AddNew" again.'
                  + ' The cursor will be placed on the row where'
                  + ' the error occurred.'
                  WITH STYLE = POPUP;
            SCROLL $tblfld_name TO :IIrownumber;
            RESUME FIELD $tblfld_name;
        ENDIF;

    ENDIF;
END;
