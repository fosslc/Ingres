@ECHO OFF
REM
REM History:
REM
REM	27-Jan-2005 (drivi01)
REM		Submitted thinclient setup file to piccolo.
REM 	15-Jun-2005 (fanra01)
REM     	Bug 114685
REM     	Change registry values in config.dat from peer to slave.
REM     	This should ensure that an existing instance of Ingres is always the
REM     	master.
REM 	16-Sep-2005 (drivi01)
REM		Updated thinclient to set ingstop parameter in 
REM		config.dat to -force.


IF .%3 == . ECHO syntax setclient installationid hostname username
IF .%3 == . goto end

md "%II_SYSTEM%\ingres\temp"
set II_TEMPORARY=%II_SYSTEM%\ingres\temp
set PATH=%II_SYSTEM%\ingres\bin;%II_SYSTEM%\ingres\utility;%PATH%

if "%TMP_II_LANGUAGE%" == ""  set TMP_II_LANGUAGE=ENGLISH
if "%TMP_II_TIMEZONE_NAME%" == ""  set TMP_II_TIMEZONE_NAME=NA-EASTERN
if "%TMP_TERM_INGRES%" == ""  set TMP_TERM_INGRES=IBMPC
if "%TMP_II_CHARSETXX%" == "" set TMP_II_CHARSETXX=WIN1252
if "%TMP_II_DATE_FORMAT%" == "" set TMP_II_DATE_FORMAT=US
if "%TMP_MONEY_FORMAT%" == "" set TMP_MONEY_FORMAT=L:$
set TMP_II_INSTALLATION=%1
SET TMP_II_GCNxx_LCL_VNODE=%2

ingsetenv II_LANGUAGE %TMP_II_LANGUAGE%
ingsetenv II_TIMEZONE_NAME %TMP_II_TIMEZONE_NAME%
ingsetenv TERM_INGRES %TMP_TERM_INGRES%
ingsetenv II_INSTALLATION %TMP_II_INSTALLATION%
ingsetenv II_CHARSET%TMP_II_INSTALLATION% %TMP_II_CHARSETXX%
ingsetenv II_DATE_FORMAT %TMP_II_DATE_FORMAT%
ingsetenv II_MONEY_FORMAT %TMP_MONEY_FORMAT%
ingsetenv II_TEMPORARY "%II_TEMPORARY%"
REM now remove II_TEMPORARY (since it is now in symbol table)
set II_TEMPORARY=

ingsetenv II_CONFIG "%II_SYSTEM%\ingres\files"
ingsetenv II_GCN%TMP_II_INSTALLATION%_LCL_VNODE %TMP_II_GCNxx_LCL_VNODE%

REM create empty config.dat
echo 2>"%II_SYSTEM%\ingres\files\config.dat"

REM generate config.dat
iigenres %2 "%II_SYSTEM%\ingres\files\tng.rfm"

REM set privileges to current user for starting Ingres
iisetres ii.%2.privileges.user.Administrator SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED
iisetres ii.%2.privileges.user.ingres SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED
iisetres ii.%2.privileges.user.%3% SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED

REM set gcc protocol parameters
iisetres ii.%2.gcc.*.wintcp.status ON
iisetres ii.%2.gcc.*.wintcp.port %TMP_II_INSTALLATION%
iisetres ii.%2.gcc.*.tcp_ip.status ON
iisetres ii.%2.gcc.*.tcp_ip.port %TMP_II_INSTALLATION%

iisetres ii.%2.gcc.*.lanman.status OFF
iisetres ii.%2.gcc.*.lanman.port %TMP_II_INSTALLATION%

iisetres ii.%2.gcc.*.nvlspx.status OFF
iisetres ii.%2.gcc.*.nvlspx.port %TMP_II_INSTALLATION%

iisetres ii.%2.gcc.*.decnet.status OFF
iisetres ii.%2.gcc.*.decnet.port %TMP_II_INSTALLATION%

REM set registry entries for discovery feature
iisetres ii.%2.gcn.registry_type slave
iisetres ii.%2.gcc.*.registry_type slave
REM protect derived parameters (derived from ii.(host).registry.wintcp.port)
REM iisetres +p ii.%2.gcc.*.lanman.port
REM iisetres +p ii.%2.gcc.*.nvlspx.port
REM iisetres +p ii.%2.gcc.*.wintcp.port
REM iisetres +p ii.%2.registry.lanman.port
REM iisetres +p ii.%2.registry.nvlspx.port
iisetres ii.%2.registry.tcp_ip.port 16903
iisetres ii.%2.registry.tcp_ip.status ON
REM release protection of derived parameters
REM iisetres -p ii.%2.gcc.*.lanman.port
REM iisetres -p ii.%2.gcc.*.nvlspx.port
REM iisetres -p ii.%2.gcc.*.wintcp.port
REM iisetres -p ii.%2.registry.lanman.port
REM iisetres -p ii.%2.registry.nvlspx.port

iisetres ii.%2.ingstart.*.gcd 0
iisetres ii.%2.prefs.ingstop -force

:end
rem end
