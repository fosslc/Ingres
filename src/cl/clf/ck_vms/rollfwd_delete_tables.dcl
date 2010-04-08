$!
$!  ROLLFWD_DELETE_TABLES.COM -	Script to delete physical file for a table in 
$!                              the specified location
$!  
$!	p1 = full pathname of directory used for the location
$!	p2 = physical file to be deleted
$!	p3 = table to be deleted
$!
$!##  25-jun-2003 (horda03)
$!##      Ensure that the value of P1 is suitable for SET DEF
$!  ===========================================================================
$!
$ write sys$output "deleting table ", p3, " in database location ", p1
$
$ use_dir = P1 - "]["
$ SET DEF 'use_dir'
$
$ DELETE/NOLOG 'P2';*
