call %II_SYSTEM%\ingres\bin\ipset.bat II_ICE_DBUSER %II_SYSTEM%\ingres\utility\iigetres ii.%COMPUTERNAME%.ice.default_userid
if "%II_ICE_DBUSER%" == "" goto end
%II_SYSTEM%\ingres\utility\iisetres ii.%COMPUTERNAME%.icesvr.*.default_dbuser %II_ICE_DBUSER%
echo insert into ice_dbusers values( 2, '%II_ICE_DBUSER%', '%II_ICE_DBUSER%', '', 'ice 2.0 compatible');\p\g | sql icesvr >NUL 2>&1

:end
