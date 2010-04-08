##      -- template file: intblfnd.tf   (menuitems Find, Top & Bottom)

KEY FRSKEY7 (EXPLANATION = 'Search table field for a specified value') =
BEGIN
    IIint = CALLPROC find_record();
END

KEY FRSKEY5 (EXPLANATION = 'Scroll to top of table field') =
BEGIN
    INQUIRE_FORMS FIELD '' (IIobjname = 'NAME', IIint = TABLE);
    IF IIint = 0 THEN
        CALLPROC beep();    /* 4gl built-in procedure */
        MESSAGE 'Your cursor must be in a table field'
              + ' before selecting "Top".'
              WITH STYLE = POPUP ;
    ELSE
        SCROLL :IIobjname TO 1;
    ENDIF;
END

KEY FRSKEY6 (EXPLANATION = 'Scroll to bottom of table field') =
BEGIN
    INQUIRE_FORMS FIELD '' (IIobjname = 'NAME', IIint = TABLE);
    IF IIint = 0 THEN
        CALLPROC beep();    /* 4gl built-in procedure */
        MESSAGE 'Your cursor must be in a table field before'
          + ' selecting "Bottom".'
          WITH STYLE = POPUP ;
    ELSE
        SCROLL :IIobjname TO END;
    ENDIF;
END
