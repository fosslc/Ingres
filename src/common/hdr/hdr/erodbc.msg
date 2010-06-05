/*
**   (C) Copyright 1997,1998 Ingres Corporation
**
**   Description: Message file of Ingres ODBC Driver 
**
**   Name:  ERODBC.MSG
**
**   Change History
**   --------------
**
**   date        programmer          description
**
**   12/12/1997  Jean Teng           Created. 
**   10/22/1998  Dave Thole          Change class to next in line. 
**   10/03/2001  Dave Thole          Add messages for 3.5 upgrade. 
**   02/17/2005  Ralph Loen          SIR 113913.  Add messages for ODBC 
**                                   installer routines.
**   12/02/2005  Ralph Loen          Bug 115582.  Added 
**                                   F_OD0157_IDS_ERR_FRAC_TRUNC.
**     23-Jan-2008 (Ralph Loen) Bug 119800
**          Added F_OD0169_IDS_ERR_PREP_UCURS.
**    30-Jan-2008 (Ralph Loen) SIR 119723
**          Added F_OD0170_IDS_ERR_ROW_RANGE.
**    09-Mar-2010 (Ralph Loen) Bug 123392
**          Added F_OD0171_IDS_ERR_NULL_PTR.
**    03-May-2010 (Ralph Loen) Bug 123676
**          Added F_OD0172_IDS_ERR_CORRUPT_SECTION. 
*/

#define _OD_CLASS 235

F_OD0000_IDS_ERR_STORAGE
"No storage for %s"

F_OD0001_IDS_BIND_ARRAY
"bind column array"

F_OD0002_IDS_CACHE_BUFFER
"cache buffer"

F_OD0003_IDS_CACHE_SQLDA
"cache SQLDA"

F_OD0004_IDS_CONNECTION
"connection"

F_OD0005_IDS_DSNLIST
"DSN list"

F_OD0006_IDS_FETCH_BUFFER
"fetch buffer"

F_OD0007_IDS_FETCH_SQLDA
"fetch SQLDA"

F_OD0008_IDS_PARM_BUFFER         
"parameter buffer"

F_OD0009_IDS_PARM_SQLDA          
"parameter SQLDA"

F_OD0010_IDS_PARSE_BUFFER        
"internal parse buffer"

F_OD0011_IDS_RESULTS_ARRAY       
"SQLMoreResults array"

F_OD0012_IDS_SQL_QUERY           
"internal SQL query"

F_OD0013_IDS_SQL_SAVE            
"SQL syntax save area"

F_OD0014_IDS_STATEMENT           
"statement"

F_OD0015_IDS_WORKAREA            
"work area"

F_OD0016_IDS_MSG_COMMITTED       
"Transaction committed"

F_OD0017_IDS_MSG_MAXLENGTH       
"SQL_MAX_LENGTH set to data source maximum"

F_OD0018_IDS_MSG_TRACE           
"Trace option set"

F_OD0019_IDS_DRIVERCONNECT       
"%s DriverConnect"

F_OD0020_IDS_DRV_DATABASE        
"Database name is required"

F_OD0021_IDS_DRV_DATASOURCE      
"Data source name is required"

F_OD0022_IDS_DRV_DICTIONARY      
"Dictionary name is required"

F_OD0023_IDS_DRV_NODE            
"Node name is required"

F_OD0024_IDS_DRV_TASK            
"Task code is required"

F_OD0025_IDS_DRV_TIMEOUT         
"Timeout must be from 0 to 255"

F_OD0026_IDS_DRV_USERID          
"User id is required"

F_OD0027_IDS_ERR_ARGUMENT        
"Invalid argument value"

F_OD0028_IDS_ERR_ASSIGNMENT      
"Error in assignment"

F_OD0029_IDS_ERR_CANCELED        
"Operation canceled"

F_OD0030_IDS_ERR_CANCEL_FREE     
"Cancel treated as FreeStmt/Close"

F_OD0031_IDS_ERR_COLUMN          
"Invalid column number"

F_OD0032_IDS_ERR_CONSTRAINT      
"Integrity constraint violation"

F_OD0033_IDS_ERR_CURSOR_DUP      
"Duplicate cursor name"

