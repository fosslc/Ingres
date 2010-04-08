$!===== ADD DIRECTORIES TO A NEW DEVICE ============================
$!
$!	INGDEFDEV - define a directory structure for [ingres]
$!		and the subdirectories of INGRES.
$!
$!	Parameters:
$!		dev_name(P1) - if specified, is the name of a disk
$!			or concealed device which should have
$!			the ingres subdirectories added to it.
$!			If no parameter given, prompt for this
$!			information
$!
$!		dir_type(P2) - should be one of "DATABASES", "CHECKPOINTS",
$!			"DUMPS", "JOURNALS", or "WORK".  It determines what
$!			type of	directory structure should be made.
$!
$!		dev_good - is return value of "TRUE" if the device
$!			was successfully added, and "FALSE" if it
$!			was not.
$!
$!! History:
$!!	01-sep-2005 (abbjo03)
$!!	    Correct reference to the Installation Guide to DBA Guide.

$ on control_y then goto FINISH
$ dev_good :== "FALSE"

$ if "''p1'" .eqs. "" then goto NOPARGIVEN
$ devname := 'p1'
$ goto PARGIVEN

$ NOPARGIVEN:
$ type sys$input
|------------------------------------------------------------------
|
| This command procedure creates directories for INGRES on a device 
| (disk or concealed device) on which you plan to place components 
| (databases, journals, dumps, checkpoints, or work) of an INGRES 
| database.  
|
| Enter the name (either physical or SYSTEM logical name) of a disk 
| or concealed device on which components of INGRES will be placed.
|
|------------------------------------------------------------------

$ GET_DEV:
$ inquire devname "Enter device on which INGRES component will be placed"
$ if ''devname' .eqs. "" then goto BAD_DEV
$ PARGIVEN:
$!
$!	Now, see if the specified device has a colon following it.
$!	If not, put one on.
$!
$ colon = 'f$length(devname)' - 1
$ if 'f$locate(":", devname)' .eq. colon then goto DEVOKMAY
$ devname := 'devname'':'	!Needs a colon so we add it

$ DEVOKMAY:
$!
$!	Now, see if the device exists.  If so, try to create the
$!	[INGRES] directories.  If not, get out and give error.
$! 
$ devstat := 'f$getdvi(devname, "exists")'
$ if "''devstat'" .eq. "TRUE" then goto DEVOKMAY1
$ goto BAD_DEV

$ DEVOKMAY1:
$!
$!	Now, see if the root directory for INGRES exists.  If not,
$!	terminate the procedure.
$!
$ rootdir := "''devname'[ingres]"
$ if "''f$parse(rootdir)'" .eqs. "" then goto ERR_CTRL

$ on error then goto ERR_CTRL
$ if "''p2'" .nes. "" then goto PARGIVEN2
$ GET_DIR_TYPE:
$ type sys$input

|------------------------------------------------------------------------
|
| Please enter the Ingres directory type you wish to create:
| Valid choices = "DATABASES", "JOURNALS" "DUMPS", "CHECKPOINTS" or "WORK".
|                 Type <CTRL> Y to exit.
|
|------------------------------------------------------------------------

$ inquire p2 "Please enter Ingres directory type"
$ PARGIVEN2:
$ dir_type := 'p2'
$ if (("''dir_type'".nes."DATABASES") -
	.and. ("''dir_type'".nes."CHECKPOINTS") -
	.and. ("''dir_type'".nes."DUMPS") -
	.and. ("''dir_type'".nes."JOURNALS") -
	.and. ("''dir_type'".nes."WORK")) -
		 then goto BAD_DIR
$!
$!	Now, create the directories needed on this device.
$!	If they are already there, skip over them.
$!
$ if "''p2'" .nes. "DATABASES" then goto DO_JNL_DEV
$ x1 := "''devname'[ingres.data]"
$ if "''f$parse(x1)'" .nes. "" then goto DATA_EX
$ write sys$output "Creating ''devname'[ingres.data]..."
$ create/dir 'devname'[ingres.data]
$ set prot=(s:rwe,o:rwe,g,w:r) 'devname'[ingres]data.dir
$ symb_db :== 'devname'
$ DATA_EX:
$ goto DEVEXIT

