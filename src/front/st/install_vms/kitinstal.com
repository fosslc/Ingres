$ goto beginning !		KITINSTAL.COM

		Ingres V 1.0 installation procedure for VMS systems


	History:
	    5-jan-1993 (rtroy)
		First draft Created.
	    26-jul-1993 (kellyp)
		Finished.
	    18-aug-1993 (kellyp)
		Define II_MSGDIR to be used while running VMSINSTAL
	    3-sep-1993 (kellyp)
		Undefines II_AUTHORIZATION in the environment before checking 
		for it's validity.
	    7-sep-1993 (kellyp)
		Deassigns II_AUTHORIZATION at the process level as well as job
		level regardless of mode before validating the user provided
		authorization string.
	    9-sep-1993 (kellyp)
		Corrected the value that II_MSGDIR gets defined to
	    14-sep-1993 (kellyp)
		Corrected incorrect comments
	    15-oct-1993 (kellyp)
		II_SYSTEM must be defined to a rooted concealed logical.
		Changed the default to running the IVP.
		Added the invocation of script to setup symbols for INGRES.
	    21-oct-1993 (kellyp)
		Modified sections init and existing and modified instructions
		among other things.
	    26-oct-1993 (kellyp)
		Deassigns II_MSGDIR, II_TERMCAP_FILE, II_PACKAGE_LIST
		before exiting VMSINSTAL. 
		Move release.dat to [ingres.install] from the working directory.
	    26-oct-1993 (kellyp)
		Changed the wording for a particular prompt
	    01-nov-1993 (joplin)
		Fixed a problem when trying to install to DEV:[000000.]
	    01-nov-1993 (kellyp)
		II_AUTHORIZATION is required.
	    03-nov-1993 (kellyp)
		Deleted /NEW_VERSION from RENAME_FILE.
	    09-nov-1993 (kellyp)
		Corrected logic problem when responding to invalid location
		for II_SYSTEM.
		Also, solved the problem of installing over existing 
		installation.  by not running in SAFETY mode and using 
		RENAME instead of the callback RENAME_FILE.
	    17-nov-1993 (joplin)
		Removed requirement for II_SYSTEM to be defined to an
		intermediate concealed logical device.  Changed perform_ivp
		to set uic to the owner of the installation before spawning
		off the setup programs.  This required that the real uic
		be obtained from an existing installation.
	    18-nov-1993 (kellyp)
		Added code to set the permission back to the original setting.
		Need to do this because PROVIDE_FILE for some odd reason
		changes the file permissions.
		Also,  need to define II_MANIFEST_DIR for iimaninf since 
		iimaninf now requires that II_MANIFEST_DIR is set.
	    22-nov-1993 (joplin)
		Further enhancements to execute the IVP on behalf of
		the owner of the installation.
	    23-nov-1993 (kellyp)
		Took out the code which sets up symbols for INGRES.
		Because VMSINSTAL clears all the symbols, this is a futile
		attempt.
	    29-nov-1993 (kellyp)
		Modified to give the user the flexibility to override 
		default II_AUTHORIZATION from the environment.
		( fix to bug 57365 )
	    03-dec-1993 (kellyp)
		Deletes extraneous files from the old installation in the
		case of upgrade ( fix to bug 57366 )	
	    15-dec-1993 (joplin)
		Made sure owner of installation will own the INGRES.DIR
		and files created during the IVP.  Allowed bracketed rights
		identifiers to be used, such as [INGRES] for owner uic.
	    22-dec-1993 (joplin)
		Beefed up disk space checking.  Added support for this check
		to work for AWD option and of cases where the installation
		and the VMI$KWD share the same disk.
	    05-jan-1994 (joplin)
		Changed account creation for installation owners.
	    31-jan-1994 (joplin)
		Fixed UIC problem.  Fixed spelling errors and Ingres name.
	    22-feb-1994 (joplin)
		Dis-allowed the selection for the "install" package.
	    09-apr-1994 (joplin)
		Changed II_AUTHORIZATION to II_AUTH_STRING.
	    02-Feb-1995 (canor01)
		save and restore ii_auth_string on aborted install
	    11-Apr-1995 (canor01)
		Test for existence of files to be renamed before renaming
	    01-may-1995 (wolf) 
		Create the [.ingres] directory after validating II_SYSTEM.
		Waiting until the subroutine do_it is too late, as proven
		by bug 68289.
		Add C2 to the list of IVP setups to run
	    06-jun-1995 (wolf) 
		Refer to OpenINGRES rather than INGRES.  Be consistent in
		references to OpenINGRES manuals.
	    16-jan-1996 (dougb)
		Implement the new "package" installation procedure.
	    09-feb-1996 (albany)
		Fix typo's and some logic from the prior change.
	    20-mar-1996 (albany)
		Make 'package' install and 'all' options work properly.
	    01-aug-1996 (rosga02)
		Added test to make sure upgrade can be done to OPEN ingres
                installation only. Also, changes references to VERSION*.REL
                to RELEASE.DAT 
	    28-aug-1996 (rosga02)
		Fixed two problems:
                - Installation history was always showing emty list,
                  because a file in the default dir was used instead of one
                  in sys$common:[sysexe]. 
                - Installation history records didn't have complete release id
                  because f$element starts from count 0 not 1.
	    01-oct-1996 (rosga02)
	        Allow upgrade from 6.4 by searching either RELEASE.DAT or
		VERSION*.REL file                
            03-feb-1997 (boama01)
		Fixes and changes for Bug 80057:
                - When processing delete_files.dat, skip comment lines (those
		  reduced to "" by f$edit(,"UNCOMMENT")).
		- INSTALL pkg is now INVISIBLE; in package_loop, comment old
		  logic forcing selection of visible pkg and add logic to
		  always include pkg in package_list.
            24-msr-1997 (rosga02)
		Made changes changes for SEVMS: Include iisues.com in IVP
            12-jun-1997 (rosga02)
		Reveresed the above change. The iisues.com should not be in IVP
            13-jun-1997 (rosga02)
		Delete old SLUTIL.EXE instead of Rename, otherwise repetead
		upgrade may cause renaming to the same version and abort
	    23-Dec-1997 (kinte01)
		Clean up spelling errors and replace occurrences of 
		CA-OpenIngres with OpenIngres
	    21-Apr-1998 (kinte01)
		Replace OpenINGRES with Ingres.
	    08-Sep-1998 (kinte01)
		Added new CA Licensing installation procedures. 
		To do this required:
		1) Restoring the INGRESII.Z saveset that is now provided in
		   the II_SYSTEM:[ingres.install] directory. The CA licensing
		   procedures state that the CA licensing saveset provided
		   be renamed from CALIC010.Z to <product name>.Z This
		   saveset is restored into the VMS$KWD directory. 
		2) There is a file silent.com that is part of the INGRESII.Z
		   saveset that should be able to be used to do the license
		   install from this point on. Unfortunately, it did
		   not do the trick as the section for renaming the calic.olb
		   to sys$share:calic.olb removed the file from my working
		   directory. Shouldn't do this but appears to be because we
		   are calling a kitinstal.com procedure from within a 
		   kitinstal procedure. Did not research this failure 
		   exhaustively. Instead I took essential parts of silent.com 
		   inserted it in the old auth string check area with some
		   minor modifications.  The modifications were just to
		   change the location of where the shared library link
		   looked for the calic.olb file.
	25-may-1999 (kinte01)
		Add modifications necessary to support an alternate 
		release.dat file for gateways.
	02-jun-1999 (kinte01)
		Cleaned up a reference to the license authorized string.
	09-jun-1999 (kinte01)
		Need to initialize gwonly
	24-jun-1999 (kinte01)
		Add additional information for new licensing saveset 
		distributed -- It now includes LicenseIT & InstalIT
	06-jul-1999 (kinte01)
		Update search for lic98.dat
	15-Nov-2000 (kinte01)
	   Replace procedures from 8-sep-1998 for handling the licenseit
	   saveset.  Procedures are fully explained in the checklist.txt
 	10-Apr-2001 (kinte01)
	   Change the procedures for the gateways (104584)
	25-Apr-2001 (kinte01)
	   If choosing to have this procedure create the Ingres System
	   Administration account, the default process privileges will
	   now match those that are documented in the Getting Started 
	   Guide (102541)
	16-May-2001 (kinte01)
	   Write release String from the kitinstal.data file into the
	   sys$system:ingres_installations.dat file.
	18-Jun-2001 (horda03)
	   Ensure that the Ingres System Administrator own the top level
	   Ingres directory.
	06-dec-2002 (abbjo03)
	    Change GS Guide for OpenVMS to simply GS Guide.
	08-jan-2003 (abbjo03)
	    Temporarily disable cluster support.
	15-jan-2003 (abbjo03)
	    Increase ISA pgflquota to 250,000.
	02-apr-2004 (abbjo03)
	    Use translation of II_SYSTEM when creating the ISA account.
	01-mar-2005 (abbjo03)
	    Remove CA licensing.
	31-may-2005 (abbjo03)
	    Remove Licensed? column when displaying packages.
	01-nov-2005 (abbjo03)
	    Add display of TOSL and ask acceptance from user.
	05-Jan-2007 (bonro01)
	    Change Getting Started Guide to Installation Guide.
	14-May-2007 (upake01)
	    VMS version # in Kitinstal.DATA has two digits before decimal.
            Modify to extract two digits from version to display correct
            error message if the VMS is below the minimum version.
	04-sep-2007 (abbjo03)
	    Revert change of 1-nov-2005: don't display LICENSE.
	21-Sep-2007 (upake01)
	    Ingres installation fails for VMS versions vv.u-m (example
            Version 07.3-2).  The installation procedure (VmsInstal.COM)
            sets two global symbols - VMI$VMS_VERSION and VMI$FULL_VMS_VERSION.
            The symbol VMI$VMS_VERSION does not contain the "m" part of the
            version.  Symbol VMI$FULL_VMS_VERSION contains the complete version.
            The version # in Kitinstal.Data needs to be compared with
            VMI$FULL_VMS_VERSION and NOT VMI$VMS_VERSION.
        10-Feb-2010 (horda03) Bug 122944 
            Enable the license prompt. 
        05-mar-2010 (joea)
            Increase ENQLM, PGFLQUOTA and TQELM to the minimal Alpha and IA64
            defaults.


Description:

	This routine is the "installation script" for the Ingres product
	set in the OpenVMS environment. It is responsible only for depositing
	the appropriate files in the appropriate location(s). "Configuration"
	is the responsibility of another tool, CBF, and is not addressed here.
	No Ingres software will be up and running at the completion of this
	routine, although the user may elect to perform configuration as
	an immediate post installation step.

	This routine is invoked by VMSINSTAL.COM, which ensures certain
	conditions are true, and provides various facilities specific to
	the OpenVMS environment. VMSINSTAL is the standard installation tool
	in the OpenVMS environment, although Ingres does not require its 
	use.

	Any mechanism which deposits the files correctly would suffice. In
	using VMSINSTAL, this tool must conform to some fairly strict rules:
	Files already existing on the system cannot be accessed directly,
	some DCL commands are prohibited, and no outside utilities can be used.

	This script performs a dialogue with the installer to determine
	what portions of the Ingres product set to provide, and where to
	provide them.

VMSINSTAL Environment

	When this routine is invoked via VMSINSTAL.COM, the following
	defaults are in effect:

	.	The UIC code is [1,4] (SYSTEM)
	.	The default file protection is [s:rwed,o:rwed,g:rwed,w:re].
	.	The default device and directory are MISSING:[MISSING], which
		does not exist. This ensures that it is set explicitly.
	.	All privileges except BYPASS are in effect.
	.	Messages will be output in full format.

Conventions and Guidelines:

	.	Facility code:		INGRES
	.	While in the installation portion (not IVP), you are restricted
		from using the SHOW command or the output from any other utility
	.	While in the installation portion (not IVP), you are restricted
		from using any SET commands. Exceptions:
		-	SET ON & SET NOON - no restrictions
		-	SET VERIFY - only if installer specified DEBUG option
		-	SET FILE - only with files in the VMI$KWD directory.
	.	All global symbols and logicals must begin with 'INGRES_'
	.	All questions are asked as early as possible.
	.	DCL commands, and VMSINSTAL logicals or symbols are
		CAPITALIZED for clarity.
	.	VMSINSTAL defined symbols and logicals begin with: VMI$
	.	Logical references:
		VMI$KWD		Kit Working Directory

	.	's' is used for status

VMSINSTAL options:

	There are ten options available with VMSINSTAL. Not all will be
	useful to either the user or CA.  They are listed below as a 
	guide to what is available.

	User Options
	A	auto-answer	captures/uses a script of answers to questions
	G	get		move the kit savesets to a specified location
	L	log-file	log the entire VMSINSTAL session
	R	alternate root	allows specification of an alternate 
	N	notes		obtain just the release notes

	Developer Options
	B	boot		allows recovery from a reboot during install
	C	callback-trace	logs callbacks for analysis
	K	kit-debug	passes a boolean argument to KITINSTAL.COM
	Q	QA-mode		'quality assurance' performs/ignores various
				callbacks and displays status to terminal
	S	statistics	collects usage info, eg: files/disk block use

IVP - Installation Verification Procedure

	An IVP is "mandatory." It is a command procedure invoked via:

		$ @VMI$KWD:KITINSTAL VMI$_IVP

	Further, it must exit with a return status of either VMI$_SUCCESS, or
	VMI$_FAILURE.

