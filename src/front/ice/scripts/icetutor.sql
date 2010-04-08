/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:    icetutor.sql
**
** Description:
**      Script for setting up the ice design tool as a set of public documents
**      managed under an ice location.
**
## History:
##      23-Oct-98 (fanra01)
##          Add history.
##          Add association of the business unit and the default temp HTTP
##          location.
##          Also corrected location flag from relative HTTP to absolute ice.
##      09-Apr-1999 (peeje01)
##          Correct case on external file names (piccolo on NT)
##      10-May-1999 (peeje01)
##          Correct case on autoDeclare to autodeclare because the ICE server
##          processes this file in 2.0 mode and does not map the name (user
##          hasn't logged in yet).
##	24-Jan-2000 (somsa01)
##	    Changed timeout from 60000 to 1800 (30 minutes).
##      09-Oct-2000 (fanra01)
##          Add creation of fortunoff location from SDK.
##      06-Nov-2002 (fanra01)
##          Bug 109079
##          Removed entry for unused user.html file.
##          Removed entry for unused password.html file.
##      03-Dec-2002 (fanra01)
##          Bug 109208
##          Removed entry for unused if5.html file.
*/
/*
** Create a location for the fortunoff demo
*/
insert into ice_locations values(9, 'fortunoff', 3, 'fortunoff', '');\p\g

/*
** Create the application name
*/
delete from ice_applications where id=2 and name='iceTutorial';\p\g
insert into ice_applications values (2, 'iceTutorial');\p\g

/*
** Create the icetutor profile using dbuser id 1
*/
delete from ice_profiles where id=1 and name='icetutor_profile';\p\g
insert into ice_profiles values (1, 'icetutor_profile', 1, 0, 1800);\p\g

/*
** Create the play role
*/
delete from ice_roles where id=1 and name='icetutor_role';\p\g
insert into ice_roles values (1, 'icetutor_role', 'Role for the ICE Desgin Tool');\p\g

/*
** Create the execute right
*/
delete from ice_res_rrights where role_id=1 and resource='p2';
insert into ice_res_rrights values (1, 'p2', 16);

/*
** Associate the profile with the role
*/
delete from ice_profiles_roles where profile=1 and role=1;\p\g
insert into ice_profiles_roles values (1, 1);\p\g


/*
** Create a default temporary location
*/
delete from ice_locations
    where id=1 and name='default' and type=3 and path='ice/tmp';\p\g
insert into ice_locations values (1, 'default', 3, 'ice/tmp', '');\p\g

/*
** Create a locations for tutorial and name it
*/
delete from ice_locations
    where id=3 and name='icetutor' and type=5 and path='[II_WEB_SYSTEM]/ingres/ice/icetutor';\p\g
insert into ice_locations values (3, 'icetutor', 5, '[II_WEB_SYSTEM]/ingres/ice/icetutor', '');\p\g

/*
** Create the business unit
*/
delete from ice_units where id=2 and name='tutorial' and owner=0;\p\g
insert into ice_units values (2, 'tutorial', 0);\p\g


/*
** Associate the tutorial unit with a location
*/
delete from ice_units_locs where unit=2 and loc=1;\p\g
delete from ice_units_locs where unit=2 and loc=3;\p\g
insert into ice_units_locs values (2, 1);\p\g
insert into ice_units_locs values (2, 3);\p\g

