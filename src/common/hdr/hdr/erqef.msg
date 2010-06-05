SQLSTATE_MAPPING_REQUIRED

/*
** Copyright(c) 1993, 2004 Ingres Corporation
**
**
**    Message file for Facility code: _QE
**
**    History:
**
**	16-feb-1993 (dianeh)
**	    Added NO_SQLSTATE_MAPPING to end of file.
**	11-oct-93 (rblumer)
**	    Changed SQLSTATE for E_QE0038_DUPLICATE_RECORD,
**	    E_QE0039_DUPLICATE_KEY and E_QE004D_DUPLICATE_KEY_UPDATE from
**	    SS50001_INVALID_DUP_ROW to SS23501_UNIQUE_CONS_VIOLATION.
**	    Also changed SQLSTATEs for QE0138, QE0139, and QE0144 to be the
**	    appropriate constraint-violation sqlstate.
**	18-oct-93 (rblumer)
**	    Corrected typo on SQLSTATE for error QE0144.
**	25-oct-1993 (dianeh)
**	    Refine text destined for messages.txt file.
**	7-dec-93 (robf)
**          Added E_QE028C_DROP_ALARM_ERR,
**		  E_QE028B_CREATE_ALARM_ERR
**		  E_QE028D_ALARM_DBEVENT_ERR
**	15-dec-93 (rblumer)
**          Changed 230_DMU_TOO_MANY error to 236_DMU_TOO_MANY,
**          since that's what QEF code believes it should be.
**          Also noted that errors 230 through 235 are reserved. 
**	20-jan-94 (rickh)
**	    Added E_QE0307_SESSION_INIT_MO_ERROR.
**	04-mar-94 (andre)
**	    added messages E_QE0146_QSO_JUST_TRANS_ERROR and 
**	    E_QE0147_QSO_DESTROY_ERROR
**	7-mar-94 (robf)
**          Added E_QE019D_INVALID_ROLEGRANT_OP
**      17-may-95 (dilma04)
**          Added I_QE2034_READONLY_TABLE_ERR. 
**      21-aug-1995 (tchdi01)
**          Added production mode related error messages
**              E_QE7610_SPROD_UPD_IIDB
**              E_QE7612_PROD_MODE_ERR
**      19-apr-1996 (stial01)
**          Added E_QE0309_NO_BMCACHE_BUFFERS
**	24-jul-96 (inkdo01)
**	    Added E_QE030A_TTAB_PARM_ERROR for global temptab as
**	    procparm support.
**      23-jul-1996 (ramra01)
**          Added E_QE009D_ALTER_TABLE_SUPP
**          Added E_QE016B_COLUMN_HAS_OBJECTS
**	10-oct-1996 (canor01)
**	    Make messages more generic.
**          Disallow modify of temporary tables to diff page sizes.
**          map the error message E_DM0183_CANT_MOD_TEMP_TABLE to
**	    E_QE009E_CANT_MOD_TEMP_TABLE.
**	3-Apr-98 (thaju02)
**	    Added E_QE011D_PERIPH_CONVERSION.
**	1-apr-98 (inkdo01)
**	    Added QE013A for unique/referential constraint creation 
**	    errors resulting from errors in index creation.
**	17-Sept-98 (johco01)
**	    Changed message associated to error E_QE0236
**	3-feb-99 (inkdo01)
**	    Added QE030C, 30D, 30E for row producing procedures.
**	30-nov-2000 (balst01, gupsh01)
**	    Added new error message W_QE0148, W_QE0149 for the 
**	    errors in iiqef_raat_table_lock internal database
**	    procedure.
**	2-dec-2000 (stephenb)
**	    Fix up (re-format) above messages. Add E_QE014A.
**	18-mar-02 (inkdo01)
**	    Added QE00A0 for "NOCYCLE sequence exceeds max".
**	5-mar-03 (inkdo01)
**	    Added QE00A2 for currval's that precede nextval's.
**      29-Apr-2003 (hanal04) Bug 109682 INGSRV 2155, Bug 109564 INGSRV 2237
**          Added I_QE2037_CONSTRAINT_TAB_IDX_ERR for DDL operations which 
**          have been blocked because they would cause a constraint to be 
**          abandoned.
**	2-jan-04 (inkdo01)
**	    Added QE00A5 for end of rows in partition grouping (internal
**	    condition in QEF).
**	6-Feb-2004 (schka24)
**	    Remove QE0236, no longer happens.
**	09-Apr-2004 (jenjo02)
**	    Added QE007F_ERROR_UNFIXING_PAGES for DMR_UNFIX.
**	02-nov-2004 (devjo01)
**	    Corrected misspelling (encounted) in E_QE0134_LISTFILE_ERROR.
**	11-Nov-2004 (gorvi01)
**	    Added E_QE028E_DB_NOT_UNEXTENDED for unextenddb.
**      10-may-2005 (raodi01) bug110709 INGSRV2470
**          Added E_QE00A3_MOD_TEMP_TABLE_KEY
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	29-may-06 (dougi)
**	    Added QE00C0 to display information about database procedure
**	    errors.
**	26-Jun-2006 (jonj)
**	    Add QE005C for XA transactions.
**	07-sep-06 (toumi01)
**	    Add hint about HELP PROCEDURE to QE00C0.
**	31-jan-2007 (dougi)
**	    Add QE00A6 for scrollable cursors.
**	9-apr-2007 (dougi)
**	    Add QE00A7 for keyset scrollable cursors in which row was
**	    updated/deleted.
**	18-dec-2007 (dougi)
**	    Add QE00A8 for bad values of first/offset "n".
**	30-jan-2008 (dougi)
**	    Add QE00A9 for bad offset "n" and fixed previous msg.
**      09-Sep-2008 (gefei01)
**          Add QE0310 for table procedure.
**      21-Jan-2009 (horda03) Bug 121519
**            Add E_QE00AA_ERROR_CREATING_TABLE.
**	19-Mar-2010 (gupsh01) SIR 123444
**	    Add E_QE016C_RENAME_TAB_HAS_OBJS for rename tables.
*/


#define _QE_CLASS   6


E_QE0000_OK:SS00000_SUCCESS
"QEF successful return."
E_QE0001_INVALID_ALTER:SS42000_SYN_OR_ACCESS_ERR
"Invalid parameter in alter QEF setting array."
E_QE0002_INTERNAL_ERROR:SS50006_FATAL_ERROR
"A QEF internal error occurred.
\n Associated error messages which provide more detailed information about the
 problem can be found in the error log (errlog.log)"
E_QE0003_INVALID_SPECIFICATION:SS42000_SYN_OR_ACCESS_ERR
"An out-of-range parameter value was specified."
E_QE0004_NO_TRANSACTION:SS25000_INV_XACT_STATE
"No transaction is currently in progress."
E_QE0005_CANNOT_ESCALATE:SS25000_INV_XACT_STATE
"Cannot convert the internal transaction to an SST or MST."
E_QE0006_TRANSACTION_EXISTS:SS25000_INV_XACT_STATE
"A transaction is currently in progress."
E_QE0007_NO_CURSOR:SS24000_INV_CURS_STATE
"Single table cursor is not open."
E_QE0008_CURSOR_NOT_OPENED:SS24000_INV_CURS_STATE
"The cursor specified has not been opened."
E_QE0009_NO_PERMISSION:SS42503_INSUF_PRIV
"The specified cursor does not support the requested action."
E_QE000A_READ_ONLY:SS24000_INV_CURS_STATE
"The cursor was opened read-only."
E_QE000B_ADF_INTERNAL_ERROR:SS50000_MISC_ERRORS
"An internal error occurred while executing an ADF function."
E_QE000C_CURSOR_ALREADY_OPENED:SS24000_INV_CURS_STATE
"The cursor is already opened."
E_QE000D_NO_MEMORY_LEFT:SS50003_EXHAUSTED_RESOURCE
"QEF is out of memory."
E_QE000E_ACTIVE_COUNT_EXCEEDED:SS50003_EXHAUSTED_RESOURCE
"There are too many active query plans in QEF."
E_QE000F_OUT_OF_OTHER_RESOURCES:SS50003_EXHAUSTED_RESOURCE
"QEF is out of resources."
E_QE0010_DUPLICATE_ROW:SS42502_DUPL_OBJECT
"This update duplicates a row in the table."
E_QE0011_AMBIGUOUS_REPLACE:SS27000_TRIG_DATA_CHNG_ERR
"Attempted to update the same row twice."
E_QE0012_DUPLICATE_KEY:SS42502_DUPL_OBJECT
"This update duplicates a key in the table."
E_QE0013_INTEGRITY_FAILED:SS23000_CONSTR_VIOLATION
"Update failed because an integrity constraint was violated."
E_QE0014_NO_QUERY_PLAN:SS50000_MISC_ERRORS
"The specified query plan is unknown to QEF."
E_QE0015_NO_MORE_ROWS:SS02000_NO_DATA
"No more rows are available."
E_QE0016_INVALID_SAVEPOINT:SS42000_SYN_OR_ACCESS_ERR
"Nonexistent savepoint."
E_QE0017_BAD_CB:SS50000_MISC_ERRORS
"An invalid control block was passed to QEF."
E_QE0018_BAD_PARAM_IN_CB:SS50000_MISC_ERRORS
"Illegal parameter in control block."
E_QE0019_NON_INTERNAL_FAILURE:SS50000_MISC_ERRORS
"An error occurred in a routine called by QEF."
E_QE001A_DATATYPE_OVERFLOW:SS5000B_INTERNAL_ERROR
"Datatype overflow."
E_QE001B_COPY_IN_PROGRESS:SS50000_MISC_ERRORS
"A copy command is currently in progress."
E_QE001C_NO_COPY:SS50000_MISC_ERRORS
"An error in the copy command occurred."
E_QE001D_FAIL:SS50000_MISC_ERRORS
"General failure in QEF (%0x).
\n Associated error messages which provide more detailed information about the
problem can be found in the error log (errlog.log)"
E_QE001E_NO_MEM:SS50003_EXHAUSTED_RESOURCE
"Unable to allocate memory from SCF."
E_QE001F_INVALID_REQUEST:SS50000_MISC_ERRORS
"An unsupported function was requested."
E_QE0020_OPEN_CURSOR:SS24000_INV_CURS_STATE
"A cursor is currently open."
E_QE0021_NO_ROW:SS24000_INV_CURS_STATE
"The cursor is not currently positioned on a row."
E_QE0022_QUERY_ABORTED:SS40001_SERIALIZATION_FAIL
"The query has been aborted."
E_QE0023_INVALID_QUERY:SS5000B_INTERNAL_ERROR
"This query plan is no longer valid."
E_QE0024_TRANSACTION_ABORTED:SS50003_EXHAUSTED_RESOURCE
"The transaction log file is full.  The transaction will be aborted."
E_QE0025_USER_ERROR:SS5000B_INTERNAL_ERROR
"A user error has occurred."
E_QE0026_DEFERRED_EXISTS:SS24000_INV_CURS_STATE
"Only one deferred cursor can be open at a time."
E_QE0027_BAD_QUERY_ID:SS50007_INVALID_SQL_STMT_ID
"Query identifier is not valid."
E_QE0028_BAD_TABLE_NAME:SS42506_INVALID_IDENTIFIER
"Table name is not valid."
E_QE0029_BAD_OWNER_NAME:SS28000_INV_AUTH_SPEC
"Owner name is not valid."
E_QE002A_DEADLOCK:SS40001_SERIALIZATION_FAIL
"Deadlock detected."
E_QE002B_TABLE_EXISTS:SS42502_DUPL_OBJECT
"The operation specified tried to create a table that already exists."
E_QE002C_BAD_ATTR_TYPE:SS42000_SYN_OR_ACCESS_ERR
"Invalid attribute type specified."
E_QE002D_BAD_ATTR_NAME:SS42506_INVALID_IDENTIFIER
"A bad attribute name was specified."
E_QE002E_BAD_ATTR_SIZE:SS42000_SYN_OR_ACCESS_ERR
"Invalid size for specified attribute type."
E_QE002F_BAD_ATTR_PRECISION:SS42000_SYN_OR_ACCESS_ERR
"Invalid precision for specified attribute type."
E_QE0030_BAD_LOCATION_NAME:SS5000B_INTERNAL_ERROR
"The specified location name is invalid."
E_QE0031_NONEXISTENT_TABLE:SS42500_TBL_NOT_FOUND
"The operation required the use of a table that does not exist."
E_QE0032_BAD_SAVEPOINT_NAME:SS42506_INVALID_IDENTIFIER
"An invalid savepoint name was specified."
E_QE0033_BAD_TID:SS42000_SYN_OR_ACCESS_ERR
"An invalid TID was specified."
E_QE0034_LOCK_QUOTA_EXCEEDED:SS50003_EXHAUSTED_RESOURCE
"The locking mechanism is out of locks."
E_QE0035_LOCK_RESOURCE_BUSY:SS50003_EXHAUSTED_RESOURCE
"The requested lock is not available."
E_QE0036_LOCK_TIMER_EXPIRED:SS50003_EXHAUSTED_RESOURCE
"The requested lock could not be granted in during the time allotted."
E_QE0037_ERROR_DELETING_RECORD:SS50000_MISC_ERRORS
"An error occurred while deleting a record."
E_QE0038_DUPLICATE_RECORD:SS23501_UNIQUE_CONS_VIOLATION
"Duplicate records were found."
E_QE0039_DUPLICATE_KEY:SS23501_UNIQUE_CONS_VIOLATION
"Duplicate primary keys were found."
E_QE003A_FOPEN_FROM:SS50000_MISC_ERRORS
"An error occurred opening the file in a COPY FROM command."
E_QE003B_FOPEN_TO:SS50000_MISC_ERRORS
"An error occurred opening the file in a COPY INTO command."
E_QE003C_COLUMN_TOO_SMALL:SS42000_SYN_OR_ACCESS_ERR
"A column is too small for data conversion in the COPY command."
E_QE003D_BAD_INPUT_STRING:SS42000_SYN_OR_ACCESS_ERR
"The input string is not valid for the COPY command."
E_QE003E_UNEXPECTED_EOF:SS5000B_INTERNAL_ERROR
"An unexpected end of file was encountered while reading the input file."
E_QE003F_UNTERMINATED_C0:SS5000B_INTERNAL_ERROR
"An unterminated c0 field was encountered."
E_QE0040_NO_FULL_PATH:SS42000_SYN_OR_ACCESS_ERR
"The full path of the file must be specified."
E_QE0041_DUPLICATES_IGNORED:SS01000_WARNING
"Duplicate tuples were encountered and ignored."
E_QE0042_CONTROL_TO_BLANK:SS01000_WARNING
"Control characters were converted to blanks."
E_QE0043_TRUNC_C0:SS22001_STRING_RIGHT_TRUNC
"A c0 format field was truncated."
E_QE0044_ILLEGAL_FTYPE:SS42000_SYN_OR_ACCESS_ERR
"An illegal file type was specified."
E_QE0045_BINARY_F_T0:SS42000_SYN_OR_ACCESS_ERR
"The use of the Text(0) format requires the use of binary files."
E_QE0046_ERROR_ON_ROW:SS50000_MISC_ERRORS
"An error occurred processing row."
E_QE0047_INVALID_CHAR:SS5000B_INTERNAL_ERROR
"Invalid character value found while processing row."
E_QE0048_TUP_TOO_WIDE:SS50002_LIMIT_EXCEEDED
"Tuple is too wide for query processing."
E_QE0049_ALL_KEY:SS42000_SYN_OR_ACCESS_ERR
"All keys must be specified for this MODIFY operation."
E_QE004A_KEY_SEQ:SS42000_SYN_OR_ACCESS_ERR
"An illegal key sequence was specified."
E_QE004B_NOT_ALL_ROWS:SS5000B_INTERNAL_ERROR
"Could not SELECT all rows from the subselect."
E_QE004C_NOT_ZEROONE_ROWS:SS21000_CARD_VIOLATION
"Subselect returned more that one row."
E_QE004D_DUPLICATE_KEY_UPDATE:SS23501_UNIQUE_CONS_VIOLATION
"The requested update was not performed because it would duplicate a
key existing in the table."
E_QE004E_ULH_INIT_FAILURE:SS50000_MISC_ERRORS
"An error occurred while initializing ULH for the DSH environment."
E_QE0050_TEMP_TABLE_EXISTS:SS50000_MISC_ERRORS
"An error occurred trying to create a temporary table."
E_QE0051_TABLE_ACCESS_CONFLICT:SS5000B_INTERNAL_ERROR
"Table access conflict, table opened twice."
E_QE0052_RESOURCE_QUOTA_EXCEEDED:SS50003_EXHAUSTED_RESOURCE
"Out of disk space, disk quota, or open file quota."
E_QE0053_TRAN_ID_NOT_UNIQUE:SS50000_MISC_ERRORS
"The distributed transaction ID is not unique in the logging system."
E_QE0054_DIS_TRAN_UNKNOWN:SS50000_MISC_ERRORS
"The distributed transaction ID is unknown to the logging system."
E_QE0055_DIS_TRAN_OWNER:SS50000_MISC_ERRORS
"The distributed transaction is owned by another server in the logging system.\n
Disconnect the previous association and try to re-connect later."
E_QE0056_CANT_SECURE_IN_CLUSTER:SS0A000_FEATUR_NOT_SUPPORTED
"Can't support two-phase commit protocol in a cluster installation."
E_QE0057_ERROR_RESUME_TRAN:SS50000_MISC_ERRORS
"Error in resuming a transaction."
E_QE0058_ERROR_SECURE_TRAN:SS50000_MISC_ERRORS
"Error in securing a transaction."
E_QE0059_ILLEGAL_STMT:SS50000_MISC_ERRORS
"Illegal statement specified. Only ROLLBACK or COMMIT statement is allowed \n
immediately after the PREPARE TO COMMIT statement."
E_QE005A_DIS_DB_UNKNOWN:SS50000_MISC_ERRORS
"The database connected by the distributed transaction is unknown to the logging system."
E_QE005C_INVALID_TRAN_STATE:SS25000_INV_XACT_STATE
"The state of the transaction does not permit this operation."
E_QE005F_BAD_DB_ACCESS_MODE:SS42000_SYN_OR_ACCESS_ERR
"Bad database access mode."
E_QE0060_BAD_DB_ID:SS42506_INVALID_IDENTIFIER
"Bad database identifier."
E_QE0061_BAD_DB_NAME:SS42506_INVALID_IDENTIFIER
"Bad database name."
E_QE0062_BAD_INDEX:SS50000_MISC_ERRORS
"Corrupt index table found."
E_QE0063_BAD_TABLE_OWNER:SS42506_INVALID_IDENTIFIER
"Bad table owner."
E_QE0064_BAD_TRAN_ID:SS42506_INVALID_IDENTIFIER
"Bad transaction identifier."
E_QE0065_DB_ACCESS_CONFLICT:SS5000B_INTERNAL_ERROR
"Database access conflict."
E_QE0066_DB_OPEN:SS50000_MISC_ERRORS
"Database is already open."
E_QE0067_DB_OPEN_QUOTA_EXCEEDED:SS50003_EXHAUSTED_RESOURCE
"Open database quota exceeded."
E_QE0068_DB_QUOTA_EXCEEDED:SS50003_EXHAUSTED_RESOURCE
"Server database quota exceeded."
E_QE0069_DELETED_TID:SS50000_MISC_ERRORS
"Tuple at tid already deleted."
E_QE006A_NONEXISTENT_DB:SS42505_OBJ_NOT_FOUND
"Nonexistent database."
E_QE006B_SESSION_QUOTA_EXCEEDED:SS50003_EXHAUSTED_RESOURCE
"Server quota on sessions exceeded."
E_QE006C_CANT_MOD_CORE_SYSCAT:SS42503_INSUF_PRIV
"User does not have enough privilege to modify the core system catalogs:\n
        iirelation, iiattribute, iiindexes, ii_relidx, iidevices."
