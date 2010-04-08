##  -- Master-only in simple fields APPEND frame; template file 'msappend.tf'
##INCLUDE indoc.tf
##INCLUDE intopdef.tf
INITIALIZE (
##  GENERATE HIDDEN_FIELDS

    /* working variables needed by generated code */
    IIchar1     = CHAR(1),              /* holds answer to Yes/No prompts */
    IIclear     = CHAR(1),              /* 'y' means clear after Save */
    IIerrorno   = INTEGER(4),           /* holds DBMS statement error number */
    IIint       = INTEGER(4),           /* general purpose integer */
    IIint2      = INTEGER(4),           /* general purpose integer */
##  IF $lookup_exists THEN
    IIinvalmsg1 = VARCHAR(1),           /* LookUp-table invalid value msg1 */
    IIinvalmsg2 = VARCHAR(70),          /* LookUp-table invalid value msg2 */
##  ENDIF
    IImtries	= INTEGER(4),		/* deadlock retry counter */
    IIobjname   = CHAR(32) NOT NULL,    /* holds an object name */
    IIrowcount  = INTEGER(4),           /* holds DBMS statement row count */
    IItimeout   = INTEGER(4)            /* Inherited timeout value */
) =
##  GENERATE LOCAL_PROCEDURES DECLARE
BEGIN
    IIclear  = 'y';     /* Clear screen after every Save */
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
##
    SET_FORMS FORM (MODE = 'update');   /* keep cursor out of Query-only flds */
##  INCLUDE intimset.tf

    COMMIT WORK;
    SET AUTOCOMMIT OFF;
##
##  IF $master_seqfld_exists THEN
##  IF $master_seqfld_used THEN
##      -- generate new value for sequenced (surrogate) field.
##  INCLUDE inseqfld.tf
    COMMIT WORK;        /* success above incrementing sequenced field */
##  ENDIF       -- master_seqfld_used
##  ENDIF       -- master_seqfld_exists
##
##  GENERATE SET_DEFAULT_VALUES SIMPLE_FIELDS  -- defaults set in Visual Query
##  GENERATE USER_ESCAPE FORM_START
END
##
##INCLUDE intimout.tf
##INCLUDE indbevnt.tf
##
##GENERATE USER_MENUITEMS
##GENERATE USER_ESCAPE MENULINE_MENUITEMS

'Save' (ACTIVATE = 1,
        EXPLANATION = 'Write current screen data to database'),
        KEY FRSKEY8 (ACTIVATE = 1) =
BEGIN
    VALIDATE;   /* VALIDATE all fields on form */

##  IF $master_seqfld_exists THEN
##  IF $master_seqfld_used THEN
##  ELSE
##      -- frame has a non-used sequenced field. increment it.
##  INCLUDE inseqfld.tf
    COMMIT WORK;        /* success above incrementing sequenced field */
##  ENDIF       -- master_seqfld_used
##  ENDIF       -- master_seqfld_exists
##
##  GENERATE USER_ESCAPE APPEND_START
##
    MESSAGE 'Saving . . .';

    IImtries = 0;		/* Deadlock retry counter */

    /* We loop more than once only if a deadlock is encountered */

    IIloop1: WHILE (IImtries <= $_deadlock_retry) DO
    IIloop2: WHILE (1=1) DO	/* dummy loop for branching */
##  DEFINE $_loopb 'IIloop2'
##
##  GENERATE QUERY INSERT MASTER REPEATED

##  INCLUDE inmsuerr.tf		-- Error handling, including deadlock
##
##  GENERATE USER_ESCAPE APPEND_END

    COMMIT WORK;

    INQUIRE_SQL (IIerrorno = ERRORNO);
    IF (IIerrorno != 0) THEN
        ROLLBACK WORK;
        MESSAGE 'An error occurred while Saving this data.'
              + ' Details about the error were described by'
              + ' the message immediately preceding this one.'
              + ' The "Save" operation was not performed.'
              + ' Please correct the error and select "Save"'
              + ' again.'
              WITH STYLE = POPUP;
        RESUME;
    ENDIF;
    ENDLOOP IIloop1;
    ENDWHILE;			/* IIloop2 */

    /* Deadlocks pass through here; outer loop reruns */
    ENDWHILE;			/* IIloop1 */

