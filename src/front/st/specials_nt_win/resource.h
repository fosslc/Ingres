/********************************************************************
//
//  Copyright (C) 1995 Ingres Corporation
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : resource.h
//    resources definition
//
********************************************************************/

#ifndef __RESOURCE_INCLUDED__
#define __RESOURCE_INCLUDED__

//
//  User defined resource ids
//
#define MENUSELECT_RESOURCETYPE   100
#define MENUSELECT_DOM_DOC        201

//
// Constants for icons, bitmaps, menus, ...
//  --- Makeintresource must be used in the code ---
//
// Convention on values:
//  ICONS                  : start from 1000
//  BITMAPS                : start from 2000
//  CURSORS                : start from 3000
//  MENUS and ACCELERATORS : start from 4000
//  DIALOGS : rather complicated:
//            - boxes already defined with appstudio by rich williams
//              start from low value (around 100) in include\dlgres.h
//            - starting from March 9, 95:
//              - Rich W. should continue using appstudio autonumbers,
//                assumed that they remain below 5000, in include\dlgres.h
//              - Vut Uk should start from 5001, in include\dlgres.h
//              - Emmanuel B. should start from 5501, in winmain\resource.h
//            - Moreover, any value can be used for the dialog boxes
//              controls ids without taking care of values overlap,
//              since the controls ids are related to a given dlg box.
//
//  USER-DEFINED : start from 6000
//

//
// Configuration window
//
#define IDR_CFG_WINDOW        6000
#define CFG_WINDOW           300

//
// ICONS
//
#define IDI_FRAME                 1001
#define IDI_DOC_DOM               1002
#define IDI_DOC_SQLACT            1003
#define ID_ICON_DOC_SQLACT        IDI_DOC_SQLACT
#define IDI_DOC_PROPERTY          1004

// tree statics of level 0
#define IDI_TREE_STATIC_DATABASE  1011
#define IDI_TREE_STATIC_PROFILE   1012
#define IDI_TREE_STATIC_USER      1013
#define IDI_TREE_STATIC_GROUP     1014
#define IDI_TREE_STATIC_ROLE      1015
#define IDI_TREE_STATIC_LOCATION  1016
#define IDI_TREE_STATIC_SYNONYMED 1017
#define IDI_TREE_STATIC_DBAREA    1018
#define IDI_TREE_STATIC_STOGROUP  1019

// tree statics of level 1, children of database
#define IDI_TREE_STATIC_TABLE       1101
#define IDI_TREE_STATIC_VIEW        1102
#define IDI_TREE_STATIC_PROCEDURE   1103
#define IDI_TREE_STATIC_SCHEMA      1104
#define IDI_TREE_STATIC_SYNONYM     1105
#define IDI_TREE_STATIC_DBEVENT     1106
// continued after other level 1 not to resequence constants numbers

// tree icons of level 1, children of user
#define IDI_TREE_STATIC_R_USERSCHEMA 1107
#define IDI_TREE_STATIC_R_USERGROUP  1108

#define IDI_TREE_STATIC_R_SECURITY        1109
#define IDI_TREE_STATIC_R_SEC_SEL_SUCC    1111
#define IDI_TREE_STATIC_R_SEC_SEL_FAIL    1112
#define IDI_TREE_STATIC_R_SEC_DEL_SUCC    1114
#define IDI_TREE_STATIC_R_SEC_DEL_FAIL    1115
#define IDI_TREE_STATIC_R_SEC_INS_SUCC    1117
#define IDI_TREE_STATIC_R_SEC_INS_FAIL    1118
#define IDI_TREE_STATIC_R_SEC_UPD_SUCC    1120
#define IDI_TREE_STATIC_R_SEC_UPD_FAIL    1121

#define IDI_TREE_STATIC_R_GRANT             1122  // rights granted
#define IDI_TREE_STATIC_R_DBEGRANT          1123  // grants for dbevents
#define IDI_TREE_STATIC_R_DBEGRANT_RAISE    1124
#define IDI_TREE_STATIC_R_DBEGRANT_REGISTER 1125
#define IDI_TREE_STATIC_R_PROCGRANT         1126  // grants for procedures
#define IDI_TREE_STATIC_R_PROCGRANT_EXEC    1127
#define IDI_TREE_STATIC_R_DBGRANT           1128  // grants for databases
#define IDI_TREE_STATIC_R_DBGRANT_ACCESY    1129
#define IDI_TREE_STATIC_R_DBGRANT_ACCESN    1130
#define IDI_TREE_STATIC_R_DBGRANT_CREPRY    1131
#define IDI_TREE_STATIC_R_DBGRANT_CREPRN    1132
#define IDI_TREE_STATIC_R_DBGRANT_CRETBY    1133
#define IDI_TREE_STATIC_R_DBGRANT_CRETBN    1134
#define IDI_TREE_STATIC_R_DBGRANT_DBADMY    1135
#define IDI_TREE_STATIC_R_DBGRANT_DBADMN    1136
#define IDI_TREE_STATIC_R_DBGRANT_LKMODY    1137
#define IDI_TREE_STATIC_R_DBGRANT_LKMODN    1138
#define IDI_TREE_STATIC_R_DBGRANT_QRYIOY    1139
#define IDI_TREE_STATIC_R_DBGRANT_QRYION    1140
#define IDI_TREE_STATIC_R_DBGRANT_QRYRWY    1141
#define IDI_TREE_STATIC_R_DBGRANT_QRYRWN    1142
#define IDI_TREE_STATIC_R_DBGRANT_UPDSCY    1143
#define IDI_TREE_STATIC_R_DBGRANT_UPDSCN    1144

#define IDI_TREE_STATIC_R_DBGRANT_SELSCY    1145
#define IDI_TREE_STATIC_R_DBGRANT_SELSCN    1146
#define IDI_TREE_STATIC_R_DBGRANT_CNCTLY    1147
#define IDI_TREE_STATIC_R_DBGRANT_CNCTLN    1148
#define IDI_TREE_STATIC_R_DBGRANT_IDLTLY    1149
#define IDI_TREE_STATIC_R_DBGRANT_IDLTLN    1150
#define IDI_TREE_STATIC_R_DBGRANT_SESPRY    1151
#define IDI_TREE_STATIC_R_DBGRANT_SESPRN    1152
#define IDI_TREE_STATIC_R_DBGRANT_TBLSTY    1153
#define IDI_TREE_STATIC_R_DBGRANT_TBLSTN    1154

#define IDI_TREE_STATIC_R_TABLEGRANT        1155  // grants for tables
#define IDI_TREE_STATIC_R_TABLEGRANT_SEL    1156
#define IDI_TREE_STATIC_R_TABLEGRANT_INS    1157
#define IDI_TREE_STATIC_R_TABLEGRANT_UPD    1158
#define IDI_TREE_STATIC_R_TABLEGRANT_DEL    1159
#define IDI_TREE_STATIC_R_TABLEGRANT_REF    1160

#define IDI_TREE_STATIC_R_TABLEGRANT_CPI    1161
#define IDI_TREE_STATIC_R_TABLEGRANT_CPF    1162

#define IDI_TREE_STATIC_R_TABLEGRANT_ALL    1163
#define IDI_TREE_STATIC_R_VIEWGRANT         1164  // grants for views
#define IDI_TREE_STATIC_R_VIEWGRANT_SEL     1165
#define IDI_TREE_STATIC_R_VIEWGRANT_INS     1166
#define IDI_TREE_STATIC_R_VIEWGRANT_UPD     1167
#define IDI_TREE_STATIC_R_VIEWGRANT_DEL     1168
#define IDI_TREE_STATIC_R_ROLEGRANT         1169  // grants for roles

// tree statics of level 1, children of groups
#define IDI_TREE_STATIC_GROUPUSER           1170

// tree statics of level 1, children of role
#define IDI_TREE_STATIC_ROLEGRANT_USER      1171

// tree statics of level 1, children of location
#define IDI_TREE_STATIC_R_LOCATIONTABLE     1172

// continuation of level 1, children of database
// (in order not to resequence constants numbers of previous levels)
// kept 10 of margin
#define IDI_TREE_STATIC_DBGRANTEES          1173
#define IDI_TREE_STATIC_DBGRANTEES_ACCESY   1174
#define IDI_TREE_STATIC_DBGRANTEES_ACCESN   1175
#define IDI_TREE_STATIC_DBGRANTEES_CREPRY   1176
#define IDI_TREE_STATIC_DBGRANTEES_CREPRN   1177
#define IDI_TREE_STATIC_DBGRANTEES_CRETBY   1178
#define IDI_TREE_STATIC_DBGRANTEES_CRETBN   1179
#define IDI_TREE_STATIC_DBGRANTEES_DBADMY   1180
#define IDI_TREE_STATIC_DBGRANTEES_DBADMN   1181
#define IDI_TREE_STATIC_DBGRANTEES_LKMODY   1182
#define IDI_TREE_STATIC_DBGRANTEES_LKMODN   1183
#define IDI_TREE_STATIC_DBGRANTEES_QRYIOY   1184
#define IDI_TREE_STATIC_DBGRANTEES_QRYION   1185
#define IDI_TREE_STATIC_DBGRANTEES_QRYRWY   1186
#define IDI_TREE_STATIC_DBGRANTEES_QRYRWN   1187
#define IDI_TREE_STATIC_DBGRANTEES_UPDSCY   1188
#define IDI_TREE_STATIC_DBGRANTEES_UPDSCN   1189

#define IDI_TREE_STATIC_DBGRANTEES_SELSCY   1190
#define IDI_TREE_STATIC_DBGRANTEES_SELSCN   1191
#define IDI_TREE_STATIC_DBGRANTEES_CNCTLY   1192
#define IDI_TREE_STATIC_DBGRANTEES_CNCTLN   1193
#define IDI_TREE_STATIC_DBGRANTEES_IDLTLY   1194
#define IDI_TREE_STATIC_DBGRANTEES_IDLTLN   1195
#define IDI_TREE_STATIC_DBGRANTEES_SESPRY   1196
#define IDI_TREE_STATIC_DBGRANTEES_SESPRN   1197
#define IDI_TREE_STATIC_DBGRANTEES_TBLSTY   1198
#define IDI_TREE_STATIC_DBGRANTEES_TBLSTN   1199

#define IDI_TREE_STATIC_DBALARM                  1200
#define IDI_TREE_STATIC_DBALARM_CONN_SUCCESS     1201
#define IDI_TREE_STATIC_DBALARM_CONN_FAILURE     1202
#define IDI_TREE_STATIC_DBALARM_DISCONN_SUCCESS  1203
#define IDI_TREE_STATIC_DBALARM_DISCONN_FAILURE  1204

// tree icons for classes
#define IDI_TREE_STATIC_CLASSES                  1205
#define IDI_TREE_STATIC_SUBCLASSES               1206

// tree icons of level 1, children of database/replicator
#define IDI_TREE_STATIC_REPLICATOR                1207
#define IDI_TREE_STATIC_REPLIC_CONNECTION         1208
#define IDI_TREE_STATIC_REPLIC_CDDS               1209
#define IDI_TREE_STATIC_REPLIC_MAILUSER           1210
#define IDI_TREE_STATIC_REPLIC_REGTABLE           1211
#define IDI_TREE_STATIC_REPLIC_SERVER             1212

// tree statics of level 2, children of "table of database"
#define IDI_TREE_STATIC_INTEGRITY                 1213
#define IDI_TREE_STATIC_RULE                      1214
#define IDI_TREE_STATIC_INDEX                     1215
#define IDI_TREE_STATIC_TABLELOCATION             1216
#define IDI_TREE_STATIC_R_REPLIC_TABLE_CDDS       1217


#define IDI_TREE_STATIC_SECURITY          1218
#define IDI_TREE_STATIC_SEC_SEL_SUCC      1219
#define IDI_TREE_STATIC_SEC_SEL_FAIL      1220
#define IDI_TREE_STATIC_SEC_DEL_SUCC      1221
#define IDI_TREE_STATIC_SEC_DEL_FAIL      1222
#define IDI_TREE_STATIC_SEC_INS_SUCC      1223
#define IDI_TREE_STATIC_SEC_INS_FAIL      1224
#define IDI_TREE_STATIC_SEC_UPD_SUCC      1225
#define IDI_TREE_STATIC_SEC_UPD_FAIL      1226

#define IDI_TREE_STATIC_R_TABLESYNONYM    1227
#define IDI_TREE_STATIC_R_TABLEVIEW       1228

#define IDI_TREE_STATIC_TABLEGRANTEES         1231
#define IDI_TREE_STATIC_TABLEGRANT_SEL_USER   1232
#define IDI_TREE_STATIC_TABLEGRANT_INS_USER   1233
#define IDI_TREE_STATIC_TABLEGRANT_UPD_USER   1234
#define IDI_TREE_STATIC_TABLEGRANT_DEL_USER   1235
#define IDI_TREE_STATIC_TABLEGRANT_REF_USER   1236

#define IDI_TREE_STATIC_TABLEGRANT_CPI_USER   1237
#define IDI_TREE_STATIC_TABLEGRANT_CPF_USER   1238

#define IDI_TREE_STATIC_TABLEGRANT_ALL_USER   1239

// desktop
#define IDI_TREE_STATIC_TABLEGRANT_INDEX_USER 1240
#define IDI_TREE_STATIC_TABLEGRANT_ALTER_USER 1241


#define IDI_TREE_STATIC_R_DBGRANT_QRYCPY    1251
#define IDI_TREE_STATIC_R_DBGRANT_QRYCPN    1252
#define IDI_TREE_STATIC_R_DBGRANT_QRYPGY    1253
#define IDI_TREE_STATIC_R_DBGRANT_QRYPGN    1254
#define IDI_TREE_STATIC_R_DBGRANT_QRYCOY    1255
#define IDI_TREE_STATIC_R_DBGRANT_QRYCON    1256

#define IDI_TREE_STATIC_DBGRANTEES_QRYCPY   1261
#define IDI_TREE_STATIC_DBGRANTEES_QRYCPN   1262
#define IDI_TREE_STATIC_DBGRANTEES_QRYPGY   1263
#define IDI_TREE_STATIC_DBGRANTEES_QRYPGN   1264
#define IDI_TREE_STATIC_DBGRANTEES_QRYCOY   1265
#define IDI_TREE_STATIC_DBGRANTEES_QRYCON   1266


// start from 1300 for next levels

// tree statics of level 2, children of "view of database"
#define IDI_TREE_STATIC_VIEWTABLE           1327
#define IDI_TREE_STATIC_R_VIEWSYNONYM       1328

#define IDI_TREE_STATIC_VIEWGRANTEES        1329
#define IDI_TREE_STATIC_VIEWGRANT_SEL_USER  1330
#define IDI_TREE_STATIC_VIEWGRANT_INS_USER  1331
#define IDI_TREE_STATIC_VIEWGRANT_UPD_USER  1332
#define IDI_TREE_STATIC_VIEWGRANT_DEL_USER  1333

// tree statics of level 2, child of "procedure of database"
#define IDI_TREE_STATIC_PROCGRANT_EXEC_USER 1334
#define IDI_TREE_STATIC_R_PROC_RULE         1335

// tree statics of level 2, child of "dbevent of database"
#define IDI_TREE_STATIC_DBEGRANT_RAISE_USER 1336
#define IDI_TREE_STATIC_DBEGRANT_REGTR_USER 1337

// tree icons of level 2, children of replicator cdds
#define IDI_TREE_STATIC_REPLIC_CDDS_DETAIL  1338
#define IDI_TREE_STATIC_R_REPLIC_CDDS_TABLE 1339

// tree statics of level 3, child of "index on table of database"
#define IDI_TREE_STATIC_R_INDEXSYNONYM      1340

// tree statics of level 3, child of "rule on table of database"
#define IDI_TREE_STATIC_RULEPROC            1341

// new style alarms (with 2 sub-branches alarmee and launched dbevent)
#define IDI_TREE_STATIC_ALARMEE             1342
#define IDI_TREE_STATIC_ALARM_EVENT         1343

//
//  Star management : customized icons for objects database/table/view
//
#define IDI_TREE_STAR_DB_DDB                1344
#define IDI_TREE_STAR_DB_CDB                1345
#define IDI_TREE_STAR_TBL_NATIVE            1346
#define IDI_TREE_STAR_TBL_LINK              1347
#define IDI_TREE_STAR_VIEW_NATIVE           1348
#define IDI_TREE_STAR_VIEW_LINK             1349
#define IDI_TREE_STAR_PROCEDURE             1350
#define IDI_TREE_STAR_INDEX                 1351

//
// Star management: CDB as a sub branch for a DDB
//
#define IDI_TREE_STATIC_R_CDB               1352

// table/view/procedure as subbranches of schema
#define IDI_TREE_STATIC_SCHEMAUSER_TABLE      1353
#define IDI_TREE_STATIC_SCHEMAUSER_VIEW       1354
#define IDI_TREE_STATIC_SCHEMAUSER_PROCEDURE  1355

//
// ICE
//
#define IDI_TREE_STATIC_ICE                      1356
// Under "Security"
#define IDI_TREE_STATIC_ICE_SECURITY             1357
#define IDI_TREE_STATIC_ICE_DBUSER               1358
#define IDI_TREE_STATIC_ICE_DBCONNECTION         1359
#define IDI_TREE_STATIC_ICE_WEBUSER              1360
#define IDI_TREE_STATIC_ICE_WEBUSER_ROLE         1361
#define IDI_TREE_STATIC_ICE_WEBUSER_CONNECTION   1362
#define IDI_TREE_STATIC_ICE_PROFILE              1363
#define IDI_TREE_STATIC_ICE_PROFILE_ROLE         1364
#define IDI_TREE_STATIC_ICE_PROFILE_CONNECTION   1365
// Under "Bussiness unit" (BUNIT)
#define IDI_TREE_STATIC_ICE_BUNIT                1366
#define IDI_TREE_STATIC_ICE_BUNIT_SECURITY       1367
#define IDI_TREE_STATIC_ICE_BUNIT_SEC_ROLE       1368
#define IDI_TREE_STATIC_ICE_BUNIT_SEC_USER       1369
#define IDI_TREE_STATIC_ICE_BUNIT_FACET          1370
#define IDI_TREE_STATIC_ICE_BUNIT_PAGE           1371
#define IDI_TREE_STATIC_ICE_BUNIT_LOCATION       1372
// Under "Server"
#define IDI_TREE_STATIC_ICE_SERVER               1373
#define IDI_TREE_STATIC_ICE_SERVER_APPLICATION   1374
#define IDI_TREE_STATIC_ICE_SERVER_LOCATION      1375
#define IDI_TREE_STATIC_ICE_SERVER_VARIABLE      1376
// Added later
#define IDI_TREE_STATIC_ICE_ROLE                 1377

