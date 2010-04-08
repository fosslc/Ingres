@echo off
echo local iimscw, with same gt as makefile
if x%1==x echo Useage: 1) To compile xxx.c, usage is %0 xxx
if x%1==x echo -       2) Creates xxx.err and xxx.obj
if x%1==x echo -       3) Do not use full-paths or you will exceed 127 chars in the cmd line!
if x%1==x goto dun
if not exist %1.c echo Cannot find file %1.c
if not exist %1.c goto dun
REM The next line may not exceed 121 characters in length!
cl /c /W3 /Alfw /G2w /Zp1i /FPi /Od /Gt6 /Dstdin=((FILE*)0) /Dstdout=((FILE*)0) /Dexit=IIxit /Dmain=main_ %1.c>ii.err
rem cl /c /W3 /Alfw /G2w /Zp1i /FPi /Od /Gt6 /DDEBUGSQL /Dstdin=((FILE*)0) /Dstdout=((FILE*)0) /Dexit=IIxit /Dmain=main_ %1.c>ii.err
copy ii.err %1.err > nul
del  ii.err
type %1.err
:dun
