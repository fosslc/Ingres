##  --  Master in table field UPDATE frame, template file 'mtupdate.tf'
##INCLUDE indoc.tf
##INCLUDE intopdef.tf
INITIALIZE (
##  GENERATE HIDDEN_FIELDS

    /* working variables needed by template file */
    IIchar1     = CHAR(1),              /* holds answer to Yes/No prompts */
##  IF $insert_master_allowed THEN
    IIclear     = CHAR(1),              /* 'y' means clear after AddNew */
##  ENDIF
##  IF $user_qualified_query THEN
    IIclear2    = CHAR(1),              /* 'y' means clear before query mode*/
##  ENDIF
    IIerrorno   = INTEGER(4),           /* holds DBMS statement error nmbr */
    IIint       = INTEGER(4),           /* general purpose integer */
    IIint2      = INTEGER(4),           /* general purpose integer */
##  IF $lookup_exists THEN
    IIinvalmsg1 = VARCHAR(1),           /* LookUp-table invalid value msg1 */
    IIinvalmsg2 = VARCHAR(70),          /* LookUp-table invalid value msg2 */
##  ENDIF
    IImtries	= INTEGER(4),		/* deadlock retry counter */
    IIobjname   = CHAR(32) NOT NULL,    /* holds an object name */
##  IF $insert_master_allowed THEN
    IIsaveasnew = CHAR(1),              /* tells if AddNew previously run */
##  ENDIF
    IIrowcount  = INTEGER(4),           /* holds DBMS statement row count */
    IIrownumber = INTEGER(4) NOT NULL,  /* holds table field row number */
    IIrowsfound = CHAR(1),              /* tells if query selected >0 rows */
    IIrowstate  = INTEGER(4),           /* holds table field row state */
    IItimeout   = INTEGER(4)            /* Inherited timeout value */
) =
##  GENERATE LOCAL_PROCEDURES DECLARE
BEGIN
##  IF $insert_master_allowed THEN
    IIclear  = 'y';     /* Clear all fields after every AddNew */
##  ENDIF
##  IF $user_qualified_query THEN
    IIclear2 = 'y';     /* CLEAR FIELD ALL before returning to Query mode.
			** Note that switching table field mode to Query
			** clears table field -- no way to stop that.
			*/
##  ENDIF
    IIretval = 1;       /* Success. This built-in global is used for
                        ** communication between frames.
                        */
##  INCLUDE intop.tf
##
##  IF $lookup_exists THEN
##  -- IIinvalmsg1 & IIinvalmsg2 are used in code created by:
##  --     ##GENERATE USER_ESCAPE AFTER_FIELD_ACTIVATES
    IIinvalmsg1 = '';
    IIinvalmsg2 =
        ' is not a valid value for this field (select ListChoices for help).';
##  ENDIF

    /* query mode required for qualification() function */
    SET_FORMS FORM (MODE = 'query');
    SET_FORMS FIELD $form_name (MODE($tblfld_name) = 'query');
##  INCLUDE intimset.tf

##--=========================================================================
##  IF $user_qualified_query THEN -- Run Select after user enters
##  --                               query qualification & selects "Go".
    COMMIT WORK;
    SET AUTOCOMMIT OFF;
##  GENERATE USER_ESCAPE FORM_START
END
##--=========================================================================
##  ELSE                        -- Run query in this Initialize block, without
##                              -- allowing user to type values to qualify
##                              -- the query. (Continuation of the Initialize
##                              -- section started above).
    IIrowsfound = 'n';
##  GENERATE USER_ESCAPE FORM_START

    REDISPLAY;          /* show form and passed in parameters */
##  GENERATE USER_ESCAPE QUERY_START

    MESSAGE 'Selecting data . . .';

    IImtries = 0;		/* deadlock retry counter */

    IIloop0: WHILE (1=1) DO      /* dummy loop for branching */

    /* SELECTing to table field with qualification() function
    ** changes table field mode to Update.
    */
##  GENERATE QUERY SELECT MASTER

