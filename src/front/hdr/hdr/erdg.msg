/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	erdg.msg -	Front-End Common Message File.
**
** Description:
**	Front End Common Message File.
**
**	These messages are commonly used throughout the frontends,
**	and the constants defined should be used whenever possible
**	in order to ease the work of the translators.
**
** History:
**	Revision 1.0  22/02/1996
**	Initial revision.
**      02-Feb-2010 (horda03) Bug 121811
**          Fix parameter in open file message
**/
 
#define	_DG_CLASS	229
 
E_DG0001_UNKNOWN_FLAG 		
"Flag used is unknown to the command"
E_DG0002_UNKNOWN_EVSET 		
"The specified evidence set was unknown"
E_DG0003_NO_EVSET	 	
"No evidence set was supplied"
E_DG0004_EXPORT_CREATE 		
"Cannot create export file check premissions etc"
E_DG0005_COPY_FAIL 		
"Failed to copy file"
E_DG0006_NOT_INGRES 		
"Not running as user ingres"
E_DG0007_SYNTAX_ERROR 		
"There was a syntax error in the statement"
E_DG0008_UNKNOWN_ACTION 	
"Action specified not known"
E_DG0009_USAGE 			
"Incorrect use of this command"
E_DG0010_VIEW_BINARY_FAIL
"Not allowed to view binary files"
E_DG0011_OPEN_FAIL 		
"Failed to open file %0c check permissions etc"
E_DG0012_CREATE_FAIL 		
"Failed to create file check permissions etc"
E_DG0013_UNKNOWN_TYPE 		
"The type specified in the input file is not known"
E_DG0014_NOT_INGRES		
"The user this command is being run under is not ingres.\
This command can only be run as ingres"
E_DG0015_EXCEPTION
"Exception occurred"
E_DG0016_NO_II_EXCEPTION
"Environment variable/logical II_EXCEPTION not created or correct"
E_DG0017_EVSET_FULL
"Evidence set directories are full"
E_DG0018_DELETE_FAIL
"Failed to delete the evidence set"
E_DG0019_CREATE_EVSET_FAIL
"Failed to create evidence set"
E_DG0020_PIPE_FAILED
"Pipe Open Failed"
E_DG0021_WRITE_FAILED
"Write failed"
E_DG0022_EVSET_CORRUPT
"Corrupt evidence set"
E_DG0023_EVSET_NOMORE
"No more evidence sets or evidence files can be found"
E_DG0024_PARAM_ERROR
"Filename buffer too short"
E_DG0025_EVSET_FILE_DEL_FAIL
"Failed to delete evidence set file"