#define IDI_TREE_STATIC_ICE_BUNIT_FACET_ROLE     1378
#define IDI_TREE_STATIC_ICE_BUNIT_FACET_USER     1379
#define IDI_TREE_STATIC_ICE_BUNIT_PAGE_ROLE      1380
#define IDI_TREE_STATIC_ICE_BUNIT_PAGE_USER      1381

//
// INSTALLATION LEVEL SETTINGS
//
#define IDI_TREE_STATIC_INSTALL                  1391
#define IDI_TREE_STATIC_INSTALL_SECURITY         1392
#define IDI_TREE_STATIC_INSTALL_GRANTEES         1393
#define IDI_TREE_STATIC_INSTALL_ALARMS           1394

// Icons for the configuration window
#define IDI_CFGICON1  1950
#define IDI_CFGICON2  1951
#define IDI_CFGICON3  1952
#define IDI_CFGICON4  1953
#define IDI_CFGICON5  1954
#define IDI_CFGICON6  1955
#define IDI_CFGICON7  1956
#define IDI_CFGICON8  1957
#define IDI_CFGICON9  1958
#define IDI_CFGICON10 1959
#define IDI_CFGICON11 1960
#define IDI_CFGICON12 1961
#define IDI_CFGICON13 1962
#define IDI_CFGICON14 1963
#define IDI_CFGICON15 1964

//
// BITMAPS
//

// test bitmaps - TO BE REMOVED
#define IDB_TEST_BUTTON1      31001
#define IDB_TEST_BUTTON2      31002

// bitmaps on global tool bar
#define IDB_NEW                   2001    // Space oddity
#define IDB_OPEN                  2002
#define IDB_SAVE                  2003
#define IDB_CONNECT               2004
#define IDB_DISCONNECT            2005
#define IDB_PREFERENCES           2006
#define IDB_CUT                   2007
#define IDB_COPY                  2008
#define IDB_PASTE                 2009
#define IDB_FIND                  2010
#define IDB_SQLACT                2011
#define IDB_SPACECALC             2012
#define IDB_WINDOWTILE_VERT       2013

// bitmaps on dom tool bar
#define IDB_DUMMY                 2101
#define IDB_ADDOBJECT             2102
#define IDB_ALTEROBJECT           2103
#define IDB_DROPOBJECT            2104
#define IDB_PROPERTIES            2105
#define IDB_REFRESH               2106
#define IDB_NEWWINDOW             2107
#define IDB_TEAROUT               2108
#define IDB_RESTARTFROMPOS        2109
#define IDB_PRINT                 2110
#define IDB_CLASSB_EXPANDONE      2111
#define IDB_CLASSB_EXPANDBRANCH   2112
#define IDB_CLASSB_EXPANDALL      2113
#define IDB_CLASSB_COLLAPSEONE    2114
#define IDB_CLASSB_COLLAPSEBRANCH 2115
#define IDB_CLASSB_COLLAPSEALL    2116

// bitmaps on sqlact tool bar
#define IDB_QUERYCLEAR            2131
#define IDB_QUERY_GO              2132
#define IDB_SQLWIZARD             2133
#define IDB_QUERYSHOWRES          2134

//
// CURSORS
//
#define IDC_HGBOLT            3001    // Hourglass while ingres server query
#define IDC_DIVIDE            3002    // Divide for SQL Act window
#define IDC_DOM_MV_CANDROP    3003    // Dom drag/drop move when can drop
#define IDC_DOM_MV_CANNOTDROP 3004    // Dom drag/drop move when cannot drop
#define IDC_DOM_CP_CANDROP    3005    // Dom drag/drop copy when can drop
#define IDC_DOM_CP_CANNOTDROP 3006    // Dom drag/drop copy when cannot drop

//
// MENUS and ACCELERATORS
//
#define ID_MENU_MAINMENU      4001    // when no doc
#define IDA_ACCELS            4002
#define ID_MENU_GWNONE        4003    // when active doc on OpenIngres node
#define ID_MENU_GWOIDT        4004    // when active doc on IngresDesktop node

//
// DIALOGS  - start from 5501 for resource.h - See previous note
//
#define ABORTPRINTDIALOG            5501    // Taken from VO neddlg.h
#define PRINTPAGESETUP              5502    // Taken from VO neddlg.h
#define BKTASKREFRESHDIALOG         5503
#define REMOTECOMMANDDIALOG         5504    // remotecommand_box in .hpj
#define REMOTEPILOTDIALOG           5505    // sendanswer_box in .hpj
#define STATUSBARPREFDIALOG         5506
#define SQLACTPREFDIALOG            5507
#define TOOLBARPREFDIALOG           5508
#define DOMPREFDIALOG               5509
#define DOMFILTERSDIALOG            5510    // setfilter_box in .hpj
#define VERIFYENVPASSWORDDIALOG     5511
// statement wizard boxes
#define IDD_STMTWIZ_FUNCCHOICE      5512    // main screen
#define IDD_STMTWIZ_FUNCPARAM       5513    // function parameters
#define IDD_STMTWIZ_AGGREGPARAM     5514    // Aggregates parameters
#define IDD_STMTWIZ_COMPARPARAM     5515    // Comparision parameters
#define IDD_STMTWIZ_AAEPARAM        5516    // Any/All/Exists parameters
#define IDD_STMTWIZ_DBOBJPARAM      5517    // database object parameters
#define IDD_STMTWIZ_INPARAM         5518    // 'In Predicate' parameters
// customized standard dialogs
#define IDD_CUSTOM_PRINTSETUP       5531
#define IDD_CUSTOM_FONTSETUP        5532

#define IDD_METER                   5533

// Display of sql statements errors
#define IDD_SQLERR                  5534

// Emb 04/03/96 : custom template for fonts,
// duplicated from IDD_CUSTOM_FONTSETUP
// for dom font choice in 32 bit mode with Win95 tree control
#define IDD_DOM_FONTSETUP        5535

// MFC version
#define IDD_IPM_FONTSETUP           5536
#define IDD_NODE_FONTSETUP          5537

//
// Constants for dialog boxes items
//
// start from 1001 for any box

// box ABORTPRINTDIALOG
#define IDD_PABORT_CANCEL       1001      // Taken from VO neddlg.h
#define IDD_PABORT_TEXT         1012      // Taken from VO neddlg.h

// box PRINTPAGESETUP
#define IDD_PSETUP_LEFTEDIT     1001      // Taken from VO neddlg.h
#define IDD_PSETUP_RIGHTEDIT    1002      // Taken from VO neddlg.h
#define IDD_PSETUP_TOPEDIT      1003      // Taken from VO neddlg.h
#define IDD_PSETUP_BOTTOMEDIT   1004      // Taken from VO neddlg.h
#define IDD_PSETUP_CM           1005      // Taken from VO neddlg.h
#define IDD_PSETUP_INCH         1006      // Taken from VO neddlg.h
#define IDD_PSETUP_LEFTSCROLL   1007      // Taken from VO neddlg.h
#define IDD_PSETUP_RIGHTSCROLL  1008      // Taken from VO neddlg.h
#define IDD_PSETUP_TOPSCROLL    1009      // Taken from VO neddlg.h
#define IDD_PSETUP_BOTTOMSCROLL 1010      // Taken from VO neddlg.h
#define IDD_PSETUP_OKAY         1011      // Taken from VO neddlg.h
#define IDD_PSETUP_CANCEL       1012      // Taken from VO neddlg.h
#define IDD_PSETUP_WIDTH        1013      // Taken from VO neddlg.h
#define IDD_PSETUP_HEIGHT       1014      // Taken from VO neddlg.h

// box BKTASKREFRESHDIALOG
#define IDD_TEXT                1001

// box REMOTECOMMANDDIALOG
#define IDD_REMOTECMD_TXT         1001
#define IDD_REMOTECMD_FEEDBACK    1002
#define IDD_REMOTECMD_ANSWERGO    1003
//#define IDD_REMOTECMD_ANSWEREDIT  1004

// box REMOTEPILOTDIALOG
#define IDD_REMOTEPILOT_ANSWER  1001

// box STATUSBARPREFDIALOG
#define IDD_SBPREF_VISIBLE        1001
#define IDD_SBPREF_DATE           1002
#define IDD_SBPREF_TIME           1003
#define IDD_SBPREF_SERVERCOUNT    1004
#define IDD_SBPREF_CURRENTSERVER  1005
#define IDD_SBPREF_OBJECTSCOUNT   1006
#define IDD_SBPREF_ENVIRONMENT    1007
#define IDD_SBPREF_CHOOSEFONT     1008

// box SQLACTPREFDIALOG
#define IDD_SQLPREF_SHOWDATA      1001
#define IDD_SQLPREF_ALL           1002
#define IDD_SQLPREF_SPECIFYNUM    1003
#define IDD_SQLPREF_NUM           1004
#define IDD_SQLPREF_NUMSPIN       1005
#define IDD_SQLPREF_AUTOCOMMIT    1006
#define IDD_SQLPREF_READLOCK      1007
#define IDD_SQLPREF_FONT          1008
// Sqlact (new concept)
#define IDC_SQLPREF_TABNUM        1009
#define IDC_SQLPREF_TABNUMSPIN    1010
#define IDC_SQLPREF_SHOWNONSELECT 1011
#define IDC_SQLPREF_SELECT_TRACE  1012
#define IDC_SQLPREF_TRACE_BUFNUM      1013
#define IDC_SQLPREF_TRACE_BUFNUMSPIN  1014
#define IDC_SQLPREF_TRACE_BUFUNIT     1015
#define IDC_SQLPREF_RECORD_BUFUNIT    1016

// box TOOLBARPREFDIALOG
#define IDD_TBPREF_VISIBLE        1001

// box DOMPREFDIALOG
#define IDD_DOMPREF_3D            1001
#define IDD_DOMPREF_NORMAL        1002
#define IDD_DOMPREF_FONT          1003

// box DOMFILTERSDIALOG
#define IDD_DOMFILTER_BASE        1001
#define IDD_DOMFILTER_OTHER       1002

// box VERIFYENVPASSWORDDIALOG
#define IDD_LOADENV_PASSWORD        1001
#define IDD_LOADENV_CONFIRMPASSWORD 1002

// box IDD_SQLERR
#define IDC_SQLERR_STMT       1001
#define IDC_SQLERR_ERRCODE    1002
#define IDC_SQLERR_ERRTXT     1003
#define IDC_SQLERR_FIRST      1004
#define IDC_SQLERR_NEXT       1005
#define IDC_SQLERR_PREV       1006
#define IDC_SQLERR_LAST       1007

// statement wizard box IDD_STMTWIZ_FUNCCHOICE
// (1000 - 1010 reserved for the wizard technology)
#define IDC_STMWIZ_CHOICE_FAMILY    2001
#define IDC_STMWIZ_CHOICE_FUNCTION  2002
#define IDC_STMWIZ_CHOICE_FNAME     2003
#define IDC_STMWIZ_CHOICE_FHELP1    2004
#define IDC_STMWIZ_CHOICE_FHELP2    2005


// statement wizard box IDD_STMTWIZ_FUNCPARAM
// (1000 - 1010 reserved for the wizard technology)
#define IDC_STMWIZ_FPAR_FNAME       2001
#define IDC_STMWIZ_FPAR_FHELP1      2002
#define IDC_STMWIZ_FPAR_FHELP2      2003
#define IDC_STMWIZ_FPAR_FHELP3      2004

#define IDC_STMWIZ_FPAR_PAR1        2005
#define IDC_STMWIZ_FPAR_BTN1        2006
#define IDC_STMWIZ_FPAR_ED1         2007
#define IDC_STMWIZ_FPAR_TXT1        2008
#define IDC_STMWIZ_FPAR_OP1         2009

#define IDC_STMWIZ_FPAR_PAR2        2010
#define IDC_STMWIZ_FPAR_BTN2        2011
#define IDC_STMWIZ_FPAR_ED2         2012
#define IDC_STMWIZ_FPAR_TXT2        2013
#define IDC_STMWIZ_FPAR_OP2         2014

#define IDC_STMWIZ_FPAR_PAR3        2015
#define IDC_STMWIZ_FPAR_BTN3        2016
#define IDC_STMWIZ_FPAR_ED3         2017


// statement wizard box IDD_STMTWIZ_AGGREGPARAM
// (1000 - 1010 reserved for the wizard technology)
#define IDC_STMWIZ_APAR_FNAME       2001
#define IDC_STMWIZ_APAR_FHELP1      2002
#define IDC_STMWIZ_APAR_FHELP2      2003
#define IDC_STMWIZ_APAR_FHELP3      2004

#define IDC_STMWIZ_APAR_PAR1        2005
#define IDC_STMWIZ_APAR_BTN1        2006
#define IDC_STMWIZ_APAR_ED1         2007

#define IDC_STMWIZ_APAR_R_ALL       2008
#define IDC_STMWIZ_APAR_R_DISTINCT  2009


// statement wizard box IDD_STMTWIZ_COMPARPARAM
// (1000 - 1010 reserved for the wizard technology)
#define IDC_STMWIZ_CPAR_FNAME       2001
#define IDC_STMWIZ_CPAR_FHELP1      2002
#define IDC_STMWIZ_CPAR_FHELP2      2003
#define IDC_STMWIZ_CPAR_FHELP3      2004

#define IDC_STMWIZ_CPAR_PAR1        2005
#define IDC_STMWIZ_CPAR_BTN1        2006
#define IDC_STMWIZ_CPAR_ED1         2007

#define IDC_STMWIZ_CPAR_PAR2        2008

#define IDC_STMWIZ_CPAR_R_GT        2009
#define IDC_STMWIZ_CPAR_R_LT        2010
#define IDC_STMWIZ_CPAR_R_GE        2011
#define IDC_STMWIZ_CPAR_R_LE        2012
#define IDC_STMWIZ_CPAR_R_NE        2013
#define IDC_STMWIZ_CPAR_R_EQ        2014

#define IDC_STMWIZ_CPAR_PAR3        2015
#define IDC_STMWIZ_CPAR_BTN2        2016
#define IDC_STMWIZ_CPAR_ED2         2017


// statement wizard box IDD_STMTWIZ_AAEPARAM
// (1000 - 1010 reserved for the wizard technology)
#define IDC_STMWIZ_AAPAR_FNAME      2001
#define IDC_STMWIZ_AAPAR_FHELP1     2002
#define IDC_STMWIZ_AAPAR_FHELP2     2003
#define IDC_STMWIZ_AAPAR_FHELP3     2004

// radio group : removed 20/06/95
//#define IDC_STMWIZ_AAPAR_R_ALL      2005
//#define IDC_STMWIZ_AAPAR_R_ANY      2006
//#define IDC_STMWIZ_AAPAR_R_EXISTS   2007

#define IDC_STMWIZ_AAPAR_PAR1       2008
#define IDC_STMWIZ_AAPAR_BTN1       2009
#define IDC_STMWIZ_AAPAR_ED1        2010


// statement wizard box IDD_STMTWIZ_DBOBJPARAM
// (1000 - 1010 reserved for the wizard technology)
#define IDC_STMWIZ_DPAR_FNAME       2001
#define IDC_STMWIZ_DPAR_FHELP1      2002
#define IDC_STMWIZ_DPAR_FHELP2      2003
#define IDC_STMWIZ_DPAR_FHELP3      2004

#define IDC_STMWIZ_DPAR_LB1         2005  // listbox


// statement wizard box IDD_STMTWIZ_INPARAM
// (1000 - 1010 reserved for the wizard technology)
#define IDC_STMWIZ_IPAR_FNAME       2001
#define IDC_STMWIZ_IPAR_FHELP1      2002
#define IDC_STMWIZ_IPAR_FHELP2      2003
// Unused : #define IDC_STMWIZ_IPAR_FHELP3      2004

#define IDC_STMWIZ_IPAR_BTN1        2005
#define IDC_STMWIZ_IPAR_ED1         2006

#define IDC_STMWIZ_IPAR_R_IN        2007
#define IDC_STMWIZ_IPAR_R_NOTIN     2008

#define IDC_STMWIZ_IPAR_R_SUBQ      2009
#define IDC_STMWIZ_IPAR_R_LIST      2010

#define IDC_STMWIZ_IPAR_BTN2        2011
#define IDC_STMWIZ_IPAR_ED2         2012

#define IDC_STMWIZ_IPAR_LB1         2013
#define IDC_STMWIZ_IPAR_BTNINS      2014
#define IDC_STMWIZ_IPAR_BTNADD      2015
#define IDC_STMWIZ_IPAR_BTNREPLACE  2016
#define IDC_STMWIZ_IPAR_BTNREMOVE   2017

#define IDC_STMWIZ_IPAR_BTN3        2018
#define IDC_STMWIZ_IPAR_ED3         2019

//
// stringtable values
// convention for values : start from 1
//
// 20001 TO 40000 RESERVED FOR DLGRES.H (dialog boxes)
//
#define IDS_APPNAME               1
#define IDS_DOMDOC_TITLE          2
#define IDS_DOMDOC_LOWONRESOURCE  3
#define IDS_DOMDOC_CANNOTCREATE   4
#define IDS_DOMDOC_PRINTING       6
#define IDS_DOMDOC_PRINTPAGENUM   7
#define IDS_VERSION_MARKER              8
#define IDS_VERSION_MARKER_DEMOCLASSES  9
#define IDS_VERSION_ABOUT         10    // split from IDS_VERSION_MARKER July 9, 96

// create/alter/drop messages - TO BE FINISHED: SEEM NOT TO BE USED ANYMORE
#define IDS_RES_ERR               11
#define IDS_RES_TIMEOUT           12
#define IDS_RES_NOGRANT           13
#define IDS_RES_ALREADYEXIST      14

// Dom Tree status lines - ids of fake strings used to compute width
#define IDS_TS_W_OBJTYPE            21
#define IDS_TS_W_OBJNAME            22
#define IDS_TS_W_CAPT_OWNER         23
#define IDS_TS_W_OWNERNAME          24
#define IDS_TS_W_CAPT_COMPLIM       25
#define IDS_TS_W_COMPLIM            26
#define IDS_TS_W_BSYSTEM            27

// Dom Tree status lines - ids of displayed strings
#define IDS_TS_CAPT_OWNER         28
#define IDS_TS_SYSTEMOBJ          29
#define IDS_TS_NONSYSTEMOBJ       30

// Dom title : sub-strings
#define IDS_DOMDOC_TITLE_NORMAL     31
#define IDS_DOMDOC_TITLE_TEAROUT    32
#define IDS_DOMDOC_TITLE_REPOS      33
#define IDS_DOMDOC_TITLE_SCRATCHPAD 34

// dom system menu custom menuitem for scratchpad
#define IDS_DOMDOC_CHANGEVNODE      35