E_QE006D_CANT_INDEX_CORE_SYSCAT:SS42503_INSUF_PRIV
"User cannot place an index on the core system catalogs:\n
        iirelation, iiattribute, iiindexes, ii_relidx, iidevices."
E_QE006E_TRAN_IN_PROGRESS:SS25000_INV_XACT_STATE
"Transaction already in progress."
E_QE006F_TRAN_NOT_IN_PROGRESS:SS25000_INV_XACT_STATE
"Transaction in not in progress."
E_QE0070_TRAN_QUOTA_EXCEEDED:SS50003_EXHAUSTED_RESOURCE
"Session transaction quota exceeded."
E_QE0071_TRAN_TABLE_OPEN:SS50000_MISC_ERRORS
"Table still open at end of transaction."
E_QE0072_LOCATIONS_TOO_MANY:SS50003_EXHAUSTED_RESOURCE
"Number of locations specified exceeds the maximum allowed for this operation."
E_QE0074_BAD_TABLE_CREATE:SS50000_MISC_ERRORS
"Could not create table."
E_QE0075_BAD_KEY_DIRECTION:SS42000_SYN_OR_ACCESS_ERR
"Illegal key direction."
E_QE0076_BTREE_BAD_KEY:SS42000_SYN_OR_ACCESS_ERR
"Invalid key length for btree table."
E_QE0077_LOCATION_EXISTS:SS42502_DUPL_OBJECT
"Location already exists."
E_QE0078_ERROR_ADDING_DB:SS50000_MISC_ERRORS
"Error trying to add database to server."
E_QE0079_ERROR_DELETING_DB:SS50000_MISC_ERRORS
"Error deleting database from server."
E_QE007A_ERROR_OPENING_DB:SS50000_MISC_ERRORS
"Error opening database in server."
E_QE007B_ERROR_CLOSING_DB:SS50000_MISC_ERRORS
"Error closing database in server."
E_QE007C_ERROR_GETTING_RECORD:SS50000_MISC_ERRORS
"Error trying to get a record.
\n Associated error messages which provide more detailed information about the
problem can be found in the error log (errlog.log)"
E_QE007D_ERROR_PUTTING_RECORD:SS50000_MISC_ERRORS
"Error trying to put a record."
E_QE007E_ERROR_REPLACING_RECORD:SS50000_MISC_ERRORS
"Error trying to replace a record."
E_QE007F_ERROR_UNFIXING_PAGES:SS50000_MISC_ERRORS
"Error trying to unfix a table's pages."
E_QE0080_ERROR_POSITIONING:SS50000_MISC_ERRORS
"Error trying to position a table."
E_QE0081_ERROR_OPENING_TABLE:SS50000_MISC_ERRORS
"Error opening a table."
E_QE0082_ERROR_CLOSING_TABLE:SS50000_MISC_ERRORS
"Error closing a table."
E_QE0083_ERROR_MODIFYING_TABLE:SS50000_MISC_ERRORS
"Error modifying a table."
E_QE0084_ERROR_INDEXING_TABLE:SS50000_MISC_ERRORS
"Error creating an index for a table."
E_QE0085_ERROR_SORTING:SS50000_MISC_ERRORS
"Error sorting data."
E_QE0086_ERROR_BEGINNING_TRAN:SS50000_MISC_ERRORS
"Error beginning a transaction."
E_QE0087_ERROR_COMMITING_TRAN:SS50000_MISC_ERRORS
"Error committing a transaction."
E_QE0088_ERROR_ABORTING_TRAN:SS50000_MISC_ERRORS
"Error aborting a transaction."
E_QE0089_ERROR_SAVEPOINTING:SS50000_MISC_ERRORS
"An error occurred while trying to create a savepoint."
E_QE008A_BAD_TABLE_DESTROY:SS50000_MISC_ERRORS
"Error trying to destroy a table."
E_QE008B_CANT_OPEN_VIEW:SS50000_MISC_ERRORS
"Illegal attempt was made to open a view, base table(s) must be opened."
E_QE008C_LOCK_MANAGER_ERROR:SS50000_MISC_ERRORS
"Error creating a server lock list."
E_QE008D_BAD_SYSCAT_MOD:SS50000_MISC_ERRORS
"Error trying to modify core system catalogs:\n
         iirelation,iiattribute, iiindexes, ii_relidx, iidevices."
E_QE008E_ERROR_RELOCATING_TABLE:SS50000_MISC_ERRORS
"An error occurred trying to relocate a table."
E_QE008F_ERROR_DUMPING_DATA:SS50000_MISC_ERRORS
"An error occurred trying to truncate the data of a temporary file."
E_QE0090_ERROR_LOADING_DATA:SS50000_MISC_ERRORS
"An error occurred while trying to load data into a file."
E_QE0091_ERROR_ALTERING_TABLE:SS50000_MISC_ERRORS
"An error occurred while alter the characteristics of a table."
E_QE0092_ERROR_SHOWING_TABLE:SS50000_MISC_ERRORS
"An error occurred while showing information about a table."
E_QE0093_ERROR_ALTERING_DB:SS50000_MISC_ERRORS
"An error occurred while altering the characteristics of a database."
E_QE0094_ISAM_BAD_KEY_LENGTH:SS50000_MISC_ERRORS
"Invalid key length for isam table."
E_QE0095_COMP_BAD_KEY_LENGTH:SS50000_MISC_ERRORS
"Invalid key length for compressed table."
E_QE0096_MOD_IDX_UNIQUE:SS50001_INVALID_DUP_ROW
"It is not allowed to modify a secondary index with non-unique keys
to a unique structure."
E_QE0097_NOT_A_VIEW:SS42000_SYN_OR_ACCESS_ERR
"Error trying to retrieve the query id for a relation which is not a view."
E_QE0098_SEM_FAILS:SS50000_MISC_ERRORS
"Error trying to acquire or to release QEF semaphore."
E_QE0099_DB_INCONSISTENT:SS50000_MISC_ERRORS
"Database becomes inconsistent.  The transaction will be aborted"
E_QE009A_DUP_LOCATION_NAME:SS42502_DUPL_OBJECT
"The same location has been specified twice."
E_QE009B_LOCATION_LIST_ERROR:SS42000_SYN_OR_ACCESS_ERR
"The two location lists specified for table relocation have different number
of entries."
E_QE009C_UNKNOWN_ERROR:SS50000_MISC_ERRORS
"Unexpected error received from another facility.  Check the server error log."
E_QE009D_ALTER_TABLE_SUPP:SS50000_MISC_ERRORS
"Alter Table ADD/DROP column support for pages greater than 2k only."
E_QE009E_CANT_MOD_TEMP_TABLE:SS50000_MISC_ERRORS
"Cannot modify temporary tables to different page sizes."
E_QE00A0_SEQ_LIMIT_EXCEEDED:SS2200H_SEQ_EXCEEDED
"Next value requested for a NOCYCLE sequence that exceeded the boundary value."
E_QE00A1_IISEQ_NOTDEFINED:SS50000_MISC_ERRORS
"An attempt to create/alter/drop a sequence was made on a database\n
that does not support sequences."
E_QE00A3_MOD_TEMP_TABLE_KEY:SS50000_MISC_ERRORS
"Cannot modify temporary tables with key compression"
/*%
** Parameters:
**	none.
**
** Explanation:
**	A sequence DDL statement was issued on a database created before
**	the introduction of the sequence feature. Sequences can only be
**	defined in newly created or upgraded (to 2.65 or later) databases.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	If this is Ingres release 2.65 or later, run upgradedb on the 
**	database before attempting to create a sequence. Otherwise, sequences
**	can only be created on newly created databases.
*/
E_QE00A2_CURRVAL_NONEXTVAL:SS50000_MISC_ERRORS
"An attempt to issue a CURRENT VALUE request on a sequence was made \n
before issuing a NEXT VALUE request."
/*%
** Parameters:
**	none.
**
** Explanation:
**	The CURRENT VALUE (or CURRVAL) operator may only be used after a
**	NEXT VALUE (or NEXTVAL) operator has been used on the same sequence
**	in the same transaction.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Assure that a NEXT VALUE operator is requested in the same statement
**	or in a previously executed statement in the transaction.
*/
E_QE00A5_END_OF_PARTITION:SS02000_NO_DATA
"No more rows are available from current partition grouping."
/*%
** Parameters:
**	none.
**
** Explanation:
**	An attempt was made (typically with a scrollable cursor) to fetch
**	a row from a position outside the result set. Can also occur when
**	a FETCH is issued after the last row has already been fetched. 
**	The result is a "no data" SQLSTATE condition.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Assure that FETCH statements reference rows in the result set.
*/
E_QE00A6_NO_DATA:SS02000_NO_DATA
"Attempt was made to fetch a row from outside the range of the result set."
E_QE00A7_ROW_DELETED:SS02000_NO_DATA
"Attempt was made to fetch a row updated or deleted from the result set."
E_QE00A8_BAD_FIRSTN:SS42000_SYN_OR_ACCESS_ERR
"A first n value less than or equal 0 was specified. It must be > 0."
E_QE00A9_BAD_OFFSETN:SS42000_SYN_OR_ACCESS_ERR
"An offset n value less than 0 was specified. It must be >= 0."
E_QE00AA_ERROR_CREATING_TABLE:SS50000_MISC_ERRORS
"Creation of the table failed."
/*%
** Parameters:
**      none.
**
** Explanation:
**      An attempt to create a table has failed. The reason for the
**      failure will be listed in the ERRLOG. The expected reason is
**      that the maximum number for a table ID has been reached.
**
** System Status:
**      The query is aborted.
**
** Recommendation:
**      If E_DM0021 is the cause, unload the DB and recreate. Consider
**      modifying applications to use Global Temporary Tables in place
**      of "temporary" permanent ones.
*/
E_QE00C0_PROC_STMTNO:SS42000_SYN_OR_ACCESS_ERR
"The following error occurred executing statement %0d (%1c action) 
of procedure %2c. Use HELP PROCEDURE to view statement numbers."
/*%
** Parameters:
**	%0n - line number of procedure currently being executed
**	%1c - action header type
**	%2c - name of procedure being executed
**
** Explanation:
**	This error is issued in conjunction with any other error generated
**	by the execution of a database procedure. The intent is to accurately
**	identify the procedure statement that caused the error.
**
** System Status:
**	The "EXECUTE PROCEDURE" statement is aborted.
**	If this is a rule invocation, the original query triggering the rule
**	is also aborted with a fatal error.
**
** Recommendation:
**	Fix the database procedure.
*/
E_QE0100_CREATE_DSH_OBJECT:SS50000_MISC_ERRORS
"Error trying to create DSH object in ULH table."
E_QE0101_DESTROY_DSH_OBJECT:SS50000_MISC_ERRORS
"Error trying to destroy DSH object in ULH table."
E_QE0102_CATALOG_DSH_OBJECT:SS50000_MISC_ERRORS
"Error trying to catalog DSH object into a ULH class."
E_QE0103_ACCESS_DSH_OBJECT:SS50000_MISC_ERRORS
"Error trying to access DSH object in ULH table."
E_QE0104_RELEASE_DSH_OBJECT:SS50000_MISC_ERRORS
"Error trying to release DSH object in ULH table."
E_QE0105_CORRUPTED_CB:SS50000_MISC_ERRORS
"Internal call block corrupted in the QEF procedure '%0c'.\n
The internal diagnostics are: %1c."
/*%
** Parameters:
**	%0c - procedure name
**	%1c - internal diagnostics
**
** Explanation:
**	This error is an internal system error.  Please report this error
**	within the context in which it was generated.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Report this error, including the internal diagnostics, to
**	Technical Support.
*/
E_QE0110_NO_ROWS_QUALIFIED:SS02000_NO_DATA
"No rows satisfied the specified qualification."
E_QE0111_ERROR_SENDING_MESSAGE:SS50000_MISC_ERRORS
"Error trying to send a database procedure message to the user's application."
E_QE0113_PARAM_CONVERSION:SS42000_SYN_OR_ACCESS_ERR
"Error converting value for the database procedure parameter '%0c'.
The formal and actual parameter types referenced by an EXECUTE PROCEDURE statement
or database rule invocation are not compatible."
E_QE0114_PARAM_MISSING:SS42000_SYN_OR_ACCESS_ERR
"The values for one or more parameters of the database procedure are missing.
Neither default nor null values have been specified for the parameter in an
EXECUTE PROCEDURE statement or database rule invocation."
/*
** 15-aug-91 (markg)
**	This message is now obsolete, it has been replaced by
**	E_QE011B_PARAM_MISSING.
*/
E_QE0115_TABLES_MODIFIED: SS50000_MISC_ERRORS
"A table referenced in the currently executing database procedure
was modified in such a way to invalidate the procedure plan.  The table
modification may be due to transaction related statements (COMMIT or
ROLLBACK), transaction related errors (dead-lock or force-abort) or
transaction context upon initial entry to the procedure (open transaction
or cursors).  Procedure execution is terminated."
E_QE0116_CANT_ALLOCATE_DBP_ID:SS50000_MISC_ERRORS
"Error allocating a database-wide unique id for the database procedure."
E_QE0117_NONEXISTENT_DBP:SS42505_OBJ_NOT_FOUND
"Procedure not found when dropping or modifying procedure attributes."
E_QE0118_COMPRESSION_CONFLICT:SS50000_MISC_ERRORS
"Table must be modified to take advantage of new tuple compression routines."
E_QE011A_NOT_ALLOWED_IN_MST:SS25000_INV_XACT_STATE
"The no recovery option on the COPY statement is not allowed within a
mulit-statement transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The "COPY" statement included the "WITH RECOVERY=FALSE" qualifier,
**	which is illegal inside a multi-statement transaction.
**
** System Status:
**	Current statement is rejected.
**
** Recommendation:
**	Reissue the "COPY" statement, either leaving out the
**	"WITH RECOVERY=FALSE" qualifier or make sure this is the only
**	statement inside the transaction.
**
*/
E_QE011B_PARAM_MISSING:SS42000_SYN_OR_ACCESS_ERR
"The value for parameter '%0c' of the database procedure '%1c' is missing.
Neither default nor null values have been specified for the parameter in an
EXECUTE PROCEDURE statement or database rule invocation."
/*%
** Parameters:
**	%0c - database procedure parameter
**	%1c - database procedure name
**
** Explanation:
**	A database procedure has been invoked (either directly or through a
**	rule) with either a missing parameter or a parameter name mismatch.
**
** System Status:
**	The query will be aborted.
**
** Recommendation:
**	Reissue the call to the database procedure with the correct parameters.
*/
E_QE011C_NONEXISTENT_SYNONYM:SS42505_OBJ_NOT_FOUND
"DROP SYNONYM: Schema '%0c' does not contain synonym '%1c'."
/*%
** Parameters:
**	%0c - schema name
**	%1c - synonym name
**
** Explanation:
**	The synonym specified by the user does not exist.  This error is likely
**	to happen if the RDF cache entry describing the synonym is stale.
** 
** System Status:
**	The DBMS will attempt to drop other synonyms (if any) named in the
**	statement.
**
** Recommendation:
**	None.
*/
E_QE011D_PERIPH_CONVERSION:SS42000_SYN_OR_ACCESS_ERR
"Error converting value for the database procedure parameter '%0c'.
A peripheral object parameter is associated with an EXECUTE 
PROCEDURE statement or database rule invocation.  
Please see errlog.log for more information."
E_QE0123_PARAM_INVALID:SS42000_SYN_OR_ACCESS_ERR
"The database procedure parameter '%0c' is invalid.
The parameter name does not exist in the procedure referenced by an
EXECUTE PROCEDURE statement or database rule invocation."
/*%
** Parameters:
**	%0c - database procedure parameter
**
** Explanation:
**	This error will occur when executing a procedure via the "EXECUTE
**	PROCEDURE" statement or via the firing of a rule.
**
** System Status:
**	If this is an "EXECUTE PROCEDURE" statement, the statement is aborted.
**	If this is a rule invocation, the original query triggering the rule
**	is aborted with a fatal error.
**
** Recommendation:
**	Correct the parameter name in the "EXECUTE PROCEDURE" statement or
**	"EXECUTE PROCEDURE" clause in the "CREATE RULE" statement.
*/
E_QE0124_PARAM_MANY:SS42000_SYN_OR_ACCESS_ERR
"A database procedure parameter has been specified more than once
in an EXECUTE PROCEDURE statement or database rule invocation. There are
%0d actual parameter(s) and %1d formal parameter(s)."
/*%
** Parameters:
**	%0d - number of actual parameters passed to the procedure invocation
**	%1d - number of formal parameters specified in the procedure definition
**
** Explanation:
**	This error will occur when executing a procedure via the "EXECUTE
**	PROCEDURE" statement or via the firing of a rule.
**
** System Status:
**	If this is an "EXECUTE PROCEDURE" statement, the statement is aborted.
**	If this is a rule invocation, the original query triggering the rule
**	is aborted with a fatal error.
**
** Recommendation:
**	Remove the duplicate parameter instance in the "EXECUTE PROCEDURE"
**	statement or "EXECUTE PROCEDURE" clause in the "CREATE RULE" statement.
*/
E_QE0130_NONEXISTENT_IPROC:SS50000_MISC_ERRORS
"Error:  Specified internal procedure does not exist."
/*%
** Parameters:
**	None.
**
** Explanation:
**	No internal procedure by the specified name exists.
**
** System Status:
**	The "EXECUTE PROCEDURE" statement is aborted.
**
** Recommendation:
**	The internal procedure "user" is a system-supplied frontend
**	program. It should not generate internal procedure errors; please
**	contact Ingres Corporation Technical Support.
*/
E_QE0131_MISSING_IPROC_PARAM:SS50000_MISC_ERRORS
"Error:  mismatch between expected and actual parameters for Internal Procedure"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The internal procedure that QEF knows expects a different number of
**	parameters than it is being supplied.
**
** System Status:
**	The "EXECUTE PROCEDURE" statement is aborted.
**
** Recommendation:
**	The internal procedure "user" is a system-supplied frontend
**	program. It should not generate internal procedure errors; please
**	contact Ingres Corporation Technical Support.
*/
E_QE0132_WRONG_PARAM_NAME:SS50000_MISC_ERRORS
"Error:  Invalid parameter name supplied to Internal Procedure"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The internal procedure that QEF knows expects certain parameter names.
**	One or more of the supplied parameter names does not match what QEF
**	expects.
**
** System Status:
**	The "EXECUTE PROCEDURE" statement is aborted.
**
** Recommendation:
**	The internal procedure "user" is a system-supplied frontend
**	program. It should not generate internal procedure errors; please
**	contact Ingres Corporation Technical Support.
*/
E_QE0133_WRONG_PARAM_TYPE:SS50000_MISC_ERRORS
"Error:  Invalid parameter supplied to Internal Procedure"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The internal procedure that QEF knows expects certain parameter
**	names/types. One or more of the supplied parameter types does not
**	match what QEF expects.
**
** System Status:
**	The "EXECUTE PROCEDURE" statement is aborted.
**
** Recommendation:
**	The internal procedure "user" is a system-supplied frontend
**	program. It should not generate internal procedure errors; please
**	contact Ingres Corporation Technical Support.
*/
E_QE0134_LISTFILE_ERROR:SS50000_MISC_ERRORS
"Error encountered while performing iiQEF_listfile Internal Procedure.  See errlog.log
 file for details."
