$!
$!  ABFDEMO.COM    09/13/88 (mah,tjw)
$!
$! Determines whether or not the demo has been installed by checking for the
$! existence of the file:
$!
$!	 	sys$login:[abfdemo]iidb_name.txt
$!
$! If the file exists, start up abf on the database name found in iidb_name.txt
$! otherwise,
$!	-- get a database name from the user. will give an error if the 
$!		database already exists.
$!	-- create the directory sys$login:[abfdemo]
$!	-- create the directory sys$login:[abfdemo.abf] & subdirectories 
$!	-- create a database and fill it with data
$!	-- start abf on the database with the sample application (projmgmt)
$!
$!
$! Turn off error abort
$!
$ set noon
$ savetmp == "No"
$!
$! save original message state
$!
$ ORIGMSG = f$environment("MESSAGE")
$!
$! assign local variables of standard VMS commands
$! note that the error messages produced when there is no assignment
$! REQUIRE turning all error messages OFF:
$!
$ set message/notext/nofacility/noseverity/noidentification
$ open	  :=
$ close   :=
$ read	  :=
$ write   :=
$ edit	  :=
$ type	  :=
$ del	  :=
$ purge   :=
$ inquire :=
$ create  :=
$ NORMAL  :=
$ BOLD	  :=
$ REVERSE :=
$ define/process II_PATTERN_MATCH "SQL"
$ define/process II_DML_DEF "SQL"
$!
$ setup:
$    quiet    := "define/user_mode sys$output nl:"
$    printblk := "type sys$input"
$    println  := "write sys$output"
$    ORIGDIR   = f$environment("DEFAULT")
$    orig_ingabfdir = f$trnlnm("ING_ABFDIR", "LNM$JOB")
$    on control_y then goto cleanup
$! 
$! check for INGRES 6 installation
$!
$ INGRESINSTALLED = f$trnlnm("II_SYSTEM")
$ if (INGRESINSTALLED .nes. "") then goto found_installation
$ printblk

	INGRES Release 6 was not found in your environment. Please
	consult your System Manager for proper access to INGRES.

$     goto cleanup
$ found_installation:
$!
$! check if terminal is defined to be a vt100.  If so, then define escape
$! sequences for bold and reverse video.
$!
$     TERMNAME = f$edit(f$trnlnm("TERM_INGRES"),"UPCASE")
$     if TERMNAME .nes. "VT100F" then goto not_vt100
$     BOLD := "[1m"
$     NORMAL := "[0m"
$     REVERSE := "[7m"
$ not_vt100:
$    if TERMNAME .nes. ""  then goto cont1
$
$!
$! define the terminal so that F1 is the ingres menu key
$! recommend that the user use a TERM_INGRES definition if none exists.
$!
You do not have the logical TERM_INGRES set to your terminal type. It is
recommended that you set the this logical before you use the demo.

See the INGRES Terminal users guide for more information.

$ cont1:
$!
$! if abfdemo does not find sys$login:[abfdemo], then print the initial
$! greeting and create the database if requested.
$! allow the user to exit after he has been told what the side effects of the
$! abfdemo are.
$!
$    set default sys$login:
$    if f$search("abfdemo.dir") .eqs. "" then goto no_demo_dir
$    set default [.abfdemo]
$!
$! get the full path of the abfdemo directory
$!
$    ABFDEMODIR = f$environment("DEFAULT")
$!
$! get the DBNAME from [.ABFDEMO]iidb_name.txt
$!
$    if f$search("dbname.txt") .eqs. "" then goto continue
It seems that you already have the ABFDEMO version 5.0 installed.

