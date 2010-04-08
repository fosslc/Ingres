##      -- template file: inrdelmn.tf     (RowDelete menuitem)

'RowDelete' (VALIDATE = 0, ACTIVATE = 0,
    EXPLANATION = 'Delete current row from table field'),
    KEY FRSKEY14 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
    INQUIRE_FORMS FIELD '' (IIobjname = 'NAME', IIint = TABLE);
    IF (IIint != 1) THEN
        CALLPROC beep();    /* 4gl built-in procedure */
        MESSAGE 'You can only "RowDelete" when your cursor is'
              + ' in a table field.' WITH STYLE = POPUP;
    ELSE
        DELETEROW :IIobjname;
        SET_FORMS FORM (CHANGE = 1);
    ENDIF;
END
