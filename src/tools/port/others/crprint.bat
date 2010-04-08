@echo off
REM
REM  crprint - prints first and last 25 pages of object code or executable
REM          - prints first 10 pages of source code
REM
REM  Arguments:
REM
REM    %1 - object or executable file
REM    %2 - source code file
REM
REM  Example:
REM
REM    crprint %ING_SRC%\back\bin\iidbms.exe %ING_SRC%\back\scf\scs\scsqncr.c
REM
REM  Output:
REM
REM    dmp1.lst - listing of object or executable file in dump format
REM    dmp2.lst - listing of source code file in source format
REM
REM    After running crprint, dmp1.lst and dmp2.lst are suitable for printing
REM    for copyright registration.
REM
REM  Arguments for the dmp command:
REM
REM    %1 - filename to be printed
REM
REM    %2 - type of print:
REM         1 - dump
REM         2 - source code
REM
REM    %3 - pages to print
REM
REM    %4 - lines per page
REM
REM    %5 - lines to skip when printing source code
REM
SET skip_lines=0
dmp %1 1 25 60 >dmp1.lst
dmp %2 2 10 60 %skip_lines% >dmp2.lst
