@echo off
REM This script pipes input to NETU to show, add, or delete 
REM user or node authorizations. 
REM
REM 	$1 = s(how), a(dd) or d(elete)
REM 	$2 = p(rivate) or g(lobal)
REM 	$3 = n(node) or u(user)
REM
REM If $1 is 'a' (add) or 'd' (delete), the next arg is required:
REM
REM 	$4 = v-node name
REM
REM When $3 is 'n' (node), these args also apply:
REM
REM	$5 = software type  (defaults to 'tcp_ip')
REM	$6 = node name      (defaults to v-node name)
REM	$7 = listen address (defaults to II)
REM
REM When $3 is 'u' (user), these args also apply:
REM
REM	$5 = user name (required unless $1 is 's' (show) )
REM	$6 = user pw   (defaults to user name)
REM	
REM
REM Examples:
REM
REM   To show global user entries:
REM
REM	authnetu s g u
REM
REM   To add v-node 'hydra' with defaults for software, node, and listen
REM   address:
REM 
REM	authnetu a g n hydra
REM
REM   To add v-node 'hydraXX' for node 'hydra' with listen address 'XX':
REM
REM	authnetu a g n hydraXX tcp_ip hydra XX
REM
REM	
REM History:
REM
REM	22-sep-1995 (somsa01)
REM		Created from authnetu.sh .
REM	02-Feb-1996 (somsa01)
REM		Replaced "c:\tmp" with "ingprenv II_TEMPORARY".
REM
REM 	03-Apr-1997 (clate01)
REM		Shortened label names, in order to run successfully
REM		on Windows'95, label names longer than 8 chars are
REM		truncated, which caused problems
REM

setlocal
set netu_escape=

call ipset tmpdir ingprenv II_TEMPORARY
set junk_out=%tmpdir%\junk$$.out

if not "%1"=="a" if not "%1"=="d" if not "%1"=="s" goto BAD
  set action=%1
  goto CONTINU
:BAD
  echo First arg must be 'a' (add), 'd' (delete), or 's' (show)
  goto DONE

:CONTINU
if not "%2"=="g" if not "%2"=="p" goto BAD2
  set auth_type=%2
  goto CONTINU2 
:BAD2
  echo Second arg must be 'g' or 'p' (global or private)
  goto DONE

:CONTINU2
if not "%action%"=="s" goto ELSE
  if "%4"=="" set vnode=*
  if not "%4"=="" set vnode=%4
  goto CONTINU3
:ELSE
REM ${4?"must be set to vnode name"}
  set vnode=%4

:CONTINU3
if not "%3"=="n" goto NEXT
   set what=n 

   if not "%action%"=="a" goto ELSE2
    	if not "%5"=="" set software=%5
    	if "%5"=="" set software=tcp_ip
	if not "%6"=="" set node=%6
	if "%6"=="" set node=%vnode%
      	if not "%7"=="" set listen=%7
      	if "%7"=="" set listen=II
	goto CONTINU4
:ELSE2
      	if not "%5"=="" set software=%5
      	if "%5"=="" set software=*
	if not "%6"=="" set node=%6
	if "%6"=="" set node=%vnode%
      	if not "%7"=="" set listen=%7
      	if "%7"=="" set listen=*
	goto CONTINU4

:NEXT
if not "%3"=="u" goto NEXT2
   set what=u 

   if not "%action%"=="s" goto ELSE3
	if not "%5"=="" set user=%5
	if "%5"=="" set user=*
	goto NEXTIF
:ELSE3
REM ${5?"5th arg must be username"}
      	set user=%5

:NEXTIF
   if "%action%"=="a" set pw=%6
REM ${6?"6th arg must be password"}
   goto CONTINU4

:NEXT2
   echo Third arg must be 'n' (node) or 'u' (user)
   goto DONE
   
:CONTINU4
if not "%action%%what%"=="su" goto NEXT3

   netu >%junk_out% <<EOF_sa
a
s
%auth_type%
*
*
%netu_escape%
e
EOF_sa

   echo.
   sed -e "/V_NODE/,/^$/p" -n %junk_out%

   goto DONE1

:NEXT3
if not "%action%%what%"=="au" goto NEXT4
   echo Using NETU to add %auth_type% user %user% to v-node %vnode%

   netu >%junk_out% <<EOF_aa
a
a
%auth_type%
%vnode%
%user%
%pw%
%pw%
%netu_escape%
EOF_aa

   grep "Server Registry" %junk_out%

   goto DONE1
 
:NEXT4
if not "%action%%what%"=="du" goto NEXT5
   echo Using NETU to delete %auth_type% user %user% from v-node %vnode%

   netu >%junk_out% <<EOF_da
a
d
%auth_type%
%vnode%
%user%
%netu_escape%
EOF_da

   grep "Server Registry" %junk_out%

   goto DONE1

:NEXT5
if not "%action%%what%"=="an" goto NEXT6
   echo Using NETU to add vnode %vnode% for node %node% (with software %software% and listen address %listen%)

   netu >%junk_out% <<EOF_an
n
a
%auth_type%
%vnode%
%software%
%node%
%listen%
EOF_an

   grep "Server Registry" %junk_out%

   goto DONE1

:NEXT6
if not "%action%%what%"=="sn" goto NEXT7
   netu >%junk_out% <<EOF_sn
n
s
%auth_type%
%vnode%
%software%
%node%
%listen%
EOF_sn

   echo.
   sed -e "/V_NODE/,/^$/p" -n %junk_out%

   goto DONE1

:NEXT7
if not "%action%%what%"=="dn" goto DONE1
   echo Using NETU to delete vnode %vnode% (node %node%)

   netu >%junk_out% <<EOF_dn
n
d
%auth_type%
%vnode%
%software%
%node%
%listen%
EOF_dn

   grep "Server Registry" %junk_out%


:DONE1
del %junk_out%
 
:DONE
endlocal
