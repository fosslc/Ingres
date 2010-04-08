@ECHO off
REM Copyright (c) 2005 Ingres Corporation
REM
REM Name:
REM     ACPEXIT - Archiver Termination Command Script
REM
REM Usage:
REM     acpexit error_number [database_name]
REM
REM         error_number:   Termination status from archiver.
REM                         A list of termination error codes is given
REM                         at the end of this script.
REM
REM         database_name:  Name of database being processed when
REM                         archiver stopped.  This argument may not be
REM                         present if the termination reason was not
REM                         associated with a database or if the database
REM                         name was not known.
REM
REM Description:
REM
REM     The acpexit script is run automatically by the archiver whenever
REM     some situation prevents it from moving records from the Ingres
REM     Log file to the database journal files (usually lack of disk
REM     space on the journal disk).
REM
REM     When the archive process stops, the script is executed to give
REM     notification of the event.
REM     The installation will remain active and database writes will
REM	    be allowed, but no log records will be moved out of the Ingres
REM	    Log File until archive processing is restarted.
REM
REM	    If archive processing is not restarted then eventually the Ingres
REM	    Log File will become full and work in the installation will be
REM	    suspended until log records can be archived.  This situation is
REM	    known as LOGFULL and is indicated by the LOGFULL status given
REM	    by logstat.
REM
REM	    Upon termination of archive processing (and the execution of this
REM	    script), steps should be taken to resolve the archive problem and
REM	    archiving should be restarted with the Ingres command
REM	    ingstart -dmfacp.
REM
REM	History:
REM	    25-Jul-2005 (fanra01)
REM	        Bug 90455
REM	        Created based on acpexit.sh.

SETLOCAL

SET DEBUG=
SET ACPEXIT_LOG=NUL
IF "%DEBUG%" == "ON" SET ACPEXIT_LOG="%II_SYSTEM%\ingres\files\acpexit.log"
IF "%DEBUG%" == "ON" DATE /T > %ACPEXIT_LOG% & TIME /T >>%ACPEXIT_LOG% & ECHO ACPEXIT LOG >>%ACPEXIT_LOG%

REM
REM Collect input arguments.
REM
SET ERROR_NUMBER=%1
SET DATABASE_NAME=%2

IF "%ERROR_NUMBER%" == "" SET ERROR_NUMBER=0

REM
REM If the archiver termination is NORMAL - due to system shutdown, then ignore
REM the termination as it is expected.
REM
REM Perform special handling for certain termination codes:
REM

IF "%ERROR_NUMBER%" == "235600" GOTO NORMEXIT
IF "%ERROR_NUMBER%" == "235601" GOTO ACPINIT
IF "%ERROR_NUMBER%" == "235602" GOTO JNLFULL
IF "%ERROR_NUMBER%" == "235603" GOTO RSRCEXIT
IF "%ERROR_NUMBER%" == "235604" GOTO JNLEXIT
IF "%ERROR_NUMBER%" == "235605" GOTO LOGEXIT
IF "%ERROR_NUMBER%" == "235606" GOTO DBCNFEXIT
IF "%ERROR_NUMBER%" == "235607" GOTO LRECEXIT
IF "%ERROR_NUMBER%" == "235608" GOTO CHKPEXIT
IF "%ERROR_NUMBER%" == "235609" GOTO SEGVEXIT 
IF "%ERROR_NUMBER%" == "235610" GOTO PROTEXIT
IF "%ERROR_NUMBER%" == "235611" GOTO INTERROR
IF "%ERROR_NUMBER%" == "235612" GOTO DMPEXIT
IF "%ERROR_NUMBER%" == "235615" GOTO UNKNOWN

:NORMEXIT
ECHO E_DM9850_ACP_NORMAL_EXIT >> %ACPEXIT_LOG%
GOTO END

:ACPINIT
ECHO E_DM9851_ACP_INITIALIZE_EXIT >> %ACPEXIT_LOG%
GOTO END

