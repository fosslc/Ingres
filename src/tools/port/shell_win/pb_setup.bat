@echo  off
REM
REM Name: pb_setup.bat
REM
REM Description:
REM    Implement patch build command, pb_setup 
REM Parameters:
REM    %1: SFP
REM    %2: $DEEDXID, %DEEDXID%, or value of %DEEDXID%
REM        This is pyyyy where yyyy is the patch number
REM        of the patch being created.
REM    %3: mxxxx where xxxx is mark number
REM
REM History:
REM    05-Nov-2002 (rigka01)
REM     created for purpose of patch creation procedures

echo pb_setup: Initializing ...
if not "SFP"=="%1" echo pbsetup: Error: Second parameter must be SFP& goto SYNTAX
call pb_nt_setup.bat
if "%DEEDXID%"=="%2" goto CONT 
if "%2"=="" echo pb_setup: Error: 2nd parameter required but not specified& goto SYNTAX 
if not "$DEEDXID"=="%2" echo pb_setup: Error: 2nd parameter, %2, does not match $DEEDXID nor %DEEDXID%& goto  SYNTAX
:CONT 
if "%3"=="" echo pb_setup: Error: 3rd parameter required but not specified& goto SYNTAX 
if not "m%MARK%"=="%3" echo pb_setup: Error: 3rd parameter, %3, does not match m%MARK%& goto SYNTAX 
echo pb_setup: Creating patch working directories
mkdir %PATCHTMP%\work\%DEEDXID%
mkdir %BLDROOT%\%DEEDXID%\collect
mkdir %BLDROOT%\%DEEDXID%\pack
mkdir %BLDROOT%\%DEEDXID%\%DEEDXID%
echo pb_setup: Copying unzipped vanilla mark %MARK% patch %MARKPATCH% to %II_RELEASE_DIR_PATH% 
REM first be sure the mark patch exists and is unzipped
dir %PATCHTMP%\m%MARK%\%MARKPATCH%\ingres\patch.txt  >markpatchexists.doc
if errorlevel 1 echo pb_setup.bat: Error: Vanilla mark patch %MARKPATCH% for mark %MARK% does not exist in %PATCHTMP%\m%MARK%\%MARKPATCH% or is not unzipped.& goto ERROR 
cp -fr %PATCHTMP%\m%MARK%\%MARKPATCH%\* %II_RELEASE_DIR_PATH% 
echo pb_setup: Transferring info from readme.dat to record.dat (Not implemented)
echo pb_setup: readme.dat: 
echo pb_setup: record.dat:
echo pb_setup: ...
echo pb_setup: Creating/replacing VERS file in front\st\patch:
set PATCHNO=%DEEDXID:p=%
echo IngVersion=II %VERS% (int.w32/00) > %ING_SRC%\front\st\patch\VERS
echo PATCHNO=%PATCHNO%>> %ING_SRC%\front\st\patch\VERS
echo FILES_CHANGED_BEGIN>> %ING_SRC%\front\st\patch\VERS
echo FILES_CHANGED_END>> %ING_SRC%\front\st\patch\VERS
cp %ING_SRC%\front\st\patch\VERS %ING_SRC%\front\st\patch\vers.%DEEDXID%
echo pb_setup: Ensuring defs/srclist/dirlists/makefiles etc are up-to-date:
cd %ING_SRC%\tools\port\defs
nmake -a >nmake.%DEEDXID%.log
cd %ING_SRC%\tools\port\srclist
nmake -a >nmake.%DEEDXID%.log
cd %ING_SRC%\tools\port\dirlist
nmake -a >nmake.%DEEDXID%.log
cd %ING_SRC%\tools\port\make
nmake -a >nmake.%DEEDXID%.log
cd %ING_SRC%
cd front\st\patch
echo pb_setup: use piccolo to open %PATCHDOCFILE%
p open %PATCHDOCFILE%
echo pb_setup: Successfully completed section 1
goto DONE
:SYNTAX
echo pb_setup: To invoke this command, issue: 
echo pb_setup:        pb_setup SFP $DEEDXID myyyy 
echo pb_setup:     where DEEDXID has been set and yyyy is the mark level
:ERROR 
echo pb_setup: Failed. See previous "pb_setup: Error:" messages
:DONE
