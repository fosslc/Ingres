$!
$!  CKP_TO_DISK.COM -	Script to copy a database directory to disk for
$!			a checkpoint.
$!  
$!	p1 = database directory to be copied
$!	p2 = full pathname of checkpoint file to be written
$!	p3 = list of tables if partial checkpoint
$!	p4 = current location number
$!
$!  ===========================================================================
$!
$ backup/exclude=(pppp*.t*,*.m*,*.d*,*.sm*) 'p1''p3'-
    'p2'/save/prot=(s:rwed,o:rwed,g,w)/nocrc/group=0
