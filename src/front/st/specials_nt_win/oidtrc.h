#ifndef OIDTRC_HEADER
#define OIDTRC_HEADER

//
// IDD_ must start from 5100 to 5200

#define IDD_OIDTFOREIGNKEY              5100
#define IDD_OIDTCREATEINDEX             5101
#define IDD_OIDTGTRANTTABLE             5102
#define IDD_OIDTGRANTCONNECTTO          5103
#define IDD_OIDTGRANTDBARESOURCE        5104
#define IDD_OIDTTRIGGER                 5105
#define IDD_OIDTEDIT                    5106
#define IDD_OIDTTRIGGERREF              5107
#define IDD_OIDTINDEXFUNCTION           5108
#define IDD_OIDTGRANTEXECUTE            5109
#define IDD_OIDTCREATETABLE             5110
#define IDD_OIDTREVOKETABLE             5111

#define IDD_OIDTPROC_SUBADD             5112  // Procedure parameter or declare section
#define IDD_OIDTEVENT                   5113  // Event
#define IDD_OIDTDBAREA                  5114  // Dbarea
#define IDD_OIDTSTOGROUP                5115  // Stogroup
#define IDD_OIDTDATABASE                5116  // Database
#define IDD_OIDT_TSTAT                  5117  // Table Statistics
#define IDD_OIDT_ISTAT                  5118  // Index Statistics
#define IDD_OIDT_ISTAT2                 5119  // Index Statistics Subbox
#define IDD_OIDT_UNLOAD                 5120  // Database Unload
#define IDD_OIDT_LOAD                   5121  // Database Load
#define IDD_OIDT_DOWNLOAD               5122  // IICOPY - Download box
#define IDD_OIDT_RUNSCRIPTS             5123  // IICOPY - Run Scripts box
#define IDD_OIDT_VIEWLOG                5124  // IICOPY - View Log
#define IDD_OIDT_EDITOR                 5125  // IICOPY - Log View External Editor Configuration
#define IDD_OIDT_DOMFILTER              5126  // Filter box for dom on desktop node
#define IDD_OIDT_DOWNLOAD_COLUMNS       5127  // IICOPY - Specify columns box

// IMPORTANT NOTE: see dlgres.h, lines 169 and following,
// for defines specific for context help in dialog boxes

#define IDC_COMBOCONSTRAINTNAME         1001
#define IDC_EDITREFERENCING             1002
#define IDC_LISTREFERENCINGCOLUMN       1003
#define IDC_COMBOREFERENCED             1004
#define IDC_EDITREFERENCEDPKEY          1005
#define IDC_BUTTONUP                    1006
#define IDC_BUTTONDOWN                  1007
#define IDC_LISTCOLUMNS                 1011
#define IDC_COMBODELETERULE             1013
#define IDC_EDITINDEXNAME               1014
#define IDC_CHECKUNIQUE                 1015
#define IDC_CHECKCLUSTER                1016
#define IDC_EDITPCTFREE                 1018
#define IDC_EDITSIZE                    1019
#define IDC_COMBOROWBUCKET              1020
#define IDC_RADIOUSERS                  1023
#define IDC_RADIOPUBLIC                 1024
#define IDC_LISTUSERS                   1025
#define IDC_CHECKSELECT                 1026
#define IDC_CHECKINSERT                 1027
#define IDC_CHECKDELETE                 1028
#define IDC_CHECKINDEX                  1029
#define IDC_CHECKALTER                  1030
#define IDC_CHECKUPDATE                 1031
#define IDC_STRUCTURE                   1032
#define IDC_CHECKPARTIALCOLUMNS         1035
#define IDC_EDITUSERNAME                1037
#define IDC_EDITPASSWORD                1038
#define IDC_EDITCPASSWORD               1039
#define IDC_EDITTRIGGERNAME             1041
#define IDC_RADIOBEFORE                 1042
#define IDC_RADIOAFTER                  1043
#define IDC_RADIOUPDATE                 1044
#define IDC_RADIODELETE                 1045
#define IDC_RADIOINSERT                 1046
#define IDC_RADIOOLD                    1049
#define IDC_RADIONEW                    1050
#define IDC_RADIOPROCEDURE              1064
#define IDC_RADIOINLINE                 1065
#define IDC_COMBOPROCEDURES             1066
#define IDC_EDITPARAMETER               1067
#define IDC_EDITINLINETEXT              1068
#define IDC_BUTTONTEXT                  1069
#define IDC_STATICOVTN                  1072
#define IDC_STATICNVTN                  1073
#define IDC_STATIC2NVTN                 1074
#define IDC_STATIC2OVTN                 1075
#define IDC_STATICPARAMETERS            1076
#define IDC_EDITTEXT                    1079
#define IDC_STATICTABLE                 1081
#define IDC_LISTTABLES                  1083
#define IDC_LISTPARTIALCOLUMNS          1084
#define IDC_CHECKREFERENCING            1085
#define IDC_BUTTONREFERENCING           1086
#define IDC_RADIONONE                   1088
#define IDC_RADIOSTATEMENT              1089
#define IDC_RADIOROW                    1090
#define IDC_EDITOVTN                    1091
#define IDC_EDIT2NVTN                   1092
#define IDC_EDITNVTN                    1093
#define IDC_TABLE                       1094
#define IDC_EDIT2OVTN                   1094
#define IDC_STATICSIZE                  1096
#define IDC_COMBOFUNCTION               1097
#define IDC_EDITARG1                    1098
#define IDC_EDITARG2                    1099
#define IDC_STATICFARG1                 1102
#define IDC_STATICFARG2                 1103
#define IDC_LISTPROCEDURES              1107
#define IDC_COMBOPRIV                   1110
#define IDC_STATICPRIVILEGE             1111
#define IDC_STATIC_NAME                 1112
#define IDC_STATIC_COLUMNS              1113
#define IDC_BUTTON_ASSISTANT            1114
#define IDC_CHECKASSELECT               1115
#define IDC_EDITCOLUMNHEADER            1116
#define IDC_EDITSQL                     1117
#define IDC_STATIC_COLHEADER            1118
#define IDC_STATIC_SQL                  1119
#define IDC_CONTAINER                   1123
#define IDC_ADD                         1132
#define IDC_COLNAME                     1160
#define IDC_DROP                        1161
#define IDC_REFERENCES                  1169

