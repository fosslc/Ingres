##   -- template file: inendsub.tf  (Check for changes before End from SubMenu)
##
INQUIRE_FORMS FORM (IIint = CHANGE);
IF (IIint = 1) THEN
    IIchar1 = PROMPT 'Do you wish to leave $frame_type'
                           + ' without saving changes (y/n)? ';
    IF (UPPERCASE(:IIchar1) != 'Y') THEN
        MESSAGE 'Select menuitem "Save" to save changes.';
        SLEEP 2;
        RESUME MENU;
    ENDIF;
ENDIF;