CALLBACKS

	Callbacks are recursive invocations of VMSINSTAL.COM and have the form:

		$ VMI$CALLBACK callback_type [parameter-2 ...]

Special Notes:

     BUG IN DCL: Any IF THEN construct which requires an ENDIF must have the
	THEN command on its own line, preceeded by a $, as in:

	$ IF condition
	$ THEN
	$ ...
	$ ENDIF

     CALLBACK problems or issues:

	PROVIDE_FILE is picky! The file specification MUST include the . suffix
		for an extension even if there is no extension! Also, if it
		returns the error message BADSPEC, it usu. means the directory
		doesn't exits.
	
	CREATE_DIRECTORY always logs to the screen!

	FIND_FILE aborts the installation if the directory spec. is invalid... 
		so we can't rely on it! But, F$SEARCH works! (as does F$PARSE)

	SET PURGE, All PROVIDE_FILE callbacks must be done before the SET PURGE.

$ beginning: 
$ ON CONTROL_Y THEN GOTO control_y
$ ON WARNING THEN GOTO error_handler
$ IF "''p1'".EQS."VMI$_INSTALL" THEN GOTO perform_install
$ IF "''p1'".EQS."VMI$_IVP" THEN GOTO perform_ivp
$ s=VMI$_UNSUPPORTED

________________________________________________________________________________
				Installation

	First perform preliminary checklist such as the OpenVMS version,
	minimum disk space requirements, etc. To add a check, add an element
	to 'preliminaries' which is the label name to GOSUB to, which RETURNs
	with 'ok' either true or false. 

$ perform_install:
$ ! --> Temporary code to enable tracing when debug (K) option is supplied:
$ IF P2  THEN SET VERIFY
$ ! --> End of temporary code.
$ SET DEFAULT VMI$KWD
$ VMI$CALLBACK SET SAFETY NO	! Keep VMSINSTAL from balking at existing files
$ GOSUB init			! initialize symbols, etc.
$ !
$ i=0
$ next_check:
$ next_step=F$ELEMENT(i,",",preliminaries)
$ IF "''next_step'" .eqs. "," THEN GOTO checklist_completed
$ i=i+1
$ GOSUB 'next_step
$ IF ok THEN GOTO next_check
$ GOTO fail_exit

$ checklist_completed:
$ i=0
$ step:
$ next_step=F$ELEMENT(i,",",install_steps)
$ IF "''next_step'" .eqs. "," THEN GOTO install_done
$ i=i+1
$ GOSUB 'next_step
$ IF ok THEN GOTO step
$ GOTO fail_exit

$ install_done:
$ !
$ IF "''ivp'".eqs."1"
$ THEN
$	! All of the local symbols will be gone when the IVP is executed, 
$	! so save these in the JOB logical name table.
$	DEFINE/JOB II_PACKAGE_LIST	"''package_list'"
$	DEFINE/JOB II_SUINGRES_UIC	"''uic'"
$	DEFINE/JOB II_SUINGRES_USER 	"''username'"
$ ENDIF
$ s=VMI$_SUCCESS
$ GOTO done
________________________________________________________________________________
		Installation Verification Procedure(s)

$ perform_ivp:
$ GOSUB init
$ package_list=F$TRNLNM("II_PACKAGE_LIST","LNM$JOB") ! Echos from past lives...
$ uic=         F$TRNLNM("II_SUINGRES_UIC","LNM$JOB")
$ ii_system=   F$TRNLNM("II_SYSTEM","LNM$JOB")
$ !To add default IVPs, just put the pkg name to IVP_LIST!
$ ivp_list="dbms,c2,net,star"
$ IF "''package_list'".EQS."" THEN package_list=ivp_list !default IVPs = all!
$ file_name = "VMI$KWD:ivp.dat"
$ imt -setup='package_list -output='file_name -loc=VMI$KWD:
$ GOSUB open_file_read
$ IF .NOT.ok THEN GOTO file_open_read_error
$ !
$ !Now we need to run LOGINOUT.EXE on behalf of the owner, so that we make
$ !sure group resources  are initialized (in particular logical name tables).
$ ivptime=F$TIME()  
$ pname="IISTOP_" + F$CVTIME(ivptime,,"HOUR") + F$CVTIME(ivptime,,"MINUTE") + -
  F$CVTIME(ivptime,,"SECOND") + F$CVTIME(ivptime,,"HUNDREDTH")
$ DEFINE/USER/NOLOG SYS$OUTPUT NLA0:
$ RUN SYS$SYSTEM:LOGINOUT.EXE/PROCESS_NAME='pname' -
  /UIC='uic'/DETACHED/INPUT=NLA0:/OUTPUT=NLA0:/ERROR=NLA0:
$ !
$ read_setup_data:
$ GOSUB read_w_close
$ IF ok
$ THEN
$	! Save some settings
$	ii_saveuic=F$GETJPI("","UIC")
$	ii_savescratch=F$TRNLNM("SYS$SCRATCH","LNM$JOB")
$	!
$	! CL bug workaround
$	rec="II_SYSTEM:[INGRES.UTILITY]" + - 
	f$parse(rec,,,"NAME","SYNTAX_ONLY") + -
	f$parse(rec,,,"TYPE","SYNTAX_ONLY")
$!
$	! Invoke the IVP procedure.  Set the UIC to the owner of the 
$	! installation, make sure sys$scratch is writeable by the owner, and
$	! then spawn off the setup procedure.
$       SET UIC 'uic'
$	DEFINE/JOB/NOLOG sys$scratch ii_system:[ingres]
$	ON WARNING THEN CONTINUE	! Continue if setup fails
$	DEFINE/NOLOG/USER/EXEC/TRANSLATION=(CONCEALED) II_SYSTEM 'ii_system 
$       SPAWN/WAIT/NOLOG @'rec
$       s = $status
$	ON WARNING THEN GOTO error_handler 
$	! Restore the settings and check the results of the IVP procedure
$       SET UIC 'ii_saveuic'
$	IF ii_savescratch.NES."" THEN DEFINE/NOLOG/JOB sys$scratch "''ii_savescratch'"
$ 	IF .not. s
$ 	THEN
$	    WRITE SYS$OUTPUT "''rec' failed."
$	    INQUIRE ingres_ok "Do you want to continue [No]"
$	    IF ingres_ok.EQS."" THEN ingres_ok == "No"
$	    IF .NOT.ingres_ok
$	    THEN
$		GOSUB close_file_read
$		ok=0
$		GOTO ivps_done
$	    ENDIF
$ 	ELSE
$	    WRITE SYS$OUTPUT "''rec' completed."
$ 	ENDIF
$	GOTO read_setup_data
$ ELSE
$	IF "''s'".NES."eof"
$	THEN
$	    WRITE SYS$OUTPUT "Unable to read setup data file."
$	    ok=0
$	ENDIF
$ ENDIF
$ ok=1

$ ivps_done:
$!
$! Set the ownership of any files created during the IVP 
$! to the owner of the Ingres installation (uic).
$ ivptime=F$CVTIME(ivptime,"COMPARISON","DATETIME")
$ ivpmsg=F$ENVIRONMENT("MESSAGE")
$ DEFINE/NOLOG/USER/EXEC/TRANSLATION=(CONCEALED) II_SYSTEM 'ii_system
$ SET NOON
$ ivp_file_loop:
$	fstr=F$SEARCH("II_SYSTEM:[INGRES...]*.*;*")
$	IF "''fstr'".EQS."" THEN GOTO ivp_file_end
$	cdttime=F$CVTIME(F$FILE_ATTRIBUTES(fstr,"CDT"),"COMPARISON","DATETIME")
$	IF "''cdttime'".GES."''ivptime'" 
$	THEN 
$	    SET MESSAGE/NOFACILITY/NOSEVERITY/NOIDENTIFICATION/NOTEXT
$	    SET FILE/OWNER_UIC='uic' 'fstr'
$	    SET MESSAGE 'ivpmsg
$	ENDIF
$	GOTO ivp_file_loop
$ ivp_file_end:
$ SET ON
$!
$ IF ok
$ THEN
$	s=VMI$_SUCCESS
$ ELSE
$	s=VMI$_FAILURE
$ ENDIF
$ ivp_done=1
$ IF "''F$TRNLNM("II_SUINGRES_UIC","LNM$JOB")'".NES."" -
  THEN DEASSIGN/JOB II_SUINGRES_UIC
$ IF "''F$TRNLNM("II_PACKAGE_LIST","LNM$JOB")'".NES."" -
  THEN DEASSIGN/JOB II_PACKAGE_LIST
$ IF "''F$TRNLNM("II_SUINGRES_USER","LNM$JOB")'".NES."" -
  THEN DEASSIGN/JOB II_SUINGRES_USER
$ GOTO done


________________________________________________________________________________
				Subroutines

$ init:
$ !			      IMPORTANT NOTICE
$ !
$ !	THE FOLLOWING ASSIGNMENT SHOULD BE CHECKED FOR EACH RELEASE
$ !
$ !	Please note: 0 means infinite! Not all quotas can be infinite, however.
$ !
$ acnt_qualifiers="/pass=welcome/tqel=100/prcl=15/bytl=500000/pgfl=256000"
$ acnt_qualifiers=acnt_qualifiers+"/dio=200/bio=400/fillm=1000/astlm=200"
$ acnt_qualifiers=acnt_qualifiers+"/enql=4000/jt=20000/maxd=0/priv=all"
$ acnt_qualifiers=acnt_qualifiers+"/directory=[ingres]"
$ acnt_qualifiers=acnt_qualifiers+"/defpriv=(oper,tmpmbx,netmbx)"
$ acnt_qualifiers=acnt_qualifiers+"/priv=(oper,tmpmbx,netmbx)"
$ update_qualifiers="/priv=(altpri,cmkrnl,impersonate,exquota,prmgbl,prmmbx,"
$ update_qualifiers=update_qualifiers+"readall,share,sysgbl,syslck,sysnam,"
$ update_qualifiers=update_qualifiers+"sysprv,world)/defpriv=(altpri,cmkrnl,"
$ update_qualifiers=update_qualifiers+"impersonate,exquota,prmgbl,prmmbx,"
$ update_qualifiers=update_qualifiers+"readall,share,sysgbl,syslck,sysnam,"
$ update_qualifiers=update_qualifiers+"sysprv,world)/flag=nodisuser"
$ !
$ !	Also, the file KITINSTAL.DATA contains definitions which may change
$ !	from release to release, so we've developed a flexible way to provide
$ !	this data so that this procedure should never have to be edited.
$ !
$ !
$ say = "write sys$output"
$ cr[0,8] = %x0D
$ lf[0,8] = %x0A
$ tab = "	"
$ ok = 0		! status of internal subroutines or calls
$ ivp_done=0 		! tells whether or not IVPS are done
$ upgrade=0             ! tells whether or not this is an upgrade
$ t=0			! temporary integer variable
$ tstr=""		! temporary string variable
$ tstr2=""		! temporary string variable
$ fstr=""		! temporary string variable
$ kstr=""		! temporary string variable
$ msg=""		! temporary string variable
$ src=""		! temporary string variable
$ dst=""		! temporary string variable
$ r_filename=""		! r_filename and k_filename and w_filename 
$ k_filename=""		! are initialized to "" to
$ w_filename=""		! preclude an undefined symbol on ^y or ^z exit.
$ ! remember: GLOBALS MUST BE PRECEEDED WITH 'INGRES_'...
$ ingres_ok == 0	! global boolean for external (callback) calls
$ ingres_int==0		! global temp integer
$ ingres_str==""	! global temp string
$ vaxcluster=F$GETSYI("CLUSTER_MEMBER")		!is this vax cluster or not?
$vaxcluster="FALSE"
$ package_menu_options="display,describe,selected,all,authorized"
$ preliminaries = "get_kit_data,welcome,vms_version,disk_space,opening_messages" 
$	! labels to GOSUB to, all RETURN with 'ok'
$ install_steps="existing,owner,where,getuser,auth_string,choose_packages,"
$ install_steps=install_steps+"resolve,confirm,build_scripts,do_it"
$ create_record_list="provide_file_record,purge_files_record,set_prot_record"
$ !the above is the list of DCL labels to GOSUB to to help create scripts

$ !non-forms based access to manifest, returns info from manifest-format file
$ imt= "$VMI$KWD:IIMANINF " 

$ DEFINE II_MSGDIR "''VMI$KWD'"
$ DEFINE II_MANIFEST_DIR "''VMI$KWD'"
$ !
$ !Installation phase initialization
$ IF "''p1'".eqs."VMI$_INSTALL"
$ THEN
$       ! Save some in case of accident
$       save_ii_system = f$trnlnm( "II_SYSTEM" )
$	!Make sure there are no conflicting logical names
$	! Look at process defined values for II_SYSTEM
$ 	IF "''F$TRNLNM("II_SYSTEM","LNM$PROCESS",,"EXECUTIVE")'".NES."" -
  	THEN DEASSIGN/PROCESS/EXEC II_SYSTEM
$ 	IF "''F$TRNLNM("II_SYSTEM","LNM$PROCESS",,"SUPERVISOR")'".NES."" -
  	THEN DEASSIGN/PROCESS/SUPER II_SYSTEM
$ 	IF "''F$TRNLNM("II_SYSTEM","LNM$PROCESS",,"USER")'".NES."" -
  	THEN DEASSIGN/PROCESS/USER II_SYSTEM
$	! Look at job defined values for II_SYSTEM
$ 	IF "''F$TRNLNM("II_SYSTEM","LNM$JOB",,"EXECUTIVE")'".NES."" -
  	THEN DEASSIGN/JOB/EXEC II_SYSTEM
