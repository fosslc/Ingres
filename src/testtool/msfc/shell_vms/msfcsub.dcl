$!
$!  MSFC_SUB	      - submit, the actual command procedure to execute
$!			the msfc test, to the batch queue.
$!
$!  Inputs:
$!              none
$!
$!      History:
$!
$!      ??-???-????     Unknown
$!              Created Multi-Server Fast Commit test.
$!      21-Nov-1991     Jeromef
$!              Add msfc submit to piccolo library
$!
$!
$!
$!
$! set verify
$!
$!	synch/entry=262/que=mts1$batch
$!	 wait 00:05:00
$
$ on error then goto SET_PRV
$	sho log ii_dbms_server
$	deassign/process ii_dbms_server
$!
$!
$SET_PRV:
$	set proc/priv=all
$	set def ing_tst:[be.msfc.dcl]
$	@ing_tst:[be.msfc.dcl]msfcset.com
$	on error then continue
$	destroydb msfcdb1
$	on error then continue
$	destroydb msfcdb2
$	on error then continue
$	on error then exit
$	createdb msfcdb1
$	createdb msfcdb2
$! set noverify
$!
$!	run the msfc test
$!
$	@ing_tst:[be.msfc.dcl]msfcrun.com 'p1'
$!
$ exit
