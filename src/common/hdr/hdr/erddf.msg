/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erddf.msg -	Message file for class DF
**
** DDDcription:
**	This file defines the messages for class DF.
**
** History:
**	01-jan-98 (marol01)
**		Created.
**	10-apr-98 (mcgem01)
**		Change product name to Ingres.
**	11-may-98 (mcgem01)
**		Correct two typos in previous submission and add missing
**		messages.
**  27-May-98 (fanra01)
**      Modified to remove error identifier from message.  Also tidied
**      and changed error codes.
**  21-Jul-98 (marol01)
**      Added new messages : E_DF0060_DDS_EXISTING_USER_ROLE and 
**		E_DF0061_DDS_EXISTING_USER_DB
**  05-Jan-2000 (fanra01)
**      Bug 99959
**      Correct spelling of define.
**  04-May-2001 (fanra01)
**      Sir 103813
**      Add the E_DF0065_DDS_INVALID_SESSION message.
**  18-Jun-2002 (fanra01)
**      Bug 108066
**      Update E_DF0046_DDS message text to refer to role rather than profile.
**  20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**      Ingres message file cleanup.
**/

#define		_DF_CLASS	100

E_DF0001_OUT_OF_MEMORY
"Out of memory"

E_DF0002_HASH_ITEM_ALR_EXIST
"Duplicate hash entry detected during an insert operation"

E_DF0005_HASH_NULL_TABLE
"Attempted to use a hash table which has not been created"

E_DF0006_HASH_NOT_NULL_TABLE
"Attempted to remove an existing hash table"

E_DF0007_SEM_CANNOT_CREATE
"Unable to create the semaphore"

E_DF0008_SEM_CANNOT_OPEN
"Unable to open the semaphore"

E_DF0009_SEM_CANNOT_CLOSE
"Unable to close the semaphore"

E_DF0010_SEM_BAD_INIT
"Attempted to use an uninitialized semaphore"

E_DF0011_DDG_INVALID_OBJ_NUM
"Invalid object number"

E_DF0013_DDG_INVALID_PROP_NUM
"Invalid property number or object number out of range"

E_DF0015_DDG_INVALID_STMT_NUM
"Invalid statement number"

E_DF0016_DDG_UNSUPPORTED_TYPE
"Unknown property type not supported"

E_DF0017_DDG_BAD_PARAM
"Invalid parameter properties"

E_DF0018_OPEN_DRV_ERROR
"Unable to locate the data access library for the Data Dictionary Services"

E_DF0019_PROC_DRV_ERROR
"Missing data access library entry point"

E_DF0020_DB_ST_NO_DATA
"No more data available"

E_DF0021_DB_ST_ERROR
"Database error"

E_DF0022_DB_ST_FAILURE
"Database failure"

E_DF0023_DB_ST_INVALID_HANDLE
"Invalid pointer sends to the database"

E_DF0024_DB_ST_OUT_OF_MEMORY
"Database Out of Memory"

E_DF0025_DB_ST_UNKNOWN_ERROR
"Undefined database error"

E_DF0028_DB_ST_NOT_INITIALIZED
"Data access library is unable to accept the request"

E_DF0026_DB_BAD_INIT
"Error occurred during data access library initialization"

E_DF0029_DB_SESSION_NOT_OPENED
"Attempted to issue a query without a connected session"

E_DF0030_DB_SESSION_ALR_OPENED
"Attempted to establish a connection while the session is in use"

E_DF0036_DB_UNSUP_QRY_TYPE
"Encountered an unsupported query type.  The request has not been executed"

E_DF0037_DB_PARAMS_NOT_MATCH
"Invalid parameter declared"

E_DF0038_DB_INVALID_PROP_NUM
"Property number out of range"

E_DF0039_DB_INCORRECT_NAME
"Invalid database name"

E_DF0040_DB_INVALID_TYPE
"Unknown type not supported"

E_DF0042_DDS_MEM_NOT_INIT
"Data Dictionary Security Service was not initialized"

E_DF0043_DDS_BAD_PASSWORD
"Invalid password"

E_DF0044_DDS_BAD_USER
"Invalid user"

E_DF0045_DDS_UNKNOWN_USER
"Undefined user"

E_DF0046_DDS_UNKNOWN_ROLE
"Undefined role"

E_DF0047_DDS_MEM_USER_ALR_REQ
"This user has already been requested"

E_DF0049_DDS_EXISTING_USER
"Attempted to duplicate an existing user entry"

E_DF0050_DDS_EXISTING_ROLE
"Attempted to duplicate an existing role entry"

E_DF0051_DDS_UNKNOWN_DBUSER
"Attempted operation with an invalid user"

E_DF0052_DDS_EXISTING_DBUSER
"Attempted to duplicate an existing database user"

E_DF0053_DDS_INV_ID
"Object Identifier invalid"

E_DF0054_DDS_CRYPT
"Error occurred trying to encode/decode the password"

E_DF0055_DDS_DBUSER_USED
"Attempted to remove a database user entry that is still referenced"

E_DF0056_DDS_ROLE_USED
"Attempted to remove a role entry that is still referenced"

E_DF0057_DDS_EXISTING_DB
"Attempted to duplicate an existing database entry"

E_DF0058_DDS_UNKNOWN_DB
"Undefined database"

E_DF0059_DDS_DB_USED
"Attempted to delete a referenced database connection"

E_DF0060_DDS_EXIST_USER_ROLE
"Attempted to duplicate an existing User/Role relation"

E_DF0061_DDS_EXIST_USER_DB
"Attempted to duplicate an existing User/Database connection relation"

E_DF0062_DB_CANNOT_OPEN
"Error occurred to open the file"

E_DF0063_INCORRECT_KEY
"Object key is invalid"

E_DF0064_UNINITLIZED_DRIVER
"The driver management system was not intialized"

E_DF0065_DDS_INVALID_SESSION
"Invalid session"
