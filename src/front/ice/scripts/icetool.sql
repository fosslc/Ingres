/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:    icetool.sql
**
** Description:
**      Script for setting up the ice admin tool as a set of public documents
**      managed under an ice location.
**
## History:
##      23-Oct-98 (fanra01)
##          Add history.
##          Add association of the business unit and the default temp HTTP
##          location.
##      09-Apr-1999 (peeje01)
##          Correct case on external files (piccolo on NT)
##      03-Dec-2002 (fanra01)
##          Bug 109207
##          Remove registration of bummenu.html as file is not used.
*/
/*
** Create the application name
*/
delete from ice_applications where id=1 and name='iceTools';\p\g
insert into ice_applications values (1, 'iceTools');\p\g

/*
** Create a default temporary location
*/
delete from ice_locations
    where id=1 and name='default' and type=3 and path='ice/tmp';\p\g
insert into ice_locations values (1, 'default', 3, 'ice/tmp', '');\p\g

/*
** Create a locations for iceTools and name it
*/
delete from ice_locations
    where id=2 and name='icetool' and type=5 and path='[II_WEB_SYSTEM]/ingres/ice/icetool';\p\g
insert into ice_locations values (2, 'icetool', 5, '[II_WEB_SYSTEM]/ingres/ice/icetool', '');\p\g

/*
** Create the business unit
*/
delete from ice_units where id=1 and name='iceTools' and owner=0;\p\g
insert into ice_units values (1, 'iceTools', 0);\p\g


/*
** Associate the iceTool unit with a location
*/
delete from ice_units_locs where unit=1 and loc=1;\p\g
delete from ice_units_locs where unit=1 and loc=2;\p\g
insert into ice_units_locs values (1, 1);\p\g
insert into ice_units_locs values (1, 2);\p\g