$ 	IF "''F$TRNLNM("II_SYSTEM","LNM$JOB",,"SUPERVISOR")'".NES."" -
  	THEN DEASSIGN/JOB/SUPER II_SYSTEM
$ 	IF "''F$TRNLNM("II_SYSTEM","LNM$JOB",,"USER")'".NES."" -
  	THEN DEASSIGN/JOB/USER II_SYSTEM
$ ENDIF
$ RETURN

$ get_kit_data:
$ file_name="kitinstal.data" ! stores dynamic definitions
$ GOSUB open_file_read
$ IF .NOT.ok THEN GOTO file_open_read_error
$ read_kit_data:
$ GOSUB read_w_close
$ IF ok
$ THEN
$	rec=F$EDIT(rec,"COMPRESS,UNCOMMENT")
$	IF F$LOCATE("RELEASE",rec).LT.F$LENGTH(rec) THEN release=F$EXTRACT(F$LOCATE(":",rec)+2,F$LENGTH(rec),rec)
$	IF F$LOCATE("MINIMUM VMS",rec).LT.F$LENGTH(rec) THEN min_vms=F$EXTRACT(F$LOCATE(":",rec)+2,F$LENGTH(rec),rec)
$	IF F$LOCATE("MINIMUM SPACE",rec).LT.F$LENGTH(rec) THEN min_space=F$EXTRACT(F$LOCATE(":",rec)+2,F$LENGTH(rec),rec)
$	IF F$LOCATE("LAST SAVESET",rec).LT.F$LENGTH(rec) THEN last_saveset=F$EXTRACT(F$LOCATE(":",rec)+2,F$LENGTH(rec),rec)
$	GOTO read_kit_data
$ ELSE
$	IF "''s'".NES."eof"
$	THEN
$	    VMI$CALLBACK MESSAGE F KITDATA "Unable to read kitinstal data file."
$	    ok=0
$	    RETURN
$	ENDIF
$ ENDIF
$ ok=1
$ IF F$LENGTH(last_saveset).NE.1	! this is a litmus test ...
$ THEN
$ 	VMI$CALLBACK MESSAGE F KITDATA "Invalid saveset data read from kitinstal data file."
$	ok=0
$ ENDIF
$ RETURN

$ welcome:
$ ok=1
$ TYPE /PAGE=SAVE=16 LICENSE.
$ VMI$CALLBACK ASK ingres_ok "Do you accept this license agreement? (y or n)" "" BD
$ if .not. ingres_ok
$ then
$    VMI$CALLBACK MESSAGE I NOLICENSE "Installation Cancelled"
$    verbose=0
$    ok=0
$    s=VMI$_FAILURE
$    RETURN
$ endif
$ VMI$CALLBACK MESSAGE I BEGIN "Beginning the installation of Ingres"
$ say ""
$ VMI$CALLBACK MESSAGE I RELEASE "This version is ''release'"
$ TYPE SYS$INPUT

	This procedure may be used to either create new installations, upgrade
	older installations, or to add product packages to existing ones.

	Ingres Corporation strongly recommends reading all appropriate
	documentation prior to performing any installation, most notably
	the release notes for this release and the:

			Ingres Installation Guide

$ VMI$CALLBACK ASK ingres_ok "Display explanatory text prior to each question" "Yes" B
$ verbose=ingres_ok
$ RETURN

$ vms_version:
$ ok=1
$ vms_type=F$EXTRACT(0,F$LOCATE(",",VMI$VMS_VERSION),VMI$VMS_VERSION)
$ IF F$LOCATE("RELEASED",vms_type).EQ.F$LENGTH(vms_type)
$ THEN
$	VMI$CALLBACK MESSAGE W VMSNOTRELEASED "This version of OpenVMS is NOT a standard release."
$	VMI$CALLBACK MESSAGE W VMS_RELEASE "This version of OpenVMS is: ''vms_type'"
$	GOSUB press_return
$ ENDIF
$ vms_ver=F$EXTRACT(F$LOCATE(",",VMI$FULL_VMS_VERSION)+1,4,VMI$FULL_VMS_VERSION)
$ IF "''vms_ver'".GES."''min_vms'" THEN RETURN
$ ! version too low - reformat version to "human" form for error message
$ min_vms="V"+F$EXTRACT(0,2,min_vms)+"."+F$EXTRACT(2,2,min_vms)
$ VMI$CALLBACK MESSAGE W VMS_RELEASE "This version of OpenVMS is: ''vms_ver'"
$ VMI$CALLBACK MESSAGE W VERSION -
	"This product requires OpenVMS version ''min_vms' or later."
$ ok=0
$ RETURN

	Minimum disk space is both savesets and some scratch (say 100 blocks)
	on the system disk - other space requirements unknown.

$ disk_space:
$ ! Make sure there is enough free disk space in the working directory
$ t=F$GETDVI("VMI$KWD","FREEBLOCKS")
$ IF (t.LT.min_space)
$ THEN
$	tstr=F$GETDVI("VMI$KWD","FULLDEVNAM")
$       VMI$CALLBACK MESSAGE W NOSPACE -
                "''min_space' disk blocks are required on ''tstr'"
$       VMI$CALLBACK MESSAGE W NOSPACE -
                "Use the VMSINSTAL AWD option or free some disk space"
$       ok=0
$       RETURN
$ ENDIF
$ ok=1
$ RETURN


$ opening_messages:
$ ok=1
$ IF .NOT.verbose
$ THEN
$	VMI$CALLBACK MESSAGE I SKIP "Skipping display of background information"
$	IF vaxcluster
$		THEN
$	VMI$CALLBACK MESSAGE I SKIP "Skipping VAXCluster option information."
$		ENDIF
$	RETURN
$ ENDIF
$ TYPE SYS$INPUT

			   Ingres Installations

	A given system may have multiple, functionally independent INGRES
	configurations called "installations".  While more complete information
	on installations may be found in the Ingres Installation Guide, 
	the following may be helpful in making decisions while running this 
	procedure.

	Each installation has its own UNIQUE:

	1)	Directory tree (excepting CLIENT ONLY installations) pointed
		to by the II_SYSTEM logical name.

	2)	Group UIC code (usually the UIC of an INGRES account)

	3)	Installation ID (two letters with the first being unique)

	Multiple installations play an important role for most customers.
	It is helpful to consider multiple installations before beginning.

$ VMI$CALLBACK ASK ingres_ok "Do you have, or plan to have multiple installations" "Yes" B
$ multiple=ingres_ok
$ TYPE SYS$INPUT

				Required Information

	This procedure requires the following information:

	1)	A location to put the necessary files.  This may be an existing
		location, or it can be created by this procedure.

	2)	A File Owner UIC.  The UIC may remain unchanged or be reset for
		existing installations, or for new installations, may be either
		an existing UIC, or an account may be created by this procedure.

	3)	A List of packages to install.  The list may be all available
		packages or only selected packages.

$ GOSUB press_return
$ IF vaxcluster 
$ THEN
$	TYPE SYS$INPUT

				OpenVMS Cluster Nodes

	This node is a OpenVMS Cluster member.  OpenVMS Cluster nodes have 
	the most options available. Ingres installations on a OpenVMS 
	Cluster may:

	-	Serve local applications and databases only

	-	Use Ingres Networking in a Client/Server fashion

	-	Utilize the special OpenVMS Cluster features
		of the Ingres DBMS.

	When configuring this installation, you must be certain that you
	understand these options so you can choose what's best for you and
	have it operate properly.

$	GOSUB press_return
$ ENDIF

$ IF .NOT.multiple THEN RETURN
$ TYPE SYS$INPUT

			Accessing a Specific Installation

	Access to the appropriate installation is accomplished primarily
	through use of the appropriate logical names.  Each installation
	keeps its set in a logical name table.  "Production" installations
	use the LNM$SYSTEM table, and all others use their Group UICs'
	LNM$GROUP table.

		      Keeping Track of Multiple Installations

	It is ultimately your responsibility to maintain your installations
	and keep them separate.  However, this procedure will record each
	installation in a simple text file as it occurs.

	Prior installations may not have been logged, so a tool is provided
	which will help you populate and view the contents of this file.  The
	file is maintained on the system disk to be globally accessible, and
	its specification is: SYS$COMMON:[SYSEXE]INGRES_INSTALLATIONS.DAT. 
	The tool is II_SYSTEM:[INGRES.INSTALL]II_INSTALLS.COM.

$ GOSUB press_return
$ RETURN

$ existing:
$ IF verbose
$ THEN
$	TYPE SYS$INPUT

			Be Aware of Existing Installations

	Before defining the details of this installation, it is important to
	know as much as possible about the status and detail of previous
	installations.  This section provides an opportunity to discover other
	installations, and perform appropriate action.

$ ELSE
$	VMI$CALLBACK MESSAGE I HISTORY "Please be alert to previous installations."
$ ENDIF
$ ok=1
$ IF "''F$TRNLNM("ii_system")'".NES.""
$ THEN
$ !
$ !	NOTE:	The FIND_FILE callback aborts the installation if the directory
$ !		spec. is invalid... so we can't rely on it! But, F$SEARCH works!
$ !
$	ingres_str==F$SEARCH("ii_system:[ingres.install]release.dat")
$ !     Try Ingres release 
$	IF "''ingres_str'".EQS."" then ingres_str==F$SEARCH("ii_system:[ingres]version*.rel")
$	IF "''ingres_str'".EQS.""
$	THEN
$		VMI$CALLBACK MESSAGE I LOGICAL "The logical name II_SYSTEM is defined."
$		VMI$CALLBACK MESSAGE W NOVERSION "II_SYSTEM:[INGRES] may not contain an installation."
$		IF verbose
$		THEN
$			TYPE SYS$INPUT

			II_SYSTEM is Already Defined

	Please note that a logical name definition for II_SYSTEM was found
	indicating that a prior installation may exist. However the file
	RELEASE.DAT was not found indicating that this installation may not
	be valid.

$		ENDIF
$	ELSE
$		VMI$CALLBACK MESSAGE I FOUNDINSTALL "An installation was found in :"
$		IF verbose
$		THEN
$			TYPE SYS$INPUT

			     Installation Found

		Please note that there appears to be an
		installation located in II_SYSTEM:[INGRES].

$		ENDIF
$		upgrade=1
$	ENDIF
$	say "II_SYSTEM is defined as follows: "+cr+lf
$	SHOW LOGICAL/FULL ii_system
$	GOSUB press_return
$ ENDIF
$ VMI$CALLBACK FIND_FILE ingres_installs -
	SYS$COMMON:[SYSEXE]ingres_installations.dat "" S ingres_str
$ provide_history=0
$ IF "''ingres_str'".NES."S"
$ THEN
$	provide_history=1
$	VMI$CALLBACK MESSAGE W NOHISTORY "No Ingres installation history logfile found."
$	VMI$CALLBACK MESSAGE I HISTORYFILE "Installation history is recorded in SYS$COMMON:[SYSEXE]INGRES_INSTALLATIONS.DAT"
$	RETURN
$ ENDIF
$ file_name="SYS$COMMON:[SYSEXE]ingres_installations.dat"
$ t=0
$ GOSUB open_file_read
$ IF ok THEN GOTO read_installs
$ VMI$CALLBACK MESSAGE F OPENREADERR "Error opening the Ingres installation history logfile."
$ VMI$CALLBACK MESSAGE I HISTORYFILE "Installation history is recorded in SYS$COMMON:[SYSEXE]INGRES_INSTALLATIONS.DAT"
$ VMI$CALLBACK MESSAGE I CONTINUING "Attempting to continue anyway."
$ GOSUB close_file_read
$ RETURN

$ read_installs:	! t is an output line counter for "press_return" calls
$ t=7
$ read_hist_file:
$ GOSUB read_file
$ IF .NOT.ok THEN GOTO file_read_error
$ IF s.EQS."eof"
$ THEN
$	GOSUB close_file_read
$	say cr+lf+"________________________________________________________________________________"+cr+lf
$	IF t.NE.0 THEN GOSUB press_return
$	RETURN
$ ENDIF
$ t=t+1
$ say "''rec'"
$ IF t.EQ.21
$ THEN
$	GOSUB press_return
$	t=1
$ ENDIF
$ GOTO read_hist_file

$ owner:
$ IF verbose
$ THEN
$ 	TYPE SYS$INPUT

			    Installation Ownership

	An owner must be assigned to the files making up an installation.
	It is recommended that each installation has its own OpenVMS account.
	This account, if it exists, is generally known as "the INGRES
	account".  In any event, the GROUP portion of the ownership UIC MUST
	BE UNIQUE from all other INGRES installations.  This is because the
	LNM$GROUP logical name table is used for each installations logicals.
$ 	ENDIF
$ owner_menu:
$ TYPE SYS$INPUT

	Please choose how you will define the Ownership of this installation:

	1)	Provide the USERNAME of the INGRES account for this install.
		If it does not exist, it can be created by this procedure.

	2)	Provide the Owner UIC, or Rights Identifier. It must exist.

	3)	For existing installations only, retain the current owner.

