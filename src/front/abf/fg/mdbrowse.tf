##  --  Master-Detail BROWSE frame, template file 'mdbrowse.tf'
##INCLUDE indoc.tf
##INCLUDE intopdef.tf
INITIALIZE (
##  GENERATE HIDDEN_FIELDS

    /* working variables needed by template file */
##  IF $user_qualified_query THEN
    IIclear2    = CHAR(1),              /* 'y' means clear before query mode*/
##  ENDIF
    IIdtries	= INTEGER(4),		/* deadlock retry counter (detail) */
    IIerrorno   = INTEGER(4),           /* holds DBMS statement error number*/
    IIint       = INTEGER(4),           /* general purpose integer */
    IIint2      = INTEGER(4),           /* general purpose integer */
    IIobjname   = CHAR(32) NOT NULL,    /* holds an object name */
    IIrowcount  = INTEGER(4),           /* holds DBMS statement row count */
    IIrowsfound = CHAR(1),              /* tells if query selected >0 rows */
    IItimeout   = INTEGER(4)            /* Inherited timeout value */
) =
##  GENERATE LOCAL_PROCEDURES DECLARE
BEGIN
##  IF $user_qualified_query THEN
    IIclear2 = 'y';     /* Clear all fields before returning to Query mode */
##  ENDIF
    IIretval = 1;       /* Success. This built-in global is used for
                        ** communication between frames.
                        */
##  INCLUDE intop.tf
##
    INITTABLE $tblfld_name READ;
    /* form must be in query mode for qualification() function */
    SET_FORMS FORM (MODE = 'query');
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

    IIloop0: WHILE (1=1) DO      /* dummy loop for branching */
##---------------------------------------------------------------------------
##  IF $singleton_select THEN
##
    /* Since we are only interested in the first row returned by
    ** this query, improve performance by running it as a singleton
    ** query, rather than as a query with an attached menu.
    */
##  GENERATE QUERY SELECT MASTER

##  INCLUDE inmsserr.tf		-- Error handling

##  GENERATE QUERY SELECT DETAIL REPEATED

##  INCLUDE indserr.tf		-- Error handling

    /* Assertion: master row Selected above */
    COMMIT WORK;                        /* release locks */

    SET_FORMS FORM (MODE = 'read');
    IIrowsfound = 'y';    /* indicate that >0 rows qualified */

    DISPLAY SUBMENU
    BEGIN
    INITIALIZE =
    BEGIN
##      GENERATE USER_ESCAPE QUERY_NEW_DATA    -- run only for first master
    END
##  -- repeat on-timeout and on-dbevent escape code in the submenu
##  INCLUDE intimout.tf
##  INCLUDE indbevnt.tf
##
##  GENERATE USER_MENUITEMS
##  GENERATE USER_ESCAPE MENULINE_MENUITEMS
##---------------------------------------------------------------------------
##  ELSE          -- Query with attached menu (!= $singleton_select)

##  GENERATE QUERY SELECT MASTER_DETAIL REPEATED NOTERM
    BEGIN
    INITIALIZE =
    BEGIN
##      INCLUDE inmsserr.tf		-- Error handling
	COMMIT WORK;          /* release locks */
        SET_FORMS FORM (MODE = 'read');
        IIrowsfound = 'y';    /* indicate that >0 rows qualified */
##      GENERATE USER_ESCAPE QUERY_NEW_DATA    -- run only for first master
    END

##  -- repeat on-timeout and on-dbevent escape code in the submenu
##  INCLUDE intimout.tf
##  INCLUDE indbevnt.tf
##
##  GENERATE USER_MENUITEMS
##  GENERATE USER_ESCAPE MENULINE_MENUITEMS

    'Next' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Display next row of selected data'),
        KEY FRSKEY4 (VALIDATE = 0, ACTIVATE = 0) =
    BEGIN 
        NEXT;
##
        COMMIT WORK;    /* Release any locks acquired while selecting data
                        ** for the "NEXT" statement.
                        */
##
##      GENERATE USER_ESCAPE QUERY_NEW_DATA     --run for all but first master
    END
##
##  ENDIF         -- $singleton_select
##---------------------------------------------------------------------------
##-- Add these menuitems to both singleton Select and Queries with Next:
##
##  INCLUDE intblfnd.tf -- menu commands: 'Find', 'Top' & 'Bottom'. 

    'Help' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Display help for this frame'),
        KEY FRSKEY1 (VALIDATE = 0, ACTIVATE = 0) =
    BEGIN
##      IF $singleton_select THEN
##      GENERATE HELP fgmdbnnp.hlp
##      ELSE
##      GENERATE HELP fgmdbxnp.hlp
##      ENDIF
    END
##
##  IF $default_start_frame THEN
##  ELSE

    'TopFrame' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Return to top frame in application'),
        KEY FRSKEY17 (VALIDATE = 0, ACTIVATE = 0) =
    BEGIN
        IIretval = 2;           /* return to top frame */
        ENDLOOP IIloop0;         /* exit WHILE loop */
    END
##  ENDIF -- $default_start_frame

    'End' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Return to previous frame'),
        KEY FRSKEY3 (VALIDATE = 0, ACTIVATE = 0) =
    BEGIN
        ENDLOOP IIloop0;         /* exit WHILE loop */
    END
##
##  GENERATE USER_ESCAPE BEFORE_FIELD_ACTIVATES
##
    END;        /* END of submenu in Initialize block */

    ENDLOOP IIloop0;             /* exit WHILE loop */

    ENDWHILE;   /* end of loop:  "IIloop0: WHILE (1=1) DO" */

