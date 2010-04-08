/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : msghandl.h
//
//    Definition of constants and prototypes for message handler
//
**  27-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**  28-Jun-2001 (schph01)
**     (SIR 105158) add new prototypes for message.
**  8-Oct-2001 (schph01)
**     (SIR 105881) add new define for messages.
**  10-Dec-2001 (noifr01)
**     (sir 99596) removal of obsolete code and resources
**  28-Aug-2002 (schph01)
**     (BUG 107249) add new define for message.
**  14-Jan-2003 (schph01)
**     (SIR 109430) add new define for messages.
**  26-Mar-2003 (noifr01)
**   (sir 107523) management of sequences
**  17-May-2004 (schph01)
**     (SIR 112514) Add new define for message.
**	01-June-2005 (lakvi01)
**	   (BUG 114599) Added IDS_E_RMCMD_PRIVILEGES.
**  09-Mar-2010 (drivi01)
**     SIR 123397
**     Add function ErrorMessage2 which does the same thing
**     as ErrorMessage except takes an error in a form of a
**     string instead of an integer.
********************************************************************/

#ifndef    MSGHANDL_HEADER
#define    MSGHANDL_HEADER


/*
// IDS_W_  (Warning)       range [40 001, 45 000]
// IDS_C_  (Confirmed)     range [45 001, 50 000]
// IDS_E_  (Error)         range [50 001, 55 000]
// IDS_I_  (Information)   range [55 001, 60 000]
*/




// 
// Constants for warning
//

#define IDS_W_CANCEL_NULL_DATA                          40001
#define IDS_W_REPL_NOT_INSTALL                          40002
#define IDS_W_SCHEMA_WILL_CHANGE                        40003

// 
// Constants for confirmation
//

#define IDS_C_DROP_USER                                 45001
#define IDS_C_DROP_LOCATION                             45002
#define IDS_C_DROP_PROCEDURE                            45003
#define IDS_C_DROP_GROUP                                45004
#define IDS_C_DROP_RULE                                 45005
#define IDS_C_DROP_ROLE                                 45006
#define IDS_C_DROP_SECURITY                             45007
#define IDS_C_DROP_SYNONYM                              45008
#define IDS_C_DROP_VIEW                                 45009
#define IDS_C_DROP_DBEVENT                              45010
#define IDS_C_DROP_INTEGRITY                            45011
#define IDS_C_DROP_INDEX                                45012
#define IDS_C_DROP_TABLE                                45013
#define IDS_C_DROP_SECURITY_ALARM                       45014
#define IDS_C_DELETE_VNODE                              45015
#define IDS_C_SAVE_CHANGE                               45016
#define IDS_C_OVERWRITE_CHANGE                          45017

#define IDS_C_APPLY_TO_ALL_SETTING                      45020
#define IDS_C_ALTER_RESTART_LASTCHANGES                 45021
#define IDS_C_ALTER_CONFIRM_OVERWRITE                   45022
#define IDS_C_ALTER_CONFIRM_CREATE                      45023
#define IDS_C_DROP_USERFROM_GROUP                       45024
#define IDS_C_DROP_OBJECT                               45025
#define IDS_C_DROP_DATABASE                             45026
#define IDS_C_POPULATE_NEWTABLE                         45027   
#define IDS_C_BUILD_SERVER                              45028   
#define IDS_C_DEREPLICATION                             45029   

#define IDS_C_DELETE_ALL_DATA                           45030   
#define IDS_C_DELETE_AND_POPULATE                       45031   

// 
// Constants for error
//

#define IDS_E_CANNOT_ALLOCATE_MEMORY                    50001
#define IDS_E_CANNOT_CREATE_DIALOG                      50002

#define IDS_E_GRANT_TABLE_FAILED                        50003
#define IDS_E_GRANT_DBEVENT_FAILED                      50004
#define IDS_E_GRANT_PROCEDURE_FAILED                    50005
#define IDS_E_GRANT_DATABASE_FAILED                     50006


