##      -- template file: inseqfld.tf   (Generate new sequenced field value)
##
/* table has sequenced field. increment it */
$master_seqfld_fname := CALLPROC sequence_value 
    (table_name  = '$master_table_name', 
     column_name = '$master_seqfld_cname', 
     increment   = 1,
     start_value = 1
    );
INQUIRE_SQL (IIerrorno = ERRORNO);
IF ($master_seqfld_fname <= 0) OR (IIerrorno != 0) THEN
    ROLLBACK WORK;
    CALLPROC beep();    /* 4gl built-in procedure */
    MESSAGE 'An error occurred while generating a new sequence key value' 
          + ' for table "$master_table_name". Details about the error were'
          + ' described by the message immediately preceding this one.'
          + ' If no error message preceded this one, then check the'
          + ' reference manual for instructions on what to do when an error'
          + ' occurs generating a new sequence value.'
          + ' Do not try to Save further'
          + ' work in this frame until you resolve this problem.'
          WITH STYLE = POPUP;
    RESUME;
ENDIF;