##  INCLUDE indeadlk.tf	 -- Generate success/failure message after deadlock

    SET_FORMS FORM (CHANGE = 0);
    IF (IIclear = 'y') THEN
        MODE 'fill';        /* display default values + clear simple fields */
        SET_FORMS FORM (MODE = 'update');   /* keep cursor off Query-only flds*/
##      GENERATE SET_DEFAULT_VALUES SIMPLE_FIELDS  -- defaults set in Visual Qry
    ENDIF;

##  IF $master_seqfld_exists THEN
##  IF $master_seqfld_used THEN
    IIchar1 = PROMPT 'Do you wish to Insert another (y/n)? '; 
    IF (UPPERCASE(:IIchar1) != 'Y') THEN
##
##      GENERATE USER_ESCAPE FORM_END
##
	SET_FORMS FRS (TIMEOUT = IItimeout); /* Restore saved timeout value */
        RETURN $default_return_value;
    ENDIF;

##  INCLUDE inseqfld.tf
    COMMIT WORK;        /* success above incrementing sequenced field */
##  ENDIF       -- master_seqfld_used
##  ENDIF       -- master_seqfld_exists
END

'Clear' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Clear all fields'),
        KEY FRSKEY16 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
    IIchar1 = 'Y';

    INQUIRE_FORMS FORM (IIint = CHANGE);
    IF (IIint = 1) THEN
        /* User typed data on form. Confirm before CLEAR. */
        IIchar1 = PROMPT 'Do you wish to clear the form (y/n)?';
    ENDIF;

    IF (UPPERCASE(:IIchar1) = 'Y') THEN
##
##      IF $master_seqfld_exists THEN
##      IF $master_seqfld_displayed THEN
        IIint = $master_seqfld_fname;   /* Save sequenced field value */
##      ENDIF       -- master_seqfld_displayed
##      ENDIF       -- master_seqfld_exists
##
        MODE 'fill';        /* display default values + clear simple fields */
        SET_FORMS FORM (MODE = 'update');   /* keep cursor off Query-only flds*/
##      GENERATE SET_DEFAULT_VALUES SIMPLE_FIELDS  -- defaults set in Visual Qry
##
##      IF $master_seqfld_exists THEN
##      IF $master_seqfld_displayed THEN
        $master_seqfld_fname = IFNULL(IIint, 0);  /*Restore saved seqfld value*/
##      ENDIF       -- master_seqfld_displayed
##      ENDIF       -- master_seqfld_exists
##
        SET_FORMS FORM (CHANGE = 0);
    ENDIF;
END
##
##INCLUDE inlookup.tf           -- ListChoices menuitem

'Help' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Display help for this frame'),
        KEY FRSKEY1 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
##  GENERATE HELP fgmsappp.hlp
END
##
##IF $default_start_frame THEN
##ELSE

'TopFrame' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Return to top frame in application'),
        KEY FRSKEY17 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
##  INCLUDE inend.tf    -- check if want to save changes before end.
    IIretval = 2;       /* return to top frame */
##  GENERATE USER_ESCAPE FORM_END

    SET_FORMS FRS (TIMEOUT = IItimeout); /* Restore saved timeout value */
    RETURN $default_return_value;
END
##ENDIF -- $default_start_frame

'End' (VALIDATE = 0, ACTIVATE = 0,
        EXPLANATION = 'Return to previous frame'),
        KEY FRSKEY3 (VALIDATE = 0, ACTIVATE = 0) =
BEGIN
##  INCLUDE inend.tf    -- check if want to save changes before leave.
##  GENERATE USER_ESCAPE FORM_END

    SET_FORMS FRS (TIMEOUT = IItimeout); /* Restore saved timeout value */
    RETURN $default_return_value;
END
##
##GENERATE USER_ESCAPE BEFORE_FIELD_ACTIVATES
##GENERATE USER_ESCAPE AFTER_FIELD_ACTIVATES  -- Field-Change & Field-Exit

##GENERATE LOCAL_PROCEDURES