In order to install the 6.0 version, you must run the 5.0 version of DELDEMO.
$    goto cleanup
$ continue:
$    if f$search("iidb_name.txt") .eqs. "" then goto no_db_file
$    FNAME = ABFDEMODIR + "iidb_name.txt"
$    open/read TEMPFILE 'FNAME'
$    read TEMPFILE ''DBNAME''
$    close TEMPFILE
$!
$! always set [.abfdemo.abf] as ing_abfdir
$! create it if it doesn't exist
$!
$    if f$search("abf.dir") .nes. "" then goto loc_abf_dir
$    quiet
$    create/directory [.ABF]
$    quiet
$    create/directory [.ABF.'DBNAME']
$    quiet
$    create/directory [.ABF.'DBNAME'.PROJMGMT]
$!
$ loc_abf_dir:
$    set default [.abf]
$    define/job ING_ABFDIR 'f$environment("DEFAULT")'
$    set default [-]
$!
$ dexists1:
$!
$!   println "Starting abfdemo on ",DBNAME," in ",ABFDEMODIR," w/",f$directory()
$!
$    goto demo_exists
$!
$! if the directory exists but there is nofile called "iidb_name.txt", 
$! then tell the user to rename the directory and exit
$!
$ no_db_file:
$    println BOLD
$    println "Your login directory already has a sub-directory called:"
$    println NORMAL
$    println "	",ABFDEMODIR
$    println BOLD
$    println "To use the INGRES ABF Demonstration, you must -
rename this sub-directory:"
$    println NORMAL
$    println "	RENAME  ",f$logical("sys$login"),"ABFDEMO.DIR",-
		"  ",f$logical("sys$login"),"ABFDEMO1.DIR"
$    println ""
$    goto cleanup
$!
$! if the subdirectory [.abfdemo] does not exist, INSTALL the user
$!
$ no_demo_dir:
$    set default [.abfdemo]
$    ABFDEMODIR = f$environment("DEFAULT")
$    set default sys$login
$    println REVERSE
$    printblk
                                                                            
                                                                            
                                                                            
               Welcome to the INGRES ABF DEMONSTRATION!                     
                                                                            
        To use the INGRES ABF Demonstration, a database and some            
        files must be created for your use. The demonstration               
        creates a subdirectory called [.ABFDEMO] in your login              
        (home) directory as well as a database using a name you             
        supply. When you run this command for the first time, it will       
        take several minutes while the necessary files are set up,          
        but after the installation process is completed, you can use        
        this command to start ABF and pick up where you left off.           
        The "deldemo" command can be used to uninstall the demo and         
        remove your database.                                               
                                                                            
                                                                            

$    println NORMAL
$ cont:
$    println BOLD
$    println "Do you wish to continue?"
$    println NORMAL
$    inquire RESP ""
$    if RESP .eqs. "" then goto cont
$    if .not. RESP then goto cleanup
$!
$! Set up logical II_DML_DEF
$!
$    define/process II_DML_DEF "SQL"
$!
$! get the name of the database from the user
$!
$ get_db_name:
$    inquire DBNAME "What name do you want to give your ABFDEMO database? " 
$    if DBNAME .eqs. ""EXIT"" .or.-
        DBNAME .eqs. ""QUIT"" then goto cleanup
