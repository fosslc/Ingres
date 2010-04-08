@echo off
rem Copyright (c) 1993, 1997  Ingres Corporation
rem
rem Name:	rpgensql.bat - process generated SQL 
rem
rem Description:
rem	Calls the SQL terminal monitor to process SQL script.
rem
rem Parameters:
rem	1	- database owner
rem	2	- nodename
rem	3	- database name
rem	4	- script name
rem
rem ## History:
rem ##	13-jan-97 (joea)
rem ##		Created based on rpgensql.sh in replicator library.

ipset TMPDIR ingprenv II_TEMPORARY
sql -u%1 %2::%3 < %4  > %TMPDIR%\rpgen.log
