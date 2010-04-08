@echo off 
REM 
REM ts - recursive grep which only prints filenames
REM
REM History :
REM
REM 	24-jun-95 (tutto01)
REM 	    Created.


@find . -name '*' -exec grep -l "%1" "{}" ;
