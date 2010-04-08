@REM copyhlp.bat - copies all .hlp files to the english directory.
@REM
@REM History:
@REM
@REM	04-oct-95 (tutto01)
@REM	    Created.
@REM	15-aug-96 (mcgem01)
@REM	    leave cp in place since it's more versatile than copy and 
@REM	    then change the read-only attrib on the help files once copied.
@REM     05-may-1998 (canor01)
@REM         Quote pathnames to support embedded spaces.
@REM

@echo.
@echo copyhlp locating help files -- please wait...
cp %ING_SRC%\*\*\*\*.hlp "%II_SYSTEM%\ingres\files\english"
attrib -r "%II_SYSTEM%\ingres\files\english\*.hlp"
