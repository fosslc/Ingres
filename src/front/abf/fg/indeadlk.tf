## -- indeadlk.tf: Generate success or failure messages following deadlock
##
##INCLUDE intopdef.tf 
##
## DEFINE $_temp $_deadlock_retry+1
##
##IF ($master_detail_frame) THEN
IF (IImtries > $_deadlock_retry OR IIdtries > $_deadlock_retry) THEN
##ELSE
IF (IImtries > $_deadlock_retry) THEN
##ENDIF
    ROLLBACK WORK;
    MESSAGE 'Deadlock was detected during all $_temp'
        + ' attempts to save this data. The "Save" operation'
        + ' was not performed. Please correct the error and'
        + ' select "Save" again.'
        WITH STYLE = POPUP;
    RESUME;
##IF $master_detail_frame THEN
ELSEIF (IImtries > 0 OR IIdtries > 0) THEN
##ELSE
ELSEIF (IImtries > 0) THEN
##ENDIF
    /* Save succeeded on a reattempt following deadlock */
    MESSAGE 'Transaction completed successfully.'
        WITH STYLE = POPUP;
ENDIF;
