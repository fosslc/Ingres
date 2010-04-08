$! set verify
$!
$!  MSFC_RUN	    - run multi-server fast commit tests
$!
$!  Inputs:
$!		base_number   - number uniquely identifying this driver
$!			        allows multiple drivers to run at same time.
$!
$!	Starts up multiple fast commit servers on a common buffer manager
$!	and runs fast commit test suites on each server concurrently.
$!
$!	Multiple suites may be run concurrently on the same and/or different
$!	database.  All suites will use the same buffer manager.
$!
$!	Multiple drivers may be started up concurrently to test multiple
$!	buffer managers - each driver must specify a diffent base_number.
$!
$!	MSFC_DRIVER  base_number
$!
$!		base_number   - number uniquely identifying this driver
$!			        allows multiple drivers to run at same time.
$!
$!	Driver runs according to a definition file - msfc_XX.def, where XX is
$!	the base number given as an argument to this routine.
$!
$!	The definition file has the following format:
$!
$!		Server: startup options
$!		    Database: dbname session_count
$!		Server: startup options
$!		    Database: dbname session_count
$!		    Database: dbname session_count
$!
$!	There are two types of lines in the definition file:
$!
$!	    Lines beginning with "Server:" indicate to start a server with
$!	        specified start options. Following the "Server:" line should
$!		be one or more "Database:" lines indicating how many sessions
$!	        to run on that server.
$!	    Lines beginning with "Database:" indicate a database to run the
$!		fast commit test suite on.  The session_count parameter
$!		indicates the number of test suites to run on that database
$!		using the current server.
$!
$!	Each session will use its own set of tables to run the fast commit
$!	test suite.  All sessions will use a common buffer manager.
$!
$!	A driver program will be used to keep each session in sync.  Each
$!	session is run to the point where it is ready for a server crash,
$!	then all servers are brought down, recovery is started and all
$!	sessions go on to the next test.
$!
$!      The following logicals will be defined before running test:
$!
$!	    MSFC_BIN:	    Job level logical indicating where fast commit test
$!			    executables are.
$!	    MSFC_RESULT:    Job level logical indicating where sessions write
$!			    result files.
$!	    MSFC_DATA:	    Job level logical indicating data directory.  This
$!			    directory must hold the definition file and is used
$!			    by the fast commit driver and test suites to write
$!			    temporary copy files.
$!
$!      History:
$!
$!      ??-???-????     Unknown
$!              Created Multi-Server Fast Commit test.
$!      17-aug-90	rogerk
$!		Fixed problem in loop which checked test results.  Added goto 
$!	    	back to top of session loop (SESSR_LOOP).  We were only 
$!		checking results for one session for each session line.
$!		 Added more trace output messages.
$!      21-Nov-1991     Jeromef
$!              Add msfc test to piccolo library
$!
$!
$
$ on error then exit
$
$!
$! Get argument list
$!
$ base_num = p1
$ if "''base_num'" .eqs. "" then goto USAGE
$
$!
$! Run setup script
$!
$ @ing_tst:[be.msfc.dcl]msfcset.com
$
$!
$! Loop for each fast commit test
$!    clean up copy files and sync tables left from previous loop
$!    start servers for each session
$!    create subprocess to run session
$!    run driver to sync up each session before server crash
$!
$
$ create msfc_data:ctab_'base_num'_dum.in
$ delete msfc_data:ctab_'base_num'_*.in;*
$
$ test_loop = 26
$ TEST_LOOP:
$ if test_loop .gt. 28 then goto TEST_DONE
$
$     write sys$output " "
$     write sys$output "Starting test loop number ''test_loop' (''test_loop'/26)"
$     write sys$output " "
$!
$!    Clean up from possible previous runs of driver
$!	Create dummy files before deleting just so delete won't return an
$!      error if there is no files to delete.
$!
$     create msfc_data:msfc_copy_'base_num'_dum.in
$     delete msfc_data:msfc_copy_'base_num'_*.in;*
$
$!
$!    Start up servers and sessions according to test definition file
$!
$     close/nolog test_def
$     open test_def msfc_data:msfc'base_num'.def
$     session_num = 0
$     sync_db = ""
$
$!
$!    Read definition file:
$!        If line is a server line, then start a new server
$!	  If line is a database line, start specified number of sessions on
$!	      that database
$!
$
$     SRV_LOOP:
$	  read/end_of_file=SRV_DONE test_def input_line
$	  input_line = f$edit(input_line, "COMPRESS, TRIM, UPCASE")
$
$	  if f$element(0, " ", input_line) .eqs. "SERVER:" then goto SRV_START
$	  if f$element(0, " ", input_line) .eqs. "DATABASE:" then goto SESS_START
$	  goto DEFAULT
$
$	  SRV_START:
$!
$!	      Start server with specified options
$!	      Save ii_dbms_server definition to pass to slave process
$!
$	      write sys$output "    Starting Server"
$	      opt_pos = f$locate(" ", input_line)
$	      server_options = f$extract(opt_pos,132,input_line)
$	      start_server 'server_options' cache_name="MSFC_''base_num'"
$	      server :== "''f$trnlnm("II_DBMS_SERVER", "LNM$JOB")'"
$	      goto DEFAULT
$
$	  SESS_START:
$!
$!	      Start specified number of sessions on this database
$!	      Each session is started on the latest server definition
$!	      If this is the first database (sync_db not defined), then run
$!		  the cleanup routine and use this db as the sync database.
$!
$	      dbname = f$element(1, " ", input_line)
$	      sess_count = f$element(2, " ", input_line)
$
$	      if "''sync_db'" .nes. "" then goto NOSYNC
$		  sync_db = dbname
$		  sync_srv = server
$		  def/proc/nolog ii_dbms_server 'sync_srv'
$		  msfc_cleanup 'sync_db' 'base_num'
$	      NOSYNC:
$
$	      sess_loop_num = 1
$	      SESS_LOOP:
$	          if sess_loop_num .gt. sess_count then goto SESS_LOOP_DONE
$		  session_num = session_num + 1
$		  write sys$output "        Starting session ''session_num' - database ''dbname'"
$		  spawn/nowait/input=nl:/output=-
		      msfc_result:msfc_base'base_num'_sess'session_num'_'test_loop'.out -
		      msfc_slave 'test_loop' 'session_num' 'base_num' -
			  'sync_db' 'dbname' 'server'