/*%
** Parameters:
**	None.
**
** Explanation:
**	DMD encountered (and logged in the error log) an error
**	while attempting a DIlistfile() or DIlistdir().
**
** System Status:
**	The "EXECUTE PROCEDURE" statement is aborted.
**
** Recommendation:
**	The internal procedure "user" is a system-supplied frontend
**	program. It should not generate internal procedure errors; please
**	contact Ingres Corporation Technical Support.
*/
E_QE0135_DROPFILE_ERROR:SS50000_MISC_ERRORS
"Error encounted while performing iiQEF_dropfile Internal Procedure.  See errlog.log
 file for details."
/*%
** Parameters:
**	None.
**
** Explanation:
**	DMF encountered (and logged) an error while attempting a DIdelete().
**
** System Status:
**	The "EXECUTE PROCEDURE" statement is aborted.
**
** Recommendation:
**	The internal procedure "user" is a system-supplied frontend
**	program. It should not generate internal procedure errors; please
**	contact Ingres Corporation Technical Support.
*/
E_QE0136_FINDDBS_ERROR:SS50000_MISC_ERRORS
"An Error was encounted while performing iiQEF_finddbs Internal Procedure.\n
See errlog.log file for details."
/*%
** Parameters:
**	None.
**
** Explanation:
**	DMF encountered (and logged) an error while attempting a dmm_finddbs()
**	operation.
**
** System Status:
**	The "EXECUTE PROCEDURE" statement is aborted.
**
** Recommendation:
**	The internal procedure "user" is a system-supplied frontend
**	program. It should not generate internal procedure errors; please
**	contact Ingres Corporation Technical Support.
*/
E_QE0137_SEVERE_FINDDBS_ERR:SS50000_MISC_ERRORS
"A FATAL Error was encounted while performing iiQEF_finddbs Internal Procedure.\n
See errlog.log file for details."
/*%
** Parameters:
**	None.
**
** Explanation:
**	DMF encountered (and logged) an error while attempting a dmm_finddbs()
**	operation.
**
** System Status:
**	The "EXECUTE PROCEDURE" statement is aborted.
**
** Recommendation:
**	The internal procedure "user" is a system-supplied frontend
**	program. It should not generate internal procedure errors; please
**	contact Ingres Corporation Technical Support.
*/
E_QE0138_CANT_CREATE_UNIQUE_CONSTRAINT:SS23501_UNIQUE_CONS_VIOLATION
"Could not create a UNIQUE CONSTRAINT (PRIMARY KEY)\n
on table '%0c'.  The columns specified in the CONSTRAINT\n
(PRIMARY KEY) do not form a unique key.  Duplicates on that key were found
in the table."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	The user attempted to add a unique constraint to a pre-existing
**	table.  Unfortunately, the table contains duplicate tuples on
**	the columns which the user specified in an "ALTER TABLE ADD
**	CONSTRAINT" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Delete duplicate tuples, and try again.
*/
E_QE0139_CANT_ADD_REF_CONSTRAINT:SS23000_CONSTR_VIOLATION
"Could not add a REFERENTIAL CONSTRAINT.  Some rows\n
in table '%0c' do not have matching keys\n
in table '%1c'."
/*%
** Parameters:
**	%0c - referencing table name
**	%1c - referenced table name
**
** Explanation:
**	The user attempted to add a referential constraint linking pre-existing
**	tables.  Unfortunately, the referring table contains tuples whose keys
**	don't match any keys in the referenced table.  That is, the data
**	currently in the tables already violates the referential constraint.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The following query will show you which rows in the referencing
**	table don't have corresponding keys in the referenced table:
**
**		SELECT * FROM
**		( referencingTable leftTable LEFT JOIN
**		  referencedTable rightTable
**		    ON leftTable.1stKeyColumn = rightTable.1stKeyColumn
**			...
**		    AND leftTable.LastKeyColumn = rightTable.LastKeyColumn
**		)
**		WHERE rightTable.1stKeyColumn IS NULL;
**
**	Force the tables to agree by either 1) deleting rows from the
**	referencing table, or 2) inserting new rows into the referenced table.
*/
E_QE013A_CANT_CREATE_CONSTRAINT:SS5000B_INTERNAL_ERROR
"Could not create a UNIQUE/REFERENTIAL constraint\n
on table '%0c'. Creation of underlying index failed."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	The user attempted to add a unique or referential constraint. 
**	An attempt by Ingres to create the index needed to support the
**	constraint failed. See the error log for the index error. It
**	is possible that the index creation failed because of conflicting
**	index definition options supplied with the constraint definition.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Confirm the validity of any index options specificed with the
**	constraint definition and try again.
*/
E_QE0140_CANT_EXLOCK_REF_TABLE:SS50000_MISC_ERRORS
"Could not add a REFERENTIAL CONSTRAINT because one of the tables involved\n
('%0c') is currently in use.  Creating a REFERENTIAL CONSTRAINT requires\n
exclusive access to both tables linked by the REFERENCES clause."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	The user attempted to add a referential constraint linking pre-existing
**	tables.  Unfortunately, either the referring table or the referenced
**	table is open in some other user's transaction.  While we build
**	support for enforcing the constraint, we must be sure that no one
**	can corrupt the data.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Find out who is accessing the table, and get them to commit their work.
*/
E_QE0141_V_UBTID_GETINFO_ERR:SS5000B_INTERNAL_ERROR
"An error occurred while trying to obtain view tree for a
\nview (%0d, %0d)."
/*%
** Parameters:
**	%0d - db_tab_base of a view's ID
**	%1d - db_tab_index of a view's ID
**
** Explanation:
**	In the course of processing a DDL query involving a view, it became
**	necessary to obtain the ID of the view's underlying base table.
**	Attempts to obtain the view's tree from RDF failed unexpectedly.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_QE0142_V_UBTID_UNFIX_ERR:SS5000B_INTERNAL_ERROR
"An error occurred while trying to unfix view description for a
\nview (%0d, %0d)."
/*%
** Parameters:
**	%0d - db_tab_base of a view's ID
**	%1d - db_tab_index of a view's ID
**
** Explanation:
**	In the course of processing a DDL query involving a view, it became
**	necessary to obtain the ID of the view's underlying base table.
**	Attempts to unfix the view's description from RDF failed unexpectedly.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_QE0143_V_UBTID_GETDESC_ERR:SS5000B_INTERNAL_ERROR
"An error occurred while trying to obtain view description for a
\nview (%0d, %0d)."
/*%
** Parameters:
**	%0d - db_tab_base of a view's ID
**	%1d - db_tab_index of a view's ID
**
** Explanation:
**	In the course of processing a DDL query involving a view, it became
**	necessary to obtain the ID of the view's underlying base table.
**	Attempts to obtain the view's description from RDF failed unexpectedly.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_QE0144_CANT_ADD_CHECK_CONSTRAINT:SS23500_CHECK_CONS_VIOLATION
"Could not add a CHECK CONSTRAINT.  Some rows\n
in table '%0c' violate the check condition."
/*%
** Parameters:
**	%0c - constrained table name
**
** Explanation:
**	The user attempted to add a check constraint to a pre-existing
**	table.  Unfortunately, the table contains tuples which fail the
**	check condition specified in the constraint.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Delete the rows that violate the condition, and try again.
*/
E_QE0145_CANT_EXLOCK_CHECK_TABLE:SS50000_MISC_ERRORS
"Could not add a CHECK CONSTRAINT to table ('%0c')\n
because that table is currently in use."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	The user attempted to add a check constraint to a pre-existing
**	table.  Unfortunately, the table is open in some other user's
**	transaction.  While we build support for enforcing the constraint,
**	we must be sure that no one can corrupt the data.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Find out who is accessing the table, and get them to commit their work.
*/
E_QE0146_QSO_JUST_TRANS_ERROR:SS50000_MISC_ERRORS
"Unexpected error encountered while trying to obtain 
\nhandle of a QP object for database procedure
\n'%0c'.'%1c'."
/*%
** Parameters:
**	%0c - schema to which a database procedure belongs
**	%1c - name of the database procedure
**
** Explanation:
**	In the course of processing destruction of some object or revocation of
**	some privilege, it has become necessary to destroy query plan for 
**	this database procedure.  However, an attempt to gain access to the 
**	query plan QSF object failed with unexpected error.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_QE0147_QSO_DESTROY_ERROR:SS50000_MISC_ERRORS
"Unexpected error encountered while trying to destroy 
\na QP object for database procedure
\n'%0c'.'%1c'."
/*%
** Parameters:
**	%0c - schema to which a database procedure belongs
**	%1c - name of the database procedure
**
** Explanation:
**	In the course of processing destruction of some object or revocation of
**	some privilege, it has become necessary to destroy query plan for 
**	this database procedure.  However, an attempt to destroy the query plan
**	QSF object failed with unexpected error.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_QE0160_INVALID_GRPID_OP:SS50000_MISC_ERRORS
"Invalid QEF operation for iiusergroup encountered."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The qeu_group() function encountered an invalid operation code
**	in qeu_agid_mask.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_QE0161_INVALID_APLID_OP:SS50000_MISC_ERRORS
"Invalid QEF operation for iiapplication_id encountered."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The qeu_aplid() function encountered an invalid operation code in
**	qeu_agid_mask.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_QE0162_GRPID_ANCHOR_MISSING:SS5000J_INCONSISTENT_DBMS_CAT
"Missing anchor detected for group '%0c'."
/*%
** Parameters:
**	%0c - group name
**
** Explanation:
**	The qeu_dgroup() function discovered a group member record that did not
**	have an associated anchor record.  This should never happen.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ensure that no user deleted the anchor tuple.  Contact
**	Technical Support.
*/
E_QE0163_GRPMEM_PURGE:SS50000_MISC_ERRORS
"Invalid request to purge group member encountered\n
         Group: '%0c'\n
        Member: '%0c'"
