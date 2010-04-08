##      -- template file: inclear.tf    (menuitem "Clear", with confirmation)
##

'Clear' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Clear all fields'),
        KEY FRSKEY16 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
    IIchar1 = 'Y';

    INQUIRE_FORMS FORM (IIint = CHANGE);
    IF (IIint = 1) THEN
        /* User typed data on form. Confirm before CLEAR. */
        IIchar1 = PROMPT 'Do you wish to clear the form (y/n)?';
    ENDIF;

    IF (UPPERCASE(:IIchar1) = 'Y') THEN
        CLEAR FIELD ALL;
        SET_FORMS FORM (CHANGE = 0);
    ENDIF;
END
