@echo off
REM Name:   iceload.bat
REM
REM Description:
REM     Performs a bulk update of blobs contained in a table based upon the
REM     id value.
REM
REM
if "%1" == "" echo Misssing session group name&goto usage
if "%2" == "" echo Misssing table name&goto usage
if "%3" == "" echo Misssing blob column name&goto usage
if "%4" == "" echo Misssing database name&goto usage

goto %1

:icetool
blobstor -t %2 -b %3 -u -w id=1095  %4    "%II_SYSTEM%\ingres\ice\%1\nobranch.gif"
blobstor -t %2 -b %3 -u -w id=1096  %4    "%II_SYSTEM%\ingres\ice\%1\empty.gif"
blobstor -t %2 -b %3 -u -w id=1097  %4    "%II_SYSTEM%\ingres\ice\%1\icon_rpage.gif"
blobstor -t %2 -b %3 -u -w id=1098  %4    "%II_SYSTEM%\ingres\ice\%1\icon_page.gif"
blobstor -t %2 -b %3 -u -w id=1099  %4    "%II_SYSTEM%\ingres\ice\%1\icon_rmulti.gif"
blobstor -t %2 -b %3 -u -w id=1100  %4    "%II_SYSTEM%\ingres\ice\%1\icon_multi.gif"
blobstor -t %2 -b %3 -u -w id=1101  %4    "%II_SYSTEM%\ingres\ice\%1\lastopened.gif"
blobstor -t %2 -b %3 -u -w id=1102  %4    "%II_SYSTEM%\ingres\ice\%1\lastleaf.gif"
blobstor -t %2 -b %3 -u -w id=1103  %4    "%II_SYSTEM%\ingres\ice\%1\lastclosed.gif"
blobstor -t %2 -b %3 -u -w id=1104  %4    "%II_SYSTEM%\ingres\ice\%1\opened.gif"
blobstor -t %2 -b %3 -u -w id=1105  %4    "%II_SYSTEM%\ingres\ice\%1\leaf.gif"
blobstor -t %2 -b %3 -u -w id=1106  %4    "%II_SYSTEM%\ingres\ice\%1\closed.gif"
blobstor -t %2 -b %3 -u -w id=1107  %4    "%II_SYSTEM%\ingres\ice\%1\icon_units.gif"
blobstor -t %2 -b %3 -u -w id=1108  %4    "%II_SYSTEM%\ingres\ice\%1\icon_unit.gif"
blobstor -t %2 -b %3 -u -w id=1109  %4    "%II_SYSTEM%\ingres\ice\%1\icon_locs.gif"
blobstor -t %2 -b %3 -u -w id=1110  %4    "%II_SYSTEM%\ingres\ice\%1\icon_pages.gif"
blobstor -t %2 -b %3 -u -w id=1111  %4    "%II_SYSTEM%\ingres\ice\%1\icon_multis.gif"
blobstor -t %2 -b %3 -u -w id=1112  %4    "%II_SYSTEM%\ingres\ice\%1\icon_access.gif"
blobstor -t %2 -b %3 -u -w id=1113  %4    "%II_SYSTEM%\ingres\ice\%1\icon_loc.gif"
blobstor -t %2 -b %3 -u -w id=1114  %4    "%II_SYSTEM%\ingres\ice\%1\cancel.gif"
blobstor -t %2 -b %3 -u -w id=1115  %4    "%II_SYSTEM%\ingres\ice\%1\submit.gif"
blobstor -t %2 -b %3 -u -w id=1116  %4    "%II_SYSTEM%\ingres\ice\%1\menu.gif"
blobstor -t %2 -b %3 -u -w id=1117  %4    "%II_SYSTEM%\ingres\ice\%1\logout.gif"
blobstor -t %2 -b %3 -u -w id=1118  %4    "%II_SYSTEM%\ingres\ice\%1\iipe_anim.gif"
blobstor -t %2 -b %3 -u -w id=1119  %4    "%II_SYSTEM%\ingres\ice\%1\update.gif"
blobstor -t %2 -b %3 -u -w id=1120  %4    "%II_SYSTEM%\ingres\ice\%1\delete.gif"
blobstor -t %2 -b %3 -u -w id=1121  %4    "%II_SYSTEM%\ingres\ice\%1\view.gif"
blobstor -t %2 -b %3 -u -w id=1122  %4    "%II_SYSTEM%\ingres\ice\%1\code.gif"
blobstor -t %2 -b %3 -u -w id=1123  %4    "%II_SYSTEM%\ingres\ice\%1\access.gif"
blobstor -t %2 -b %3 -u -w id=1124  %4    "%II_SYSTEM%\ingres\ice\%1\download.gif"
blobstor -t %2 -b %3 -u -w id=1125  %4    "%II_SYSTEM%\ingres\ice\%1\server_anim.gif"
blobstor -t %2 -b %3 -u -w id=1126  %4    "%II_SYSTEM%\ingres\ice\%1\security_anim.gif"
blobstor -t %2 -b %3 -u -w id=1127  %4    "%II_SYSTEM%\ingres\ice\%1\icon_dbu.gif"
blobstor -t %2 -b %3 -u -w id=1128  %4    "%II_SYSTEM%\ingres\ice\%1\icon_role.gif"
blobstor -t %2 -b %3 -u -w id=1129  %4    "%II_SYSTEM%\ingres\ice\%1\icon_db.gif"
blobstor -t %2 -b %3 -u -w id=1130  %4    "%II_SYSTEM%\ingres\ice\%1\icon_user.gif"
blobstor -t %2 -b %3 -u -w id=1131  %4    "%II_SYSTEM%\ingres\ice\%1\icon_dbs.gif"
blobstor -t %2 -b %3 -u -w id=1132  %4    "%II_SYSTEM%\ingres\ice\%1\icon_roles.gif"
blobstor -t %2 -b %3 -u -w id=1133  %4    "%II_SYSTEM%\ingres\ice\%1\icon_profile.gif"
blobstor -t %2 -b %3 -u -w id=1134  %4    "%II_SYSTEM%\ingres\ice\%1\icon_dbus.gif"
blobstor -t %2 -b %3 -u -w id=1135  %4    "%II_SYSTEM%\ingres\ice\%1\icon_users.gif"
blobstor -t %2 -b %3 -u -w id=1136  %4    "%II_SYSTEM%\ingres\ice\%1\icon_profiles.gif"
blobstor -t %2 -b %3 -u -w id=1137  %4    "%II_SYSTEM%\ingres\ice\%1\icon_var.gif"
blobstor -t %2 -b %3 -u -w id=1138  %4    "%II_SYSTEM%\ingres\ice\%1\icon_vars.gif"
blobstor -t %2 -b %3 -u -w id=1139  %4    "%II_SYSTEM%\ingres\ice\%1\icon_monit.gif"
blobstor -t %2 -b %3 -u -w id=1140  %4    "%II_SYSTEM%\ingres\ice\%1\icon_act_sess.gif"
blobstor -t %2 -b %3 -u -w id=1141  %4    "%II_SYSTEM%\ingres\ice\%1\icon_cache.gif"
blobstor -t %2 -b %3 -u -w id=1142  %4    "%II_SYSTEM%\ingres\ice\%1\icon_trans.gif"
blobstor -t %2 -b %3 -u -w id=1143  %4    "%II_SYSTEM%\ingres\ice\%1\icon_curs.gif"
blobstor -t %2 -b %3 -u -w id=1144  %4    "%II_SYSTEM%\ingres\ice\%1\icon_copy.gif"
blobstor -t %2 -b %3 -u -w id=1145  %4    "%II_SYSTEM%\ingres\ice\%1\icon_apps.gif"
blobstor -t %2 -b %3 -u -w id=1146  %4    "%II_SYSTEM%\ingres\ice\%1\icon_app.gif"
goto end