#ifdef MAINLIB
#define IDB_OIDTBITMAPLIST              1170
#define IDB_CACHKLISTBOX                1171
#define IDB_CACHKNT                     1172
#define IDB_OIDTCNTBITMAP               1173
#else   // MAINLIB
#define IDB_OIDTBITMAPLIST              30995
#define IDB_CACHKLISTBOX                30996
#define IDB_CACHKNT                     30997
#define IDB_OIDTCNTBITMAP               30998
#endif  // MAINLIB




//
// EMB Control IDs Section (start)
//
// Emb : for the IDD_OIDTPROC_SUBADD box
#define IDC_OIDTPROC_SUBADD_NAME        1001
#define IDC_OIDTPROC_SUBADD_TYPE        1002
#define IDC_OIDTPROC_SUBADD_INPUT       1003
#define IDC_OIDTPROC_SUBADD_INOUT       1004

// Emb : for the IDD_OIDTEVENT box
#define IDC_OIDTEVENT_RAISEAT_EDIT      1070
#define IDC_OIDTEVENT_EVERY_EDIT        1071
#define IDC_OIDTEVENT_NAME              1080
#define IDC_OIDTEVENT_RAISEAT           1081
#define IDC_OIDTEVENT_EVERY             1082
#define IDC_OIDTEVENT_EVERY_COMBO       1083

// Emb : for the IDD_OIDTDBAREA box
#define IDC_OIDTDBAREA_NAME             1084
#define IDC_OIDTDBAREA_FILE             1085
#define IDC_OIDTDBAREA_SIZE             1086

// Emb : for the IDD_OIDTSTOGROUP box
#define IDC_OIDTSTOGROUP_NAME           1087
#define IDC_OIDTSTOGROUP_DBAREAS        1088

// Emb : for the IDD_OIDTDATABASE box
#define IDC_OIDTDATABASE_NAME           1088
#define IDC_OIDTDATABASE_IN             1089
#define IDC_OIDTDATABASE_LOG            1090
#define IDC_CHECK_FRONTEND              1091

//Emb : for the IDD_OIDT_TSTAT box
#define IDC_OIDT_TSTAT2_ROWCK           1093
#define IDC_OIDT_TSTAT2_ROWVAL          1094
#define IDC_OIDT_TSTAT2_PGCK            1095
#define IDC_OIDT_TSTAT2_PGVAL           1096
#define IDC_OIDT_TSTAT2_ROWPGCK         1097
#define IDC_OIDT_TSTAT2_ROWPGVAL        1098
#define IDC_OIDT_TSTAT2_LONGPGCK        1099
#define IDC_OIDT_TSTAT2_LONGPGVAL       1100

