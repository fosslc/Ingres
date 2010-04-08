##      -- template file: inlookup.tf   (ListChoices menuitem)

'ListChoices' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Show valid values for current field'),
        KEY FRSKEY10 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
    IIint = -1;
##
##  IF $lookup_exists THEN
##
##  GENERATE USER_ESCAPE BEFORE_LOOKUP
##
##  GENERATE LOOKUP     -- resets IIint if look_up is called

    IF (IIint < 0) THEN
        /* No look_up available for current field */
        IIint = CALLPROC help_field();
##
##  IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
    ELSE
        /* look_up was done above */
        COMMIT WORK;    /* release shared locks on lookup table */
##  ENDIF
##
    ENDIF;
##
##  GENERATE USER_ESCAPE AFTER_LOOKUP
##
##  ELSE
##
    IIint = CALLPROC help_field();
##
##  ENDIF       -- $lookup_exists

    IF (IIint > 0) THEN
        /* value was selected onto form */
        SET_FORMS FORM (CHANGE = 1);
    ENDIF;
END
