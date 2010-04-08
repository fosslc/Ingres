$!
$!  MSFC_SLV.COM  - Driver for multi-server fast commit slave
$!
$!  Runs fast commit test
$!  First waits for fast commit driver to be sync up through the
$!	msfc_sync program.
$!
$!  Inputs:
$!	test_num - fast commit test to run.
$!	session  - session number for this test (multiple test suites may be
$!		   running concurrently.
$!	base_num - fast commit driver identifier.
$!	sync_db	 - database to use for syncing with driver.
$!	dbname	 - database for fast commit test.
$!	server	 - server to use.
$!
$!      History:
$!
$!      ??-???-????     Unknown
$!              Created Multi-Server Fast Commit test.
$!      21-Nov-1991     Jeromef
$!              Add msfc setup to piccolo library
$!
$
$ test_num = p1
$ session = p2
$ base_num = p3
$ sync_db = p4
$ dbname = p5
$ server = p6
$
$ define/proc ii_dbms_server "''server'"
$ msfc_test :== $ing_src:[bin]msfctest.exe
$ msfc_sync :== $ing_src:[bin]msfcsync.exe
$
$ msfc_sync 'sync_db' 'base_num'
$ msfc_test 'dbname' 'test_num' 'base_num' 'session'
$ exit