#define IDS_E_NOT_A_VALIDE_OBJECT                       50007
#define IDS_E_NOT_A_VALIDE_DATABASE                     50008
#define IDS_E_NOT_A_VALIDE_TABLE                        50009
#define IDS_E_INVALID_PASSWORD                          50010
#define IDS_E_REPLMAIL_FAILED                           50011
#define IDS_E_REPL_CONN_FAILED                          50012
#define IDS_E_GRANT_VIEW_FAILED                         50013
#define IDS_E_PLEASE_RETYPE_PASSWORD                    50014
#define IDS_E_PASSWORD_REQUIRED                         50015
#define IDS_E_OBJECT_NOT_EXIST                          50016
#define IDS_E_SHOULD_SPECIFY_LEN                        50017

#define IDS_E_CREATE_USER_FAILED                        51001
#define IDS_E_CREATE_LOCATION_FAILED                    51002
#define IDS_E_CREATE_PROCEDURE_FAILED                   51003
#define IDS_E_CREATE_GROUP_FAILED                       51004
#define IDS_E_CREATE_RULE_FAILED                        51005
#define IDS_E_CREATE_ROLE_FAILED                        51006
#define IDS_E_CREATE_SECURITY_FAILED                    51007
#define IDS_E_CREATE_SYNONYM_FAILED                     51008
#define IDS_E_CREATE_VIEW_FAILED                        51009
#define IDS_E_CREATE_DBEVENT_FAILED                     51010
#define IDS_E_CREATE_INTEGRITY_FAILED                   51011
#define IDS_E_ADD_USR2GRP_FAILED                        51012
#define IDS_E_CREATE_PROFILE_FAILED                     51013
#define IDS_E_CREATE_TABLE_FAILED                       51014
#define IDS_E_CREATE_INDEX_FAILED                       51015
#define IDS_E_CREATE_CDDS_FAILED                        51016
#define IDS_E_SHOW_PROPERIES_FAILED                     51017
#define IDS_E_CREATE_DATABASE_FAILED                    51018
#define IDS_E_REPL_CANNOT_DELETE_LOCALDB                51019


#define IDS_E_ALTER_USER_FAILED                         52001
#define IDS_E_ALTER_LOCATION_FAILED                     52002
#define IDS_E_ALTER_PROCEDURE_FAILED                    52003
#define IDS_E_ALTER_GROUP_FAILED                        52004
#define IDS_E_ALTER_RULE_FAILED                         52005
#define IDS_E_ALTER_ROLE_FAILED                         52006
#define IDS_E_ALTER_SECURITY_FAILED                     52007
#define IDS_E_ALTER_SYNONYM_FAILED                      52008 
#define IDS_E_ALTER_VIEW_FAILED                         52009
#define IDS_E_ALTER_DBEVENT_FAILED                      52010
#define IDS_E_ALTER_INTEGRITY_FAILED                    52011
#define IDS_E_ALTER_PROFILE_FAILED                      52012
#define IDS_E_ALTER_TABLE_FAILED                        52013
#define IDS_E_ALTER_INDEX_FAILED                        52014
#define IDS_E_ALTER_ACCESS_PROBLEM                      52015
#define IDS_E_ALTER_CDDS_FAILED                         52016
#define IDS_E_ALTER_OBJECT_DENIED                       52017
#define IDS_E_ALTER_REPL_CONNECT_FAILED                 52018

#define IDS_E_DROP_USER_FAILED                          53001
#define IDS_E_DROP_LOCATION_FAILED                      53002
#define IDS_E_DROP_PROCEDURE_FAILED                     53003
#define IDS_E_DROP_GROUP_FAILED                         53004
#define IDS_E_DROP_RULE_FAILED                          53005
#define IDS_E_DROP_ROLE_FAILED                          53006
#define IDS_E_DROP_SECURITY_FAILED                      53007
#define IDS_E_DROP_SYNONYM_FAILED                       53008
#define IDS_E_DROP_VIEW_FAILED                          53009
#define IDS_E_DROP_DBEVENT_FAILED                       53010
#define IDS_E_DROP_INTEGRITY_FAILED                     53011