##  INCLUDE inmtserr.tf

    /* Assertion: data Selected above */
##  IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
    COMMIT WORK;            /* release locks */
##  ENDIF

##  IF $insert_master_allowed THEN
    IIsaveasnew = 'n';  /* Set status variable. 'n' means the AddNew
                        ** menuitem has not been run on this data.
                        */
    /* put table field in 'fill' mode so can open rows at bottom */
    SET_FORMS FIELD $form_name (MODE($tblfld_name) = 'fill');
##  ENDIF
##
    SET_FORMS FORM (MODE = 'update');
    IIrowsfound = 'y';          /* indicate that >0 rows qualified */

    DISPLAY SUBMENU
    BEGIN
    INITIALIZE =
    BEGIN
    END
##  -- repeat on-timeout and on-dbevent escape code in the submenu
##  INCLUDE intimout.tf
##  INCLUDE indbevnt.tf
##
##  GENERATE USER_MENUITEMS
##  GENERATE USER_ESCAPE MENULINE_MENUITEMS
##
##  INCLUDE inmtsvmn.tf         -- master in table field Save menuitem.
##
##  IF $insert_master_allowed THEN
##  DEFINE $_loopa 'IIloop3'
##  DEFINE $_loopb 'IIloop4'
##  INCLUDE inmtsnmn.tf         -- master in table field AddNew menuitem.
##  ENDIF        -- $insert_master_allowed
##
##  IF $delete_master_allowed THEN
##  INCLUDE inmtdlmn.tf         -- master in table field Delete menuitem.
##  ENDIF
##
##  IF $delete_master_allowed THEN
##  INCLUDE inrdelmn.tf		-- RowDelete menuitem
##  ENDIF
##
##  IF $insert_master_allowed THEN
##  INCLUDE inrinsmn.tf		-- RowInsert menuitem
##  ENDIF
##
##  INCLUDE inlookup.tf         -- ListChoices menuitem.
##
##  INCLUDE intblfnd.tf -- menu commands: 'Find', 'Top' & 'Bottom'. 

    'Help' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Display help for this frame'),
        KEY FRSKEY1 (VALIDATE = 0, ACTIVATE = 0) =
    BEGIN
##      GENERATE HELP fgmdunnp.hlp
    END
##
##  IF $default_start_frame THEN
##  ELSE

    'TopFrame' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Return to top frame in application'),
        KEY FRSKEY17 (VALIDATE = 0, ACTIVATE = 0) =
    BEGIN
##
##      INCLUDE inend.tf        -- check if want to save changes before leave.
        IIretval = 2;           /* return to top frame */
        ENDLOOP IIloop0;         /* exit WHILE loop */
    END
##  ENDIF -- $default_start_frame 

    'End' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Return to previous frame'),
        KEY FRSKEY3 (VALIDATE = 0, ACTIVATE = 0) =
    BEGIN
##
##      INCLUDE inend.tf        -- check if want to save changes before leave.
        ENDLOOP IIloop0;         /* exit WHILE loop */
    END
##
##  GENERATE USER_ESCAPE BEFORE_FIELD_ACTIVATES
##  GENERATE USER_ESCAPE AFTER_FIELD_ACTIVATES  -- Field-Change & Field-Exit
##
    END;        /* end of submenu in Initialize block */
##
    ENDLOOP IIloop0;             /* exit WHILE loop */

    ENDWHILE;   /* end of loop:  "IIloop0: WHILE (1=1) DO" */

##  GENERATE USER_ESCAPE QUERY_END
    /* Above submenu exited via either: Zero rows retrieved;
    ** Error happened selecting data (and hence Zero rows retrieved);
    ** TopFrame, End or Save menuitem; Escape code issued "ENDLOOP IIloop0;".
    */
    INQUIRE_SQL (IIerrorno = ERRORNO);

##  IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
    COMMIT WORK;