/*%
** Parameters:
**	%0c - group name
**	%1c - member name
**
** Explanation:
**	The qeu_dgroup() function detected a request to purge, but the input
**	tuple represented a member, not an anchor.  This is inconsistent.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_QE0164_GRPID_PURGE:SS50000_MISC_ERRORS
"Unable to recreate anchor during purge of group '%0c'."
/*%
** Parameters:
**	%0c - group name
**
** Explanation:
**	When purging a group, the qeu_dgroup() function successfully deleted
**	all group records, but could not add the anchor record back using
**	qeu_cgroup().  This is a traceback message; qeu_cgroup() should have
**	issued a previous message indicating the exact nature of the failure.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_QE0165_NONEXISTENT_RULE:SS42505_OBJ_NOT_FOUND
"Rule not found when linking to constraint."
E_QE0166_NO_CONSTRAINT_IN_SCHEMA:SS42505_OBJ_NOT_FOUND
"ALTER TABLE DROP CONSTRAINT: 
\nconstraint '%0c' does not exist in schema '%1c'."
/*%
** Parameters:
**	%0c - constraint name
**	%1c - schema name
**
** Explanation:
**	A constraint with a name specified by the user could not be found
**	in the schema to which the table named in the "ALTER TABLE" statement
**	belongs.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_QE0167_DROP_CONS_WRONG_TABLE:SS42505_OBJ_NOT_FOUND
"ALTER TABLE DROP CONSTRAINT: 
\ntable '%1c'.'%2c' does not contain constraint '%0c'."
/*%
** Parameters:
**	%0c - constraint name
**	%1c - schema name
**	%2c - table name
**
** Explanation:
**	A constraint with a name specified by the user exists, but not on the
**	table named in the "ALTER TABLE DROP CONSTRAINT" statement.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_QE0168_ABANDONED_CONSTRAINT:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE DROP CONSTRAINT RESTRICT: 
cascaded destruction of constraint '%0c' 
may not proceed because it would result in another
constraint becoming abandoned."
/*%
** Parameters:
**	%0c - constraint name
**
** Explanation:
**	Restricted destruction of a UNIQUE constraint may not proceed because
**	it would render some REFERENCES constraint abandoned.  To effect
**	destruction of the UNIQUE constraint along with dependent constraints,
**	one must specify an "ALTER TABLE DROP CONSTRAINT CASCADE" statement.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_QE0169_UNKNOWN_OBJTYPE_IN_DROP:SS5000B_INTERNAL_ERROR
"Unknown object type (%0d) encountered while performing cascading destruction."
/*%
** Parameters:
**	%0d - object type found in iidbdepends.dbv_dtype
**
** Explanation:
**	In the course of processing cascading destruction, QEF has encountered
**	an object of a type with which it was not prepared to deal. This could
**	be caused by corruption of data in iidbdepends.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_QE016A_NOT_DROPPABLE_CONS:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE DROP CONSTRAINT: 
Constraint '%0c' cannot be dropped because it represents a NOT NULL
constraint that is part of the table structure, and the table structure
cannot be altered."
/*%
** Parameters:
**	%0c - constraint name
**
** Explanation:
**	Normal check constraints are implemented as
**	internal rules on a table.  These rules can be easily
**	dropped when the constraint is dropped.
**	
**	In the ANSI/ISO SQL92 standard, "NOT NULL" is considered
**	to be another form of a CHECK constraint, equivalent to
**	"CHECK (<column> IS NOT NULL)".
**
**	But when the user defines a column as "NOT NULL", this is
**	implemented as a non-nullable datatype, instead of as
**	a rule (as it would for a normal check constraint).  So,
**	since we cannot (in 6.5) alter the datatype of a column,
**	this "NOT NULL" "constraint" cannot be dropped.
**	
**	NOTE: For ANSI compatibility, we create a tuple in the
**	constraints catalog to represent this "NOT NULL" "constraint",
**	so a user can find the name of this "constraint" and try to
**	drop it.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Do not attempt to drop this constraint.  If you need to insert
**	nulls into a "NOT NULL" column, you must recreate the table with
**	a nullable datatype for that column.
*/
E_QE016B_COLUMN_HAS_OBJECTS:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE DROP COLUMN RESTRICT: 
restricted destruction of base table column cannot proceed as it 
has one or more dependent objects which would become abandoned."
/*%
** Parameters:
**	None
**
** Explanation:
**	Restricted destruction of a column cannot proceed because it has been
**	determined that it has one or more dependent objects which would
**	become abandoned. This may be a view, constraint, rule, permission etc.
**
**	To effect destruction of the base table column along with all the
**	dependent objects, specify "ALTER TABLE DROP COLUMN CASCADE".
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_QE016C_RENAME_TAB_HAS_OBJS:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as 
one or more objects eg procedures, views, or non grant compatible permits
depend on it. Check the error log (errlog.log) for details."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**
** Explanation:
**	Rename operation on a table is not allowed if it has a 
**	dependent views, procedures, rules, security alarm, 
**	constraint or non grant compatible permits on it. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	To rename the table, drop the dependent objects prior 
**	to rename and recreate the objects after rename.  
*/
E_QE016D_RENAME_TAB_HAS_PROC:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
procedure %2c depends on it."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**	%2c - Procedure name
**
** Explanation:
**	Rename operation on a table is not allowed if it has a 
**	dependent view, procedures, rules, security alarm, 
**	constraint or non grant compatible permits on it. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	To rename the table, drop the dependent objects prior 
**	to rename and recreate the objects after rename.  
*/
E_QE016E_RENAME_TAB_HAS_VIEW:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
view %2c depends on it."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**	%2c - View name
**
** Explanation:
**	Rename operation on a table is not allowed if it has a 
**	dependent view, procedures, rules, security alarm, 
**	constraint or non grant compatible permits on it. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	To rename the table, drop the dependent objects prior 
**	to rename and recreate the objects after rename.  
*/
E_QE016F_RENAME_TAB_HAS_RULE:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
rule %2c depends on it."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**	%2c - View name
**
** Explanation:
**	Rename operation on a table is not allowed if it has a 
**	dependent view, procedures, rules, security alarm, 
**	constraint or non grant compatible permits on it. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	To rename the table, drop the dependent objects prior 
**	to rename and recreate the objects after rename.  
*/
E_QE0170_RENAME_TAB_HAS_NGPERM:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
permit %2d defined on it is non grant compatible."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**	%2d - Permit number
**
** Explanation:
**	Rename operation on a table is not allowed if it has a 
**	dependent view, procedures, rules, security alarm, 
**	constraint or non grant compatible permits on it. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	To rename the table, drop the dependent objects prior 
**	to rename and recreate the objects after rename.  
*/
E_QE0171_RENAME_TAB_HAS_CONS:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
it has constraint %2c defined on it."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**
** Explanation:
**	Rename operation on a table is not allowed if it has a 
**	dependent view, procedures, rules, security alarm, 
**	constraint or non grant compatible permits on it. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	To rename the table, drop the dependent objects prior 
**	to rename and recreate the objects after rename.  
*/
E_QE0172_RENAME_TAB_HAS_SECALARM:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
it has security alarm %2c defined on it."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**	%2c - Security alarm name
**
** Explanation:
**	Rename operation on a table is not allowed if it has a 
**	dependent view, procedures, rules, security alarm, 
**	constraint or non grant compatible permits on it. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	To rename the table, drop the dependent objects prior 
**	to rename and recreate the objects after rename.  
*/
E_QE0173_RENAME_TAB_IS_CATALOG:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
it is a system catalog."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**
** Explanation:
**	Rename operation on a table is not allowed if it has a 
**	is a core or extended system catalog.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Cannot rename the system or extended system catalog.
*/
E_QE0174_RENAME_TAB_IS_SYSGEN:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
it is a system generated or system maintained table."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**
** Explanation:
**	Rename operation on a table is not allowed if it
**	is a system generated or a system maintained table.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Cannot rename a system generated or maintained table.
*/
E_QE0175_RENAME_TAB_IS_ETAB:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
it is a blob extension table."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**
** Explanation:
**	Rename operation on a table is not allowed if it
**	is a extention table created for blob.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Cannot rename the internal blob etab tables.
*/
E_QE0176_RENAME_TAB_IS_GATEWAY:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
it is a Gateway table."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**
** Explanation:
**	Rename operation on a table is not allowed if it
**	is a Gateway table. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Cannot rename the Gateway tables. 
*/
E_QE0177_RENAME_TAB_IS_PARTITION:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
it is a partion table."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**
** Explanation:
**	Rename operation on a table is not allowed if it
**	is a partition. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Cannot rename the table or column. 
*/
E_QE0178_RENAME_TAB_IS_INCONSIST:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
it is inconsistent."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**
** Explanation:
**	Rename operation on a table is not allowed if it
**	is inconsistent. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Cannot rename the table or column.
*/
E_QE0179_RENAME_TAB_IS_READONLY:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
it is a read only table."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**
** Explanation:
**	Rename operation on a table is not allowed if it
**	is a read only table.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Cannot rename the table or column.
*/
E_QE0180_RENAME_TAB_HAS_INTEGRITY:SS42000_SYN_OR_ACCESS_ERR
"Rename of table %0c owned by %1c, or its columns is not permitted as
it has integrity %2d defined on it."
/*%
** Parameters:
**	%0c - Table name
**	%1c - Owner name
**	%2d - Integrity number
**
** Explanation:
**	Rename operation on a table is not allowed if it has a 
**	dependent view, procedures, rules, security alarm, 
**	constraints, integrity or non grant compatible permits on it. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	To rename the table, drop the dependent objects prior 
**	to rename and recreate the objects after rename.  
*/
E_QE0181_RENAME_COL_HAS_OBJS:SS42000_SYN_OR_ACCESS_ERR
"Rename of column %0c for table %1c owned by %2c, is not permitted as 
one or more objects eg procedures, views, or non grant compatible permits
depend on it. Check the error log (errlog.log) for details."
/*%
** Parameters:
**	%0c - Column name
**	%1c - Table name
**	%2c - Owner name
**
** Explanation:
**	Rename operation on a table column is not allowed if it has a 
**	dependent views, procedures, rules, security alarm, 
**	constraint or non grant compatible permits on it. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	To rename the column, drop the dependent objects prior 
**	to rename and recreate the objects after rename.  
*/
E_QE0199_CALL_ALLOC:SS50003_EXHAUSTED_RESOURCE
"Failed attempt to nest procedure (rule) '%0c' (%1c) at query depth %2d
due to insufficient resources in the DBMS server.  The current query is
aborted with a fatal error.  Increase the server startup option
CURSORS_PER_SESSION (and internal option QEP_SIZE) in order to increase
the amount of available memory for your session."
/*%
** Parameters:
**	%0c - procedure name
**	%1c - rule name (if from a rule) or NULL
**	%2c - current nesting depth of the procedure query plan
**
** Explanation:
**	A deeply nested or recursive rule (or procedure) has caused the QEF
**	server facility to exceed the amount of memory allocated for this
**	session.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Request from your system administrator to allocate more memory
**	per session by increasing the values of the startup options specified
**	in the error message.  If this is not an alternative, then try to
**	modify the flow of the procedure (or rule) so that the nesting
**	depth is decreased.
*/
E_QE019A_EVENT_MESSAGE:SS42000_SYN_OR_ACCESS_ERR
"The %0c DBEVENT statement may not be issued because your application is
running with an older communication (GCA) version.  This statement, when
executed, will cause a new GCA message type to be transmitted
but your application is not prepared to receive such a message type."
/*%
** Parameters:
**	%0 - name of dbevent operation
**
** Explanation:
**	An attempt to register a dbevent failed because, when the dbevent is
**	later raised, the system will attempt to transmit a new GCA message
**	type to your registered application.  This message type, however,
**	belongs to a more recent version of GCA and cannot be interpreted by
**	your application (losing the functionality of dbevents).
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Relink your application with a more recent set of run-time libraries.
*/
E_QE019B_DMM_CREATE_DB_ERROR:SS5000B_INTERNAL_ERROR
"An unexpected error was encountered while attempting to create a database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	During the initial creation of a new database, the server encountered
**	an internal error. Additional details about the precise error should
**	already have been logged in the error log file, and these
**	additional error messages describe the actual problem encountered.
**
** System Status:
**	The createdb operation is aborted.
**
** Recommendation:
**	The error log should contain additional error messages, possibly
**	beginning with the prefix "E_DMxxxx", that will describe the actual
**	problem that was encountered when creating the database. Using the
**	information in these messages, resolve the error condition and reissue
**	the createdb command. If you are unable to determine the reason for the
**	error, contact Ingres Corporation Technical Support.
*/
E_QE019C_SEC_WRITE_ERROR:SS42000_SYN_OR_ACCESS_ERR
"An error occurred while attempting to write a Security Audit Record.
Check the error log (errlog.log) for more information why this occurred."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error was detected while attempting to write a security audit record.
**	Additional details about the precise error should already have been
**	logged in the error log file, and these additional error
**	messages describe the actual problem encountered.
**
** System Status:
**	The operation is aborted. The DBMS may stop auditing or shut down,
**	depending on the action selected by the security audit configuration.
**
** Recommendation:
**	Check the system status and retry.
*/
E_QE019D_INVALID_ROLEGRANT_OP:SS42000_SYN_OR_ACCESS_ERR
"Invalid QEF operation for iirolegrant encountered."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The qeu_rolegrant() function encountered an invalid operation code
**	in qeu_agid_mask. This is an internal error.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_QE0200_NO_EVENTS:SS42000_SYN_OR_ACCESS_ERR
"The %0c DBEVENT statement may not be issued because the DBMS dbevent subsystem
has not been initialized.  This may be caused by the use of a zero DBEVENTS
startup option, by a server startup error, by the forced removal of the
server dbevent thread, or an internal error during dbevent processing.  Check
the server error log for dbevent-related errors."
/*%
** Parameters:
**	%0 - dbevent operation name
**
** Explanation:
**	An attempt to "REGISTER", "REMOVE" or "RAISE" a dbevent failed,
**	because the internal dbevent subsystem has not been initialized.
**	This can occur if:
**	  - the server DBEVENTS=0 command-line parameter was used
**	  - the DBMS server startup phase encountered a memory allocation error
**	  - the dbevent subsystem encountered a connect-time error
**	  - the DBMS dbevent thread was removed (through iimonitor) or exited
**	      due to internal dbevent errors
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	If the server has been installed with DBEVENTS=0, the above
**	statements are not allowed.
**
**	All other errors are detailed in the server error log. Determine
**	which server the user is currently using, and check that
**	server's error log for dbevent-related errors.  The action
**	to take will depend on the error discovered.
*/
E_QE0201_BAD_SORT_KEY:SS5000B_INTERNAL_ERROR
"Illegal QP detected.  A sort key is not one of sort tuple attributes."
E_QE0202_RESOURCE_CHCK_INVALID:SS50000_MISC_ERRORS
"QEF consistency check. An attempt was made to apply resource checking
to an action that did not contain a QEP."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error internal to QEF has occurred. QEF has attempted to apply
**	resource checking to a node that does not use query resources. Only
**	nodes containing QEP's use query resources. Essentially, resource
**	checking has been invoked by an action procedure that does not
**	implement query processing.
**
** System Status:
**	The current query is aborted with a fatal error status.
**
** Recommendation:
**	Report this internal system error to Ingres Corporation Technical Support.
*/
E_QE0203_DIO_LIMIT_EXCEEDED:SS50003_EXHAUSTED_RESOURCE
"The estimated number of IO requests to run this query (%0d) exceeds
the IO limit (%1d) established for this session. \n
The query is aborted with a fatal error."
/*%
** Parameters:
**	%0d - estimated number of I/O operations required to execute the query
**	%1d - I/O limit established for this session
**
** Explanation:
**	Each user may be constrained by resource limits established by the
**	DBA or system administrator. The limits may be individually
**	established for each database via the GRANT statement.
**
**	Each query that is executed contains an estimate of the resources
**	required to execute the query. If the estimate for a query exceeds
**	the resource limit established for the current session, the query
**	request is rejected. In this case, the estimate of I/O operations
**	exceeded the limit for this session.
**
** System Status:
**	The query is aborted with a fatal error.
**
** Recommendation:
**	1. Attempt to run a variant of the query that will use fewer resources.
**	   This may be possible by adding additional qualifications to the
**	   query or by further restricting the range of values in current
**	   qualifications.
**	2. Determine if the database is properly organized for efficient
**	   execution of the query under consideration. If this is a common
**	   query, it may be advisable to change the storage structures or
**	   index structures on the tables involved.
**	3. Ask your DBA or system administrator to increase your
**	   resource limits.
*/
E_QE0204_ROW_LIMIT_EXCEEDED:SS50002_LIMIT_EXCEEDED
"The estimated number of rows returned to the application (%0d) exceeds
the row limit (%1d) established for this session.\n
The query is aborted with a fatal error."
/*%
** Parameters:
**	%0d - estimated number of rows returned by the query
**	%1d - row limit established for this session
**
** Explanation:
**	Each user may be constrained by resource limits established by the
**	DBA or system administrator. The limits may be individually
**	established for each database via the "GRANT" statement.
**
**	Each query that is executed contains an estimate of the resources
**	required to execute the query. If the estimate for a query exceeds the
**	resource limit established for the current session, the query request
**	is rejected. In this case, the estimate of the number of rows to be
**	returned to the application exceeded the limit for this session. Note
**	that this limit applies only to "SELECT" and "OPEN CURSOR" statements.
**	Statements that do not return rows to the application such as "INSERT",
**	"UPDATE", "DELETE", and "SELECT AS" ignore this limit.
**
** System Status:
**	The query is aborted with a fatal error.
**
** Recommendation:
**	1. Attempt to run a variant of the query that will return fewer
**	   rows. This may be possible by adding additional qualifications
**	   to the query or by further restricting the range of values in
**	   current qualifications.
**	2. Ask your DBA or system administrator to increase your
**	   resource limits.
*/
E_QE0205_CPU_LIMIT_EXCEEDED:SS50002_LIMIT_EXCEEDED
"The estimated CPU time to run this query (%0d normalized units) exceeds
the CPU limit (%1d normalized units) established for this session.\n
The query is aborted with a fatal error."
/*%
** Parameters:
**	%0d - estimated units of CPU resource required to execute the query
**	%1d - CPU resource limit established for this session
**
** Explanation:
**	Each user may be constrained by resource limits established by the
**	DBA or system administrator. The limits may be individually
**	established for each database via the "GRANT" statement.
**
**	Each query that is executed contains an estimate of the resources
**	required to execute the query. If the estimate for a query exceeds
**	the resource limit established for the current session, the query
**	request is rejected. In this case, the estimate of CPU resources
**	exceeded the limit for this session. Note that CPU resources are
**	estimated in normalized units and provide only a relative measure
**	of the CPU time required to execute a query. The estimates do not
**	correspond to the actual CPU time used on a specific processor.
**
** System Status:
**	The query is aborted with a fatal error.
**
** Recommendation:
**	1. Attempt to run a variant of the query that will use fewer resources.
**	   This may be possible by adding additional qualifications to the
**	   query or by further restricting the range of values in current
**	   qualifications.
**	2. Determine if the database is properly organized for efficient
**	   execution of the query under consideration. If this is a common
**	   query, it may be advisable to change the storage structures or
**	   index structures on the tables involved.
**	3. Ask your DBA or system administrator to increase your
**	   resource limits.
*/
E_QE0206_PAGE_LIMIT_EXCEEDED:SS50002_LIMIT_EXCEEDED
"The estimated number of pages referenced to run this query (%0d) exceeds
the page reference limit (%1d) established for this session.\n
The query is aborted with a fatal error."
/*%
** Parameters:
**	%0d - estimated number of pages referenced to execute the query
**	%1d - page reference limit established for this session
**
** Explanation:
**	Each user may be constrained by resource limits established by the
**	DBA or system administrator. The limits may be individually
**	established for each database via the "GRANT" statement.
**
**	Each query that is executed contains an estimate of the resources
**	required to execute the query. If the estimate for a query exceeds
**	the resource limit established for the current session, the query
**	request is rejected. In this case, the estimate of the number of
**	database pages referenced by the query exceeded the limit for this
**	session.
**
** System Status:
**	The query is aborted with a fatal error.
**
** Recommendation:
**	1. Attempt to run a variant of the query that will use fewer resources.
**	   This may be possible by adding additional qualifications to the
**	   query or by further restricting the range of values in current
**	   qualifications.
**	2. Determine if the database is properly organized for efficient
**	   execution of the query under consideration. If this is a common
**	   query, it may be advisable to change the storage structures or
**	   index structures on the tables involved.
**	3. Ask your DBA or system administrator to increase your
**	   resource limits.
*/
E_QE0207_COST_LIMIT_EXCEEDED:SS50002_LIMIT_EXCEEDED
"The total estimated cost to run this query (%0d) exceeds
the cost limit (%1d) established for this session.\n
The query is aborted with a fatal error."
/*%
** Parameters:
**	%0d - estimated cost to execute the query
**	%1d - cost limit established for this session
**
** Explanation:
**	Each user may be constrained by resource limits established by the
**	DBA or System Administrator. The limits may be individually
**	established for each database via the "GRANT" statement.
**
**	Each query that is executed contains an estimate of the resources
**	required to execute the query. If the estimate for a query exceeds
**	the resource limit established for the current session, the query
**	request is rejected. In this case, the total estimated cost
**	exceeded the limit for this session. The total estimated cost is
**	a normalized function of the estimated number of I/O operations and
**	estimated CPU cost to execute the query. This number serves only
**	as a relative measure and does not represent physical resources
**	consumed on a specific processor.
**
** System Status:
**	The query is aborted with a fatal error.
**
** Recommendation:
**	1. Attempt to run a variant of the query that will use fewer resources.
**	   This may be possible by adding additional qualifications to the
**	   query or by further restricting the range of values in current
**	   qualifications.
**	2. Determine if the database is properly organized for efficient
**	   execution of the query under consideration. If this is a common
**	   query, it may be advisable to change the storage structures or
**	   index structures on the tables involved.
**	3. Ask your DBA or system administrator to increase your
**	   resource limits.
*/
E_QE0208_EXCEED_MAX_CALL_DEPTH:SS50002_LIMIT_EXCEEDED
"Query exceeds the limit (%0d) for cascading rules and nested database
procedures.  The query is aborted with a fatal error."
/*%
** Parameters:
**	%0d - maximum nesting level for database procedures
**
** Explanation:
**	This error will occur when executing queries that trigger rule
**	processing. The rule processing may be such that a query triggers
**	a rule, a query within that rule triggers another rule, and so on.
**	Each such nested rule firing causes the execution of the database
**	procedure associated with the rule to be stacked and the database
**	procedure invoked by the newly triggered rule to begin execution.
**	The maximum depth of such nesting is constrained by an internal
**	system parameter. The current query has triggered rule execution
**	that has cascaded beyond the maximum depth allowed.
**
** System Status:
**	The original query triggering rule execution is aborted with a
**	fatal error. All queries run by the database procedures invoked
**	by rule processing are rolled back.
**
** Recommendation:
**	The following are possible causes of this error:
**	    1. The collection of rules defined in the database establishes
**	       dependencies between more tables than are supported by the
**	       system. Both the database design and the set of rules defined
**	       should be inspected and changed to limit the dependencies
**	       between tables.
**	    2. Circular dependencies may be defined that will cause rules
**	       to cascade indefinitely. Inspect the set of rules defined
**	       and modify one or more rules in the offending rule set.
*/
E_QE0209_BAD_RULE_STMT:SS42000_SYN_OR_ACCESS_ERR
"The %0c statement cannot be issued while firing a database rule.
The current query is aborted."
/*%
** Parameters:
**	%0c - statement name
**
** Explanation:
**	This error will occur when executing queries that trigger rule
**	processing.  Various statements cannot be issued while processing a
**	rule, as they modify the state of the database in relation to the
**	original query that fired the rule.  Currently statements that cannot
**	be issued from within rule processing are: "COMMIT" and "ROLLBACK".
**
** System Status:
**	The original query triggering rule execution is aborted with a
**	fatal error. All queries run by the database procedures invoked
**	by rule processing are rolled back.
**
** Recommendation:
**	If the database procedure invoked by the rule is also used
**	independently (via the "EXECUTE PROCEDURE" statement), add another
**	parameter to the procedure that enables the conditional issuing of
**	the violating statement.
**
**	If the database procedure is always invoked from a rule, remove
**	the violating statement, and issue it after control returns to the
**	application.
*/
E_QE020A_EVENT_EXISTS:SS42502_DUPL_OBJECT
"CREATE DBEVENT: Dbevent '%0c' already exists."
/*%
** Parameters:
**	%0c - dbevent name
**
** Explanation:
**	The named dbevent already exists for the current user.  Duplicate
**	dbevent names are not allowed.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Choose a unique name for the new dbevent, or delete the existing named
**	dbevent.
*/
E_QE020B_EVENT_ABSENT:SS42505_OBJ_NOT_FOUND
"DROP DBEVENT: Dbevent '%0c' does not exist or is not owned by you."
/*%
** Parameters:
**	%0c - dbevent name
**
** Explanation:
**	The named dbevent does not exist for the current user; dbevents owned
**	by other users cannot be dropped.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Retype the dbevent name.
*/
E_QE020C_EV_REGISTER_EXISTS:SS42502_DUPL_OBJECT
"REGISTER DBEVENT: Dbevent '%0c.%1c' is already registered.
This new registration is ignored."
/*%
** Parameters:
**	%0c - dbevent owner
**	%1c - dbevent name
**
** Explanation:
**	The named dbevent is already registered for the current application.
**	Duplicate dbevent registrations are not allowed.
**
** System Status:
**	The current "REGISTER" statement is aborted, but the original
**	registration remains in effect.  If this statement was issued from
**	within a database procedure, the error causes the default error
**	semantics of database procedures.
**
** Recommendation:
**	Control the registration of dbevents to only register them once.
**	The "REMOVE DBEVENT" statement may be used to de-register a dbevent.
*/
E_QE020D_EV_REGISTER_ABSENT:SS42505_OBJ_NOT_FOUND
"REMOVE DBEVENT: Dbvent '%0c.%1c' is not registered.  This statement is ignored."
/*%
** Parameters:
**	%0c - dbevent owner
**	%1c - dbevent name
**
** Explanation:
**	The named dbevent is not (yet) registered for the current application.
**	Registration removal may only apply to registered dbevents.  Note
**	that application (session) termination automatically removes all your
**	registrations.
**
** System Status:
**	The current "REMOVE" statement is aborted without modifying the state
**	of already registered dbevents.  If this statement was issued from
**	within a database procedure, the error causes the default error
**	semantics of database procedures.
**
** Recommendation:
**	Control the registration and de-registration of dbevents to only
**	register dbevents once, and only remove them after they are registered.
*/
E_QE020E_EV_PERMIT_ABSENT:SS42505_OBJ_NOT_FOUND
"Dbevent '%0c' owned by '%1c' does not exist for GRANT, REVOKE or DROP PERMIT
privilege modification."
/*%
** Parameters:
**	%0c - dbevent name
**	%1c - current user
**
** Explanation:
**	The named dbevent does not exist.  Privileges may only be granted or
**	revoked on existing dbevents.  If a dbevent is dropped, all privileges
**	associated with that dbevent are also dropped.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Use the "CREATE DBEVENT" statement to recreate the dbevent.
*/
E_QE020F_EVENT_SCF_FAIL:SS50000_MISC_ERRORS
"SCF failure on operation '%0c', with error number %1x (%1d),
on dbevent '%2c.%3c'.  Contact your System Administrator
to check the installation error log file for more details."
/*%
** Parameters:
**	%0c - current statement name ("REGISTER", "REMOVE", or "RAISE")
**	%1c - internal failure status
**	%2c - dbevent owner
**	%3c - dbevent name
**
** Explanation:
**	The current operation failed due to an internal failure.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Check the DBMS error log file for more information.  This may be
**	an internal error, or you may have encountered server initialization
**	errors.  Check the error log for earlier dbevent-related errors.
*/

