@echo off
REM --- Checks that there are no working files and produces a list of deletable
REM --- files.
REM History:
REM	15-May-1996 (fanra01)
REM	    Created.

pushd %ING_SRC%
echo Piccolo working files ...
@echo .
p working

@echo .
echo Files with read/write attribute ...
@echo .
dir /s /a-r-d /b back	>  purge2.tmp 2>&1
dir /s /a-r-d /b cl	>> purge2.tmp 2>&1
dir /s /a-r-d /b common	>> purge2.tmp 2>&1
dir /s /a-r-d /b dbutil	>> purge2.tmp 2>&1
dir /s /a-r-d /b front	>> purge2.tmp 2>&1
dir /s /a-r-d /b gl	>> purge2.tmp 2>&1
dir /s /a-r-d /b sig	>> purge2.tmp 2>&1
dir /s /a-r-d /b tools	>> purge2.tmp 2>&1
egrep -iv "\.obj|\.err|\.lib|\.pdb|\.ilk|\.vcp|\.exp|\.exe|\.dll\.map|\.yf|y\." purge2.tmp
del purge2.tmp > NUL 2>&1
popd
