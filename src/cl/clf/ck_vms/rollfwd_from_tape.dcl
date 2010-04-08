$!
$!  ROLLFWD_FROM_TAPE.COM -	Script to reload a database directory from a
$!				tape checkpoint.
$!  
$!	p1 = total number of locations
$!	p2 = disk directory to be reloaded
$!	p3 = device name given for tape (-m flag on ROLLFORWARDDB command line)
$!	p4 = name the checkpoint file has on the tape.
$!	p5 = list of tables if partial checkpoint
$!	p6 = current location number
$!  ===========================================================================
$!
$ alloc 'p3'
$ backup/verify/ignore=label_processing  'p3''p4'/save/select=('p5') 'p2'/new_ver/owner=orig
$ dismount/nounload 'p3'
