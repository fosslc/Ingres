REM This copies and build the necessary files to support alternate
REM collation sequences.

copy %ING_SRC%\common\adf\adm\multi.dsc %II_SYSTEM%\ingres\files\collation\multi.dsc
aducompile %ING_SRC%\common\adf\adm\multi.dsc multi
copy %ING_SRC%\common\adf\adm\spanish.dsc %II_SYSTEM%\ingres\files\collation\spanish.dsc
aducompile %ING_SRC%\common\adf\adm\spanish.dsc spanish