$ DO_JNL_DEV:
$ if p2 .nes. "JOURNALS" then goto DO_DMP_DEV
$ x1 := "''devname'[ingres.jnl]"
$ if "''f$parse(x1)'" .nes. "" then goto JNL_EX
$ write sys$output "Creating ''devname'[ingres.jnl]..."
$ create/dir 'devname'[ingres.jnl]
$ set prot=(s:rwe,o:rwe,g,w:re) 'devname'[ingres]jnl.dir
$ symb_jnl :== 'devname'
$ JNL_EX:
$ goto DEVEXIT

$ DO_DMP_DEV:
$ if p2 .nes. "DUMPS" then goto DO_CKP_DEV
$ x1 := "''devname'[ingres.dmp]"
$ if "''f$parse(x1)'" .nes. "" then goto DMP_EX
$ write sys$output "Creating ''devname'[ingres.dmp]..."
$ create/dir 'devname'[ingres.dmp]
$ set prot=(s:rwe,o:rwe,g,w:re) 'devname'[ingres]dmp.dir
$ symb_dmp :== 'devname'
$ DMP_EX:
$ goto DEVEXIT

$ DO_CKP_DEV:
$ if p2 .nes. "CHECKPOINTS" then goto DO_WRK_DEV 
$ x1 := "''devname'[ingres.ckp]"
$ if "''f$parse(x1)'" .nes. "" then goto CKP_EX
$ write sys$output "Creating ''devname'[ingres.ckp]..."
$ create/dir 'devname'[ingres.ckp]
$ set prot=(s:rwe,o:rwe,g,w:re) 'devname'[ingres]ckp.dir
$ symb_ckp :== 'devname'
$ CKP_EX:
$ goto DEVEXIT

$ DO_WRK_DEV:
$ if p2 .nes. "WORK" then goto BAD_DIR
$ x1 := "''devname'[ingres.work]"
$ if "''f$parse(x1)'" .nes. "" then goto WRK_EX
$ write sys$output "Creating ''devname'[ingres.work]..."
$ create/dir 'devname'[ingres.work]
$ set prot=(s:rwe,o:rwe,g,w:re) 'devname'[ingres]work.dir
$ symb_wrk :== 'devname'
$ WRK_EX:
$ goto DEVEXIT

$ BAD_DEV:
$ type sys$input

|------------------------------------------------------------------------------
|
| 	A bad device parameter has been submitted to the INGDEFDEV utility.
|   At the prompt, re-enter a valid device or <CTRL> Y to exit this procedure.
|
|------------------------------------------------------------------------------

$ goto GET_DEV

$ ERR_CTRL:
$ type sys$input
|------------------------------------------------------------------------------
|	One of the following errors has occurred to halt this procedure:
|     - The device:[000000]000000.DIR does not have at least W:E protection
|     - The root [INGRES] directory does not exist on the specified device
|     - The root [INGRES] directory protection is not set correctly
|     - The root [INGRES] directory is not owned by [INGRES]
|
|	  Refer to Chapter 3 of the Ingres Database Administrator Guide
|------------------------------------------------------------------------------
$ wait 00:00:05
$ goto FINISH

$ BAD_DIR:
$ type sys$input

|------------------------------------------------------------------------
|          A bad directory type parameter has been entered.
|  It must be one of "databases", "checkpoints", "journals", or "work".
|------------------------------------------------------------------------

$ goto GET_DIR_TYPE

$ DEVEXIT:
$ dev_good :== "TRUE"		! Build.com receives successful status
$ exit

$ FINISH:			! Control-Y Handler
$ dev_good :== "FALSE"		! Build.com receives un-successful status
$ exit
