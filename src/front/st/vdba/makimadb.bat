@echo off
REM
REM Create and populate the Ingres Management Architecture repository.
REM
## History:
##
##      24-feb-1999 (mcgem01)
##          Added.

createdb -u$ingres imadb
call sql -u$ingres imadb <makiman.sql
