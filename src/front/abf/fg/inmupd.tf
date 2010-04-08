## -- inmupd.tf
##
##IF $nullable_master_key THEN
##
##-- If table has nullable key, then can't simply generate a WHERE
##-- clause with "keycolumn = :keyfield_name", because ":keyfield_name" may
##-- contain a NULL value, and "keycolumn = NULL" never matches anything!
##-- Must generate a more complicated WHERE clause for such columns and
##-- use Flag variables to indicate when "keyfield_name" contains NULL.
##
##GENERATE SET_NULL_KEY_FLAGS MASTER

##ENDIF
##
##GENERATE QUERY UPDATE MASTER REPEATED

##INCLUDE inmsuerr.tf		-- Error handling, including deadlock
