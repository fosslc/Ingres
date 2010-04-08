$!
$!  CKP_TO_TAPE.COM -	Script to copy a database directory to tape for
$!			a checkpoint.
$!			This script does the backup without a /rewind so that
$!			more than one location can go on the same tape.
$!			
$!	p1 = total number of locations
$!	p2 = disk directory to be backed up
$!	p3 = device name given for tape backup (-m flag on CKPDB command line)
$!	p4 = name the checkpoint file should get when put onto the tape.
$!	p5 = list of tables if partial checkpoint
$!	p6 = current location number
$!
$!  ===========================================================================
$!
$ alloc 'p3'
$ backup/verify/ignore=label_processing/exclude=(pppp*.t*,*.m*,*.d*,*.sm*)-
    'p2''p5' 'p3''p4'
$ dismount/nounload 'p3'