$		  sess_loop_num = sess_loop_num + 1
$	      SESS_LOOP_DONE:
$
$	      goto DEFAULT
$
$	  DEFAULT:
$	  goto SRV_LOOP
$
$     SRV_DONE:
$     close/nolog test_def
$     
$!
$!    Start driver program
$!
$     write sys$output "    Waiting for sessions to complete ..."
$     def/proc/nolog ii_dbms_server 'sync_srv'
$	sho log ii_dbms_server
$     msfc_driver 'sync_db' 'base_num' 'session_num'
$
$     wait 0:02:00
$
$     test_loop = test_loop + 1
$     goto TEST_LOOP
$ TEST_DONE:
$
$!
$! Generate output file of all the tables created during the test
$!
$ write sys$output " "
$ write sys$output " "
$ write sys$output "Checking results of Fast-Commit Tests"
$ write sys$output "-------------------------------------"
$ write sys$output " "
$
$ deassign/process ii_dbms_server
$ start_server shared multi
$ session_num = 1
$ all_passed = 1
$ close/nolog test_def
$ open test_def msfc_data:msfc'base_num'.def
$ RESULT_LOOP:
$     read/end_of_file=RES_DONE test_def input_line
$     input_line = f$edit(input_line, "COMPRESS, TRIM, UPCASE")
$
$     if f$element(0, " ", input_line) .nes. "DATABASE:" then goto RESULT_LOOP
$
$     dbname = f$element(1, " ", input_line)
$     sess_count = f$element(2, " ", input_line)
$
$     sess_loop_num = 1
$     SESSR_LOOP:
$	  if sess_loop_num .gt. sess_count then goto SESSR_DONE
$
$	  write sys$output "Checking database ''dbname' : session ''session_num'"
$	  define/proc sys$output MSFC_RESULT:msfc_base'base_num'_'session_num'.res
$	  msfc_check 'dbname' 'base_num' 'session_num'
$	  deassign/proc sys$output
$
$	  !
$	  ! Diff results with canon
$	  !
$         diff_file="MSFC_DIFS:msfc_base'base_num'_'session_num'.dif"
$         define/proc sys$output 'diff_file'
$	  diff MSFC_RESULT:msfc_base'base_num'_'session_num'.res MSFC_CANON:msfc.res
$	  deassign/proc sys$output
$
$	  !
$	  ! If no diffs, then delete result files and clean up the database
$	  !
$	  difs = "0"
$         search/nooutput 'diff_file' "Number of difference sections found: 0"
$         if "''$SEVERITY'" .nes. 1 then difs="1"
$         if 'difs' .eqs. 1 then all_passed = 0
$         if 'difs' .eqs. 1 then goto DIFFS_YES
$             write sys$output "    Test shows no diffs (''all_passed') - Cleaning up test results"
$             delete MSFC_RESULT:msfc_base'base_num'_'session_num'.res;*
$             delete MSFC_RESULT:msfc_base'base_num'_sess'session_num'_*.out;*
$             msfc_cleandb 'dbname' 'base_num' 'session_num'
$	      goto DIFFS_NO
$         DIFFS_YES:
$	      write sys$output "    Test has differences"
$         DIFFS_NO:
$
$	  sess_loop_num = sess_loop_num + 1
$	  session_num = session_num + 1
$	  goto SESSR_LOOP
$     SESSR_DONE:
$     goto RESULT_LOOP
$ RES_DONE:
$
$!
$! Shut down server used to collect results
$!
$ iimonitor
set server shut
quit
$
$!
$! Remove temp data files left around
$!
$ create msfc_data:ctab_'base_num'_dum.in
$ create msfc_data:msfc_copy_'base_num'_dum.in
$ delete msfc_data:ctab_'base_num'_*.in;*
$ delete msfc_data:msfc_copy_'base_num'_*.in;*
$
$!
$! If all the tests passed, then remove the database log files.
$!
$ if 'all_passed' .eqs. 0 then goto NOT_ALL_PASSED
$     create MSFC_RESULT:iidbms.log_dum
$     delete MSFC_RESULT:iidbms.log*;*
$     write sys$output " "
$     write sys$output "All tests passed with no diffs"
$     write sys$output " "
$     exit
$
$ NOT_ALL_PASSED:
$ write sys$output " "
$ write sys$output "There were test differences"
$ write sys$output " "
$
$ exit
$
$
$ USAGE:
$ type/page ing_tst:[be.msfc.dcl]msfchlp.txt
$ exit
