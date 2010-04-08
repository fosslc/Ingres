$!------------------------------------------------------------------------
$!
$!  Copyright (c) 1993, 2005 Ingres Corporation
$!
$!  Name: iircpseg -- returns output of iishowres to the configuration
$!	rules system.
$!
$!  Usage:
$!      @iircpseg out_file
$!
$!  Description:
$!	Send output of iishowres to out_file.
$!
$!! History:
$!!	18-apr-2005 (abbjo03)
$!!	    Correct name in comments above, at QA's request.
$!
$!------------------------------------------------------------------------
$!
$ iishowres := $ii_system:[ingres.bin]iishowres.exe
$ if "" .nes. p1 then -
$    define sys$output 'p1
$
$ iishowres
$
$ if "" .nes. p1 then -
$    deassign sys$output
