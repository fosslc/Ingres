##      -- template file: inrinsmn.tf     (RowInsert menuitem)

'RowInsert' (EXPLANATION = 'Open new row in table field'),
    KEY FRSKEY15 =
BEGIN
    INQUIRE_FORMS FIELD '' (IIobjname = 'NAME', IIint = TABLE);
    IF (IIint != 1) THEN
        CALLPROC beep();    /* 4gl built-in procedure */
        MESSAGE 'You can only "RowInsert" when your cursor is'
              + ' in a table field.' WITH STYLE = POPUP;
    ELSE        /* insert a row with _STATE = 0 (Undefined) */
        VALIDROW :IIobjname;    /* error if current row invalid */
        INQUIRE_FORMS TABLE '' (IIint = ROWNO);
        INSERTROW :IIobjname [IIint -1] (_STATE = 0);
    ENDIF;
END
