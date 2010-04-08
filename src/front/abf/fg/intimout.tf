##  -- intimout.tf: Generate ON-TIMEOUT escape code.
##
##IF $timeout_code_exists THEN

ON TIMEOUT =
BEGIN
##  GENERATE USER_ESCAPE ON_TIMEOUT
END
##ENDIF

