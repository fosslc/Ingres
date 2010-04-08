$!
$!  ROLLFWD_TAPE_SETUP.COM -	Script to set up for a recovery from tape.
$!  
$!	p1 = device name given for tape backup (-m flag on CKPDB command line)
$!      p2 = total number of locations
$!      p3 = list of tables if partial
$!      p4 = current location number
$!
$!  ===========================================================================
$!
$ if (p3 .nes. "") 
$ then
$ write sys$output "PARTIAL: beginning restore of tape ''p1' of ''p2' locations" 
$ else
$ write sys$output "beginning restore of tape ''p1' of ''p2' locations"
$ endif
$ alloc 'p1'
$ mount/foreign 'p1'
$ set magtape/rewind 'p1'
$ dismount/nounload 'p1'
$ dealloc 'p1'