#define IDS_E_DROP_OBJECT_FAILED                        53012
#define IDS_E_DROP_OBJECT_DENIED                        53013


#define IDS_E_REVOKE_DATABASE_FAILED                    54001
#define IDS_E_REVOKE_TABLE_FAILED                       54002
#define IDS_E_REVOKE_VIEW_FAILED                        54003
#define IDS_E_REVOKE_DBEVENT_FAILED                     54004
#define IDS_E_REVOKE_PROCEDURE_FAILED                   54005
#define IDS_E_REVOKE_ROLE_FAILED                        54006
#define IDS_E_GRANT_ROLE_FAILED                         54007

#define IDS_E_MODIFY_TABLE_STRUCT_FAILED                54008
#define IDS_E_MODIFY_INDEX_STRUCT_FAILED                54009
#define IDS_E_CHANGELOC_TABLE_FAILED                    54010
#define IDS_E_CHANGELOC_INDEX_FAILED                    54011
#define IDS_E_RELOC_TABLE_FAILED                        54012
#define IDS_E_RELOC_INDEX_FAILED                        54013

#define IDS_E_SHRINKBTREE_FAILED                        54020
#define IDS_E_DELETE_ALLDATA_FAILED                     54021
#define IDS_E_ADDPAGES_FAILED                           54022
#define IDS_E_POPULATE_TABLE_FAILED                     54023

#define IDS_E_NUMBER_RANGE                              54024
#define IDS_E_INITIAL_NUMBER                            54025
#define IDS_E_CDDS_EXISTS                               54026

#define IDS_E_DUPLICATE_PATH                            54027
#define IDS_E_LOCAL_TARGET                              54028
#define IDS_E_SOURCE_TARGET                             54029
#define IDS_E_NO_CONNECTION_DEFINED                     54030
#define IDS_E_TOO_MANY_TABLES_SELECTED                  54031
#define IDS_E_TOO_MANYTABLES_OR_DIR                     54032
#define IDS_E_TOO_MANYTABLES_OR_SDIR                    54033
#define IDS_E_TOO_MANYTABLES_OR_DDIR                    54034
#define IDS_E_SYNTAX_ONE_COLUMN                         54035
#define IDS_E_SYNTAX_CLOSEPARENTHESE                    54036
#define IDS_E_SYNTAX_ILLEGAL_CHAR_OR_COL                54037
#define IDS_E_SYNTAX_ILLEGAL_CHAR                       54038
#define IDS_E_CANNOT_USE_THIS                           54039
#define IDS_E_PROPAGATE_FAILED                          54040
#define IDS_E_CLOSING_SESSION                           54041
#define IDS_E_CLOSING_SESSION2                          54042    // ..WINMAIN\RMCMDCLI.SC
#define IDS_E_TOO_MANY_PRODUCTS_SELECTED                54043
#define IDS_E_OBJECT_NOT_FOUND                          54044
#define IDS_E_USER_ID_ERROR                             54045
#define IDS_E_GRANT_SYS_ERROR                           54046
#define IDS_E_REVOKE_SYS_ERROR                          54047

#define IDS_E_ENV_VAR_NOTDEFINED                        54048
#define IDS_E_CANNOT_READ_FILE                          54049
#define IDS_E_TOO_MANY_VNODE                            54050
#define IDS_E_NO_PROTOCOL_DEFINED                       54051

