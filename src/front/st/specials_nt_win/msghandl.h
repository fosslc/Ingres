/********************************************************************
//
//  Copyright (C) 1995 Ingres Corporation
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : msghandl.h
//
//    Defition of constants and prototyps for message handler
//
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

//
// V20
#define IDS_E_DUPLICATE_CONSTRAINT1                     54200
#define IDS_E_DUPLICATE_CONSTRAINT2                     54201
#define IDS_E_UNIQUE_MUSTBE_NOTNUL                      54202
#define IDS_E_USESUBDLG_2_DELETECONSTRAINT              54203
#define IDS_E_PAGESIZE_LESSTHAN_2K                      54204
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
#define IDS_I_REPLIC_SERVER                             55040   // (IDS_I_DATABASE+28)   
#define IDS_I_UNKNOWN                                   55041   // (IDS_I_DATABASE+29)   
#define IDS_I_ALLOBJECT                                 55042   // (IDS_I_DATABASE+30)   
#define IDS_I_VIEWS                                     55043   // (IDS_I_DATABASE+31)   
// Added Emb Sep. 13, 95 for use in dom2.c
#define IDS_I_REPLIC_CONN                               55044   // (IDS_I_DATABASE+32)
#define IDS_I_REPLIC_MAILUSR                            55045   // (IDS_I_DATABASE+33)
#define IDS_I_REPLIC_CDDS                               55046   // (IDS_I_DATABASE+34)

// OpenIngres Desktop
#define IDS_I_DBAREAFILE                                55047
#define IDS_I_DBAREASIZE                                55048
#define IDS_I_DBAREAS_IN_STOGROUP                       55049

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

// OpenIngres Desktop
#define IDS_I_DROP_DBAREA        55998
#define IDS_I_DROP_STOGROUP      55999

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
#define IDS_I_STOGROUP_D         56044
#define IDS_I_DBAREA_D           56045
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

/*
// 57000 -60000
// Reserve for wizard 
*/

#define IDS_I_F_PREDCOMB_AND1    57000
#define IDS_I_F_PREDCOMB_OR1     57001
#define IDS_I_F_PREDCOMB_NOT1    57002
#define IDS_I_F_PREDCOMB_AND2    57003
#define IDS_I_F_PREDCOMB_OR2     57004
#define IDS_I_F_PREDCOMB_NOT2    57005
#define IDS_I_F_PREDCOMB_AND3    57006
#define IDS_I_F_PREDCOMB_OR3     57007
#define IDS_I_F_PREDCOMB_NOT3    57008


#define IDS_I_F_PRED_COMP1       57009
#define IDS_I_F_PRED_COMP2       57010
#define IDS_I_F_PRED_COMP3       57011
#define IDS_I_F_PRED_LIKE1       57012
#define IDS_I_F_PRED_LIKE2       57013
#define IDS_I_F_PRED_LIKE3       57014
#define IDS_I_F_PRED_BETWEEN1    57015
#define IDS_I_F_PRED_BETWEEN2    57016
#define IDS_I_F_PRED_BETWEEN3    57017
#define IDS_I_F_PRED_IN1         57018
#define IDS_I_F_PRED_IN2         57019
#define IDS_I_F_PRED_IN3         57020
#define IDS_I_F_PRED_ANY1        57021
#define IDS_I_F_PRED_ANY2        57022
#define IDS_I_F_PRED_ANY3        57023
#define IDS_I_F_PRED_ALL1        57024
#define IDS_I_F_PRED_ALL2        57025
#define IDS_I_F_PRED_ALL3        57026
#define IDS_I_F_PRED_EXIST1      57027
#define IDS_I_F_PRED_EXIST2      57028
#define IDS_I_F_PRED_EXIST3      57029
#define IDS_I_F_PRED_ISNUL1      57030
#define IDS_I_F_PRED_ISNULL2     57031
#define IDS_I_F_PRED_ISNULL3     57032
                                 

