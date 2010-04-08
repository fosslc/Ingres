$! Copyright (c) 1993-2002  Computer Associates International, Inc.
$!
$! Name:        rpserver.com - run Replicator Server
$!
$! Description:
$!      repserv.exe gets run with the qualifiers specified in
$!      ii_system:[ingres.bin]rpserver.com
$!
$!! History:
$!!      28-may-98 (kinte01)
$!!              Created based on dd_server.com in the replicator library
$!!	08-nov-2002 (abbjo03) (orig 11-dec-97 and 26-jun-98)
$!!		Replace RepServer executable name and location. If DD_RSERVERS
$!!		is not defined, use default.

$ on error   then exit $status
$ on warning then exit $status
$!
$ prcnam = f$edit(f$getjpi("", "prcnam"), "lowercase")  ! process name
$ snum = f$element(2, "_", prcnam)                      ! server number
$ rservers = f$trnlnm("DD_RSERVERS")
$ if "''rservers'" .eqs. "" then rservers = "II_SYSTEM:[ingres.rep]"
$ srvdir = rservers - "]" + ".servers.server''snum']"	! server directory
$!
$ set default 'srvdir'
$ run II_SYSTEM:[ingres.bin]repserv.exe
$ exit $status
