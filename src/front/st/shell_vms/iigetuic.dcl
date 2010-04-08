$!------------------------------------------------------------------------
$!
$!  Copyright (c) 2004 Ingres Corporation
$!
$!  Name: iigetuic.com -- returns value of f$user() to the configuration
$!	rules system.
$!
$!  Usage:
$!      @iigetuic out_file [ node ]
$!
$!  Description:
$!	Writes the value returned by f$user() to the out_file.
$!
$!------------------------------------------------------------------------
$!
$ open /write outfile 'p1
$ write outfile f$user()
$ close outfile