#define IDS_I_F_DB_COLUMNS1      57033
#define IDS_I_F_DB_COLUMNS2      57034
#define IDS_I_F_DB_COLUMNS3      57035
#define IDS_I_F_DB_TABLES1       57036
#define IDS_I_F_DB_TABLES2       57037
#define IDS_I_F_DB_TABLES3       57038
#define IDS_I_F_DB_DATABASES1    57039
#define IDS_I_F_DB_DATABASES2    57040
#define IDS_I_F_DB_DATABASES3    57041
#define IDS_I_F_DB_USERS1        57042
#define IDS_I_F_DB_USERS2        57043
#define IDS_I_F_DB_USERS3        57044
#define IDS_I_F_DB_GROUPS1       57045
#define IDS_I_F_DB_GROUPS2       57046
#define IDS_I_F_DB_GROUPS3       57047
#define IDS_I_F_DB_ROLES1        57048
#define IDS_I_F_DB_ROLES2        57049
#define IDS_I_F_DB_ROLES3        57050
#define IDS_I_F_DB_PROFILES1     57061
#define IDS_I_F_DB_PROFILES2     57062
#define IDS_I_F_DB_PROFILES3     57063
#define IDS_I_F_DB_LOCATIONS1    57064
#define IDS_I_F_DB_LOCATIONS2    57065
#define IDS_I_F_DB_LOCATIONS3    57066


#define IDS_I_F_DT_BYTE1         57067
#define IDS_I_F_DT_BYTE2         57068
#define IDS_I_F_DT_BYTE3         57069
#define IDS_I_F_DT_C1            57070
#define IDS_I_F_DT_C2            57071 
#define IDS_I_F_DT_C3            57072
#define IDS_I_F_DT_CHAR1         57073
#define IDS_I_F_DT_CHAR2         57074
#define IDS_I_F_DT_CHAR3         57075
#define IDS_I_F_DT_DATE1         57076
#define IDS_I_F_DT_DATE2         57077
#define IDS_I_F_DT_DATE3         57078
#define IDS_I_F_DT_DECIMAL1      57079
#define IDS_I_F_DT_DECIMAL2      57080
#define IDS_I_F_DT_DECIMAL3      57081
#define IDS_I_F_DT_DOW1          57082
#define IDS_I_F_DT_DOW2          57083
#define IDS_I_F_DT_DOW3          57084
#define IDS_I_F_DT_FLOAT41       57085
#define IDS_I_F_DT_FLOAT42       57086
#define IDS_I_F_DT_FLOAT43       57087
#define IDS_I_F_DT_FLOAT81       57088
#define IDS_I_F_DT_FLOAT82       57089
#define IDS_I_F_DT_FLOAT83       57090
#define IDS_I_F_DT_HEX1          57091
#define IDS_I_F_DT_HEX2          57092
#define IDS_I_F_DT_HEX3          57093
#define IDS_I_F_DT_INT11         57094
#define IDS_I_F_DT_INT12         57095
#define IDS_I_F_DT_INT13         57096
#define IDS_I_F_DT_INT21         57097
#define IDS_I_F_DT_INT22         57098
#define IDS_I_F_DT_INT23         57099
#define IDS_I_F_DT_INT41         57100
#define IDS_I_F_DT_INT42         57101
#define IDS_I_F_DT_INT43         57102
#define IDS_I_F_DT_LONGBYTE1     57103
#define IDS_I_F_DT_LONGBYTE2     57104
#define IDS_I_F_DT_LONGBYTE3     57105
#define IDS_I_F_DT_LONGVARC1     57106
#define IDS_I_F_DT_LONGVARC2     57107
#define IDS_I_F_DT_LONGVARC3     57108
#define IDS_I_F_DT_MONEY1        57109
#define IDS_I_F_DT_MONEY2        57110
#define IDS_I_F_DT_MONEY3        57111
#define IDS_I_F_DT_OBJKEY1       57112
#define IDS_I_F_DT_OBJKEY2       57113
#define IDS_I_F_DT_OBJKEY3       57114
#define IDS_I_F_DT_TABKEY1       57115
#define IDS_I_F_DT_TABKEY2       57116
#define IDS_I_F_DT_TABKEY3       57117
#define IDS_I_F_DT_TEXT1         57118
#define IDS_I_F_DT_TEXT2         57119
#define IDS_I_F_DT_TEXT3         57120
#define IDS_I_F_DT_VARBYTE1      57121
#define IDS_I_F_DT_VARBYTE2      57122
#define IDS_I_F_DT_VARBYTE3      57123
#define IDS_I_F_DT_VARCHAR1      57124
#define IDS_I_F_DT_VARCHAR2      57125
#define IDS_I_F_DT_VARCHAR3      57126
                                 

