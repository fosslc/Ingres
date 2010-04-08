$!
$!  ROLLFWD_FROM_DISK.COM -	Script to reload a database directory from a
$!				disk checkpoint.
$!  
$!	p1 = full pathname of saveset file
$!	p2 = disk directory to be reloaded
$!      p3 = current location number
$!
$!  ===========================================================================
$!
$ backup 'p1'/save/select=('p3') 'p2'/new_ver/owner=orig
