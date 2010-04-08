## -- indupd.tf         UNLOADTABLE loops to save updates to detail data.
##
##IF $delete_detail_allowed THEN
/* Process the deleted rows first.
** If we process deleted rows last, which is the order
** that Unloadtable delivers them in, then we will lose data 
** if a row with an identical key is deleted and then inserted 
** into the table field before the user selects Save.
** Process Deleted rows only:
*/
UNLOADTABLE $tblfld_name
        (IIrowstate = _STATE, IIrownumber = _RECORD)
BEGIN

    IF (IIrowstate = 4) THEN        /* deleted */
        /* delete row using hidden field keys in where clause. */
##
##      IF $nullable_detail_key THEN
##--
##--    If table has nullable key, then can't simply generate a WHERE
##--    clause with "keycolumn = :keyfield_name", because ":keyfield_name" may
##--    contain a NULL value, and "keycolumn = NULL" never matches anything!
##--    Must generate a more complicated WHERE clause for such columns and
##--    use Flag variables to indicate when "keyfield_name" contains NULL.

##      GENERATE SET_NULL_KEY_FLAGS DETAIL
##      ENDIF

##      GENERATE QUERY DELETE DETAIL REPEATED

##  INCLUDE induerr.tf		-- Error handling, including deadlock
    ENDIF;

END;        /* end of first unloadtable */
##ENDIF -- $delete_detail_allowed

/* process all but Deleted rows */
UNLOADTABLE $tblfld_name
        (IIrowstate = _STATE, IIrownumber = _RECORD)
BEGIN
    IF (IIrowstate = 1) THEN        /* new */

##      GENERATE QUERY INSERT DETAIL REPEATED 
##      INCLUDE induerr.tf		-- Error handling, including deadlock

    ELSEIF (IIrowstate = 3)                         /* changed */
##      IF $update_cascades THEN
        OR (IIrowstate = 2 AND IIupdrules > 0) THEN
        /* Changed row, OR Unchanged row, but join
        ** field(s) changed and Update Cascades.
        */
##      ELSE
        THEN
##      ENDIF  -- $update_cascades 
        /*
        ** Update row using hidden version of key fields
        ** in where clause.
        */
##
##      IF $nullable_detail_key THEN
##--
##--    If table has nullable key, then can't simply generate a WHERE
##--    clause with "keycolumn = :keyfield_name", because ":keyfield_name" may
##--    contain a NULL value, and "keycolumn = NULL" never matches anything!
##--    Must generate a more complicated WHERE clause for such columns and
##--    use Flag variables to indicate when "keyfield_name" contains NULL.

##      GENERATE SET_NULL_KEY_FLAGS DETAIL
##      ENDIF

##      GENERATE QUERY UPDATE DETAIL REPEATED 
##      INCLUDE induerr.tf		-- Error handling, including deadlock
    ENDIF;

END;        /* end of second unloadtable */
