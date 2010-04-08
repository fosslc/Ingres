$! MTS.COM
$! This script spawns the coordinator and slaves processes.
$!
$! Parameters:
$!	p1 = database name
$! 	p2 = test number - used for the name for the result files
$! 	p3 = number of slaves
$! 	p4 = traceflag
$!
$! Output:
$!	- MTS*.RES are the results file used in SEP canon comparision
$!
$!! History:
$!!
$!! 09/25/92 erniec
$!!	Modify such that mtsslave and mtscoord processes are spawned
$!!	instead of submitted. This does away with the mtscoord.com and
$!!	mtsslave.com scripts.  
$!!
$!!	The mtsslave and mtscoord result files now diff with the
$!!	SEP test canons, so we no longer need commands to parse the result 
$!!	files of VMS-type info before we can diff them.
$!!
$!!	synchronization with the spawned subprocesses is now made
$!!	by checking the subprocess count - f$getjpi("","prccnt")
$!!
$!!	NOTE: the user resource "BYTLM" was increased from 50000 to 250000
$!!	      to allow 30 slaves to execute
$!!
$!---------------------------------------------------------------
$! Check for a valid number of arguements
$!
$ if (p1 .nes. "") .and. (p2 .nes. "") .and. (p3 .nes. "") -
			.and. (p4 .nes. "") then $goto ARGSOK
$ 	write sys$output-
        "Usage: mts <database> <cc_number> <no_of_kids> <traceflag>"
$ 	exit
$ ARGSOK:
$!---------------------------------------------------------------
$! Submit coordinator and slave processes
$!
$ spawn/nowait/output=TST_OUTPUT:mts'p2'_cord.res -
  /process=mts'p2'_0 mtscoord  "''p1'" 'p2 'p3 'p4
$!
$ write sys$output " "
$ write sys$output "Started MTS coordinator"
$!
$ total_slaves = p3
$ count = 1
$ STARTLOOP:
$ spawn/nowait/output=TST_OUTPUT:mts'p2'_'count'.res -
  /process=mts'p2'_'count' mtsslave "''p1'" 'p2 'count 'p4
$!
$!
$ write sys$output " "
$ write sys$output "Started MTS slave number ''count'"
$!
$ count = count + 1
$ if (count .le. total_slaves) then $goto STARTLOOP
$!
$ write sys$output " "
$ write sys$output "All MTS slaves are running"
$!
$!----------------------------------------------------------------
$! Wait until all subprocesses created by this process complete execution
$!
$ write sys$output "Number of subprocesses:", f$getjpi("","prccnt")
$!
$ WAIT:
$ wait 00:00:30
$ if (f$getjpi("","prccnt") .gt. 0) then $goto WAIT
$!
$!
$ write sys$output " "
$ write sys$output "MTS coordinator and slaves are done"
$!
$ exit