#define IDS_E_TYPEOF_ERROR1                             54052   // ..WINMAIN\DOMDFILE.C
#define IDS_E_OPEN_FOR_SAVE                             54053   // ..WINMAIN\DOMDFILE.C
#define IDS_E_OPEN_FOR_LOAD                             54054   // ..WINMAIN\DOMDFILE.C
#define IDS_E_FILE_SAVE                                 54055   // ..WINMAIN\DOMDFILE.C
#define IDS_E_FILE_LOAD                                 54056   // ..WINMAIN\DOMDFILE.C
#define IDS_E_LAUNCHING_RMCMD                           54057   // ..WINMAIN\RMCMDCLI.SC
#define IDS_E_TYPEOF_ERROR2                             54058
#define IDS_E_CONNECTION_FAILED                         54059
//#define IDS_E_NOT_OPENINGRES1_1                         54060
#define IDS_E_NOT_IMPLEMENT_COLLAPSE                    54061
#define IDS_E_TYPEOF_ERROR                              54062
#define IDS_E_SYSTEM_ERROR                              54063
#define IDS_E_SQL_CRITICAL_SECTION                      54064
#define IDS_E_RUNONLYBY_INGRES                          54065
#define IDS_E_POPULATE_UNSUCCESS                        54066
#define IDS_E_REPL_ALREADY_INSTALLED                    54067
#define IDS_E_REPL_NOT_INSTALLED                        54068
#define IDS_E_QUERYFAILED                               54069
#define IDS_E_CDDS_NOINDEX                              54070
#define IDS_E_COLUMN_NOT_UNREGISTERED                   54072
#define IDS_E_TABLE_IN_OTHER_CDDS                       54073
#define IDS_E_REPL_MOBIL_FAILED                         54074
#define IDS_E_RMCMD_PRIVILEGES                          54075	// ..WINMAIN\RMCMDCLI.SC

//
// V20
#define IDS_E_DUPLICATE_CONSTRAINT1                     54200
#define IDS_E_DUPLICATE_CONSTRAINT2                     54201
#define IDS_E_UNIQUE_MUSTBE_NOTNUL                      54202
#define IDS_E_USESUBDLG_2_DELETECONSTRAINT              54203
#define IDS_E_NO_PKEY_NOR_UKEYS                         54205
#define IDS_E_NO_PKEY                                   54206
#define IDS_E_NO_UKEYS                                  54207
#define IDS_E_FKEY_MUST_MATCH_PKEY_UKEYS                54208
#define IDS_E_CONSTRAINT_NAME_EXIST                     54209
#define IDS_E_FAILED_TOGETPKEY                          54210
#define IDS_E_FAILED_TOGETUKEYS                         54211
#define IDS_E_MUST_CHOOSE_PARENT_TABLE                  54212
#define IDS_E_MUST_HAVE_COLUMNS                         54213
#define IDS_E_MUST_HAVE_EXPRESSION                      54214
#define IDS_E_CANNOT_FETCH_ROWS                         54215
#define IDS_E_NULLABLEANDUNIQUE                         54216
#define IDS_E_INVALID_CDDS_NO                           54217
#define IDS_E_TABLE_WITH_UDT_COLUMN_TYPE                54218
#define IDS_W_TBL_NULL_KEYS                             54219
#define IDS_E_TABLE_EVALUATE_COLUMNS                    54220
#define IDS_W_ENTER_TABLE_NAME                          54221
#define IDS_W_CHOICE_SECURITY_AUDIT                     54222
#define IDS_W_CHANGE_PAGESIZE_NEEDED                    54223
#define IDS_W_CHANGE_ROWSIZE_EXCEDED                    54224
#define IDS_E_MODIFY_PAGE_SIZE_FAILED                   54225
#define IDS_PAGESIZE_SUCESS                             54226
#define IDS_CHANGE_DBMS_CONFIG                          54227
#define IDS_E_NULLABLE_MANDATORY                        54228
#define IDS_ERR_ONLY_WITH_TWO_KEY_COL                   54229
#define IDS_ERR_TBL_ONLY_WITH_TWO_KEY_COL               54230
#define IDS_W_CALCULATION_WITH_DEFAULT                  54231
#define IDS_APPLY_DEFAULT_FILLFACTOR_DATA               54232
#define IDS_E_REVOKE_SEQUENCE_FAILED                    54233
#define IDS_E_TABLE_WITH_UNKNOWN_COLUMN_TYPE            54234
#define IDS_E_ALTER_TABLE_CANNOT_ALTER_COLUMN           54235