// "New environment" and "Open environment" menuitem messages
#define IDS_NEW_QUERYSAVE_TXT       36
#define IDS_NEW_UNTITLED_ENV        37
#define IDS_NEW_QUERYSAVE_CAPT      38

// Global status line - ids of fake strings used to compute width
#define IDS_GS_W_DATE               41
#define IDS_GS_W_TIME               42
#define IDS_GS_W_CAPT_CONNSRV       43
#define IDS_GS_W_CONNSRV            44
#define IDS_GS_W_CAPT_CURRSRV       45
#define IDS_GS_W_CURRSRV            46
#define IDS_GS_W_CAPT_OBJECTS       47
#define IDS_GS_W_OBJECTS            48
#define IDS_GS_W_ENVNAME            49
// Keep space for key states

// Global status line - ids of displayed strings
#define IDS_GS_T_CAPT_CONNSRV       56
#define IDS_GS_T_CAPT_CURRSRV       57
#define IDS_GS_T_CAPT_OBJECTS       58
#define IDS_GS_ERR_TIMER            59
#define IDS_GS_T_UNKNOWNOBJCOUNT    60
#define IDS_GS_T_OBJECTS_NOAPPLY    61

// special items on tree lines
#define IDS_TREE_DUMMY                  101
#define IDS_TREE_ERR_NOGRANT            102
#define IDS_TREE_ERR_TIMEOUT            103
#define IDS_TREE_ERROR                  104
#define IDS_TREE_GRANTEE_NONEXISTENT    105
#define IDS_TREE_VIEWTABLE_NONEXISTENT  106
#define IDS_TREE_SYNONYMED_NONEXISTENT  107
#define IDS_TREE_ALARMEE_NONEXISTENT    108
#define IDS_TREE_REPLICTABLE_NONEXISTENT  109

// tree lines of level 0
#define IDS_TREE_DATABASE_STATIC  111
#define IDS_TREE_DATABASE_NONE    112
#define IDS_TREE_PROFILE_STATIC   113
#define IDS_TREE_PROFILE_NONE     114
#define IDS_TREE_USER_STATIC      115
#define IDS_TREE_USER_NONE        116
#define IDS_TREE_GROUP_STATIC     117
#define IDS_TREE_GROUP_NONE       118
#define IDS_TREE_ROLE_STATIC      119
#define IDS_TREE_ROLE_NONE        120
#define IDS_TREE_LOCATION_STATIC  121
#define IDS_TREE_LOCATION_NONE    122
#define IDS_TREE_SYNONYMED_STATIC 123     
#define IDS_TREE_DBAREA_STATIC    124     // Desktop
#define IDS_TREE_DBAREA_NONE      125     // Desktop
#define IDS_TREE_STOGROUP_STATIC  126     // Desktop
#define IDS_TREE_STOGROUP_NONE    127     // Desktop

// tree lines of level 1, children of database
#define IDS_TREE_TABLE_STATIC       200
#define IDS_TREE_TABLE_NONE         201
#define IDS_TREE_VIEW_STATIC        202
#define IDS_TREE_VIEW_NONE          203
#define IDS_TREE_PROCEDURE_STATIC   204
#define IDS_TREE_PROCEDURE_NONE     205
#define IDS_TREE_SCHEMA_STATIC      206
#define IDS_TREE_SCHEMA_NONE        207
#define IDS_TREE_SYNONYM_STATIC     208
#define IDS_TREE_SYNONYM_NONE       209
#define IDS_TREE_DBEVENT_STATIC     210
#define IDS_TREE_DBEVENT_NONE       211
// continued after level 3 not to resequence constants numbers

// tree lines of level 1, children of group
#define IDS_TREE_GROUPUSER_STATIC 213
#define IDS_TREE_GROUPUSER_NONE   214

// tree lines of level 1, children of user
#define IDS_TREE_R_USERSCHEMA_STATIC  215
#define IDS_TREE_R_USERSCHEMA_NONE    216
#define IDS_TREE_R_USERGROUP_STATIC   217
#define IDS_TREE_R_USERGROUP_NONE     218

#define IDS_TREE_R_SECURITY_STATIC      219
#define IDS_TREE_R_SEC_SEL_SUCC_STATIC  221
#define IDS_TREE_R_SEC_SEL_SUCC_NONE    222
#define IDS_TREE_R_SEC_SEL_FAIL_STATIC  223
#define IDS_TREE_R_SEC_SEL_FAIL_NONE    224
#define IDS_TREE_R_SEC_DEL_SUCC_STATIC  226
#define IDS_TREE_R_SEC_DEL_SUCC_NONE    227
#define IDS_TREE_R_SEC_DEL_FAIL_STATIC  228
#define IDS_TREE_R_SEC_DEL_FAIL_NONE    229
#define IDS_TREE_R_SEC_INS_SUCC_STATIC  231
#define IDS_TREE_R_SEC_INS_SUCC_NONE    232
#define IDS_TREE_R_SEC_INS_FAIL_STATIC  233
#define IDS_TREE_R_SEC_INS_FAIL_NONE    234
#define IDS_TREE_R_SEC_UPD_SUCC_STATIC  236
#define IDS_TREE_R_SEC_UPD_SUCC_NONE    237
#define IDS_TREE_R_SEC_UPD_FAIL_STATIC  238
#define IDS_TREE_R_SEC_UPD_FAIL_NONE    239

#define IDS_TREE_R_GRANT_STATIC             240  // rights granted
#define IDS_TREE_R_DBEGRANT_STATIC          241  // grants for dbevents
#define IDS_TREE_R_DBEGRANT_RAISE_STATIC    242
#define IDS_TREE_R_DBEGRANT_RAISE_NONE      243
#define IDS_TREE_R_DBEGRANT_REGISTER_STATIC 244
#define IDS_TREE_R_DBEGRANT_REGISTER_NONE   245
#define IDS_TREE_R_PROCGRANT_STATIC         246  // grants for procedures
#define IDS_TREE_R_PROCGRANT_EXEC_STATIC    247
#define IDS_TREE_R_PROCGRANT_EXEC_NONE      248
#define IDS_TREE_R_DBGRANT_STATIC           249  // grants for databases
#define IDS_TREE_R_DBGRANT_ACCESY_STATIC    250
#define IDS_TREE_R_DBGRANT_ACCESY_NONE      251
#define IDS_TREE_R_DBGRANT_ACCESN_STATIC    252
#define IDS_TREE_R_DBGRANT_ACCESN_NONE      253
#define IDS_TREE_R_DBGRANT_CREPRY_STATIC    254
#define IDS_TREE_R_DBGRANT_CREPRY_NONE      255
#define IDS_TREE_R_DBGRANT_CREPRN_STATIC    256
#define IDS_TREE_R_DBGRANT_CREPRN_NONE      257
#define IDS_TREE_R_DBGRANT_CRETBY_STATIC    258
#define IDS_TREE_R_DBGRANT_CRETBY_NONE      259
#define IDS_TREE_R_DBGRANT_CRETBN_STATIC    260
#define IDS_TREE_R_DBGRANT_CRETBN_NONE      261
#define IDS_TREE_R_DBGRANT_DBADMY_STATIC    262
#define IDS_TREE_R_DBGRANT_DBADMY_NONE      263
#define IDS_TREE_R_DBGRANT_DBADMN_STATIC    264
#define IDS_TREE_R_DBGRANT_DBADMN_NONE      265
#define IDS_TREE_R_DBGRANT_LKMODY_STATIC    266
#define IDS_TREE_R_DBGRANT_LKMODY_NONE      267
#define IDS_TREE_R_DBGRANT_LKMODN_STATIC    268
#define IDS_TREE_R_DBGRANT_LKMODN_NONE      269
#define IDS_TREE_R_DBGRANT_QRYIOY_STATIC    270
#define IDS_TREE_R_DBGRANT_QRYIOY_NONE      271
#define IDS_TREE_R_DBGRANT_QRYION_STATIC    272
#define IDS_TREE_R_DBGRANT_QRYION_NONE      273
#define IDS_TREE_R_DBGRANT_QRYRWY_STATIC    274
#define IDS_TREE_R_DBGRANT_QRYRWY_NONE      275
#define IDS_TREE_R_DBGRANT_QRYRWN_STATIC    276
#define IDS_TREE_R_DBGRANT_QRYRWN_NONE      277
#define IDS_TREE_R_DBGRANT_UPDSCY_STATIC    278
#define IDS_TREE_R_DBGRANT_UPDSCY_NONE      279
#define IDS_TREE_R_DBGRANT_UPDSCN_STATIC    280
#define IDS_TREE_R_DBGRANT_UPDSCN_NONE      281

#define IDS_TREE_R_DBGRANT_SELSCY_STATIC    6001
#define IDS_TREE_R_DBGRANT_SELSCY_NONE      6002
#define IDS_TREE_R_DBGRANT_SELSCN_STATIC    6003
#define IDS_TREE_R_DBGRANT_SELSCN_NONE      6004
#define IDS_TREE_R_DBGRANT_CNCTLY_STATIC    6005
#define IDS_TREE_R_DBGRANT_CNCTLY_NONE      6006
#define IDS_TREE_R_DBGRANT_CNCTLN_STATIC    6007
#define IDS_TREE_R_DBGRANT_CNCTLN_NONE      6008
#define IDS_TREE_R_DBGRANT_IDLTLY_STATIC    6009
#define IDS_TREE_R_DBGRANT_IDLTLY_NONE      6010
#define IDS_TREE_R_DBGRANT_IDLTLN_STATIC    6011
#define IDS_TREE_R_DBGRANT_IDLTLN_NONE      6012
#define IDS_TREE_R_DBGRANT_SESPRY_STATIC    6013
#define IDS_TREE_R_DBGRANT_SESPRY_NONE      6014
#define IDS_TREE_R_DBGRANT_SESPRN_STATIC    6015
#define IDS_TREE_R_DBGRANT_SESPRN_NONE      6016
#define IDS_TREE_R_DBGRANT_TBLSTY_STATIC    6017
#define IDS_TREE_R_DBGRANT_TBLSTY_NONE      6018
#define IDS_TREE_R_DBGRANT_TBLSTN_STATIC    6019
#define IDS_TREE_R_DBGRANT_TBLSTN_NONE      6020

#define IDS_TREE_R_TABLEGRANT_STATIC        282  // grants for tables
#define IDS_TREE_R_TABLEGRANT_SEL_STATIC    283
#define IDS_TREE_R_TABLEGRANT_SEL_NONE      284
#define IDS_TREE_R_TABLEGRANT_INS_STATIC    285
#define IDS_TREE_R_TABLEGRANT_INS_NONE      286
#define IDS_TREE_R_TABLEGRANT_UPD_STATIC    287
#define IDS_TREE_R_TABLEGRANT_UPD_NONE      288
#define IDS_TREE_R_TABLEGRANT_DEL_STATIC    289
#define IDS_TREE_R_TABLEGRANT_DEL_NONE      290
#define IDS_TREE_R_TABLEGRANT_REF_STATIC    291
#define IDS_TREE_R_TABLEGRANT_REF_NONE      292

#define IDS_TREE_R_TABLEGRANT_CPI_STATIC    6021
#define IDS_TREE_R_TABLEGRANT_CPI_NONE      6022
#define IDS_TREE_R_TABLEGRANT_CPF_STATIC    6023
#define IDS_TREE_R_TABLEGRANT_CPF_NONE      6024

#define IDS_TREE_R_TABLEGRANT_ALL_STATIC    293
#define IDS_TREE_R_TABLEGRANT_ALL_NONE      294
#define IDS_TREE_R_VIEWGRANT_STATIC         295  // grants for views
#define IDS_TREE_R_VIEWGRANT_SEL_STATIC     296
#define IDS_TREE_R_VIEWGRANT_SEL_NONE       297
#define IDS_TREE_R_VIEWGRANT_INS_STATIC     298
#define IDS_TREE_R_VIEWGRANT_INS_NONE       299
#define IDS_TREE_R_VIEWGRANT_UPD_STATIC     300
#define IDS_TREE_R_VIEWGRANT_UPD_NONE       301
#define IDS_TREE_R_VIEWGRANT_DEL_STATIC     302
#define IDS_TREE_R_VIEWGRANT_DEL_NONE       303
#define IDS_TREE_R_ROLEGRANT_STATIC         304
#define IDS_TREE_R_ROLEGRANT_NONE           305

// tree statics of level 1, children of role
#define IDS_TREE_ROLEGRANT_USER_STATIC      306
#define IDS_TREE_ROLEGRANT_USER_NONE        307

// tree statics of level 1, children of location
#define IDS_TREE_R_LOCATIONTABLE_STATIC     308
#define IDS_TREE_R_LOCATIONTABLE_NONE       309

// tree lines of level 2, children of "table of database"
#define IDS_TREE_INTEGRITY_STATIC     311
#define IDS_TREE_INTEGRITY_NONE       312
#define IDS_TREE_RULE_STATIC          313
#define IDS_TREE_RULE_NONE            314
#define IDS_TREE_INDEX_STATIC         315
#define IDS_TREE_INDEX_NONE           316
#define IDS_TREE_TABLELOCATION_STATIC 317
#define IDS_TREE_TABLELOCATION_NONE   318

#define IDS_TREE_SECURITY_STATIC      319
#define IDS_TREE_SEC_SEL_SUCC_STATIC  321
#define IDS_TREE_SEC_SEL_SUCC_NONE    322
#define IDS_TREE_SEC_SEL_FAIL_STATIC  323
#define IDS_TREE_SEC_SEL_FAIL_NONE    324
#define IDS_TREE_SEC_DEL_SUCC_STATIC  326
#define IDS_TREE_SEC_DEL_SUCC_NONE    327
#define IDS_TREE_SEC_DEL_FAIL_STATIC  328
#define IDS_TREE_SEC_DEL_FAIL_NONE    329
#define IDS_TREE_SEC_INS_SUCC_STATIC  331
#define IDS_TREE_SEC_INS_SUCC_NONE    332
#define IDS_TREE_SEC_INS_FAIL_STATIC  333
#define IDS_TREE_SEC_INS_FAIL_NONE    334
#define IDS_TREE_SEC_UPD_SUCC_STATIC  336
#define IDS_TREE_SEC_UPD_SUCC_NONE    337
#define IDS_TREE_SEC_UPD_FAIL_STATIC  338
#define IDS_TREE_SEC_UPD_FAIL_NONE    339

#define IDS_TREE_R_TABLESYNONYM_STATIC  340
#define IDS_TREE_R_TABLESYNONYM_NONE    341
#define IDS_TREE_R_TABLEVIEW_STATIC     342
#define IDS_TREE_R_TABLEVIEW_NONE       343

#define IDS_TREE_TABLEGRANTEES_STATIC         344
#define IDS_TREE_TABLEGRANT_SEL_USER_STATIC   345
#define IDS_TREE_TABLEGRANT_SEL_USER_NONE     346
#define IDS_TREE_TABLEGRANT_INS_USER_STATIC   347
#define IDS_TREE_TABLEGRANT_INS_USER_NONE     348
#define IDS_TREE_TABLEGRANT_UPD_USER_STATIC   349
#define IDS_TREE_TABLEGRANT_UPD_USER_NONE     350
#define IDS_TREE_TABLEGRANT_DEL_USER_STATIC   351
#define IDS_TREE_TABLEGRANT_DEL_USER_NONE     352
#define IDS_TREE_TABLEGRANT_REF_USER_STATIC   353
#define IDS_TREE_TABLEGRANT_REF_USER_NONE     354

#define IDS_TREE_TABLEGRANT_CPI_USER_STATIC   6025
#define IDS_TREE_TABLEGRANT_CPI_USER_NONE     6026
#define IDS_TREE_TABLEGRANT_CPF_USER_STATIC   6027
#define IDS_TREE_TABLEGRANT_CPF_USER_NONE     6028

#define IDS_TREE_TABLEGRANT_ALL_USER_STATIC   355
#define IDS_TREE_TABLEGRANT_ALL_USER_NONE     356

// desktop
#define IDS_TREE_TABLEGRANT_INDEX_USER_STATIC   378
#define IDS_TREE_TABLEGRANT_INDEX_USER_NONE     379
#define IDS_TREE_TABLEGRANT_ALTER_USER_STATIC   380
#define IDS_TREE_TABLEGRANT_ALTER_USER_NONE     381

// tree lines of level 2, children of "view of database"
#define IDS_TREE_VIEWTABLE_STATIC             357
#define IDS_TREE_VIEWTABLE_NONE               358
#define IDS_TREE_R_VIEWSYNONYM_STATIC         359
#define IDS_TREE_R_VIEWSYNONYM_NONE           360

#define IDS_TREE_VIEWGRANTEES_STATIC          361
#define IDS_TREE_VIEWGRANT_SEL_USER_STATIC    362
#define IDS_TREE_VIEWGRANT_SEL_USER_NONE      363
#define IDS_TREE_VIEWGRANT_INS_USER_STATIC    364
#define IDS_TREE_VIEWGRANT_INS_USER_NONE      365
#define IDS_TREE_VIEWGRANT_UPD_USER_STATIC    366
#define IDS_TREE_VIEWGRANT_UPD_USER_NONE      367
#define IDS_TREE_VIEWGRANT_DEL_USER_STATIC    368
#define IDS_TREE_VIEWGRANT_DEL_USER_NONE      369

// tree statics of level 2, child of "procedure of database"
#define IDS_TREE_PROCGRANT_EXEC_USER_STATIC   370
#define IDS_TREE_PROCGRANT_EXEC_USER_NONE     371
#define IDS_TREE_R_PROC_RULE_STATIC           372
#define IDS_TREE_R_PROC_RULE_NONE             373

// tree statics of level 2, child of "dbevent of database"
#define IDS_TREE_DBEGRANT_RAISE_USER_STATIC   374
#define IDS_TREE_DBEGRANT_RAISE_USER_NONE     375
#define IDS_TREE_DBEGRANT_REGTR_USER_STATIC   376
#define IDS_TREE_DBEGRANT_REGTR_USER_NONE     377

// tree lines of level 3, child of "index on table of database"
#define IDS_TREE_R_INDEXSYNONYM_STATIC  401
#define IDS_TREE_R_INDEXSYNONYM_NONE    402

// tree lines of level 3, child of "rule on table of database"
#define IDS_TREE_RULEPROC_STATIC    403
#define IDS_TREE_RULEPROC_NONE      404