//Emb : for the IDD_OIDT_ISTAT box
#define IDC_OIDT_ISTAT_HCK              1101
#define IDC_OIDT_ISTAT_HVAL             1102
#define IDC_OIDT_ISTAT_LEAFCK           1103
#define IDC_OIDT_ISTAT_LEAFVAL          1104
#define IDC_OIDT_ISTAT_CLUSTCK          1105
#define IDC_OIDT_ISTAT_CLUSTVAL         1106
#define IDC_OIDT_ISTAT_PRIMCK           1107
#define IDC_OIDT_ISTAT_OVFLCK           1108
#define IDC_OIDT_ISTAT_OVFLVAL          1109
#define IDC_OIDT_ISTAT_DISTLIST         1110
#define IDC_OIDT_ISTAT_DISTADD          1111
#define IDC_OIDT_ISTAT_IDXCK            1112
#define IDC_OIDT_ISTAT_IDXVAL           1113
#define IDC_OIDT_ISTAT_DISTMOD          1114
#define IDC_OIDT_ISTAT_PRIMVAL          1115
#define IDC_OIDT_ISTAT_DISTDEL          1116

//Emb : for the IDD_OIDT_ISTAT2 box
#define IDC_OIDT_ISTAT2_COLUMNS         1091
#define IDC_OIDT_ISTAT2_VAL             1092

//Emb : for the IDD_OIDT_UNLOAD box
#define IDC_OIDT_UNLOAD_DB              1118
#define IDC_OIDT_UNLOAD_DDL             1119
#define IDC_OIDT_UNLOAD_ALLDATA         1120
#define IDC_OIDT_UNLOAD_TBD             1121
#define IDC_OIDT_UNLOAD_TBLDATA         1121
#define IDC_OIDT_UNLOAD_TBL             1122
//#define IDC_CONTAINER                   1123
#define IDC_OIDT_UNLOAD_SQL             1123
#define IDC_OIDT_UNLOAD_ASCII           1124
#define IDC_OIDT_UNLOAD_DIF             1125
#define IDC_OIDT_UNLOAD_FILE            1126
#define IDC_OIDT_UNLOAD_CTRLFILE        1127
#define IDC_OIDT_UNLOAD_FILENAME        1129
#define IDC_OIDT_UNLOAD_LOGNAME         1130
#define IDC_OIDT_UNLOAD_OVER            1131
#define IDC_OIDT_UNLOAD_COMPRESS        1132
#define IDC_OIDT_UNLOAD_SINGLETBL       1133
#define IDC_OIDT_UNLOAD_MULTTBL         1134
#define IDC_OIDT_UNLOAD_ALL             1135

//Emb : for the IDD_OIDT_LOAD box
#define IDC_OIDT_LOAD_DIF               1128
#define IDC_OIDT_LOAD_LOGNAME           1131
#define IDC_OIDT_LOAD_SINGLETBL         1134
#define IDC_OIDT_LOAD_FILE              1135
#define IDC_OIDT_LOAD_CTRLFILE          1136
#define IDC_OIDT_LOAD_FILENAME          1137
#define IDC_OIDT_LOAD_SQL               1138
#define IDC_OIDT_LOAD_SQLCOMP           1139
#define IDC_OIDT_LOAD_ASCII             1140
#define IDC_OIDT_LOAD_STARTAT           1143

//Emb : for the IDD_OIDT_DOWNLOAD box
#define IDC_OIDT_DOWNLOAD_CBNODE             1002
#define IDC_OIDT_DOWNLOAD_RADALL             1003
#define IDC_OIDT_DOWNLOAD_CBDATABASE         1004
#define IDC_OIDT_DOWNLOAD_RADSEL             1005
#define IDC_OIDT_DOWNLOAD_LISTSEL            1006
#define IDC_OIDT_DOWNLOAD_ACT_ONESTEP        1007
#define IDC_OIDT_DOWNLOAD_ACT_TWOSTEPS       1008
#define IDC_OIDT_DOWNLOAD_ACT_DOWN           1009
#define IDC_OIDT_DOWNLOAD_PATH               1010
#define IDC_OIDT_DOWNLOAD_SCRIPTS_GENCOPY    1011
#define IDC_OIDT_DOWNLOAD_SCRIPTS_EXECCOPY   1012
#define IDC_OIDT_DOWNLOAD_SCRIPTS_KEEPSCHEMA 1013
#define IDC_OIDT_DOWNLOAD_BNPATH             1014
#define IDC_OIDT_DOWNLOAD_BNNODE             1015

