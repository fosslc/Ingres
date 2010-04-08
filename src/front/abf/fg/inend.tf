##      -- template file: inend.tf      (Check for changes before Ending)
##
INQUIRE_FORMS FORM (IIint = CHANGE);
IF (IIint = 1) THEN
    IIchar1 = PROMPT 'Do you wish to leave $frame_type'
                   + ' without saving changes (y/n)? ';
    IF (UPPERCASE(:IIchar1) != 'Y') THEN
        RESUME;
    ENDIF;
ENDIF;