#define IDS_E_UNKNOWN_ERROR                             55000





//
// Constants for information
//

#define IDS_I_RG_on_DG                                  55001
#define IDS_I_TIMEOUT                                   55002
#define IDS_I_NOGRANT                                   55003
#define IDS_I_OBJECT_ALREADY_EXIST                      55004

#define IDS_I_SECONDS                                   55005
#define IDS_I_MINUTES                                   55006
#define IDS_I_HOURS                                     55007
#define IDS_I_DAYS                                      55008
#define IDS_I_WEEKS                                     55009
#define IDS_I_MONTHS                                    55010
#define IDS_I_YEARS                                     55011

#define IDS_I_DATABASE                                  55012
#define IDS_I_PROFILE                                   55013   // (IDS_I_DATABASE+1)    
#define IDS_I_USER                                      55014   // (IDS_I_DATABASE+2)    
#define IDS_I_GROUP                                     55015   // (IDS_I_DATABASE+3)    
#define IDS_I_GROUPUSER                                 55016   // (IDS_I_DATABASE+4)    
#define IDS_I_ROLE                                      55017   // (IDS_I_DATABASE+5)    
#define IDS_I_LOCATION                                  55018   // (IDS_I_DATABASE+6)    
#define IDS_I_TABLE                                     55019   // (IDS_I_DATABASE+7)    
#define IDS_I_TABLELOCATION                             55020   // (IDS_I_DATABASE+8)    
#define IDS_I_VIEW                                      55021   // (IDS_I_DATABASE+9)    
#define IDS_I_VIEWTABLE                                 55022   // (IDS_I_DATABASE+10)   
#define IDS_I_INDEX                                     55023   // (IDS_I_DATABASE+11)   
#define IDS_I_INTEGRITY                                 55024   // (IDS_I_DATABASE+12)   
#define IDS_I_PROCEDURE                                 55025   // (IDS_I_DATABASE+13)   
#define IDS_I_RULE                                      55026   // (IDS_I_DATABASE+14)   
#define IDS_I_SCHEMAUSER                                55027   // (IDS_I_DATABASE+15)   
#define IDS_I_SYNONYM                                   55028   // (IDS_I_DATABASE+16)   
#define IDS_I_DBEVENT                                   55029   // (IDS_I_DATABASE+17)   
#define IDS_I_SECURITYALARM                             55030   // (IDS_I_DATABASE+18)   
#define IDS_I_GRANTEE                                   55031   // (IDS_I_DATABASE+19)   
#define IDS_I_DBGRANTEE                                 55032   // (IDS_I_DATABASE+20)   
#define IDS_I_GRANT                                     55033   // (IDS_I_DATABASE+21)   
#define IDS_I_COLUMN                                    55034   // (IDS_I_DATABASE+22)   
#define IDS_I_REPLIC_CONNECTION                         55035   // (IDS_I_DATABASE+23)   
#define IDS_I_ROLEGRANTEE                               55036   // (IDS_I_DATABASE+24)   
#define IDS_I_CDDS                                      55037   // (IDS_I_DATABASE+25)   
#define IDS_I_REGTABLE                                  55038   // (IDS_I_DATABASE+26)   
#define IDS_I_MAILUSER                                  55039   // (IDS_I_DATABASE+27)   
#define IDS_I_UNKNOWN                                   55041   // (IDS_I_DATABASE+29)   
#define IDS_I_ALLOBJECT                                 55042   // (IDS_I_DATABASE+30)   
#define IDS_I_VIEWS                                     55043   // (IDS_I_DATABASE+31)   
// Added Emb Sep. 13, 95 for use in dom2.c
#define IDS_I_REPLIC_CONN                               55044   // (IDS_I_DATABASE+32)
#define IDS_I_REPLIC_MAILUSR                            55045   // (IDS_I_DATABASE+33)
#define IDS_I_REPLIC_CDDS                               55046   // (IDS_I_DATABASE+34)

