##      -- template file: inmdsnmn.tf (Master/detail AddNew menuitem)

'AddNew' (ACTIVATE = 1,
        EXPLANATION = 'Insert current screen data into database'),
        KEY FRSKEY12 (ACTIVATE = 1) =
BEGIN
    VALIDATE;        /* validate all fields on form */
##  GENERATE USER_ESCAPE APPEND_START
##
    MESSAGE 'Saving new data . . .';

    IImtries = 0;	/* deadlock retry counter (master table) */
    IIdtries = 0;	/* deadlock retry counter (detail table) */
## -- $_loopa, $_loopb: defined in inmdammn.tf or inmdanmn.tf
    $_loopa:	/* loop to retry if deadlock is encountered */
    WHILE (IImtries <= $_deadlock_retry AND IIdtries <= $_deadlock_retry) DO
    $_loopb: WHILE (1=1) DO	/* dummy loop for branching */

##  INCLUDE inmins.tf   -- INSERT for master

##  INCLUDE indins.tf   -- unloadtable loop to INSERT details

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

##  INCLUDE indeadlk.tf  -- Generate success/failure message after deadlock
##
    SET_FORMS FORM (CHANGE = 0);
    IIsaveasnew = 'y';    /* Set status variable. 'y' means the AddNew
                          ** menuitem has been run on this data.
                          */
    IF (IIclear = 'y') THEN
        MODE 'fill';        /* display default values + clear smpl flds*/
        SET_FORMS FORM (MODE = 'update'); /* cursor off Query-only flds*/
        CLEAR FIELD $tblfld_name;
    ENDIF;

END /* end of 'AddNew' menuitem */
