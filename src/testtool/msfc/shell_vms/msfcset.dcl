$!  MSFC_SETUP	- Define the envirnoment for the fast commit test.
$!
$!  Inputs:
$!		none
$!
$!      History:
$!
$!      ??-???-????     Unknown
$!              Created Multi-Server Fast Commit test.
$!      21-Nov-1991     Jeromef
$!              Add msfc setup to piccolo library
$!
$!
$!
$ def/job/nolog msfc_bin	ing_src:[bin]
$ def/job/nolog msfc_src	ing_tst:[be.msfc.src]
$ def/job/nolog msfc_data	ing_tst:[be.msfc.data]
$ def/job/nolog msfc_result	ing_tst:[output]
$ def/job/nolog msfc_canon	ing_tst:[be.msfc.canon]
$ def/job/nolog msfc_difs	ing_tst:[output]
$
$ def/group/nolog ii_dbms_log	msfc_result:iidbms.log
$
$ msfc_test	:== $msfc_bin:msfctest.exe
$ msfc_sync	:== $msfc_bin:msfcsync.exe
$ msfc_check	:== $msfc_bin:msfcchk.exe
$ msfc_driver	:== $msfc_bin:msfcdrv.exe
$ msfc_cleanup	:== $msfc_bin:msfcclup.exe
$ msfc_cleandb	:== $msfc_bin:msfccldb.exe
$ msfc_create	:== $msfc_bin:msfccrea.exe
$
$ msfc_slave :== @ing_tst:[be.msfc.dcl]msfcslv.com
$ start_server :== @ing_tst:[be.msfc.dcl]strsrv.com