// continuation of level 1, children of database
// (in order not to resequence constants numbers of previous levels)
// kept small margin
#define IDS_TREE_DBGRANTEES_STATIC          411
#define IDS_TREE_DBGRANTEES_ACCESY_STATIC   412
#define IDS_TREE_DBGRANTEES_ACCESY_NONE     413
#define IDS_TREE_DBGRANTEES_ACCESN_STATIC   414
#define IDS_TREE_DBGRANTEES_ACCESN_NONE     415
#define IDS_TREE_DBGRANTEES_CREPRY_STATIC   416
#define IDS_TREE_DBGRANTEES_CREPRY_NONE     417
#define IDS_TREE_DBGRANTEES_CREPRN_STATIC   418
#define IDS_TREE_DBGRANTEES_CREPRN_NONE     419
#define IDS_TREE_DBGRANTEES_CRETBY_STATIC   420
#define IDS_TREE_DBGRANTEES_CRETBY_NONE     421
#define IDS_TREE_DBGRANTEES_CRETBN_STATIC   422
#define IDS_TREE_DBGRANTEES_CRETBN_NONE     423
#define IDS_TREE_DBGRANTEES_DBADMY_STATIC   424
#define IDS_TREE_DBGRANTEES_DBADMY_NONE     425
#define IDS_TREE_DBGRANTEES_DBADMN_STATIC   426
#define IDS_TREE_DBGRANTEES_DBADMN_NONE     427
#define IDS_TREE_DBGRANTEES_LKMODY_STATIC   428
#define IDS_TREE_DBGRANTEES_LKMODY_NONE     429
#define IDS_TREE_DBGRANTEES_LKMODN_STATIC   430
#define IDS_TREE_DBGRANTEES_LKMODN_NONE     431
#define IDS_TREE_DBGRANTEES_QRYIOY_STATIC   432
#define IDS_TREE_DBGRANTEES_QRYIOY_NONE     433
#define IDS_TREE_DBGRANTEES_QRYION_STATIC   434
#define IDS_TREE_DBGRANTEES_QRYION_NONE     435
#define IDS_TREE_DBGRANTEES_QRYRWY_STATIC   436
#define IDS_TREE_DBGRANTEES_QRYRWY_NONE     437
#define IDS_TREE_DBGRANTEES_QRYRWN_STATIC   438
#define IDS_TREE_DBGRANTEES_QRYRWN_NONE     439
#define IDS_TREE_DBGRANTEES_UPDSCY_STATIC   440
#define IDS_TREE_DBGRANTEES_UPDSCY_NONE     441
#define IDS_TREE_DBGRANTEES_UPDSCN_STATIC   442
#define IDS_TREE_DBGRANTEES_UPDSCN_NONE     443

#define IDS_TREE_DBGRANTEES_SELSCY_STATIC   6031
#define IDS_TREE_DBGRANTEES_SELSCY_NONE     6032
#define IDS_TREE_DBGRANTEES_SELSCN_STATIC   6033
#define IDS_TREE_DBGRANTEES_SELSCN_NONE     6034
#define IDS_TREE_DBGRANTEES_CNCTLY_STATIC   6035
#define IDS_TREE_DBGRANTEES_CNCTLY_NONE     6036
#define IDS_TREE_DBGRANTEES_CNCTLN_STATIC   6037
#define IDS_TREE_DBGRANTEES_CNCTLN_NONE     6038
#define IDS_TREE_DBGRANTEES_IDLTLY_STATIC   6039
#define IDS_TREE_DBGRANTEES_IDLTLY_NONE     6040
#define IDS_TREE_DBGRANTEES_IDLTLN_STATIC   6041
#define IDS_TREE_DBGRANTEES_IDLTLN_NONE     6042
#define IDS_TREE_DBGRANTEES_SESPRY_STATIC   6043
#define IDS_TREE_DBGRANTEES_SESPRY_NONE     6044
#define IDS_TREE_DBGRANTEES_SESPRN_STATIC   6045
#define IDS_TREE_DBGRANTEES_SESPRN_NONE     6046
#define IDS_TREE_DBGRANTEES_TBLSTY_STATIC   6047
#define IDS_TREE_DBGRANTEES_TBLSTY_NONE     6048
#define IDS_TREE_DBGRANTEES_TBLSTN_STATIC   6049
#define IDS_TREE_DBGRANTEES_TBLSTN_NONE     6050

#define IDS_TREE_DBALARM_STATIC                 444
#define IDS_TREE_DBALARM_CONN_SUCCESS_STATIC    445
#define IDS_TREE_DBALARM_CONN_SUCCESS_NONE      446
#define IDS_TREE_DBALARM_CONN_FAILURE_STATIC    447
#define IDS_TREE_DBALARM_CONN_FAILURE_NONE      448
#define IDS_TREE_DBALARM_DISCONN_SUCCESS_STATIC 449
#define IDS_TREE_DBALARM_DISCONN_SUCCESS_NONE   450
#define IDS_TREE_DBALARM_DISCONN_FAILURE_STATIC 451
#define IDS_TREE_DBALARM_DISCONN_FAILURE_NONE   452

// replicator strings - all levels mixed
#define IDS_TREE_REPLICATOR_STATIC          453
#define IDS_TREE_REPLICATOR_NONE            454

#define IDS_TREE_REPLIC_CONNECTION_STATIC   455
#define IDS_TREE_REPLIC_CONNECTION_NONE     456
#define IDS_TREE_REPLIC_CDDS_STATIC         457
#define IDS_TREE_REPLIC_CDDS_NONE           458
#define IDS_TREE_REPLIC_MAILUSER_STATIC     459
#define IDS_TREE_REPLIC_MAILUSER_NONE       460
#define IDS_TREE_REPLIC_REGTABLE_STATIC     461
#define IDS_TREE_REPLIC_REGTABLE_NONE       462
#define IDS_TREE_REPLIC_SERVER_STATIC       463
#define IDS_TREE_REPLIC_SERVER_NONE         464

#define IDS_TREE_REPLIC_CDDS_DETAIL_STATIC  465
#define IDS_TREE_REPLIC_CDDS_DETAIL_NONE    466
#define IDS_TREE_R_REPLIC_CDDS_TABLE_STATIC 467
#define IDS_TREE_R_REPLIC_CDDS_TABLE_NONE   468

#define IDS_TREE_R_REPLIC_TABLE_CDDS_STATIC 469
#define IDS_TREE_R_REPLIC_TABLE_CDDS_NONE   470

// new style alarms (with 2 sub-branches alarmee and launched dbevent)
#define IDS_TREE_ALARMEE_STATIC             471
#define IDS_TREE_ALARMEE_NONE               472   // SHOULD NOT OCCUR
#define IDS_TREE_ALARM_EVENT_STATIC         473
#define IDS_TREE_ALARM_EVENT_NONE           474

// strings for classes
#define IDS_TREE_CLASSES_STATIC             481
#define IDS_TREE_CLASSES_NONE               482
#define IDS_TREE_SUBCLASSES_STATIC          483
#define IDS_TREE_SUBCLASSES_NONE            484

// strings related to edits and checkboxes on DOM doc. button line
#define IDS_STAT_BASE_OWNER         500
#define IDS_STAT_OTHER_OWNER        501
#define IDS_STAT_ALL_OWNER          502
#define IDS_STAT_SYSTEMOBJECTS      503
#define IDS_CHECKBOX_SYSTEMOBJECTS  504
#define IDS_FILTER_OWNER_ALL        505
#define IDS_FILTER_OWNER_ERROR      506
#define IDS_FILTER_OWNER_NOGRANT    507

// SQL activity document
#define IDS_SQLACTDOC_TITLE         601
#define IDS_OWNER                   602
#define IDS_TBLNAME                 603
#define IDS_COLUMNS                 604
#define IDS_COMMENTS                605
#define IDS_COMTXT                  606
#define IDS_EXECTXT                 607
#define IDS_ELAPTXT                 608
#define IDS_COSTTXT                 609

#define IDS_QUERYCLEAR              610
#define IDS_QUERYOPEN               611
#define IDS_QUERYSAVEAS             612
#define IDS_QUERYGO                 613
#define IDS_QUERYCONNECT            614
#define IDS_QUERYEXEC               615
#define IDS_QUERYSHOWRES            616

#define IDS_STAT_DATABASES          617
#define IDS_DATABASES_ERROR         618
#define IDS_DATABASES_NOGRANT       619
#define IDS_DATABASES_NONE          620
#define IDS_SQLACT_CANNOTCONNECT    621
#define IDS_SQLACTCNT_DEFCAPT       622
#define IDS_SQLACT_MORETUPLES       623
#define IDS_SQLWIZARD               624
#define IDS_SQLACT_PRINTPAGENUM     625
#define IDS_SQLACT_DBNAME_CAPT      626
#define IDS_SQLACT_QUERY_CAPT       627
#define IDS_SQLACT_TIMES_CAPT       628
#define IDS_SQLACT_TIMES            629
#define IDS_SQLACT_TUPLES_CAPT      630
#define IDS_SQLFILES                631
#define IDS_SQLFILES_FILTER         632
#define IDS_ALLSQLFILES             633
#define IDS_ALLSQLFILES_FILTER      634
#define IDS_SQLNULLVAR              635
#define IDS_SQLACT_REQRESULT        636
#define IDS_SQLACT_SELACTIVE        637
#define IDS_SQLACT_WRONGSTM         638

// Object property document messages
#define IDS_PROPERTYDOC_TITLE       701
#define IDS_PROPDOC_UNKNOWNTYPE     702
#define IDS_PROPDOC_CANNOTCREATE    703

// Find messages
#define IDS_FIND_TREE_NOTFOUND      751

// strings for status bar information about current non-static item
// values : 799 to 950
#define IDS_TREE_STATUS_OT_UNKNOWN                  799

// level 0
#define IDS_TREE_STATUS_OT_DATABASE                 800
#define IDS_TREE_STATUS_OT_PROFILE                  801
#define IDS_TREE_STATUS_OT_USER                     802
#define IDS_TREE_STATUS_OT_GROUP                    803
#define IDS_TREE_STATUS_OT_ROLE                     804
#define IDS_TREE_STATUS_OT_LOCATION                 805
#define IDS_TREE_STATUS_OT_SYNONYMED                806
#define IDS_TREE_STATUS_OT_DBAREA                   797   // Desktop
#define IDS_TREE_STATUS_OT_STOGROUP                 798   // Desktop
#define IDS_TREE_STATUS_OT_REPLICATOR               796

// level 1, child of database
#define IDS_TREE_STATUS_OT_TABLE                    807
#define IDS_TREE_STATUS_OT_VIEW                     808
#define IDS_TREE_STATUS_OT_PROCEDURE                809
#define IDS_TREE_STATUS_OT_SCHEMAUSER               810
#define IDS_TREE_STATUS_OT_SYNONYM                  811
#define IDS_TREE_STATUS_OT_DBGRANT_ACCESY_USER      812
#define IDS_TREE_STATUS_OT_DBGRANT_ACCESN_USER      813
#define IDS_TREE_STATUS_OT_DBGRANT_CREPRY_USER      814
#define IDS_TREE_STATUS_OT_DBGRANT_CREPRN_USER      815
#define IDS_TREE_STATUS_OT_DBGRANT_CRETBY_USER      816
#define IDS_TREE_STATUS_OT_DBGRANT_CRETBN_USER      817
#define IDS_TREE_STATUS_OT_DBGRANT_DBADMY_USER      818
#define IDS_TREE_STATUS_OT_DBGRANT_DBADMN_USER      819
#define IDS_TREE_STATUS_OT_DBGRANT_LKMODY_USER      820
#define IDS_TREE_STATUS_OT_DBGRANT_LKMODN_USER      821
#define IDS_TREE_STATUS_OT_DBGRANT_QRYIOY_USER      822
#define IDS_TREE_STATUS_OT_DBGRANT_QRYION_USER      823
#define IDS_TREE_STATUS_OT_DBGRANT_QRYRWY_USER      824
#define IDS_TREE_STATUS_OT_DBGRANT_QRYRWN_USER      825
#define IDS_TREE_STATUS_OT_DBGRANT_UPDSCY_USER      826
#define IDS_TREE_STATUS_OT_DBGRANT_UPDSCN_USER      827

#define IDS_TREE_STATUS_OT_DBGRANT_SELSCY_USER      6061
#define IDS_TREE_STATUS_OT_DBGRANT_SELSCN_USER      6062
#define IDS_TREE_STATUS_OT_DBGRANT_CNCTLY_USER      6063
#define IDS_TREE_STATUS_OT_DBGRANT_CNCTLN_USER      6064
#define IDS_TREE_STATUS_OT_DBGRANT_IDLTLY_USER      6065
#define IDS_TREE_STATUS_OT_DBGRANT_IDLTLN_USER      6066
#define IDS_TREE_STATUS_OT_DBGRANT_SESPRY_USER      6067
#define IDS_TREE_STATUS_OT_DBGRANT_SESPRN_USER      6068
#define IDS_TREE_STATUS_OT_DBGRANT_TBLSTY_USER      6069
#define IDS_TREE_STATUS_OT_DBGRANT_TBLSTN_USER      6070

#define IDS_TREE_STATUS_OT_DBEVENT                  828

#define IDS_TREE_STATUS_OT_DBALARM_CONN_SUCCESS     829
#define IDS_TREE_STATUS_OT_DBALARM_CONN_FAILURE     830
#define IDS_TREE_STATUS_OT_DBALARM_DISCONN_SUCCESS  831
#define IDS_TREE_STATUS_OT_DBALARM_DISCONN_FAILURE  832

// level 1, child of user
#define IDS_TREE_STATUS_OTR_USERSCHEMA              833
#define IDS_TREE_STATUS_OTR_USERGROUP               834
#define IDS_TREE_STATUS_OTR_ALARM_SELSUCCESS_TABLE  835
#define IDS_TREE_STATUS_OTR_ALARM_SELFAILURE_TABLE  836
#define IDS_TREE_STATUS_OTR_ALARM_DELSUCCESS_TABLE  837
#define IDS_TREE_STATUS_OTR_ALARM_DELFAILURE_TABLE  838
#define IDS_TREE_STATUS_OTR_ALARM_INSSUCCESS_TABLE  839
#define IDS_TREE_STATUS_OTR_ALARM_INSFAILURE_TABLE  840
#define IDS_TREE_STATUS_OTR_ALARM_UPDSUCCESS_TABLE  841
#define IDS_TREE_STATUS_OTR_ALARM_UPDFAILURE_TABLE  842
#define IDS_TREE_STATUS_OTR_GRANTEE_RAISE_DBEVENT   843
#define IDS_TREE_STATUS_OTR_GRANTEE_REGTR_DBEVENT   844
#define IDS_TREE_STATUS_OTR_GRANTEE_EXEC_PROC       845
#define IDS_TREE_STATUS_OTR_DBGRANT_ACCESY_DB       846
#define IDS_TREE_STATUS_OTR_DBGRANT_ACCESN_DB       847
#define IDS_TREE_STATUS_OTR_DBGRANT_CREPRY_DB       848
#define IDS_TREE_STATUS_OTR_DBGRANT_CREPRN_DB       849
#define IDS_TREE_STATUS_OTR_DBGRANT_CRETBY_DB       850
#define IDS_TREE_STATUS_OTR_DBGRANT_CRETBN_DB       851
#define IDS_TREE_STATUS_OTR_DBGRANT_DBADMY_DB       852
#define IDS_TREE_STATUS_OTR_DBGRANT_DBADMN_DB       853
#define IDS_TREE_STATUS_OTR_DBGRANT_LKMODY_DB       854
#define IDS_TREE_STATUS_OTR_DBGRANT_LKMODN_DB       855
#define IDS_TREE_STATUS_OTR_DBGRANT_QRYIOY_DB       856
#define IDS_TREE_STATUS_OTR_DBGRANT_QRYION_DB       857
#define IDS_TREE_STATUS_OTR_DBGRANT_QRYRWY_DB       858
#define IDS_TREE_STATUS_OTR_DBGRANT_QRYRWN_DB       859
#define IDS_TREE_STATUS_OTR_DBGRANT_UPDSCY_DB       860
#define IDS_TREE_STATUS_OTR_DBGRANT_UPDSCN_DB       861

#define IDS_TREE_STATUS_OTR_DBGRANT_SELSCY_DB       6081
#define IDS_TREE_STATUS_OTR_DBGRANT_SELSCN_DB       6082
#define IDS_TREE_STATUS_OTR_DBGRANT_CNCTLY_DB       6083
#define IDS_TREE_STATUS_OTR_DBGRANT_CNCTLN_DB       6084
#define IDS_TREE_STATUS_OTR_DBGRANT_IDLTLY_DB       6085
#define IDS_TREE_STATUS_OTR_DBGRANT_IDLTLN_DB       6086
#define IDS_TREE_STATUS_OTR_DBGRANT_SESPRY_DB       6087
#define IDS_TREE_STATUS_OTR_DBGRANT_SESPRN_DB       6088
#define IDS_TREE_STATUS_OTR_DBGRANT_TBLSTY_DB       6089
#define IDS_TREE_STATUS_OTR_DBGRANT_TBLSTN_DB       6090


#define IDS_TREE_STATUS_OTR_GRANTEE_SEL_TABLE       862
#define IDS_TREE_STATUS_OTR_GRANTEE_INS_TABLE       863
#define IDS_TREE_STATUS_OTR_GRANTEE_UPD_TABLE       864
#define IDS_TREE_STATUS_OTR_GRANTEE_DEL_TABLE       865
#define IDS_TREE_STATUS_OTR_GRANTEE_REF_TABLE       866

#define IDS_TREE_STATUS_OTR_GRANTEE_CPI_TABLE       6091
#define IDS_TREE_STATUS_OTR_GRANTEE_CPF_TABLE       6092

#define IDS_TREE_STATUS_OTR_GRANTEE_ALL_TABLE       867
#define IDS_TREE_STATUS_OTR_GRANTEE_SEL_VIEW        868
#define IDS_TREE_STATUS_OTR_GRANTEE_INS_VIEW        869
#define IDS_TREE_STATUS_OTR_GRANTEE_UPD_VIEW        870
#define IDS_TREE_STATUS_OTR_GRANTEE_DEL_VIEW        871
#define IDS_TREE_STATUS_OTR_GRANTEE_ROLE            872

// level 1, child of group
#define IDS_TREE_STATUS_OT_GROUPUSER                873

// level 1, child of role
#define IDS_TREE_STATUS_OT_ROLEGRANT_USER           874

// level 1, child of location
#define IDS_TREE_STATUS_OTR_LOCATIONTABLE           875

// start level 2 at 901
// level 2, child of "table of database"
#define IDS_TREE_STATUS_OT_INTEGRITY                902
#define IDS_TREE_STATUS_OT_RULE                     903
#define IDS_TREE_STATUS_OT_INDEX                    904
#define IDS_TREE_STATUS_OT_TABLELOCATION            905