F_OD0034_IDS_ERR_CURSOR_INVALI  
"Invalid cursor name"

F_OD0035_IDS_ERR_CURSOR_NONE     
"No cursor name available"

F_OD0036_IDS_ERR_CURSOR_STATE    
"Invalid cursor state"

F_OD0037_IDS_ERR_DATE_OVERFLOW   
"Datetime field overflow"

F_OD0038_IDS_ERR_DESCRIBE        
"Too many columns to DESCRIBE"

F_OD0039_IDS_ERR_DESCRIPTOR      
"Descriptor type out of range"

F_OD0040_IDS_ERR_DRIVER          
"Unsupported driver name"

F_OD0041_IDS_ERR_ESCAPE          
"Invalid or unsupported ODBC escape sequence"

F_OD0042_IDS_ERR_EXEC_DATA       
"No data at execution values pending"

F_OD0043_IDS_ERR_EXTENSION       
"Invalid use of Ingres API extension"

F_OD0044_IDS_ERR_FETCH           
"Too many columns to FETCH"

F_OD0045_IDS_ERR_INFO_TYPE       
"Information type out of range"

F_OD0046_IDS_ERR_INVALID         
"Invalid internal SQLSTATE index %d"

F_OD0047_IDS_ERR_INVALID_NOW     
"Operation invalid at this time"

F_OD0048_IDS_ERR_LENGTH          
"Invalid string or buffer length"

F_OD0049_IDS_ERR_MISMATCH        
"String data, length mismatch"

F_OD0050_IDS_ERR_NOINDICATOR     
"Indicator variable required but not supplied"

F_OD0051_IDS_ERR_NOMATCH         
"No matching row"

F_OD0052_IDS_ERR_NOMESSAGE       
"DB%6.6ld Error (no message text from server)"

F_OD0053_IDS_ERR_NOPROMPT        
"Insufficient connect information and dialog prohibited"

F_OD0054_IDS_ERR_NOT_CAPABLE     
"Driver not capable"

F_OD0055_IDS_ERR_NUMERIC         
"Invalid NUMERIC or DECIMAL data in column %s"

F_OD0056_IDS_ERR_OPTION          
"Option type out of range"

F_OD0057_IDS_ERR_OPTION_CHANGE  
"Option value changed"

F_OD0058_IDS_ERR_PARMS           
"Too many parameters"

F_OD0059_IDS_ERR_PARM_LIST       
"Wrong number of parameters"

F_OD0060_IDS_ERR_PARM_TYPE       
"Invalid parameter type"

F_OD0061_IDS_ERR_PRECISION       
"Invalid precision value"

F_OD0062_IDS_ERR_RANGE           
"Numeric value out of range"

F_OD0063_IDS_ERR_RESOURCE        
"Unable to load user define resource id = %d type = %d"

F_OD0064_IDS_ERR_SCALE           
"Invalid scale value"

F_OD0065_IDS_ERR_SECTION         
"Cannot assign SECTION number"

F_OD0066_IDS_ERR_SEQUENCE        
"Function sequence error"

F_OD0067_IDS_ERR_STATE           
"Invalid transaction state"

F_OD0068_IDS_ERR_SYNTAX          
"Syntax error token = %s"

F_OD0069_IDS_ERR_TRACE           
"Trace option invalid or not specified"

F_OD0070_IDS_ERR_TRANSDLL        
"Unable to load translation DLL %s rc = %d"

F_OD0071_IDS_ERR_TRUNC_STRING    
"String data right truncation"

F_OD0072_IDS_ERR_TRUNCATION      
"Data truncation"

F_OD0073_IDS_ERR_TYPE_ATTRIBUT  
"Restricted data type attribute violation"

F_OD0074_IDS_ERR_TYPE_INVALID    
"SQL data type out of range"

F_OD0075_IDS_ERR_TYPE_NONE       
"Unsupported server or OpenSQL data type"

F_OD0076_IDS_ERR_UNCOMMITTED     
"Isolation mode of READ UNCOMMITTED requires READ ONLY"

F_OD0077_IDS_ERR_NOT_SUPPORTED   
"Driver does not support this function"