E_QE0210_QEC_ALTER:SS50000_MISC_ERRORS
"An error occurred while establishing QEF session parameters.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The qec_alter() function returned bad status when storing session
**	database privileges.  This is an internal error, probably due to
**	a bad control block.
**
** System Status:
**	If it occurs during session initialization, the session
**	is disconnected.
**
** Recommendation:
**	Contact the system administrator.
*/
E_QE022A_IIUSERGROUP:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the
iiusergroup catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening,
**	reading, or writing this catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs during session initialization, the session is
**	disconnected.  NOTE: Group identifiers are not verified for
**	superusers. If it occurs during CREATE/ALTER/DROP processing,
**	the statement is aborted.
**
** Recommendation:
**	Contact your system administrator.
*/
E_QE022B_IIAPPLICATION_ID:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iirole catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening,
**	reading, or writing this catalog.  It will appear
**	if that catalog has been clobbered, or if it doesn't exist.
**
** System Status:
**	If it occurs during session initialization, the session is
**	disconnected.  NOTE: Role identifiers are not verified for
**	superusers. If it occurs during CREATE/ALTER/DROP processing,
**	the statement is aborted.
**
** Recommendation:
**	Contact your system administrator.
*/
E_QE022C_IIDBPRIV:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iidbpriv catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening,
**	reading, or writing this catalog.  It will appear
**	if that catalog has been clobbered, or if it doesn't exist.
**
** System Status:
**	If it occurs during session initialization, the session is
**	disconnected; superusers are allowed to continue. If it occurs
**	while processing GRANT/REVOKE statements for database privileges,
**	the statement is aborted.
**
** Recommendation:
**	Contact your system administrator.
*/
E_QE022D_IIDATABASE:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iidatabase catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if QEF has trouble opening, reading, or
**	closing this catalog.  It will appear if that catalog
**	has been clobbered, or if it doesn't exist.
**
** System Status:
**	If it occurs while processing "GRANT" or "REVOKE" statements for
**	database privileges, the statement is aborted.
**
** Recommendation:
**	Contact your system administrator.
*/
E_QE022E_IIUSER:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iiuser catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if QEF has trouble opening, reading, or closing
**	this catalog.  It will appear if that catalog has been clobbered,
**	or if it doesn't exist.
**
** System Status:
**	If it occurs while processing "GRANT" or "REVOKE" statements for
**	database privileges, the statement is aborted.
**
** Recommendation:
**	Contact your system administrator.
*/
E_QE022F_SCU_INFO_ERROR:SS50000_MISC_ERRORS
"Internal error:\n
QEF detected an error in scu_information.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if QEF gets a bad return code from
**	scu_information().
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Contact your system administrator.
*/
/*
**NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*
** The following messages are reserved.
**	They are translated to user errors by qef_error().
**	See ERUSF.MSG for an explanation of these user errors.
**					Translation
**					-----------
**	E_QE0230_USRGRP_NOT_EXIST	E_US18F6_6390
**	E_QE0231_USRMEM_ADDED		E_US18F6_6391
**	E_QE0232_USRMEM_DROPPED		E_US18F6_6392
**	E_QE0233_ALLDBS_NOT_AUTH	E_US18C6_6342
**	E_QE0234_USER_ROLE		E_US18F6_6393
**	E_QE0235_USER_GROUP		E_US18F6_6394
*/
/* E_QE0236 is unused, no longer happens, available. */
/*
**NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*
** E_QE0237_DROP_DBA -- This message is reserved.
**	                It is translated to E_US18C3_6339 by qef_error().
**	                See ERUSF.MSG for an explanation of this user error.
*/
/*
**NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*NOTE*
** E_QE0238_NO_MSTRAN -- This message is reserved.
**	                 It is translated to E_US08B4_2228 by qef_error().
**	                 See ERUSF.MSG for an explanation of this user error.
**/
E_QE0239_SEQ_PERMIT_ABSENT:SS42505_OBJ_NOT_FOUND
"Sequence '%0c' owned by '%1c' does not exist for GRANT, REVOKE or DROP PERMIT
privilege modification."
/*%
** Parameters:
**	%0c - sequence name
**	%1c - current user
**
** Explanation:
**	The named sequence does not exist.  Privileges may only be granted or
**	revoked on existing sequences.  If a sequence is dropped, all privileges
**	associated with that sequence are also dropped.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Use the "CREATE SEQUENCE" statement to recreate the sequence.
*/
E_QE0240_BAD_LOC_TYPE:SS50000_MISC_ERRORS
"The iiQEF_alter_extension internal procedure currently supports changing a
defaultable WORK location extension to an auxiliary WORK location extension,
and vice versa.  No further operations are supported."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to change the attributes of a WORK location
**	but the internal procedure was invoked with invalid
**	parameters.  This procedure is used to change a defaultable WORK
**	location extension to an auxiliary WORK location extension, or vice
**	versa.  Upon decoding the parameters sent to it, we discovered that
**	neither of these operations was being attempted.  Anything else is
**	invalid, so we issue this error.
**
**	Users should not be invoking this procedure themselves.  The accessdb
**	utility should be the only program to call this procedure, and if this
**	error is encountered while using accessdb, then this should be
**	considered an accessdb bug.
**
** System Status:
**	The procedure invocation is nullified; no net effect.  The accessdb
**	utility reports an error, and the operation fails.
**
** Recommendation:
**	Contact your system administrator.
*/
E_QE0241_DB_NOT_EXTENDED:SS50000_MISC_ERRORS
"This database has not been extended to this location.  Before altering the
extension type of this location you must first extend the database to it."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to change the attributes of a WORK location before
**	the database was extended to this location; this is not allowed.  You
**	must first extend the database to a location, and then alter the
**	extension.
**
** System Status:
**	The accessdb utility fails without making the requested change to
**	the location.
**
** Recommendation:
**	Extend the database to this location first.
*/
E_QE0242_BAD_EXT_TYPE:SS50000_MISC_ERRORS
"The location extension you are attempting to alter is not the correct type;
either it is not a WORK location extension or the operation you are attempting
has already been done."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to invoke the iiQEF_alter_extension internal
**	procedure to change a location extension; however, the type of this
**	extension was either not a WORK location (which is illegal) or it
**	was not the specified type of WORK location.
**
**	For example, if you attempt to change a defaultable WORK location
**	extension to an auxiliary WORK location extension, but this extension
**	is already an auxiliary WORK location extension, you will get this
**	error.
**
** System Status:
**	The accessdb utility fails without making the requested change to the
**	location.
**
** Recommendation:
**	Correct your action based on the above information.
*/
E_QE0243_CNF_INCONSIST:SS5000J_INCONSISTENT_DBMS_CAT
"Config file is inconsistent with information in IIEXTEND catalog.  Either the
extent listed in the IIEXTEND catalog was not in the database's config file, or
the extension type bits in the IIEXTEND catalog were inconsistent with the
corresponding type bits in the config file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Preliminary checks in QEF based on the information in the iiextend
**	catalog were satisfied, but DMF found that the information in the
**	database's configuration file was inconsistent with this information.
**
**	This is an internal error and should be reported as a bug.
**
** System Status:
**	The procedure invocation is nullified; no net effect.
**
** Recommendation:
**	Contact your system administrator.
*/
E_QE0244_TOO_FEW_LOCS:SS50000_MISC_ERRORS
"There are currently no WORK locations being used for this session.  The
operation you are attempting requires at least one WORK location."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted an operation which required a minimum of one
**	WORK location; but no WORK location was found.  WORK locations are
**	required for operations such as sorting.  See the documentation set
**	for more information.  In particular, see the sections on WORK
**	locations and the SET WORK LOCATIONS command.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	If you want to successfully complete this operation, you must have at
**	least one WORK location.  If you or the application you are running has
**	issued a SET WORK LOCATIONS command to reduce the number of WORK
**	locations, you may add some of these back using this command.  If
**	you have no WORK locations because of a GRANT NOALLOCATE, you must
**	consult the DBA of the database to obtain a GRANT ALLOCATE on at least
**	one WORK location.
*/
E_QE0245_TOO_MANY_LOCS:SS50002_LIMIT_EXCEEDED
"There are currently more than 256 WORK locations being used for this session.
This exceeds the maximum allowed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The caller attempted an operation which can make use of a maximum of
**	256 WORK locations, but the operation would require more.  WORK
**	locations are required for operations such as sorting.  See the
**	documentation set for more information.  In particular, see the
**	sections on WORK locations and the "SET WORK LOCATIONS" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	If you want to successfully complete this operation, you must decrease
**	the number of WORK locations your session is using.  You can accomplish
**	this either with a "SET WORK LOCATIONS" statement, or by having the DBA
**	of the database issue a "GRANT NOALLOCATE" statement. See the 
**	documentation set for more details.
*/
E_QE0250_ABANDONED_PERMIT:SS2B000_DEP_PRIV_EXISTS
"DROP PERMIT/REVOKE ... RESTRICT: cascading revocation must be specified
\nsince otherwise permit %0d on %1c '%2c'
\nowned by '%3c' would become abandoned."
/*%
** Parameters:
**	%0d - permit number
**	%1c - object type
**	%2c - object name
**	%3c - object owner
**
** Explanation:
**	The "DROP PERMIT" or "REVOKE ... RESTRICT" issued by the user will not
**	be allowed to complete, because it would render some permit abandoned.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Use "REVOKE ... CASCADE" to revoke a privilege and destroy all
**	objects/permits dependent on it.
*/
E_QE0251_ABANDONED_VIEW:SS2B000_DEP_PRIV_EXISTS
"DROP PERMIT/REVOKE ... RESTRICT: cascading revocation must be specified
\nsince otherwise view '%0c' owned by '%1c'
\nwould become abandoned."
/*%
** Parameters:
**	%0c - view name
**	%1c - view owner
**
** Explanation:
**	The "DROP PERMIT" or "REVOKE ... RESTRICT" issued by the user will not
**	be allowed to complete, because it would render some view abandoned.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Use "REVOKE ... CASCADE" to revoke a privilege and destroy all
**	objects/permits dependent on it.
*/
E_QE0252_ABANDONED_DBPROC:SS2B000_DEP_PRIV_EXISTS
"DROP PERMIT/REVOKE ... RESTRICT: cascading revocation must be specified
\nsince otherwise database procedure '%0c' owned by '%1c'
\nwould become abandoned."
/*%
** Parameters:
**	%0c - database procedure name
**	%1c - database procedure owner
**
** Explanation:
**	The "DROP PERMIT" or "REVOKE ... RESTRICT" issued by the user will not
**	be allowed to complete, because it would render some database procedure
**	abandoned.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Use "REVOKE ... CASCADE" to revoke a privilege and destroy all
**	objects/permits dependent on it.
*/
E_QE0253_ALL_PRIV_NONE_REVOKED:SS01006_PRIV_NOT_REVOKED
"REVOKE ALL [PRIVILEGES]: No privileges were revoked from %0c '%1c'
\non '%2c' because none were granted by '%3c'."
/*%
** Parameters:
**	%0c - grantee type
**	%1c - grantee name
**	%2c - object name
**	%3c - grantor name
**
** Explanation:
**	A "REVOKE ALL [PRIVILEGES]" was specified, but for this combination of
**	grantor, object, and grantee, no privileges could be revoked because
**	none existed.
**
** System Status:
**	Processing continues for the remaining grantees and/or objects.
**
** Recommendation:
**	None.
*/
E_QE0254_SOME_PRIV_NOT_REVOKED:SS01006_PRIV_NOT_REVOKED
"REVOKE: Some of specified privileges on '%0c' were not revoked
\nfrom %1c '%2c' because were not granted by '%3c'."
/*%
** Parameters:
**	%0c - object name
**	%1c - grantee type
**	%2c - grantee name
**	%3c - grantor name
**
** Explanation:
**	A "REVOKE <privilege list>" was specified, but for this combination of
**	grantor, object, and grantee some of the specified privileges could not
**	be revoked, because they did not exist.
**
** System Status:
**	Those of the specified privileges that were found will be revoked;
**	processing will continue for the remaining combinations of objects
**	and/or grantees.
**
** Recommendation:
**	None.
*/
E_QE0255_NO_SCHEMA_TO_DROP:SS42505_OBJ_NOT_FOUND
"DROP SCHEMA: schema '%0c' owned by '%1c' does not exist."
/*%
** Parameters:
**	%0c - schema name
**	%1c - schema owner
**
** Explanation:
**	The user attempted to drop a nonexistent schema.  
**
** System Status:
**	The query will be aborted.
**
** Recommendation:
**	If the specified name was incorrect, correct the query and reenter.
*/
E_QE0256_NONEMPTY_SCHEMA:SS42000_SYN_OR_ACCESS_ERR
"DROP SCHEMA ... RESTRICT: Restricted destruction of schema '%0c'
\nowned by '%1c' could not be performed because that schema
\ncontains a %2c '%3c'."
/*%
** Parameters:
**	%0c - schema name
**	%1c - schema owner
**	%2c - object type (one of: table, view, synonym, database procedure,
**	        or dbevent)
**	%3c - object name
**
** Explanation:
**	The user attempted restricted destruction of a schema that contains
**	at least one table, view, synonym, dbevent or database procedure.
**	Restricted destruction of a schema may proceed only if it does not
**	contain any tables, views, synonyms, database procedures, or dbevents.
**
** System Status:
**	The query will be aborted.
**
** Recommendation:
**	If the user wants to destroy the schema along with any objects
**	contained therein, a "DROP SCHEMA <schema name> CASCADE" must be
**	used.
*/
E_QE0257_ABANDONED_CONSTRAINT:SS2B000_DEP_PRIV_EXISTS
"DROP PERMIT/REVOKE ... RESTRICT: cascading revocation must be specified
\nsince otherwise constraint '%0c'
\non table '%1c'.'%2c' would become abandoned."
/*%
** Parameters:
**	%0c - constraint name
**	%1c - schema name
**	%2c - table name
**
** Explanation:
**	A "DROP PERMITA " or "REVOKE ... RESTRICT" issued by the user will
**	not be allowed to complete because it would render some constraint
**	abandoned.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Use "REVOKE ... CASCADE" to revoke a privilege and destroy all
**	objects/permits dependent on it.
*/
E_QE0258_ILLEGAL_XACT_STMT:SS50000_MISC_ERRORS
"A commit/abort/rollback was attempted in a context where transaction
\n statements are not allowed."
/*%
** Parameters:
**	None.
**
** System Status:
**	The commit/abort/rollback transaction will be aborted.
**
** Recommendation:
**	Remove the transaction modifying statement (COMMIT/ABORT/ROLLBACK)
**	from the database procedure or direct execute immediate.
*/
E_QE0259_INTERNAL_DEADLOCK:SS50000_MISC_ERRORS
"Unable to continue execution of procedure due to deadlocks while
\n restarting a transaction."
/*%
** Parameters:
**	None.
**
** System Status:
**	The procedure has been aborted.
**
** Recommendation:
**	Contact your system administrator.
*/
E_QE0280_MAC_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred while performing MAC arbitration on an object."
/*%
** Parameters:
**	None.
**
** System Status:
**	Access to the object is denied.
**
** Recommendation:
**	Check the error log for more information about why this error
**	occurred, then correct the problem.
*/
E_QE0281_DEF_LBL_ERROR:SS5000B_INTERNAL_ERROR
"Error attempting to get the default security label."
/*%
** Parameters:
**	None
**
** Explanation:
**	QEF tried to get the default label, but failed unexpectedly.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Check the error log for more information about why this error
**	occurred, then correct the problem.
*/
E_QE0282_IIPROFILE:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iiprofile catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if QEF has trouble opening, reading, or
**	closing this catalog.  Check the error log for
**	more information about why this condition occurred.
**
** System Status:
**	If it occurs while processing CREATE/ALTER USER/PROFILE statements
**	for subject privileges, the statement is aborted.
**
** Recommendation:
**	Contact your system administrator.
*/
E_QE0285_PROFILE_USER_MRG_ERR:SS5000B_INTERNAL_ERROR
"Error merging profile information with user tuples when a profile was
altered or dropped."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to propagate changed profile
**	information to user tuples. 
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Check the error log for more information about why this
**	error occurred.
*/
E_QE0286_PROFILE_ABSENT:SS5000B_INTERNAL_ERROR
"Profile '%0c' was not found in the iiprofile catalog."
/*%
** Parameters:
**	%0c - profile name
**
** Explanation:
**	A profile tuple was not found in this catalog when it
**	was expected to be.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Check the error log for more information about why this
**	error occurred.
*/
E_QE0287_PROFILE_IN_USE:SS42000_SYN_OR_ACCESS_ERR
"DROP PROFILE: This profile is assigned to one or more users.\n
Use the CASCADE option if you wish to drop a profile which is in use."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to drop a profile which is in use by users.
**	This is only allowed when the "CASCADE" option is specified, when
**	those users will have their profile set to the default profile.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Check the error log for more information about why this
**	error occurred.
*/
E_QE0288_DROP_DEFAULT_PROFILE:SS42000_SYN_OR_ACCESS_ERR
"DROP PROFILE: The default profile may not be dropped."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to drop the default profile. This is not
**	allowed. The only operation allowed on the default profile is
**	ALTER.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Try to ALTER the profile.
*/
E_QE0289_EXTEND_DB_LOC_LABEL:SS42000_SYN_OR_ACCESS_ERR
"Database label must dominate location label when extending a database to
a location"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to extend a database to a location with
**	a security label which it does not dominate. Locations are 
**	container objects whose label define the lowest label of objects
**	at that location. Because of this, databases extended to a new location
**	must dominate the location label.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Make sure the database label dominates the location label.
*/
E_QE028A_EXTEND_DB_LOC_ERR:SS5000B_INTERNAL_ERROR
"Error occurred comparing database and location security labels."
/*%
** Parameters: 
**	None.
**
** Explanation:
**	An unexpected error occurred while attempting to compare the
**	security labels of a location and a database.
**
** System Status:
**	Operation is rejected
**
** User Action:
**	Check the error log for more information why this
**	occurred. If the condition persists contact your support
**	representative
*/
E_QE028B_CREATE_ALARM_ERR:SS5000B_INTERNAL_ERROR
"Error occurred in QEF while creating a security alarm"
/*%
** Parameters: 
**	None
**
** Explanation:
**	An unexpected error occurred while attempting to create a 
**	security alarm.
**
** System Status:
**	Operation is rejected
**
** User Action:
**	Check the error log for more information why this
**	occurred. If the condition persists contact your support
**	representative
*/
E_QE028C_DROP_ALARM_ERR:SS5000B_INTERNAL_ERROR
"Error occurred in QEF while dropping a security alarm"
/*%
** Parameters: 
**	None
**
** Explanation:
**	An unexpected error occurred while attempting to drop a 
**	security alarm.
**
** System Status:
**	Operation is rejected
**
** User Action:
**	Check the error log for more information why this
**	occurred. If the condition persists contact your support
**	representative
*/
E_QE028D_ALARM_DBEVENT_ERR:SS5000B_INTERNAL_ERROR
"Error occurred in QEF while raising a dbevent associated with a security
alarm firing."
/*%
** Parameters: 
**	None
**
** Explanation:
**	An unexpected error occurred while attempting to raise a
**	dbevent associated  with a dbevent.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Check the error log for more information about why this
**	error occurred. If the condition persists, contact your
**	Technical Support representative.
*/
E_QE028E_DB_NOT_UNEXTENDED:SS50000_MISC_ERRORS
"This database has not been extended to this location. Before unextending
this location you must first extend the database to it."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to unextend a database that has not been extended
**	before; this is not allowed.  You must first extend the database to a
**	location, and then unextend it.
**
** System Status:
**	The unextenddb utility shows the error without making the requested change to
**	the location.
**
** Recommendation:
**	Extend the database to this location first.
*/
E_QE0300_USER_MSG:SS5000B_INTERNAL_ERROR 
"%0c %1d from %2c: '%3c'" 
/*%
** Parameters:
**	%0c - "MESSAGE" if MESSAGE statement; "ERROR" if RAISE ERROR statement
**	%1d - message/error number, or 0 if not specified
**	%2c - effective user identifier
**	%3c - message/error text, if specified
**
** Explanation:
**	The "DESTINATION" clause of the "MESSAGE" or "RAISE ERROR" statement
**	specified the ERROR_LOG option.
**
** System Status:
**	The message is written to the error log.
**
** Recommendation:
**	None.
*/
E_QE0301_TBL_SIZE_CHANGE:SS5000B_INTERNAL_ERROR
" The table referenced in this query has significantly changed in size.  The
query or database procedure will be re-optimized."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The database procedure references a table whose size has significantly
**	changed since the procedure was compiled.  The procedure will be re-
**	compiled.
**
** System Status:
**	The query is recompiled.  This is not an error and will not be
**	reported.
**
** Recommendation:
**	None.
*/
E_QE0302_SCHEMA_EXISTS:SS42502_DUPL_OBJECT
"CREATE SCHEMA: Schema '%0c' already exists."
/*%
** Parameters:
**	%0c -schema name
**
** Explanation:
**	A "CREATE SCHEMA AUTHORIZATION" was issued a second time by the
**	same user (having the same autorization identifier).  The schema
**	already exists.
**
** System Status:
**	QEF will reject the query.
**
** Recommendation:
**	Issue the DDL statements within the "CREATE SCHEMA" statement without
**	the "CREATE SCHEMA" wrapper.
*/
E_QE0303_PARSE_EXECIMM_QTEXT:SS5000B_INTERNAL_ERROR
"The query text for CRATE VIEW/GRANT statement\n
in CREATE SCHEMA statement needs to be parsed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	To resolve backward/forward references in a "CREATE SCHEMA" statement,
**	the query text for "CREATE VIEW/GRANT" statement(s) is parsed after the
**	objects on which the definition of the view/privilege depends is 
**	created in QEF.
**
** System Status:
**	The query text will be parsed. This is not an error and will not be
**	reported.
**
** Recommendation:
**	None.
*/
E_QE0304_SCHEMA_DOES_NOT_EXIST:SS5000B_INTERNAL_ERROR
"There is no schema named '%0c'."
/*%
** Parameters:
**	%0c - schema name
**
** Explanation:
**	An unknown schema name was encountered.  This may happen during
**	DDL if an attempt is made to create an object in a schema which
**	has not yet been defined.
**
** System Status:
**	QEF will reject the query.
**
** Recommendation:
**	Try creating the schema.  However, this may also indicate an internal
**	server error:  the server is trying to reference a schema which
**	it, itself, should have already created.  If that appears to be the
**	case, contact Ingres Corporation Technical Support.
*/
E_QE0305_UDT_DEFAULT_TOO_BIG:SS5000B_INTERNAL_ERROR
"The UDT attribute '%0c' has a default value which is too big.\n
The maximum size for UDT default values is %1d."
/*%
** Parameters:
**	%0c - UDT attribute
**	%1d - maximum size for a UDT default
**
** Explanation:
**	During the conversion of a pre-65 database to release level 65,
**	upgradedb fills in iidefault tuples for any pre-existing UDT attributes
**	which take defaults.  The iidefault tuple itself has a limited
**	length.  If a UDT takes a default that is longer, this error will
**	be emitted, and a default will not be filled in for this UDT.
**
** System Status:
**	The upgradedb utility will continue; however, this UDT will not have
**	a valid default.
**
** Recommendation:
**	Try recreating the UDT with a smaller default value.
*/
E_QE0306_ALLOCATE_BYREF_TUPLE:SS5000B_INTERNAL_ERROR
"We need to allocate a tuple for returning byref parameters."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Because SCF does not know the size of the byref parameters before
**	a procedure is called, SCF calls QEF, which then returns this
**	error to SCF, which can then allocate the tuple and call back QEF.
**
** System Status:
**	SCF will allocate the tuple and call qeq_query() again.
**
** Recommendation:
**	The user should never see this error.
*/
E_QE0307_INIT_MO_ERROR:SS5000B_INTERNAL_ERROR
"Could not initialize QEF management objects."
/*%
** Parameters:
**	None.
**
** Explanation:
**	During server startup, the QEF management objects couldn't be
**	registered with the IMA internal gateway.
**
**
** System Status:
**	QEF startup (and therefore server startup) will fail.
**
** Recommendation:
**	The user should never see this error.
*/
E_QE0308_COL_SIZE_MISCOMPILED:SS5000B_INTERNAL_ERROR
"A column's size was incorrectly compiled."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The code generator turned out bad datatype manipulation code.
**	More specifically, a column's size was incorrectly compiled.
**
**
** System Status:
**	The query will fail.
**
** Recommendation:
**	This indicates a bug in OPC (the code generator).  As a workaround,
**	try re-writing the query.
**
*/
E_QE0309_NO_BMCACHE_BUFFERS:SS5000B_INTERNAL_ERROR
"The DBMS cache is not configured for this page size."
/*%
** Parameters:
**	None.
**
** Explanation:
**      The DBMS cache is not configured for the page size specified.
**
** System Status:
**	The query will fail.
**
** Recommendation:
**      Specify a different page size, or reconfigure the DBMS cache
**      and restart the server.
**
*/
E_QE030A_TTAB_PARM_ERROR:SS50000_MISC_ERRORS
"Temporary table '%0c' doesn't match formal parameter definition of procedure '%1c'."
/*%
** Parameters:
**	%0c - global temporary table parameter.
**	%1c - database procedure. 
**
** Explanation:
**	A global temporary table parameter was passed to a procedure. 
**	Its definition did not exactly match the "set of" formal parameter
**	in the database procedure. 
**
** System Status:
**	QEF will reject the query.
**
** Recommendation:
**	The table's column definitions must exactly match the "set of"
**	parameter definition (column names, data types and nullability).
**	Assure that this is the case, and retry the "execute procedure"
**	statement.
*/
E_QE030B_RULE_PROC_MISMATCH:SS50000_MISC_ERRORS
"Row level rule '%0c' is calling SET OF procedure '%1c'. Must be statement level rule."
/*%
** Parameters:
**	%0c - rule name.
**	%1c - procedure name.
**
** Explanation:
**	A row level rule triggers the invocation of a procedure with a 
**	SET OF parameter. Only statement level rules may invoke SET OF 
**	procedures. 
**
** System Status:
**	QEF will reject the query.
**
** Recommendation:
**	Either the rule must be changed to be statement level, or the 
**	procedure must be recoded to use a simple (non-SET OF) parameter
**	list.
*/
E_QE030C_BYREF_IN_ROWPROC:SS50000_MISC_ERRORS
"Procedure '%0c' produces result rows and cannot be invoked with BYREF parameters."
/*%
** Parameters:
**	%0c - procedure name.
**
** Explanation:
**	A row producing procedure (one containing "return row" statements)
**	cannot be invoked with a "byref" parameter (the execute procedure 
**	statement included a parameter with the "byref" notation).
**
** System Status:
**	QEF will reject the query.
**
** Recommendation:
**	The "byref" notation must be removed from all parameters in the
**	execute procedure statement.
*/
E_QE030D_NESTED_ROWPROCS:SS50000_MISC_ERRORS
"Procedure '%0c' produces result rows and cannot be called from another procedure."
/*%
** Parameters:
**	%0c - procedure name.
**
** Explanation:
**	A row producing procedure (one containing "return row" statements)
**	cannot be invoked from within another procedure.
**
** System Status:
**	QEF will reject the query.
**
** Recommendation:
**	Row producing procedures may not be the target of nested calls. 
*/
E_QE030E_RULE_ROWPROCS:SS50000_MISC_ERRORS
"Procedure '%0c' produces result rows and cannot be invoked from a rule definition."
/*%
** Parameters:
**	%0c - procedure name.
**
** Explanation:
**	A row producing procedure (one containing "return row" statements)
**	cannot be the target of a rule invocation.
**
** System Status:
**	QEF will reject the query.
**
** Recommendation:
**	Do not define rules which execute row producing procedures.
*/
E_QE0310_INVALID_TPROC_ACT:SS50000_MISC_ERRORS
"Unsupported procedure statement type: %0c"
/*%
** Parameters:
**      None.
**
** Explanation:
**      Unsupported statement type is found in the table procedure.
**
** System Status:
**      QEF will reject the query.
**
** Recommendation:
**      Use allowed statement types in the table procedure.
*/
E_QE0400_GATEWAY_ERROR:SS50005_GW_ERROR
"An error was returned from a Gateway.\n
Check the error log (errlog.log) for more information\n
concerning the specific problem,"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error status was returned by an gateway. Check the
**	error log for more specific information on the problem.
**
** System Status:
**	The query/statement will be aborted.
**
** Recommendation:
**	Correct the problem and rerun the query.
*/
E_QE0401_GATEWAY_ACCESS_ERROR:SS50005_GW_ERROR
"An illegal operation was attempted on a Gateway table,\n
such as Modify, Alter, Relocate or Reorganize, or by an attempt
to update a read-only gateway table."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to perform an illegal operation on a gateway table.
**
** System Status:
**	The query/statement will be aborted.
**
** Recommendation:
**	Correct the problem and rerun the query.
*/
E_QE0500_RQF_GENERIC_ERR:SS50000_MISC_ERRORS
"RQF reported an unspecified error."
E_QE0501_WRONG_COLUMN_COUNT:SS50000_MISC_ERRORS
"RQF detected a column-number mismatch."
E_QE0502_NO_TUPLE_DESCRIPTION:SS50000_MISC_ERRORS
"RQF detected a missing tuple description."
E_QE0503_TOO_MANY_COLUMNS:SS50000_MISC_ERRORS
"RQF detected an excessive column count."
E_QE0504_BIND_BUFFER_TOO_SMALL:SS50000_MISC_ERRORS
"RQF detected an excessively small bind buffer."
E_QE0505_CONVERSION_FAILED:SS50000_MISC_ERRORS
"RQF detected a data conversion failure."
E_QE0506_CANNOT_GET_ASSOCIATION:SS08500_LDB_UNAVAILABLE
"Either the node or the database is not defined or cannot be reached."
E_QE0507_BAD_REQUEST_CODE:SS50000_MISC_ERRORS
"RQF detected an invalid operation."
E_QE0508_SCU_MALLOC_FAILED:SS50003_EXHAUSTED_RESOURCE
"RQF failed to allocate space from SCU."
E_QE0509_ULM_STARTUP_FAILED:SS50000_MISC_ERRORS
"RQF failed to start up ULM."
E_QE0510_ULM_OPEN_FAILED:SS50000_MISC_ERRORS
"RQF failed to open with ULM."
E_QE0511_INVALID_READ:SS50000_MISC_ERRORS
"RQF detected an invalid read operation."
E_QE0512_INVALID_WRITE:SS50000_MISC_ERRORS
"RQF detected an invalid write operation."
E_QE0513_ULM_CLOSE_FAILED:SS50000_MISC_ERRORS
"RQF failed to close with ULM."
E_QE0514_QUERY_ERROR:SS50000_MISC_ERRORS
"An (LDB) query error or warning occurred."
E_QE0515_UNEXPECTED_MESSAGE:SS50000_MISC_ERRORS
"RQF detected an unexpected message."
E_QE0516_CONVERSION_ERROR:SS50000_MISC_ERRORS
"RQF detected a conversion error."
E_QE0517_NO_ACK:SS50000_MISC_ERRORS
"RQF failed to obtain an acknowledgment."
E_QE0518_SHUTDOWN_FAILED:SS50000_MISC_ERRORS
"RQF failed to shutdown."
E_QE0519_COMMIT_FAILED:SS50000_MISC_ERRORS
"RQF failed to commit."
E_QE0520_ABORT_FAILED:SS50000_MISC_ERRORS
"RQF failed to abort."
E_QE0521_BEGIN_FAILED:SS50000_MISC_ERRORS
"RQF failed to begin a transaction."
E_QE0522_END_FAILED:SS50000_MISC_ERRORS
"RQF failed to end a transaction."
E_QE0523_COPY_FROM_EXPECTED:SS50000_MISC_ERRORS
"RQF detected a missing COPY FROM operation."
E_QE0524_COPY_DEST_FAILED:SS50000_MISC_ERRORS
"RQF failed to copy to a destination LDB."
E_QE0525_COPY_SOURCE_FAILED:SS50000_MISC_ERRORS
"RQF failed to copy from a source LDB."
E_QE0526_QID_EXPECTED:SS50000_MISC_ERRORS
"RQF detected a missing query id."
E_QE0527_CURSOR_CLOSE_FAILED:SS50000_MISC_ERRORS
"RQF failed to close a local cursor."
E_QE0528_CURSOR_FETCH_FAILED:SS50000_MISC_ERRORS
"RQF failed to fetch from a local cursor."
E_QE0529_CURSOR_EXEC_FAILED:SS50000_MISC_ERRORS
"RQF failed to open a local cursor."
E_QE0530_CURSOR_DELETE_FAILED:SS50000_MISC_ERRORS
"RQF failed to delete the current row of a local cursor."
E_QE0531_INVALID_CONTINUE:SS50000_MISC_ERRORS
"RQF detected an invalid CONTINUE operation."
E_QE0532_DIFFERENT_TUPLE_SIZE:SS50000_MISC_ERRORS
"The schema of a local table has been changed since it was created."
E_QE0533_FETCH_FAILED:SS50000_MISC_ERRORS
"RQF failed to retrieve data."
E_QE0534_COPY_CREATE_FAILED:SS50000_MISC_ERRORS
"RQF failed to create a temporary LDB table."
E_QE0535_BAD_COL_DESC_FORMAT:SS50000_MISC_ERRORS
"RQF detected a column-description format mismatch."
E_QE0536_COL_DESC_EXPECTED:SS50000_MISC_ERRORS
"RQF detected a missing column description."
E_QE0537_II_LDB_NOT_DEFINED:SS50000_MISC_ERRORS
"RQF detected an undefined LDB."
E_QE0538_ULM_ALLOC_FAILED:SS50003_EXHAUSTED_RESOURCE
"RQF failed to allocate space from ULM."
E_QE0539_INTERRUPTED:SS50000_MISC_ERRORS
"RQF was interrupted."
E_QE0540_UNKNOWN_REPEAT_Q:SS50000_MISC_ERRORS
"RQF detected an unknown repeat query id."
E_QE0541_ERROR_MSG_FROM_LDB:SS50000_MISC_ERRORS
"RQF detected an error message from an LDB."
E_QE0542_LDB_ERROR_MSG:SS50000_MISC_ERRORS
"RQF detected an LDB error message."
E_QE0543_CURSOR_UPDATE_FAILED:SS50000_MISC_ERRORS
"RQF detected a CURSOR UPDATE failure."
E_QE0544_CURSOR_OPEN_FAILED:SS50000_MISC_ERRORS
"RQF detected a CURSOR OPEN failure."
E_QE0545_CONNECTION_LOST:SS08006_CONNECTION_FAILURE
"RQF detected the loss of an LDB association."
E_QE0546_RECV_TIMEOUT:SS08006_CONNECTION_FAILURE
"RQF detected a timeout condition."
E_QE0547_UNAUTHORIZE_USER:SS50000_MISC_ERRORS
"User unauthorized to connect to LDB"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The current user is not authorized to connect to the LDB.
**
** System Status:
**	The connection is aborted.
**
** Recommendation:
**	Request proper authorization to connect to the LDB.
*/
E_QE0548_SECURE_FAIL:SS50000_MISC_ERRORS
"RQF failed to secure local transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an internal error.  One of the local servers participating in
**	the distributed transaction refused to secure a local transaction.
**
** System Status:
**	TPF is expected to have taken the appropriate action.
**
** Recommendation:
**	None.
*/
E_QE0549_RESTART_FAILED:SS50000_MISC_ERRORS
"RQF failed to restart a session for a failed distributed transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an internal error.  RQF failed to establish a connection with
**	a database server to recover a failed distributed transaction.
**
** Recommendation:
**	Other error messages will indicate corrective actions, if any.
*/
E_QE0700_TPF_GENERIC_ERR:SS50000_MISC_ERRORS
"TPF reported an unspecified error."
E_QE0701_INVALID_REQUEST:SS50000_MISC_ERRORS
"TPF detected an invalid operation."
E_QE0702_SCF_MALLOC_FAILED:SS50003_EXHAUSTED_RESOURCE
"TPF failed to allocate space from SCF."
E_QE0703_SCF_MFREE_FAILED:SS50000_MISC_ERRORS
"TPF failed to free space with SCF."
E_QE0704_MULTI_SITE_WRITE:SS25000_INV_XACT_STATE
"Updating more than 1 site is not allowed within a transaction."
E_QE0705_UNKNOWN_STATE:SS25000_INV_XACT_STATE
"TPF detected an unknown transaction state."
E_QE0706_INVALID_EVENT:SS25000_INV_XACT_STATE
"TPF detected an invalid transaction event."
E_QE0707_INVALID_TRANSITION:SS25000_INV_XACT_STATE
"TPF detected an invalid transaction transition."
E_QE0708_TXN_BEGIN_FAILED:SS50000_MISC_ERRORS
"TPF failed to begin a transaction."
E_QE0709_TXN_FAILED:SS50000_MISC_ERRORS
"TPF detected a transaction failure."
E_QE0710_SAVEPOINT_FAILED:SS50000_MISC_ERRORS
"TPF failed to establish a savepoint."
E_QE0711_SP_ABORT_FAILED:SS50000_MISC_ERRORS
"TPF failed to abort to a savepoint."
E_QE0712_SP_NOT_EXIST:SS50000_MISC_ERRORS
"TPF detected a non-existent savepoint."
E_QE0713_AUTO_ON_NO_SP:SS50000_MISC_ERRORS
"AUTOCOMMIT is not allowed since a savepoint exists."
E_QE0714_ULM_STARTUP_FAILED:SS50000_MISC_ERRORS
"TPF detected a ULM startup failure."
E_QE0715_ULM_OPEN_FAILED:SS50000_MISC_ERRORS
"TPF detected a ULM open failure."
E_QE0716_ULM_ALLOC_FAILED:SS50003_EXHAUSTED_RESOURCE
"TPF detected a ULM allocation failure."
E_QE0717_ULM_CLOSE_FAILED:SS50000_MISC_ERRORS
"TPF detected a ULM close failure."
E_QE0720_INTERNAL:SS50000_MISC_ERRORS
"TPF detected an internal error."
/*%
** Parameters:
**	None.
**
** System Status:
**	This is an internal error.  TPF will act accordingly.
**
** Recommendation:
**	This is an informational message logged to the error log file; other
**	error messages will have corrective recommendations, if any.
*/
E_QE0721_LOGDX_FAILED:SS50000_MISC_ERRORS
"TPF could not log the distributed transaction."
/*%
** Parameters:
**	None.
**
** System Status:
**	TPF has aborted the distributed transaction.
**
** Recommendation:
**	This is an informational message logged to the error log file; other
**	error messages will have corrective recommendations, if any.
**
**	(A possible contributing problem: the CDB becomes unavailable.)
*/
E_QE0722_SECURE_FAILED:SS50000_MISC_ERRORS
"TPF could not secure the distributed transaction."
/*%
** Parameters:
**	None.
**
** System Status:
**	TPF has aborted the distributed transaction.
**
** Recommendation:
**	This is an informational message logged to the error log file; other
**	error messages will have corrective recommendations, if any.
**
**	(One possible contributing problem: an LDB becomes unavailable.)
*/
E_QE0723_COMMIT_FAILED:SS40000_XACT_ROLLBACK
"TPF could not commit the distributed transaction."
/*%
** Parameters:
**	None.
**
** System Status:
**	TPF has aborted the distributed transaction.
**
** Recommendation:
**	This is an informational message logged to the error log file; other
**	error messages will have corrective recommendations, if any.
**
**	(One possible contributing problem: an LDB refuses to commit or becomes
**	unavailable before a unanimous COMMIT decision can be reached.)
*/
E_QE0902_NO_LDB_TAB_ON_CRE_LINK:SS42000_SYN_OR_ACCESS_ERR
"Cannot register a non-existent LDB table."
E_QE0960_CONNECT_ON_CONNECT:SS42000_SYN_OR_ACCESS_ERR
"Invalid nested DIRECT CONNECT."
E_QE0961_CONNECT_ON_XACTION:SS25000_INV_XACT_STATE
"DIRECT CONNECT is not allowed within a transaction."
E_QE0962_DDL_CC_ON_XACTION:SS25000_INV_XACT_STATE
"DDL_CONCURRENCY cannot be changed within a transaction."
E_QE096E_INVALID_DISCONNECT:SS42000_SYN_OR_ACCESS_ERR
"Invalid DIRECT DISCONNECT."
E_QE0970_BAD_USER_NAME:SS28000_INV_AUTH_SPEC
"Invalid user name detected."
E_QE0980_LDB_REPORTED_ERROR:SS50000_MISC_ERRORS
"Error reported by LDB."
E_QE0990_2PC_BAD_LDB_MIX:SS50000_MISC_ERRORS
"The query involves LDBs that cannot process 2PC or cause a 1PC/2PC \
conflict in the current distributed transaction."
/*%
** Parameters:
**	None.
**
** System Status:
**	QEF will abort the 2PC transaction because of an attempted COMMIT/ABORT
**	by the LDB when executing a database procedure or when executing direct
**	execute immediate.
**
** Recommendation:
**	Remove the transaction modifying statement (COMMIT/ABORT/ROLLBACK) from 
**	the database procedure or direct execute immediate.
*/
E_QE0991_LDB_ALTERED_XACT:SS50000_MISC_ERRORS
"The query has been aborted because of an attempted commit/abort \
within a registered database procedure or direct execute immediate."
/*%
** Parameters:
**	None.
**
** System Status:
**	QEF will reject the query and leave the distributed transaction intact.
**
** Recommendation:
**	Analyze the LDBs involved in the transaction and correct the offending
**	query to eliminate the LDBs that would conflict with the transaction.
*/
E_QE1001_NO_ACTIONS_IN_QP:SS50000_MISC_ERRORS
"QEF consistency check.  There are no actions in the current QP."
E_QE1002_BAD_ACTION_COUNT:SS50000_MISC_ERRORS
"QEF consistency check. The count of actions does not agree with the list
of action headers."
E_QE1003_NO_VALID_LIST:SS50000_MISC_ERRORS
"QEF consistency check.  The list of validation is missing from the current
action."
E_QE1004_TABLE_NOT_OPEN:SS50000_MISC_ERRORS
"QEF consistency check.  Error validating an action.  Trying to position a
table that has not been opened."
E_QE1005_WRONG_JOIN_NODE:SS50000_MISC_ERRORS
"QEF consistency check. Error in QP, qen_sort is being used by a node other
than qen_sijoin."
E_QE1006_BAD_SORT_DESCRIPTOR:SS50000_MISC_ERRORS
"QEF consistency check. The QEN_SHD structure is not clean.  Some memory
could be lost."
E_QE1007_BAD_SORT_COUNT:SS50000_MISC_ERRORS
"QEF consistency check. No tuples in the sort buffer or the list of DM_MDATA
does not agree with the count of tuples."
E_QE1009_DSH_NOTMATCH_QP:SS50000_MISC_ERRORS
"QEF consistency check.  The number of sort nodes in the QP is not the same
as the actual count."
E_QE1010_HOLDING_SORT_BUFFER:SS50000_MISC_ERRORS
"QEF consistency check. The session terminates while holding sort buffers."
E_QE1995_BAD_AGGREGATE_ACTION:SS50000_MISC_ERRORS
"Query plan contains a bad aggregate action."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an internal optimizer error.
**
** System Status:
**	QEF will reject the query and leave the distributed transaction intact.
**
** Recommendation:
**	Report the query and problem to Ingres Corporation Technical Support.
*/
E_QE1996_BAD_ACTION:SS50000_MISC_ERRORS
"QEF consistency check.  Invalid action detected."
E_QE1997_NO_ACTION:SS50000_MISC_ERRORS
"QEF consistency check.  No or invalid action detected."
E_QE1998_BAD_QMODE:SS50000_MISC_ERRORS
"QEF consistency check.  Invalid query mode detected."
E_QE1999_BAD_QP:SS50000_MISC_ERRORS
"QEF consistency check.  Invalid query plan detected."