#define IDS_I_F_NUM_ABS1         57127  
#define IDS_I_F_NUM_ABS2         57128
#define IDS_I_F_NUM_ABS3         57129
#define IDS_I_F_NUM_ATAN1        57130
#define IDS_I_F_NUM_ATAN2        57131
#define IDS_I_F_NUM_ATAN3        57132
#define IDS_I_F_NUM_COS1         57133
#define IDS_I_F_NUM_COS2         57134
#define IDS_I_F_NUM_COS3         57135
#define IDS_I_F_NUM_EXP1         57136
#define IDS_I_F_NUM_EXP2         57137
#define IDS_I_F_NUM_EXP3         57138
#define IDS_I_F_NUM_LOG1         57139
#define IDS_I_F_NUM_LOG2         57140
#define IDS_I_F_NUM_LOG3         57141
#define IDS_I_F_NUM_MOD1         57142
#define IDS_I_F_NUM_MOD2         57143
#define IDS_I_F_NUM_MOD3         57144
#define IDS_I_F_NUM_SIN1         57145
#define IDS_I_F_NUM_SIN2         57146
#define IDS_I_F_NUM_SIN3         57147
#define IDS_I_F_NUM_SQRT1        57148
#define IDS_I_F_NUM_SQRT2        57149
#define IDS_I_F_NUM_SQRT3        57150


#define IDS_I_F_STR_CHAREXT1     57151
#define IDS_I_F_STR_CHAREXT2     57152
#define IDS_I_F_STR_CHAREXT3     57153
#define IDS_I_F_STR_CONCAT1      57154
#define IDS_I_F_STR_CONCAT2      57155
#define IDS_I_F_STR_CONCAT3      57156
#define IDS_I_F_STR_LEFT1        57157
#define IDS_I_F_STR_LEFT2        57158
#define IDS_I_F_STR_LEFT3        57159
#define IDS_I_F_STR_LENGTH1      57260
#define IDS_I_F_STR_LENGTH2      57261
#define IDS_I_F_STR_LENGTH3      57262
#define IDS_I_F_STR_LOCATE1      57263
#define IDS_I_F_STR_LOCATE2      57264
#define IDS_I_F_STR_LOCATE3      57265
#define IDS_I_F_STR_LOWERCS1     57266
#define IDS_I_F_STR_LOWERCS2     57267
#define IDS_I_F_STR_LOWERCS3     57268
#define IDS_I_F_STR_PAD1         57269
#define IDS_I_F_STR_PAD2         57270
#define IDS_I_F_STR_PAD3         57271
#define IDS_I_F_STR_RIGHT1       57272
#define IDS_I_F_STR_RIGHT2       57273
#define IDS_I_F_STR_RIGHT3       57274
#define IDS_I_F_STR_SHIFT1       57275
#define IDS_I_F_STR_SHIFT2       57276
#define IDS_I_F_STR_SHIFT3       57277
#define IDS_I_F_STR_SIZE1        57278
#define IDS_I_F_STR_SIZE2        57279
#define IDS_I_F_STR_SIZE3        57280
#define IDS_I_F_STR_SQUEEZE1     57281
#define IDS_I_F_STR_SQUEEZE2     57282
#define IDS_I_F_STR_SQUEEZE3     57283
#define IDS_I_F_STR_TRIM1        57284
#define IDS_I_F_STR_TRIM2        57285
#define IDS_I_F_STR_TRIM3        57286
#define IDS_I_F_STR_NOTRIM1      57287
#define IDS_I_F_STR_NOTRIM2      57288
#define IDS_I_F_STR_NOTRIM3      57289
#define IDS_I_F_STR_UPPERCS1     57290
#define IDS_I_F_STR_UPPERCS2     57291
#define IDS_I_F_STR_UPPERCS3     57292
                                 
 
#define IDS_I_F_DAT_DATETRUNC1   57293
#define IDS_I_F_DAT_DATETRUNC2   57294
#define IDS_I_F_DAT_DATETRUNC3   57295
#define IDS_I_F_DAT_DATEPART1    57296
#define IDS_I_F_DAT_DATEPART2    57297
#define IDS_I_F_DAT_DATEPART3    57298
#define IDS_I_F_DAT_GMT1         57299
#define IDS_I_F_DAT_GMT2         57300
#define IDS_I_F_DAT_GMT3         57301
#define IDS_I_F_DAT_INTERVAL1    57302
#define IDS_I_F_DAT_INTERVAL2    57303
#define IDS_I_F_DAT_INTERVAL3    57304
#define IDS_I_F_DAT_DATE1        57305
#define IDS_I_F_DAT_DATE2        57306
#define IDS_I_F_DAT_DATE3        57307
#define IDS_I_F_DAT_TIME1        57308
#define IDS_I_F_DAT_TIME2        57309
#define IDS_I_F_DAT_TIME3        57310


