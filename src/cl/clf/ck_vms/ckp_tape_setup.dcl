$!
$!  CKP_TAPE_SETUP.COM -	Script to set up for a checkpoint to tape.
$!  
$!	p1 = device name given for tape backup (-m flag on CKPDB command line)
$!      p2 = total number of locations
$!      p3 = list of tables if partial checkpoint
$!      p4 = current location number
$!
$!  ===========================================================================
$!
$ if (p3 .nes. "") 
$ then
$ write sys$output "PARTIAL: beginning checkpoint to tape ''p1' of ''p2' locations" 
$ else
$ write sys$output "beginning checkpoint to tape ''p1' of ''p2' locations"
$ endif
$ alloc 'p1'
$ initialize 'p1' ingres
$ dealloc 'p1'
