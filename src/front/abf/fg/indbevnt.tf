## -- indbevnt.tf: Generate ON-DBEVENT escape code.
##
##IF $dbevent_code_exists THEN

ON DBEVENT =
BEGIN
##  GENERATE USER_ESCAPE ON_DBEVENT
END
##ENDIF