#define IDS_I_ACCESS_PROBLEM     55050
#define IDS_I_OBJECT_DELETED     55051
#define IDS_I_OBJECT_WAS_CHANGED 55052
#define IDS_I_UPTO10DATABASES    55053
#define IDS_I_LOCATE_ALL         55054
#define IDS_I_NOGROUP            55055
#define IDS_I_NOPROFILE          55056
#define IDS_I_ASCENDING          55057
#define IDS_I_DESCENDING         55058
#define IDS_I_NODBEVENT          55059

#define IDS_I_NAME               55060
#define IDS_I_LOCAL              55061
#define IDS_I_ORIGINAL           55062
#define IDS_I_TARGET             55063
#define IDS_I_SOURCE             55064
#define IDS_I_REP                55065
#define IDS_I_REP_KEY            55066
#define IDS_I_DESTROYDB          55067

#define IDS_I_CONFIRM_DROP       56000
#define IDS_I_DROP_SECURITY_ON   56001
#define IDS_I_DROP_SECURITY_ONDB 56002
#define IDS_I_DROP_SECURITY      56003
#define IDS_I_DROP_NOTDEFINED    56004
#define IDS_I_CREATE_NOTDEFINED  56005
#define IDS_I_ALTER_NOTDEFINED   56006

#define IDS_I_REFRESH_INPROGRESS 56007
#define IDS_I_REFRESH_WHOLE_NODE 56008
#define IDS_I_REFRESH_1          56009
#define IDS_I_REFRESH_2          56010
#define IDS_I_REFRESH_3          56011
#define IDS_I_MOUSE_COORDINATES  56012
#define IDS_I_PROPAGATE_REPOBJ   56013
#define IDS_I_INSTALL_REPLOBJ    56014
#define IDS_I_PASTE_OBJECT       56015
#define IDS_I_NODATA_READ        56016
#define IDS_I_BUILD_SERVER       56017
#define IDS_I_BUILD_SERVER_ON    56018
#define IDS_I_REMOVE_REPLICATION 56019
#define IDS_I_REMOVE_REPL_FOR    56020
#define IDS_I_QUERYSUCCESS       56021  

#define IDS_I_PROPERTY           56022
#define IDS_I_VALUE              56023
#define IDS_I_USAGE_TYPE         56024
#define IDS_I_PRIVILEGE          56025
#define IDS_I_DEFAULT_PRIVILEGE  56026
#define IDS_I_SECURITY_AUDIT     56027
#define IDS_I_USERS_IN_GROUP     56028
#define IDS_I_LOCATIONS          56029
#define IDS_I_NEW_LOCATIONS      56030
#define IDS_I_OLD_LOCATIONS      56031
#define IDS_I_INDEX_COLUMNS      56032
#define IDS_I_TYPE               56033
#define IDS_I_RANK_PRIM_KEY      56034
#define IDS_I_DEFAULT_VALUE      56035
#define IDS_I_REFERENCE          56036
#define IDS_I_PRIMARY_KEY        56037
#define IDS_I_SEARCH_COND        56038
#define IDS_I_TABLE_COND         56039
#define IDS_I_SELECT_STATEMENT   56040
#define IDS_I_LOCALNODE          56041

#define IDS_I_USER_I             56042
#define IDS_I_PROFILE_I          56043
#define IDS_I_GROUP_I            56046
#define IDS_I_GROUPUSER_I        56047
#define IDS_I_ROLE_I             56048
#define IDS_I_LOCATION_I         56049
#define IDS_I_TABLE_LOCATION_I   56050
#define IDS_I_INTEGRITY_I        56051
#define IDS_I_RULE_TRIGGER       56052
#define IDS_I_SCHEMA_I           56053
#define IDS_I_MONITOR_DATA_I     56054
#define IDS_I_NODE_DEFS          56055
#define IDS_I_EVENT_DBEVENT      56056
#define IDS_I_DBEVENT_I          56057
#define IDS_I_ICE_OBJS           56058
#define IDS_I_SECALARM_I         56059
#define IDS_I_ROLE_GRANTEES      56060
#define IDS_I_REPLIC_OBJECTS_I   56061

