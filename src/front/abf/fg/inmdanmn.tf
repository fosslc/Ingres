##      -- template file: inmdanmn.tf  (Master/detail Append new data)
##GENERATE USER_MENUITEMS
##
##INCLUDE inmdsnmn.tf         -- master/detail AddNew menuitem.
##
##INCLUDE inirmn.tf           -- RowInsert, RowDelete menuitems.
##
##INCLUDE inclear.tf          -- Clear menuitem
##
##INCLUDE inlookup.tf         -- ListChoices menuitem.
##
##INCLUDE intblfnd.tf         -- menu commands: 'Find', 'Top' & 'Bottom'. 

'Help' (VALIDATE = 0, ACTIVATE = 0,
    EXPLANATION = 'Display help for this frame'),
    KEY FRSKEY1 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
##  GENERATE HELP fgmdupda.hlp
END

'End' (VALIDATE = 0, ACTIVATE = 0,
    EXPLANATION = 'Return to previous frame'),
    KEY FRSKEY3 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
    INQUIRE_FORMS FORM (IIint = CHANGE);
    IF (IIint = 1) THEN
        IIchar1 =
            PROMPT 'Do you wish to leave without saving (y/n)?';
        IF (UPPERCASE(:IIchar1) != 'Y') THEN
            MESSAGE 'Select menuitem "AddNew" to save.';
            SLEEP 2;
            RESUME MENU;
        ENDIF;
    ENDIF;
##  GENERATE USER_ESCAPE FORM_END
    SET_FORMS FRS (TIMEOUT = IItimeout); /* Restore saved timeout value */
    RETURN $default_return_value;
END
##
##GENERATE USER_ESCAPE BEFORE_FIELD_ACTIVATES
##GENERATE USER_ESCAPE AFTER_FIELD_ACTIVATES  -- Field-Change & Field-Exit