#define IDS_TREE_STATUS_OT_S_ALARM_SELSUCCESS_USER  906
#define IDS_TREE_STATUS_OT_S_ALARM_SELFAILURE_USER  907
#define IDS_TREE_STATUS_OT_S_ALARM_DELSUCCESS_USER  908
#define IDS_TREE_STATUS_OT_S_ALARM_DELFAILURE_USER  909
#define IDS_TREE_STATUS_OT_S_ALARM_INSSUCCESS_USER  910
#define IDS_TREE_STATUS_OT_S_ALARM_INSFAILURE_USER  911
#define IDS_TREE_STATUS_OT_S_ALARM_UPDSUCCESS_USER  912
#define IDS_TREE_STATUS_OT_S_ALARM_UPDFAILURE_USER  913

#define IDS_TREE_STATUS_OT_TABLEGRANT_SEL_USER      914
#define IDS_TREE_STATUS_OT_TABLEGRANT_INS_USER      915
#define IDS_TREE_STATUS_OT_TABLEGRANT_UPD_USER      916
#define IDS_TREE_STATUS_OT_TABLEGRANT_DEL_USER      917
#define IDS_TREE_STATUS_OT_TABLEGRANT_REF_USER      918

#define IDS_TREE_STATUS_OT_TABLEGRANT_CPI_USER      6093
#define IDS_TREE_STATUS_OT_TABLEGRANT_CPF_USER      6094

#define IDS_TREE_STATUS_OT_TABLEGRANT_ALL_USER      919

// desktop
#define IDS_TREE_STATUS_OT_TABLEGRANT_INDEX_USER    920
#define IDS_TREE_STATUS_OT_TABLEGRANT_ALTER_USER    941

#define IDS_TREE_STATUS_OTR_TABLESYNONYM            921
#define IDS_TREE_STATUS_OTR_TABLEVIEW               922

// level 2, child of "view of database"
#define IDS_TREE_STATUS_OT_VIEWTABLE                923
#define IDS_TREE_STATUS_OTR_VIEWSYNONYM             924

#define IDS_TREE_STATUS_OT_VIEWGRANT_SEL_USER       931
#define IDS_TREE_STATUS_OT_VIEWGRANT_INS_USER       932
#define IDS_TREE_STATUS_OT_VIEWGRANT_UPD_USER       933
#define IDS_TREE_STATUS_OT_VIEWGRANT_DEL_USER       934

// level 2, child of "procedure of database"
#define IDS_TREE_STATUS_OT_PROCGRANT_EXEC_USER      935
#define IDS_TREE_STATUS_OTR_PROC_RULE               936

// level 2, child of "dbevent of database"
#define IDS_TREE_STATUS_OT_DBEGRANT_RAISE_USER      937
#define IDS_TREE_STATUS_OT_DBEGRANT_REGTR_USER      938

// level 3, child of "index on table of database"
#define IDS_TREE_STATUS_OTR_INDEXSYNONYM            939

// level 3, child of "rule on table of database"
#define IDS_TREE_STATUS_OT_RULEPROC                 940

// replicator, all levels mixed
#define IDS_TREE_STATUS_OT_REPLIC_CONNECTION        951
#define IDS_TREE_STATUS_OT_REPLIC_CDDS              952
#define IDS_TREE_STATUS_OT_REPLIC_MAILUSER          953
#define IDS_TREE_STATUS_OT_REPLIC_REGTABLE          954
#define IDS_TREE_STATUS_OT_REPLIC_SERVER            955

#define IDS_TREE_STATUS_OT_REPLIC_CDDS_DETAIL       956
#define IDS_TREE_STATUS_OTR_REPLIC_CDDS_TABLE       957
#define IDS_TREE_STATUS_OTR_REPLIC_TABLE_CDDS       958

// "Solved" types
#define IDS_TREE_STATUS_OT_GRANTEE_SOLVED_USER      961
#define IDS_TREE_STATUS_OT_GRANTEE_SOLVED_GROUP     962
#define IDS_TREE_STATUS_OT_GRANTEE_SOLVED_ROLE      963

// Classes
#define IDS_TREE_STATUS_OT_CLASSES                  964
#define IDS_TREE_STATUS_OT_SUBCLASSES               965

// new style alarms (with 2 sub-branches alarmee and launched dbevent)
#define IDS_TREE_STATUS_OT_ALARMEE                  966
#define IDS_TREE_STATUS_OT_ALARM_EVENT              967
#define IDS_TREE_STATUS_OT_ALARMEE_SOLVED_USER      968
#define IDS_TREE_STATUS_OT_ALARMEE_SOLVED_GROUP     969
#define IDS_TREE_STATUS_OT_ALARMEE_SOLVED_ROLE      970

// strings for parent reminder when child teared out
#define IDS_PARENT                                  971

//
// Strings for the configuration window
//
#define IDS_CFG_TITLE  991
#define IDS_CFG_MENU   992
#define IDS_CFG_EXIT   993

// strings for status bar messages on global toolbar buttons
#define IDS_ST_NEW                   1001    // Space oddity
#define IDS_ST_OPEN                  1002
#define IDS_ST_SAVE                  1003
#define IDS_ST_CONNECT               1004
#define IDS_ST_DISCONNECT            1005
#define IDS_ST_PREFERENCES           1006
#define IDS_ST_CUT                   1007
#define IDS_ST_COPY                  1008
#define IDS_ST_PASTE                 1009
#define IDS_ST_FIND                  1010
#define IDS_ST_SQLACT                1011
#define IDS_ST_SPACECALC             1012
#define IDS_ST_WINDOWTILE_VERT       1013

// strings for status bar messages on dom toolbar buttons
#define IDS_ST_ADDOBJECT             1102
#define IDS_ST_ALTEROBJECT           1103
#define IDS_ST_DROPOBJECT            1104
#define IDS_ST_PROPERTIES            1105
#define IDS_ST_REFRESH               1106
#define IDS_ST_NEWWINDOW             1107
#define IDS_ST_TEAROUT               1108
#define IDS_ST_RESTARTFROMPOS        1109
#define IDS_ST_PRINT                 1110
#define IDS_ST_CLASSB_EXPANDONE      1111
#define IDS_ST_CLASSB_EXPANDBRANCH   1112
#define IDS_ST_CLASSB_EXPANDALL      1113
#define IDS_ST_CLASSB_COLLAPSEONE    1114
#define IDS_ST_CLASSB_COLLAPSEBRANCH 1115
#define IDS_ST_CLASSB_COLLAPSEALL    1116

// strings for extra string caption in status bar
#define IDS_EXTRACAPT_LOCATION          1201
#define IDS_EXTRACAPT_RULE              1202
#define IDS_EXTRACAPT_INDEX             1203
#define IDS_EXTRACAPT_PROC              1204
#define IDS_EXTRACAPT_QRYIOY_USER       1205
#define IDS_EXTRACAPT_QRYRWY_USER       1206
#define IDS_EXTRACAPT_CNCTLY_USER       1207
#define IDS_EXTRACAPT_SESPRY_USER       1208
#define IDS_EXTRACAPT_IDLTLY_USER       1209
#define IDS_EXTRACAPT_SYNONYM           1210
#define IDS_EXTRACAPT_REPLIC_CONNECTION 1211
#define IDS_EXTRACAPT_REPLIC_REGTABLE   1212
#define IDS_EXTRACAPT_REPLIC_SERVER     1213

// new strings for 1.1 version
#define IDS_EXTRACAPT_REPLIC11_CONNECTION 1214

// Save/Load Error box messages
#define IDS_ERR_SAVE_OPEN         1301
#define IDS_ERR_SAVE_VERSIONID    1302
#define IDS_ERR_SAVE_SETTINGS     1303
#define IDS_ERR_SAVE_CACHE        1304
#define IDS_ERR_SAVE_DISP         1305
#define IDS_ERR_SAVE_CLOSE        1306
#define IDS_ERR_SAVE_PASSWORD     1307

#define IDS_ERR_LOAD_OPEN         1311
#define IDS_ERR_LOAD_VERSIONID    1312
#define IDS_ERR_LOAD_SETTINGS     1313
#define IDS_ERR_LOAD_CACHE        1314
#define IDS_ERR_LOAD_DISP         1315
#define IDS_ERR_LOAD_CLOSE        1316
#define IDS_ERR_LOAD_PASSWORD     1317
#define IDS_ERR_LOAD_BADPASSWORD  1318

// replicator installation messages
#define IDS_CONFIRM_INSTALL_REPLICATOR  1321
#define IDS_ERROR_INSTALL_REPLICATOR    1322

// Clipboard messages
#define IDS_ERR_CLIPBOARD_DOM_COPY      1351
#define IDS_ERR_CLIPBOARD_DOM_PASTE     1352

// Objects Drag/Drop messages - star messages added
#define IDS_CONFIRM_MV_DRAGDROP         1401
#define IDS_CONFIRM_CP_DRAGDROP         1402
#define IDS_CONFIRM_DRAGDROP_CAPT       1403
#define IDS_CONFIRM_DRAGTODDB           1404

// Background task messages
#define IDS_ERR_TIMER_BKTASK            1451

// remote command messages
#define IDS_ERR_TIMER_REMOTECMD         1461
#define IDS_ERR_MEM_REMOTECMD           1462

// Load/Save query messages
#define IDS_ERR_QUERY_LOAD_FILE         1471
#define IDS_ERR_QUERY_LOAD_MEM          1472
#define IDS_ERR_QUERY_SAVE_FILE         1473
#define IDS_ERR_QUERY_SAVE_MEM          1474

// Force refresh messages
#define IDS_FORCEREFRESH_CURSUB         1481
#define IDS_FORCEREFRESH_ALL            1482
#define IDS_FORCEREFRESH_LONG           1483

// expand messages
#define IDS_EXPANDDIRECT                1491
#define IDS_EXPANDALL                   1492
#define IDS_EXPAND_LONG                 1493

// tree resequence messages
#define IDS_RESEQUENCING_TREE           1495

// OpenIngres Desktop statistics
#define IDS_OIDTDB_UPDATESTAT_CAPT      1496
#define IDS_OIDTDB_UPDATESTAT_LONG      1497

// JFS - strings to be used in INI settings

#define IDS_INIFILENAME                 1500
#define IDS_INISETTINGS                 1501
#define IDS_INIBALLOON                  1502
#define IDS_INIBALLOONFORMAT            1503
#define IDS_INIBROWSFONT                1504
#define IDS_INIFONTFORMAT               1505
#define IDS_INIBALLOONFONT              1506
#define IDS_INICONTFONT                 1507
#define IDS_INISTATUSBAR                1508
#define IDS_INISTATUSBARFORMAT          1509
#define IDS_INISTATUSBARFONT            1510
#define IDS_INISQLACT                   1511
#define IDS_INISQLACTFORMAT             1512
#define IDS_INISQLACTFONT               1513
#define IDS_INITOOLBAR                  1514
#define IDS_INITOOLBARFORMAT            1515
#define IDS_INIDOM                      1516
#define IDS_INIDOMFORMAT                1517
#define IDS_INIBKTASK                   1518  // Emb June 10, 95
#define IDS_INIBKTASKFORMAT             1519  // Emb June 10, 95

#define IDS_INIIPMFONT                  1520
#define IDS_ININODEFONT                 1521

// strings for remote command dialog boxes
#define IDS_REMOTECMD_INPROGRESS        1601
#define IDS_REMOTECMD_FINISHED          1602
#define IDS_REMOTECMD_NOTACCEPTED       1603
#define IDS_REMOTECMD_NOANSWER          1604
#define IDS_REMOTECMD_WARNING           1605

// confirm messages
#define IDS_CONFIRM_EXIT                1701
#define IDS_CONFIRM_EXIT_CAPT           1702

// Locate messages
#define IDS_LOCATE_ERR_CAPT             1801
#define IDS_LOCATE_ERR_NOOBJ            1802
#define IDS_LOCATE_ERR_MEMORY           1803
#define IDS_LOCATE_ERR_ADDLINE          1804
#define IDS_LOCATE_ERR_WILD_UNACC       1805

// statement wizard
#define IDS_STMTWZD_CAPTION             1901
#define IDS_STMTWZD_ERR_CAPT            1902
#define IDS_STMTWZD_ERR_COLUMN          1903

// winnt registry
#define IDS_MAINREGKEY                  1904
#define IDS_REGCLASS                    1905

#define IDS_INVALIDCFGPLATFORM          1906

//
//  Strings for short status text related to static items in the tree
//
#define IDS_DATABASE_STATIC_STATUS                2001
#define IDS_PROFILE_STATIC_STATUS                 2002
#define IDS_USER_STATIC_STATUS                    2003
#define IDS_GROUP_STATIC_STATUS                   2004
#define IDS_ROLE_STATIC_STATUS                    2005
#define IDS_LOCATION_STATIC_STATUS                2006
#define IDS_SYNONYMED_STATIC_STATUS               2007
#define IDS_DBAREA_STATIC_STATUS                  1998
#define IDS_STOGROUP_STATIC_STATUS                1999

// level 1
#define IDS_TABLE_STATIC_STATUS                   2008
#define IDS_VIEW_STATIC_STATUS                    2009
#define IDS_PROCEDURE_STATIC_STATUS               2010
#define IDS_SCHEMA_STATIC_STATUS                  2011
#define IDS_SYNONYM_STATIC_STATUS                 2012
#define IDS_DBEVENT_STATIC_STATUS                 2013
#define IDS_DBGRANTEES_STATIC_STATUS              2014
#define IDS_DBGRANTEES_ACCESY_STATIC_STATUS       2015
#define IDS_DBGRANTEES_ACCESN_STATIC_STATUS       2016
#define IDS_DBGRANTEES_CREPRY_STATIC_STATUS       2017
#define IDS_DBGRANTEES_CREPRN_STATIC_STATUS       2018
#define IDS_DBGRANTEES_CRETBY_STATIC_STATUS       2019
#define IDS_DBGRANTEES_CRETBN_STATIC_STATUS       2020
#define IDS_DBGRANTEES_DBADMY_STATIC_STATUS       2021
#define IDS_DBGRANTEES_DBADMN_STATIC_STATUS       2022
#define IDS_DBGRANTEES_LKMODY_STATIC_STATUS       2023
#define IDS_DBGRANTEES_LKMODN_STATIC_STATUS       2024
#define IDS_DBGRANTEES_QRYIOY_STATIC_STATUS       2025
#define IDS_DBGRANTEES_QRYION_STATIC_STATUS       2026
#define IDS_DBGRANTEES_QRYRWY_STATIC_STATUS       2027
#define IDS_DBGRANTEES_QRYRWN_STATIC_STATUS       2028
#define IDS_DBGRANTEES_UPDSCY_STATIC_STATUS       2029
#define IDS_DBGRANTEES_UPDSCN_STATIC_STATUS       2030

#define IDS_DBGRANTEES_SELSCY_STATIC_STATUS       6101
#define IDS_DBGRANTEES_SELSCN_STATIC_STATUS       6102
#define IDS_DBGRANTEES_CNCTLY_STATIC_STATUS       6103
#define IDS_DBGRANTEES_CNCTLN_STATIC_STATUS       6104
#define IDS_DBGRANTEES_IDLTLY_STATIC_STATUS       6105
#define IDS_DBGRANTEES_IDLTLN_STATIC_STATUS       6106
#define IDS_DBGRANTEES_SESPRY_STATIC_STATUS       6107
#define IDS_DBGRANTEES_SESPRN_STATIC_STATUS       6108
#define IDS_DBGRANTEES_TBLSTY_STATIC_STATUS       6109
#define IDS_DBGRANTEES_TBLSTN_STATIC_STATUS       6110

#define IDS_DBALARM_STATIC_STATUS                 2031
#define IDS_DBALARM_CONN_SUCCESS_STATIC_STATUS    2032
#define IDS_DBALARM_CONN_FAILURE_STATIC_STATUS    2033
#define IDS_DBALARM_DISCONN_SUCCESS_STATIC_STATUS 2034
#define IDS_DBALARM_DISCONN_FAILURE_STATIC_STATUS 2035
#define IDS_GROUPUSER_STATIC_STATUS               2036
#define IDS_R_USERSCHEMA_STATIC_STATUS            2037
#define IDS_R_USERGROUP_STATIC_STATUS             2038
#define IDS_R_SECURITY_STATIC_STATUS              2039
#define IDS_R_SEC_SEL_SUCC_STATIC_STATUS          2040
#define IDS_R_SEC_SEL_FAIL_STATIC_STATUS          2041
#define IDS_R_SEC_DEL_SUCC_STATIC_STATUS          2042
#define IDS_R_SEC_DEL_FAIL_STATIC_STATUS          2043
#define IDS_R_SEC_INS_SUCC_STATIC_STATUS          2044
#define IDS_R_SEC_INS_FAIL_STATIC_STATUS          2045
#define IDS_R_SEC_UPD_SUCC_STATIC_STATUS          2046
#define IDS_R_SEC_UPD_FAIL_STATIC_STATUS          2047
#define IDS_R_GRANT_STATIC_STATUS                 2048
#define IDS_R_DBEGRANT_STATIC_STATUS              2049
#define IDS_R_DBEGRANT_RAISE_STATIC_STATUS        2050
#define IDS_R_DBEGRANT_REGISTER_STATIC_STATUS     2051
#define IDS_R_PROCGRANT_STATIC_STATUS             2052
#define IDS_R_PROCGRANT_EXEC_STATIC_STATUS        2053
#define IDS_R_DBGRANT_STATIC_STATUS               2054
#define IDS_R_DBGRANT_ACCESY_STATIC_STATUS        2055
#define IDS_R_DBGRANT_ACCESN_STATIC_STATUS        2056
#define IDS_R_DBGRANT_CREPRY_STATIC_STATUS        2057
#define IDS_R_DBGRANT_CREPRN_STATIC_STATUS        2058
#define IDS_R_DBGRANT_CRETBY_STATIC_STATUS        2059
#define IDS_R_DBGRANT_CRETBN_STATIC_STATUS        2060
#define IDS_R_DBGRANT_DBADMY_STATIC_STATUS        2061
#define IDS_R_DBGRANT_DBADMN_STATIC_STATUS        2062
#define IDS_R_DBGRANT_LKMODY_STATIC_STATUS        2063
#define IDS_R_DBGRANT_LKMODN_STATIC_STATUS        2064
#define IDS_R_DBGRANT_QRYIOY_STATIC_STATUS        2065
#define IDS_R_DBGRANT_QRYION_STATIC_STATUS        2066
#define IDS_R_DBGRANT_QRYRWY_STATIC_STATUS        2067
#define IDS_R_DBGRANT_QRYRWN_STATIC_STATUS        2068
#define IDS_R_DBGRANT_UPDSCY_STATIC_STATUS        2069
#define IDS_R_DBGRANT_UPDSCN_STATIC_STATUS        2070