// New for Star Remove (related with "Register as link")
#define IDS_I_CONFIRM_REMOVE       56062
#define IDS_C_REMOVE_OBJECT        56063

#define IDS_TRIGGER					56064
#define IDS_I_NO_ROWS_WITH_UNLOADDB 56065
// Sequence
#define IDS_I_SEQUENCE              56066

/*
// 57000 -60000
// Reserve for wizard 
*/


//
// FOR GRANT AND REVOKE
//

#define IDS_I_G_ACCESS                 46345  // TEMPORARY FOR MAINMFC !

#define IDS_I_G_CREATE_PROCEDURE       57346
#define IDS_I_G_CREATE_TABLE           57347
#define IDS_I_G_CONNECT_TIME_LIMIT     57348  
#define IDS_I_G_DATABASE_ADMIN         57349
#define IDS_I_G_LOCKMOD                57350
#define IDS_I_G_QUERY_IO_LIMIT         57351
#define IDS_I_G_QUERY_ROW_LIMIT        57352
#define IDS_I_G_SELECT_SYSCAT          57353
#define IDS_I_G_SESSION_PRIORITY       57354
#define IDS_I_G_UPDATE_SYSCAT          57355
#define IDS_I_G_TABLE_STATISTICS       57356
#define IDS_I_G_IDLE_TIME_LIMIT        57357

#define IDS_I_G_NO_ACCESS              57358
#define IDS_I_G_NO_CREATE_PROCEDURE    57359
#define IDS_I_G_NO_CREATE_TABLE        57360
#define IDS_I_G_NO_CONNECT_TIME_LIMIT  57361
#define IDS_I_G_NO_DATABASE_ADMIN      57362
#define IDS_I_G_NO_LOCKMOD             57363
#define IDS_I_G_NO_QUERY_IO_LIMIT      57364
#define IDS_I_G_NO_QUERY_ROW_LIMIT     57365
#define IDS_I_G_NO_SELECT_SYSCAT       57366
#define IDS_I_G_NO_SESSION_PRIORITY    57367
#define IDS_I_G_NO_UPDATE_SYSCAT       57368
#define IDS_I_G_NO_TABLE_STATISTICS    57369
#define IDS_I_G_NO_IDLE_TIME_LIMIT     57370

#define IDS_I_G_SELECT                 57371
#define IDS_I_G_INSERT                 57372
#define IDS_I_G_UPDATE                 57373
#define IDS_I_G_DELETE                 57374
#define IDS_I_G_COPY_INTO              57375
#define IDS_I_G_COPY_FROM              57376
#define IDS_I_G_REFERENCE              57377
#define IDS_I_G_RAISE                  57378
#define IDS_I_G_REGISTER               57379
#define IDS_I_G_EXECUTE                57380
                                  
#define IDS_I_G_QUERY_CPU_LIMIT        57381
#define IDS_I_G_QUERY_PAGE_LIMIT       57382
#define IDS_I_G_QUERY_COST_LIMIT       57383
#define IDS_I_G_NO_QUERY_CPU_LIMIT     57384
#define IDS_I_G_NO_QUERY_PAGE_LIMIT    57385
#define IDS_I_G_NO_QUERY_COST_LIMIT    57386
                                   
#define IDS_I_G_CREATE_SEQUENCE        57387
#define IDS_I_G_NO_CREATE_SEQUENCE     57388

void   WarningMessage  (unsigned int message_id);
int    ConfirmedMessage(unsigned int message_id);
int    ConfirmedDrop   (unsigned int message_id, unsigned char* object);
void   ErrorMessage    (unsigned int message_id, int reason);
void   ErrorMessage2    (char* message_id, char* buf);
void   InformedMessage (unsigned int message_id);
void   MsgInvalidObject(unsigned int message_id);

void   ErrorMessagr4Integer (unsigned int message_id, long min, long max);
void   ErrorMessagr4Object  (unsigned int error_type);


#endif