//Emb : for the IDD_OIDT_RUNSCRIPTS box
#define IDC_OIDT_RUNSCRIPTS_PATH        1023
#define IDC_OIDT_RUNSCRIPTS_SCRIPTS_ALL 1024
#define IDC_OIDT_RUNSCRIPTS_SCRIPTS_CREATE 1025
#define IDC_OIDT_RUNSCRIPTS_BNPATH      1026
#define IDC_OIDT_RUNSCRIPTS_SCRIPTS_CHECKEXIST 1027
#define IDC_OIDT_RUNSCRIPTS_LBSCRIPT    1029

//Emb : for the IDD_OIDT_VIEWLOG box
#define IDC_OIDT_VIEWLOG_LOG            1001
#define IDC_OIDT_VIEWLOG_EDITOR         1002
#define IDC_OIDT_VIEWLOG_CONFIG         1003

// Emb : for the IDD_OIDT_EDITOR  box
#define IDC_OIDT_EDITOR_CMDLINE         1001


// Emb : for the IDD_OIDT_DOMFILTER box
// we use IDD_DOMFILTER_OTHER to have common dlg proc with ingres node

// Emb 23/07/97: added thanks to resource editor
// for 3 steps plus specify columns in IICOPY
#define IDC_OIDT_DOWNLOAD_SCRIPTS_CONVERT 1014
#define IDC_OIDT_DOWNLOAD_BNCOLUMNS     1016
#define IDC_OIDT_DOWNLOAD_CHKSELCOLUMNS 1017
#define IDC_OIDT_DOWNLOAD_OPT_RECOVOFF  1169
#define IDC_OIDT_DOWNLOAD_OPT_PAUSE     1170
#define IDC_OIDT_DOWNLOAD_ACT_CPOUT     1171
#define IDC_OIDT_DOWNLOAD_ACT_CPIN      1172
#define IDC_OIDT_RUNSCRIPTS_OPT_PAUSE   1173
#define IDC_OIDT_RUNSCRIPTS_ACT_CPIN    1174
#define IDC_OIDT_RUNSCRIPTS_ACT_CPOUT_CREATE 1175
#define IDC_OIDT_RUNSCRIPTS_OPT_RECOVOFF 1176
#define IDC_OIDT_RUNSCRIPTS_ACT_CPOUT_CPY 1177
#define IDC_OIDT_DOWNLOAD_LISTCOL       1177

//
// EMB Control IDs Section (end)
//







//
// For the string table range [35001, 36000]
//