:JNLFULL
ECHO E_DM9852_ACP_JNL_WRITE_EXIT >> %ACPEXIT_LOG%
REM     
REM      Out of disk space on journal device.
REM      Free up disk space on device and restart archive processing.
REM     
REM
REM      Remove file with space that was reserved in case the journal
REM      device become low in order to let the installation proceed
REM      until new space can be found.
REM
REM      rmdir /Q /S "%II_JOURNAL%\save_space\block1"
REM
REM      Restart archive processing
REM     
REM      ingstart -dmfacp
GOTO END

:RSRCEXIT
ECHO E_DM9853_ACP_RESOURCE_EXIT >> %ACPEXIT_LOG%
GOTO END

:JNLEXIT
ECHO E_DM9854_ACP_JNL_ACCESS_EXIT >> %ACPEXIT_LOG%
GOTO END

:LOGEXIT
ECHO E_DM9855_ACP_LG_ACCESS_EXIT >> %ACPEXIT_LOG%
GOTO END

:DBCNFEXIT
ECHO E_DM9856_ACP_DB_ACCESS_EXIT >> %ACPEXIT_LOG%
GOTO END

:LRECEXIT
ECHO E_DM9857_ACP_JNL_RECORD_EXIT >> %ACPEXIT_LOG%
GOTO END

:CHKPEXIT
ECHO E_DM9858_ACP_ONLINE_BACKUP_EXIT >> %ACPEXIT_LOG%
GOTO END

:SEGVEXIT
ECHO E_DM9859_ACP_EXCEPTION_EXIT >> %ACPEXIT_LOG%
GOTO END

:PROTEXIT
ECHO E_DM985A_ACP_JNL_PROTOCOL_EXIT >> %ACPEXIT_LOG%
GOTO END

:INTERROR
ECHO E_DM985B_ACP_INTERNAL_ERR_EXIT >> %ACPEXIT_LOG%
GOTO END

:DMPEXIT
ECHO E_DM985C_ACP_DMP_WRITE_EXIT  >> %ACPEXIT_LOG%
GOTO END

:UNKNOWN
ECHO E_DM985F_ACP_UNKNOWN_EXIT >> %ACPEXIT_LOG%
GOTO END

