##  -- template file: 'ntmenu.tf'       (Menu frame template)
##
##INCLUDE indoc.tf
##INCLUDE intopdef.tf
INITIALIZE (
##  GENERATE HIDDEN_FIELDS

##  IF $tablefield_menu THEN
    /* working variables needed by generated code */
    IIobjname   = CHAR(32) NOT NULL,    /* Holds an object name */
    IIchoice    = VARCHAR(80),          /* Menu item chosen */
    IIfound     = INTEGER(4),           /* Was the chosen Menu item found */
##  ENDIF
    IIint       = INTEGER(4),           /* General purpose integer */
    IItimeout   = INTEGER(4)            /* Inherited timeout value */
) =
##  GENERATE LOCAL_PROCEDURES DECLARE
BEGIN
    IIretval = 1;       /* Success. This built-in global is used for
                        ** communication between frames.
                        */
##  INCLUDE intimset.tf
##  INCLUDE intop.tf
##
##  IF $tablefield_menu THEN

    INITTABLE $tblfld_name READ;
##  GENERATE LOAD_MENUITEMS
##  ENDIF
##  GENERATE USER_ESCAPE FORM_START
END
##
##INCLUDE intimout.tf
##INCLUDE indbevnt.tf
##
##IF $tablefield_menu THEN

'Select' (VALIDATE = 1, ACTIVATE = 1,
         EXPLANATION = 'Select a command'),
         KEY FRSKEY4 (VALIDATE = 1, ACTIVATE = 1) =
BEGIN
    INQUIRE_FORMS FIELD '' (IIobjname = 'NAME', IIint = TABLE);
    IF (IIint != 1 OR IIobjname != '$tblfld_name') THEN
        CALLPROC beep();    /* 4gl built-in procedure */
        MESSAGE 'You can only "Select" when your cursor is in the' +
                ' table field with the commands.'
        WITH STYLE = POPUP;
        RESUME;
    ENDIF;
    
    INQUIRE_FORMS TABLE '' (IIint = DATAROWS($tblfld_name));
    IF (IIint = 0) THEN
        RESUME;        /* No commands in tablefield */
    ENDIF;

    IIfound = 0;
    IIchoice = $tblfld_name[].command;
##  GENERATE USER_MENUITEMS LISTCHOICES

    IF (IIfound = 0) THEN
##      GENERATE USER_ESCAPE TABLE_FIELD_MENUITEMS
        IF (IIfound = 0) THEN
            MESSAGE '"' + IIchoice + '" is an unrecognized command.'
            WITH STYLE = POPUP;
	ENDIF;
    ENDIF;

    IF ((IIretval = 2) AND
        (ii_frame_name('current') <> ii_frame_name('entry'))) THEN
        /* Return to top (this is not the start frame) */
	SET_FORMS FRS (TIMEOUT = IItimeout); /* Restore saved timeout value */
        RETURN $default_return_value;
    ELSE
        IIretval = 1;   /* restore default value */
    ENDIF;
END
##ELSE
##GENERATE USER_MENUITEMS
##ENDIF
##
##GENERATE USER_ESCAPE MENULINE_MENUITEMS
##
##IF $tablefield_menu THEN
##INCLUDE intblfnd.tf
##ENDIF

'Help' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Display help for this frame'),
        KEY FRSKEY1 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
##  IF $tablefield_menu THEN
##  GENERATE HELP fgntlcmp.hlp
##  ELSE
##  GENERATE HELP fgntmnup.hlp
##  ENDIF
END
##
##IF $default_start_frame THEN
##ELSE

'TopFrame' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Return to top frame in application'),
        KEY FRSKEY17 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
    IIretval = 2;       /* return to top frame */
##  GENERATE USER_ESCAPE FORM_END
    SET_FORMS FRS (TIMEOUT = IItimeout); /* Restore saved timeout value */
    RETURN $default_return_value ;
END
##ENDIF -- $default_start_frame 

'End' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Return to previous frame'),
        KEY FRSKEY3 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
##  GENERATE USER_ESCAPE FORM_END
    SET_FORMS FRS (TIMEOUT = IItimeout); /* Restore saved timeout value */
    RETURN $default_return_value ;
END
##
##GENERATE USER_ESCAPE BEFORE_FIELD_ACTIVATES
##GENERATE USER_ESCAPE AFTER_FIELD_ACTIVATES  -- Field-Change & Field-Exit

##GENERATE LOCAL_PROCEDURES