/*
**  TheI_QE2nnn_  Messages are used as Informational
**  text in the security audit messages.  These
**  messages are put here in order to insure that they
**  can be language sensitive.
*/
I_QE2001_ROLE_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to define new ROLE."
I_QE2002_ROLE_DROP:SS5000B_INTERNAL_ERROR
"Attempt to remove a ROLE."
I_QE2003_DBACCESS_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to grant database access to user '%0c'."
I_QE2004_DBACCESS_DROP:SS5000B_INTERNAL_ERROR
"Attempt to revoke database access for user '%0c'."
I_QE2005_DBPRIV_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to grant database privileges '%1c' to user '%0c'."
I_QE2006_DBPRIV_DROP:SS5000B_INTERNAL_ERROR
"Attempt to revoke database privileges '%1c' for user '%0c'."
I_QE2007_GROUP_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to define new GROUP."
I_QE2008_GROUP_MEM_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to add new member '%0c' to group."
I_QE2009_GROUP_DROP:SS5000B_INTERNAL_ERROR
"Attempt to remove a GROUP."
I_QE200A_GROUP_MEM_DROP:SS5000B_INTERNAL_ERROR
"Attempt to remove member '%0c' from group."
I_QE200B_LOCATION_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to define a new LOCATION."
I_QE200C_USER_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to define a new USER."
I_QE200D_USER_DROP:SS5000B_INTERNAL_ERROR
"Attempt to remove a USER."
I_QE200E_SEC_EVENT_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to begin auditing security event '%0c'."
I_QE200F_SEC_EVENT_DROP:SS5000B_INTERNAL_ERROR
"Attempt to end auditing security event '%0c'."
I_QE2010_SEC_LEVEL_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to begin auditing level '%0c'."
I_QE2011_SEC_LEVEL_DROP:SS5000B_INTERNAL_ERROR
"Attempt to end auditing level '%0c'."
I_QE2012_DBPROC_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to create DATABASE PROCEDURE."
I_QE2013_DBPROC_DROP:SS5000B_INTERNAL_ERROR
"Attempt to drop a DATABASE PROCEDURE."
I_QE2014_VIEW_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to define a VIEW."
I_QE2015_VIEW_DROP:SS5000B_INTERNAL_ERROR
"Attempt to remove a VIEW."
I_QE2016_PROT_TAB_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to define a PROTECTION."
I_QE2017_PROT_DBP_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to define a PROTECTION."
I_QE2018_PROT_TAB_DROP:SS5000B_INTERNAL_ERROR
"Attempt to remove a PROTECTION."
I_QE2019_PROT_DBP_DROP:SS5000B_INTERNAL_ERROR
"Attempt to remove a PROTECTION."
I_QE201A_DATABASE_ACCESS:SS5000B_INTERNAL_ERROR
"Attempt to access a DATABASE."
I_QE201B_ROLE_ACCESS:SS5000B_INTERNAL_ERROR
"Attempt to access a ROLE."
I_QE201C_SEC_CAT_UPDATE:SS5000B_INTERNAL_ERROR
"Attempt to directly update security system catalog."
I_QE201D_DBPROC_EXECUTE:SS5000B_INTERNAL_ERROR
"Attempt to access a DATABASE PROCEDURE."
I_QE201E_TABLE_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to define a TABLE."
I_QE201F_DATABASE_EXTEND:SS5000B_INTERNAL_ERROR
"Attempt to extend a DATABASE to location '%0c'."
I_QE2020_PROT_REJECT:SS5000B_INTERNAL_ERROR
"Protection violation when accessing a TABLE."
I_QE2021_ROLE_ALTER:SS5000B_INTERNAL_ERROR
"Attempt to alter a ROLE."
I_QE2022_GROUP_ALTER:SS5000B_INTERNAL_ERROR
"Attempt to alter a GROUP."
I_QE2023_USER_ALTER:SS5000B_INTERNAL_ERROR
"Attempt to alter a USER."
I_QE2024_USER_ACCESS:SS5000B_INTERNAL_ERROR
"Attempt to authorize USER."
I_QE2025_TABLE_DROP:SS5000B_INTERNAL_ERROR
"Attempt to drop a TABLE."
I_QE2026_TABLE_ACCESS:SS5000B_INTERNAL_ERROR
"Attempt to access a TABLE."
I_QE2027_VIEW_ACCESS:SS5000B_INTERNAL_ERROR
"Attempt to access a VIEW."
I_QE2028_ALARM_EVENT:SS5000B_INTERNAL_ERROR
"Alarm signaled for TABLE."
I_QE2029_DATABASE_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to create a DATABASE."
I_QE202A_DATABASE_DROP:SS5000B_INTERNAL_ERROR
"Attempt to drop a DATABASE."
I_QE202B_LOCATION_ALTER:SS5000B_INTERNAL_ERROR
"Attempt to alter a LOCATION."
I_QE202C_LOCATION_DROP:SS5000B_INTERNAL_ERROR
"Attempt to drop a LOCATION."
I_QE202D_ALARM_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to create a SECURITY_ALARM."
I_QE202E_ALARM_DROP:SS5000B_INTERNAL_ERROR
"Attempt to drop a SECURITY_ALARM."
I_QE202F_RULE_ACCESS:SS5000B_INTERNAL_ERROR
"Attempt to access a RULE."
I_QE2030_PROT_EV_CREATE:SS5000B_INTERNAL_ERROR
"Attempt to define a PROTECTION for a DBEVENT."
I_QE2031_PROT_EV_DROP:SS5000B_INTERNAL_ERROR
"Attempt to remove a PROTECTION for a DBEVENT."
I_QE2032_EVENT_ACCESS:SS5000B_INTERNAL_ERROR
"Attempt to access a DBEVENT."
I_QE2033_SEC_VIOLATION:SS5000B_INTERNAL_ERROR
"Subject and object security labels must be identical
to perform this operation."
I_QE2034_READONLY_TABLE_ERR:SS5000B_INTERNAL_ERROR
"Attempt to perform the illegal operation
against a table in READONLY mode."
I_QE2035_SET_ID:SS5000B_INTERNAL_ERROR
"Attempt to %0c to '%1c'."
I_QE2036_INTEGRITY_DROP:SS5000B_INTERNAL_ERROR
"Attempt to drop an INTEGRITY."
I_QE2037_CONSTRAINT_TAB_IDX_ERR:SS5000B_INTERNAL_ERROR
"Attempt to perform DDL operations on a table/index which would
cause dependent constraints to be abandoned."
I_QE2038_CONSTRAINT_TAB_IDX_ERR:SS5000B_INTERNAL_ERROR
"Attempt to perform DDL operations on a table/index which would
cause dependent constraints to be abandoned."
E_QE7600_LDB_GENERIC_ERR:SS50000_MISC_ERRORS
"QEF detected a general failure reported by an LDB."
E_QE7601_LDB_BAD_SQL_QRY:SS42000_SYN_OR_ACCESS_ERR
"QEF detected an SQL query that cannot be processed by an LDB."
E_QE7602_LDB_XACT_ENDED:SS40001_SERIALIZATION_FAIL
"QEF aborted the distributed transaction because of an LDB's unilateral abort
action."
E_QE7610_SPROD_UPD_IIDB:SS42000_SYN_OR_ACCESS_ERR
"Attempt to put non-local database in production mode."
/*%
** Parameters:
**	None
**
** Explanation:
**	A SET PRODUCTION (ON|OFF) statement has been issued on a non-local
**	database
**
** System Status:
**	The statement is rejected
**
** Recommendation:
**	Only a local database can be put in production mode
*/
E_QE7612_PROD_MODE_ERR:SS42000_SYN_OR_ACCESS_ERR
"This operation cannot be executed in production mode."
/*%
** Parameters:
**	None
**
** Explanation:
**	A DDL statement has been issued on a database which is in
**	production mode
**
** System Status:
**	The statement is rejected
**
** Recommendation:
**	Switch the production mode off
*/


NO_SQLSTATE_MAPPING