F_OD0078_IDS_ERR_DS_NOT_SUPPORT  
"Data source does not support this statement"

F_OD0079_IDS_ERR_DT_INTERVALS    
"Date/Time intervals are not supported"

F_OD0080_IDS_DLG_DATABASE
"&Database:"

F_OD0081_IDS_DLG_DATASOURCE
"&Data Source:"

F_OD0082_IDS_DLG_USERID
"&User Id:"

F_OD0083_IDS_DLG_PASSWORD
"&Password:"

F_OD0084_IDS_DLG_ROLENAME
"&Role Name:"

F_OD0085_IDS_DLG_ROLEPWD
"Role P&WD:"

F_OD0086_IDS_DLG_OK
"OK"

F_OD0087_IDS_DLG_CANCEL
"Cancel"

F_OD0088_IDS_DLG_HELP
"&Help"

F_OD0089_IDS_DLG_SERVERNAME
"&ServerName:"

F_OD0090_IDS_DLG_SERVERTYPE
"Server&Type:"

F_OD0091_IDS_DLG_NODENAME
"&Node Name:"

F_OD0092_IDS_DLG_TIMEOUT
"T&imeout:"

F_OD0093_IDS_DLG_TASKCODE
"&Task Code:"

F_OD0094_IDS_DRVRCONNECT_ERROR
"SQLDriverConnect Error"

F_OD0095_IDS_ERR_THREAD_NDX
"Error %d allocating Thread Local Storage index"

F_OD0096_IDS_ERR_THREAD
"Error %d allocating Thread Local Storage"

F_OD0097_IDS_ERR_THREAD_ADDR
"Error %d setting Thread Local Storage address"

F_OD0098_IDS_ERR_GETVERSIONEX
"GetVersionEx failed, contact Ingres Corporation technical support"

F_OD0099_IDS_LOAD_ERROR_TITLE
"Ingres Load Error"

F_OD0100_IDS_LOAD_ERROR_TEXT
"Attempted load of old version of the API library.\nODBC driver is not installed correctly.\nRecheck release notes, reinstall ODBC driver\nand specify correct Ingres directory." 

F_OD0101_IDS_LOADLIB_ERROR
"\nLoadLibrary error code = %d"

F_OD0102_IDS_LOADLIB_NOT_VALID
" (DLL is not a valid 32-bit DLL)"

F_OD0103_IDS_LOADLIB_NOT_FOUND
" (DLL could not be found in the path)"

F_OD0104_OIAPI_NOT_LOADED
"The API library could not be loaded."

F_OD0105_OICLFNT_NOT_LOADED
"The compatibility library could not be loaded."

F_OD0106_OICLFDATA_NOT_LOADED
"The compatability data DLL could not be loaded."

F_OD0107_OPING_ODBC_DRIVER
"[Ingres][%s ODBC Driver]"

F_OD0108_OPING_ODBC_DRIVER1
"[Ingres][%s ODBC Driver][%s]"

F_OD0109_INVALID_ADDRESS
"ErrState: invalid ENV, DBC, or STMT address"

F_OD0110_OPING_DRIVER_ERROR
"Ingres Driver Error"

F_OD0111_INVLID_LIT_PARM
"Invalid literal parameter"

F_OD0112_INVLID_COMMAND
"Invalid command"

F_OD0113_IDS_TAB_CAPTION
"%s ODBC Administrator"

F_OD0114_IDS_TAB_DATASOURCE
"Data Source"

F_OD0115_IDS_TAB_ABOUT
"About"

F_OD0116_IDS_TAB_ADVANCED
"Advanced"

F_OD0117_IDS_ERR_TITLE
" Administrator Error"

F_OD0118_IDS_ERR_DSN
"Invalid data source name"

F_OD0119_CONFIG_ABOUT
"About"

F_OD0120_IDC_DS_ADMIN      
"Data Source Administrator"

F_OD0121_IDC_COPYRIGHT           
"Copyright © 1993,1998"

F_OD0122_IDC_CA          
"Ingres Corporation"

F_OD0123_IDS_MSG_REQUIRED        
"Error: %s is required."

F_OD0124_IDS_MSG_CONFIRM         
"Are you sure you want to delete the %s %s?"