##  GENERATE USER_ESCAPE QUERY_END
    /* User exited above submenu via either: Zero rows retrieved; NEXT past
    ** last selected master; TopFrame or End menuitem; Error
    ** happened selecting data (and hence Zero rows retrieved);
    ** Escape code issued "ENDLOOP IIloop0;".
    */
    INQUIRE_SQL (IIerrorno = ERRORNO);

    COMMIT WORK;

    IF (IIrowsfound = 'n') THEN         /* no rows retrieved above */
        IF (IIerrorno > 0) THEN
            IIretval = -1;      /* error */
        ENDIF;
    ENDIF;

##  GENERATE USER_ESCAPE FORM_END
    SET_FORMS FRS (TIMEOUT = IItimeout); /* Restore saved timeout value */
    RETURN $default_return_value;
END     /* End of INITIALIZE section */
##
##INCLUDE intimout.tf
##INCLUDE indbevnt.tf

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
##ENDIF -- $user_qualified_query
##--=========================================================================
##IF $user_qualified_query THEN -- Run Select after user enters
##  --                             query qualification & selects "Go".
##
##GENERATE USER_MENUITEMS
##GENERATE USER_ESCAPE MENULINE_MENUITEMS

'Go' (EXPLANATION = 'Run query'), KEY FRSKEY4 =
BEGIN
    IIrowsfound = 'n';

##  GENERATE USER_ESCAPE QUERY_START
##
    MESSAGE 'Selecting data . . .';

##---------------------------------------------------------------------------
##IF $singleton_select THEN
##
    /* Since we are only interested in the first row returned by
    ** this query, improve performance by running it as a singleton
    ** query, rather than as a query with an attached menu.
    */
##  GENERATE QUERY SELECT MASTER

##  INCLUDE inmsserr.tf		-- Error handling

##  GENERATE QUERY SELECT DETAIL REPEATED

##  INCLUDE indserr.tf		-- Error handling

    /* Assertion: master row Selected above */
    COMMIT WORK;                        /* release locks */

    IIrowsfound = 'y';    /* indicate that >0 rows qualified */
    SET_FORMS FORM (MODE = 'read');

    DISPLAY SUBMENU
    BEGIN
    INITIALIZE =
    BEGIN
##      GENERATE USER_ESCAPE QUERY_NEW_DATA    -- run only for first master
    END
##  -- repeat on-timeout and on-dbevent escape code in the submenu
##  INCLUDE intimout.tf
##  INCLUDE indbevnt.tf
##
##  GENERATE USER_MENUITEMS
##  GENERATE USER_ESCAPE MENULINE_MENUITEMS
##---------------------------------------------------------------------------
##ELSE          -- Query with attached menu (!= $singleton_select)
##
##  GENERATE QUERY SELECT MASTER_DETAIL REPEATED NOTERM

    BEGIN
    INITIALIZE =
    BEGIN
##      INCLUDE inmsserr.tf		-- Error handling
	COMMIT WORK;        /* release locks */
        IIrowsfound = 'y';    /* indicate that >0 rows qualified */
        SET_FORMS FORM (MODE = 'read');
##      GENERATE USER_ESCAPE QUERY_NEW_DATA     -- run only for first master 
    END

##  -- repeat on-timeout and on-dbevent escape code in the submenu
##  INCLUDE intimout.tf
##  INCLUDE indbevnt.tf
##
##  GENERATE USER_MENUITEMS
##  GENERATE USER_ESCAPE MENULINE_MENUITEMS

    'Next' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Display next row of selected data'),
        KEY FRSKEY4 (VALIDATE = 0, ACTIVATE = 0) =
    BEGIN 
        NEXT;
##
        COMMIT WORK;    /* Release any locks acquired while selecting data
                        ** for the "NEXT" statement.
                        */
##
##      GENERATE USER_ESCAPE QUERY_NEW_DATA     --run for all but first master
    END
##ENDIF         -- $singleton_select
##---------------------------------------------------------------------------
##-- Add these menuitems to both singleton Select and Queries with Next:
##
##  INCLUDE intblfnd.tf -- menu commands: 'Find', 'Top' & 'Bottom'. 

    'Help' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Display help for this frame'),
        KEY FRSKEY1 (VALIDATE = 0, ACTIVATE = 0) =
    BEGIN
##      IF $singleton_select THEN
##      GENERATE HELP fgmdbnqp.hlp
##      ELSE
##      GENERATE HELP fgmdbxqp.hlp
##      ENDIF
    END

    'End' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Return from Browse mode to Query mode'),
        KEY FRSKEY3 (VALIDATE = 0, ACTIVATE = 0) =
    BEGIN
        ENDLOOP;        /* exit submenu */
    END
##
##  GENERATE USER_ESCAPE BEFORE_FIELD_ACTIVATES
##
    END;        /* END of submenu in 'Go' menuitem */

##  IF $singleton_select THEN
    SET_FORMS FORM (MODE = 'query');    /* so user can enter another query */
##  ELSE
    IF (IIrowsfound = 'n') THEN
        COMMIT WORK;          /* release locks if "SELECT" is unsuccessful */
    ENDIF;
##  ENDIF
##
##  GENERATE USER_ESCAPE QUERY_END

    IF ((IIrowsfound = 'y')
##      IF $user_qualified_query THEN
        AND (IIclear2 = 'y')
##      ENDIF
       ) THEN
        CLEAR FIELD ALL;
    ENDIF;
END /* end of 'Go' menuitem */

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
##  GENERATE HELP fgmdbrsq.hlp
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
##ENDIF     -- $user_qualified_query
##--=========================================================================

##GENERATE LOCAL_PROCEDURES
