$!
$!  ROLLFWD_DELETE_FILES.COM -	Script to delete all files in the specified
$!                              location
$!  
$!	p1 = full pathname of directory used for the location
$!
$!##  18-dec-1998 (chash01)
$!##      Remove the blank line at the end of the file.  It causes an error
$!##      %RMS-E-MTLBLLONG
$!##  25-jun-2003 (horda03)
$!##      Ensure that the value of P1 is suitable for SET DEF
$!##      Ensure that files exist to be deleted.
$!  ===========================================================================
$!
$ write sys$output "deleting files in location ''P1'"
$
$ use_dir = P1 - "]["
$ SET DEF 'use_dir'
$
$ IF F$SEARCH( "*.*" ) .NES. ""
$ THEN
$    DELETE/NOLOG *.*;*
$ ENDIF
