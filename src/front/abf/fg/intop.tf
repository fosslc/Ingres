##      -- template file: intop.tf     (Place in initialize block of each frame)
##
SET_FORMS FRS (VALIDATE(NEXTFIELD) = 1, VALIDATE(PREVIOUSFIELD) = 1,
    ACTIVATE(NEXTFIELD) = 1, ACTIVATE(PREVIOUSFIELD) = 1,
    ACTIVATE(MENUITEM) = 1,  ACTIVATE(KEYS) = 1,
    GETMESSAGES = 0);