/*
** Enter documents for the iceTool application
*/
delete from ice_documents where unit=1;\p\g
insert into ice_documents values(1001, 1, 'actbuLoc'     ,'html', 48, 0, 2, 'actbuloc'       ,'html');\p\g
insert into ice_documents values(1002, 1, 'actbuRole'    ,'html', 48, 0, 2, 'actburole'      ,'html');\p\g
insert into ice_documents values(1003, 1, 'actbuUser'    ,'html', 48, 0, 2, 'actbuuser'      ,'html');\p\g
insert into ice_documents values(1004, 1, 'actDB'        ,'html', 48, 0, 2, 'actdb'          ,'html');\p\g
insert into ice_documents values(1005, 1, 'actDBUser'    ,'html', 48, 0, 2, 'actdbuser'      ,'html');\p\g
insert into ice_documents values(1006, 1, 'actDoc'       ,'html', 48, 0, 2, 'actdoc'         ,'html');\p\g
insert into ice_documents values(1007, 1, 'actDocRole'   ,'html', 48, 0, 2, 'actdocrole'     ,'html');\p\g
insert into ice_documents values(1008, 1, 'actDocUser'   ,'html', 48, 0, 2, 'actdocuser'     ,'html');\p\g
insert into ice_documents values(1009, 1, 'action'       ,'html', 48, 0, 2, 'action'         ,'html');\p\g
insert into ice_documents values(1010, 1, 'actLoc'       ,'html', 48, 0, 2, 'actloc'         ,'html');\p\g
insert into ice_documents values(1011, 1, 'actProfConn'  ,'html', 48, 0, 2, 'actprofconn'    ,'html');\p\g
insert into ice_documents values(1012, 1, 'actProfile'   ,'html', 48, 0, 2, 'actprofile'     ,'html');\p\g
insert into ice_documents values(1013, 1, 'actProfRole'  ,'html', 48, 0, 2, 'actprofrole'    ,'html');\p\g
insert into ice_documents values(1014, 1, 'actRole'      ,'html', 48, 0, 2, 'actrole'        ,'html');\p\g
insert into ice_documents values(1015, 1, 'actUnit'      ,'html', 48, 0, 2, 'actunit'        ,'html');\p\g
insert into ice_documents values(1016, 1, 'actUser'      ,'html', 48, 0, 2, 'actuser'        ,'html');\p\g
insert into ice_documents values(1017, 1, 'actUserConn'  ,'html', 48, 0, 2, 'actuserconn'    ,'html');\p\g
insert into ice_documents values(1018, 1, 'actUserRole'  ,'html', 48, 0, 2, 'actuserrole'    ,'html');\p\g
insert into ice_documents values(1019, 1, 'act_sess'     ,'html', 48, 0, 2, 'act_sess'       ,'html');\p\g
insert into ice_documents values(1020, 1, 'buDRight'     ,'html', 48, 0, 2, 'budright'       ,'html');\p\g
insert into ice_documents values(1021, 1, 'buLoc'        ,'html', 48, 0, 2, 'buloc'          ,'html');\p\g
insert into ice_documents values(1022, 1, 'buLocs'       ,'html', 48, 0, 2, 'buLocs'         ,'html');\p\g
insert into ice_documents values(1023, 1, 'buMain'       ,'html', 48, 0, 2, 'bumain'         ,'html');\p\g
insert into ice_documents values(1024, 1, 'buMenu'       ,'html', 48, 0, 2, 'bumenu'         ,'html');\p\g
insert into ice_documents values(1026, 1, 'buDocs'       ,'html', 48, 0, 2, 'bupages'        ,'html');\p\g
insert into ice_documents values(1027, 1, 'buPCode'      ,'html', 48, 0, 2, 'bupcode'        ,'html');\p\g
insert into ice_documents values(1028, 1, 'buDelete'     ,'html', 48, 0, 2, 'bupdelete'      ,'html');\p\g
insert into ice_documents values(1029, 1, 'buDMenu'      ,'html', 48, 0, 2, 'bupmenu'        ,'html');\p\g
insert into ice_documents values(1030, 1, 'buProp'       ,'html', 48, 0, 2, 'buprop'         ,'html');\p\g
insert into ice_documents values(1031, 1, 'buUpdate'     ,'html', 48, 0, 2, 'bupupdate'      ,'html');\p\g
insert into ice_documents values(1032, 1, 'buDView'      ,'html', 48, 0, 2, 'bupview'        ,'html');\p\g
insert into ice_documents values(1033, 1, 'buRight'      ,'html', 48, 0, 2, 'buright'        ,'html');\p\g
insert into ice_documents values(1034, 1, 'buUnit'       ,'html', 48, 0, 2, 'buunit'         ,'html');\p\g
insert into ice_documents values(1035, 1, 'buUnits'      ,'html', 48, 0, 2, 'buunits'        ,'html');\p\g
insert into ice_documents values(1036, 1, 'buWelcome'    ,'html', 48, 0, 2, 'buwelcome'      ,'html');\p\g
insert into ice_documents values(1037, 1, 'cache'        ,'html', 48, 0, 2, 'cache'          ,'html');\p\g
insert into ice_documents values(1038, 1, 'fill'         ,'html', 48, 0, 2, 'fill'           ,'html');\p\g
insert into ice_documents values(1039, 1, 'jasmenu'      ,'html', 48, 0, 2, 'jasmenu'        ,'html');\p\g
insert into ice_documents values(1040, 1, 'login'        ,'html', 49, 0, 2, 'login'          ,'html');\p\g
insert into ice_documents values(1041, 1, 'main'         ,'html', 48, 0, 2, 'main'           ,'html');\p\g
insert into ice_documents values(1042, 1, 'menProp'      ,'html', 48, 0, 2, 'menprop'        ,'html');\p\g
insert into ice_documents values(1043, 1, 'menTitle'     ,'html', 48, 0, 2, 'mentitle'       ,'html');\p\g
insert into ice_documents values(1044, 1, 'menu'         ,'html', 48, 0, 2, 'menu'           ,'html');\p\g
insert into ice_documents values(1045, 1, 'secDB'        ,'html', 48, 0, 2, 'secdb'          ,'html');\p\g
insert into ice_documents values(1046, 1, 'secdbs'       ,'html', 48, 0, 2, 'secdbs'         ,'html');\p\g
insert into ice_documents values(1047, 1, 'secDBu'       ,'html', 48, 0, 2, 'secdbu'         ,'html');\p\g
insert into ice_documents values(1048, 1, 'secDBuProp'   ,'html', 48, 0, 2, 'secdbuprop'     ,'html');\p\g
insert into ice_documents values(1049, 1, 'secDBus'      ,'html', 48, 0, 2, 'secdbus'        ,'html');\p\g
insert into ice_documents values(1050, 1, 'secMenu'      ,'html', 48, 0, 2, 'secmenu'        ,'html');\p\g
insert into ice_documents values(1051, 1, 'secProfConn'  ,'html', 48, 0, 2, 'secprofconn'    ,'html');\p\g
insert into ice_documents values(1052, 1, 'secProfConns' ,'html', 48, 0, 2, 'secprofconns'   ,'html');\p\g
insert into ice_documents values(1053, 1, 'secProfile'   ,'html', 48, 0, 2, 'secprofile'     ,'html');\p\g
insert into ice_documents values(1054, 1, 'secProfiles'  ,'html', 48, 0, 2, 'secprofiles'    ,'html');\p\g
insert into ice_documents values(1055, 1, 'secProfRole'  ,'html', 48, 0, 2, 'secprofrole'    ,'html');\p\g
insert into ice_documents values(1056, 1, 'secProfRoles' ,'html', 48, 0, 2, 'secprofroles'   ,'html');\p\g
insert into ice_documents values(1057, 1, 'secRole'      ,'html', 48, 0, 2, 'secrole'        ,'html');\p\g
insert into ice_documents values(1058, 1, 'secRols'      ,'html', 48, 0, 2, 'secrols'        ,'html');\p\g
insert into ice_documents values(1059, 1, 'section'      ,'html', 48, 0, 2, 'section'        ,'html');\p\g
insert into ice_documents values(1060, 1, 'secUser'      ,'html', 48, 0, 2, 'secuser'        ,'html');\p\g
insert into ice_documents values(1061, 1, 'secUserConn'  ,'html', 48, 0, 2, 'secuserConn'    ,'html');\p\g
insert into ice_documents values(1062, 1, 'secUserConns' ,'html', 48, 0, 2, 'secuserConns'   ,'html');\p\g
insert into ice_documents values(1063, 1, 'secUserRole'  ,'html', 48, 0, 2, 'secuserRole'    ,'html');\p\g
insert into ice_documents values(1064, 1, 'secUserRoles' ,'html', 48, 0, 2, 'secuserRoles'   ,'html');\p\g
insert into ice_documents values(1065, 1, 'secUses'      ,'html', 48, 0, 2, 'secuses'        ,'html');\p\g
insert into ice_documents values(1066, 1, 'secWelcome'   ,'html', 48, 0, 2, 'secwelcome'     ,'html');\p\g
insert into ice_documents values(1067, 1, 'serLoc'       ,'html', 48, 0, 2, 'serloc'         ,'html');\p\g
insert into ice_documents values(1068, 1, 'serLocs'      ,'html', 48, 0, 2, 'serlocs'        ,'html');\p\g
insert into ice_documents values(1069, 1, 'serMenu'      ,'html', 48, 0, 2, 'sermenu'        ,'html');\p\g
insert into ice_documents values(1070, 1, 'serWelcome'   ,'html', 48, 0, 2, 'serwelcome'     ,'html');\p\g
insert into ice_documents values(1071, 1, 'title'        ,'html', 48, 0, 2, 'title'          ,'html');\p\g
insert into ice_documents values(1072, 1, 'upload'       ,'html', 48, 0, 2, 'upload'         ,'html');\p\g
insert into ice_documents values(1073, 1, 'usr_sess'     ,'html', 48, 0, 2, 'usr_sess'       ,'html');\p\g
insert into ice_documents values(1074, 1, 'dirfiles'     ,'html', 48, 0, 2, 'dirfiles'       ,'html');\p\g
insert into ice_documents values(1075, 1, 'buDocsInfo'   ,'html', 48, 0, 2, 'bupagesInfo'    ,'html');\p\g
insert into ice_documents values(1076, 1, 'begin'        ,'html', 48, 0, 2, 'begin'          ,'html');\p\g
insert into ice_documents values(1077, 1, 'end'          ,'html', 48, 0, 2, 'end'            ,'html');\p\g
insert into ice_documents values(1078, 1, 'result'       ,'html', 48, 0, 2, 'result'         ,'html');\p\g
insert into ice_documents values(1079, 1, 'transaction'  ,'html', 48, 0, 2, 'transaction'    ,'html');\p\g
insert into ice_documents values(1080, 1, 'cursor'       ,'html', 48, 0, 2, 'cursor'         ,'html');\p\g
insert into ice_documents values(1081, 1, 'connection'   ,'html', 48, 0, 2, 'connection'     ,'html');\p\g
insert into ice_documents values(1082, 1, 'actUserSess'  ,'html', 48, 0, 2, 'actusersess'    ,'html');\p\g
insert into ice_documents values(1083, 1, 'actTrans'     ,'html', 48, 0, 2, 'acttrans'       ,'html');\p\g
insert into ice_documents values(1084, 1, 'actCursor'    ,'html', 48, 0, 2, 'actcursor'      ,'html');\p\g
insert into ice_documents values(1085, 1, 'actCache'     ,'html', 48, 0, 2, 'actcache'       ,'html');\p\g
insert into ice_documents values(1086, 1, 'actConn'      ,'html', 48, 0, 2, 'actconn'        ,'html');\p\g
insert into ice_documents values(1087, 1, 'actVar'       ,'html', 48, 0, 2, 'actvar'         ,'html');\p\g
insert into ice_documents values(1088, 1, 'serVar'       ,'html', 48, 0, 2, 'servar'         ,'html');\p\g
insert into ice_documents values(1089, 1, 'serVars'      ,'html', 48, 0, 2, 'servars'        ,'html');\p\g
insert into ice_documents values(1090, 1, 'actbuCopy'    ,'html', 48, 0, 2, 'actbucopy'      , 'html');\p\g
insert into ice_documents values(1091, 1, 'buCopy'       ,'html', 48, 0, 2, 'bucopy'         , 'html');\p\g
insert into ice_documents values(1092, 1, 'actApp'       ,'html', 48, 0, 2, 'actapp'         , 'html');\p\g
insert into ice_documents values(1093, 1, 'serApp'       ,'html', 48, 0, 2, 'serapp'         , 'html');\p\g
insert into ice_documents values(1094, 1, 'serApps'      ,'html', 48, 0, 2, 'serapps'        , 'html');\p\g
insert into ice_documents values(1095, 1, 'nobranch'     ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1096, 1, 'empty'        ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1097, 1, 'icon_rpage'   ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1098, 1, 'icon_page'    ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1099, 1, 'icon_rmulti'  ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1100, 1, 'icon_multi'   ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1101, 1, 'lastopened'   ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1102, 1, 'lastleaf'     ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1103, 1, 'lastclosed'   ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1104, 1, 'opened'       ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1105, 1, 'leaf'         ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1106, 1, 'closed'       ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1107, 1, 'icon_units'   ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1108, 1, 'icon_unit'    ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1109, 1, 'icon_locs'    ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1110, 1, 'icon_pages'   ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1111, 1, 'icon_multis'  ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1112, 1, 'icon_access'  ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1113, 1, 'icon_loc'     ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1114, 1, 'cancel'       ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1115, 1, 'submit'       ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1116, 1, 'menu'         ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1117, 1, 'logout'       ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1118, 1, 'iipe_anim'    ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1119, 1, 'update'       ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1120, 1, 'delete'       ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1121, 1, 'view'         ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1122, 1, 'code'         ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1123, 1, 'access'       ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1124, 1, 'download'     ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1125, 1, 'server_anim'  ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1126, 1, 'security_anim','gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1127, 1, 'icon_dbu'     ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1128, 1, 'icon_role'    ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1129, 1, 'icon_db'      ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1130, 1, 'icon_user'    ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1131, 1, 'icon_dbs'     ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1132, 1, 'icon_roles'   ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1133, 1, 'icon_profile' ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1134, 1, 'icon_dbus'    ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1135, 1, 'icon_users'   ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1136, 1, 'icon_profiles','gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1137, 1, 'icon_var'     ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1138, 1, 'icon_vars'    ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1139, 1, 'icon_monit'   ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1140, 1, 'icon_act_sess','gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1141, 1, 'icon_cache'   ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1142, 1, 'icon_trans'   ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1143, 1, 'icon_curs'    ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1144, 1, 'icon_copy'    ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1145, 1, 'icon_apps'    ,'gif' , 72, 0, 0, ''               , ''  );\p\g
insert into ice_documents values(1146, 1, 'icon_app'     ,'gif' , 72, 0, 0, ''               , ''  );\p\g