#define IDS_R_DBGRANT_SELSCY_STATIC_STATUS        6121
#define IDS_R_DBGRANT_SELSCN_STATIC_STATUS        6122
#define IDS_R_DBGRANT_CNCTLY_STATIC_STATUS        6123
#define IDS_R_DBGRANT_CNCTLN_STATIC_STATUS        6124
#define IDS_R_DBGRANT_IDLTLY_STATIC_STATUS        6125
#define IDS_R_DBGRANT_IDLTLN_STATIC_STATUS        6126
#define IDS_R_DBGRANT_SESPRY_STATIC_STATUS        6127
#define IDS_R_DBGRANT_SESPRN_STATIC_STATUS        6128
#define IDS_R_DBGRANT_TBLSTY_STATIC_STATUS        6129
#define IDS_R_DBGRANT_TBLSTN_STATIC_STATUS        6130

#define IDS_R_TABLEGRANT_STATIC_STATUS            2071
#define IDS_R_TABLEGRANT_SEL_STATIC_STATUS        2072
#define IDS_R_TABLEGRANT_INS_STATIC_STATUS        2073
#define IDS_R_TABLEGRANT_UPD_STATIC_STATUS        2074
#define IDS_R_TABLEGRANT_DEL_STATIC_STATUS        2075
#define IDS_R_TABLEGRANT_REF_STATIC_STATUS        2076

#define IDS_R_TABLEGRANT_CPI_STATIC_STATUS        6131
#define IDS_R_TABLEGRANT_CPF_STATIC_STATUS        6132

#define IDS_R_TABLEGRANT_ALL_STATIC_STATUS        2077
#define IDS_R_VIEWGRANT_STATIC_STATUS             2078
#define IDS_R_VIEWGRANT_SEL_STATIC_STATUS         2079
#define IDS_R_VIEWGRANT_INS_STATIC_STATUS         2080
#define IDS_R_VIEWGRANT_UPD_STATIC_STATUS         2081
#define IDS_R_VIEWGRANT_DEL_STATIC_STATUS         2082
#define IDS_R_ROLEGRANT_STATIC_STATUS             2083
#define IDS_ROLEGRANT_USER_STATIC_STATUS          2084
#define IDS_R_LOCATIONTABLE_STATIC_STATUS         2085
#define IDS_INTEGRITY_STATIC_STATUS               2086
#define IDS_RULE_STATIC_STATUS                    2087
#define IDS_INDEX_STATIC_STATUS                   2088
#define IDS_TABLELOCATION_STATIC_STATUS           2089
#define IDS_SECURITY_STATIC_STATUS                2090
#define IDS_SEC_SEL_SUCC_STATIC_STATUS            2091
#define IDS_SEC_SEL_FAIL_STATIC_STATUS            2092
#define IDS_SEC_DEL_SUCC_STATIC_STATUS            2093
#define IDS_SEC_DEL_FAIL_STATIC_STATUS            2094
#define IDS_SEC_INS_SUCC_STATIC_STATUS            2095
#define IDS_SEC_INS_FAIL_STATIC_STATUS            2096
#define IDS_SEC_UPD_SUCC_STATIC_STATUS            2097
#define IDS_SEC_UPD_FAIL_STATIC_STATUS            2098
#define IDS_R_TABLESYNONYM_STATIC_STATUS          2099
#define IDS_R_TABLEVIEW_STATIC_STATUS             2100
#define IDS_TABLEGRANTEES_STATIC_STATUS           2101
#define IDS_TABLEGRANT_SEL_USER_STATIC_STATUS     2102
#define IDS_TABLEGRANT_INS_USER_STATIC_STATUS     2103
#define IDS_TABLEGRANT_UPD_USER_STATIC_STATUS     2104
#define IDS_TABLEGRANT_DEL_USER_STATIC_STATUS     2105
#define IDS_TABLEGRANT_REF_USER_STATIC_STATUS     2106

#define IDS_TABLEGRANT_CPI_USER_STATIC_STATUS     6133
#define IDS_TABLEGRANT_CPF_USER_STATIC_STATUS     6134

#define IDS_TABLEGRANT_ALL_USER_STATIC_STATUS     2107

// desktop
#define IDS_TABLEGRANT_INDEX_USER_STATIC_STATUS   2134
#define IDS_TABLEGRANT_ALTER_USER_STATIC_STATUS   2135

#define IDS_VIEWTABLE_STATIC_STATUS               2108
#define IDS_R_VIEWSYNONYM_STATIC_STATUS           2109
#define IDS_VIEWGRANTEES_STATIC_STATUS            2110
#define IDS_VIEWGRANT_SEL_USER_STATIC_STATUS      2111
#define IDS_VIEWGRANT_INS_USER_STATIC_STATUS      2112
#define IDS_VIEWGRANT_UPD_USER_STATIC_STATUS      2113
#define IDS_VIEWGRANT_DEL_USER_STATIC_STATUS      2114
#define IDS_PROCGRANT_EXEC_USER_STATIC_STATUS     2115
#define IDS_R_PROC_RULE_STATIC_STATUS             2116
#define IDS_DBEGRANT_RAISE_USER_STATIC_STATUS     2117
#define IDS_DBEGRANT_REGTR_USER_STATIC_STATUS     2118
#define IDS_R_INDEXSYNONYM_STATIC_STATUS          2119
#define IDS_RULEPROC_STATIC_STATUS                2120
#define IDS_REPLICATOR_STATIC_STATUS              2121
#define IDS_REPLIC_CONNECTION_STATIC_STATUS       2122
#define IDS_REPLIC_CDDS_STATIC_STATUS             2123
#define IDS_REPLIC_MAILUSER_STATIC_STATUS         2124
#define IDS_REPLIC_REGTABLE_STATIC_STATUS         2125
#define IDS_REPLIC_SERVER_STATIC_STATUS           2126
#define IDS_REPLIC_CDDS_DETAIL_STATIC_STATUS      2127
#define IDS_R_REPLIC_CDDS_TABLE_STATIC_STATUS     2128
#define IDS_R_REPLIC_TABLE_CDDS_STATIC_STATUS     2129
#define IDS_CLASSES_STATIC_STATUS                 2130
#define IDS_SUBCLASSES_STATIC_STATUS              2131
#define IDS_ALARMEE_STATIC_STATUS                 2132
#define IDS_ALARM_EVENT_STATIC_STATUS             2133

//
// memory shortage specific error messages
// see gMemErrFlag management in esltools.c
//

#define IDS_E_MEMERR_STARTUP            2201
#define IDS_E_MEMERR_LOADCACHE          2202
#define IDS_E_MEMERR_LOADPROPS          2203
#define IDS_E_MEMERR_LOADDISP           2204
#define IDS_E_MEMERR_REFRESHONLOAD      2205
#define IDS_E_MEMERR_NEWDOM             2206
#define IDS_E_MEMERR_EXPANDONE          2207
#define IDS_E_MEMERR_EXPANDBRANCH       2208
#define IDS_E_MEMERR_EXPANDALL          2209
#define IDS_E_MEMERR_SHOWSYSTEM         2210
#define IDS_E_MEMERR_PRINTDOM           2211
#define IDS_E_MEMERR_SHOWPROPS          2212
#define IDS_E_MEMERR_INSTALLREPLIC      2213
#define IDS_E_MEMERR_BUILDSERVER        2214
#define IDS_E_MEMERR_FORCEREFRESH       2215
#define IDS_E_MEMERR_FILTERCHANGE       2216
#define IDS_E_MEMERR_BASEFILTERCHANGE   2217
#define IDS_E_MEMERR_OTHERFILTERCHANGE  2218
#define IDS_E_MEMERR_QUERYGO            2219
#define IDS_E_MEMERR_PRINTSQLACT        2220
#define IDS_E_MEMERR_STANDARD           2221
#define IDS_E_MEMERR_DBAGETFIRSTOBJECT  2222
#define IDS_E_MEMERR_PRINTCONTAINER     2223
#define IDS_E_MEMERR_SQLACT             2224

//
// log file messages
//
#define IDS_E_SQLERR_STMT               2251
#define IDS_E_SQLERR_ERRNUM             2252
#define IDS_E_SQLERR_ERRTXT             2253

//
// for OpenIngres Desktop - duplicate sources
//
#define IDS_TREE_OIDTEVENT_STATIC               2301
#define IDS_OIDTEVENT_STATIC_STATUS             2302
#define IDS_TREE_OIDTEVENT_NONE                 2303
#define IDS_TREE_OIDTGRANTEES_ACCESY_STATIC     2304
#define IDS_OIDTGRANTEES_ACCESY_STATIC_STATUS   2305
#define IDS_TREE_OIDTGRANTEES_ACCESY_NONE       2306
#define IDS_TREE_OIDTGRANTEES_CRETBY_STATIC     2307
#define IDS_OIDTGRANTEES_CRETBY_STATIC_STATUS   2308
#define IDS_TREE_OIDTGRANTEES_CRETBY_NONE       2309
#define IDS_TREE_OIDTGRANTEES_DBADMY_STATIC     2310
#define IDS_OIDTGRANTEES_DBADMY_STATIC_STATUS   2311
#define IDS_TREE_OIDTGRANTEES_DBADMY_NONE       2312
#define IDS_TREE_OIDTRULE_STATIC                2313
#define IDS_OIDTRULE_STATIC_STATUS              2314
#define IDS_TREE_OIDTRULE_NONE                  2315
#define IDS_TREE_R_PROC_OIDTRULE_STATIC         2316
#define IDS_R_PROC_OIDTRULE_STATIC_STATUS       2317
#define IDS_TREE_R_PROC_OIDTRULE_NONE           2318
#define IDS_TREE_OIDTRULEPROC_STATIC            2319
#define IDS_OIDTRULEPROC_STATIC_STATUS          2320
#define IDS_TREE_OIDTRULEPROC_NONE              2321

#define IDS_TREE_STATUS_OT_OIDTDBGRANT_ACCESY_USER  2322
#define IDS_TREE_STATUS_OT_OIDTDBGRANT_CRETBY_USER  2323
#define IDS_TREE_STATUS_OT_OIDTDBGRANT_DBADMY_USER  2324
#define IDS_TREE_STATUS_OT_OIDTEVENT                2325
#define IDS_TREE_STATUS_OT_OIDTRULE                 2326
#define IDS_TREE_STATUS_OTR_PROC_OIDTRULE           2327
#define IDS_TREE_STATUS_OT_OIDTRULEPROC             2328

#define IDS_EXTRACAPT_OIDTRULE                      2329
#define IDS_E_RUNONLYBY_INGRES_OR_DBA               2330

// Star management: CDB as a sub branch for a DDB
#define IDS_TREE_R_CDB_STATIC                       2331
#define IDS_TREE_R_CDB_STATIC_STATUS                2332
#define IDS_TREE_R_CDB_NONE                         2333
#define IDS_TREE_STATUS_OTR_CDB                     2334

// tree lines of level 2, children of 'schema' on database 
#define IDS_TREE_SCHEMAUSER_TABLE_STATIC          2335
#define IDS_SCHEMAUSER_TABLE_STATIC_STATUS        2336
#define IDS_TREE_SCHEMAUSER_TABLE_NONE            2337
#define IDS_TREE_SCHEMAUSER_VIEW_STATIC           2338
#define IDS_SCHEMAUSER_VIEW_STATIC_STATUS         2339
#define IDS_TREE_SCHEMAUSER_VIEW_NONE             2340
#define IDS_TREE_SCHEMAUSER_PROCEDURE_STATIC      2341
#define IDS_SCHEMAUSER_PROCEDURE_STATIC_STATUS    2342
#define IDS_TREE_SCHEMAUSER_PROCEDURE_NONE        2343

#define IDS_TREE_STATUS_OT_SCHEMAUSER_TABLE       2344
#define IDS_TREE_STATUS_OT_SCHEMAUSER_VIEW        2345
#define IDS_TREE_STATUS_OT_SCHEMAUSER_PROCEDURE   2346

//
// ICE
//
#define IDS_TREE_ICE_STATIC                       2347
#define IDS_TREE_ICE_STATIC_STATUS                2348
#define IDS_TREE_ICE_NONE                         2349
#define IDS_TREE_STATUS_OT_ICE                    2350
// Under "Security"
#define IDS_TREE_ICE_SECURITY_STATIC              2351
#define IDS_TREE_ICE_SECURITY_STATIC_STATUS       2352
#define IDS_TREE_ICE_SECURITY_NONE                2353
#define IDS_TREE_STATUS_OT_ICE_SECURITY           2354

#define IDS_TREE_ICE_DBUSER_STATIC                2355
#define IDS_TREE_ICE_DBUSER_STATIC_STATUS         2356
#define IDS_TREE_ICE_DBUSER_NONE                  2357
#define IDS_TREE_STATUS_OT_ICE_DBUSER             2358

#define IDS_TREE_ICE_DBCONNECTION_STATIC          2359
#define IDS_TREE_ICE_DBCONNECTION_STATIC_STATUS   2360
#define IDS_TREE_ICE_DBCONNECTION_NONE            2361
#define IDS_TREE_STATUS_OT_ICE_DBCONNECTION       2362

#define IDS_TREE_ICE_WEBUSER_STATIC               2363
#define IDS_TREE_ICE_WEBUSER_STATIC_STATUS        2364
#define IDS_TREE_ICE_WEBUSER_NONE                 2365
#define IDS_TREE_STATUS_OT_ICE_WEBUSER            2366

#define IDS_TREE_ICE_WEBUSER_ROLE_STATIC          2367
#define IDS_TREE_ICE_WEBUSER_ROLE_STATIC_STATUS   2368
#define IDS_TREE_ICE_WEBUSER_ROLE_NONE            2369
#define IDS_TREE_STATUS_OT_ICE_WEBUSER_ROLE       2370

#define IDS_TREE_ICE_WEBUSER_CONNECTION_STATIC          2371
#define IDS_TREE_ICE_WEBUSER_CONNECTION_STATIC_STATUS   2372
#define IDS_TREE_ICE_WEBUSER_CONNECTION_NONE            2373
#define IDS_TREE_STATUS_OT_ICE_WEBUSER_CONNECTION       2374

#define IDS_TREE_ICE_PROFILE_STATIC               2375
#define IDS_TREE_ICE_PROFILE_STATIC_STATUS        2376
#define IDS_TREE_ICE_PROFILE_NONE                 2377
#define IDS_TREE_STATUS_OT_ICE_PROFILE            2378

#define IDS_TREE_ICE_PROFILE_ROLE_STATIC          2379
#define IDS_TREE_ICE_PROFILE_ROLE_STATIC_STATUS   2380
#define IDS_TREE_ICE_PROFILE_ROLE_NONE            2381
#define IDS_TREE_STATUS_OT_ICE_PROFILE_ROLE       2382

#define IDS_TREE_ICE_PROFILE_CONNECTION_STATIC          2383
#define IDS_TREE_ICE_PROFILE_CONNECTION_STATIC_STATUS   2384
#define IDS_TREE_ICE_PROFILE_CONNECTION_NONE            2385
#define IDS_TREE_STATUS_OT_ICE_PROFILE_CONNECTION       2386

// Under "Bussiness unit" (BUNIT)
#define IDS_TREE_ICE_BUNIT_STATIC                 2387
#define IDS_TREE_ICE_BUNIT_STATIC_STATUS          2388
#define IDS_TREE_ICE_BUNIT_NONE                   2389
#define IDS_TREE_STATUS_OT_ICE_BUNIT              2390

#define IDS_TREE_ICE_BUNIT_SECURITY_STATIC          2391
#define IDS_TREE_ICE_BUNIT_SECURITY_STATIC_STATUS   2392
#define IDS_TREE_ICE_BUNIT_SECURITY_NONE            2393
#define IDS_TREE_STATUS_OT_ICE_BUNIT_SECURITY       2394

#define IDS_TREE_ICE_BUNIT_SEC_ROLE_STATIC          2395
#define IDS_TREE_ICE_BUNIT_SEC_ROLE_STATIC_STATUS   2396
#define IDS_TREE_ICE_BUNIT_SEC_ROLE_NONE            2397
#define IDS_TREE_STATUS_OT_ICE_BUNIT_SEC_ROLE       2398

#define IDS_TREE_ICE_BUNIT_SEC_USER_STATIC          2399
#define IDS_TREE_ICE_BUNIT_SEC_USER_STATIC_STATUS   2400
#define IDS_TREE_ICE_BUNIT_SEC_USER_NONE            2401
#define IDS_TREE_STATUS_OT_ICE_BUNIT_SEC_USER       2402

#define IDS_TREE_ICE_BUNIT_FACET_STATIC             2403
#define IDS_TREE_ICE_BUNIT_FACET_STATIC_STATUS      2404
#define IDS_TREE_ICE_BUNIT_FACET_NONE               2405
#define IDS_TREE_STATUS_OT_ICE_BUNIT_FACET          2406

#define IDS_TREE_ICE_BUNIT_PAGE_STATIC              2407
#define IDS_TREE_ICE_BUNIT_PAGE_STATIC_STATUS       2408
#define IDS_TREE_ICE_BUNIT_PAGE_NONE                2409
#define IDS_TREE_STATUS_OT_ICE_BUNIT_PAGE           2410

#define IDS_TREE_ICE_BUNIT_LOCATION_STATIC          2411
#define IDS_TREE_ICE_BUNIT_LOCATION_STATIC_STATUS   2412
#define IDS_TREE_ICE_BUNIT_LOCATION_NONE            2413
#define IDS_TREE_STATUS_OT_ICE_BUNIT_LOCATION       2414

// Under "Server"
#define IDS_TREE_ICE_SERVER_STATIC                 2415
#define IDS_TREE_ICE_SERVER_STATIC_STATUS          2416
#define IDS_TREE_ICE_SERVER_NONE                   2417
#define IDS_TREE_STATUS_OT_ICE_SERVER              2418

#define IDS_TREE_ICE_SERVER_APPLICATION_STATIC          2419
#define IDS_TREE_ICE_SERVER_APPLICATION_STATIC_STATUS   2420
#define IDS_TREE_ICE_SERVER_APPLICATION_NONE            2421
#define IDS_TREE_STATUS_OT_ICE_SERVER_APPLICATION       2422

#define IDS_TREE_ICE_SERVER_LOCATION_STATIC         2423
#define IDS_TREE_ICE_SERVER_LOCATION_STATIC_STATUS  2424
#define IDS_TREE_ICE_SERVER_LOCATION_NONE           2425
#define IDS_TREE_STATUS_OT_ICE_SERVER_LOCATION      2426

#define IDS_TREE_ICE_SERVER_VARIABLE_STATIC         2427
#define IDS_TREE_ICE_SERVER_VARIABLE_STATIC_STATUS  2428
#define IDS_TREE_ICE_SERVER_VARIABLE_NONE           2429
#define IDS_TREE_STATUS_OT_ICE_SERVER_VARIABLE      2430

// Added later
#define IDS_TREE_ICE_ROLE_STATIC                    2431
#define IDS_TREE_ICE_ROLE_STATIC_STATUS             2432
#define IDS_TREE_ICE_ROLE_NONE                      2433
#define IDS_TREE_STATUS_OT_ICE_ROLE                 2434