$ create_account=0
$ VMI$CALLBACK ASK ingres_int "Your choice please" "1" DI
$ IF  (ingres_int.lt.1).OR.(ingres_int.GT.3)
$ THEN
$	VMI$CALLBACK MESSAGE E "Your choice must be 1, 2, or 3."
$	GOTO owner_menu
$ ENDIF
$ username=""		! Invalidate the username
$ IF ingres_int.EQ.3	! chose existing installation, keep the old UIC
$ THEN
$	VMI$CALLBACK MESSAGE W MUSTEXIST "The location chosen must already exist."
$	force_existence=1	!without a UIC the tree must already exist.
$	uic="PARENT"	! the existing directory owner will own the installation
$	ok=1
$	RETURN
$ ENDIF
$ force_existence=0 ! the location specified later does not have to exist yet
$ IF ingres_int.EQ.1
$ THEN
$	VMI$CALLBACK ASK ingres_str "Username" "INGRES" SZ
$	IF "''ingres_str'".EQS."^Z" THEN GOTO control_z
$	username=ingres_str
$	SET DEFAULT SYS$SYSTEM
$	DEFINE/USER SYS$OUTPUT VMI$KWD:authorization.data
$	SET NOON
$	MCR AUTHORIZE SHOW/BRIEF 'username
$	s=$STATUS
$       SET ON
$ 	SET DEFAULT VMI$KWD:
$	IF "''s'".EQS."%X00000001"	! if status is success then...
$	THEN
$		VMI$CALLBACK MESSAGE I ACNTEXISTS "The specified account exists."
$		file_name="VMI$KWD:authorization.data"
$		GOSUB open_file_read
$		IF .NOT.ok THEN GOTO file_open_read_error
$		GOSUB read_w_close	!read heading
$		IF .NOT.ok THEN GOTO file_read_error
$		GOSUB read_w_close	! read blank line
$		IF .NOT.ok THEN GOTO file_read_error
$		GOSUB read_w_close	!read user data
$		IF .NOT.ok THEN GOTO file_read_error
$		GOSUB close_file_read
$		IF .NOT.ok THEN GOTO close_file_read_error
$		VMI$CALLBACK DELETE_FILE 'file_name'
$		uic=F$EXTRACT(F$LOCATE("[",rec),(F$LOCATE("]",rec)-F$LOCATE("[",rec)+1),rec)
$		rec=F$EDIT(rec,"COMPRESS")
$		parse_acnt:
$		IF F$LOCATE(" ",rec).NE.F$LENGTH(rec)
$		THEN
$			rec=F$EXTRACT(F$LOCATE(" ",rec)+1,F$LENGTH(rec),rec)
$			GOTO parse_acnt
$		ENDIF
$		IF F$LOCATE(".INGRES]",rec).NE.F$LENGTH(rec) THEN rec=F$EXTRACT(0,F$LOCATE(".INGRES]",rec),rec)+"]"
$		IF F$LOCATE("[INGRES]",rec).NE.F$LENGTH(rec) THEN rec=F$EXTRACT(0,F$LOCATE("[INGRES]",rec),rec)
$		tstr=rec	! the above removes [ingres] from being the dir
$		GOSUB parse_location
$		ok=1
$		RETURN
$	ENDIF	! Drop through if the account DOESN'T exist
$	VMI$CALLBACK MESSAGE I NOACNT "The specified account does not exist."
$	VMI$CALLBACK ASK ingres_ok "Do you want it created by this procedure" "Yes" BD
$	IF .NOT.ingres_ok THEN GOTO owner_menu
$	IF verbose
$	THEN
$		TYPE SYS$INPUT

	The Username specified will be added to this systems UAF file, but
	first, the UIC must be specified. Please consider the following:

	1)	The UIC GROUP specification should not belong to the SYSTEM
		group, defined as any group number less than or equal to
		the SYSGEN parameter MAXSYSGROUP.

	2)	The UIC GROUP specification should be unique among INGRES
		installations within this security domain.

	The default disk and directory will be defined as II_SYSTEM:[INGRES].
	If you wish otherwise, you may change it by using AUTHORIZE later.

$	ELSE
$		VMI$CALLBACK MESSAGE I DEFAULT "The default disk and directory will be II_SYSTEM:[INGRES]."
$	ENDIF
$	create_account=1
$	GOTO ask_uic
$ ENDIF
$ ! IF ingres_int.EQ.2 this is the only option left!
$ IF verbose
$ THEN
$	TYPE SYS$INPUT

	You may provide the UIC in either the [group,member] UIC syntax,
	or by specifying the identifier associated with it. Please do not
	specify a "SYSTEM UIC", as defined by OpenVMS.

$ ENDIF
$ ask_uic:
$ VMI$CALLBACK ASK ingres_str "Please specify the ownership UIC" "No Default" SZ
$ IF "''ingres_str'".EQS."^Z" THEN GOTO control_z
$ IF F$EDIT(ingres_str,"UPCASE").EQS."NO DEFAULT"
$ THEN 
$	VMI$CALLBACK MESSAGE W NODEFAULT "No default UIC can be provided."
$	GOTO owner_menu
$ ENDIF
$ ! Perform some simple tests and checks on the uic specified.
$ IF F$LOCATE("[",ingres_str).NE.F$LENGTH(ingres_str) .AND. create_account.NE.1 
$ THEN
$       tstr=F$EXTRACT(F$LOCATE("[",ingres_str)+1, F$LENGTH(ingres_str), ingres_str)
$       IF F$LOCATE("]",tstr).EQ.F$LENGTH(tstr) THEN GOTO invalid_uic
$       tstr=F$EXTRACT(0,F$LOCATE("]",tstr), tstr)
$       ! Check/adjust for a bracketed identifier, i.e. [INGRES]
$	tstr2 = F$EXTRACT(0, 1, tstr)
$       IF F$TYPE(tstr2).EQS."STRING" THEN ingres_str==tstr
$ ENDIF
$ uic=ingres_str
$ IF F$LOCATE("[",uic).NE.F$LENGTH(uic) THEN GOTO numeric_uic
$ ! Does the text for the uic translates to a valid identifier?
$ IF F$IDENTIFIER(ingres_str,"NAME_TO_NUMBER").NE.0 
$ THEN 
$	! Can't use an existing identifier if we need to create a new account
$ 	IF create_account.EQ.1
$	THEN
$		VMI$CALLBACK MESSAGE E BADFORMAT "Use the standard numeric UIC format."
$		VMI$CALLBACK MESSAGE I NOIDENT "Identifiers are not allowed."
$		GOTO ask_uic
$	ENDIF
$	uic=F$FAO("!%U", F$IDENTIFIER(ingres_str,"NAME_TO_NUMBER") )
$	GOTO valid_uic
$ ENDIF
$ invalid_uic:
$ VMI$CALLBACK MESSAGE W INVALIDUIC "The UIC specified, ''uic', is invalid."
$ GOTO owner_menu
			NOTES ON UICs:

	[group,member] 		=	identifier

There must always be a "numeric uic", but not always a matching identifier.
Identifiers are provided as a convenient way to handle UICs, but are optional.

We want to check that the group is valid, but only complete UICs can be checked.

We check validity through use of F$IDENTIFIER: If it returns 0 or "", the UIC
is invalid.

To check a "numeric" uic, one that is in brackets, we must convert it to
a LONGWORD value that contains both group and member.

Remember that UICs are IN OCTAL!! 

$ numeric_uic:
$ uic_group=F$EXTRACT(F$LOCATE("[",ingres_str)+1,(F$LOCATE(",",ingres_str)-F$LOCATE("[",ingres_str))-1,ingres_str)
$ !Cannot use group numbers 1 to 8 which are reserved for SYSTEM usage
$ IF F$INTEGER(uic_group).LE.8
$ THEN
$ 	VMI$CALLBACK MESSAGE E SYSTEMUIC "Group numbers 1-8 reserved for SYSTEM."
$ 	GOTO owner_menu
$ ENDIF
$ uic_member=F$EXTRACT(F$LOCATE(",",ingres_str)+1,(F$LOCATE("]",ingres_str)-F$LOCATE(",",ingres_str))-1,ingres_str)
$ uic_lw=(F$INTEGER("%O''uic_member'")+(%X10000*F$INTEGER("%O''uic_group'")))
$ IF create_account.EQ.1
$ THEN
$ !      Creating a new account...
$ 	IF "''F$IDENTIFIER(uic_lw,"NUMBER_TO_NAME")'".NES.""
$ 	THEN	
$ 		VMI$CALLBACK MESSAGE E DUPUIC "UIC needs to be unique."
$ 		GOTO owner_menu
$ 	ENDIF
$ ELSE
$ !      Naming an existing account...
$ 	IF "''F$IDENTIFIER(uic_lw,"NUMBER_TO_NAME")'".EQS."" THEN GOTO INVALID_UIC
$ ENDIF
$ uic="[''uic_group',''uic_member']"
$ VMI$CALLBACK GET_SYSTEM_PARAMETER ingres_str MAXSYSGROUP 
$ IF F$INTEGER(uic_group).LE.F$INTEGER(ingres_str)
$ THEN
$	VMI$CALLBACK MESSAGE W SYSTEMUIC "The group you specified is a SYSTEM UIC"
$	VMI$CALLBACK MESSAGE W SYSTEMUIC "The SYSGEN parameter MAXSYSGROUP is defined as ''ingres_str'"
$	VMI$CALLBACK ASK ingres_ok "Are you sure you wish to use this UIC" "Yes" BD
$	IF .NOT.ingres_ok THEN GOTO owner_menu
$ ENDIF
$ valid_uic:
$ VMI$CALLBACK MESSAGE I VALIDUIC "The UIC ''uic' is valid."
$ IF create_account THEN VMI$CALLBACK MESSAGE I CREATEACNT "The Username ''username' will be added to the UAF."
$ ok=1
$ RETURN

Remember: an INSTALLATION is defined as each being unique: II_SYSTEM,
II_INSTALLATION, and the associated UIC/Production... but here we only care
about location...

$ where:
$ ok=1
$ IF verbose
$ THEN
$	TYPE SYS$INPUT

			Disk and Directory Location
			   for this Installation

	This procedure requires that you provide a location which will contain
	the [INGRES] directory and its subdirectories, Ingres executable
	images, and miscellaneous configuration files and command procedures.

	This location is known as II_SYSTEM: and is a device specification.
	By defining II_SYSTEM as a "rooted directory," the [INGRES] directory
	may be located within another directory. An example is the OpenVMS 
	logical SYS$SYSROOT, which is typically defined as 
	SYS$SYSDEVICE:[SYS0.]

	Please provide the appropriate device and directory specification
	within which the directory [INGRES] will reside. Note that while logical
	names may be provided now, their translation into a real device and
	optional directory specification will be coded into the definition of
	II_SYSTEM used for Ingres startup; they themselves will not be used.

$ENDIF
$ ii_system=""
$ ask_location:
$ ! ii_dev is of the format $disk1:
$ ! ii_root is of the format [user.ingres.]
$ IF "''ii_dev'''ii_dir'".NES.""
$ THEN
$	tstr=ii_dev+ii_dir 
$ ELSE
$	tstr="No Default"
$	IF "''F$TRNLNM("II_SYSTEM")'".NES."" THEN tstr="''F$TRNLNM("II_SYSTEM")'"
$ ENDIF
$ VMI$CALLBACK ASK ingres_str "Location for this installation" "''tstr'" DSZ
$ IF "''ingres_str'".EQS."^Z" THEN GOTO control_z
$ IF "''F$EDIT(ingres_str,"UPCASE")'".EQS."NO DEFAULT"
$ THEN
$	VMI$CALLBACK MESSAGE E NODEFAULT "No default can be provided."
$	GOTO ask_location
$ ENDIF
$ tstr=F$EDIT(ingres_str,"COLLAPSE,UPCASE") !no spaces please
$ input_ii_system=tstr
$ create_location=0
$ GOSUB parse_location
$ IF .NOT.ok THEN GOTO bad_location
$ GOSUB test_location
$ tstr=ii_dev+ii_dir
$ IF .NOT.ok	! this is from test_location
$ THEN
$	VMI$CALLBACK MESSAGE I NOLOCATION "The location ''ii_dev'''ii_dir' does not exist"     
$	IF force_existence
$	THEN
$		VMI$CALLBACK MESSAGE W NOOWNER "When UIC is unspecified, the location must already exist."
$		GOTO ask_location
$	ENDIF
$	VMI$CALLBACK ASK ingres_ok "Create ''tstr'" "Yes" BD
$	IF .NOT.ingres_ok THEN GOTO ask_location
$	create_location=1      
$ ELSE
$	VMI$CALLBACK MESSAGE I EXISTS "Location ''tstr' exists."
$	ingres_str==F$SEARCH(ii_root+"[ingres.install]release.dat")
$	IF "''ingres_str'".EQS."" then ingres_str==F$SEARCH("ii_system:[ingres]version*.rel")
$	IF "''ingres_str'".NES.""
$	THEN
$		VMI$CALLBACK MESSAGE I VERSIONFOUND  "An INGRES version file was found in this location."
$		VMI$CALLBACK MESSAGE I EXISTING_INSTAL "This location may contain an existing installation."
$	ENDIF
$	VMI$CALLBACK ASK ingres_ok "Confirm use of location ''tstr'" "Yes" BD 
$	IF .NOT.ingres_ok THEN GOTO ask_location
$ ENDIF
$ GOSUB check_location_space
$ IF .NOT.ok THEN GOTO ask_location
$ ii_system=ii_dev+ii_root
$ DEFINE/JOB/EXEC/TRANSLATION=(CONCEALED) II_SYSTEM 'ii_system      
$ VMI$CALLBACK MESSAGE S SELECTED "Location ''tstr' was selected." 
$ VMI$CALLBACK MESSAGE S DEFINED "II_SYSTEM is defined as ''ii_system'"
$ IF "''F$SEARCH("II_SYSTEM:[000000]INGRES.DIR")'" .EQS. "" 
$ THEN 
$!
$!    Create the [.ingres] directory if it doesn't exist
$! 
$    tstr=ii_dev+f$extract(0,f$length(ii_dir)-1,ii_dir)+".INGRES]"
$    VMI$CALLBACK MESSAGE I CREATEDIR "Creating the [INGRES] directory ''tstr'"
$    VMI$CALLBACK CREATE_DIRECTORY USER "''tstr'" "/NOLOG/OWNER=''uic'/PROTECTION=(S:RWED,O:RWED,G:RE,W:R)"
$    tstr=ii_dev+f$extract(0,f$length(ii_dir)-1,ii_dir)+".INGRES]"
$    IF .NOT.ok
$    THEN
$	VMI$CALLBACK MESSAGE W NOTCREATED "Unable to create ''ii_system':[INGRES]"
$	VMI$CALLBACK MESSAGE W NOLOCATION "Location ''tstr' was not created."
$	VMI$CALLBACK MESSAGE E INVALID "The location information appears to be invalid."
$	VMI$CALLBACK MESSAGE I DEVICE "The device was parsed as ''ii_dev'"
$	VMI$CALLBACK MESSAGE I DIRECTORY "The directory was parsed as ''ii_dir'"
$	IF verbose
$	THEN
$		TYPE SYS$INPUT