##  ENDIF
##
##  IF $insert_master_allowed THEN
##
    IIchar1 = 'n';      /* reset below if (IIrowsfound = 'n') */

    IF ((IIrowsfound = 'n') AND (IIretval != -1)) THEN
        /* no rows retrieved above */
        IF (IIerrorno > 0) THEN
            IIretval = -1;      /* error */
        ENDIF;

        IIchar1 =
            PROMPT 'No rows retrieved. Do you wish to add new data (y/n)?';
    ENDIF;

    IF (UPPERCASE(:IIchar1) != 'Y') THEN
##      GENERATE USER_ESCAPE FORM_END
	SET_FORMS FRS (TIMEOUT = IItimeout); /* Restore saved timeout value */
        RETURN $default_return_value;
    ELSE
        /* Get set to Append new rows */
        SET_FORMS FORM (MODE = 'update');
        SET_FORMS FIELD $form_name (MODE($tblfld_name) = 'fill');
        SET_FORMS FORM (CHANGE = 0);
    ENDIF;
##
##  ELSE        -- (not allowed to insert new masters).
##
    IF (IIrowsfound = 'n') THEN         /* no rows retrieved above */
        IF (IIerrorno > 0) THEN
            IIretval = -1;      /* error */
        ENDIF;
    ENDIF;

##  GENERATE USER_ESCAPE FORM_END
    SET_FORMS FRS (TIMEOUT = IItimeout); /* Restore saved timeout value */
    RETURN $default_return_value;
##
##  ENDIF       -- $insert_master_allowed
##
END     /* End of INITIALIZE section */
##
##INCLUDE intimout.tf
##INCLUDE indbevnt.tf

##
##IF $insert_master_allowed THEN
##
##DEFINE $_loopa 'IIloop7'
##DEFINE $_loopb 'IIloop8'
##INCLUDE inmtanmn.tf           -- Menuitems for Appending new data
##
##ELSE

'End (Error: check for RESUME stmt in Escape Code)'
    (VALIDATE = 0, ACTIVATE = 0),
    KEY FRSKEY3 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
    /* This menuitem is only needed to eliminate the compile errors
    ** that happen when all a frame's code is in the INITIALIZE block.
    ** This menuitem is not displayed unless a RESUME was issued in
    ** a FORM-START, or QUERY-START escape (which is an error).
    */
    SET_FORMS FRS (TIMEOUT = IItimeout); /* Restore saved timeout value */
    RETURN $default_return_value;
END
##ENDIF -- $insert_master_allowed 
##ENDIF -- $user_qualified_query
##--=========================================================================
##IF $user_qualified_query THEN -- Run Select after user enters
##  --                             query qualification & selects "Go".
##GENERATE USER_MENUITEMS
##GENERATE USER_ESCAPE MENULINE_MENUITEMS

'Go' (EXPLANATION = 'Run query'), KEY FRSKEY4 =
BEGIN
    IIrowsfound = 'n';

##  GENERATE USER_ESCAPE QUERY_START
##
    MESSAGE 'Selecting data . . .';

    /* SELECTing to table field with qualification() function
    ** changes table field mode to Update, even if 0 rows selected.
    */
##  GENERATE QUERY SELECT MASTER

##  INCLUDE inmtserr.tf

    /* Assertion: rows Selected above */
##  IF ('$locks_held' = 'none' OR '$locks_held' = 'optimistic') THEN
    COMMIT WORK;                        /* release locks */
##  ENDIF

##  IF $insert_master_allowed THEN
    IIsaveasnew = 'n';    /* Set status variable. 'n' means the AddNew
                          ** menuitem has not been run on this data.
                          */
    SET_FORMS FIELD $form_name (MODE($tblfld_name) = 'fill');
##  ENDIF
##
    SET_FORMS FORM (CHANGE = 0);        /* typing query Qual set change=1 */
    IIrowsfound = 'y';    /* indicate that >0 rows qualified */
    SET_FORMS FORM (MODE = 'update');

    DISPLAY SUBMENU
    BEGIN
    INITIALIZE =
    BEGIN
    END