REM----------------------------------------------------------------------------
REM
REM Termination error numbers:
REM
REM   E_DM9850_ACP_NORMAL_EXIT
REM
REM	Value : 235600		Hex: 39850
REM	Message written to Archiver Log:
REM	
REM		Archiver shutdown has occurred as part of a shutdown of 
REM		the Ingres installation.  For further information on the 
REM		installation shutdown, refer to the II_RCP log file.
REM
REM	Description:
REM
REM		The archiver has terminated upon request of the Logging System.
REM		The Logging System will direct the archiver to shut down as part
REM		of normal installation shutdown proceedings.
REM
REM		This message is written by the archiver as part of its shutdown
REM		protocols.  It does not indicate an error in archive processing.
REM
REM
REM  E_DM9851_ACP_INITIALIZE_EXIT
REM
REM	Value : 235601		Hex: 39851
REM	Message written to Archiver Log:
REM	
REM		An error has been encountered during Archiver initialization 
REM		which prevents archive processing from starting.  Please make 
REM		sure that the Ingres installation is running and that there 
REM		is not already an active Archiver.  See the Archiver Log 
REM		(iiacp.log) for more detailed information on the failure.
REM
REM	Description:
REM
REM		An error occurred during archiver startup and the archiver 
REM		could not be initialized.  The Archiver Log (iiacp.log) 
REM		should contain more detailed information about the specific 
REM		errors encountered.
REM
REM  E_DM9852_ACP_JNL_WRITE_EXIT
REM
REM	Value : 235602		Hex: 39852
REM	Message written to Archiver Log:
REM		Archive processing cannot continue due to the inability to 
REM		write journal records for database %0c to the database journal 
REM		directory: %1c.  This is probably due to a lack of available 
REM		disk space on the journal device.  The error occurred during 
REM		an attempt to write to the journal file.  Please check for 
REM		disk space and the ability to create/write files in the above 
REM		journal directory.  The Archiver Log (iiacp.log) will have 
REM		more detailed information about the problem, including the 
REM		Operating System's return code from the requested operation.
REM		To continue archive processing following resolution of the 
REM		above problem, run the Ingres command 'ingstart -dmfacp'.
REM	
REM	Description:
REM		Archiver could not write journal records for indicated database
REM		to the journal files.  This is probably due to lack of disk
REM		space on the journal device.
REM
REM		The error may be encountered during an attempt to create a new
REM		journal file or extend an old one.  Journal files are numbered
REM		sequentially in the database journal location with access done
REM		only to the highest number file.
REM
REM
REM  E_DM9853_ACP_RESOURCE_EXIT
REM
REM	Value : 235603		Hex: 39853
REM	Message written to Archiver Log:
REM		Archive processing cannot continue due to a failure to 
REM		allocate a %0c resource.  Please check the Archiver Log 
REM		(iiacp.log) for more detailed information about the resource 
REM		required.  This error may occur due to lack of logging, 
REM		locking, or memory resources.   If logging or locking 
REM		resources have been exhausted the system may need to be 
REM		shutdown, reconfigured to allocate more %1c, and then 
REM		restarted.  Inability to allocate memory may indicate process 
REM		quota limitations.  If it is suspected that the resource 
REM		exhaustion was a temporary condition, continuation of archive 
REM		processing without system reconfiguration may be attempted by 
REM		running 'ingstart -dmfacp'.  If system reconfiguration cannot 
REM		be performed immediately, resources may be recovered by 
REM		temporarily decreasing the amount of concurrent activity 
REM		to the Ingres system.  'ingstart -dmfacp' must be run to 
REM		continue archive processing.
REM
REM	Description:
REM		Archiver could not allocate a resource necessary for processing.
REM
REM		Types of resource failure are:
REM
REM		Logging - Could not start transaction for archive processing.
REM			  Logging system configuration should be altered to
REM			  increase maximum number of concurrent transactions.
REM		Locking - Could not allocate lock list for archive processing.
REM			  Locking system configuration should be altered to
REM			  increase the number of lock lists.
REM		Memory  - Could not allocate memory for archive processing.
REM			  This may be due to process quotas or actual system
REM			  resources.  On VMS systems, quotas on the ACP should
REM			  be examined to determine if they can be increased
REM			  (See the Ingres utility script RUN_ACP.COM).  If
REM			  system memory is exhausted, then the system load may
REM			  need to be decreased.
REM
REM		Since most resource exhaustions are temporary and occur at peak
REM		loads, the archiver can often be restarted immediately without
REM		actually having to reconfigure the system.  Some resources may
REM		be recovered by decreasing the amount of concurrent activity to
REM		the Ingres system.
REM
REM
REM  E_DM9854_ACP_JNL_ACCESS_EXIT
REM
REM	Value : 235604		Hex: 39854
REM	Message written to Archiver Log:
REM		Archive processing cannot continue due to the inability to 
REM		access a journal file for database %0c.  Please check the 
REM		Archiver Log (iiacp.log) for the type of access which failed, 
REM		the name of the journal file accessed and any Operating System 
REM		messages describing the type of failure.  Make sure that the 
REM		database journal directory (%1c) exists and that the Archiver 
REM		has permission to create and write to files in it.  This error 
REM		could also be the result of exhaustion of system or process 
REM		file resources or quotas.  To continue archive processing 
REM		following resolution of the above problem, run the Ingres 
REM		command 'ingstart -dmfacp'.
REM
REM	Description:
REM		Archiver could not open/write/close journal records for the 
REM		indicated database to the journal files.
REM
REM		This error may indicate a permission or quota problem.  The 
REM		Archiver Log should be reviewed for detailed information 
REM		specific to the problem just encountered.
REM
REM
REM  E_DM9855_ACP_LG_ACCESS_EXIT
REM
REM	Value : 235605		Hex: 39855
REM	Message written to Archiver Log:
REM		Archive processing cannot continue due an error reading 
REM		information from the Logging System. Please check the 
REM		Archiver Log (iiacp.log) for the specific operation which 
REM		failed and more detailed messages describing the error.  
REM		Verify that the Ingres Installation is currently running.  
REM		It is possible that failures attempting to read the Ingres 
REM		Log File could be caused by process quota limitations; if so, 
REM		this should be reflected in the error log.  If the cause of 
REM		the error is not apparent, try restarting archive processing 
REM		via the Ingres command 'ingstart -dmfacp'.  If the same error is
REM		encountered, try stopping and restarting the Ingres 
REM		installation.
REM	
REM	Description:
REM		Archiver encountered an error from a call to the Logging 
REM		System or in an attempt to read/write transaction log file 
REM		records.
REM
REM		This may indicate a problem with the Ingres Log File or with the
REM		information in the Logging System.  First verify that 
REM		permissions and quotas for the Archiver are SET correctly.
REM
REM		If the problem is with Logging System information, then 
REM		restarting the installation should clear up the problem.
REM
REM
REM  E_DM9856_ACP_DB_ACCESS_EXIT
REM
REM	Value : 235606		Hex: 39856
REM	Message written to Archiver Log:
REM		Archive processing cannot continue on database %0c due to a 
REM		failure accessing the database config file.  Please check the 
REM		Archiver Log (iiacp.log) for further information about the 
REM		error encountered.  Check the existence of and permissions on 
REM		the file 'aaaaaaaa.cnf' in the database's root DATA location.  
REM		If the database config file has been removed or no longer 
REM		exists, it must be restored using a backup copy.  A backup 
REM		copy of the config file may normally be found in the database 
REM		DUMP location.  To continue archive processing following 
REM		resolution of the above problem, run the Ingres command 
REM		'ingstart -dmfacp'.
REM	
REM	Description:
REM		Archiver could not open/close/write the database config file
REM		for the specified database.
REM
REM		This may indicate a permission or quota problem - or could be
REM		caused by non-existence of the config file.
REM
REM		Most database-specific problems can be bypassed by disabling
REM		logging on the failing database in order to get archiver 
REM		processing going - then re-enabling journal processing after 
REM		the archiver has skipped over the troublesome area of journal 
REM		processing.  This can be done through:
REM
REM			 'alterdb -disable_journaling'  
REM
REM		Note that this will cause journal records for the database to 
REM		be discarded, so that they cannot be used in auditdb or 
REM		rollforward processing; the database will no longer be 
REM		journaled.  Journaling should be re-enabled as soon as possible 
REM		by issuing a new checkpoint: ckpdb +j.
REM
REM
REM  E_DM9857_ACP_JNL_RECORD_EXIT
REM
REM	Value : 235607		Hex: 39857
REM	Message written to Archiver Log:
REM		Archive processing cannot continue on database %0c due to an 
REM		unexpected log record which the archiver cannot process.  
REM		Please check the Archiver Error Log (iiacp.log) for further 
REM		information about the error encountered.  This may indicate a 
REM		journaling error on the above database which prevents journaled 
REM		log records from being moved from the transaction log file to 
REM		the database journal file.  If the Archiver Log does not 
REM		indicate a solution, try restarting archive processing (using 
REM		the 'ingstart -dmfacp' command) to see if the problem is 
REM		temporary rather than a symptom of inconsistent journal records.
REM		If the same problem is encountered, then journaling should be 
REM		disabled on the database to allow the archiver to bypass the 
REM		inconsistent journal records.  See the 'alterdb' command for 
REM		information on disabling journal processing for a database.
REM	
REM	Description:
REM		A journaling error was encountered while attempting to process
REM		a journal record for the specified database.  This probably 
REM		indicates an inconsistency in journal information for that db.
REM
REM		This may be an invalid or unrecognized type of log record, or
REM		a log record not associated with a transaction known to the
REM		archiver (ie. the Begin Transaction / End Transaction pair for 
REM		this transaction were not found).
REM
REM		Most database-specific problems can be bypassed by disabling
REM		logging on the failing database in order to get archiver 
REM		processing going - then re-enabling journal processing after 
REM		the archiver has skipped over the troublesome area of journal 
REM		processing.  This can be done through:
REM
REM			 'alterdb -disable_journaling'  
REM
REM		Note that this will cause journal records for the database to 
REM		be discarded, so that they cannot be used in auditdb or 
REM		rollforward processing; the database will no longer be 
REM		journaled.  Journaling should be re-enabled as soon as possible 
REM		by issuing a new checkpoint: ckpdb +j.
REM
REM
REM  E_DM9858_ACP_ONLINE_BACKUP_EXIT
REM
REM	Value : 235608		Hex: 39858
REM	Message written to Archiver Log:
REM		Archive processing failed due to an error processing records 
REM		associated with an Online Checkpoint operation on database 
REM		%0c.  Please check the Archiver Log (iiacp.log) for further 
REM		information about the error encountered.  Interrupt the 
REM		Checkpoint request and issue the command 'ingstart -dmfacp' to 
REM		restore archive processing.  Use 'logstat' to check the state of
REM		the Ingres installation; verify that the START_ARCHIVER state is
REM		turned off.  This indicates that archive processing is 
REM		continuing normally.  Reissue the Checkpoint request to backup 
REM		the database.  If subsequent attempts to checkpoint the 
REM		database fail, try using the -l flag to force an Offline 
REM		Checkpoint.
REM	
REM	Description:
REM		The archiver failed due to an error in Online Checkpoint 
REM		processing.
REM
REM		Virtually all errors encountered during online backup are mapped
REM		to this error.  The problem may have occurred accessing II_DUMP,
REM		database or logging system information, or be due to resource
REM		or quota limitations.
REM
REM		The Archiver Log should give more detailed information as to
REM		what operation failed.
REM
REM		Usually, the archiver can be restarted immediately since the
REM		Online Backup operation will have failed.  When archive 
REM		processing continues, DUMP processing will not be done as the 
REM		checkpoint will no longer be in progress.
REM
REM		If the Checkpoint error was really an indication of bad archiver
REM		problems that were not necessarily related to checkpoint 
REM		processing (the error just happened to be encountered in a 
REM		checkpoint-specific routine), then attempts to restart the 
REM		archiver may also fail.
REM
REM
REM  E_DM9859_ACP_EXCEPTION_EXIT
REM
REM	Value : 235609		Hex: 39859
REM	Message written to Archiver Log:
REM		An unexpected exception has halted archive processing.  Please 
REM		check the Archiver Log (iiacp.log) for further information 
REM		about the exception.  Archive processing should be restarted 
REM		with the 'ingstart -dmfacp' command.  If the exception error 
REM		continues to occur and prevents any archive work from being 
REM		done, the installation should be shut down and restarted to 
REM		clear the problem.
REM	
REM	Description:
REM		An exception has forced the archiver to stop.  This will most 
REM		likely represent an Ingres bug and should be reported as such 
REM		to Ingres Technical Support.
REM
REM		The Archiver Log File (iiacp.log) should be saved and if 
REM		possible, a LOGDUMP should be performed to gather information 
REM		to use in tracking down the bug.
REM
REM		In some cases archive processing can be continued immediately 
REM		via the ingstart -dmfacp command.  If the exception error 
REM		continues, the installation should be restarted.  If the 
REM		exception still continues, then the error is probably being 
REM		triggered by some condition in the Log File itself.
REM
REM		Inspecting the Archiver Log File may give a clue as to whether 
REM		the exception occurs on processing of a particular database.  
REM		Most database-specific problems can be bypassed by temporarily 
REM		disabling logging on the database to allow the archiver to move
REM		forward past the 'bad' log records.  See the ALTERDB command 
REM		for further information on this.  Note that journaling should 
REM		be re-enabled as soon as possible since the database is no 
REM		longer protected by the journaling system after journaling is 
REM		disabled.
REM
REM		If the problem cannot be isolated to processing for a particular
REM		database, or if disabling journaling does not resolve the 
REM		exception condition, then the Ingres Log File may need to be 
REM		reinitialized to clear the condition.
REM
REM
REM  E_DM985A_ACP_JNL_PROTOCOL_EXIT
REM
REM	Value : 235610		Hex: 3985A
REM	Message written to Archiver Log:
REM		Archive processing cannot continue due to an error in protocols 
REM		between the Logging System and the Ingres Archiver.  The 
REM		archiver encountered a record that it did not expect or did not 
REM		find a record that it did expect in the Ingres Log File.  
REM		Please check the Archiver Log (iiacp.log) for further 
REM		information about the problem.  Archive processing should be 
REM		restarted with the 'ingstart -dmfacp' command.  If the problem 
REM		persists, the installation should be shut down and restarted.  
REM		If the problem still persists, it may indicate a Log File 
REM		inconsistency which will require a Log File reinitialization.
REM	
REM	Description:
REM		The archiver encountered a log record or was given LG 
REM		information that it did not understand, or did not think was 
REM		consistent.
REM
REM		This error will usually be triggered if the archiver reads a 
REM		log record that it cannot match to any journaled database. This 
REM		will most likely represent an Ingres bug and should be reported 
REM		as such to Ingres Technical Support.
REM
REM		The Archiver Log File (iiacp.log) should be saved and if 
REM		possible, a LOGDUMP should be performed to gather information 
REM		to use in tracking down the bug.
REM
REM		In some cases archive processing can be continued immediately 
REM		via the ingstart -dmfacp command.  If the protocol error 
REM		continues, the installation should be restarted.
REM
REM
REM  E_DM985B_ACP_INTERNAL_ERR_EXIT
REM
REM	Value : 235611		Hex: 3985B
REM	Message written to Archiver Log:
REM		Archive processing cannot continue due to an internal error in 
REM		archiver processing.  Please check the Archiver Log 
REM		(iiacp.log) for further information about the problem.  
REM		Archive processing should be restarted with the 
REM		'ingstart -dmfacp' command.  If the problem persists, the 
REM		installation should be shut down and restarted.  If the problem
REM		still persists, it may indicate a Log File inconsistency which 
REM		will require a Log File reinitialization.
REM	
REM	Description:
REM		The archiver encountered an internal error that prevents further
REM		processing. This will most likely represent an Ingres bug and 
REM		should be reported as such to Ingres Technical Support.
REM
REM		The Archiver Log File (iiacp.log) should be saved and if 
REM		possible, a LOGDUMP should be performed to gather information 
REM		to use in tracking down the bug.
REM
REM		In some cases archive processing can be continued immediately 
REM		via the ingstart -dmfacp command.  If the protocol error 
REM		continues, the installation should be restarted.
REM
REM
REM  E_DM985C_ACP_DMP_WRITE_EXIT 
REM
REM	Value : 235612		Hex: 3985C
REM	Message written to Archiver Log:
REM		Archive processing cannot continue due to the inability to 
REM		write dump records during an Online Checkpoint operation on 
REM		database %0c to the database dump directory: %1c.  This is 
REM		probably due to a lack of available disk space on the dump 
REM		device.  The error occurred during an attempt to write to the 
REM		dump file.  Please check for disk space and the ability to 
REM		create/write files in the above dump directory.  The Archiver 
REM		Log (iiacp.log) will have more detailed information about the 
REM		problem, including the Operating System's return code from the 
REM		requested operation.  The Checkpoint request should be 
REM		interrupted and the archive processing restored through the 
REM		Ingres command 'ingstart -dmfacp'.
REM	
REM	Description:
REM		Archiver could not write log records for indicated database
REM		to the dump files.  This is probably due to lack of disk
REM		space on the dump device.
REM
REM		The error may be encountered during an attempt to create a new
REM		dump file or extend an old one.  Dump files are numbered
REM		sequentially in the database journal location with access done
REM		only to the highest number file.
REM
REM
REM  E_DM985F_ACP_UNKNOWN_EXIT
REM
REM	Value : 235615		Hex: 3985F
REM	Message written to Archiver Log:
REM		Archive processing cannot continue due to an internal error 
REM		in archiver processing.  Please check the Archiver Log 
REM		(iiacp.log) for further information about the problem.  
REM		Archive processing should be restarted with the 
REM		'ingstart -dmfacp' command.  If the problem persists, the 
REM		installation should be shut down and restarted.  If the problem 
REM		still persists, it may indicate a Log File inconsistency which 
REM               will require a Log File reinitialization.
REM	
REM	Description:
REM		The archiver encountered an internal error that prevents further
REM		processing. This will most likely represent an Ingres bug and 
REM		should be reported as such to Ingres Technical Support.
REM
REM		The Archiver Log File (iiacp.log) should be saved and if 
REM		possible, a LOGDUMP should be performed to gather information 
REM		to use in tracking down the bug.
REM
REM		In some cases archive processing can be continued immediately 
REM		via the ingstart -dmfacp command.  If the protocol error 
REM		continues, the installation should be restarted.
REM

:END
endlocal