The location you specify must follow these guidelines:

	-	It must be syntactically correct to OpenVMS DCL standards.
	-	The disk you specify must be online and accessible at this time.
	-	A disk specification must be included.
	-	A directory specification is optional.
	-	Logical names may be "concealed" or "rooted."

$	ENDIF
$	GOTO fail_exit
$    ELSE
$	VMI$CALLBACK MESSAGE S CREATED "Created location ''tstr'"
$    ENDIF
$ ELSE
$    SET FILE/OWNER_UIC='uic' II_SYSTEM:[000000]INGRES.DIR
$ ENDIF
$ ok=1
$ RETURN

$ bad_location:
$ VMI$CALLBACK MESSAGE E INVALID "The location information appears to be invalid."
$ VMI$CALLBACK MESSAGE I INVALID "Your input was ''ingres_str'"
$ IF "''ii_dev'".NES."" THEN VMI$CALLBACK MESSAGE I DEVICE "The device was parsed as ''ii_dev'"
$ IF "''ii_dir'".NES."" THEN VMI$CALLBACK MESSAGE I DIRECTORY "The directory was parsed as ''ii_dir'"
$ IF verbose
$ THEN
$	TYPE SYS$INPUT

	The location you specify must follow these guidelines:

	-	It must be syntactically correct to OpenVMS DCL standards.
	-	The disk you specify must be online and accessible at this time.
	-	A disk specification must be included.
	-	A directory specification is optional.
	-	Logical names may be "concealed" or "rooted" and will be
		translated to their existing values. They will NOT be included
		in the final definition of II_SYSTEM.

$ ENDIF
$ GOTO ask_location


Now have II_SYSTEM, it exists... Determine the username of the owner

$ getuser:
$ ok=1
$ IF "''username'".NES."" 	! Routine is unnecessary if we already got it
$ THEN
$ 	VMI$CALLBACK MESSAGE I GETUSER -
		"Installation owner - Username: ''username', UIC: ''uic'"
$ 	RETURN
$ ENDIF
$ !
$ !At this point we only know the UIC. Translate this into a username.
$ !Our goal is to translate the UIC to the form [numeric_group,numeric_member]
$ t=F$LOCATE(",", "''uic'" )
$ IF "''t'" .EQS. F$LENGTH("''uic'")
$ THEN
$	! This is a UIC of the form "[identifier]", Get identifier string.
$ 	uic_member=F$EXTRACT(1, F$LENGTH("''uic'") - 2, "''uic'")
$ ELSE
$	! This is a UIC of the form "[group,member]".  Get the member.
$	tstr=F$EXTRACT("''t'" + 1, F$LENGTH("''uic'"), "''uic'")
$	uic_member=F$EXTRACT(0, F$LENGTH("''tstr'") - 1, "''tstr'")
$ ENDIF
$ !Translate the uic_member to a numeric UIC.
$ IF F$TYPE(uic_member).EQS."STRING"
$ THEN
$	uic_lw=F$IDENTIFIER( uic_member, "NAME_TO_NUMBER")
$	IF "''uic_lw'" .EQS. "0" THEN GOTO getuser_error
$	uic = F$FAO("!%U", uic_lw)
$ ENDIF
$ !
$ !Now use AUTHORIZE to translate the UIC into a username.
$ SET DEFAULT SYS$SYSTEM
$ file_name="VMI$KWD:authorization.data"
$ DEFINE/USER SYS$OUTPUT 'file_name'
$ SET NOON
$ MCR AUTHORIZE SHOW/BRIEF 'uic'
$ s=$STATUS
$ SET ON
$ SET DEFAULT VMI$KWD:
$ IF "''s'"	! if status is success then...
$ THEN
$	GOSUB open_file_read
$	IF .NOT.ok THEN GOTO getuser_error
$	GOSUB read_w_close	!read heading
$	IF .NOT.ok THEN GOTO getuser_error_close
$	GOSUB read_w_close	!read blank line
$	IF .NOT.ok THEN GOTO getuser_error_close
$	GOSUB read_w_close	!read user data
$	IF .NOT.ok THEN GOTO getuser_error_close
$	GOSUB close_file_read
$	IF .NOT.ok THEN GOTO getuser_error
$	VMI$CALLBACK DELETE_FILE 'file_name'
$	!Parse the username from the authorize data
$	rec=F$EDIT(F$EXTRACT(0, F$LOCATE("[",rec) - 1, rec), "COMPRESS,TRIM")
$	t=0			!The username will be the last word of rec
$	parse_user:
$		tstr=F$ELEMENT(t," ",rec)
$		IF "''tstr'" .EQS. " " THEN GOTO parse_user_end 
$		username=tstr
$		t=t+1
$		GOTO parse_user
$	parse_user_end:
$ 	VMI$CALLBACK MESSAGE I GETUSER -
		"Installation owner is Username: ''username', UIC: ''uic'"
$	RETURN
$ ENDIF	
$ ! Handle errors in the username lookup.  It is possible that a UIC may
$ ! not correspond to a username.  We can not use these UIC-s.
$ getuser_error:
$ ok=0
$ VMI$CALLBACK MESSAGE F GETUSER "Unable to determine USERNAME of UIC ''uic'"
$ RETURN

$ getuser_error_close:
$ GOSUB close_file_read
$ GOTO getuser_error

Now have II_SYSTEM and the owners username... get list of products to install...

$ auth_string:
$ !
$ ! Now provide the files...
$ !
$!
$ IF verbose
$ THEN
$	TYPE SYS$INPUT

			Choose which Packages to Install
$ ENDIF
$ OK = 1
$ RETURN
$!
$ choose_packages:
$ VMI$CALLBACK ASK ingres_ok -
  "Would you like to perform a package install (not customized)" "Yes" BD
$ IF ingres_ok
$ THEN
$    opt = "-packages"
$    tstr = ""
$    custom = 0
$ ELSE
$    opt = "-custom"
$    tstr = "Custom "
$    custom = 1
$ ENDIF
$ VMI$CALLBACK MESSAGE I PACKAGES "Now retrieving listing of ''tstr'packages."
$ file_name = "VMI$KWD:packages.dat"
$ imt 'opt' -output='file_name
$choice_menu:
$ tint=3
$ IF custom
$ THEN
$  TYPE SYS$INPUT

		You have the following options:

		1)	Display all packages and return to this menu.

		2)	Describe selected packages, and return to this menu.

		3)	Install only selected packages

		4)	Install ALL packages (not recommended)

$ ELSE
$  ! Package install -- allow only one package to be chosen.
$  TYPE sys$input

		You have the following options:

		1)	Display all packages and return to this menu.

		2)	Describe selected package, and return to this menu.

		3)	Install selected package.


	NOTE: Only 1 package can be selected. 


$ ENDIF
$ VMI$CALLBACK ASK ingres_int "Your choice please" "''tint'" DI
$ say ""
$ choice=F$ELEMENT(ingres_int-1,",",package_menu_options)

$ !invalid choice
$ IF ("''choice'".EQS.",") -
    .OR. ( .NOT.custom .AND. "all" .EQS. "''choice'" ) -
    .OR. ( .NOT.custom .AND. "authorized" .EQS. "''choice'" )
$ THEN
$	VMI$CALLBACK MESSAGE E NOTVALID "Invalid input. Please choose the appropriate integer."
$	GOTO choice_menu
$ ENDIF

$ IF "''choice'".EQS."display"
$ THEN
$	TYPE/PAGE VMI$KWD:packages.dat
$	say ""
$	GOSUB press_return
$	GOTO choice_menu
$ ENDIF

$ IF "''choice'".EQS."describe"
$ THEN
$	GOSUB package_loop
$	GOSUB press_return
$	GOTO choice_menu
$ ENDIF

$ file_name="VMI$KWD:chosen_packages.dat"
$ GOSUB open_file_write
$ IF .NOT.ok THEN GOTO file_open_write_error
$ rec=cr+lf+"You have selected the following packages to be installed:"
$ GOSUB write_file
$ IF .NOT.ok THEN GOTO file_write_error
$ rec=cr+lf+"	Feature name      Package"
$ GOSUB write_file
$ IF .NOT.ok THEN GOTO file_write_error
$ rec="	----------------  ------------------------------"
$ GOSUB write_file
$ IF .NOT.ok THEN GOTO file_write_error
$ GOSUB package_loop
$!
$! Package installation:  user selected more than one package...
$ IF .NOT.custom .AND. f$LOCATE( ",", package_list ) .ne. f$LENGTH( package_list )
$ THEN
$    VMI$CALLBACK MESSAGE F MULTPACKAGE "You have selected more than one package."
$    VMI$CALLBACK ASK ingres_str "Exit, or continue" "continue" SZ
$    IF "''ingres_str'".NES."CONTINUE" THEN GOTO control_z
$    goto choice_menu
$ ENDIF
$!
$! Custom or package:  nothing to do! 
$ IF "''package_list'".EQS.""
$ THEN
$    IF "''choice'".EQS."all"
$    THEN
$	VMI$CALLBACK MESSAGE F NOPRODUCTS "All Packages selected, but none provided."
$	ok=0
$	RETURN
$    ENDIF
$    VMI$CALLBACK MESSAGE F NOPRODUCTS "No products selected for installation."
$    VMI$CALLBACK ASK ingres_str "Exit, or continue" "continue" SZ
$    IF "''ingres_str'".NES."CONTINUE" THEN GOTO control_z
$!
$! Custom or package:  looks okay; make sure this is what the user wants...
$ ELSE
$    TYPE/PAGE VMI$KWD:chosen_packages.dat
$    VMI$CALLBACK DELETE_FILE VMI$KWD:chosen_packages.dat
$    VMI$CALLBACK ASK ingres_ok "Please confirm" "Yes" BD
$    IF ingres_ok
$    THEN
$	ok=1
$	RETURN
$    ENDIF
$ ENDIF
$ goto choice_menu

$ package_loop:
$ package_list=""
$ file_name="VMI$KWD:packages.dat"
$ GOSUB open_file_read	!REMEMBER: the fun begins on line 3...
$ IF .NOT.ok THEN GOTO file_open_read_error
$ gosub read_w_close	!	Read header line
$ IF .NOT.ok THEN GOTO pkg_file_error
$ gosub read_w_close	!	Read underline for header
$ IF .NOT.ok THEN GOTO pkg_file_error
$ IF (choice.EQS."describe").OR.(choice.EQS."selected")
$ THEN
$	TYPE SYS$INPUT

Please select from the following list by responding to each prompt.

	Feature name      Package
	----------------  ------------------------------

$ ENDIF
$ nxt_pkg:
$ tint=0	! this time tint=true/false for selecting a specific package
$ gosub read_w_close
$ IF .NOT.ok
$ THEN
$	IF s.EQS."eof"
$		THEN
$		IF ("''choice'".NES."display").AND.("''choice'".NES."describe") THEN GOSUB close_file_write
$! (4-feb-1997, boama01) INSTALL pkg now invisible; always add it as 1st pkg
$!   to a custom selected package list:
$		IF custom -
  .AND.("''choice'".NES."display").AND.("''choice'".NES."describe")-
  .AND.("''package_list'".NES."") THEN package_list="INSTALL,"+package_list
$		RETURN
$		ENDIF
$	GOTO pkg_file_error
$ ENDIF
$ pkg=F$EXTRACT(1,F$LENGTH(rec),F$EDIT(rec,"COMPRESS"))
$ pkg=F$EXTRACT(0,F$LOCATE(" ",pkg),pkg)
$ IF (choice.EQS."describe").OR.(choice.EQS."selected")
$ THEN
$! (4-feb-1997, boama01) INSTALL pkg now invisible; commented special case...
$!	! Don't allow the "install" package to be de-selected
$!	IF (choice.EQS."selected").and.(pkg.eqs."install")
$!	THEN 
$!		ingres_ok == "Yes"
$!	ELSE
$		say cr+lf+rec
$		tstr="YES"	! set default to whether or not it's licensed
$		VMI$CALLBACK ASK ingres_ok "Select this package" "''tstr'" BD
$!	ENDIF
$	IF .not.ingres_ok THEN GOTO nxt_pkg	! this saves an extra IF
$	IF "''choice'".EQS."describe"
$	THEN
$		say ""
$		imt -describe='pkg
$	ELSE
$		tint=1	! this package was selected
$	ENDIF
$ ELSE ! choice was either all or authorized
$		tint=1
$ ENDIF
$ IF .NOT.tint THEN GOTO nxt_pkg
$ IF "''package_list'".NES."" THEN package_list=package_list+","
$ package_list=package_list+pkg
$ GOSUB write_file	! save for confirmation purposes
$ IF .NOT.ok THEN GOTO file_write_error
$ GOTO nxt_pkg