$    if DBNAME .eqs. "" then goto get_db_name
$!
$ create_db:
$    println "Creating ABFDEMO database """,''DBNAME'',""""
$!   quiet
$    delete iiabfdemo.tmp;*
$    define/user sys$output iiabfdemo.tmp
$    createdb 'DBNAME'
$    quiet
$    search iiabfdemo.tmp;1 successfully
$    if $status .eqs. "%X00000001" then goto good_db_name
$    quiet
$    search iiabfdemo.tmp;1 exists
$    if $status .nes. "%X00000001" then goto problem 
$    println "A DATABASE CALLED """, ''DBNAME'', """ ALREADY EXISTS."
$    println ""
$    println "TRY A DIFFERENT NAME, OR TYPE ""QUIT"" TO EXIT ABFDEMO."
$    goto get_db_name
$ problem:
$    printblk 
            There was a problem creating the database.
    Please ask your System Manager to examine iiabfdemo.tmp;*.
$    savetmp = "Yes"
$    goto cleanup
$!
$ good_db_name:
$    println "Creating the directory ",ABFDEMODIR
$    quiet
$    create/directory [.ABFDEMO]
$!
$!   create the abf directories so that abf doesn't complain on startup
$!
$    quiet
$    create/directory [.ABFDEMO.ABF]
$    quiet
$    create/directory [.ABFDEMO.ABF.'DBNAME']
$    quiet
$    create/directory [.ABFDEMO.ABF.'DBNAME'.PROJMGMT]
$!
$! Copy in application, changing source directory
$!
$    println "Copying projmgmt application to the database." 
$    quiet
$    copyapp in -dii_system:[ingres.files.abfdemo] -r - 
	-s'ABFDEMODIR' 'DBNAME' iicopyapp.tmp
$!
$!   use copy form to get emp, tasks, and projects forms for qbf
$!
$    println "Copying in forms for QBF."
$    quiet
$    copyform -i -s 'DBNAME' ii_system:[ingres.files.abfdemo]qbf_forms.frm
$!
$!   use sreport to get experience report into database
$!
$    println "Copying in reports."
$    quiet
$    sreport -s 'DBNAME' ii_system:[ingres.files.abfdemo]experience.rw
$!
$    println "Running Sysmod on database."
$    quiet
$    sysmod -w 'DBNAME'
$!
$! Copy all the needed files from the main directory, to the users directory.
$!
$    println "Copying files."
$    copy ii_system:[ingres.files.abfdemo]*.*/exclude= *.osq [.abfdemo]
$!
$! record the dbname in a file called [.ABFDEMO]iidb_name.txt
$!
$    println "Database name """,DBNAME,""" being stored in [.abfdemo]iidb_name.txt"
$    set default [.abfdemo]
$!
$! fill the database with the already prepared data.
$!
$    println "Filling the database." 
$!
$    quiet
$     sql -s 'DBNAME' <sqlcopy.in
$    quiet
$     sql -s 'DBNAME' <delemp.sql
$!
$    ABFDEMODIR = f$environment("DEFAULT")
$    set default [.abf]
$    define/job ING_ABFDIR 'f$environment("DEFAULT")'
$    set default [-]
$    FNAME = ABFDEMODIR + "iidb_name.txt"
$    open/write TEMPFILE 'FNAME'
$    write TEMPFILE ''DBNAME''
$    close TEMPFILE
$    println "Converting file paths in 4GL source code."
$    set default 'ABFDEMODIR'
$    EDTCOMM =  ABFDEMODIR + "edtcomm.in"
$    open/write TEMPFILE 'EDTCOMM'
$    write TEMPFILE "s/II_SYSTEM:[INGRES.FILES.ABFDEMO]/",ABFDEMODIR,"/w"
$    write TEMPFILE "ex"
$    close TEMPFILE
$    quiet
$    edit/edt/command='EDTCOMM' DATABASE.OSQ
$    quiet
$    edit/edt/command='EDTCOMM' EMPTASKS.OSQ
$    quiet
$    edit/edt/command='EDTCOMM' EMPDEP.OSQ
$    quiet
$    edit/edt/command='EDTCOMM' LIST.OSQ
$    quiet
$    edit/edt/command='EDTCOMM' TOP.OSQ
$    del 'EDTCOMM';
$    purge 'ABFDEMODIR'
$!
$! installation is completed
$!
$    printblk

	The INGRES ABF Demonstration has been installed for your use.
$!
$    println "	The name of your database is """,DBNAME,""" and your application files 
$    println "	are stored in the directory ",ABFDEMODIR
$    printblk
	The next time you give the command "abfdemo", ABF will start up
	using this database and the sample application "projmgmt".
	At this point, you can either go directly to ABF or return to VMS.
$!
$ ask:
$    println BOLD
$    println "Do you wish to start up ABF?"
$    println NORMAL
$    inquire RESP ""
$    if RESP .eqs. "" then goto ask
$    if .not. RESP then goto cleanup
$    goto startup
$!
$ demo_exists:
$ startup:
$    println ""
$    println "Starting ABF on """,DBNAME,"""" 
$    println "   Your application files are in: ",ABFDEMODIR
$    set default 'ABFDEMODIR'
$    println ""
$! For Release 6.2: removed application name from call to ABF
$    println "$ abf ",DBNAME
$    println ""
$! For Release 6.2: removed application name from call to ABF
$    define/user sys$input sys$command
$    abf 'DBNAME'
$    DBSTAT = $STATUS
$    if DBSTAT then goto cleanup
$    printblk "  Having problems connecting with database """,DBNAME,"""."
$    printblk "  Ask your system manager for help."
$!
$ cleanup:
$    println "Exiting The INGRES ABFDEMO."
$    deassign/job ING_ABFDIR
$    if orig_ingabfdir .NES. "" THEN -
    	define/job/nolog ING_ABFDIR "''orig_ingabfdir'"
$    set default 'ORIGDIR'
$    if savetmp .nes. "Yes" then delete iiabfdemo.tmp;*
$    set message 'ORIGMSG'
$    set on
$    exit
