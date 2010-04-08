$!  Copyright (c) 2004 Ingres Corporation
$!
$!  Name: iiunloadfix -- replace system_maintained with sys_maintained
$!
$!  Usage:
$!	iiunloadfix
$!
$!  Description:
$!	Due to making system_maintained a reserved key word, the
$!	copy.in script produced from unloaddb will fail.  This
$!	script will updated the create table ii_atttype statement
$!	to create the correct column called sys_maintained.
$!
$!! History:
$!!	11-apr-2002 (kinte01)
$!!	    Created based on iiunloadfix.sh for Bug 103338
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!!
$!----------------------------------------------------------------------------
$
$ copy/log copy.in copy.in_bak
$ edit := edit/edt
$ edit copy.in
s/system_maintained/sys_maintained/w
exit