#define IDS_TREE_ICE_BUNIT_FACET_ROLE_STATIC        2435
#define IDS_TREE_ICE_BUNIT_FACET_ROLE_STATIC_STATUS 2436
#define IDS_TREE_ICE_BUNIT_FACET_ROLE_NONE          2437
#define IDS_TREE_STATUS_OT_ICE_BUNIT_FACET_ROLE     2438

#define IDS_TREE_ICE_BUNIT_FACET_USER_STATIC        2439
#define IDS_TREE_ICE_BUNIT_FACET_USER_STATIC_STATUS 2440
#define IDS_TREE_ICE_BUNIT_FACET_USER_NONE          2441
#define IDS_TREE_STATUS_OT_ICE_BUNIT_FACET_USER     2442

#define IDS_TREE_ICE_BUNIT_PAGE_ROLE_STATIC         2443
#define IDS_TREE_ICE_BUNIT_PAGE_ROLE_STATIC_STATUS  2444
#define IDS_TREE_ICE_BUNIT_PAGE_ROLE_NONE           2445
#define IDS_TREE_STATUS_OT_ICE_BUNIT_PAGE_ROLE      2446

#define IDS_TREE_ICE_BUNIT_PAGE_USER_STATIC         2447
#define IDS_TREE_ICE_BUNIT_PAGE_USER_STATIC_STATUS  2448
#define IDS_TREE_ICE_BUNIT_PAGE_USER_NONE           2449
#define IDS_TREE_STATUS_OT_ICE_BUNIT_PAGE_USER      2450

//
// INSTALLATION LEVEL SETTINGS
//
#define IDS_TREE_INSTALL_STATIC                     2501
#define IDS_TREE_INSTALL_STATIC_STATUS              2502
#define IDS_TREE_INSTALL_NONE                       2503
#define IDS_TREE_STATUS_OT_INSTALL                  2504

#define IDS_TREE_INSTALL_SECURITY_STATIC                    2505
#define IDS_TREE_INSTALL_SECURITY_STATIC_STATUS             2506
#define IDS_TREE_INSTALL_SECURITY_NONE                      2507
#define IDS_TREE_STATUS_OT_INSTALL_SECURITY                 2508

#define IDS_TREE_INSTALL_GRANTEES_STATIC                    2509
#define IDS_TREE_INSTALL_GRANTEES_STATIC_STATUS             2510
#define IDS_TREE_INSTALL_GRANTEES_NONE                      2511
#define IDS_TREE_STATUS_OT_INSTALL_GRANTEES                 2512

#define IDS_TREE_INSTALL_ALARMS_STATIC                    2513
#define IDS_TREE_INSTALL_ALARMS_STATIC_STATUS             2514
#define IDS_TREE_INSTALL_ALARMS_NONE                      2515
#define IDS_TREE_STATUS_OT_INSTALL_ALARMS                 2516

//
// UNDOCUMENTED GRANTS
//
#define IDS_TREE_DBGRANTEES_QRYCPY_STATIC   6151
#define IDS_TREE_DBGRANTEES_QRYCPY_NONE     6152
#define IDS_TREE_DBGRANTEES_QRYCPN_STATIC   6153
#define IDS_TREE_DBGRANTEES_QRYCPN_NONE     6154
#define IDS_TREE_DBGRANTEES_QRYPGY_STATIC   6155
#define IDS_TREE_DBGRANTEES_QRYPGY_NONE     6156
#define IDS_TREE_DBGRANTEES_QRYPGN_STATIC   6157
#define IDS_TREE_DBGRANTEES_QRYPGN_NONE     6158
#define IDS_TREE_DBGRANTEES_QRYCOY_STATIC   6159
#define IDS_TREE_DBGRANTEES_QRYCOY_NONE     6160
#define IDS_TREE_DBGRANTEES_QRYCON_STATIC   6161
#define IDS_TREE_DBGRANTEES_QRYCON_NONE     6162

#define IDS_TREE_R_DBGRANT_QRYCPY_STATIC    6163
#define IDS_TREE_R_DBGRANT_QRYCPY_NONE      6164
#define IDS_TREE_R_DBGRANT_QRYCPN_STATIC    6165
#define IDS_TREE_R_DBGRANT_QRYCPN_NONE      6166
#define IDS_TREE_R_DBGRANT_QRYPGY_STATIC    6167
#define IDS_TREE_R_DBGRANT_QRYPGY_NONE      6168
#define IDS_TREE_R_DBGRANT_QRYPGN_STATIC    6169
#define IDS_TREE_R_DBGRANT_QRYPGN_NONE      6170
#define IDS_TREE_R_DBGRANT_QRYCOY_STATIC    6171
#define IDS_TREE_R_DBGRANT_QRYCOY_NONE      6172
#define IDS_TREE_R_DBGRANT_QRYCON_STATIC    6173
#define IDS_TREE_R_DBGRANT_QRYCON_NONE      6174

#define IDS_TREE_STATUS_OT_DBGRANT_QRYCPY_USER      6175
#define IDS_TREE_STATUS_OT_DBGRANT_QRYCPN_USER      6176
#define IDS_TREE_STATUS_OT_DBGRANT_QRYPGY_USER      6177
#define IDS_TREE_STATUS_OT_DBGRANT_QRYPGN_USER      6178
#define IDS_TREE_STATUS_OT_DBGRANT_QRYCOY_USER      6179
#define IDS_TREE_STATUS_OT_DBGRANT_QRYCON_USER      6180

#define IDS_TREE_STATUS_OTR_DBGRANT_QRYCPY_DB       6181
#define IDS_TREE_STATUS_OTR_DBGRANT_QRYCPN_DB       6182
#define IDS_TREE_STATUS_OTR_DBGRANT_QRYPGY_DB       6183
#define IDS_TREE_STATUS_OTR_DBGRANT_QRYPGN_DB       6184
#define IDS_TREE_STATUS_OTR_DBGRANT_QRYCOY_DB       6185
#define IDS_TREE_STATUS_OTR_DBGRANT_QRYCON_DB       6186

#define IDS_DBGRANTEES_QRYCPY_STATIC_STATUS       6187
#define IDS_DBGRANTEES_QRYCPN_STATIC_STATUS       6188
#define IDS_DBGRANTEES_QRYPGY_STATIC_STATUS       6189
#define IDS_DBGRANTEES_QRYPGN_STATIC_STATUS       6190
#define IDS_DBGRANTEES_QRYCOY_STATIC_STATUS       6191
#define IDS_DBGRANTEES_QRYCON_STATIC_STATUS       6192

#define IDS_R_DBGRANT_QRYCPY_STATIC_STATUS        6193
#define IDS_R_DBGRANT_QRYCPN_STATIC_STATUS        6194
#define IDS_R_DBGRANT_QRYPGY_STATIC_STATUS        6195
#define IDS_R_DBGRANT_QRYPGN_STATIC_STATUS        6196
#define IDS_R_DBGRANT_QRYCOY_STATIC_STATUS        6197
#define IDS_R_DBGRANT_QRYCON_STATIC_STATUS        6198


// messages added into resources. March/April 1999
#define IDS_MSG_INSTALL_NOT_STARTED				7001
#define IDS_MSG_NODE_EQUAL_LOCALVNODE			7002
#define IDS_ERR_MORETH1_REPSERV_DIFFLDB			7003
#define IDS_ERR_REPSERV_STATUS_UNAVAILABLE		7004
#define IDS_REFRESH_REPLICSERVER_STATUS			7005
#define IDS_ERR_UNABLE2GET_REPSERV_STATUS		7006
#define IDS_MSG_ONLOAD_SHOULDREFRESH_MON		7007
#define IDS_TITLE_PERF_MON_WINDOWS				7008
#define IDS_ERR_SOME_INFO_NOT_REFRESHED			7009
#define IDS_ERR_CONNECTLOCAL_OPINGDTUSED		7010
#define IDS_ERR_ONLY_ING_OR_STAR_CANBESTPD		7011
#define IDS_ERR_CONNECTFAILURE_REGUSER			7012
#define IDS_ERR_CANNOT_SCAN_DBEV_CLOSEW			7013
#define IDS_ERR_CANNOT_SCAN_DBEV_MON_NOTUTD		7014

#define IDS_OBJTYPES_NODES						7101
#define IDS_OBJTYPES_DATABASES					7102
#define IDS_OBJTYPES_USERS						7103
#define IDS_OBJTYPES_PROFILES					7104
#define IDS_OBJTYPES_STORAGE_GRPS				7105
#define IDS_OBJTYPES_DBAREAS					7106
#define IDS_OBJTYPES_GROUPS						7107
#define IDS_OBJTYPES_GROUPUSERS					7108
#define IDS_OBJTYPES_ROLES						7109
#define IDS_OBJTYPES_LOCATIONS					7110
#define IDS_OBJTYPES_TABLES						7111
#define IDS_OBJTYPES_TABLE_LOCATIONS			7112
#define IDS_OBJTYPES_VIEWS						7113
#define IDS_OBJTYPES_VIEWCOMPONENTS				7114
#define IDS_OBJTYPES_INDEXES					7115
#define IDS_OBJTYPES_INTEGRITIES				7116
#define IDS_OBJTYPES_PROCEDURES					7117
#define IDS_OBJTYPES_TRIGGERS					7118
#define IDS_OBJTYPES_RULES						7119
#define IDS_OBJTYPES_SCHEMAS					7120
#define IDS_OBJTYPES_SYNONYMS					7121
#define IDS_OBJTYPES_EVENTS						7122
#define IDS_OBJTYPES_DBEVENTS					7123
#define IDS_OBJTYPES_SECALARMS					7124
#define IDS_OBJTYPES_GRANTEES					7125
#define IDS_OBJTYPES_DBGRANTEES					7126
#define IDS_OBJTYPES_DBSECALARMS				7127
#define IDS_OBJTYPES_GRANTS						7128
#define IDS_OBJTYPES_COLUMNS					7129
#define IDS_OBJTYPES_REP_OBJECTS				7130
#define IDS_OBJTYPES_REP_CDDS					7131
#define IDS_OBJTYPES_REP_REGTBLS				7132
#define IDS_OBJTYPES_ERRLIST_USRNMS				7133
#define IDS_OBJTYPES_REP_SERVERS				7134
#define IDS_OBJTYPES_MON_DATA					7135
#define IDS_OBJTYPES_NODE_DEF_DATA				7136
#define IDS_OBJTYPES_ICE_ROLES					7137
#define IDS_OBJTYPES_ICE_DBUSERS				7138
#define IDS_OBJTYPES_ICE_DBCONNECTS				7139
#define IDS_OBJTYPES_ICE_WEBUSRS				7140
#define IDS_OBJTYPES_ICE_WEBUSRROLES			7141
#define IDS_OBJTYPES_ICE_WEBUSERDBCONNS			7142
#define IDS_OBJTYPES_ICE_PROFILES				7143
#define IDS_OBJTYPES_ICE_PROF_ROLES				7144
#define IDS_OBJTYPES_ICE_PROF_CONNS				7145
#define IDS_OBJTYPES_ICE_BUS					7146
#define IDS_OBJTYPES_ICE_BU_ROLES				7147
#define IDS_OBJTYPES_ICE_BU_USERS				7148
#define IDS_OBJTYPES_ICE_BU_FACETS				7149
#define IDS_OBJTYPES_ICE_BU_PAGES				7150
#define IDS_OBJTYPES_ICE_BU_LOCATIONS			7151
#define IDS_OBJTYPES_ICE_SVR_B_GROUPS			7152
#define IDS_OBJTYPES_ICE_LOCATIONS				7153
#define IDS_OBJTYPES_ICE_VARIABLES				7154
#define IDS_OBJTYPES_ICE_OBJECT_DEFS			7155

#define IDS_ASSIST_EXPRESSION				7156
#define IDS_ASSIST_OPT_LENGTH				7157
#define IDS_ASSIST_OPT_PREC					7158
#define IDS_ASSIST_OPT_SCALE				7159
#define IDS_ASSIST_ARGUMENT					7160
#define IDS_ASSIST_STRING					7161
#define IDS_ASSIST_STRING1					7162
#define IDS_ASSIST_STRING2					7163
#define IDS_ASSIST_NB_CHARS					7164
#define IDS_ASSIST_MODULO					7165
#define IDS_ASSIST_OCC_IN_STRING			7166
#define IDS_ASSIST_NB_PLACES				7167
#define IDS_ASSIST_UNIT						7168
#define IDS_ASSIST_DATE						7169
#define IDS_ASSIST_DATE_INTERVAL			7170
#define IDS_ASSIST_CHAR_STRING				7171
#define IDS_ASSIST_PREDICATE				7172
#define IDS_ASSIST_CHECKED_ARGS				7173
#define IDS_ASSIST_RETVAL_IFNULL			7174
#define IDS_ASSIST_PATTERN					7175
#define IDS_ASSIST_ESCAPE_CHAR				7176
#define IDS_ASSIST_CHECKED4NULL				7177
#define IDS_ASSIST_SELECT_STMT				7178
#define IDS_ASSIST_COMP_OPERATOR			7179
#define IDS_ASSIST_SUBQUERY					7180
#define IDS_ASSIST_NUMBER					7181
#define IDS_ASSIST_DBAREA					7182
#define IDS_ASSIST_STO_GROUP				7183
#define IDS_ASSIST_COL						7184
#define IDS_ASSIST_TABLE					7185
#define IDS_ASSIST_DATABASE					7186
#define IDS_ASSIST_USER						7187
#define IDS_ASSIST_GROUP					7188
#define IDS_ASSIST_ROLE						7189
#define IDS_ASSIST_PROFILE					7190
#define IDS_ASSIST_LOCATION					7191
#define IDS_ASSIST_X_RAD					7192
#define IDS_ASSIST_POS_ARG					7193
#define IDS_ASSIST_POSORNULL_X				7194
#define IDS_ASSIST_INT_RATE					7195
#define IDS_ASSIST_FUT_VALUE				7196
#define IDS_ASSIST_PRES_VALUE				7197
#define IDS_ASSIST_PMT						7198
#define IDS_ASSIST_INT						7199
#define IDS_ASSIST_N						7200
#define IDS_ASSIST_PRINCIPAL				7201
#define IDS_ASSIST_INTEREST					7202
#define IDS_ASSIST_PERIODS					7203
#define IDS_ASSIST_N_PERIODS				7204
#define IDS_ASSIST_COST						7205
#define IDS_ASSIST_SALVAGE					7206
#define IDS_ASSIST_LIFE						7207
#define IDS_ASSIST_FV						7208
#define IDS_ASSIST_YEAR						7209
#define IDS_ASSIST_MONTH					7210
#define IDS_ASSIST_DAY						7211
#define IDS_ASSIST_PICTURE					7212
#define IDS_ASSIST_DATE_STRING				7213
#define IDS_ASSIST_HOUR						7214
#define IDS_ASSIST_MINUTE					7215
#define IDS_ASSIST_SECOND					7216
#define IDS_ASSIST_TIME						7217
#define IDS_ASSIST_POS						7218
#define IDS_ASSIST_START					7219
#define IDS_ASSIST_LENGTH					7220
#define IDS_ASSIST_SCALE					7221

#define IDS_NONE_							7222
#define IDS_ERR_NOPRIMKEY_CANT_BE_REF		7223

#define IDS_UNAMED_W_NB						7224

#define IDS_OIDT_IICOPY_STEP1				7225
#define IDS_OIDT_IICOPY_STEP2				7226
#define IDS_OIDT_IICOPY_STEP3				7227
#define IDS_OIDT_IICOPY_STEP23				7228
#define IDS_OIDT_IICOPY_STEPVIEW			7229

#define IDS_OIDT_ADDLOCALVAR				7230

#define IDS_REPLIC_FULL_PEER				7231
#define IDS_REPLIC_PROT_RO					7232
#define IDS_REPLIC_UNPROT_RO				7233
#define IDS_REPLIC_UNDEFINED				7234

#define IDS_REPLIC_PROP_DLGTITLE			7235
#define IDS_REPLIC_DB_NOTFOUND_PROPSTP		7236
#define IDS_REPLIC_INSTALL_REPL				7237
#define IDS_REPLIC_ERR_OPN_SESS				7238
#define IDS_REPLIC_REC_1PARM_TITLE			7239
#define IDS_REPLIC_REC_2PARMS_TITLE			7240

#define IDS_CURRENT_INSTALLATION			7241

#define IDS_DRAGANDDROP_DEST_ERR			7242
#define IDS_OF								7243
#define IDS_ON_								7244
#define IDS_OFF_							7245