$ pkg_file_error:
$ VMI$CALLBACK F PKGFILEERR "File holding packages to install has a fatal error"
$ goto fail_exit

Now have all products in PACKAGE_LIST, so we can pass it off to IMT and get
a list to move...

NOTE:	We are dependent on the RESOLVE list being in EXACTLY the proper
	format... which is:

	|[complete_directory_path_from_II_SYSTEM] file_name.ext|

	Note, the only space is the one BETWEEN the directory and filename.


Then create "script files" and get set for MOVEFILES phase.
Note that this code is written to allow creation of many script files generated
from the RESOLVE list. The record is changed for each type of output by
"GOSUB"ing to the place where the record is "built" - on a record in, record
out basis.

$ resolve:
$ ok=1
$ tstr=F$EDIT(package_list,"COLLAPSE") ! remove all spaces/tabs, if any
$ VMI$CALLBACK MESSAGE I RESOLVE "Resolving a unique file list for this installation."
$ file_name="VMI$KWD:file_list.dat"
$ imt -resolve='tstr -output='file_name -loc=VMI$KWD:
$ file_name="VMI$KWD:directory_list.dat"
$ imt -directories='tstr -output='file_name -loc=VMI$KWD:
$ file_name="VMI$KWD:permission_list.dat"
$ imt -permission='tstr -output='file_name -loc=VMI$KWD:
$ !               
$ RETURN

$ confirm:
$ IF verbose
$ THEN
$	TYPE SYS$INPUT

By convention, the last step of an installation is to execute an Installation
Verification Procedure, or IVP, if the installer so desires.

The Ingres IVP verifies proper installation by performing the setup and
configuration steps, which are required before the product can be used. Of
course, these steps can be performed independently, after the installation is
complete. 

				PLEASE NOTE:

	If you elect to run the Ingres IVP, you must be prepared to perform
	all of the setup and configuration phases. This requires decisions
	to be made about how Ingres will be used. Usually, changes cannot 
	be made while the installation is up and running. Ingres Corporation
	STRONGLY RECOMMENDS reading the Ingres Installation Guide prior 
	to this step.

		PLEASE DO NOT PERFORM THE IVP UNLESS YOU
		ARE TRULY READY TO CONFIGURE Ingres.

$ ENDIF
$ VMI$CALLBACK ASK ingres_ok "Do you want to run the configuration IVP following this installation" -
	"Yes" B
$ IF ingres_ok
$ THEN
$	ivp=1
$	VMI$CALLBACK SET IVP YES
$ ELSE
$	ivp=0
$	VMI$CALLBACK SET IVP NO
$ ENDIF
$ say cr+lf+"                            Installation Summary"+cr+lf
$ say "                            ''release'"
$ say "Location:                       ''ii_dev'''ii_dir'"
$ tstr="Exists."
$ IF create_location THEN tstr="Will be created."
$ say "This location:                  ''tstr'"
$ say ""
$ say "Owner UIC:                      ''uic'"
$ IF create_account THEN say "Owner account:                  ''username' will be created."
$ say ""
$ IF "''choice'".EQS."all" THEN say "Packages selected:              All"
$ IF "''choice'".EQS."authorized"
$ THEN
$	say "Packages Selected:              All authorized"
$ ENDIF
$ IF "''choice'".EQS."selected"
$ THEN
$	tstr="Packages Selected:              ''package_list'"
$	cut_pkg_lst:
$	IF F$LENGTH(tstr).GT.80
$		THEN
$		say "''F$EXTRACT(0,79,tstr)'"
$		tstr="                                "+F$EXTRACT(79,F$LENGTH(tstr),tstr)
$		GOTO cut_pkg_lst
$		ELSE
$		say "''tstr'"
$		ENDIF
$ ENDIF
$ say ""
$ tstr=" not"
$ IF ivp THEN tstr=""
$ say "Configuration IVP:              Will''tstr' be performed."
$ say ""+cr+lf
$ VMI$CALLBACK ASK ingres_ok "Do you want to continue" "Yes" BD
$ say ""
$ ok=ingres_ok
$ IF ok
$ THEN
$	IF "''ivp'".eqs."0"
$	THEN
$		VMI$CALLBACK MESSAGE I NOMOREQ  "There are no more questions."   
$	ELSE
$		VMI$CALLBACK MESSAGE I NOMOREQ  "There are no more questions until the Ingres IVP."
$	ENDIF
$	VMI$CALLBACK MESSAGE I CONTINUE "The Installation will now continue for some time."
$ ENDIF
$ RETURN

The following section is run after confirmation because it can take some time
to complete and we don't want to take up the installers time...

$ build_scripts:
$ VMI$CALLBACK MESSAGE I SCRIPT "Creating scripts tailored for this installation."

$ create_record="provide_file_record"
$ file_name="VMI$KWD:provide_file.dat"
$ GOSUB create_script   ! if we return at all, script is OK, and files closed.


$ create_record="set_prot_record"
$ file_name="VMI$KWD:set_prot.com"
$ GOSUB create_script

$ create_record="purge_files_record"
$ file_name="VMI$KWD:purge_files.com"
$ GOSUB create_script

$ IF verbose
$ THEN
$	ingres_str==F$SEARCH("ii_system:[ingres.install]release.dat")
$	IF "''ingres_str'".EQS."" then ingres_str==F$SEARCH("ii_system:[ingres]version*.rel")
$       IF "''ingres_str'".NES.""
$               THEN
$               TYPE SYS$INPUT

	Neither RELEASE.DAT nor VERSION*.REL file was found, suggesting that
	an installation already exists in this directory tree. Because of
	this, there is a possibility that this installation will replace
	existing files. This routine will NOT purge replaced files.
$ 		ENDIF
$	TYPE SYS$INPUT

	For your convenience, the following script is provided to remove any
	files replaced by this installation:

		II_SYSTEM:[INGRES.UTILITY]PURGE_FILES.COM

	Be sure to run it only after a SYSTEM reboot, before Ingres is 
	started or after deinstalling the images with the OpenVMS INSTALL 
	utility.

$ ELSE
$	VMI$CALLBACK MESSAGE I PURGEFILES "II_SYSTEM:[INGRES.UTILITY]PURGE_FILES.COM will purge replaced files."
$	VMI$CALLBACK MESSAGE W PURGEFILES "Warning: Only purge when following a SYSTEM reboot.""
$	VMI$CALLBACK MESSAGE W PURGEFILES "Or after deinstalling the images with the VMS INSTALL utility."
$ ENDIF
$ ok=1
$ RETURN

All the files to be provided are described in the proper format in the
file VMI$KWD:PROVIDE_FILE.DAT and are moved en-masse to the target locations.

All PROVIDE_FILE callbacks must be done before the SET PURGE.

$ do_it:
$ VMI$CALLBACK SECURE_FILE "II_SYSTEM:[000000]INGRES.DIR" "''uic'"
$ VMI$CALLBACK MESSAGE I CREATEDIR "Now creating [INGRES] subdirectories, as required."
$ VMI$CALLBACK MESSAGE W SECURITY "Ownership and protection will be reset to INGRES defaults."
$ IF verbose THEN say cr+lf+"You may ignore 'already exists' messages..."+cr+lf
$ file_name="vmi$kwd:directory_list.dat"
$ GOSUB open_file_read
$ IF .NOT.ok THEN GOTO file_open_read_error
$ next_directory:
$ GOSUB read_w_close
$ IF .NOT.ok
$ THEN
$	IF "''s'".EQS."eof"
$		THEN
$		GOTO directories_ready
$		ELSE
$		GOTO file_read_error
$		ENDIF
$ ENDIF
$ tstr=ii_dev+ii_root+rec
$ VMI$CALLBACK CREATE_DIRECTORY USER "''tstr'" "/NOLOG/OWNER=''uic'/PROTECTION=(S:RWED,O:RWED,G:RE,W:R)"
$ GOTO next_directory

$ directories_ready:
$ IF create_account
$ THEN
$	acnt_qualifiers=acnt_qualifiers+"/uic=''uic'"
$	acnt_qualifiers=acnt_qualifiers+"/device=''input_ii_system'"
$	VMI$CALLBACK CREATE_ACCOUNT "''username'" "''acnt_qualifiers'"
$	s=$status
$	IF s.EQ.VMI$_SUCCESS
$		THEN
$		VMI$CALLBACK MESSAGE S CREATEACNT "The Username ''username' will be added to the UAF."
$		ELSE
$		VMI$CALLBACK MESSAGE F CANTCREATEACNT "Unable to create account ''username'."
$		VMI$CALLBACK MESSAGE F ERRSTATUS "Error status was ''s'"
$		VMI$CALLBACK MESSAGE I CONTINUE "Continuing installation."
$		ENDIF
$	VMI$CALLBACK UPDATE_ACCOUNT "''username'" "''update_qualifiers'"
$	s=$status
$	IF s.EQ.VMI$_SUCCESS
$		THEN
$		VMI$CALLBACK MESSAGE S UPDATEACNT "The Username ''username' will have its privileges updated in UAF."
$		ELSE
$		VMI$CALLBACK MESSAGE F CANTUPDATEACNT "Unable to update account ''username' privileges."
$		VMI$CALLBACK MESSAGE F ERRSTATUS "Error status was ''s'"
$		VMI$CALLBACK MESSAGE I CONTINUE "Continuing installation."
$		ENDIF
$ ENDIF

$ ! Earlier installations may have slutil.exe installed even if they are 
$ ! NOT SEVMS installation. Delete it, so it will not interfere with IVP_list
$ ! selection.
$ if f$search("ii_system:[ingres.utility]slutil.exe") .nes. ""
$  then 
$   delete/nolog ii_system:[ingres.utility]slutil.exe;* 
$ endif
$ tint=0
$ saveset_loop:
$ saveset=F$ELEMENT(tint,",","b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z")
$ VMI$CALLBACK RESTORE_SAVESET 'saveset
$ IF "''saveset'".eqs."''F$EDIT(last_saveset,"LOWERCASE")'" THEN GOTO savesets_done
$ tint=tint+1
$ GOTO saveset_loop
$
$ savesets_done:
$ VMI$CALLBACK PROVIDE_FILE "" provide_file.dat "" T
$
$ VMI$CALLBACK MESSAGE I MVFILES "Renaming non-unique file names..."
$ !Open the input file for reading
$ file_name="VMI$KWD:renamed.lst"
$ GOSUB open_file_read
$ IF .NOT.ok THEN GOTO file_open_read_error
$ !Parse the record
$ next_line:
$ GOSUB read_file
$ IF .NOT.ok THEN GOTO file_read_error
$ IF "''s'".EQS."eof" 
$ THEN
$	GOSUB close_all_files
$	GOTO end_rename
$ ENDIF
$ rec=F$EDIT(rec,"COMPRESS,TRIM,UNCOMMENT,UPCASE")
$ tstr=ii_dev+F$EXTRACT(0,F$LENGTH(ii_root)-1,ii_root)
$ tstr=tstr+F$EXTRACT(1,F$LENGTH(F$ELEMENT(0,"]",rec)),F$ELEMENT(0,"]",rec))+"]"
$ src=F$ELEMENT(1," ",rec)                  !Renamed file name
$ dst=F$ELEMENT(0," ",F$ELEMENT(1,"]",rec)) !Original file name
$ srcfile=tstr+src
$ IF F$SEARCH(srcfile) .NES.""
$ THEN
$ 	RENAME 'tstr''src' 'dst'
$ ENDIF
$! Here, we are using RENAME instead of the callback RENAME_FILE
$! because when installing over an existing installation, RENAME_FILE
$! fails if the destination file exists already.
$! VMI$CALLBACK RENAME_FILE 'tstr''src' 'dst'
$ GOTO next_line
$ end_rename:
$ VMI$CALLBACK MESSAGE I MVFILES "Renaming completed."
$
$ VMI$CALLBACK MESSAGE I SETPROTFILES "Setting protection on files..."
$ @VMI$KWD:set_prot
$ VMI$CALLBACK MESSAGE I SETPROTFILES "Setting protection completed."
$
$ VMI$CALLBACK MESSAGE I DELETEOLDFILES "Deleting files from the old installation..."
$ !Open the input file for reading
$ file_name="VMI$KWD:delete_files.dat"
$ GOSUB open_file_read
$ IF .NOT.ok THEN GOTO file_open_read_error
$ !Parse the record
$ delete_files_next_line:
$ GOSUB read_file
$ IF .NOT.ok THEN GOTO file_read_error
$ IF "''s'".EQS."eof" 
$ THEN
$	GOSUB close_all_files
$	GOTO end_delete
$ ENDIF
$ rec=F$EDIT(rec,"COMPRESS,TRIM,UNCOMMENT,UPCASE")
$ !Skip any lines that were all comment ("!"-prefixed)
$ IF "''rec'".EQS."".OR."''rec'".EQS." " THEN GOTO delete_files_next_line
$ tstr=ii_dev+F$EXTRACT(0,F$LENGTH(ii_root)-1,ii_root)
$ tstr=tstr+F$EXTRACT(1,F$LENGTH(F$ELEMENT(0,"]",rec)),F$ELEMENT(0,"]",rec))+"]"
$ src=F$ELEMENT(0," ",F$ELEMENT(1,"]",rec)) !file name
$ SET NOON
$ VMI$CALLBACK DELETE_FILE 'tstr''src'
$ SET ON
$ GOTO delete_files_next_line
$ end_delete:
$ VMI$CALLBACK MESSAGE I DELETEOLDFILES "Deleting files completed."
$
$ tstr=ii_dev+F$EXTRACT(0,F$LENGTH(ii_root)-1,ii_root)+"INGRES.INSTALL]"
$ VMI$CALLBACK PROVIDE_FILE ingres_ release.dat 'tstr' KC
$ tstr=ii_dev+F$EXTRACT(0,F$LENGTH(ii_root)-1,ii_root)+"INGRES.UTILITY]"
$ VMI$CALLBACK PROVIDE_FILE ingres_ purge_files.com 'tstr' KC
$ !
$ !	Ensure II_SYSTEM definition file and history file are provided
$ !	and updated as necessary. Do this last in case of errs.
$ !
$ file_name="sys$common:[sysexe]ingres_installations.dat"
$ IF provide_history THEN file_name="VMI$KWD:ingres_installations.dat"
$ tstr="open history file"
$ GOSUB open_file_append
$ IF .NOT.ok THEN GOTO hist_error
$ tstr=uic
$ IF "''F$EDIT(uic,"UPCASE")'".EQS."PARENT" THEN tstr="Unchanged"
$ rec=F$TIME()+" "+release+"  "+tstr+"  "+ii_dev+ii_root+"     "
$ IF "''choice'".eqs."all"
$ THEN
$	rec=rec+"All packages"
$ ELSE
$	rec=rec+package_list
$ ENDIF
$ cut_hist_rec:
$ IF F$LENGTH(rec).GT.80
$ THEN
$	tstr=F$EXTRACT(79,F$LENGTH(rec),rec)
$	rec=F$EXTRACT(0,79,rec)
$	GOSUB write_file
$	IF .NOT.ok THEN GOTO hist_error
$	rec="                       "+tstr
$	GOTO cut_hist_rec
$ ELSE
$	GOSUB write_file
$	IF .NOT.ok THEN GOTO hist_error
$ ENDIF
$ tstr="close history file"
$ GOSUB close_file_write
$ IF .NOT.ok THEN GOTO hist_error
$ IF provide_history THEN VMI$CALLBACK PROVIDE_FILE -
ingres_ ingres_installations.dat SYS$COMMON:[SYSEXE] KC
$ IF verbose
$ 	THEN
$	TYPE sys$input

				Notice:

	If you wish Ingres to start whenever the system is rebooted, add
	the following line to an appropriate place in SYSTARTUP_VMS.COM:

		     @SYS$STARTUP:INGRES_STARTUP.COM

				And...

	Edit the file SYS$STARTUP:INGRES_STARTUP.COM to ensure it is correct
	for your environment. A line is added to start EACH installation, but
	the line is commented out to prevent accidental errors.

	PLEASE EDIT SYS$STARTUP:INGRES_STARTUP.COM TO MEET YOUR NEEDS.