/*
** Enter documents for the tutorial application
** id, unit, name, suffix, flag, owner, ext_loc, ext_file, ext_suffix
*/
delete from ice_documents where unit=2;\p\g
insert into ice_documents values( 2000, 2, 'docCreation'  , 'html'  , 48, 0, 3, 'docCreation' , 'html');\p\g
insert into ice_documents values( 2001, 2, 'attr'         , 'html'  , 48, 0, 3, 'attr'        , 'html');\p\g
insert into ice_documents values( 2002, 2, 'begin'        , 'html'  , 48, 0, 3, 'begin'       , 'html');\p\g
insert into ice_documents values( 2003, 2, 'commit'       , 'html'  , 48, 0, 3, 'commit'      , 'html');\p\g
insert into ice_documents values( 2004, 2, 'content'      , 'html'  , 48, 0, 3, 'content'     , 'html');\p\g
insert into ice_documents values( 2005, 2, 'cursor'       , 'html'  , 48, 0, 3, 'cursor'      , 'html');\p\g
insert into ice_documents values( 2006, 2, 'database'     , 'html'  , 48, 0, 3, 'database'    , 'html');\p\g
insert into ice_documents values( 2007, 2, 'docAccess'    , 'html'  , 48, 0, 3, 'docAccess'   , 'html');\p\g
insert into ice_documents values( 2008, 2, 'docCache'     , 'html'  , 48, 0, 3, 'docCache'    , 'html');\p\g
insert into ice_documents values( 2009, 2, 'docCall'      , 'html'  , 48, 0, 3, 'docCall'     , 'html');\p\g
insert into ice_documents values( 2010, 2, 'docCall2'     , 'html'  , 48, 0, 3, 'docCall2'    , 'html');\p\g
insert into ice_documents values( 2011, 2, 'docCall3'     , 'html'  , 48, 0, 3, 'docCall3'    , 'html');\p\g
insert into ice_documents values( 2012, 2, 'docDef'       , 'html'  , 48, 0, 3, 'docDef'      , 'html');\p\g
insert into ice_documents values( 2013, 2, 'docLabel'     , 'html'  , 48, 0, 3, 'docLabel'    , 'html');\p\g
insert into ice_documents values( 2014, 2, 'docName'      , 'html'  , 48, 0, 3, 'docName'     , 'html');\p\g
insert into ice_documents values( 2015, 2, 'docOwner'     , 'html'  , 48, 0, 3, 'docOwner'    , 'html');\p\g
insert into ice_documents values( 2016, 2, 'in3'          , 'html'  , 48, 0, 3, 'in3'         , 'html');\p\g
insert into ice_documents values( 2017, 2, 'download'     , 'html'  , 48, 0, 3, 'download'    , 'html');\p\g
insert into ice_documents values( 2018, 2, 'else'         , 'html'  , 48, 0, 3, 'else'        , 'html');\p\g
insert into ice_documents values( 2019, 2, 'end'          , 'html'  , 48, 0, 3, 'end'         , 'html');\p\g
insert into ice_documents values( 2020, 2, 'extension'    , 'html'  , 48, 0, 3, 'extension'   , 'html');\p\g
insert into ice_documents values( 2021, 2, 'format'       , 'html'  , 48, 0, 3, 'format'      , 'html');\p\g
insert into ice_documents values( 2022, 2, 'format2'      , 'html'  , 48, 0, 3, 'format2'     , 'html');\p\g
insert into ice_documents values( 2023, 2, 'formatHtml'   , 'html'  , 48, 0, 3, 'formatHtml'  , 'html');\p\g
insert into ice_documents values( 2024, 2, 'genIlogin'    , 'html'  , 48, 0, 3, 'genIlogin'   , 'html');\p\g
insert into ice_documents values( 2025, 2, 'header'       , 'html'  , 48, 0, 3, 'header'      , 'html');\p\g
insert into ice_documents values( 2026, 2, 'header2'      , 'html'  , 48, 0, 3, 'header2'     , 'html');\p\g
insert into ice_documents values( 2027, 2, 'if'           , 'html'  , 48, 0, 3, 'if'          , 'html');\p\g
insert into ice_documents values( 2028, 2, 'if1'          , 'html'  , 48, 0, 3, 'if1'         , 'html');\p\g
insert into ice_documents values( 2029, 2, 'if3'          , 'html'  , 48, 0, 3, 'if3'         , 'html');\p\g
insert into ice_documents values( 2030, 2, 'if4'          , 'html'  , 48, 0, 3, 'if4'         , 'html');\p\g
insert into ice_documents values( 2032, 2, 'ifResult'     , 'html'  , 48, 0, 3, 'ifResult'    , 'html');\p\g
insert into ice_documents values( 2033, 2, 'in'           , 'html'  , 48, 0, 3, 'in'          , 'html');\p\g
insert into ice_documents values( 2034, 2, 'in2'          , 'html'  , 48, 0, 3, 'in2'         , 'html');\p\g
insert into ice_documents values( 2035, 2, 'include'      , 'html'  , 48, 0, 3, 'include'     , 'html');\p\g
insert into ice_documents values( 2036, 2, 'include2'     , 'html'  , 48, 0, 3, 'include2'    , 'html');\p\g
insert into ice_documents values( 2037, 2, 'include3'     , 'html'  , 48, 0, 3, 'include3'    , 'html');\p\g
insert into ice_documents values( 2038, 2, 'include32'    , 'html'  , 48, 0, 3, 'include32'   , 'html');\p\g
insert into ice_documents values( 2039, 2, 'include4'     , 'html'  , 48, 0, 3, 'include4'    , 'html');\p\g
insert into ice_documents values( 2040, 2, 'include5'     , 'html'  , 48, 0, 3, 'include5'    , 'html');\p\g
insert into ice_documents values( 2041, 2, 'intro'        , 'html'  , 48, 0, 3, 'intro'       , 'html');\p\g
insert into ice_documents values( 2042, 2, 'jasmenu'      , 'html'  , 48, 0, 3, 'jasmenu'     , 'html');\p\g
insert into ice_documents values( 2043, 2, 'links'        , 'html'  , 48, 0, 3, 'links'       , 'html');\p\g
insert into ice_documents values( 2044, 2, 'links2'       , 'html'  , 48, 0, 3, 'links2'      , 'html');\p\g
insert into ice_documents values( 2045, 2, 'login'        , 'html'  , 48, 0, 3, 'login'       , 'html');\p\g
insert into ice_documents values( 2046, 2, 'main'         , 'html'  , 48, 0, 3, 'main'        , 'html');\p\g
insert into ice_documents values( 2047, 2, 'menProp'      , 'html'  , 48, 0, 3, 'menProp'     , 'html');\p\g
insert into ice_documents values( 2048, 2, 'menu'         , 'html'  , 48, 0, 3, 'menu'        , 'html');\p\g
insert into ice_documents values( 2049, 2, 'nullvar'      , 'html'  , 48, 0, 3, 'nullvar'     , 'html');\p\g
insert into ice_documents values( 2050, 2, 'out'          , 'html'  , 48, 0, 3, 'out'         , 'html');\p\g
insert into ice_documents values( 2051, 2, 'out2'         , 'html'  , 48, 0, 3, 'out2'        , 'html');\p\g
insert into ice_documents values( 2053, 2, 'repeat'       , 'html'  , 48, 0, 3, 'repeat'      , 'html');\p\g
insert into ice_documents values( 2054, 2, 'rollback'     , 'html'  , 48, 0, 3, 'rollback'    , 'html');\p\g
insert into ice_documents values( 2055, 2, 'rows'         , 'html'  , 48, 0, 3, 'rows'        , 'html');\p\g
insert into ice_documents values( 2056, 2, 'server'       , 'html'  , 48, 0, 3, 'server'      , 'html');\p\g
insert into ice_documents values( 2057, 2, 'sqlFinal'     , 'html'  , 48, 0, 3, 'sqlFinal'    , 'html');\p\g
insert into ice_documents values( 2058, 2, 'sqlResult'    , 'html'  , 48, 0, 3, 'sqlResult'   , 'html');\p\g
insert into ice_documents values( 2059, 2, 'statement'    , 'html'  , 48, 0, 3, 'statement'   , 'html');\p\g
insert into ice_documents values( 2060, 2, 'then'         , 'html'  , 48, 0, 3, 'then'        , 'html');\p\g
insert into ice_documents values( 2061, 2, 'Title'        , 'html'  , 48, 0, 3, 'Title'       , 'html');\p\g
insert into ice_documents values( 2062, 2, 'transaction'  , 'html'  , 48, 0, 3, 'transaction' , 'html');\p\g
insert into ice_documents values( 2063, 2, 'transResult'  , 'html'  , 48, 0, 3, 'transResult' , 'html');\p\g
insert into ice_documents values( 2064, 2, 'unitAccess'   , 'html'  , 48, 0, 3, 'unitAccess'  , 'html');\p\g
insert into ice_documents values( 2065, 2, 'unitConf'     , 'html'  , 48, 0, 3, 'unitConf'    , 'html');\p\g
insert into ice_documents values( 2066, 2, 'unitCreation' , 'html'  , 48, 0, 3, 'unitCreation', 'html');\p\g
insert into ice_documents values( 2067, 2, 'unitDef'      , 'html'  , 48, 0, 3, 'unitDef'     , 'html');\p\g
insert into ice_documents values( 2068, 2, 'unitOwner'    , 'html'  , 48, 0, 3, 'unitOwner'   , 'html');\p\g
insert into ice_documents values( 2069, 2, 'upload'       , 'html'  , 48, 0, 3, 'upload'      , 'html');\p\g
insert into ice_documents values( 2071, 2, 'vardef'       , 'html'  , 48, 0, 3, 'vardef'      , 'html');\p\g
insert into ice_documents values( 2072, 2, 'varget'       , 'html'  , 48, 0, 3, 'varget'      , 'html');\p\g
insert into ice_documents values( 2073, 2, 'varset'       , 'html'  , 48, 0, 3, 'varset'      , 'html');\p\g
insert into ice_documents values( 2074, 2, 'varset2'      , 'html'  , 48, 0, 3, 'varset2'     , 'html');\p\g
insert into ice_documents values( 2075, 2, 'if2'          , 'html'  , 48, 0, 3, 'if2'         , 'html');\p\g
insert into ice_documents values( 2076, 2, 'recursive'    , 'html'  , 48, 0, 3, 'recursive'   , 'html');\p\g
insert into ice_documents values( 2077, 2, 'error'        , 'html'  , 48, 0, 3, 'error'       , 'html');\p\g
insert into ice_documents values( 2078, 2, 'checkSQL'     , 'html'  , 48, 0, 3, 'checkSQL'    , 'html');\p\g
insert into ice_documents values( 2079, 2, 'empty'        , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2080, 2, 'nobranch'     , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2081, 2, 'lastopened'   , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2082, 2, 'lastleaf'     , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2083, 2, 'lastclosed'   , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2084, 2, 'opened'       , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2085, 2, 'leaf'         , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2086, 2, 'closed'       , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2087, 2, 'icon_page'    , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2088, 2, 'icon_book'    , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2089, 2, 'icon_chap'    , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2090, 2, 'previous'     , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2091, 2, 'next'         , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2092, 2, 'logout'       , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2093, 2, 'capBUCreate'  , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2094, 2, 'capBUAccess'  , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2095, 2, 'capBULoc'     , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2096, 2, 'capDocCreate' , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2097, 2, 'capDocAccess' , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2098, 2, 'icon_multi'   , 'gif'   , 72, 0, 0,  ''           , ''    );\p\g
insert into ice_documents values( 2099, 2, 'autodeclare'     , 'html'  , 48, 0, 3, 'autodeclare'    , 'html');\p\g
