$!
$! DELDEMO.COM   09/13/88 (mah,tjw)
$!
$!       First, check to see if sys$login:abfdemo.dir exists. 
$!		if it does not, then we assume that there is no abfdemo
$!		   for this user and exit.
$!	 Second, see if database given by the user exists.
$!		if this fails then display ingres error messages and exit
$!              otherwise try to destroy the database, exit on failure
$!	 Third, (if we havn't exited already) delete all of [.abfdemo]
$!
$ set noon
$ ORIGMSG = f$environment("MESSAGE")
$ set message/notext/nofac/nosev/noid
$ open    :=
$ close   :=
$ read    :=
$ write   :=
$ edit    :=
$ type    :=
$ del     :=
$ inquire :=
$ create  :=
$ BOLD    :=
$ REVERSE :=
$ NORMAL  :=
$startup:
$ ORIGDIR = f$environment("DEFAULT")
$ on control_y then goto cleanup
$ quiet    := "define/user_mode sys$output nl:"
$ printblk := "type sys$input"
$ println  := "write sys$output"
$!
$    TERMNAME = f$edit(f$trnlnm("TERM_INGRES"),"UPCASE")
$    if TERMNAME .nes. "VT100F" then goto continue
$    BOLD := "[1m"
$    REVERSE := "[7m"
$    NORMAL := "[0m"
$ continue:
$    set def sys$login
$    if (f$search("abfdemo.dir") .eqs. "") then goto not_installed
$    set def [.abfdemo]
$    ABFDEMODIR = f$environment("DEFAULT")
$    if (f$search("iidb_name.txt") .eqs. "") then goto not_installed
$    FNAME = ABFDEMODIR + "iidb_name.txt"
$    open/read TEMPFILE 'FNAME'
$    read TEMPFILE ''DBNAME''
$    close TEMPFILE
$    println BOLD
$    printblk
	WARNING! This program deletes all files in the [.abfdemo] including
	the subdirectory of your login directory:
$    println NORMAL
$    println "		", ABFDEMODIR
$    println BOLD
$    println "	as well as destroying the database '",DBNAME,"'."
$    printblk


	You should be certain that you have no important files in this
	directory before you run this program. 

$    println NORMAL
$    set def sys$login
$ ask:
$    inquire RESP "Do you wish to continue? "
$    if RESP .eqs. "" then goto ask
$    if .not. RESP then goto cleanup
$    println BOLD
$    println "Destroying database ",''DBNAME''
$    println NORMAL
$    define/user sys$error junk
$    destroydb 'DBNAME'
$    if f$file_attributes("junk.","eof") .eqs. 0 then goto deldirs
$! couldn't find db or couldn't destroy it.
$    println BOLD
$    printblk
Because the database you specified could not be destroyed,
the files in the [.abfdemo] subdirectory have not been deleted.
$    println NORMAL
$    goto cleanup 
$!
$! sys$login:[abfdemo] or iidb_name.txt wasn't found so we assume not installed
$!
$ not_installed:
$    println BOLD
$    println "The INGRES ABFDEMO is not installed in this account."
$    println NORMAL
$    goto cleanup 
$!
$! 
$! delete all files in sys$login:[abfdemo]
$!
$ deldirs:
$    println BOLD
$    println "Deleting all files in ",ABFDEMODIR
$    println NORMAL
$    set def sys$login
$    set prot=(O:rwed) [.abfdemo...]*.*.*
$    quiet
$    del [.abfdemo...]*.*.*
$    quiet
$    del [.abfdemo...]*.*.*
$    quiet
$    del [.abfdemo...]*.*.*
$    quiet
$    del [.abfdemo]*.*.*
$    set prot=(O:rwed) abfdemo.dir
$    del abfdemo.dir;
$    println "The INGRES ABFDEMO has been removed from your account."
$!
$ cleanup:
$    del junk.;
$    set def 'ORIGDIR'
$    set message 'ORIGMSG'
$    set on
$    exit
