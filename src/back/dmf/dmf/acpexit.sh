:
# Copyright (c) 2004 Ingres Corporation
#
# Name:  
#	ACPEXIT - Archiver Termination Command Script
#
# Usage: 
#	acpexit  error_number  [database_name]
#
#	    error_number  : Termination status from archiver.
#			    A list of termination error codes is given
#			    at the end of this script.
#
#	    database_name : Name of database being processed when
#			    archiver stopped.  This argument may
#			    not be present if the termination reason
#			    was not associated with a database or if
#			    the database was not known.
#
# Description: 
#
#	The acpexit script is run automatically by the archiver whenever
#	some situation prevents it from moving records from the Ingres
#	Log File to the database journal files (usually a lack of disk
#	space on the journal disk).
#
#	When archive processing stops, this script is executed to give
#	notification of the event.
#
#	The installation will remain active and database writes will
#	be allowed, but no log records will be moved out of the Ingres
#	Log File until archive processing is restarted.
#
#	If archive processing is not restarted then eventually the Ingres
#	Log File will become full and work in the installation will be
#	suspended until log records can be archived.  This situation is
#	known as LOGFULL and is indicated by the LOGFULL status given
#	by logstat.
#
#	Upon termination of archive processing (and the execution of this
#	script), steps should be taken to resolve the archive problem and
#	archiving should be restarted with the Ingres command ingstart -dmfacp.
# 
##
##  History:
##      25-mar-1991 (rogerk)
##              Created as part of Archiver Stability project.
##      03-may-1993 (dianeh)
##              Added DEST ming hint.
##      28-oct-1993 (jnash)
##              Change II_ACP.LOG to iiacp.log.
##      03-mar-1998 (dacri01)
##              Replaced 'iiarchive' with 'ingstart -dmfacp' for OI.
##	29-Apr-2004 (bonro01)
##		Install Ingres as a userid other than 'ingres'.
##	18-Aug-2004 (hanje04)
##		Use new config parameters to determine installation owner
##		instead of directory
##  Ming hint:
##      Build executable with .def extension
#       PROGRAM    = acpexit.def
##      Build it into $ING_BUILD/files
#       DEST       = files
#

#
# Debug method to capture output of acpexit script.  Turn on to trace results
# of script.
#
# exec > /tmp/acpexit.out 2>&1
# set -x


#
# Collect input arguments.
#
error_number=$1
database_name=$2

if test "$error_number" -eq ""
then
    error_number=0
fi


#
# If the archiver termination is NORMAL - due to system shutdown, then ignore
# the termination as it is expected.
#
if test "$error_number" -eq 235600
then
    exit 0
fi


#
# Send mail to Ingres account indicating an archiving problem.
# Include the archiver log in the mail message.
# If the local system's mail program allows a subject header then include one.
#
#Check for install userid, groupid; tm is part of the base package
CONFIG_HOST=`iipmhost`
II_USERID=`iigetres ii.${CONFIG_HOST}.setup.owner.user`
II_GROUPID=`iigetres ii.${CONFIG_HOST}.setup.owner.group`
export II_USERID II_GROUPID

mail_subject="Ingres Archiver processing has stopped : reason $error_number"
if mail $II_USERID < $II_SYSTEM/ingres/files/iiacp.log > \
	/tmp/acpmail.$$ 2>&1
then
    :
else
    #
    # Log that we failed to notify through a mail message.
    #
    echo "ACP acpexit: mail notification failed." >> \
			 $II_SYSTEM/ingres/files/iiacp.log
    cat /tmp/acpmail.$$ >> $II_SYSTEM/ingres/files/iiacp.log
fi
rm /tmp/acpmail.$$

#
# Broadcast messages to appropriate users if desired.
# Cannot use the unix "write" command as it requires the sender
# to be connected to a tty device.  The archiver disconnects from
# its terminal after startup.
#
# write_subject="Ingres Archiver processing has stopped : reason $error_number"
# echo "$write_subject" | wall

#
# Perform special handling for certain termination codes:
#
#
# case $error_number in
# 
#    235602)
#	#
#	#  E_DM9852_ACP_JNL_WRITE_EXIT
#	#
#	# Out of disk space on journal device.
#	# Free up disk space on device and restart archive processing.
#	#
#
#	# Remove file with space that was reserved in case the journal
#	# device become low in order to let the installation proceed
#	# until new space can be found.
#
#	# rm $II_JOURNAL/save_space/block1
#
#	#
#	# Restart archive processing
#	#
#	# ingstart -dmfacp
#
#        ;;
#
#esac

