##  --  Main template file 'iimain.tf'. Processing always starts here.
##DEFINE $_start_template 'nosuch.tf'
##
##IF ('$frame_type' = 'Update') THEN
##    IF ($master_detail_frame = TRUE) THEN
##        -- master/detail update frame
##        DEFINE $_start_template 'mdupdate.tf'
##    ELSE
##        -- master-only update frame
##        IF ($master_in_tblfld = TRUE) THEN
##            DEFINE $_start_template 'mtupdate.tf'
##        ELSE
##            DEFINE $_start_template 'msupdate.tf'
##        ENDIF
##    ENDIF
##ENDIF
##
##IF ('$frame_type' = 'Append') THEN
##    IF ($master_detail_frame = TRUE) THEN
##        -- master/detail append frame
##        DEFINE $_start_template 'mdappend.tf'
##    ELSE
##        -- master-only append frame
##        IF ($master_in_tblfld = TRUE) THEN
##            DEFINE $_start_template 'mtappend.tf'
##        ELSE
##            DEFINE $_start_template 'msappend.tf'
##        ENDIF
##    ENDIF
##ENDIF
##
##IF ('$frame_type' = 'Browse') THEN
##    IF ($master_detail_frame = TRUE) THEN
##        -- master/detail browse frame
##        DEFINE $_start_template 'mdbrowse.tf'
##    ELSE
##        -- master-only browse frame
##        IF ($master_in_tblfld = TRUE) THEN
##            DEFINE $_start_template 'mtbrowse.tf'
##        ELSE
##            DEFINE $_start_template 'msbrowse.tf'
##        ENDIF
##    ENDIF
##ENDIF
##
##IF ('$frame_type' = 'Menu') THEN
##    DEFINE $_start_template 'ntmenu.tf'
##ENDIF
##
##INCLUDE $_start_template