##  -- repeat on-timeout and on-dbevent escape code in the submenu
##  INCLUDE intimout.tf
##  INCLUDE indbevnt.tf
##
##  GENERATE USER_MENUITEMS
##  GENERATE USER_ESCAPE MENULINE_MENUITEMS
##
##  INCLUDE inmtsvmn.tf         -- Save menuitem.
##
##  IF $insert_master_allowed THEN
##  DEFINE $_loopa 'IIloop3'
##  DEFINE $_loopb 'IIloop4'
##  INCLUDE inmtsnmn.tf         -- AddNew menuitem.
##  ENDIF
##
##  IF $delete_master_allowed THEN
##  INCLUDE inmtdlmn.tf         -- master in table field Delete menuitem.
##  ENDIF
##
##  IF $delete_master_allowed THEN
##  INCLUDE inrdelmn.tf		-- RowDelete menuitem
##  ENDIF
##
##  IF $insert_master_allowed THEN
##  INCLUDE inrinsmn.tf		-- RowInsert menuitem
##  ENDIF
##
##  INCLUDE inlookup.tf         -- ListChoices menuitem.
##
##  INCLUDE intblfnd.tf -- menu commands: 'Find', 'Top' & 'Bottom'.

    'Help' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Display help for this frame'),
        KEY FRSKEY1 (VALIDATE = 0, ACTIVATE = 0) =
    BEGIN
##      GENERATE HELP fgmduxqp.hlp
    END

    'End' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Return from Update mode to Query mode'),
        KEY FRSKEY3 (VALIDATE = 0, ACTIVATE = 0) =
    BEGIN
##      INCLUDE inendsub.tf	-- check if want to save changes before leave.
        ENDLOOP;        /* exit submenu */
    END
##
##  GENERATE USER_ESCAPE BEFORE_FIELD_ACTIVATES
##  GENERATE USER_ESCAPE AFTER_FIELD_ACTIVATES  -- Field-Change & Field-Exit
##
    END;        /* end of submenu in 'Go' menuitem */

    SET_FORMS FORM (MODE = 'query');    /* so user can enter another query */
    SET_FORMS FIELD $form_name (MODE($tblfld_name) = 'query');
##
##  GENERATE USER_ESCAPE QUERY_END
##

    IF ((IIrowsfound = 'y')
##      IF $user_qualified_query THEN
        AND (IIclear2 = 'y')
##      ENDIF
       ) THEN
        CLEAR FIELD ALL;
        SET_FORMS FORM (CHANGE = 0);
    ENDIF;
END /* end of 'Go' menuitem */
##
##IF $insert_master_allowed THEN
##
##DEFINE $_loopa 'IIloop7'
##DEFINE $_loopb 'IIloop8'
##INCLUDE inmtammn.tf           -- AppendMode menuitem.
##
##ENDIF    -- $insert_master_allowed

'Clear' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Clear all fields'),
        KEY FRSKEY16 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
    CLEAR FIELD ALL;
END
##
##INCLUDE inlookup.tf           -- ListChoices menuitem.

'Help' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Display help for this frame'),
        KEY FRSKEY1 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
##  GENERATE HELP fgmtupdq.hlp
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
    RETURN $default_return_value;
END
##ENDIF -- $default_start_frame

##  -- repeat on-timeout and on-dbevent escape code in the main loop
##  INCLUDE intimout.tf
##  INCLUDE indbevnt.tf

'End' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Return to previous frame'),
        KEY FRSKEY3 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
##  GENERATE USER_ESCAPE FORM_END
    SET_FORMS FRS (TIMEOUT = IItimeout); /* Restore saved timeout value */
    RETURN $default_return_value;
END
##
##GENERATE USER_ESCAPE BEFORE_FIELD_ACTIVATES
##GENERATE USER_ESCAPE AFTER_FIELD_ACTIVATES  -- Field-Change & Field-Exit
##ENDIF     -- $user_qualified_query
##--=======================================================================

##GENERATE LOCAL_PROCEDURES