exit 0


#----------------------------------------------------------------------------
#
# Termination error numbers:
#
#   E_DM9850_ACP_NORMAL_EXIT
#
#	Value : 235600		Hex: 39850
#	Message written to Archiver Log:
#	
#		Archiver shutdown has occurred as part of a shutdown of 
#		the Ingres installation.  For further information on the 
#		installation shutdown, refer to the II_RCP log file.
#
#	Description:
#
#		The archiver has terminated upon request of the Logging System.
#		The Logging System will direct the archiver to shut down as part
#		of normal installation shutdown proceedings.
#
#		This message is written by the archiver as part of its shutdown
#		protocols.  It does not indicate an error in archive processing.
#
#
#  E_DM9851_ACP_INITIALIZE_EXIT
#
#	Value : 235601		Hex: 39851
#	Message written to Archiver Log:
#	
#		An error has been encountered during Archiver initialization 
#		which prevents archive processing from starting.  Please make 
#		sure that the Ingres installation is running and that there 
#		is not already an active Archiver.  See the Archiver Log 
#		(iiacp.log) for more detailed information on the failure.
#
#	Description:
#
#		An error occurred during archiver startup and the archiver 
#		could not be initialized.  The Archiver Log (iiacp.log) 
#		should contain more detailed information about the specific 
#		errors encountered.
#
#  E_DM9852_ACP_JNL_WRITE_EXIT
#
#	Value : 235602		Hex: 39852
#	Message written to Archiver Log:
#		Archive processing cannot continue due to the inability to 
#		write journal records for database %0c to the database journal 
#		directory: %1c.  This is probably due to a lack of available 
#		disk space on the journal device.  The error occurred during 
#		an attempt to write to the journal file.  Please check for 
#		disk space and the ability to create/write files in the above 
#		journal directory.  The Archiver Log (iiacp.log) will have 
#		more detailed information about the problem, including the 
#		Operating System's return code from the requested operation.
#		To continue archive processing following resolution of the 
#		above problem, run the Ingres command 'ingstart -dmfacp'.
#	
#	Description:
#		Archiver could not write journal records for indicated database
#		to the journal files.  This is probably due to lack of disk
#		space on the journal device.
#
#		The error may be encountered during an attempt to create a new
#		journal file or extend an old one.  Journal files are numbered
#		sequentially in the database journal location with access done
#		only to the highest number file.
#
#
#  E_DM9853_ACP_RESOURCE_EXIT
#
#	Value : 235603		Hex: 39853
#	Message written to Archiver Log:
#		Archive processing cannot continue due to a failure to 
#		allocate a %0c resource.  Please check the Archiver Log 
#		(iiacp.log) for more detailed information about the resource 
#		required.  This error may occur due to lack of logging, 
#		locking, or memory resources.   If logging or locking 
#		resources have been exhausted the system may need to be 
#		shutdown, reconfigured to allocate more %1c, and then 
#		restarted.  Inability to allocate memory may indicate process 
#		quota limitations.  If it is suspected that the resource 
#		exhaustion was a temporary condition, continuation of archive 
#		processing without system reconfiguration may be attempted by 
#		running 'ingstart -dmfacp'.  If system reconfiguration cannot 
#		be performed immediately, resources may be recovered by 
#		temporarily decreasing the amount of concurrent activity 
#		to the Ingres system.  'ingstart -dmfacp' must be run to 
#		continue archive processing.
#
#	Description:
#		Archiver could not allocate a resource necessary for processing.
#
#		Types of resource failure are:
#
#		Logging - Could not start transaction for archive processing.
#			  Logging system configuration should be altered to
#			  increase maximum number of concurrent transactions.
#		Locking - Could not allocate lock list for archive processing.
#			  Locking system configuration should be altered to
#			  increase the number of lock lists.
#		Memory  - Could not allocate memory for archive processing.
#			  This may be due to process quotas or actual system
#			  resources.  On VMS systems, quotas on the ACP should
#			  be examined to determine if they can be increased
#			  (See the Ingres utility script RUN_ACP.COM).  If
#			  system memory is exhausted, then the system load may
#			  need to be decreased.
#
#		Since most resource exhaustions are temporary and occur at peak
#		loads, the archiver can often be restarted immediately without
#		actually having to reconfigure the system.  Some resources may
#		be recovered by decreasing the amount of concurrent activity to
#		the Ingres system.
#
#
#  E_DM9854_ACP_JNL_ACCESS_EXIT
#
#	Value : 235604		Hex: 39854
#	Message written to Archiver Log:
#		Archive processing cannot continue due to the inability to 
#		access a journal file for database %0c.  Please check the 
#		Archiver Log (iiacp.log) for the type of access which failed, 
#		the name of the journal file accessed and any Operating System 
#		messages describing the type of failure.  Make sure that the 
#		database journal directory (%1c) exists and that the Archiver 
#		has permission to create and write to files in it.  This error 
#		could also be the result of exhaustion of system or process 
#		file resources or quotas.  To continue archive processing 
#		following resolution of the above problem, run the Ingres 
#		command 'ingstart -dmfacp'.
#
#	Description:
#		Archiver could not open/write/close journal records for the 
#		indicated database to the journal files.
#
#		This error may indicate a permission or quota problem.  The 
#		Archiver Log should be reviewed for detailed information 
#		specific to the problem just encountered.
#
#
#  E_DM9855_ACP_LG_ACCESS_EXIT
#
#	Value : 235605		Hex: 39855
#	Message written to Archiver Log:
#		Archive processing cannot continue due an error reading 
#		information from the Logging System. Please check the 
#		Archiver Log (iiacp.log) for the specific operation which 
#		failed and more detailed messages describing the error.  
#		Verify that the Ingres Installation is currently running.  
#		It is possible that failures attempting to read the Ingres 
#		Log File could be caused by process quota limitations; if so, 
#		this should be reflected in the error log.  If the cause of 
#		the error is not apparent, try restarting archive processing 
#		via the Ingres command 'ingstart -dmfacp'.  If the same error is
#		encountered, try stopping and restarting the Ingres 
#		installation.
#	
#	Description:
#		Archiver encountered an error from a call to the Logging 
#		System or in an attempt to read/write transaction log file 
#		records.
#
#		This may indicate a problem with the Ingres Log File or with the
#		information in the Logging System.  First verify that 
#		permissions and quotas for the Archiver are set correctly.
#
#		If the problem is with Logging System information, then 
#		restarting the installation should clear up the problem.
#
#
#  E_DM9856_ACP_DB_ACCESS_EXIT
#
#	Value : 235606		Hex: 39856
#	Message written to Archiver Log:
#		Archive processing cannot continue on database %0c due to a 
#		failure accessing the database config file.  Please check the 
#		Archiver Log (iiacp.log) for further information about the 
#		error encountered.  Check the existence of and permissions on 
#		the file 'aaaaaaaa.cnf' in the database's root DATA location.  
#		If the database config file has been removed or no longer 
#		exists, it must be restored using a backup copy.  A backup 
#		copy of the config file may normally be found in the database 
#		DUMP location.  To continue archive processing following 
#		resolution of the above problem, run the Ingres command 
#		'ingstart -dmfacp'.
#	
#	Description:
#		Archiver could not open/close/write the database config file
#		for the specified database.
#
#		This may indicate a permission or quota problem - or could be
#		caused by non-existence of the config file.
#
#		Most database-specific problems can be bypassed by disabling
#		logging on the failing database in order to get archiver 
#		processing going - then re-enabling journal processing after 
#		the archiver has skipped over the troublesome area of journal 
#		processing.  This can be done through:
#
#			 'alterdb -disable_journaling'  
#
#		Note that this will cause journal records for the database to 
#		be discarded, so that they cannot be used in auditdb or 
#		rollforward processing; the database will no longer be 
#		journaled.  Journaling should be re-enabled as soon as possible 
#		by issuing a new checkpoint: ckpdb +j.
#
#
#  E_DM9857_ACP_JNL_RECORD_EXIT
#
#	Value : 235607		Hex: 39857
#	Message written to Archiver Log:
#		Archive processing cannot continue on database %0c due to an 
#		unexpected log record which the archiver cannot process.  
#		Please check the Archiver Error Log (iiacp.log) for further 
#		information about the error encountered.  This may indicate a 
#		journaling error on the above database which prevents journaled 
#		log records from being moved from the transaction log file to 
#		the database journal file.  If the Archiver Log does not 
#		indicate a solution, try restarting archive processing (using 
#		the 'ingstart -dmfacp' command) to see if the problem is 
#		temporary rather than a symptom of inconsistent journal records.
#		If the same problem is encountered, then journaling should be 
#		disabled on the database to allow the archiver to bypass the 
#		inconsistent journal records.  See the 'alterdb' command for 
#		information on disabling journal processing for a database.
#	
#	Description:
#		A journaling error was encountered while attempting to process
#		a journal record for the specified database.  This probably 
#		indicates an inconsistency in journal information for that db.
#
#		This may be an invalid or unrecognized type of log record, or
#		a log record not associated with a transaction known to the
#		archiver (ie. the Begin Transaction / End Transaction pair for 
#		this transaction were not found).
#
#		Most database-specific problems can be bypassed by disabling
#		logging on the failing database in order to get archiver 
#		processing going - then re-enabling journal processing after 
#		the archiver has skipped over the troublesome area of journal 
#		processing.  This can be done through:
#
#			 'alterdb -disable_journaling'  
#
#		Note that this will cause journal records for the database to 
#		be discarded, so that they cannot be used in auditdb or 
#		rollforward processing; the database will no longer be 
#		journaled.  Journaling should be re-enabled as soon as possible 
#		by issuing a new checkpoint: ckpdb +j.
#
#
#  E_DM9858_ACP_ONLINE_BACKUP_EXIT
#
#	Value : 235608		Hex: 39858
#	Message written to Archiver Log:
#		Archive processing failed due to an error processing records 
#		associated with an Online Checkpoint operation on database 
#		%0c.  Please check the Archiver Log (iiacp.log) for further 
#		information about the error encountered.  Interrupt the 
#		Checkpoint request and issue the command 'ingstart -dmfacp' to 
#		restore archive processing.  Use 'logstat' to check the state of
#		the Ingres installation; verify that the START_ARCHIVER state is
#		turned off.  This indicates that archive processing is 
#		continuing normally.  Reissue the Checkpoint request to backup 
#		the database.  If subsequent attempts to checkpoint the 
#		database fail, try using the -l flag to force an Offline 
#		Checkpoint.
#	
#	Description:
#		The archiver failed due to an error in Online Checkpoint 
#		processing.
#
#		Virtually all errors encountered during online backup are mapped
#		to this error.  The problem may have occurred accessing II_DUMP,
#		database or logging system information, or be due to resource
#		or quota limitations.
#
#		The Archiver Log should give more detailed information as to
#		what operation failed.
#
#		Usually, the archiver can be restarted immediately since the
#		Online Backup operation will have failed.  When archive 
#		processing continues, DUMP processing will not be done as the 
#		checkpoint will no longer be in progress.
#
#		If the Checkpoint error was really an indication of bad archiver
#		problems that were not necessarily related to checkpoint 
#		processing (the error just happened to be encountered in a 
#		checkpoint-specific routine), then attempts to restart the 
#		archiver may also fail.
#
#
#  E_DM9859_ACP_EXCEPTION_EXIT
#
#	Value : 235609		Hex: 39859
#	Message written to Archiver Log:
#		An unexpected exception has halted archive processing.  Please 
#		check the Archiver Log (iiacp.log) for further information 
#		about the exception.  Archive processing should be restarted 
#		with the 'ingstart -dmfacp' command.  If the exception error 
#		continues to occur and prevents any archive work from being 
#		done, the installation should be shut down and restarted to 
#		clear the problem.
#	
#	Description:
#		An exception has forced the archiver to stop.  This will most 
#		likely represent an Ingres bug and should be reported as such 
#		to Ingres Technical Support.
#
#		The Archiver Log File (iiacp.log) should be saved and if 
#		possible, a LOGDUMP should be performed to gather information 
#		to use in tracking down the bug.
#
#		In some cases archive processing can be continued immediately 
#		via the ingstart -dmfacp command.  If the exception error 
#		continues, the installation should be restarted.  If the 
#		exception still continues, then the error is probably being 
#		triggered by some condition in the Log File itself.
#
#		Inspecting the Archiver Log File may give a clue as to whether 
#		the exception occurs on processing of a particular database.  
#		Most database-specific problems can be bypassed by temporarily 
#		disabling logging on the database to allow the archiver to move #		forward past the 'bad' log records.  See the ALTERDB command 
#		for further information on this.  Note that journaling should 
#		be re-enabled as soon as possible since the database is no 
#		longer protected by the journaling system after journaling is 
#		disabled.
#
#		If the problem cannot be isolated to processing for a particular
#		database, or if disabling journaling does not resolve the 
#		exception condition, then the Ingres Log File may need to be 
#		reinitialized to clear the condition.
#
#
#  E_DM985A_ACP_JNL_PROTOCOL_EXIT
#
#	Value : 235610		Hex: 3985A
#	Message written to Archiver Log:
#		Archive processing cannot continue due to an error in protocols 
#		between the Logging System and the Ingres Archiver.  The 
#		archiver encountered a record that it did not expect or did not 
#		find a record that it did expect in the Ingres Log File.  
#		Please check the Archiver Log (iiacp.log) for further 
#		information about the problem.  Archive processing should be 
#		restarted with the 'ingstart -dmfacp' command.  If the problem 
#		persists, the installation should be shut down and restarted.  
#		If the problem still persists, it may indicate a Log File 
#		inconsistency which will require a Log File reinitialization.
#	
#	Description:
#		The archiver encountered a log record or was given LG 
#		information that it did not understand, or did not think was 
#		consistent.
#
#		This error will usually be triggered if the archiver reads a 
#		log record that it cannot match to any journaled database. This 
#		will most likely represent an Ingres bug and should be reported 
#		as such to Ingres Technical Support.
#
#		The Archiver Log File (iiacp.log) should be saved and if 
#		possible, a LOGDUMP should be performed to gather information 
#		to use in tracking down the bug.
#
#		In some cases archive processing can be continued immediately 
#		via the ingstart -dmfacp command.  If the protocol error 
#		continues, the installation should be restarted.
#
#
#  E_DM985B_ACP_INTERNAL_ERR_EXIT
#
#	Value : 235611		Hex: 3985B
#	Message written to Archiver Log:
#		Archive processing cannot continue due to an internal error in 
#		archiver processing.  Please check the Archiver Log 
#		(iiacp.log) for further information about the problem.  
#		Archive processing should be restarted with the 
#		'ingstart -dmfacp' command.  If the problem persists, the 
#		installation should be shut down and restarted.  If the problem
#		still persists, it may indicate a Log File inconsistency which 
#		will require a Log File reinitialization.
#	
#	Description:
#		The archiver encountered an internal error that prevents further
#		processing. This will most likely represent an Ingres bug and 
#		should be reported as such to Ingres Technical Support.
#
#		The Archiver Log File (iiacp.log) should be saved and if 
#		possible, a LOGDUMP should be performed to gather information 
#		to use in tracking down the bug.
#
#		In some cases archive processing can be continued immediately 
#		via the ingstart -dmfacp command.  If the protocol error 
#		continues, the installation should be restarted.
#
#
#  E_DM985C_ACP_DMP_WRITE_EXIT 
#
#	Value : 235612		Hex: 3985C
#	Message written to Archiver Log:
#		Archive processing cannot continue due to the inability to 
#		write dump records during an Online Checkpoint operation on 
#		database %0c to the database dump directory: %1c.  This is 
#		probably due to a lack of available disk space on the dump 
#		device.  The error occurred during an attempt to write to the 
#		dump file.  Please check for disk space and the ability to 
#		create/write files in the above dump directory.  The Archiver 
#		Log (iiacp.log) will have more detailed information about the 
#		problem, including the Operating System's return code from the 
#		requested operation.  The Checkpoint request should be 
#		interrupted and the archive processing restored through the 
#		Ingres command 'ingstart -dmfacp'.
#	
#	Description:
#		Archiver could not write log records for indicated database
#		to the dump files.  This is probably due to lack of disk
#		space on the dump device.
#
#		The error may be encountered during an attempt to create a new
#		dump file or extend an old one.  Dump files are numbered
#		sequentially in the database journal location with access done
#		only to the highest number file.
#
#
#  E_DM985F_ACP_UNKNOWN_EXIT
#
#	Value : 235615		Hex: 3985F
#	Message written to Archiver Log:
#		Archive processing cannot continue due to an internal error 
#		in archiver processing.  Please check the Archiver Log 
#		(iiacp.log) for further information about the problem.  
#		Archive processing should be restarted with the 
#		'ingstart -dmfacp' command.  If the problem persists, the 
#		installation should be shut down and restarted.  If the problem 
#		still persists, it may indicate a Log File inconsistency which 
#               will require a Log File reinitialization.
#	
#	Description:
#		The archiver encountered an internal error that prevents further
#		processing. This will most likely represent an Ingres bug and 
#		should be reported as such to Ingres Technical Support.
#
#		The Archiver Log File (iiacp.log) should be saved and if 
#		possible, a LOGDUMP should be performed to gather information 
#		to use in tracking down the bug.
#
#		In some cases archive processing can be continued immediately 
#		via the ingstart -dmfacp command.  If the protocol error 
#		continues, the installation should be restarted.
#