$ ELSE
$	VMI$CALLBACK MESSAGE I STARTUP_FILE "Please check SYS$STARTUP:INGRES_STARTUP.COM for correctness."
$	VMI$CALLBACK MESSAGE I STARTUP_FILE "You may call it during system startup to start Ingres automatically."
$ ENDIF
$ file_name="sys$startup:ingres_startup.com"
$ IF "''F$SEARCH(file_name)'".EQS.""
$ THEN
$	VMI$CALLBACK PROVIDE_FILE ingres_ ingres_startup.com SYS$STARTUP: KC
$	file_name="VMI$KWD:ingres_startup.com"
$ ENDIF
$ tstr="open startup file"
$ GOSUB open_file_append
$ IF .NOT.ok THEN GOTO hist_error
$ rec="$ !@"+ii_dev+F$EXTRACT(0,F$LENGTH(ii_root)-1,ii_root)+"INGRES]IISTARTUP.COM"
$ tstr="write startup message"
$ GOSUB write_file
$ IF .NOT.ok THEN GOTO hist_error
$ tstr="close startup file"
$ GOSUB close_file_write
$ IF .NOT.ok THEN GOTO hist_error
$ !
$ VMI$CALLBACK SET PURGE "NO" ! purge callback must be after ALL provide_files
$ ok=1
$ RETURN

$ hist_error:
$ s=$status
$ VMI$CALLBACK MESSAGE W FILEACCERR "File access was ''tstr'"
$ VMI$CALLBACK MESSAGE W FILEACCERR "File access error on ''file_name'"
$ VMI$CALLBACK MESSAGE I ERRSTATUS "File access error status ''s'"
$ VMI$CALLBACK MESSAGE I CONTINUING "Attempting to continue anyway. . ."
$ VMI$CALLBACK MESSAGE I SKIPPING "Possible update of system startup skipped."
$ VMI$CALLBACK MESSAGE I SKIPPING "Possible update of installation history file skipped."
$ IF verbose
$ THEN		! if ii_system is again defined in IISTARTUP then
$	TYPE sys$input	! this text should be changed to match...

			    Caution:

	The above file I/O error(s) have prevented update of
	the SYS$STARTUP:INGRES_STARTUP.COM procedure to include
	this installation. Please ensure it is correct.  Read the
	Installation Guide if necessary.

$ ENDIF
$ IF "''w_filename'".NES.""
$ THEN
$	CLOSE/ERROR=continue_anyway w_file
$	continue_anyway:	! this helps guarantee the global error
$	w_filename=""		! path is not taken ... it would abort
$ ENDIF
$ VMI$CALLBACK SET PURGE "NO" ! purge callback must be after ALL provide_files
$ DEFINE/EXEC/JOB II_SYSTEM 'ii_dev''ii_root'
$ DEFINE/EXEC/JOB II_PACKAGES 'package_list'
$ ok=1
$ RETURN
________________________________________________________________________________
			"Supporting" subroutines

