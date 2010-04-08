@echo off
REM
REM  where - search for string in wherelist
REM
REM  History:
REM
REM     14-jul-95 (tutto01)
REM         Created.
REM	17-nov-95 (tutto01)
REM	    Restored.
REM

grep -i %1 %ING_SRC%\wherelist
