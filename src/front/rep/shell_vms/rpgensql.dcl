$! Copyright (c) 1993-1998  Computer Associates International, Inc.
$!
$! Name:	rpgensql - process generated SQL
$!
$! Description:
$!	Calls the SQL terminal monitor to process SQL script.
$!
$! Parameters:
$!	p1	- database owner
$!	p2	- nodename
$!	p3	- database name
$!	p4	- script name
$!
$!! History:
$!!	28-may098 (kinte01)
$!!		Created based on rpgensql.com in replicator library.

$ sql -u'p1' 'p2'::'p3' <'p4' >II_TEMPORARY:rpgen.log