#define IDS_ERR_DUPKEY                            8001
#define IDS_ERR_ONECOLMAX_RTREE                   8002
#define IDS_ERR_LOG_USER                          8003
#define IDS_ERR_CDDS_PATH                         8004
#define IDS_ERR_IDENTICAL_PATH                    8005
#define IDS_F_SAMESOURCE_SAMETARGET               8006
#define IDS_F_MESSAGE_ONE                         8007
#define IDS_F_MESSAGE_TWO                         8008
#define IDS_F_MESSAGE_THREE                       8009
#define IDS_TITLE_WARNING                         8010
#define IDS_F_MESSAGE_FOUR                        8011
#define IDS_F_MESSAGE_FIVE                        8012
#define IDS_ERR_MESSAGE_SIX                       8013
#define IDS_F_MESSAGE_SEVEN                       8014
#define IDS_F_MESSAGE_EIGHT                       8015
#define IDS_F_MESSAGE_NINE                        8016
#define IDS_F_MESSAGE_TEN                         8017
#define IDS_F_MESSAGE_ELEVEN                      8018
#define IDS_ERR_CREATE_TBL_AS_SELECT              8019
#define IDS_F_DUPLICATE_KEY                       8020
#define IDS_E_ALTER_RULE                          8021
#define IDS_TITLE_MESSAGE                         8022
#define IDS_ERR_EXCLUDING_REFERENCE               8023
#define IDS_ERR_EXCLUDING_GRANT                   8024
#define IDS_TITLE_GRANT_CUR_INSTALL               8025
#define IDS_ERR_MODIF_TBL                         8026
#define IDS_ERR_MODIF_PHYS                        8027
#define IDS_ERR_MODIF_LOG                         8028
#define IDS_ERR_MODIF_RECOVERY                    8029
#define IDS_ERR_NB_CONNECT_REACHED                8030
#define IDS_ERR_CLOSE_SESSION                     8031
#define IDS_ERR_CREATE_DD_SERVER_SPECIAL          8032
#define IDS_ERR_DELETE_DD_SERVER_SPECIAL          8033
#define IDS_ERR_COPY_DD_SERVER_SPECIAL            8034
#define IDS_F_PROPAGATE_SVR_TYPE                  8035
#define IDS_ERR_LOCATION_LIST                     8036
#define IDS_ERR_NEW_LOCATION_LIST                 8037
#define IDS_ERR_USING_CHECKPOINT                  8038
#define IDS_ERR_MULTIPLE_AUTH_ID                  8039
#define IDS_ERR_TABLE_BE_CHECKED                  8040
#define IDS_ERR_OPTIONAL_ALARM_NAME               8041
#define IDS_F_CREATE_SECURITY                     8042
#define IDS_ERR_DATABASE_BE_CHECKED               8043
#define IDS_ERR_CREATE_SCHEMA                     8044
#define IDS_ERR_OPTION_RUN_MODE                   8045
#define IDS_ERR_SELECTED_MOBILE                   8046
#define IDS_TITLE_CONFIRMATION                    8047
#define IDS_ERR_DROP_COL                          8048
#define IDS_ERR_COLUMN_USED                       8049
#define IDS_TITLE_CONFIRM                         8050
#define IDS_ERR_INVALID_TBL_NAME                  8051
#define IDS_ERR_PCTFREE                           8052
#define IDS_ERR_SPECIFIED_OPTION                  8053
#define IDS_ERR_SAME_FOREIGN_KEY                  8054
#define IDS_ERR_CONSTRAINT_NAME                   8055
#define IDS_ERR_FOREIGN_KEY_EXISTS                8056
#define IDS_ERR_TEMP_STORAGE_FILE                 8057
#define IDS_ERR_CREATEPROCESS                     8058
#define IDS_ERR_START_EDITOR                      8059
#define IDS_ERR_LAUNCH_EDITOR                     8060
#define IDS_ERR_OPEN_LOG                          8061
#define IDS_ERR_PERFORM_DOWNLOAD                  8062
#define IDS_ERR_SELECTED_DIR                      8063
#define IDS_ERR_ALREADY_EXIST                     8064
#define IDS_TITLE_DOWNLOAD                        8065
#define IDS_ERR_OLD_FILE_EXISTS                   8066
#define IDS_ERR_TEMP_LIST_FILE                    8067
#define IDS_ERR_TBL_SELECTED                      8068
#define IDS_ERR_LOAD_OPERATION                    8069
#define IDS_TITLE_PERFORM_RUNSCRIPTS              8070
#define IDS_ERR_PERFORM_RUNSCRIPTS                8071
#define IDS_ERR_CHECK_CHECKBOX                    8072
#define IDS_ERR_NODE_DB_NAME                      8073
#define IDS_ERR_ACCESS_NODE                       8074
#define IDS_ERR_ACCESS_DATABASE                   8075
#define IDS_ERR_DAT_FILES                         8076
#define IDS_ERR_ACCESSING_INDEX                   8077
#define IDS_TITLE_ACCESSING_INDEX                 8078
#define IDS_ERR_UPDATE_STATSUB_OPE                8079
#define IDS_TITLE_UPDATE_STATSUB_OPE              8080
#define IDS_ERR_PERFORM_UNLOAD_OPE                8081
#define IDS_TITLE_PERFORM_UNLOAD_OPE              8082
#define IDS_TITLE_LOAD_OPERATION                  8083
#define IDS_ERR_PERFORM_UPDATE                    8084
#define IDS_TITLE_TBL_PERFORM_UPDATE              8085
#define IDS_TITLE_INDEX_PERFORM_UPDATE            8086
#define IDS_F_OPEN_REPORT                         8087
#define IDS_F_OPENING_SESSION                     8088
#define IDS_ERR_CLOSE_TEMP_FILE                   8089
//#define IDS_ERR_RELEASE_SESSION                   8090
#define IDS_F_COLUMN_INFO                         8091
#define IDS_ERR_BLANK_TABLE_NAME                  8092
#define IDS_F_TBL_NUMBER                          8093
#define IDS_F_INVALID_OBJECT                      8094
#define IDS_ERR_OPENING_CURSOR                    8095
#define IDS_F_FETCHING_DATA                       8096
#define IDS_ERR_SESSION_NUMBER                    8097
#define IDS_ERR_IMMEDIATE_EXECUTION               8098
#define IDS_F_EXECUT_IMMEDIAT                     8099
#define IDS_ERR_COUNT_CONNECTIONS                 8100
#define IDS_ERR_MAX_CONNECTIONS                   8101
#define IDS_ERR_FETCHING_TARGET_DB                8102
#define IDS_F_GETTING_COLUMNS                     8103
#define IDS_F_REGISTRED_COL                       8104
#define IDS_F_FLOAT_EXEC_IMMEDIATE                8105
#define IDS_ERR_CHAR_EXECUTE_IMMEDIATE            8106
#define IDS_ERR_TBL_NUMBER                        8107
#define IDS_F_NO_COLUMN_REGISTRATION              8108
#define IDS_ERR_INCONSISTENT_COUNT                8109
#define IDS_ERR_RELEASE_SESSION                   8110
#define IDS_ERR_VNODE_NOT_FOUND                   8111
#define IDS_ERR_SELECT_FAILED                     8112
#define IDS_ERR_UPDATE_DD_DISTRIB_QUEUE           8113
#define IDS_ERR_GENERATE_SHADOW                   8114
#define IDS_F_SHADOW_TBL                          8115
#define IDS_F_RELATIONSHIP                        8116
#define IDS_ERR_REGISTERED_DB_EVENTS              8117
#define IDS_ERR_ACTIVE_REPLICATION_SERV           8118
#define IDS_F_PING_SVR                            8119
#define IDS_F_REGISTER_DB_EVENTS                  8120
#define IDS_ERR_REPLICATION_VERS                  8121
#define IDS_F_MAX_CONN_SEND_EVENT                 8122
#define IDS_F_DBA_OWNER_EVENT                     8123
#define IDS_ERR_ACCESS_RMCMD_OBJ                  8124
#define IDS_ERR_RMCMD_OUTPUT_COMMAND              8125
#define IDS_ERR_RMCMD_REMOVE                      8126
#define IDS_ERR_RMCMD_NOT_SUPPORTED               8127
#define IDS_ERR_SVR_TYPE                          8128
#define IDS_ERR_RMCMD_COMMAND_NOT_EXEC            8129
#define IDS_F_RMCMD_EXEC_COMMAND                  8130
#define IDS_ERR_CREATE_PROCESS                    8131
#define IDS_ERR_TBL_ID_NOT_FOUND                  8132
#define IDS_ERR_ACTIVATE_SESSION                  8133
#define IDS_ERR_READ_TEMPORARY_FILE               8134
#define IDS_F_ALTER_CDDS                          8135
#define IDS_F_UNKNOWN_DATATYPE                    8136
#define IDS_TITLE_CREATE_DATABASE                 8137
#define IDS_ERR_OPEN_FILE                         8138
#define IDS_ERR_TBL_AND_DBF                       8139
#define IDS_ERR_QUOTE_IN_COLUMN                   8140
#define IDS_ERR_COL_MAX_LENGTH                    8141
#define IDS_ERR_CREATE_DOM                        8142
#define IDS_ERR_EXPANSION_INTERRUPTED             8143
#define IDS_ERR_TREE_ITEM                         8144
#define IDS_E_REPL_DROP_DD_SERVER_SPECIAL         8145
#define IDS_ERR_TBL_INTERNAL_NUMBER               8146
#define IDS_ERR_ACTIVATE_CDDS                     8147
#define IDS_ERR_APPLY_ALL_FULL_PEER               8148
#define IDS_AF_ACTIVATE_RECORDING                 8149
#define IDS_TITLE_ACTIVATE_RECORDING              8150
#define IDS_AF_DEACTIVATE_CDDS                    8151
#define IDS_AF_DEACTIVATE_TABLE                   8152
#define IDS_TITLE_DEACTIVATING                    8153
#define IDS_TITLE_DEACTIVATING_CHANGE_REC         8154
#define IDS_ERR_SELECTED_OBJECT_TYPE              8155
#define IDS_ERR_TITLE_DB_STAT                     8156
#define IDS_ERR_STAT_UPDATE                       8157
#define IDS_F_DRAG_DROP_COLUMN                    8158
#define IDS_F_CONFIRM_CONVERT_COL                 8159
#define IDS_A_APPLY_ALL_FULL_PEER                 8160
#define IDS_F_STARTING_INSTALL                    8161
#define IDS_ERR_GET_SESSION                       8162
#define IDS_ERR_REMOVE_OBJECT                     8163
#define IDS_ERR_DROP_CDDS_ZERO                    8164
#define IDS_ERR_GROUP_CONTAINS_USER               8165
#define IDS_ERR_MODIFY_SYSTEM_OBJECT              8166
#define IDS_ERR_REFRESH_LINK                      8167
#define IDS_ERR_CONFIRM_REFRESH_LINK              8168
#define IDS_ERR_TWO_REFRESH_LINK                  8169
#define IDS_ERR_DB_BUILD                          8170
#define IDS_TITLE_DB_BUILD                        8171
#define IDS_F_REPLICATOR_STATUS                   8172
#define IDS_ERR_UNKNOWN_ICE_TYPE                  8173
#define IDS_F_SETTING_BACKGROUND_REFRESH          8174
#define IDS_TITLE_SETTING_BACKGROUND_REFRESH      8175
#define IDS_ERR_NOT_AVAILABLE                     8176
#define IDS_TITLE_DRAG_DROP_TABLE                 8177
#define IDS_ERR_DATA_TYPES_NOTCOMPATIBLE          8178
#define IDS_ERR_REPLICATOR_STATUS                 8179
#define IDS_F_PAGE_SIZE                           8180
#define IDS_ERR_POPULATE_DATA                     8181
#define IDS_TITLE_GENERATE_RULES_PROCEDURES       8182
#define IDS_TITLE_ACTIVATING_CH_REC               8183
#define IDS_TITLE_GENERATING_SUPPORT_TABLE        8184
#define IDS_TITLE_ACTIVE_CHANGE_RECORDING         8185
#define IDS_TITLE_ARCCLEAN_OP                     8186
#define IDS_A_CONFIRM_REPMOD                      8187
#define IDS_TITLE_CONFIRM_REPMOD                  8188
#define IDS_TITLE_REPMOD_OP                       8189
#define IDS_ERR_WARNING_STAR                      8190
#define IDS_TITLE_CREATE_TBL_STAR                 8191
#define IDS_TITLE_CREATE_VIEW_STAR                8192
#define IDS_ERR_WARNING_VIEW_STAR                 8193
#define IDS_ERR_FILTERS_STATE                     8194
#define IDS_TITLE_ADD_OBJECT                      8195
#define IDS_I_DROP_ICE_ROLE                       8196
#define IDS_I_DROP_ICE_DB_USER                    8197
#define IDS_I_DROP_ICE_DB_CONNECTION              8198
#define IDS_I_DROP_ICE_WEB_USER                   8199
#define IDS_I_DROP_ICE_PROFILE                    8200
#define IDS_I_DROP_ICE_WBR                        8201
#define IDS_I_DROP_ICE_WBC                        8202
#define IDS_I_DROP_ICE_PR                         8203
#define IDS_I_DROP_ICE_PRDBC                      8204
#define IDS_I_DROP_ICE_B_UNIT                     8205
#define IDS_I_DROP_ICE_BUNIT_ROLE                 8206
#define IDS_I_DROP_ICE_BUNIT_USER                 8207
#define IDS_I_DROP_ICE_BUNIT_PAGE                 8208
#define IDS_I_DROP_ICE_BUNIT_FACET                8209
#define IDS_I_DROP_ICE_BUNIT_FROLE                8210
#define IDS_I_DROP_ICE_BUNIT_PROLE                8211
#define IDS_I_DROP_ICE_BUNIT_FUSER                8212
#define IDS_I_DROP_ICE_BUNIT_PUSER                8213
#define IDS_I_DROP_ICE_BUNIT_LOC                  8214
#define IDS_I_DROP_ICE_SESSION_GRP                8215
#define IDS_I_DROP_ICE_LOCATION                   8216
#define IDS_I_DROP_ICE_SVR_VAR                    8217
#define IDS_TITLE_PROFILE                         8218
#define IDS_ERR_REGISTER_LINK_NOT_IMPLEMENTED     8219
#define IDS_ERR_REGISTER_AS_LINK_REFRESH          8220
#define IDS_I_REFRESH_IN_PROGRESS                 8221

//
// REMINDER 20001 TO 40000 RESERVED FOR DLGRES.H
//

//
// menu items values
//
// Convention : first popup menu starts from 1000, second from 2000, etc.
//

// dummy items for controls on button bar
// these values are used by the nanbar dll to know whether a bitmap
// should be drawn or not.
#define IDM_DUMMY1                1
#define IDM_DUMMY2                2
#define IDM_DUMMY3                3

#define IDM_NULL                  10    // unaffected menu item

// IMPORTANT NOTE : since we have put a global toolbar, we have to take care
// not to use menuitems ids that overlap the NANBAR_XXX messages family:
// NANBAR_NOTIFY, NANBAR_NOTIFY_FREEBMP, ...
// so we start at 1100 instead of starting at 1000.
#define IDM_NEW                   1101
#define IDM_OPEN                  1102
// Obsolete : #define IDM_CLOSE                 1103
#define IDM_SAVE                  1104
#define IDM_SAVEAS                1105
#define IDM_CONNECT               1106
#define IDM_DISCONNECT            1107
#define IDM_PRINT                 1108
#define IDM_PREFERENCES           1109
#define IDM_FILEEXIT              1110
// Update these defines when the preferences are spec'ed out.
// CHECK main.rc IDR_CFG_WINDOW SECTION WHEN UDATING THE FOLLOWING LINES
#define IDM_PREF_1                1201
#define IDM_PREF_2                1202
#define IDM_PREF_3                1203
#define IDM_PREF_4                1204
#define IDM_PREF_5                1205
#define IDM_PREF_6                1206
#define IDM_PREF_7                1207
#define IDM_PREF_8                1208
#define IDM_PREF_9                1209
#define IDM_PREF_10               1210
#define IDM_PREF_11               1211    // mfc only: nodes
#define IDM_PREF_12               1212    // mfc only: ipm

#define IDM_CUT                   2001
#define IDM_COPY                  2002
#define IDM_PASTE                 2003
#define IDM_ADDOBJECT             2004
#define IDM_ALTEROBJECT           2005
#define IDM_DROPOBJECT            2006
#define IDM_GRANT                 2007
#define IDM_REVOKE                2008
#define IDM_POPULATE              2009
//#define IDM_OWNERSHIP             2010
#define IDM_FIND                  2011
#define IDM_LOCATE                2012

// Star management
#define IDM_REGISTERASLINK        2013
#define IDM_REFRESHLINK           2014
#define IDM_REMOVEOBJECT          2015


#define IDM_CLASSB_EXPANDONE      3001
#define IDM_CLASSB_EXPANDBRANCH   3002
#define IDM_CLASSB_EXPANDALL      3003
#define IDM_CLASSB_COLLAPSEONE    3004
#define IDM_CLASSB_COLLAPSEBRANCH 3005
#define IDM_CLASSB_COLLAPSEALL    3006
#define IDM_REFRESH               3007
#define IDM_CHMODREFRESH          3008
#define IDM_FILTER                3009
#define IDM_SHOW_SYSTEM           3010
#define IDM_PROPERTIES            3011
#define IDM_MAP                   3012

#define IDM_INFODB                4000
#define IDM_CHECKPOINT            4001
#define IDM_ROLLFORWARD           4002
#define IDM_UNLOADDB              4003
#define IDM_COPYDB                4004
#define IDM_SQLACT                4005
#define IDM_SPACECALC             4006
#define IDM_DUPLICATEDB           4007

#define IDM_GENSTAT               5001
#define IDM_DISPSTAT              5002
#define IDM_ALTERDB               5003
#define IDM_AUDIT                 5004
#define IDM_MODIFYSTRUCT          5005
#define IDM_SYSMOD                5006
#define IDM_VERIFYDB              5007
#define IDM_REPLIC_INSTALL        5008
#define IDM_REPLIC_PROPAG         5009
#define IDM_REPLIC_BUILDSRV       5010
#define IDM_REPLIC_RECONCIL       5011
#define IDM_REPLIC_DEREPLIC       5012
#define IDM_REPLIC_MOBILE         5013
#define IDM_REPLIC_ARCCLEAN       5014
#define IDM_REPLIC_REPMOD         5015
#define IDM_FASTLOAD              5016
#define IDM_REPLIC_CREATEKEYS     5017
#define IDM_REPLIC_ACTIVATE       5018
#define IDM_EXPIREDATE            5019
#define IDM_SECURITYAUDIT         5020
#define IDM_REPLIC_DEACTIVATE     5021
#define IDM_JOURNALING            5022

#define IDM_QUERYCLEAR            6001
#define IDM_QUERYOPEN             6002
#define IDM_QUERYSAVEAS           6003
#define IDM_QUERYGO               6004
#define IDM_SQLWIZARD             6005
#define IDM_QUERYSHOWRES          6006

#define IDM_WINDOWCASCADE         7001
#define IDM_WINDOWTILE_HORZ       7002
#define IDM_WINDOWTILE_VERT       7003
#define IDM_WINDOWICONS           7004
#define IDM_NEWWINDOW             7005
#define IDM_TEAROUT               7006
#define IDM_RESTARTFROMPOS        7007
#define IDM_SCRATCHPAD            7008
#define IDM_SELECTWINDOW          7009
#define IDM_WINDOWCLOSEALL        7010
#define IDM_WINDOWFIRSTCHILD      7011

#define IDM_HELPINDEX             8001
#define IDM_HELPSEARCH            8002
#define IDM_CONTEXTHELP           8003
#define IDM_ABOUT                 8004

// commands not connected to menu items
#define IDM_SQLERRORS             8501    // display sql errors


//#ifdef DEBUGMALLOC
#define IDM_DEBUG_AUDITTREE             9001
#define IDM_DEBUG_CREATEPROPERTY        9002
#define IDM_DEBUG_EMPTYPROPERTY         9003
#define IDM_DEBUG_PREFERENCE_SHOWTOOLS  9004
#define IDM_DEBUG_PREFERENCE_HIDETOOLS  9005
#define IDM_DEBUG_BKTASK_DLGBOX         9006
#define IDM_DEBUG_BTASK_OPENSTATUS      9007
#define IDM_DEBUG_BTASK_UPDATESTATUS    9008
#define IDM_DEBUG_BTASK_CLOSESTATUS     9009
//#endif

#define IDM_DOMDOC_CHANGEVNODE          10001   // added to dom system menu

// desktop specific
#define IDM_LOAD                        11001
#define IDM_UNLOAD                      11002
#define IDM_UPDSTAT                     11003
#define IDM_DOWNLOAD                    11004
#define IDM_RUNSCRIPTS                  11005

#endif //__RESOURCE_INCLUDED__