:icetutor
blobstor -t %2 -b %3 -u -w id=2079  %4    "%II_SYSTEM%\ingres\ice\%1\empty.gif"
blobstor -t %2 -b %3 -u -w id=2080  %4    "%II_SYSTEM%\ingres\ice\%1\nobranch.gif"
blobstor -t %2 -b %3 -u -w id=2081  %4    "%II_SYSTEM%\ingres\ice\%1\lastopened.gif"
blobstor -t %2 -b %3 -u -w id=2082  %4    "%II_SYSTEM%\ingres\ice\%1\lastleaf.gif"
blobstor -t %2 -b %3 -u -w id=2083  %4    "%II_SYSTEM%\ingres\ice\%1\lastclosed.gif"
blobstor -t %2 -b %3 -u -w id=2084  %4    "%II_SYSTEM%\ingres\ice\%1\opened.gif"
blobstor -t %2 -b %3 -u -w id=2085  %4    "%II_SYSTEM%\ingres\ice\%1\leaf.gif"
blobstor -t %2 -b %3 -u -w id=2086  %4    "%II_SYSTEM%\ingres\ice\%1\closed.gif"
blobstor -t %2 -b %3 -u -w id=2087  %4    "%II_SYSTEM%\ingres\ice\%1\icon_page.gif"
blobstor -t %2 -b %3 -u -w id=2088  %4    "%II_SYSTEM%\ingres\ice\%1\icon_book.gif"
blobstor -t %2 -b %3 -u -w id=2089  %4    "%II_SYSTEM%\ingres\ice\%1\icon_chap.gif"
blobstor -t %2 -b %3 -u -w id=2090  %4    "%II_SYSTEM%\ingres\ice\%1\previous.gif"
blobstor -t %2 -b %3 -u -w id=2091  %4    "%II_SYSTEM%\ingres\ice\%1\next.gif"
blobstor -t %2 -b %3 -u -w id=2092  %4    "%II_SYSTEM%\ingres\ice\%1\logout.gif"
blobstor -t %2 -b %3 -u -w id=2093  %4    "%II_SYSTEM%\ingres\ice\%1\capBUCreate.gif"
blobstor -t %2 -b %3 -u -w id=2094  %4    "%II_SYSTEM%\ingres\ice\%1\capBUAccess.gif"
blobstor -t %2 -b %3 -u -w id=2095  %4    "%II_SYSTEM%\ingres\ice\%1\capBULoc.gif"
blobstor -t %2 -b %3 -u -w id=2096  %4    "%II_SYSTEM%\ingres\ice\%1\capDocCreate.gif"
blobstor -t %2 -b %3 -u -w id=2097  %4    "%II_SYSTEM%\ingres\ice\%1\capDocAccess.gif"
blobstor -t %2 -b %3 -u -w id=2098  %4    "%II_SYSTEM%\ingres\ice\%1\icon_multi.gif"
goto end

:plays
blobstor -t %2 -b %3 -u -w id=24    %4    "%II_SYSTEM%\ingres\ice\%1\OldGlobe.gif"
blobstor -t %2 -b %3 -u -w id=25    %4    "%II_SYSTEM%\ingres\ice\%1\tragedy.gif"
blobstor -t %2 -b %3 -u -w id=26    %4    "%II_SYSTEM%\ingres\ice\%1\history.gif"
blobstor -t %2 -b %3 -u -w id=27    %4    "%II_SYSTEM%\ingres\ice\%1\romance.gif"
blobstor -t %2 -b %3 -u -w id=28    %4    "%II_SYSTEM%\ingres\ice\%1\comedy.gif"
blobstor -t %2 -b %3 -u -w id=29    %4    "%II_SYSTEM%\ingres\ice\%1\bgpaper.gif"
blobstor -t %2 -b %3 -u -w id=30    %4    "%II_SYSTEM%\ingres\ice\%1\logout.gif"
blobstor -t %2 -b %3 -u -w id=31    %4    "%II_SYSTEM%\ingres\ice\%1\play_styleSheet.css"

goto end

:usage
echo Usage:
echo       %0 ^<Session Group^> ^<table name^> ^<blob column^> ^<database^>

:end