F_OD0125_IDS_MSG_DSN             
"data source"

F_OD0126_IDS_MSG_KEEP            
"Data source exists, keep current settings?"

F_OD0127_IDS_MSG_SERVER          
"Vnode"

F_OD0128_IDS_MSG_TITLE           
" Administrator"

F_OD0129_IDS_MSG_SELECT          
"Nothing selected."

F_OD0130_IDS_MSG_DBNAME          
"Database Name"

F_OD0131_IDS_MSG_SERVERTYPE      
"Server type"

F_OD0132_IDS_MSG_CONNECTOK       
"Successful connection"

F_OD0133_IDS_MSG_CONNECTFAILED   
"Connection failed."

F_OD0134_CONFIG_ADVANCED
"Advanced"

F_OD0135_IDC_SELECT_CURSOR
"Select Loops versus Cursors"

F_OD0136_IDC_SELECT_LOOPS
"Select Loops (best performance but limited to one active result set per connection)"

F_OD0137_IDC_CURSORS
"Cursors (best application interoperability; see help for additional information)"

F_OD0138_CONFIG_DATASOURCE
"Data Source"

F_OD0139_IDC_DS
"&Data Source:"

F_OD0140_IDC_DESCRIPTION_TEXT
"D&escription"

F_OD0141_IDC_SERVER
"Server "

F_OD0142_IDC_VNODE_TEXT
"&Vnode"

F_OD0143_IDC_DATABASE
"D&atabase"

F_OD0144_IDC_TYPE
"T&ype"

F_OD0145_IDC_CONN_OPTS
"Connect Options"

F_OD0146_IDC_PROMPTUIDPWD
"Prompt User ID and Password To Override VNODE Login"

F_OD0147_IDC_GATEWAY_OPTION_TE                 
"Enterprise Access (G&ateway) WITH Options"

F_OD0148_IDC_ROLENAME_TEXT            
"&Role Name"

F_OD0149_IDC_ROLEPWD_TEXT
"Role &Password"

F_OD0150_IDC_TEST
"&Test"

F_OD0151_IDS_ERR_NOT_READ_ONLY
"Updating operations are disallowed by the Read Only driver"

F_OD0152_IDS_DLG_GROUP
"&Group:"

F_OD0153_IDS_ERR_BAD_DESC
"Inconsistent descriptor information"

F_OD0154_IDS_ERR_FETCH_TYPE
"Fetch type out of range"

F_OD0155_IDS_ERR_DESC_INDEX
"Invalid descriptor index"

F_OD0156_IDS_ERR_DATA_SOURCE
"Data Source not found and no default driver specified"

F_OD0157_IDS_ERR_RETV_CODE
"Invalid retrieval code"

F_OD0158_IDS_ERR_MEM_ALLOC
"Memory allocation error"

F_OD0159_IDS_ERR_MEM_MGT
"Memory management error"

F_OD0160_IDS_ERR_INVALID_HWND
"Invalid window handle"

F_OD0161_IDS_ERR_REQUEST_TYPE
"Invalid type of request"

F_OD0162_IDS_ERR_INVALID_NAME
"Invalid driver or translator name"

F_OD0163_IDS_ERR_KEYWORD
"Invalid keyword-value pairs"

F_OD0164_IDS_ERR_REQ_FAILED
"Request failed" 

F_OD0165_IDS_ERR_LOAD_LIB
"Could not load the driver or translator library" 

F_OD0166_IDS_ERR_OUT_OF_MEM
"Out of memory"

F_OD0167_IDS_ERR_FRAC_TRUNC
"Fractional part of numeric data truncated"

F_OD0168_IDS_ERR_SCRL_PROC
"Scrollable cursors are not supported with database procedures"

F_OD0169_IDS_ERR_PREP_UCURS
"Updatable cursor queries cannot be prepared"

F_OD0170_IDS_ERR_ROW_RANGE
"Row value out of range"

F_OD0171_IDS_ERR_NULL_PTR
"Invalid use of null pointer"

F_OD0172_IDS_ERR_CORRUPT_SECT
"Corrupted section name in ODBC configuration file"