#define IDS_I_F_AGG_ANY1         57311
#define IDS_I_F_AGG_ANY2         57312
#define IDS_I_F_AGG_ANY3         57313
#define IDS_I_F_AGG_COUNT1       57314
#define IDS_I_F_AGG_COUNT2       57315
#define IDS_I_F_AGG_COUNT3       57316
#define IDS_I_F_AGG_SUM1         57317
#define IDS_I_F_AGG_SUM2         57318
#define IDS_I_F_AGG_SUM3         57319
#define IDS_I_F_AGG_AVG1         57320
#define IDS_I_F_AGG_AVG2         57321
#define IDS_I_F_AGG_AVG3         57322
#define IDS_I_F_AGG_MAX1         57323
#define IDS_I_F_AGG_MAX2         57324
#define IDS_I_F_AGG_MAX3         57325
#define IDS_I_F_AGG_MIN1         57326
#define IDS_I_F_AGG_MIN2         57327
#define IDS_I_F_AGG_MIN3         57328
                                 
                                  
#define IDS_I_F_IFN_IFN1         57329
#define IDS_I_F_IFN_IFN2         57330
#define IDS_I_F_IFN_IFN3         57331

#define IDS_I_F_SUBSEL_SUBSEL1   57332
#define IDS_I_F_SUBSEL_SUBSEL2   57333
#define IDS_I_F_SUBSEL_SUBSEL3   57334

#define IDS_I_FF_DTCONVERSION    57335
#define IDS_I_FF_NUMFUNCTIONS    57336
#define IDS_I_FF_STRFUNCTIONS    57337
#define IDS_I_FF_DATEFUNCTIONS   57338
#define IDS_I_FF_AGGRFUNCTIONS   57339
#define IDS_I_FF_IFNULLFUNC      57340
#define IDS_I_FF_PREDICATES      57341
#define IDS_I_FF_PREDICCOMB      57342
#define IDS_I_FF_DATABASEOBJECTS 57343
#ifdef MAINLIB
#define IDS_I_FF_SUBSELECT       46344    // TEMPORARY FOR MAINMFC !
#else
#define IDS_I_FF_SUBSELECT       57344
#endif
                                  
//
// FOR GRANT AND REVOKE
//

#ifdef MAINLIB
#define IDS_I_G_ACCESS                 46345  // TEMPORARY FOR MAINMFC !
#else
#define IDS_I_G_ACCESS                 57345
#endif

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
                                   

void   WarningMessage  (unsigned int message_id);
int    ConfirmedMessage(unsigned int message_id);
int    ConfirmedDrop   (unsigned int message_id, unsigned char* object);
void   ErrorMessage    (unsigned int message_id, int reason);
void   InformedMessage (unsigned int message_id);
void   MsgInvalidObject(unsigned int message_id);

void   ErrorMessagr4Integer (unsigned int message_id, long min, long max);
void   ErrorMessagr4Object  (unsigned int error_type);


#endif

