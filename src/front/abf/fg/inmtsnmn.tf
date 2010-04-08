##      -- template file: inmtsnmn.tf (Master in table field AddNew menuitem)

'AddNew' (ACTIVATE = 1,
        EXPLANATION = 'Insert current screen data into database'),
        KEY FRSKEY12 (ACTIVATE = 1) =
BEGIN
    VALIDATE;        /* validate all fields on form */
##  GENERATE USER_ESCAPE APPEND_START
##
    MESSAGE 'Saving new data . . .';

    IImtries = 0;		/* deadlock retry counter */

## -- $_loopa, $_loopb: defined in inmdammn.tf or inmdanmn.tf
    $_loopa: WHILE (IImtries <= $_deadlock_retry) DO /* deadlock retry loop */
    $_loopb: WHILE (1=1) DO		/* dummy loop for branching */

    UNLOADTABLE $tblfld_name
            (IIrowstate = _STATE, IIrownumber = _RECORD)
    BEGIN
        /* insert NEW, UNCHANGED & CHANGED rows */
        IF ((IIrowstate = 1) OR (IIrowstate = 2)
                             OR (IIrowstate = 3)) THEN

##          GENERATE QUERY INSERT MASTER REPEATED

            INQUIRE_SQL (IIerrorno = ERRORNO);
            IF (IIerrorno = $_deadlock_error) THEN
                IImtries = IImtries + 1;
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

##  GENERATE USER_ESCAPE APPEND_END
    COMMIT WORK;

    INQUIRE_SQL (IIerrorno = ERRORNO);
    IF (IIerrorno != 0) THEN
        ROLLBACK WORK;
        MESSAGE 'An error occurred while Saving this data.'
              + ' Details about the error were described by'
              + ' the message immediately preceding this one.'
              + ' The "AddNew" operation was not performed.'
              + ' Please correct the error and select "AddNew"'
              + ' again.'
              WITH STYLE = POPUP;
        RESUME;
    ENDIF;
    ENDLOOP $_loopa;
    ENDWHILE;				/* $_loopb */
    ENDWHILE;				/* $_loopa */

## DEFINE $_retry_max   $_deadlock_retry+1

    IF (IImtries > $_deadlock_retry) THEN
        ROLLBACK WORK;
        MESSAGE 'Deadlock was detected during all $_retry_max'
              + ' attempts to add this data. The "AddNew" operation'
              + ' was not performed. Please correct the error and'
              + ' select "AddNew" again.'
              WITH STYLE = POPUP;
        RESUME;
    ELSEIF (IImtries > 0) THEN
        /* Save succeeded on a reattempt following deadlock */
        MESSAGE 'Transaction completed successfully.'
              WITH STYLE = POPUP;
    ENDIF;

    SET_FORMS FORM (CHANGE = 0);
    IIsaveasnew = 'y';    /* Set status variable. 'y' means the AddNew
                          ** menuitem has been run on this data.
                          */
    IF (IIclear = 'y') THEN
        MODE 'fill';         /* display default values + clear smpl flds*/
        SET_FORMS FORM (MODE = 'update');  /* cursor off Query-only flds*/
        CLEAR FIELD $tblfld_name;
    ENDIF;

END /* end of 'AddNew' menuitem */
