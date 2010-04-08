##      -- template file: innxmn.tf     (Next menuitem)

'Next' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Display next row of selected data'),
        KEY FRSKEY4 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
    INQUIRE_FORMS FORM (IIint = CHANGE);
    IF (IIint = 1) THEN
        IIchar1 = PROMPT
            'Do you wish to "Next" without saving changes (y/n)? ';
        IF (UPPERCASE(:IIchar1) != 'Y') THEN
            MESSAGE 'Select menuitem "Save" to save changes.';
            SLEEP 2;
            RESUME MENU;
        ENDIF;
    ENDIF;
##
    SET_FORMS FORM (CHANGE = 0);
    NEXT;
##
##  IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
    COMMIT WORK;    /* Release any locks acquired while selecting
                    ** data for the "NEXT" statement.
                    */
##
##  IF $insert_master_allowed THEN
    IIsaveasnew = 'n';    /* Set status variable. 'n' means the AddNew
                          ** menuitem has not been run on this data.
                          */
##  ENDIF
##
##  ENDIF
##
##  GENERATE USER_ESCAPE QUERY_NEW_DATA
END
