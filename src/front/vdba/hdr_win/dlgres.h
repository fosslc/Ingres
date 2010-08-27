/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Project  : CA/OpenIngres Visual DBA
**
**    Source   : dlgres.h
**
**    Definition of Dlg box control identifiers
**
** History:
** 27-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 02-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 09-May-2001 (schph01)
**    SIR 102509 remove all IDD_ and IDC_ concerning Create/Alter Location.
**    This dialog is rewrited and defined in Mainmfc.RC
** 30-May-2001 (uk$so01)
**    SIR #104736, Integrate IIA and Cleanup old code of populate.
** 18-Jun-2001 (uk$so01)
**    BUG #104799, Deadlock on modify structure when there exist an opened 
**    cursor on the table.
** 15-Oct-2001 (schph01)
**    additional change for SIR #102678, Add two controls in optimzedb
**    dialog box
** 28-Mar-2002 (noifr01)
**    (sir 99596) removed additional unused resources/sources
** 05-Jun-2002 (noifr01)
**    (bug 107773) removed resources that were used in now removed code
** 26-Mar-2003 (noifr01)
**    (sir 107523) management of sequences
** 09-Apr-2003 (noifr01)
**    (sir 107523) upon cleanup of refresh features for managing sequences,
**    removed obsolete structures and definitions
** 30-Oct-2003 (noifr01)
**    fixed minor propagation error in change 465572 for SIR 99596
** 27-Nov-2003 (schph01)
**    (sir 111389) in auditdb and rollforward dialog replace the spin control
**    increasing the checkpoint number by a button displaying the list of
**    existing checkpoints.
** 17-May-2004 (schph01)
**    (SIR 111507)Add new datatype BigInt
** 01-Jul-2004 (schph01)
**    (SIR #112590) Add define IDC_CHECK_CONCURRENT_UPDATE for new control
**    in modify structure dialog
** 23-Nov-2005 (fanra01)
**    Bug 115560
**    Add identifiers for widgets added to the alterdb dialog.
** 12-Oct-2006 (wridu01)
**    (Sir 116835) Add support for Ansi Date/Time data types.
** 10-Mar-2010 (drivi01) SIR 123397
**    vdba.exe, ingnet.exe, and iia.exe were updated to simplify the tools
**    from Usability standpoint. This file includes defines for additional
**    controls added.
** 19-Mar-2010 (drivi01)
**    Add IDC_MORE_OPTIONS define here if it isn't already defined.  
**    This is b/c resource.h gets included in a few files in mainmfc
**    causing warnings of redefine for the constant.
** 30-Jun-2010 (drivi01)
**    Bug #124006
**    Add new BOOLEAN datatype.
*/

#ifndef DLGRES_HEADER
#define DLGRES_HEADER

//{{NO_DEPENDENCIES}}
// App Studio generated include file.
// hand-modified!!!
//
#define IDC_SOURCE                             3
#define IDC_EDIT                               3
#define IDC_DEST                               4

#define IDR_MENU1                              102

//
// New Dialog Box for VDBA V20
// IDD_Starting from  5080 To 5099 ???  

#define IDD_UNIQUECONSTRAINT            5080
#define IDD_CHECKCONSTRAINT             5081


//#define IDC_TLCLIST1                    1002
//#define IDC_TLCLIST2                    1003
//#define IDC_TLCCOMBO1                   1004
//#define IDC_TLCBUTTON_ADD               1005
//#define IDC_TLCBUTTON_DELETE            1006
//#define IDC_TLCBUTTON_UP                1007
//#define IDC_TLCBUTTON_DOWN              1008
//#define IDC_TLCEDIT1                    1009
//#define IDC_TLCBUTTON_ASSISTANT         1010
//#define IDC_TLCBUTTON_UNIQUE            1011
//#define IDC_TLCBUTTON_CHECK             1012
//#define IDC_COMBOPAGESIZE               1013
//#define IDC_BUTTON_INGASSISTANT         1014
//#define IDC_INGCHECKASSELECT            1015
//#define IDC_STATIC_INGCOLUMNS           1016
//#define IDC_STATIC_INGNAME              1017
//#define IDC_STATIC_INGCOLUMNHEADER      1018
//#define IDC_INGEDITCOLUMNHEADER         1019
//#define IDC_STATIC_INGSELECT            1020
//#define IDC_INGEDITSQL                  1021



/* ----------------------------------------------------------------------------
//                                                                            &
// The identifiers defined for dialog boxe templates                          &
//                                                                            &
// ----------------------------------------------------------------------------
//
//   IDD_DISCONNECT                            103
//   IDD_CREATEDB                              107
//   IDD_SPACECALC                             108
//   IDD_SYSMOD                                110
//   IDD_DBLOCATIONS                           114
//   IDD_SQLTESTPREF                           117
//   IDD_COPYDB                                118
//   IDD_CREATEINDEX                           122
//   IDD_CREATETABLE                           123
//   IDD_TABLESTRUCT                           124
//   IDD_REFERENCES                            125
//   IDD_GROUP                                 157
//   IDD_DBEVENT                               158
//   IDD_SYNONYM                               161
//
//   IDD_SQLWIZARD                             2000
//   IDD_INSERT1                               2001
//   IDD_INSERT2                               2002
//   IDD_DELETE1                               2003
//   IDD_UPDATE1                               2004
//   IDD_UPDATE2                               2005
//   IDD_UPDATE3                               2006
//   IDD_SELECT1                               2007
//   IDD_SELECT2                               2008
//   IDD_SELECT3                               2009
//
//   IDD_VNODE                                 5001
//   IDD_INTEGRITY                             5002
//   IDD_REFRESHSET                            5003
//   IDD_SESSION                               5004
//   IDD_SYNONYM_ONOBJECT                      5005
//   IDD_PROCEDURE                             5006
//   IDD_VIEW                                  5007
//   IDD_GRANT_TABLE                           5009
//   IDD_GRANT_DBEVENT                         5010
//   IDD_GRANT_PROCEDURE                       5011
//   IDD_GRANT_DATABASE                        5012
//   IDD_GNREF_TABLE                           5014
//   IDD_GNREF_DBEVENT                         5015
//   IDD_GNREF_PROCEDURE                       5016
//   IDD_REVOKE_TABLE                          5017
//   IDD_REVOKE_DATABASE                       5019
//   IDD_REFALARM                              5020
//   IDD_REPL_CONN                             5021
//   IDD_MAIL                                  5022
//   IDD_ALTER_USER                            5023
//   IDD_ALTER_GROUP                           5024
//   IDD_ALTERROLE                             5025
//   IDD_USR2GRP                               5026
//   IDD_POPULATE                              5027
//   IDD_AUDITDB                               5028
//   IDD_AUDITDBT                              5029
//   IDD_AUDITDBF                              5030
//   IDD_OPTIMIZEDB                            5031
//   IDD_SPECIFY_COL                           5032
//   IDD_ALTERDB                               5033
//   IDD_VERIFDB                               5034
//   IDD_CHKPOINT                              5035
//   IDD_ROLLFWD                               5036
//   IDD_STATDUMP                              5037
//   IDD_DISPLAY                               5038
//   IDD_RVKGTAB                               5040
//   IDD_LOCATE                                5041
//   IDD_RVKGDB                                5042
//   IDD_PROFILE                               5043
//   IDD_USER                                  5044
//   IDD_GRANTROL                              5045
//   IDD_RVKGROL                               5046
//   IDD_RVKGDBE                               5047
//   IDD_RVKGVIEW                              5048
//   IDD_RVKGPROC                              5049
//   IDD_MODIFSTR                              5050
//   IDD_CHANGEST                              5051
//   IDD_LOCSEL01                              5052
//   IDD_RELOCATE                              5053
//   IDD_RULE                                  5056
//   IDD_ROLE                                  5057
//   IDD_REPL_CDDS                             5058
//   IDD_COPYINOUT                             5060
//   IDD_RELOADUNLOAD                          5061
//   IDD_SECURITY_ALARM                        5062
//   IDD_SECURITY_ALARM2                       5063
//   IDD_RECONCILER                            5064
//   IDD_PROPAGATE                             5070
//   IDD_REPL_CDDS_NEW                         5071
//   IDD_REPL_TYPE_AND_SERVER                  5072 
//   IDD_REPL_CONFSVR                          5073
//   IDD_MOBILE_PARAM                          5074
//
//   IDD_SAVEENV                               5401
//   IDD_SAVEENVPSW                            5402
//   IDD_PROC_SUBADD                           5403
//
//   IDD_ROLLFWD_RELOCATE                      5404
//   IDD_RVKGSEQUENCE                          5405
//   IDD_GRANT_SEQUENCE                        5406
// ----------------------------------------------------------------------------
*/



#define IDD_CREATEDB                           107
#define IDD_SPACECALC                          108
#define IDD_SYSMOD                             110

//
// Some dialog boxes are used for both create and alter with the same identifier.
// Other dialog boxes have no identifier (standard open/save, for example)
// The following identifiers are not used for creating dialog boxes.
// They are used for identifying the context help number
//
// Begin

#define IDD_VNODEDEF                           6001
#define IDD_PRINTBOX                           6002
#define IDD_PREFERENCES_WINDOW                 6003
#define IDD_OPENWORKSPACEBOX                   6004   // openworkspace_box in .hpj
#define IDD_ALTERLOCATION                      6005
#define IDD_ALTERPROFILE                       6006
#define IDD_ADDDECLAREBOX                      6007
#define IDD_GRANT_VIEW                         6008
#define IDD_REVOKE_VIEW                        6009
#define IDD_MODIF_IDX                          6010
#define IDD_RELOC_IDX                          6011
#define IDD_CHANGLOC_IDX                       6012

#define IDD_CREATESYN_TAB                      6016
#define IDD_CREATESYN_VIEW                     6018
#define IDD_CREATESYN_IDX                      6017

#define IDD_GNREF_VIEW                         6019
#define IDD_REVOKE_DBEVENT                     6020
#define IDD_SELECTWINDOW                       6021
#define IDD_CHANGEST_IDX                       6022

// Added Emb Sep 4, 95
#define IDD_CONTAINER_FONT                     5532   // fontprefs_box        in .hpj
#define IDD_BALLOON_PREF                       100    // balloonsprefs_box    in .hpj
#define IDD_SQLWIZD_FUNCTION                   6015   // sqlasst_function_box in .hpj
#define IDD_OPENQUERY                          6013   // openquery_box        in .hpj
#define IDD_SAVEQUERY                          6014   // saveasquery_box      in .hpj

// Added Emb Sep 21, 95
#define IDD_REPL_INST_CONN                     6023   // installlocaldbinformations_box in .hpj

// Added Nov 19, 96 for replicator multiple versions
#define IDD_REPL_CONN_1_05                     6025   // 1.05
#define IDD_REPL_CONN_1_1                      6026   // 1.1
// note : IDD_REPL_CONN used for 1.0

// added 15/01/97 Emb
#define IDD_OIV2_SPACECALC                     6050
#define IDD_OIV2_CHANGEST                      6051
#define IDD_OIV2_TABLESTRUCT                   6052
#define IDD_OIV2_ALTERTABLE                    6053
#define IDD_OIV2_CHANGEST_IDX                  6054

//
// End 
//

//PS
#define IDD_COPYINOUT                          5060
#define IDD_RELOADUNLOAD                       5061
//
// for COPYINOUT
// PS
#define IDC_SPECIFY_TABLE                      22001
#define IDC_CREATE_PRINT                       22002
#define IDC_TABLES_LIST                        22003
#define IDC_DESTINATION_DIR                    22004
#define IDC_SOURCE_DIR                         22005
#define IDC_DIRECTORYNAME                      22006
#define IDC_SOURCENAME                         22007
#define IDC_DESTINATION                        22008
//
// for RELOADUNLOAD
// PS
#define IDC_SOURCE_DIRECTORY                   22009
#define IDC_DESTINATION_DIRECTORY              22010
#define IDC_DIRECTORY_NAME                     22011
#define IDC_CREATE_PRINTABLE                   22012

//
// for dlg reconciler
//
#define IDD_RECONCILER                         5064
#define IDC_DBNUMBER                           22013
#define IDC_CDDSNUMBER                         22014
#define IDC_STARTTIME                          22015
#define IDC_STATICSTARTTIME                    22016
#define IDC_STATICDBNUMBER                     22017
#define IDC_STATICCDDS                         22018

//
// PROPAGATE
//
#define IDD_PROPAGATE                          5070
#define IDC_TARGETDATABASE                     22020
#define IDC_STATICPROPAGATE                    22021

//
// for SYSMOD
//
#define IDC_PRODUCT_ALL                        127
#define IDC_PRODUCT_SPECIFY                    128
#define IDC_TABLE_ALL                          131
#define IDC_TABLE_SPECIFY                      132

//
// For create integrity
//
#define IDD_INTEGRITY                          5002
#define IDC_INTEGRITY_TABLENAME                20267
#define IDC_INTEGRITY_SEARCHCONDITION          20269
#define IDC_INTEGRITY_ASSISTANCE               20270

//
// For Create group
//

#define IDD_GROUP                              157
#define IDC_CG_USERLIST                        10100
#define IDC_CG_DATABASES                       10101
#define IDC_CG_GROUPNAME                       10102

//
// For Create DBevent 
//

#define IDD_DBEVENT                            158
#define IDC_DBEVENT_NAME                       20160

//
// For DISCONNECT
//

#define IDD_DISCONNECT                         103
#define IDC_SVRDIS_SERVERS                     1071
#define IDC_SVRDIS_WINDOWS                     20781

//
// FOR CREATE SECURITY ALARM
//

#define IDD_SECURITY_ALARM                     5062    // Dlg Box number 1
#define IDD_SECURITY_ALARM2                    5063    // Dlg Box number 2
#define IDC_SALARM_NAME                        20023
#define IDC_SALARM_ONTABLE                     20024
#define IDC_SALARM_SUCCESS                     20025
#define IDC_SALARM_FAILURE                     20026
#define IDC_SALARM_SUCCESSFAILURE              20027
#define IDC_SALARM_SELECT                      20028
#define IDC_SALARM_DELETE                      20029
#define IDC_SALARM_INSERT                      20030
#define IDC_SALARM_UPDATE                      20031
#define IDC_SALARM_BYUSER                      20032
#define IDC_SALARM_CONNECT                     20033
#define IDC_SALARM_DISCONNECT                  20034
#define IDC_SALARM_USER                        20035
#define IDC_SALARM_GROUP                       20036
#define IDC_SALARM_ROLE                        20037
#define IDC_SALARM_PUBLIC                      20038
#define IDC_SALARM_DBEVENT                     20040
#define IDC_SALARM_DBEVENT_TEXT                20041
#define IDC_SALARM_DBEVENT_LTEXT               20042
#define IDC_SALARM_DATABASE                    20043
#define IDC_SALARM_TABLE                       20044
#define IDC_SALARM_CURRENTINSTALL              20045
#define IDC_SALARM_STATIC_DB                     170

//
// FOR CREATE SYNONYM
//

#define IDD_SYNONYM                            161
#define IDC_CS_NAME                            20050
#define IDC_CS_TABLE                           20051
#define IDC_CS_INDEX                           20053
#define IDC_CS_OBJECT                          20054
#define IDC_CS_VIEW                            20055

//
// FOR ROLE
//
#define IDD_ROLE                               5057
#define IDC_ROLE_NAME                          21000
#define IDC_ROLE_CREATEDB_U                    21002
#define IDC_ROLE_TRACE_U                       21003
#define IDC_ROLE_SECURITY_U                    21004
#define IDC_ROLE_OPERATOR_U                    21005
#define IDC_ROLE_MAINTAINLOCATION_U            21006
#define IDC_ROLE_AUDITOR_U                     21007
#define IDC_ROLE_MAINTAINAUDIT_U               21008
#define IDC_ROLE_MAINTAINUSER_U                21009
#define IDC_ROLE_NOPRIVILEGES_U                21010
#define IDC_ROLE_CREATEDB_D                    21011
#define IDC_ROLE_TRACE_D                       21012
#define IDC_ROLE_SECURITY_D                    21013
#define IDC_ROLE_OPERATOR_D                    21014
#define IDC_ROLE_MAINTAINLOCATION_D            21015
#define IDC_ROLE_AUDITOR_D                     21016
#define IDC_ROLE_MAINTAINAUDIT_D               21017
#define IDC_ROLE_MAINTAINUSER_D                21018
#define IDC_ROLE_NOPRIVILEGES_D                21019
#define IDC_ROLE_ALLEVENTS                     21020
#define IDC_ROLE_DEFAULTEVENTS                 21021
#define IDC_ROLE_QUERYTEXT                     21022
#define IDC_ROLE_LIMITINGSECURITY              21024
#define IDC_ROLE_OLDPASSWORD                   21025
#define IDC_ROLE_PASSWORD                      21026
#define IDC_ROLE_CONFIRMPASSWORD               21027
#define IDC_ROLE_EXTERNALPASSWORD              21028
#define IDC_ROLE_DATABASE                      21029
#define IDC_ROLE_LNAME                         21030
#define IDC_ROLE_LOLDPASSWORD                  21031
#define IDC_ROLE_LPASSWORD                     21033
#define IDC_ROLE_LCONFIRMPASSWORD              21034

//
// FOR VIEW
// 
// Modified Emb June 9, 95
//

#define IDD_VIEW                               5007
#define IDC_VIEW_NAME                          20033
#define IDC_VIEW_CHECK_OPTION                  20034
#define IDC_VIEW_SELECT_STATEMENT              20035
#define IDC_VIEW_COLUMNS                       20036
#define IDC_VIEW_SELECT_WIZ                    20037

//
// FOR VNODE
//

#define IDD_VNODE                              5001
#define IDC_VN_LISTEN                          20006
#define IDC_VN_VNODE                           20000
#define IDC_VN_USER                            20001
#define IDC_VN_PASSWORD                        20002
#define IDC_VN_CONFIRMPASSWORD                 20003
#define IDC_VN_REMOTE                          20004
#define IDC_VN_PROTOCOL                        20005
#define IDC_VNL_VNODE                          20007
#define IDC_VNL_USER                           20008
#define IDC_VNL_PASSWORD                       20009
#define IDC_VNL_CONFIRMPASSWORD                20010
#define IDC_VNL_REMOTE                         20011
#define IDC_VNL_PROTOCOL                       20012
#define IDC_VNL_LISTEN                         20013
#define IDC_VNL_GROUP                          20014
#define IDC_VN_DELETE                          20015
#define IDC_CHECKDESKTOP                       20016

//
// FOR REFRESH SETTINGS
//

#define IDD_REFRESHSET                         5003
#define IDC_REFRESH_OBJECT_TYPE                20020
#define IDC_REFRESH_FREQUENCY                  20021
#define IDC_REFRESH_FREQUENCYSPIN              20022
#define IDC_REFRESH_UNIT                       20023
#define IDC_REFRESH_SYNC_PARENT                20024
#define IDC_REFRESH_SYNC_OBJECT                20025
#define IDC_REFRESH_LOAD                       20026
#define IDC_REFRESH_IDLOAD                     20027
#define IDC_REFRESH_IDSAVE                     20028
#define IDC_REFRESH_WINDOW                     20029
#define IDC_REFRESH_STATUSBAR                  20030
#define IDC_REFRESH_NONE                       20031
#define IDC_REFRESH_IDALLOBJECT                20032
#define IDC_REFRESH_AO_FREQUENCY               20033
#define IDC_REFRESH_AO_FREQUENCYSPIN           20034
#define IDC_REFRESH_AO_UNIT                    20035
#define IDC_REFRESH_AO_SYNC_PARENT             20036
#define IDC_REFRESH_AO_SYNC_OBJECT             20037
#define IDC_REFRESH_AO_LOAD                    20038
#define IDC_REFRESH_LFREQUENCY                 20039
#define IDC_REFRESH_AOL_FREQUENCY              20040

//
// FOR SESSION SETTING
//
#define IDD_SESSION                            5004
#define IDC_SESSION_SESSIONNUMBER              20026
#define IDC_SESSION_SESSIONSPIN                20027
#define IDC_SESSION_TIMEOUTNUMBER              20028
#define IDC_SESSION_TIMEOUTSPIN                20029

//
// FOR CROSSING SYNONYM
//

#define IDD_SYNONYM_ONOBJECT                   5005
#define IDC_SYNONYM_ONOBJECT_NAME              20028

//
// FOR PROCEDURE
//
// Modified Emb  June 9, 95
//
#define IDD_PROCEDURE                          5006
#define IDC_PROCEDURE_PARAMETER                20029
#define IDC_PROCEDURE_DECLARE                  20030
#define IDC_PROCEDURE_STATEMENT                20031
#define IDC_PROCEDURE_NAME                     20032
#define IDC_ADD_PROCEDURE_PARAMETER            20033
#define IDC_ADD_PROCEDURE_DECLARE              20034
#define IDC_ADD_PROCEDURE_STATEMENT            20035
#define IDC_PROCEDURE_DYNAMIC                  20036   // Desktop specific


//
// GRANT FOR TABLE OR VIEW
//

#define IDD_GRANT_TABLE                        5009
#define IDC_GRANT_TABLE_USER                   20038
#define IDC_GRANT_TABLE_TABLE                  20039
#define IDC_GRANT_TABLE_EXCLUDINGS             20040
#define IDC_GRANT_TABLE_OPTION                 20041
#define IDC_GRANT_TABLE_INSERT                 20042
#define IDC_GRANT_TABLE_SELECT                 20043
#define IDC_GRANT_TABLE_UPDATE                 20044
#define IDC_GRANT_TABLE_DELETE                 20045
#define IDC_GRANT_TABLE_COPYINTO               20046
#define IDC_GRANT_TABLE_COPYFROM               20047
#define IDC_GRANT_TABLEL_TABLE                 20048
#define IDC_GRANT_TABLE_REFERENCE              20049
#define IDC_GRANT_TABLEL_EXCLUDINGS            20050
#define IDC_GRANT_TABLE_GRANTEES               20051
#define IDC_GRANT_TABLE_GROUP                  20052
#define IDC_GRANT_TABLE_ROLE                   20053
#define IDC_GRANT_TABLE_PUBLIC                 20054

//
// GRANT FOR DATABASE EVENT
//

#define IDD_GRANT_DBEVENT                      5010
#define IDC_GRANT_DBEVENT_GRANTEES             20051
#define IDC_GRANT_DBEVENT_DBEVENT              20052
#define IDC_GRANT_DBEVENT_RAISE                20053
#define IDC_GRANT_DBEVENT_REGISTER             20054
#define IDC_GRANT_DBEVENT_OPTION               20055
#define IDC_GRANT_DBEVENT_USER                 20056
#define IDC_GRANT_DBEVENT_GROUP                20057
#define IDC_GRANT_DBEVENT_ROLE                 20058
#define IDC_GRANT_DBEVENT_PUBLIC               20059

//
// GRANT FOR EXECUTING PROCEDURE
//

#define IDD_GRANT_PROCEDURE                    5011
#define IDC_GRANT_PROCEDURE_GRANTEES           20056
#define IDC_GRANT_PROCEDURE_PROCEDURES         20057
#define IDC_GRANT_PROCEDURE_OPTION             20058
#define IDC_GRANT_PROCEDURE_USER               20059
#define IDC_GRANT_PROCEDURE_GROUP              20060
#define IDC_GRANT_PROCEDURE_ROLE               20061
#define IDC_GRANT_PROCEDURE_PUBLIC             20062

//
// GRANT FOR DATABASE
//

#define IDD_GRANT_DATABASE                     5012
#define IDC_GRANT_DATABASE_STATICTEXT_DB       20048
#define IDC_GRANT_DATABASE_DATABASE            20060
#define IDC_GRANT_DATABASE_GRANTEES            20059
#define IDC_GRANT_DATABASE_PRIVILEGES          20061
#define IDC_GRANT_DATABASE_NOPRIVILEGES        20062
#define IDC_GRANT_DATABASE_USER                20063
#define IDC_GRANT_DATABASE_GROUP               20064
#define IDC_GRANT_DATABASE_ROLE                20065
#define IDC_GRANT_DATABASE_PUBLIC              20066
#define IDC_GRANT_DATABASE_IDPRIV              20067
#define IDC_GRANT_DATABASE_QIL                 20068
#define IDC_GRANT_DATABASE_QRL                 20069
#define IDC_GRANT_DATABASE_CTL                 20070
#define IDC_GRANT_DATABASE_SPR                 20071
#define IDC_GRANT_DATABASE_ITL                 20072
#define IDC_GRANT_DATABASE_QIL_VAL             20073
#define IDC_GRANT_DATABASE_QRL_VAL             20074
#define IDC_GRANT_DATABASE_CTL_VAL             20075
#define IDC_GRANT_DATABASE_SPR_VAL             20076
#define IDC_GRANT_DATABASE_ITL_VAL             20077

//
// FOR TABLE GRANT (REFERENCE)
//

#define IDD_GNREF_TABLE                        5014
#define IDC_GNREF_TABLE_USER                   20038
#define IDC_GNREF_TABLE_TABLE                  20039
#define IDC_GNREF_TABLE_EXCLUDINGS             20040
#define IDC_GNREF_TABLE_OPTION                 20041
#define IDC_GNREF_TABLE_INSERT                 20042
#define IDC_GNREF_TABLE_SELECT                 20043
#define IDC_GNREF_TABLE_UPDATE                 20044
#define IDC_GNREF_TABLE_DELETE                 20045
#define IDC_GNREF_TABLE_COPYINTO               20046
#define IDC_GNREF_TABLE_COPYFROM               20047
#define IDC_GNREF_TABLEL_TABLE                 20048
#define IDC_GNREF_TABLE_REFERENCE              20049
#define IDC_GNREF_TABLEL_EXCLUDINGS            20050
#define IDC_GNREF_TABLE_GROUP                  20052
#define IDC_GNREF_TABLE_ROLE                   20053
#define IDC_GNREF_TABLE_DATABASES              20054
#define IDC_GNREF_TABLE_GRANTEES               20051
#define IDC_GNREF_TABLE_PUBLIC                 20055

//
// FOR DATABASE EVENT GRANT (REFERENCE)
//

#define IDD_GNREF_DBEVENT                      5015
#define IDC_GNREF_DBEVENT_DBEVENT              20052
#define IDC_GNREF_DBEVENT_RAISE                20053
#define IDC_GNREF_DBEVENT_REGISTER             20054
#define IDC_GNREF_DBEVENT_OPTION               20055
#define IDC_GNREF_DBEVENT_USER                 20056
#define IDC_GNREF_DBEVENT_GROUP                20057
#define IDC_GNREF_DBEVENT_ROLE                 20058
#define IDC_GNREF_DBEVENT_DATABASES            20059
#define IDC_GNREF_DBEVENT_GRANTEES             20051
#define IDC_GNREF_DBEVENT_PUBLIC               20060

//
// FOR EXECUTE PROCEDURE GRANT (REFERENCE)
//

#define IDD_GNREF_PROCEDURE                    5016
#define IDC_GNREF_PROCEDURE_PROCEDURES         20057
#define IDC_GNREF_PROCEDURE_OPTION             20058
#define IDC_GNREF_PROCEDURE_USER               20059
#define IDC_GNREF_PROCEDURE_GROUP              20060
#define IDC_GNREF_PROCEDURE_ROLE               20061
#define IDC_GNREF_PROCEDURE_DATABASES          20062
#define IDC_GNREF_PROCEDURE_GRANTEES           20056
#define IDC_GNREF_PROCEDURE_PUBLIC             20063


//
// FOR REVOKE PRIVILEGES ON TABLE
//

#define IDD_REVOKE_TABLE                       5017
#define IDC_REVOKE_TABLE_PRIVILEGE             20070
#define IDC_REVOKE_TABLE_GRANT_OPTION          20071
#define IDC_REVOKE_TABLE_CASCADE               20072
#define IDC_REVOKE_TABLE_RESTRICT              20073
#define IDC_REVOKE_TABLE_USER                  20074
#define IDC_REVOKE_TABLE_GROUP                 20075
#define IDC_REVOKE_TABLE_ROLE                  20076
#define IDC_REVOKE_TABLE_PUBLIC                20077
#define IDC_REVOKE_TABLE_GRANTEES              20078
#define IDC_REVOKE_TABLE_LRGDG                 20079

//
// FOR REVOKE PRIVILEGES ON DATABASE
//

#define IDD_REVOKE_DATABASE                    5019
#define IDC_REVOKE_DATABASE_USER               20090
#define IDC_REVOKE_DATABASE_GROUP              20091
#define IDC_REVOKE_DATABASE_ROLE               20092
#define IDC_REVOKE_DATABASE_PUBLIC             20093
#define IDC_REVOKE_DATABASE_GRANTEES           20094

//
// FOR SECURITY ALARM (REFERENCE)
//

#define IDD_REFALARM                           5020
#define IDC_REFALARM_SUCCESS                   20100
#define IDC_REFALARM_FAILURE                   20101
#define IDC_REFALARM_SELECT                    20102
#define IDC_REFALARM_DELETE                    20103
#define IDC_REFALARM_INSERT                    20104
#define IDC_REFALARM_UPDATE                    20105
#define IDC_REFALARM_DATABASE                  20106
#define IDC_REFALARM_ONTABLE                   20107
#define IDC_REFALARM_BYUSER                    20108

//
// FOR REPLICATION CONNECTION
//

#define IDD_REPL_CONN                          5021
#define IDC_REPL_CONN_DESCRIPTION              20110
#define IDC_REPL_CONN_NUMBER                   20111
#define IDC_REPL_CONN_SERVER                   20112
#define IDC_REPL_CONN_VNODE                    20113
#define IDC_REPL_CONN_DBNAME                   20114
#define IDC_REPL_CONN_TYPE                     20115
#define IDC_REPL_CONN_SPIN_NUMBER              20116
#define IDC_REPL_CONN_SPIN_SERVER              20117
#define IDC_REPL_STATIC_SERVER                 20118
#define IDC_MOBILE_USER_STATIC                 20119
#define IDC_MOBILE_USER_NAME                   20120
#define IDC_DBNAME_STATIC                      20121
#define IDC_DB_NAME_EDIT                       20122

//
// FOR MAIL REPLICATION 
//

#define IDD_MAIL                               5022
#define IDC_MAIL_TEXT                          20020

//
// FOR ALTER USER
//

#define IDD_ALTER_USER                         5023
#define IDC_ALTER_USER_NAME                    21000
#define IDC_ALTER_USER_DEFAULTGROUP            21001
#define IDC_ALTER_USER_CREATEDB_U              21002
#define IDC_ALTER_USER_TRACE_U                 21003
#define IDC_ALTER_USER_SECURITY_U              21004
#define IDC_ALTER_USER_OPERATOR_U              21005
#define IDC_ALTER_USER_MAINTAINLOCATION_U      21006
#define IDC_ALTER_USER_AUDITOR_U               21007
#define IDC_ALTER_USER_MAINTAINAUDIT_U         21008
#define IDC_ALTER_USER_MAINTAINUSER_U          21009
#define IDC_ALTER_USER_NOPRIVILEGES_U          21010
#define IDC_ALTER_USER_CREATEDB_D              21011
#define IDC_ALTER_USER_TRACE_D                 21012
#define IDC_ALTER_USER_SECURITY_D              21013
#define IDC_ALTER_USER_OPERATOR_D              21014
#define IDC_ALTER_USER_MAINTAINLOCATION_D      21015
#define IDC_ALTER_USER_AUDITOR_D               21016
#define IDC_ALTER_USER_MAINTAINAUDIT_D         21017
#define IDC_ALTER_USER_MAINTAINUSER_D          21018
#define IDC_ALTER_USER_NOPRIVILEGES_D          21019
#define IDC_ALTER_USER_ALLEVENTS               21020
#define IDC_ALTER_USER_DEFAULTEVENTS           21021
#define IDC_ALTER_USER_QUERYTEXT               21022
#define IDC_ALTER_USER_EXPIREDATE              21023
#define IDC_ALTER_USER_LIMITINGSECURITY        21024
#define IDC_ALTER_USER_OLDPASSWORD             21025
#define IDC_ALTER_USER_PASSWORD                21026
#define IDC_ALTER_USER_CONFIRMPASSWORD         21027
#define IDC_ALTER_USER_EXTERNALPASSWORD        21028
#define IDC_ALTER_USER_DATABASE                21029
#define IDC_ALTER_USER_LNAME                   21030
#define IDC_ALTER_USER_LOLDPASSWORD            21031
#define IDC_ALTER_USER_DEFAULTPROFILE          21032
#define IDC_ALTER_USER_LPASSWORD               21033
#define IDC_ALTER_USER_LCONFIRMPASSWORD        21034

//
// FOR ALTER GROUP
//

#define IDD_ALTER_GROUP                        5024
#define IDC_ALTER_GROUP_GROUPNAME              20210
#define IDC_ALTER_GROUP_USERLIST               20211

//
// FOR ALTER ROLE
//
#define IDD_ALTERROLE                          5025
#define IDC_ALTERROLE_NAME                     21000
#define IDC_ALTERROLE_CREATEDB_U               21002
#define IDC_ALTERROLE_TRACE_U                  21003
#define IDC_ALTERROLE_SECURITY_U               21004
#define IDC_ALTERROLE_OPERATOR_U               21005
#define IDC_ALTERROLE_MAINTAINLOCATION_U       21006
#define IDC_ALTERROLE_AUDITOR_U                21007
#define IDC_ALTERROLE_MAINTAINAUDIT_U          21008
#define IDC_ALTERROLE_MAINTAINUSER_U           21009
#define IDC_ALTERROLE_NOPRIVILEGES_U           21010
#define IDC_ALTERROLE_CREATEDB_D               21011
#define IDC_ALTERROLE_TRACE_D                  21012
#define IDC_ALTERROLE_SECURITY_D               21013
#define IDC_ALTERROLE_OPERATOR_D               21014
#define IDC_ALTERROLE_MAINTAINLOCATION_D       21015
#define IDC_ALTERROLE_AUDITOR_D                21016
#define IDC_ALTERROLE_MAINTAINAUDIT_D          21017
#define IDC_ALTERROLE_MAINTAINUSER_D           21018
#define IDC_ALTERROLE_NOPRIVILEGES_D           21019
#define IDC_ALTERROLE_ALLEVENTS                21020
#define IDC_ALTERROLE_DEFAULTEVENTS            21021
#define IDC_ALTERROLE_QUERYTEXT                21022
#define IDC_ALTERROLE_LIMITINGSECURITY         21024
//#define IDC_ALTERROLE_OLDPASSWORD              21025
#define IDC_ALTERROLE_PASSWORD                 21026
#define IDC_ALTERROLE_CONFIRMPASSWORD          21027
#define IDC_ALTERROLE_EXTERNALPASSWORD         21028
#define IDC_ALTERROLE_DATABASE                 21029
#define IDC_ALTERROLE_LNAME                    21030
//#define IDC_ALTERROLE_LOLDPASSWORD             21031
#define IDC_ALTERROLE_LPASSWORD                21033
#define IDC_ALTERROLE_LCONFIRMPASSWORD         21034

//
// FOR ADD USER TO GROUP
//

#define IDD_USR2GRP                            5026
#define IDC_USR2GRP_USERBOX                    20220

// 
// AUDIT DB
//

#define IDD_AUDITDB                            5028
#define IDC_AUDITDB_SYSCAT                     20240
#define IDC_AUDITDB_TABLES                     20241
#define IDC_AUDITDB_IDTABLE                    20242
#define IDC_AUDITDB_FILES                      20243
#define IDC_AUDITDB_IDFILE                     20244
#define IDC_AUDITDB_BEFORE                     20245
#define IDC_AUDITDB_AFTER                      20246
#define IDC_AUDITDB_CKP                        20247
#define IDC_AUDITDB_CKP_NUMBER                 20248
#define IDC_AUDITDB_CKP_BUTTON                 20249
#define IDC_AUDITDB_ACTION_USER                20250
#define IDC_AUDITDB_WAIT                       20251
#define IDC_AUDITDB_INCONSISTENT               20252

//
// FOR SPECIFY TABLE 
//

#define IDD_AUDITDBT                           5029
#define IDC_AUDITDBT_TABLES                    20260

//
// FOR SPECIFY FILE (FOR AUDITDB)
//

#define IDD_AUDITDBF                           5030
#define IDC_AUDITDBF_TABLE                     20265
#define IDC_AUDITDBF_FILE                      20266

//
// FOR OPTIMIZE DATABASE ( OPTIMIZEDB )
//
#define IDD_OPTIMIZEDB                         5031
#define IDC_OPTIMIZEDB_PARAM                   20270
#define IDC_OPTIMIZEDB_PARAMFILE               20271
#define IDC_OPTIMIZEDB_STAT                    20272
#define IDC_OPTIMIZEDB_STATFILE                20273
#define IDC_OPTIMIZEDB_PRECILEVELNUM           20274
#define IDC_OPTIMIZEDB_PRECILEVELSPIN          20275
#define IDC_OPTIMIZEDB_ROW_PAGE                20276
#define IDC_OPTIMIZEDB_WAIT                    20277
#define IDC_OPTIMIZEDB_SYSCAT                  20278
#define IDC_OPTIMIZEDB_INFOCOL                 20279
#define IDC_OPTIMIZEDB_HISTOGRAM               20280
#define IDC_OPTIMIZEDB_GETSTAT                 20281
#define IDC_OPTIMIZEDB_COMPLETEFLAG            20282
#define IDC_OPTIMIZEDB_MINMAXVALUE             20283
#define IDC_OPTIMIZEDB_SPECIFYTABLES           20284
#define IDC_OPTIMIZEDB_IDTABLES                20285
#define IDC_OPTIMIZEDB_SPECIFYCOLUMS           20286
#define IDC_OPTIMIZEDB_IDCOLUMS                20287
#define IDC_OPTIMIZEDB_MCI_NUMBER              20288
#define IDC_OPTIMIZEDB_MCI_SPIN                20289
#define IDC_OPTIMIZEDB_MCE_NUMBER              20290
#define IDC_OPTIMIZEDB_MCE_SPIN                20291
#define IDC_OPTIMIZEDB_STATSAMPLEDATA          20292
#define IDC_OPTIMIZEDB_SORTTUPLE               20293
#define IDC_OPTIMIZEDB_PERCENT_NUMBER          20294
#define IDC_OPTIMIZEDB_PERCENT_SPIN            20295
#define IDC_OPTIMIZEDB_IDDISPLAY               20296
#define IDC_OPTIMIZEDB_MCI_CHECK               20297
#define IDC_OPTIMIZEDB_MCE_CHECK               20298
#define IDC_OPTIMIZEDB_PRECILEVEL_TXT          20299
#define IDC_OPTIMIZEDB_PERCENT_TXT             20300
#define IDC_EDIT_COLUMN_NAME                   20301
#define IDC_STATIC_COLUMN_NAME                 20302
#define IDC_CHKBX_OUTPUT_FILE                  20303
#define IDC_EDIT_OUTPUT_FILE                   20304
#define IDC_OPTIMIZEDB_CHECK_INDEX             20305
#define IDC_OPTIMIZEDB_BUTTON_INDEX            20306
#define IDC_OPTIMIZEDB_STATIC_INDEX            20307
#define IDC_OPTIMIZEDB_EDIT_INDEX              20308
#define IDC_OPTIMIZEDB_COMPOSITE_ON_BASE_TABLE 20309
#define IDC_OPTIMIZEDB_ZDN                     20310
#define IDC_OPTIMIZEDB_ZLR                     20311

//
// SPECIFY COLUMNS
//

#define IDD_SPECIFY_COL                        5032
#define IDC_SPECIFY_COL_TABLE                  20350
#define IDC_SPECIFY_COL_COL                    20351

//
// FOR ALTERDB
//
#define IDD_ALTERDB                            5033
#define IDC_ALTERDB_BLOCK                      20360
#define IDC_ALTERDB_BLOCKSIZE                  20361
#define IDC_ALTERDB_INITIAL                    20362
#define IDC_ALTERDB_DJOURNALING                20363
#define IDC_ALTERDB_DELETEOLDCKP               20364
#define IDC_ALTERDB_VERBOSE                    20365
#define IDC_ALTERDB_BLOCKSPIN                  20366
#define IDC_ALTERDB_INITIALSPIN                20367
/*
** Add identifier values for added radio buttons to the alterdb dialog.
** IDC_JOURNAL_BLOCK_SIZE is out of sequence due existence of the next
** available value.
*/
#define IDC_TARGET_JNL_BLOCKS                  20368
#define IDC_INITIAL_JNL_BLOCKS                 20369
#define IDC_JOURNAL_BLOCK_SIZE                 20359

//
// FOR VERIFYDB
//
#define IDD_VERIFDB                            5034
#define IDC_VERIFDB_MODE                       20370
#define IDC_VERIFDB_SCOPE                      20371
#define IDC_VERIFDB_OPERATION                  20372
#define IDC_VERIFDB_TABLE                      20373
#define IDC_VERIFDB_DATABASE                   20374
#define IDC_VERIFDB_VERBOSE                    20375
#define IDC_VERIFDB_NOLOGMODE                  20376
#define IDC_VERIFDB_LOGTO                      20377
#define IDC_VERIFDB_LOGTO_EDIT                 20378
#define IDC_VERIFDB_LTABLE                     20379

//
// FOR CHECKPOINT
//
#define IDD_CHKPOINT                           5035
#define IDC_CHKPOINT_DEVICE                    20380
#define IDC_CHKPOINT_DELETEPREV                20381
#define IDC_CHKPOINT_EJOURNALING               20382
#define IDC_CHKPOINT_DJOURNALING               20383
#define IDC_CHKPOINT_XLOCK                     20384
#define IDC_CHKPOINT_WAIT                      20385
#define IDC_CHKPOINT_VERBOSE                   20386
#define IDC_PARALLEL_CHECK                     20387
#define IDC_PARALLEL_LOCS                      20388
#define IDC_LOCS                               20389
#define IDC_STATIC_TABLE_NAME                  20390
#define IDC_EDIT_TABLE_NAME                    20391
#define IDC_CHKPOINT_TAPE_DEVICE	       20392
#define IDC_CHKPOINT_CHGJOURNAL		       20393

//
// FOR ROLLFORWARD
//
#define IDD_ROLLFWD                            5036
#define IDC_ROLLFWD_LASTCKP                    20400
#define IDC_ROLLFWD_JOURNALING                 20401
#define IDC_ROLLFWD_DEVICE                     20402
#define IDC_ROLLFWD_AFTER                      20403
#define IDC_ROLLFWD_BEFORE                     20404
#define IDC_ROLLFWD_SPECIFIEDCKP               20405
#define IDC_ROLLFWD_WAIT                       20406
#define IDC_ROLLFWD_VERBOSE                    20407
#define IDC_ROLLFWD_LASTCKP_NUM                20408
#define IDC_ROLLFWD_LASTCKP_BUTTON             20409
// added Dec. 19, 96
#define IDC_ROLLFWD_STATISTICS                 20410
#define IDC_ROLLFWD_RELOCATE                   20411
#define IDC_ROLLFWD_LOCATIONS                  20412
#define IDC_ROLLFWD_ONERRORCONTINUE            20413
#define IDC_ROLLFWD_NOSECONDARY_INDEX          20415
#define IDC_PARALLEL_RFW                       20416
#define IDC_PARALLEL_STATIC                    20417
#define IDC_PARALLEL_RFW_LOCS                  20418
#define IDC_CHECK_ROLLFWD_AFTER		       22100
#define IDC_CHECK_ROLLFWD_BEFORE	       22101
#define IDC_COMBO_ROLLFWD_CACHE		       22102
#define IDC_EDIT_ROLLFWD_BUF		       22103

//
// FOR ROLLFORWARD_RELOCATE
//
#define IDD_ROLLFWD_RELOCATE                   5404
#define IDC_ROLLFWD_RELOC_NEW           3
#define IDC_ROLLFWD_RELOC_DEL           4
#define IDC_ROLLFWD_RELOC_LIST          1000
#define IDC_ROLLFWD_RELOC_PREVLOC       1001
#define IDC_ROLLFWD_RELOC_REPLLOC       1005

//
// FOR STATDUMP
//

#define IDD_STATDUMP                           5037
#define IDC_STATDUMP_PARAM                     20420
#define IDC_STATDUMP_PARAMFILE                 20421
#define IDC_STATDUMP_DIRECT                    20422
#define IDC_STATDUMP_DIRECTFILE                20423
#define IDC_STATDUMP_WAIT                      20424
#define IDC_STATDUMP_DISPSYSCAT                20425
#define IDC_STATDUMP_DELSYSCAT                 20426
#define IDC_STATDUMP_QUIETMODE                 20427
#define IDC_STATDUMP_PRECILEVEL                20428
#define IDC_STATDUMP_PRECILEVELNUM             20429
#define IDC_STATDUMP_PRECILEVELSPIN            20430
#define IDC_STATDUMP_SPECIFYTABLES             20431
#define IDC_STATDUMP_IDTABLES                  20432
#define IDC_STATDUMP_SPECIFYCOLUMS             20433
#define IDC_STATDUMP_IDCOLUMS                  20434
#define IDC_STATDUMP_IDDISPLAY                 20435
#define IDC_STATDUMP_CHECK_INDEX               20436
#define IDC_STATDUMP_STATIC_INDEX              20437
#define IDC_STATDUMP_BUTTON_INDEX              20438
#define IDC_STATDUMP_EDIT_INDEX                20439
#define IDC_STATDUMP_COMPOSITE_ON_BASE_TABLE   20340

//
// FOR DISPLAY (SQL FLAG)
//

#define IDD_DISPLAY                            5038
#define IDC_DISPLAY_CHARCOLWIDTH_N             20500
#define IDC_DISPLAY_CHARCOLWIDTH_S             20501
#define IDC_DISPLAY_TEXTCOLWIDTH_N             20502
#define IDC_DISPLAY_TEXTCOLWIDTH_S             20503
#define IDC_DISPLAY_I1WIDTH_NUMBER             20504
#define IDC_DISPLAY_I1WIDTH_SPIN               20505
#define IDC_DISPLAY_I2WIDTH_NUMBER             20506
#define IDC_DISPLAY_I2WIDTH_SPIN               20507
#define IDC_DISPLAY_I4WIDTH_NUMBER             20508
#define IDC_DISPLAY_I4WIDTH_SPIN               20509
#define IDC_DISPLAY_FLOAT4                     20510
#define IDC_DISPLAY_FLOAT8                     20511
#define IDC_DISPLAY_FLOAT4_WNUMBER             20512
#define IDC_DISPLAY_FLOAT4_WSPIN               20513
#define IDC_DISPLAY_FLOAT4_DNUMBER             20514
#define IDC_DISPLAY_FLOAT4_DSPIN               20515
#define IDC_DISPLAY_FLOAT8_WNUMBER             20516
#define IDC_DISPLAY_FLOAT8_WSPIN               20517
#define IDC_DISPLAY_FLOAT8_DNUMBER             20518
#define IDC_DISPLAY_FLOAT8_DSPIN               20519

//
// FOR REVOKE TABLE IN GENERAL
//
#define IDD_RVKGTAB                            5040
#define IDC_RVKGTAB_SELECT                     20700
#define IDC_RVKGTAB_INSERT                     20701
#define IDC_RVKGTAB_DELETE                     20702
#define IDC_RVKGTAB_UPDATE                     20703
#define IDC_RVKGTAB_COPYINTO                   20704
#define IDC_RVKGTAB_COPYFROM                   20705
#define IDC_RVKGTAB_REFERENCE                  20706
#define IDC_RVKGTAB_USERS                      20707
#define IDC_RVKGTAB_GROUPS                     20708
#define IDC_RVKGTAB_ROLES                      20709
#define IDC_RVKGTAB_PUBLIC                     20710
#define IDC_RVKGTAB_GRANTEES                   20711
#define IDC_RVKGTAB_PRIV                       20712
#define IDC_RVKGTAB_GOPT                       20713
#define IDC_RVKGTAB_CASCADE                    20714
#define IDC_RVKGTAB_RESTRICT                   20715

//
// FOR LOCATING OBJECT
//
#define IDD_LOCATE                             5041
#define IDC_LOCATE_OBJECTTYPE                  20800
#define IDC_LOCATE_DATABASE                    20801
#define IDC_LOCATE_DELETE                      20802
#define IDC_LOCATE_FIND                        20803
#define IDC_LOCATE_LDATABASE                   20804

//
// FOR REVOKE DATABASE IN GENERAL
//
#define IDD_RVKGDB                             5042
#define IDC_RVKGDB_USERS                       20900
#define IDC_RVKGDB_GROUPS                      20901
#define IDC_RVKGDB_ROLES                       20902
#define IDC_RVKGDB_PUBLIC                      20903
#define IDC_RVKGDB_GRANTEES                    20904
#define IDC_RVKGDB_PRIV                        20905
#define IDC_RVKGDB_GOPT                        20906
#define IDC_RVKGDB_CASCADE                     20907
#define IDC_RVKGDB_RESTRICT                    20908
#define IDC_RVKGDB_PRIVILEGES                  20909
#define IDC_RVKGDB_NOPRIVILEGES                20910

//
// FOR CREATE/ALTER PROFILE
//
#define IDD_PROFILE                            5043
#define IDC_PROFILE_NAME                       20950
#define IDC_PROFILE_DEFAULTGROUP               20951
#define IDC_PROFILE_CREATEDB                   20952
#define IDC_PROFILE_TRACE                      20953
#define IDC_PROFILE_SECURITY                   20954
#define IDC_PROFILE_OPERATOR                   20955
#define IDC_PROFILE_MAINTAINLOCATION           20956
#define IDC_PROFILE_AUDITOR                    20957
#define IDC_PROFILE_MAINTAINAUDIT              20958
#define IDC_PROFILE_MAINTAINUSER               20959
#define IDC_PROFILE_NOPRIVILEGES               20960
#define IDC_PROFILE_CREATEDB_U                 20962
#define IDC_PROFILE_TRACE_U                    20963
#define IDC_PROFILE_SECURITY_U                 20964
#define IDC_PROFILE_OPERATOR_U                 20965
#define IDC_PROFILE_MAINTAINLOCATION_U         20966
#define IDC_PROFILE_AUDITOR_U                  20967
#define IDC_PROFILE_MAINTAINAUDIT_U            20968
#define IDC_PROFILE_MAINTAINUSER_U             20969
#define IDC_PROFILE_NOPRIVILEGES_U             20970
#define IDC_PROFILE_CREATEDB_D                 20972
#define IDC_PROFILE_TRACE_D                    20973
#define IDC_PROFILE_SECURITY_D                 20974
#define IDC_PROFILE_OPERATOR_D                 20975
#define IDC_PROFILE_MAINTAINLOCATION_D         20976
#define IDC_PROFILE_AUDITOR_D                  20977
#define IDC_PROFILE_MAINTAINAUDIT_D            20978
#define IDC_PROFILE_MAINTAINUSER_D             20979
#define IDC_PROFILE_NOPRIVILEGES_D             20980
#define IDC_PROFILE_ALLEVENTS                  20981
#define IDC_PROFILE_DEFAULTEVENTS              20982
#define IDC_PROFILE_QUERYTEXT                  20983
#define IDC_PROFILE_EXPIREDATE                 20984
#define IDC_PROFILE_LIMITINGSECURITY           20985
#define IDC_PROFILE_LNAME                      20986

//
// FOR CREATE USER
//
#define IDD_USER                               5044
#define IDC_USER_NAME                          21000
#define IDC_USER_DEFAULTGROUP                  21001
#define IDC_USER_CREATEDB_U                    21002
#define IDC_USER_TRACE_U                       21003
#define IDC_USER_SECURITY_U                    21004
#define IDC_USER_OPERATOR_U                    21005
#define IDC_USER_MAINTAINLOCATION_U            21006
#define IDC_USER_AUDITOR_U                     21007
#define IDC_USER_MAINTAINAUDIT_U               21008
#define IDC_USER_MAINTAINUSER_U                21009
#define IDC_USER_NOPRIVILEGES_U                21010
#define IDC_USER_CREATEDB_D                    21011
#define IDC_USER_TRACE_D                       21012
#define IDC_USER_SECURITY_D                    21013
#define IDC_USER_OPERATOR_D                    21014
#define IDC_USER_MAINTAINLOCATION_D            21015
#define IDC_USER_AUDITOR_D                     21016
#define IDC_USER_MAINTAINAUDIT_D               21017
#define IDC_USER_MAINTAINUSER_D                21018
#define IDC_USER_NOPRIVILEGES_D                21019
#define IDC_USER_ALLEVENTS                     21020
#define IDC_USER_DEFAULTEVENTS                 21021
#define IDC_USER_QUERYTEXT                     21022
#define IDC_USER_EXPIREDATE                    21023
#define IDC_USER_LIMITINGSECURITY              21024
#define IDC_USER_OLDPASSWORD                   21025
#define IDC_USER_PASSWORD                      21026
#define IDC_USER_CONFIRMPASSWORD               21027
#define IDC_USER_EXTERNALPASSWORD              21028
#define IDC_USER_DATABASE                      21029
#define IDC_USER_LNAME                         21030
#define IDC_USER_LOLDPASSWORD                  21031
#define IDC_USER_DEFAULTPROFILE                21032
#define IDC_USER_LPASSWORD                     21033
#define IDC_USER_LCONFIRMPASSWORD              21034

//
// FOR GRANTING ROLE TO USER
//
#define IDD_GRANTROL                           5045
#define IDC_GRANTROL_USERS                     21100
#define IDC_GRANTROL_ROLES                     21101

//
// FOR REVOKING ROLE FROM USER
//
#define IDD_RVKGROL                            5046
#define IDC_RVKGROL_USERS                      21200
#define IDC_RVKGROL_ROLES                      21201

//
// FOR REVOKING DBEVENT FROM USER (IN GENERAL)
//

#define IDD_RVKGDBE                            5047
#define IDC_RVKGDBE_RAISE                      21100   
#define IDC_RVKGDBE_REGISTER                   21101   
#define IDC_RVKGDBE_USERS                      21102   
#define IDC_RVKGDBE_GROUPS                     21103   
#define IDC_RVKGDBE_ROLES                      21104   
#define IDC_RVKGDBE_PUBLIC                     21105   
#define IDC_RVKGDBE_GRANTEES                   21106   
#define IDC_RVKGDBE_PRIV                       21107   
#define IDC_RVKGDBE_GOPT                       21108  
#define IDC_RVKGDBE_CASCADE                    21109   
#define IDC_RVKGDBE_RESTRICT                   21110   

//
// REVOKE PRIVILEGES FROM VIEW (IN GENERAL)
//

#define IDD_RVKGVIEW                           5048
#define IDC_RVKGVIEW_SELECT                    20700
#define IDC_RVKGVIEW_INSERT                    20701
#define IDC_RVKGVIEW_DELETE                    20702
#define IDC_RVKGVIEW_UPDATE                    20703
#define IDC_RVKGVIEW_USERS                     20707
#define IDC_RVKGVIEW_GROUPS                    20708
#define IDC_RVKGVIEW_ROLES                     20709
#define IDC_RVKGVIEW_PUBLIC                    20710
#define IDC_RVKGVIEW_GRANTEES                  20711
#define IDC_RVKGVIEW_PRIV                      20712
#define IDC_RVKGVIEW_GOPT                      20713
#define IDC_RVKGVIEW_CASCADE                   20714
#define IDC_RVKGVIEW_RESTRICT                  20715

//
// REVOKE PRIVILEGES FROM PROCEDURE (IN GENERAL) 
//

#define IDD_RVKGPROC                           5049
#define IDC_RVKGPROC_USERS                     21200
#define IDC_RVKGPROC_PROCS                     21201

//
// MODIFY STRUCTURE (TABLE/INDEX) PRINCIPAL DIALOG BOX
//

#define IDD_MODIFSTR                           5050
#define IDC_MODIFSTR_RELOC                     20000
#define IDC_MODIFSTR_CHANGELOC                 20001
#define IDC_MODIFSTR_CHANGESTORAGE             20002
#define IDC_MODIFSTR_ADDPAGE                   20003
#define IDC_MODIFSTR_SHRINKBTREEIDX            20004
#define IDC_MODIFSTR_DELETEALLDATA             20005
#define IDC_MODIFSTR_IDLOC01                   20006
#define IDC_MODIFSTR_IDLOC02                   20007
#define IDC_MODIFSTR_IDCHANGESTRUCT            20008
#define IDC_MODIFSTR_NBPAGE                    20009
#define IDC_MODIFSTR_LNBPAGE                   20010
#define IDC_MODIFSTR_SPIN_NBPAGE               20011
#define IDC_MODIFSTR_READONLY                  20012
#define IDC_MODIFSTR_NOREADONLY                20013
#define IDC_MARKAS_PHYSCONSIST                 20014
#define IDC_MARKAS_PHYSINCONSIST               20015
#define IDC_MARKAS_LOGICONSIST                 20016
#define IDC_MARKAS_LOGIINCONSIST               20017
#define IDC_MARKAS_ALLOWED                     20018
#define IDC_MARKAS_DISALLOWED                  20019
#define IDC_STATIC_MARKAS                      20020
#define IDC_CHECK_CONCURRENT_UPDATE            20021

//
// MODIFY STRUCTURE (TABLE/INDEX) SUB DIALOG BOX
//

#define IDD_CHANGEST                           5051
#define IDC_CHANGEST_STRUCTURE                 20000
#define IDC_CHANGEST_UNIQUESCOPE               20001
#define IDC_CHANGEST_PREALLOCPAGE              20002
#define IDC_CHANGEST_FILLFACTOR                20003
#define IDC_CHANGEST_MINPAGE                   20004
#define IDC_CHANGEST_MAXPAGE                   20005
#define IDC_CHANGEST_LEAFFILL                  20006
#define IDC_CHANGEST_NONLEAFFILL               20007
#define IDC_CHANGEST_KEY                       20008
#define IDC_CHANGEST_DATA                      20009
#define IDC_CHANGEST_EXTENTPAGE                20010
#define IDC_CHANGEST_PERSISTENCE               20011
#define IDC_CHANGEST_SPIN_PREALLOC             20012
#define IDC_CHANGEST_SPIN_FILLFACTOR           20013
#define IDC_CHANGEST_SPIN_MINPAGE              20014
#define IDC_CHANGEST_SPIN_MAXPAGE              20015
#define IDC_CHANGEST_SPIN_LEAFFILL             20016
#define IDC_CHANGEST_SPIN_NONLEAFFILL          20017
#define IDC_CHANGEST_SPIN_EXTENTPAGE           20018
#define IDC_CHANGEST_UNIQUE                    20019  
#define IDC_CHANGEST_UNIQUENO                  20020  
#define IDC_CHANGEST_UNIQUEROW                 20021
#define IDC_CHANGEST_UNIQUESTAT                20022
#define IDC_CHANGEST_L_PREALLOCPAGE            20023
#define IDC_CHANGEST_L_FILLFACTOR              20024
#define IDC_CHANGEST_L_MINPAGE                 20025
#define IDC_CHANGEST_L_MAXPAGE                 20026
#define IDC_CHANGEST_L_LEAFFILL                20027
#define IDC_CHANGEST_L_NONLEAFFILL             20028
#define IDC_CHANGEST_L_EXTENTPAGE              20029
#define IDC_CHANGEST_SPECIFYKEY                20030
#define IDC_CHANGEST_IDSPECIFYKEY              20031
#define IDC_CHANGEST_HIDATA                    20032


//
// MODIFY STRUCTURE (TABLE/INDEX) SUB DIALOG BOX FOR CHANGING LOCATIONS
//

#define IDD_LOCSEL01                           5052
#define IDC_LOCSEL01_LOC                       20000

//
// MODIFY STRUCTURE (TABLE/INDEX) SUB DIALOG BOX FOR REORGANIZING
//

#define IDD_RELOCATE                           5053
#define IDC_RELOCATE_CONTAINER                 23000


// JFS Begin
//
// For CDDS
//

#define IDD_REPL_CDDS                          5058
#define ID_CDDSNUM                             20150
#define ID_PATHCONTAINER                       20151
#define ID_ADDPATH                             20152
#define ID_ADDDONE                             20153
#define ID_REMOVEPATH                          20154
#define ID_TABLES                              20155
#define ID_SUPPORTTABLE                        20156
#define ID_RULES                               20157
#define ID_LOOKUPTABLE                         20158
#define ID_PRIORITYLOOKUP                      20159
#define ID_COLSCONTAINER                       20160
#define ID_LOOKUPTABLESTATIC                   20161
#define ID_PRIORITYLOOKUPSTATIC                20162
#define ID_GROUPCOLUMNS                        20163
#define ID_PATHORIGINAL                        20164
#define ID_PATHLOCAL                           20165
#define ID_PATHTARGET                          20166
#define ID_TBLADDALL                           20167
#define ID_TBLNONE                             20168
#define ID_COLADDALL                           20169
#define ID_COLNONE                             20170
#define ID_TBLGROUP                            20171
// ps add for new version of replicator
#define IDD_REPL_CDDS_NEW                      5071
#define ID_DATABASES_CDDS                      20174
#define ID_ALTER_DATABASES_CDDS                20175
#define ID_COLLISION_MODE                      20176
#define ID_ERROR_MODE                          20177
#define ID_NAME_CDDS                           20178
#define IDC_DB_STATIC                          20185
#define IDC_VNODE_STATIC                       20186
#define IDC_TYPE_STATIC                        20187
#define IDC_SERVER_STATIC                      20188
#define ID_ADD_DATABASES_CDDS                  20189
#define ID_DROP_DATABASES_CDDS                 20190

// ps new dialog box for replicator version 1.1
#define IDD_REPL_TYPE_AND_SERVER               5072 
#define IDC_SERVER_NUMBER                      20179
#define IDC_CA_SERVER                          20180
#define IDC_TARGET_TYPE                        20181

#define IDD_REPL_CONFSVR                       5073
#define IDC_ALL_OPTION                         20182

// ps new dialog mobile parameter Replicator 1.05
#define IDD_MOBILE_PARAM                        5074
#define IDC_MOBIL_COLLISION                    20183
#define IDC_MOBIL_ERROR                        20184

// JFS End

//
// FOR CREATE RULE
//

#define IDD_RULE                               5056
#define IDC_RULE_NAME                          30000
#define IDC_RULE_INSERT                        30001
#define IDC_RULE_UPDATE                        30002
#define IDC_RULE_DELETE                        30003
#define IDC_RULE_SPECIFYCOLUMNS                30004
#define IDC_RULE_COLUMNS                       30005
#define IDC_RULE_WHERE                         30006
#define IDC_RULE_IDASSISTANCE                  30007
#define IDC_RULE_PROCEDURE                     30008
#define IDC_RULE_PROCEDURE_PARAMS              30009



#define IDI_ICON1                              113

//
// DIALOG BOXES IDENTIFIERS
//
#define IDD_DBLOCATIONS                        114
#define IDD_SQLTESTPREF                        117
#define IDD_COPYDB                             118
#define IDD_CREATEINDEX                        122
#define IDD_CREATETABLE                        123
#define IDD_TABLESTRUCT                        124
#define IDD_REFERENCES                         125
#define IDD_CREATEINDEX_GW                     126

//
// ids for dialogs controls
//
#define IDC_COMBO1                             1000
#define IDC_EDIT1                              1001
#define IDC_COMBO4                             1001
#define IDC_COMBO2                             1002
#define IDC_EDIT2                              1003
#define IDC_CHECK1                             1004
#define IDC_EDIT5                              1004
#define IDC_CHECK2                             1005
#define IDC_CHECK3                             1006
#define IDC_CHECK4                             1007
#define IDC_CHECK5                             1008
#define IDC_LIST0                              1009
#define IDC_EDIT8                              1009
#define IDC_CHECK6                             1010
#define IDC_LIST2                              1010
#define IDC_CHECK7                             1011
#define IDC_CHECK8                             1012
#define IDC_CHECK9                             1013
#define IDC_EDIT3                              1014
#define IDC_EDIT4                              1015
#define IDC_USER1                              1016
#define IDC_USER2                              1017
#define IDC_USER3                              1018
#define IDC_USER4                              1019
#define IDC_USER5                              1020
#define IDC_EDIT6                              1021
#define IDC_USER6                              1022
#define IDC_EDIT7                              1023
#define IDC_USER7                              1024
#define IDC_BUTTON2                            1025
#define IDC_COMBO3                             1026
#define IDC_BUTTON3                            1027
#define IDC_BUTTON4                            1028
#define IDC_BUTTON5                            1029
#define IDC_CALCULATE                          1030
#define IDC_CLEAR                              1031
#define IDC_STRUCTURE                          1032
#define IDC_SPINROWWIDTH                       1033
#define IDC_SPINKEYWIDTH                       1034
#define IDC_SPINROWS                           1035
#define IDC_SPINKEYS                           1036
#define IDC_SPINDATA                           1037
#define IDC_SPINIDX                            1038
#define IDC_SPINLEAF                           1039
#define IDC_ROWS                               1040
#define IDC_ROWWIDTH                           1041
#define IDC_UNIQUEKEYS                         1042
#define IDC_KEYWIDTH                           1043
#define IDC_DATA                               1044
#define IDC_INDEX                              1045
#define IDC_LEAF                               1046
#define IDC_TXT_ROWS                           1047
#define IDC_TXT_ROWWIDTH                       1048
#define IDC_TXT_UNIQUEKEYS                     1049
#define IDC_TXT_KEYWIDTH                       1050
#define IDC_TXT_DATA                           1051
#define IDC_TXT_INDEX                          1052
#define IDC_TXT_LEAF                           1053
#define IDC_TXT_STRUCTURE                      1054
#define IDC_LANGUAGE                           1055
#define IDC_PRODUCTS                           1056
#define IDC_DBNAME                             1057
#define IDC_PUBLIC                             1059
#define IDC_PRIVATE                            1060
#define IDC_CASE                               1061
#define IDC_LOCATIONS                          1069
#define IDC_FLAGS                              1070
#define IDC_OWNER                              1072
#define IDC_OBJECT                             1073
#define IDC_DB                                 1074
#define IDC_WAITDB                             1075
#define IDC_TABLES                             1076
#define IDC_DESCRIPTION                        1077
#define IDC_NUMBER                             1078
#define IDC_SPIN_NUMBER                        1079
#define IDC_SERVER                             1080
#define IDC_SPIN_SERVER                        1081
#define IDC_VNODE                              1082
#define IDC_TYPE                               1083
#define IDC_INGRESPAGES                        1084
#define IDC_BYTES                              1085
#define IDC_BLOCKSIZE                          1086
#define IDC_BLOCK                              1087
#define IDC_SPIN_BLOCK                         1088
#define IDC_DELETEOLDEST                       1089
#define IDC_DISABLE                            1090
#define IDC_MODE                               1091
#define IDC_SCOPE                              1092
#define IDC_OPERATION                          1093
#define IDC_TABLE                              1094
#define IDC_USER                               1095
#define IDC_LOGTO                              1096
#define IDC_VERBOSE                            1097
#define IDC_LOGFILE                            1098
#define IDC_SPIN_INITIAL                       1099
#define IDC_INITIAL                            1100
#define IDC_OBJECTS                            1101
#define IDC_GROUP                              1002
#define IDC_WINDOWS                            1102
#define IDC_SHOW                               1103
#define IDC_RECORDS                            1104
#define IDC_SPIN_RECORDS                       1105
#define IDC_PRINTABLE                          1106
#define IDC_DELETE                             1107
#define IDC_ENABLE                             1108
#define IDC_EXCLUSIVE                          1109
#define IDC_CPWAIT                             1110
#define IDC_DEVICE                             1111
#define IDC_PASSWORD                           1114
#define IDC_REMOTENODE                         1115
#define IDC_LISTENADDR                         1116
#define IDC_PROTOCOL                           1117
#define IDC_COLUMNS                            1120
#define IDC_STRUCT                             1121
#define IDC_LOCATION                           1122
#define IDC_CONTAINER                          1123
#define IDC_FILLFACTOR                         1124
#define IDC_MINPAGES                           1125
#define IDC_MAXPAGES                           1126
#define IDC_LEAFFILL                           1127
#define IDC_NONLEAFFILL                        1128
#define IDC_UNIQUE                             1129
#define IDC_COMPRESSION                        1130
#define IDC_CLOSE                              1131
#define IDC_ADD                                1132
#define IDC_SPIN_MINPAGES                      1133
#define IDC_SPIN_MAXPAGES                      1134
#define IDC_SPIN_LEAFFILL                      1135
#define IDC_SPIN_NONLEAFFILL                   1136
#define IDC_SPIN_FILLFACTOR                    1137
#define IDC_PERSISTENCE                        1138
#define IDC_SCHEMA                             1139
#define IDC_MAXPAGES2                          1140
#define IDC_ALLOCATION                         1141
#define IDC_MAXPAGES3                          1142
#define IDC_EXTEND                             1143
#define IDC_SPIN_MAXPAGES2                     1144
#define IDC_SPIN_ALLOCATION                    1145
#define IDC_SPIN_MAXPAGES3                     1146
#define IDC_SPIN_EXTEND                        1147
#define IDC_UNIQUE_NO                          1148
#define IDC_UNIQUE_ROW                         1149
#define IDC_UNIQUE_STATEMENT                   1150
#define IDC_KEY_COMPRESSION                    1151
#define IDC_DATA_COMPRESSION                   1152
#define IDC_TXT_MINPAGES                       1153
#define IDC_TXT_MAXPAGES                       1154
#define IDC_TXT_ALLOCATION                     1155
#define IDC_TXT_EXTEND                         1156
#define IDC_TXT_FILLFACTOR                     1157
#define IDC_TXT_LEAFFILL                       1158
#define IDC_TXT_NONLEAFFILL                    1159
#define IDC_COLNAME                            1160
#define IDC_DROP                               1161
#define IDC_NULLABLE                           1162
#define IDC_DEFAULT                            1163
#define IDC_DEFAULTSPEC                        1164
#define IDC_CHECK                              1165
#define IDC_CHECKSPEC                          1166
#define IDC_REF_TABLE                          1167
#define IDC_REF_COLUMN                         1168
#define IDC_REFERENCES                         1169
#define IDC_LABEL_GRAN                         1170
#define IDC_SEC_AUDIT_OPTION                   1171
#define IDC_SEC_AUDIT_KEY                      1172
#define IDC_JOURNALING                         1173
#define IDC_DUPLICATES                         1174
#define IDC_NAME                               1175
#define IDC_TBLCHECK                           1176
#define IDC_IDX_UNIQUE_GW                      1215 // should be OK/unique for the GW duplicated create index dialog resource
#define IDC_TYPE_TABLE                         1216
#define IDC_TYPE_INDEX                         1217
#define IDC_FRAME_PAGEORINDEX                  1218
#define IDC_BUTTON1                            1219
#define IDC_RADIO1			       1220
#define IDC_RADIO2			       1221

//
// Controls ID added for VDBA 1.5 and V2.0
#define IDC_EDITPAGESIZE                       1177
#define IDC_STATIC_PS                          1178
#define IDC_TLCLIST1                           1179
#define IDC_TLCLIST2                           1180
#define IDC_TLCCOMBO1                          1181
#define IDC_TLCBUTTON_ADD                      1182
#define IDC_TLCBUTTON_DELETE                   1183
#define IDC_TLCBUTTON_UP                       1184
#define IDC_TLCBUTTON_DOWN                     1185
#define IDC_TLCEDIT1                           1186
#define IDC_TLCBUTTON_ASSISTANT                1187
#define IDC_TLCBUTTON_UNIQUE                   1188
#define IDC_TLCBUTTON_CHECK                    1189
#define IDC_COMBOPAGESIZE                      1190
#define IDC_BUTTON_INGASSISTANT                1191
#define IDC_INGCHECKASSELECT                   1192
#define IDC_STATIC_INGCOLUMNS                  1193
#define IDC_STATIC_INGNAME                     1194
#define IDC_STATIC_INGCOLUMNHEADER             1195
#define IDC_INGEDITCOLUMNHEADER                1196
#define IDC_STATIC_INGSELECT                   1197
#define IDC_INGEDITSQL                         1198
#define IDC_BUTTON_PUT                         1199
#define IDC_BUTTON_REMOVE                      1200
#define IDC_REMOVE_CASCADE                     1201
#define IDC_BUTTON_UP                          1202
#define IDC_BUTTON_DOWN                        1203
#define IDC_PAGESIZE                           1204
#define IDC_STATIC_PAGESIZE                    1205
#define IDC_LIST3                              1206
#define IDC_STATIC1                            1207
#define IDC_STATIC2                            1208
#define IDC_STATIC3                            1209
#define IDC_STATIC4                            1210

//
// Controls ID added for VDBA 2.5
#define IDC_BUTTON_STAR_LOCALTABLE             1211

#define ID_FILE_EXIT                           40001
#define ID_FILE_PREFERENCES                    40003
#define ID_WINDOW_TILEHORIZONTAL               40005
#define ID_WINDOW_TILEVERTICAL                 40006
#define ID_WINDOW_ARRANGEICONS                 40007
#define ID_WINDOW_SELECTWINDOW                 40008
#define ID_WINDOW_CLOSEALLWINDOWS              40009
#define ID_HELP_CONTENTS                       40010
#define ID_HELP_SEARCH                         40011
#define ID_HELP_ABOUTCADBATOOLS                40012
#define ID_SERVER_CONNECT                      40013
#define ID_SERVER_DISCONNECT                   40014
#define ID_TABLES_POPULATE                     40024
#define ID_USERS_CREATE                        40026
#define ID_USERS_DROP                          40027
#define ID_USERS_ALTER                         40028
#define ID_USERS_GRANT                         40029
#define ID_USERS_REVOKE                        40030
#define ID_SERVER_BACKUP                       40031
#define ID_SERVER_RESTORE                      40032
#define ID_VIEW_EXPANDONEBRANCH                40033
#define ID_VIEW_EXPANDALLBRANCHES              40035
#define ID_VIEW_COLLAPESONELEVEL               40036
#define ID_VIEW_COLLAPSE                       40037
#define ID_VIEW_COLLAPSEALLBRANCHES            40038
#define ID_FILE_SAVEAS                         40043
#define ID_DATABASE_SQLTEST                    40051
#define ID_DATABASE_SHUTDOWNDATABASE           40052
#define ID_DATABASE_ENABLEDATABASE             40053
#define ID_DATABASE_LOCATIONS                  40055
#define ID_VIEW_FILTER                         40062
#define ID_VIEW_SHOWSYSTEMOBJECTS              40063
#define ID_WINDOW_TEAROUT                      40064
#define ID_VIEW_PROPERTIES                     40065
#define ID_OPERATIONS_MODIFYSTRUCTURE          40066
#define ID_OPERATIONS_GENERATESTATISTICS       40067
#define ID_OPERATIONS_DISPLAYSTATISTICS        40068
#define ID_OPERATIONS_SYSMOD                   40069
#define ID_OPERATIONS_LOCKING                  40070
#define ID_DATABASE_SPACE                      40071
#define ID_OPERATIONS_VERIFYDATABASE           40072

//#define IDS_SAVECHANGESTITLE                   20001
#define IDS_SAVECHANGES                        20002
#define IDS_UNTITLED                           20003
#define IDS_DELETESYSTEMOBJECT                 20004
#define IDS_STRUCT_BTREE                       20005
#define IDS_STRUCT_HASH                        20006
#define IDS_STRUCT_HEAP                        20007
#define IDS_STRUCT_ISAM                        20008
#define IDS_ERR_OUTSIDE_RANGE                  20009
#define IDS_ERR_TITLE                          20010
//#define IDS_PRODUCT                            20012
#define IDS_PROD_INGRES                        20013
#define IDS_PROD_VISION                        20014
#define IDS_PROD_W4GL                          20015
#define IDS_PROD_INGRES_DBD                    20016
#define IDS_PROD_NOFECLIENTS                   20017
#define IDS_DATAFILES                          20021
#define IDS_CHECKPOINTS                        20022
#define IDS_JOURNALS                           20023
#define IDS_DUMPS                              20024
#define IDS_ALLFILES                           20025
#define IDS_ALLFILES_FILTER                    20026
#define IDS_CFGFILES                           20027
#define IDS_CFGFILES_FILTER                    20028
#define IDS_FINDTEXT_WNDCLASS                  20029
#define IDS_FINDTEXT_MESSAGE                   20030
#define IDS_ERROR_CALC                         20031
#define IDS_VNODE_NOTEXIST                     20032
#define IDS_CONFIRMDROP                        20033
#define IDS_DELETE_OBJECT                      20034
#define IDS_CONFIRMDELETE                      20035
#define IDS_VNODECONFIG                        20036
#define IDS_FULLPEER                           20037
#define IDS_PROTREADONLY                       20038
#define IDS_UNPROTREADONLY                     20039
#define IDS_GATEWAY                            20040
#define IDS_GATEWAY2                           20041
#define IDS_CALCERRNOTIFY                      20044
#define IDS_VIRTNODE                           20045
#define IDS_DATABASE                           20046
#define IDS_USER                               20047
#define IDS_GROUP                              20048
#define IDS_GROUPUSER                          20049
#define IDS_ROLE                               20050
#define IDS_LOCATION                           20051
#define IDS_TABLE                              20052
#define IDS_TABLELOCATION                      20053
#define IDS_VIEW                               20054
#define IDS_VIEWTABLE                          20055
#define IDS_INDEX                              20056
#define IDS_INTEGRITY                          20057
#define IDS_PROCEDURE                          20058
#define IDS_RULE                               20059
#define IDS_RULEPROC                           20060
#define IDS_SCHEMAUSER                         20061
#define IDS_SYNONYM                            20062
#define IDS_DBEVENT                            20063
#define IDS_NETBIOS                            20064
#define IDS_TCPIP                              20065
#define IDS_ERR_DIALOG                         20066

#define IDS_STRUCT_CBTREE                      20068
#define IDS_STRUCT_CHASH                       20069
#define IDS_STRUCT_CISAM                       20070
#define IDS_IDXCNTR_TITLE                      20071
#define IDS_DESCENDING                         20072
#define IDS_KEY                                20073
#define IDS_COLUMNEXISTS                       20074
#define IDS_PRIMARY                            20075
#define IDS_DEFAULT                            20076
#define IDS_NULLABLE                           20077
#define IDS_UNIQUE                             20078
#define IDS_TYPE                               20079
#define IDS_LENGTH                             20080
#define IDS_CHECK                              20081
#define IDS_ROW                                20082
#define IDS_TABLEROW                           20083
#define IDS_TABLENOROW                         20084
#define IDS_NOROW                              20086
#define IDS_SYS_DEF                            20087
#define IDS_REFERENCING                        20088
#define IDS_REFERENCED                         20089
#define IDS_SAMETABLE                          20090
#define IDS_DEFSPEC                            20091
#define IDS_NONE                               20092
#define IDS_IIDATABASE                         20093

#define IDS_INGTYPE_C                          20094
#define IDS_INGTYPE_CHAR                       20095
#define IDS_INGTYPE_TEXT                       20096
#define IDS_INGTYPE_VARCHAR                    20097
#define IDS_INGTYPE_LONGVARCHAR                20098
#define IDS_INGTYPE_INT4                       20099
#define IDS_INGTYPE_INT2                       20100
#define IDS_INGTYPE_INT1                       20101
#define IDS_INGTYPE_DECIMAL                    20102
#define IDS_INGTYPE_FLOAT8                     20103
#define IDS_INGTYPE_FLOAT4                     20104
#define IDS_INGTYPE_DATE                       20105
#define IDS_INGTYPE_MONEY                      20106
#define IDS_INGTYPE_BYTE                       20107
#define IDS_INGTYPE_BYTEVAR                    20108
#define IDS_INGTYPE_LONGBYTE                   20109
#define IDS_INGTYPE_FLOAT                      20110
#define IDS_PRESEL                             20111
#define IDS_SIGDIG                             20112
#define IDS_DROPREF                            20114
#define IDS_ON                                 20115
#define IDS_UNNAMED                            20116
#define IDS_REFEXISTS                          20117
#define IDS_DELETEREF                          20118
#define IDS_REFDESCEXIST                       20119
#define IDS_REFDESCEXISTREN                    20120
#define IDS_NOVALIDIDXCOLS                     20121
#define IDS_INGTYPE_INTEGER                    20125
#define IDS_DROPAUDITKEY                       20130
#define IDS_STRUCT_HEAPSORT                    20132
#define IDS_WORK                               20133
#define IDS_INGTYPE_BOOL                       20134

#define IDS_COLLISION_PASSIVE                  20135   // PS 05/03/96 
#define IDS_COLLISION_ACTIVE                   20136   // PS 05/03/96
#define IDS_COLLISION_BEGNIN                   20137   // PS 05/03/96
#define IDS_COLLISION_PRIORITY                 20138   // PS 05/03/96
#define IDS_COLLISION_LASTWRITE                20139   // PS 05/03/96
#define IDS_ERROR_SKIP_TRANSACTION             20140   // PS 05/03/96
#define IDS_ERROR_SKIP_ROW                     20141   // PS 05/03/96
#define IDS_ERROR_QUIET_CDDS                   20142   // PS 05/03/96
#define IDS_ERROR_QUIET_DATABASE               20143   // PS 05/03/96
#define IDS_ERROR_QUIET_SERVER                 20144   // PS 05/03/96

#define IDS_NOCOLLISISION                      20145   // PS 06/06/96
#define IDS_QUEUECLEANING                      20146   // PS 06/06/96
#define IDS_LASTWRITECOLL                      20147   // PS 06/06/96
#define IDS_FIRSTWRITECOLL                     20148   // PS 06/06/96

#define IDS_ROLLBACKRETRY                      20149   // PS 06/06/96
#define IDS_ROLLBACKQUIET                      20150   // PS 06/06/96
#define IDS_ROLLBACKRECONF                     20151   // PS 06/06/96
#define IDS_ROLLBACKSKIP                       20152   // PS 06/06/96
#define IDS_STRUCT_RTREE                       20153
#define IDS_INGTYPE_OBJECTKEY                  20154
#define IDS_INGTYPE_TABLEKEY                   20155
#define IDS_SYST_MAINTAINED                    20156
#define IDS_FAILED_2_INITIALIZE_COM            20157
#define IDS_SVRIIA_NOT_REGISTERED              20158
#define IDS_FAILED_2_CREATE_AS_AGREGATION      20159
#define IDS_FAILED_2_QUERY_INTERFACE           20160
#define IDS_EXIST_OPENCURSOR_IN_SQLTEST        20161
#define IDS_MODIFY_STRUCTURExOPENCURSOR_DOM          20162
#define IDS_MODIFY_STRUCTURExOPENCURSOR_SQLTEST      20163
#define IDS_MODIFY_STRUCTURExOPENCURSOR_SQLTESTxDOM  20164
#define IDS_INGTYPE_UNICODE_NCHR               20172
#define IDS_INGTYPE_UNICODE_NVCHR              20173
#define IDS_INGTYPE_UNICODE_LNVCHR             20174
#define IDS_INGTYPE_INT8                       20175
#define IDS_INGTYPE_BIGINT                     20176

#define IDS_INGTYPE_ADTE                       20177   // ansidate
#define IDS_INGTYPE_TMWO                       20178   // time without time zone
#define IDS_INGTYPE_TMW                        20179   // time with time zone
#define IDS_INGTYPE_TME                        20180   // time with local time zone
#define IDS_INGTYPE_TSWO                       20181   // timestamp without time zone
#define IDS_INGTYPE_TSW                        20182   // timestamp with time zone
#define IDS_INGTYPE_TSTMP                      20183   // timestamp with local time zone
#define IDS_INGTYPE_INYM                       20184   // interval year to month
#define IDS_INGTYPE_INDS                       20185   // interval day to second
#define IDS_INGTYPE_IDTE                       20186   // ingresdate

#define IDS_T_CREATE_USER                      60001
#define IDS_T_CREATE_GROUP                     60002
#define IDS_T_CREATE_ROLE                      60003
#define IDS_T_CREATE_RULE                      60004
#define IDS_T_CREATE_PROCEDURE                 60005
#define IDS_T_CREATE_SECURITY                  60006
#define IDS_T_CREATE_SYNONYM                   60007
#define IDS_T_CREATE_LOCATION                  60008
#define IDS_T_CREATE_DBEVENT                   60009
#define IDS_T_CREATE_INTEGRITY                 60010
#define IDS_T_CREATE_VIEW                      60011
#define IDS_T_CREATE_SCHEMA                    60012

#define IDS_T_GRANT_DATABASE                   60013
#define IDS_T_GRANT_TABLE                      60014
#define IDS_T_GRANT_PROCEDURE                  60015
#define IDS_T_GRANT_DBEVENT                    60016

#define IDS_T_SESSION_SETTING                  60017
#define IDS_T_REFRESH_SETTING                  60018
#define IDS_T_REFRESH_2                        60019
#define IDS_T_VNODE_SELECTION                  60020        
#define IDS_T_SVR_DISCONNECT                   60021       
#define IDS_T_MAIL                             60022       
#define IDS_T_REPL_CONN                        60023       
#define IDS_T_CREATE_REFSYNONYM                60024
#define IDS_T_GRANT_VIEW                       60025
#define IDS_T_VNODE_DEFINITION                 60026        

#define IDS_T_GNREF_DATABASE                   60027
#define IDS_T_GNREF_TABLE                      60028
#define IDS_T_GNREF_VIEW                       60029
#define IDS_T_GNREF_PROCEDURE                  60030
#define IDS_T_GNREF_DBEVENT                    60031
#define IDS_T_REPL_TYPE_AND_SERVER             60032
#define IDS_T_GRANT_SEQUENCE                   60033

#define IDS_T_ALTER_USER                       61001
#define IDS_T_ALTER_GROUP                      61002
#define IDS_T_ALTER_ROLE                       61003
#define IDS_T_ALTER_RULE                       61004
#define IDS_T_ALTER_PROCEDURE                  61005
#define IDS_T_ALTER_SECURITY                   61006
#define IDS_T_ALTER_SYNONYM                    61007
#define IDS_T_ALTER_LOCATION                   61008
#define IDS_T_ALTER_DBEVENT                    61009
#define IDS_T_ALTER_INTEGRITY                  61010
#define IDS_T_ALTER_VIEW                       61011
#define IDS_T_ALTER_SCHEMA                     61012
#define IDS_T_ALTER_DATABASE                   61013

#define IDS_T_REVOKE_ON                        62000
#define IDS_T_REVOKE_INSERT_ON                 62001 
#define IDS_T_REVOKE_UPDATE_ON                 62002 
#define IDS_T_REVOKE_DELETE_ON                 62003 
#define IDS_T_REVOKE_SELECT_ON                 62004 
#define IDS_T_REVOKE_COPYINTO_ON               62005 
#define IDS_T_REVOKE_COPYFROM_ON               62006 
#define IDS_T_REVOKE_REFERENCE_ON              62007 
#define IDS_T_REVOKE_ALL_ON                    62008
#define IDS_T_REVOKE_ACCESS_ON                 62009
#define IDS_T_REVOKE_CREATE_PROCEDURE_ON       62010
#define IDS_T_REVOKE_CREATE_TABLE_ON           62011
#define IDS_T_REVOKE_CONNECT_TIME_LIMIT_ON     62012
#define IDS_T_REVOKE_DATABASE_ADMIN_ON         62013
#define IDS_T_REVOKE_LOCKMOD_ON                62014
#define IDS_T_REVOKE_QUERY_IO_LIMIT_ON         62015
#define IDS_T_REVOKE_QUERY_ROW_LIMIT_ON        62016
#define IDS_T_REVOKE_SELECT_SYSCAT_ON          62017
#define IDS_T_REVOKE_SESSION_PRIORITY_ON       62018
#define IDS_T_REVOKE_UPDATE_SYSCAT_ON          62019
#define IDS_T_REVOKE_TABLE_STATISTICS_ON       62020
#define IDS_T_REVOKE_NO_ACCESS_ON              62021
#define IDS_T_REVOKE_NO_CREATE_PROCEDURE_ON    62022
#define IDS_T_REVOKE_NO_CREATE_TABLE_ON        62023
#define IDS_T_REVOKE_NO_CONNECT_TIME_LIMIT_ON  62024
#define IDS_T_REVOKE_NO_DATABASE_ADMIN_ON      62025
#define IDS_T_REVOKE_NO_LOCKMOD_ON             62026
#define IDS_T_REVOKE_NO_QUERY_IO_LIMIT_ON      62027
#define IDS_T_REVOKE_NO_QUERY_ROW_LIMIT_ON     62028
#define IDS_T_REVOKE_NO_SELECT_SYSCAT_ON       62029
#define IDS_T_REVOKE_NO_SESSION_PRIORITY_ON    62030
#define IDS_T_REVOKE_NO_UPDATE_SYSCAT_ON       62031
#define IDS_T_REVOKE_NO_TABLE_STATISTICS_ON    62032
#define IDS_T_REVOKE_RAISE_ON                  62033
#define IDS_T_REVOKE_REGISTER_ON               62034
#define IDS_T_REVOKE_EXECUTE_ON                62035

#define IDS_T_USR2GRP                          62036
#define IDS_T_POPULATE_TABLE                   62037
#define IDS_T_ALTERDB                          62038
#define IDS_T_AUDITDB                          62039
#define IDS_T_STATDUMP                         62040
#define IDS_T_OPTIMIZEDB                       62041
#define IDS_T_VERIFYDB                         62042
#define IDS_T_CHECKPOINT                       62043
#define IDS_T_ROLLFORWARD                      62044
#define IDS_T_LOCATE                           62045   
#define IDS_T_RVKGTAB                          62046
#define IDS_T_RVKGVIEW                         62047
#define IDS_T_RVKGDB                           62048
#define IDS_T_RVKGDBE                          62049
#define IDS_T_CREATE_PROFILE                   62050
#define IDS_T_ALTER_PROFILE                    62051
#define IDS_T_GRANT_ROLE                       62052
#define IDS_T_RVKGROL                          62053
#define IDS_T_RVKGPROC                         62054   

#define IDS_T_MODIFY_TABLE_STRUCT              62055   
#define IDS_T_MODIFY_INDEX_STRUCT              62056   
#define IDS_T_RELOCATE_TABLE                   62057   
#define IDS_T_RELOCATE_INDEX                   62058   
#define IDS_T_CHANGELOC_OF_TABLE               62059   
#define IDS_T_CHANGELOC_OF_INDEX               62060   
#define IDS_T_TABLE_STRUCT                     62061   
#define IDS_T_INDEX_STRUCT                     62062   

// Emb June 9, 95
#define IDS_T_PROCEDURE_PARAM_CAPT             62063
#define IDS_T_PROCEDURE_DECLARE_CAPT           62064

#define IDS_T_CDDS_DEFINITION_ON               62065   
#define IDS_T_RMCMD_ALTERDB                    62066   
#define IDS_T_RMCMD_COPY_INOUT                 62067
#define IDS_T_RMCMD_STATDUMP                   62068
#define IDS_T_RMCMD_VERIFDB                    62069
#define IDS_T_RMCMD_AUDITDB                    62070
#define IDS_T_RMCMD_CHKPT                      62071
#define IDS_T_RMCMD_OPTIMIZEDB                 62072
#define IDS_T_RMCMD_ROLLFWD                    62073
#define IDS_T_RMCMD_UNLOADDB                   62074

#define IDS_T_REVOKE_IDLE_TIME_LIMIT_ON        62087
#define IDS_T_REVOKE_NO_IDLE_TIME_LIMIT_ON     62088

#define IDS_T_REPL_INST_CONN                   62090       
#define IDS_T_REPL_ALTER_CONN                  62091       
#define IDS_T_REPL_ALTER_LOCAL_CONN            62092

//
// New Dlg Grant Database (MFC)
#define IDS_T_REVOKE_QUERY_CPU_LIMIT_ON        62095
#define IDS_T_REVOKE_QUERY_PAGE_LIMIT_ON       62096
#define IDS_T_REVOKE_QUERY_COST_LIMIT_ON       62097
#define IDS_T_REVOKE_NO_QUERY_CPU_LIMIT_ON     62098
#define IDS_T_REVOKE_NO_QUERY_PAGE_LIMIT_ON    62099
#define IDS_T_REVOKE_NO_QUERY_COST_LIMIT_ON    62100

#define IDS_T_REVOKE_CREATE_SEQUENCE           62101
#define IDS_T_REVOKE_NO_CREATE_SEQUENCE        62102
#define IDS_T_REVOKE_NEXT_SEQUENCE_ON          62103
#define IDS_T_GNREF_SEQUENCE                   62104
#define ID_HELPID_GNREF_SEQUENCE               62105
#define IDC_STATIC_GNREF_PROCEDURE             62106
#define IDC_STATIC_GRANT_PROCEDURE             62107
#define IDS_STATIC_SEQUENCE                    62108

////////////////////////////////////////////////////////////////////////////
//
// Wizards (1000 - 1010 reserved for the wizard)
//

#define IDCW_SELECT                            1011
#define IDCW_INSERT                            1012
#define IDCW_UPDATE                            1013
#define IDCW_DELETE                            1014
#define IDCW_TABLE                             1015
#define IDCW_COLUMNS                           1016
#define IDCW_MANUAL                            1017
#define IDCW_SUBSELECT                         1018
#define IDCW_CONTAINER                         1019
#define IDCW_SEARCH                            1020
#define IDCW_FROMTABLES                        1021
#define IDCW_CNTUPDATE                         1022
#define IDCW_DELETE1_B_COND                    1023
#define IDCW_UPDATE2_B_COND                    1024
#define IDCW_ALLCOLUMNS                        1025
#define IDCW_SPECIFYCOLUMNS                    1026
#define IDCW_DISTINCT                          1027
#define IDCW_IDWHERE                           1028
#define IDCW_EDITWHERE                         1029
#define IDCW_IDGROUPBY                         1030
#define IDCW_EDITGROUPBY                       1031
#define IDCW_IDHAVING                          1032
#define IDCW_EDITHAVING                        1033
#define IDCW_IDUNION                           1034
#define IDCW_EDITUNION                         1035
#define IDCW_CONTAINER2                        1036
#define IDCW_COLUMNS2                          1037
#define IDCW_TEXT1                             1038
#define IDCW_TEXT2                             1039
#define IDCW_TEXT3                             1040
#define IDCW_TEXTOTHERCOLS                     1041
#define IDCW_IDOTHERCOLS                       1042
#define IDCW_EDITOTHERCOLS                     1043
#define IDCW_IDSUBSELECT                       1044
#define IDCW_VIEW                              1045
#define IDCW_RADIO_TABLE                       1046
#define IDCW_RADIO_VIEW                        1047

//
// Wizard dialogs
//
#define IDD_SQLWIZARD                          2000      // SQL Wizard main dialog
#define IDD_INSERT1                            2001      // SQL Wizard insert step 1
#define IDD_INSERT2                            2002      // SQL Wizard insert step 2
#define IDD_DELETE1                            2003      // SQL Wizard delete step 1
#define IDD_UPDATE1                            2004      // SQL Wizard update step 1
#define IDD_UPDATE2                            2005      // SQL Wizard update step 2
#define IDD_UPDATE3                            2006      // SQL Wizard update step 3
#define IDD_SELECT1                            2007      // SQL Wizard select step 1
#define IDD_SELECT2                            2008      // SQL Wizard select step 2
#define IDD_SELECT3                            2009      // SQL Wizard select step 3

//
// box saveenvironment
//
#define IDD_SAVEENV                            5401
#define IDC_SAVEENV_BPASSWORD                  19001

// box Save environment : password
#define IDD_SAVEENVPSW                         5402
#define IDC_SAVEENV_PASSWORD                   101
#define IDC_SAVEENV_CONFIRMPASSWORD            102

// Added Emb June 9. 95: subbox "add" for procedure box
#define IDD_PROC_SUBADD                        5403
#define IDC_PROC_SUBADD_NAME                   101
#define IDC_PROC_SUBADD_TYPE                   102
#define IDC_PROC_SUBADD_WIDTH                  103
#define IDC_PROC_SUBADD_NULL                   104
#define IDC_PROC_SUBADD_DEFAULT                105
#define IDC_PROC_SUBADD_WIDTH_CAPT             106

#define IDD_RVKGSEQUENCE                       5405
#define IDD_GRANT_SEQUENCE                     5406
//
// Star management : new controls in dialog boxes
//
// for create database dialog ( IDD_CREATEDB ):
#define IDC_DISTRIBUTED     1211
#define IDC_STATIC_CDB      1212
#define IDC_COORDNAME       1213
#define IDC_GENFE           1214


//
// CONSTRAINT ENFORCEMENT:
#define IDC_CHECK_DELETE_RULE 1300
#define IDC_CHECK_UPDATE_RULE 1301
#define IDC_COMBO_DELETE_RULE 1302
#define IDC_COMBO_UPDATE_RULE 1303
#define IDC_CONSTRAINT_INDEX  1304

#define IDC_CHECK_PAGE_SIZE                    1219
#define IDC_COMBO_PAGESIZE                     1220

//
// OPTIMIZEDB DIALOG
#define IDC_SHOW_ADVANCED		       1222

//
// Roll Forward Dialog
#define IDC_ROLLFWD_TAPE		       1223

// More Options button
#ifndef IDC_MORE_OPTIONS
#define IDC_MORE_OPTIONS			1224
#endif

#endif// DLGRES_HEADER
