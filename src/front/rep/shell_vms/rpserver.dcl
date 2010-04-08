$! Copyright (c) 1993-2002  Computer Associates International, Inc.
$!
$! Name:	rpserver.com - run Replicator Server
$!
$! Description:
$!	Starts Replicator Server after switching to the server directory.
$!	p1 is the server number.
$!
$!! History:
$!!	28-may-98 (kinte01)
$!!		Created based on dd_server.com & dd_server1.com in 
$!!		replicator library.
$!!	13-Nov-98 (kinte01/rosga02)
$!!		To execute the rpserver command via SEP on VMS we have
$!!		create an alias referencing the run/detach command
$!!		and then use the alias to start the server
$!!	08-nov-2002 (abbjo03) (orig 26-jun-98)
$!!		If DD_RSERVERS is not defined, use default.
$!!	12-dec-2002 (abbjo03)
$!!	    Revert to use of symbol to run detached.
$!!
$ on error   then exit $status
$ on warning then exit $status
$!
$ snum = 'p1                                            ! server number
$ prcnam = "RP_SERVER_''snum'"                          ! process name
$ rservers = f$trnlnm("DD_RSERVERS")
$ if "''rservers'" .eqs. "" -
	then rservers = "II_SYSTEM:[ingres.rep]"
$ srvdir = rservers - "]" + ".servers.server''snum']"	! server directory
$!
$ rundet = "run /detached"
$ rundet /work = 1024 -
        /proce = 'prcnam' -
        /input = ii_system:[ingres.bin]rpsrvr.com -
        /output = 'srvdir'print.log -
        /prior = 4 -
        /page = 10000 -
        /buff = 56384 -
        /file = 115 -
        /job = 4096 -
	sys$system:loginout.exe
$ exit 1