#define IDS_OIDTTYPE_CHAR               35001    
#define IDS_OIDTTYPE_VARCHAR            35002
#define IDS_OIDTTYPE_DECIMAL            35003
#define IDS_OIDTTYPE_FLOAT              35004
#define IDS_OIDTTYPE_INTEGER            35005
#define IDS_OIDTTYPE_LONGVARCHAR        35006
#define IDS_OIDTTYPE_NUMBER             35007
#define IDS_OIDTTYPE_SMALLINT           35008
#define IDS_OIDTTYPE_DATE               35009
#define IDS_OIDTTYPE_DATETIME           35010
#define IDS_OIDTTYPE_TIME               35011
#define IDS_OIDTTYPE_TIMESTAMP          35012
#define IDS_OIDTNODELETERULE            35013
#define IDS_OIDTRESTRICT                35014
#define IDS_OIDTCASCADE                 35015
#define IDS_OIDTSETNULL                 35016
#define IDS_OIDTVIEWS                   35017
#define IDS_OIDTTABLES                  35018
#define IDS_OIDTGTABLETITLE             35019
#define IDS_OIDTGVIEWTITLE              35020
#define IDS_OIDTGDBATITLE               35021
#define IDS_OIDTGRESOURCETITLE          35022
#define IDS_OIDTGCONNECTTOTITLE         35023
#define IDS_OIDTCREATETRIGGERTITLE      35024
#define IDS_OIDTCREATETABLETITLE        35025
#define IDS_OIDTCREATEINDEXTITLE        35026
#define IDS_OIDTALTERTABLETITLE         35027
#define IDS_OIDTCREATETABLEFAILED       35028
#define IDS_OIDTALTERTABLEFAILED        35029
#define IDS_OIDTCREATEINDEXFAILED       35030
#define IDS_OIDTCREATETRIGGERFAILED     35031
#define IDS_OIDTGRANTTABLEFAILED        35032
#define IDS_OIDTGRANTRESOURCEFAILED     35033
#define IDS_OIDTGRANTDBAFAILED          35034
#define IDS_OIDTGRANTCONNECTTOFAILED    35035
#define IDS_OIDTGRANTVIEWFAILED         35036
#define IDS_OIDTSAMETABLE               35037
#define IDS_OIDTSELECT                  35038
#define IDS_OIDTINSERT                  35039
#define IDS_OIDTDELETE                  35040
#define IDS_OIDTINDEX                   35041
#define IDS_OIDTALTER                   35042
#define IDS_OIDTUPDATE                  35043
#define IDS_OIDTREVOKETABLE             35044
#define IDS_OIDTREVOKEVIEW              35045
#define IDS_OIDTREVOKEDBAUTH            35046
#define IDS_OIDTREVOKEPRIVILEGE         35047
#define IDS_OIDTREVOKEDBAFAILED         35048
#define IDS_OIDTREVOKERESOURCEFAILED    35049
#define IDS_OIDTREVOKECONNECTFAILED     35050
#define IDS_OIDTREVOKETABLEFAILED       35051
#define IDS_OIDTREVOKEVIEWFAILED        35052
#define IDS_OIDTPROPNAME                35053
#define IDS_OIDTPROPCREATOR             35054
#define IDS_OIDTPROPNUMROWS             35055
#define IDS_OIDTPROPNUMCOLS             35056
#define IDS_OIDTPROPPRIMARY             35057
#define IDS_OIDTPROPFOREIGNKEY          35058
#define IDS_OIDTPROPCOLS                35059
#define IDS_OIDTPROPNULLABLE            35060
#define IDS_OIDTPROPREFON               35061
#define IDS_OIDTSCALE                   35062
#define IDS_OIDTLENGTH                  35063
#define IDS_OIDTSTARTPOS                35064
#define IDS_OIDTPROPONTABLE             35065
#define IDS_OIDTPROPACTION              35066
#define IDS_OIDTPROPFREQUENCY           35067
#define IDS_OIDTPROPEACHROW             35068
#define IDS_OIDTPROPEACHSTATEMENT       35069
#define IDS_OIDTPROPOLDVALUE            35070
#define IDS_OIDTPROPNEWVALUE            35071
#define IDS_OIDTPROPEXECUTE             35072
#define IDS_OIDTPROPTRIGTITLE           35073
#define IDS_OIDTGRANTEXECUTETITLE       35074
#define IDS_OIDTREVOKEEXECUTETITLE      35075
#define IDS_OIDTGRANTEXECFAILED         35076
#define IDS_OIDTREVOKEEXECFAILED        35077
#define IDS_OIDTREVOKEEXEC              35078



// Emb : for the IDD_OIDTPROC_SUBADD box
#define IDS_OIDT_PROCTYPE_BOOLEAN       35100
#define IDS_OIDT_PROCTYPE_DATETIME      35101
#define IDS_OIDT_PROCTYPE_NUMBER        35102
#define IDS_OIDT_PROCTYPE_STRING        35103
#define IDS_OIDT_PROCTYPE_SQLHANDLE     35104
                                             
//Emb : for the IDD_OIDTEVENT
#define IDS_OIDTEVENT_CREATECAPT        35105
#define IDS_OIDTEVENT_ALTERCAPT         35106
#define IDS_OIDTEVENT_SECOND            35107
#define IDS_OIDTEVENT_MINUTE            35108
#define IDS_OIDTEVENT_HOUR              35109
#define IDS_OIDTEVENT_DAY               35110

//Emb : for the IDD_OIDTDBAREA box
#define IDS_OIDTDBAREA_CREATECAPT       35111
#define IDS_OIDTDBAREA_ALTERCAPT        35112
#define IDS_E_CREATE_DBAREA_FAILED      35113
#define IDS_E_ALTER_DBAREA_FAILED       35114

//Emb : for the IDD_OIDTSTOGROUP box
#define IDS_OIDTSTOGROUP_CREATECAPT     35115
#define IDS_OIDTSTOGROUP_ALTERCAPT      35116
#define IDS_E_CREATE_STOGROUP_FAILED    35117
#define IDS_E_ALTER_STOGROUP_FAILED     35118

//Emb : for the IDD_OIDTDATABASE box
#define IDS_OIDTDATABASE_CREATECAPT       35119
#define IDS_OIDTDATABASE_ALTERCAPT        35120
#define IDS_E_CREATE_OIDTDATABASE_FAILED  35121
#define IDS_E_ALTER_OIDTDATABASE_FAILED   35122

#endif

