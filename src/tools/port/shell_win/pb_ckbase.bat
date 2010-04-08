@echo off
REM
REM Name: pb_ckbase.bat
REM
REM Description:
REM    Implement patch build command, pb_ckbase
REM
REM History:
REM    05-Nov-2002 (rigka01)
REM     created for purpose of patch creation procedures

echo pb_ckbase: Initializing ...
call pb_nt_setup.bat
REM switch to the build drive 
%ING__SRC_DRIVE%:
REM switch to the build directory
cd %ING_SRC%

REM check if piccolo is available
p here>pb_ckbase.p_here
grep "!" pb_ckbase.p_here >grep.out
if errorlevel 1 echo pb_ckbase: Error: piccolo is not available& goto DONE
echo pb_ckbase: Phase 1 of 2 ...
echo pb_ckbase: Finding appropriate have list for client 
set pb_date=%DATE: =% 
set pb_date=%pb_date:/=% 
set pb_date=%pb_date: =% 
set pb_time=
set pb_time=%TIME: =0% 
set pb_time=%pb_time::=%
set pb_time=%pb_time:.=%
set pb_time=%pb_time: =%
p have>pb_ckbase.have.%pb_time%.%pb_date%
echo pb_ckbase: Printing out label: %LABEL% 
p label -p %LABEL%>pb_ckbase.%LABEL%.label.dat
echo pb_ckbase: Comparing have list with mark %MARK% label
diff pb_ckbase.have.%pb_time%.%pb_date% pb_ckbase.%LABEL%.label.dat >pb_ckbase.have.%MARK%.diff
grep "!" pb_ckbase.have.%MARK%.diff >grep.out
if errorlevel 1 echo pb_ckbase: Client's have list and mark %MARK% match.& goto NEXT1
echo pb_ckbase: WARNING: Client's have list and mark %MARK% do not match. Check %ING_SRC%\pb_ckbase.have.%MARK%.diff.
:NEXT1
p working>pb_ckbase.working
grep "!" pb_ckbase.working >grep.out
if errorlevel 1 echo pb_ckbase: No working files exist for the client& goto NEXT2
echo pb_ckbase: WARNING: There are working piccolo files for this client.
:NEXT2
echo pb_ckbase: Phase 2 of 2 ...
echo pb_ckbase: Checking treasure (licensing) files 
cd treasure
echo pb_ckbase: Generating treasure have list for client 
p have>pb_ckbase.have.%pb_time%.%pb_date%
echo pb_ckbase: Printing out label: %TREASURELABEL% 
p label -p %TREASURELABEL%>pb_ckbase.%TREASURELABEL%.label.dat
echo pb_ckbase: Comparing treasure have list with mark %MARK% treasure label
diff pb_ckbase.have.%pb_time%.%pb_date%  pb_ckbase.%TREASURELABEL%.label.dat >pb_ckbase.have.%MARK%.treasure.diff
grep "!" pb_ckbase.have.%MARK%.treasure.diff >grep.out
if errorlevel 1 echo pb_ckbase: Client's treasure have list and mark %MARK% treasure label match.& goto NEXT3
echo pb_ckbase: WARNING: Client's treasure have list and mark %MARK% treasure do not match.  Check %ING_SRC%\treasure\pb_ckbase.have.%MARK%.treasure.diff.
:NEXT3
p working>pb_ckbase.working
grep "!" pb_ckbase.working >grep.out
if errorlevel 1 echo pb_ckbase: No working treasure files exist for the client& goto NEXT4
echo pb_ckbase: WARNING: There are working treasure piccolo files for this client.
:NEXT4
cd %ING_SRC%
echo pb_ckbase: Exit 
:DONE