$ parse_location:
$ ! first omit logical device names by repetitive translation... save all chars!
$ ii_dev=F$EXTRACT(0,F$LOCATE(":",tstr),tstr)
$ ii_dir=F$EXTRACT(F$LOCATE(":",tstr)+1,F$LENGTH(tstr),tstr)
$ IF "''F$TRNLNM(ii_dev)'".NES.""
$ THEN
$	tstr=F$TRNLNM(ii_dev)+ii_dir
$	GOTO parse_location
$ ENDIF
$ IF F$LOCATE(":",ii_dev).EQ.F$LENGTH(ii_dev) THEN ii_dev=ii_dev+":"
$ IF "''ii_dir'".EQS."" THEN ii_dir="[000000]"
$ correct_dir_spec:
$ ok=1
$ IF F$LOCATE("][",ii_dir).NE.F$LENGTH(ii_dir)
$ THEN ! ][ indicates a rooted spec was given ( eg: ...dir.][dir... )
$	ii_dir=F$EXTRACT(0,F$LOCATE("][",ii_dir),ii_dir)+F$EXTRACT(F$LOCATE("][",ii_dir)+2,f$length(ii_dir),ii_dir)
$	ok=0
$ ENDIF
$ IF F$LOCATE("000000",ii_dir).NE.F$LENGTH(ii_dir)
$ THEN ! if the zeros aren't at the beginning THEN they're from rooting
$	IF (F$LENGTH(ii_dir).GE.8).AND.(F$LOCATE("000000",ii_dir).NE.1)
$		THEN 
$		ii_dir=F$EXTRACT(0,F$LOCATE("000000",ii_dir),ii_dir)+F$EXTRACT(F$LOCATE("000000",ii_dir)+6,F$LENGTH(ii_dir),ii_dir)
$		ok=0
$		ENDIF
$ ENDIF
$ IF F$LOCATE("..",ii_dir).NE.F$LENGTH(ii_dir)
$ THEN ! double dots are from rooting too...
$	ii_dir=F$EXTRACT(0,F$LOCATE("..",ii_dir),ii_dir)+F$EXTRACT(F$LOCATE("..",ii_dir)+1,F$LENGTH(ii_dir),ii_dir)
$	ok=0
$ ENDIF
$ IF F$LOCATE(".]",ii_dir).NE.F$LENGTH(ii_dir)
$ THEN ! shouldn't get .] but handle it just in case
$	IF F$LOCATE(".]",ii_dir).NE.(F$LENGTH(ii_dir)-2)
$		THEN
$		tstr=F$EXTRACT(F$LOCATE(".]",ii_dir)+2,F$LENGTH(ii_dir),ii_dir)
$		VMI$CALLBACK MESSAGE W ROOTINGERR "Possible directory specification error."
$		VMI$CALLBACK MESSAGE I OMITSPEC "Truncating the specification. Removing: ''tstr'"
$		ENDIF
$	ii_root=ii_dir
$	ii_dir=F$EXTRACT(0,F$LOCATE(".]",ii_dir),ii_dir)+"]"
$	ok=0
$ ELSE
$	ii_root=F$EXTRACT(0,F$LOCATE("]",ii_dir),ii_dir)+".]"
$ ENDIF
$ IF .NOT.ok THEN GOTO correct_dir_spec	! loop because possible multiple rooting
$ ok=1
$ IF "''ii_dev'".EQS."" THEN ok=0	! or other suitable test perhaps
$ IF "''ii_dir'".EQS.""
$ THEN
$	ok=0	! or other suitable test perhaps
$ ELSE
$	IF F$LOCATE("INGRES]",ii_dir).NE.F$LENGTH(ii_dir) THEN VMI$CALLBACK MESSAGE W SUBDIR "The [INGRES] directory must be a subdirectory to ''ii_dir'"
$ ENDIF
$ RETURN

$ test_location:
$ ok=1
$ IF "''F$GETDVI(ii_dev,"EXISTS")'".EQS.""
$ THEN
$	VMI$CALLBACK MESSAGE W NOSUCHDEV "No such device exists: ''ii_dev'"
$	ok=0
$	RETURN
$ ENDIF
$ ON WARNING THEN CONTINUE
$ SET MESSAGE/NOFACILITY/NOSEVERITY/NOIDENT/NOTEXT
$ SET DEFAULT 'ii_dev''ii_dir'	! Get to directory
$ child=f$directory()
$ SET DEFAULT [-]		! Up to parent of location
$ parent=f$directory()
$ IF F$LOCATE("000000",parent).NE.F$LOCATE("000000",child) ! became MFD
$ THEN ! we know child is a "top level" directory
$	child=F$EXTRACT(F$LOCATE("000000",parent),F$LENGTH(child)-F$LOCATE("000000",parent)-1,child)
$ ELSE 
$     IF child.eqs.parent         ! check to see if it was the root directory
$     THEN
$         child = "000000"        ! both are the root directory, "[000000]"
$     ELSE
$         ! parent isn't the MFD, and child isn't a top level dir
$	  child=F$EXTRACT(F$LENGTH(parent),F$LENGTH(parent)-F$LENGTH(child)-1,child)
$     ENDIF
$ ENDIF
$ child=child+".DIR"
$ SET DEFAULT VMI$KWD
$ ON WARNING THEN GOTO error_handler
$ SET MESSAGE/FACILITY/SEVERITY/IDENT/TEXT
$ !'parent' should equal "ddcu:[parent_dir]"
$ !'child' should equal "child.dir"
$ tstr=ii_dev+parent+child
$ ok=0
$ IF "''F$PARSE(tstr)'".NES.""
$ THEN
$	VMI$CALLBACK FIND_FILE INGRES_ 'tstr' "" S ret_str
$	IF "''ret_str'".EQS."S" THEN ok=1
$!
$	!If an existing installation is used it must exist, gets its UIC
$	IF "''force_existence'".eq.1 
$	THEN
$		tstr = ii_dev + ii_dir + "INGRES.DIR"
$		IF "''F$SEARCH(tstr)'".EQS."" 
$		THEN
$			VMI$CALLBACK MESSAGE W NOSUCHDIR "No such directory exists: ''tstr'"
$			VMI$CALLBACK MESSAGE W NOOWNER -
				"When UIC is unspecified, the location must already exist."
$			ok = 0
$		ELSE
$			uic=F$FILE_ATTRIBUTES(tstr,"UIC")
$		ENDIF
$	ENDIF
$ ENDIF
$ RETURN


Make sure there is enough free disk space on the target device,
Enter check_location_space with ii_dev set to the target device.
The symbol tstr will be modified if the check fails.

$ check_location_space:
$ t=F$GETDVI(ii_dev,"FREEBLOCKS")
$ IF (t.LT.min_space)
$ THEN
$	VMI$CALLBACK MESSAGE W NOSPACE -
		"''min_space' disk blocks are required on ''ii_dev'"
$       ok=0
$       RETURN
$ ENDIF
$ ! If the target device is the same as the device which contains the
$ ! KWD, then double the min_space requirement.
$ IF F$GETDVI(ii_dev,"FULLDEVNAM").EQS.F$GETDVI("VMI$KWD","FULLDEVNAM")
$ THEN
$ 	IF (t.LT.(min_space * 2))
$ 	THEN
$		t=min_space * 2
$		tstr=F$GETDVI(ii_dev,"FULLDEVNAM")
$		VMI$CALLBACK MESSAGE W NOSPACE -
			"''tstr' contains both the target directory and the VMSINSTAL working directory"
$		VMI$CALLBACK MESSAGE W NOSPACE -
			"''t' disk blocks are temporarily required on ''tstr'"
$		ok=0
$		RETURN
$	ENDIF
$ ENDIF
$ ok=1
$ RETURN

Enter CREATE_SCRIPT with FILE_NAME set to the OUTPUT file name!!
And, with CREATE_RECORD set to the label of the right subroutine.

$ create_script:
$ IF F$LOCATE(create_record,create_record_list).EQ.F$LENGTH(create_record_list) 
$ THEN
$	VMI$CALLBACK MESSAGE F UNSUPPORTED "Unable to locate specified DCL label ''create_record'"
$	VMI$CALLBACK MESSAGE F CORRUPTED "This indicates probable corruption with this software kit."
$	GOTO fail_exit
$ ENDIF

$ !Open the output file for writing
$ GOSUB open_file_write
$ IF .NOT.ok THEN GOTO file_open_write_error

$ !Open the input file for reading
$ IF "''create_record'".EQS."rename_record"
$ THEN
$       file_name="VMI$KWD:renamed.lst"
$ ELSE 
$	IF "''create_record'".EQS."set_prot_record"
$ 	THEN
$       	file_name="VMI$KWD:permission_list.dat"
$ 	ELSE
$       	file_name="VMI$KWD:file_list.dat"
$	ENDIF
$ ENDIF
$ GOSUB open_file_read

$ IF .NOT.ok THEN GOTO file_open_read_error
$ IF "''create_record'".EQS."provide_file_record"
$ THEN
$ 	IF F$SEARCH("VMI$KWD:renamed.lst") .NES.""
$ 	THEN
$ 		file_name="VMI$KWD:renamed.lst"
$ 		GOSUB open_isam_read
$ 		IF .NOT.ok THEN GOTO isam_open_read_error
$	ENDIF
$ ENDIF       
$
$ !Parse the record & write to the output file
$ next_in_resolve:
$ GOSUB read_file
$ IF .NOT.ok THEN GOTO file_read_error
$ IF "''s'".EQS."eof" THEN GOTO close_all_files	!which RETURNs, or error exits
$ rec=F$EDIT(rec,"COMPRESS,TRIM,UNCOMMENT,UPCASE")
$ GOSUB 'create_record
$ GOSUB write_file
$ IF .NOT.ok THEN GOTO file_write_error
$ GOTO next_in_resolve

$ provide_file_record:
$ tstr=ii_dev+F$EXTRACT(0,F$LENGTH(ii_root)-1,ii_root)
$ tstr=tstr+F$EXTRACT(1,F$LENGTH(F$ELEMENT(0," ",rec)), F$ELEMENT(0," ",rec))
$ !
$ !     First we initialize fstr (the filename) to the one found in the
$ !     manifest listing.  If VMI$KWD:renamed.lst (the "rename" ISAM file)   
$ !     exists, then we do a keyed lookup passing it a directory and filename.
$ !     If the key is found, then we reset fstr (the filename) to the renamed
$ !     filename found in the lookup.
$ !
$ fstr=F$ELEMENT(1," ",rec)
$ kstr=F$EDIT(rec,"COLLAPSE")
$ GOSUB read_isam
$ IF ok 
$ THEN 
$	krec=F$EDIT(krec,"COMPRESS,TRIM,UNCOMMENT,UPCASE")
$	fstr=F$ELEMENT(1," ",krec)
$ ENDIF
$ rec="INGRES_ ''fstr' ''tstr' KC"
$ RETURN

$ purge_files_record:
$ rec="$PURGE II_SYSTEM:''F$ELEMENT(0," ",rec)'''F$ELEMENT(1," ",rec)'"
$ RETURN

$ set_prot_record:
$ rec="$SET PROT II_SYSTEM:''F$ELEMENT(0," ",rec)'''F$ELEMENT(1," ",rec)'/PROTECTION=(''F$ELEMENT(2," ",rec)')"
$ RETURN

$ press_return:
$ VMI$CALLBACK ASK ingres_str "Press return to continue" "continue" SZ
$ IF "''ingres_str'".EQS."^Z" THEN GOTO control_z
$ RETURN

		NOTICE FOR FILE ACCESS SUBROUTINES:

	We don't exit/error out immediately on an error so that the
	caller has a choice... There ARE several cases where the
	caller doesn't want or need to exit. Following the access
	routines are the error/cleanup routines which provide a
	common reporting mechanism for those errors that are FATAL!

	Also, the symbol FILE_NAME may change out from under the
	access routines, so they keep track of their own open file
	filenames - one for read, one for write. The caller is not
	asked to specify a read/write filename because that could
	complicate matters unnecessarily. This is merely a convenience.

$ read_w_close:		! ok THEN REC contains a record, else 
$ GOSUB read_file	! end of file & S = "eof" AND file is closed else
$ IF .NOT.ok THEN GOTO file_read_error	! an error path was taken, no return...
$ IF "''s'".EQS."eof"
$ THEN 
$	GOSUB close_file_read
$	IF .NOT.ok THEN GOTO close_file_read_error
$	ok=0
$ ELSE
$	ok=1
$ ENDIF
$ RETURN


$ open_file_read:
$ ok=1
$ r_filename=file_name	! help guarantee that an error report has the right name
$ ! and, if set, specifies that there IS an open read file...
$ OPEN/READ/ERROR=file_error r_file 'file_name
$ RETURN

$ open_isam_read:
$ ok=1
$ k_filename=file_name	! help guarantee that an error report has the right name
$ ! and, if set, specifies that there IS an open read file...
$ OPEN/READ/ERROR=file_error k_file 'file_name
$ RETURN

$ open_file_write:
$ ok=1
$ w_filename=file_name ! help guarantee that an error report has the right name
$ ! and, if set, specifies that there IS an open read file...
$ OPEN/WRITE/ERROR=file_error w_file 'file_name
$ RETURN

$ open_file_append:
$ ok=1
$ w_filename=file_name ! help guarantee that an error report has the right name
$ ! and, if set, specifies that there IS an open read file...
$ OPEN/APPEND/ERROR=file_error w_file 'file_name
$ RETURN

$ close_file_read:
$ ok=1
$ CLOSE/ERROR=file_error r_file
$ r_filename=""
$ RETURN

$ close_isam_read:
$ ok=1
$ CLOSE/ERROR=file_error k_file
$ k_filename=""
$ RETURN

$ close_file_write:
$ ok=1
$ CLOSE/ERROR=file_error w_file
$ w_filename=""
$ RETURN

$ read_file:
$ ok=1
$ s=1	!change status to clear old errors so check relates to THIS access
$ READ/ERROR=file_error/END=eof r_file rec
$ RETURN

$ read_isam:
$ ok=1
$ s=1	!change status to clear old errors so check relates to THIS access
$ READ/ERROR=file_error/END=eof/INDEX=0/KEY='kstr k_file krec
$ msg=""
$ RETURN

$ eof:
$ s="eof"
$ RETURN

$ write_file:
$ ok=1
$ WRITE/ERROR=file_error w_file rec
$ RETURN

$ file_error:
$ s=$status
$ ok=0
$ RETURN

________________________________________________________________________________
				Error handlers and
				cleanup routines

	Note:	The file close routines could be cleaned up. Remember that we
		MUST let the caller decide whether or not to exit based on an
		open error. R_FILENAME and W_FILENAME exist to "remember" both
		whether a file is currently open, and if so, its' filename,
		since we can only have one open of each at a time.

$ file_open_read_error:
$ file_name=r_filename
$ WRITE SYS$OUTPUT "File open for read error."
$ GOTO return_filename

$ isam_open_read_error:
$ file_name=k_filename
$ WRITE SYS$OUTPUT "Keyed file open for read error."
$ GOTO return_k_filename

$ file_open_write_error:
$ file_name=w_filename
$ WRITE SYS$OUTPUT "File open for write error."
$ GOTO return_filename

$ file_read_error:
$ file_name=r_filename
$ WRITE SYS$OUTPUT "File read error."
$ GOTO return_filename

$ isam_read_error:
$ file_name=k_filename
$ WRITE SYS$OUTPUT "Keyed file read error."
$ GOTO return_k_filename

$ file_write_error:
$ file_name=w_filename
$ WRITE SYS$OUTPUT "File write error."
$ GOTO return_filename

$ close_file_read_error:
$ file_name=r_filename
$ WRITE SYS$OUTPUT "File open for read, close error."
$ GOTO return_filename

$ close_isam_read_error:
$ file_name=k_filename
$ WRITE SYS$OUTPUT "Keyed file open for read, close error."
$ GOTO return_k_filename

$ close_file_write_error:
$ file_name=w_filename
$ WRITE SYS$OUTPUT "File open for write, close error."
$ GOTO return_filename

$ return_filename:
$ WRITE SYS$OUTPUT "File name was ''file_name'"
$ r_filename=""
$ CLOSE/ERROR=exiting_close_error r_file

$ return_k_filename:
$ k_filename=""
$ CLOSE/ERROR=exiting_close_error k_file

$ last_chance_close:
$ IF "''w_filename'".NES.""
$ THEN
$	w_filename=""
$	CLOSE/ERROR=exiting_close_error w_file
$ ENDIF
$ GOTO error_handler	! this passes back the original VMS error.

...handle read file close error before checking write file

$ exiting_close_error:
$ WRITE SYS$OUTPUT "Error closing file during abort."
$ IF "''w_filename'".NES."" THEN GOTO last_chance_close
$ GOTO error_handler

$ control_z:
$ WRITE SYS$OUTPUT "Exit requested by Installer."
$ GOSUB close_all_files
$ s=VMI$_FAILURE
$ ok=0
$ RETURN

$ control_y:
$ WRITE SYS$OUTPUT "Exit requested by Installer."
$ GOSUB close_all_files
$ VMI$CALLBACK CONTROL_Y
$ GOTO fail_exit	! should never return to this GOTO...

$ close_all_files:
$ IF "''r_filename'".NES.""
$ THEN
$	GOSUB close_file_read
$	IF .NOT.ok THEN GOTO close_file_read_error
$ ENDIF
$ IF "''k_filename'".NES.""
$ THEN
$	GOSUB close_isam_read
$	IF .NOT.ok THEN GOTO close_isam_read_error
$ ENDIF
$ IF "''w_filename'".NES.""
$ THEN 
$	GOSUB close_file_write
$	IF .NOT.ok THEN GOTO close_file_write_error
$ ENDIF
$ RETURN

$ fail_exit:
$ GOSUB close_all_files
$ s=VMI$_FAILURE
$ GOTO done

$ success_exit:
$ GOSUB close_all_files
$ s=VMI$_SUCCESS
$ GOTO done

$ error_handler:
$ s=$STATUS	! this allows propagation of the error status to outer layers

$ done:
$ IF "''s'".EQS."''VMI$_SUCCESS'"
$ THEN
$	IF ivp_done ! INGRES IVP ran successfully
$	THEN
$		TYPE SYS$INPUT

	You can now use the "ingstart" command to start your Ingres 
	servers.  Refer to the Ingres Installation Guide for more
	information about starting and using Ingres.

	Also, following scripts have been provided to setup Ingres symbols:
	   II_SYSTEM:[INGRES]INGSYSDEF.COM : defines Ingres sysadmin symbols
	   II_SYSTEM:[INGRES]INGDBADEF.COM : defines Ingres DBA symbols
	   II_SYSTEM:[INGRES]INGUSRDEF.COM : defines Ingres user symbols

$ 	ELSE	! Did not run INGRES IVP
$		TYPE SYS$INPUT

				PLEASE NOTE:
	In order to start running the Ingres DBMS server, you should 
	first run all the necessary setup programs as documented in the 
	Ingres Installation Guide and then run "ingstart".
	Before running the setup programs, you must be prepared to perform
	all of the setup and configuration phases.  This requres decisions
	to be made about how Ingres shall be used.  Usually, changes
	cannot be made while the installation is up and running.
	Ingres Corporation STRONGLY RECOMMENDS reading the Ingres
	Installation Guide prior to this step.
	Refer to the Ingres Installation Guide for more information 
	about starting and using Ingres.

	Also, the following scripts have been provided to setup INGRES symbols:
	   II_SYSTEM:[INGRES]INGSYSDEF.COM : defines Ingres sysadmin symbols
	   II_SYSTEM:[INGRES]INGDBADEF.COM : defines Ingres DBA symbols
	   II_SYSTEM:[INGRES]INGUSRDEF.COM : defines Ingres user symbols

$	ENDIF
$ ENDIF
$ IF "''F$TRNLNM("II_MANIFEST_DIR", "LNM$PROCESS")'".NES."" THEN -
DEASSIGN/PROC II_MANIFEST_DIR
$ IF "''F$TRNLNM("II_MSGDIR", "LNM$PROCESS")'".NES."" THEN -
DEASSIGN/PROC II_MSGDIR
$ IF "''F$TRNLNM("II_TERMCAP_FILE","LNM$JOB")'".NES."" THEN -
DEASSIGN/JOB II_TERMCAP_FILE	
$ !restore ii_system and ii_auth_string if we aborted at a bad time
$ if "''f$trnlnm("II_SYSTEM")'".eqs.""
$ then
$   if "''save_ii_system'".nes."" then define/job ii_system "''save_ii_system'"
$ endif
$ EXIT s
