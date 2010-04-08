## -- intimset.tf: Set timeout period.
##
INQUIRE_FORMS FRS (IItimeout = TIMEOUT);         /* Get parent timeout */
## IF $timeout_code_exists THEN
IF (IItimeout = 0) THEN
    SET_FORMS FRS (TIMEOUT = $_timeout_seconds); /* Use system default */
ENDIF;
## ELSE
SET_FORMS FRS (TIMEOUT = 0);   /* No on-timeout escape code, clear timeout */
## ENDIF
