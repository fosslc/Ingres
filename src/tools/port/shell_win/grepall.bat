@echo off 
REM 
REM grepall - recursive grep
REM
REM History :
REM
REM 	24-jun-95 (tutto01)
REM 	    Created.


@find . -name '*' -exec grepall2 %1 "{}" ;
