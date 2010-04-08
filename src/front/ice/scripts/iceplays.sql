/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:    iceplays.sql
**
** Description:
**      Script for setting up the ice plays as a set of private documents
**      managed under an ice location.
**
## History:
##      06-Nov-98 (fanra01)
##          Createed
##      09-Apr-1999 (peeje01)
##          Make external files all lower case (piccolo on NT)
##          Replace '-' with '_' in file names
##	24-Jan-2000 (somsa01)
##	    Reduced timeout from 60000 to 1800 (30 minutes).
##      03-Jan-2003 (fanra01)
##          Bug 109365
##          Removed registration of unused romance.gif file.
*/
/*
** Create the application name
*/
delete from ice_applications where id=3 and name='playgroup';\p\g
insert into ice_applications values (3, 'playgroup');\p\g

/*
** Create the playgroup profile using dbuser id 1
*/
delete from ice_profiles where id=2 and name='play_profile';\p\g
insert into ice_profiles values (2, 'play_profile', 1, 0, 1800);\p\g

/*
** Create the play role
*/
delete from ice_roles where id=2 and name='play_role';\p\g
insert into ice_roles values (2, 'play_role', 'Role for the plays web application');\p\g

/*
** Create the execute right
*/
delete from ice_res_rrights where role_id=2 and resource='p3';
insert into ice_res_rrights values (2, 'p3', 16);

/*
** Associate the profile with the role
*/
delete from ice_profiles_roles where profile=2 and role=2;\p\g
insert into ice_profiles_roles values (2, 2);\p\g

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
    where id=8 and name='iceplays' and type=5 and path='[II_WEB_SYSTEM]/ingres/ice/plays';\p\g
insert into ice_locations values (8, 'iceplays', 5, '[II_WEB_SYSTEM]/ingres/ice/plays', '');\p\g

/*
** Create the business unit
*/
delete from ice_units where id=3 and name='plays' and owner=0;\p\g
insert into ice_units values (3, 'plays', 0);\p\g

/*
** Associate the tutorial unit with a location
*/
delete from ice_units_locs where unit=3 and loc=1;\p\g
delete from ice_units_locs where unit=3 and loc=8;\p\g
insert into ice_units_locs values (3, 1);\p\g
insert into ice_units_locs values (3, 8);\p\g

/*
** Enter documents for the tutorial application
** id, unit, name, suffix, flag, owner, ext_loc, ext_file, ext_suffix
*/
delete from ice_documents where unit=3;\p\g

insert into ice_documents values( 1, 3, 'play_home'             ,'html' , 48, 0, 8, 'play_home'            , 'html');\p\g
insert into ice_documents values( 2, 3, 'play_autoUser'         ,'html' , 49, 0, 8, 'play_autoUser'        , 'html');\p\g
insert into ice_documents values( 3, 3, 'play_allWrap'          ,'html' , 48, 0, 8, 'play_allWrap'         , 'html');\p\g
insert into ice_documents values( 4, 3, 'play_SessionControl_h' ,'html' , 48, 0, 8, 'play_SessionControl_h', 'html');\p\g
insert into ice_documents values( 5, 3, 'play_login'            ,'html' , 49, 0, 8, 'play_login'           , 'html');\p\g
insert into ice_documents values( 6, 3, 'play_allWrapSub'       ,'html' , 48, 0, 8, 'play_allWrapSub'      , 'html');\p\g
insert into ice_documents values( 7, 3, 'play_typeList'         ,'html' , 48, 0, 8, 'play_typeList'        , 'html');\p\g
insert into ice_documents values( 8, 3, 'play_subSet'           ,'html' , 48, 0, 8, 'play_subSet'          , 'html');\p\g
insert into ice_documents values( 9, 3, 'play_all'              ,'html' , 48, 0, 8, 'play_all'             , 'html');\p\g
insert into ice_documents values(10, 3, 'play_typeLink'         ,'html' , 48, 0, 8, 'play_typeLink'        , 'html');\p\g
insert into ice_documents values(11, 3, 'play_typeLinkSubSet'   ,'html' , 48, 0, 8, 'play_typeLinkSubSet'  , 'html');\p\g
insert into ice_documents values(12, 3, 'play_typeGLink'        ,'html' , 48, 0, 8, 'play_typeGLink'       , 'html');\p\g
insert into ice_documents values(13, 3, 'play_typeGSLink'       ,'html' , 48, 0, 8, 'play_typeGSLink'      , 'html');\p\g
insert into ice_documents values(14, 3, 'play_TxnCndCmt_h'      ,'html' , 48, 0, 8, 'play_TxnCndCmt_h'     , 'html');\p\g
insert into ice_documents values(15, 3, 'play_shopHome'         ,'html' , 48, 0, 8, 'play_shopHome'        , 'html');\p\g
insert into ice_documents values(16, 3, 'play_shopDescribe'     ,'html' , 48, 0, 8, 'play_shopDescribe'    , 'html');\p\g
insert into ice_documents values(17, 3, 'play_shopAdd'          ,'html' , 48, 0, 8, 'play_shopAdd'         , 'html');\p\g
insert into ice_documents values(18, 3, 'play_shopView'         ,'html' , 48, 0, 8, 'play_shopView'        , 'html');\p\g
insert into ice_documents values(19, 3, 'play_shopRemove'       ,'html' , 48, 0, 8, 'play_shopRemove'      , 'html');\p\g
insert into ice_documents values(20, 3, 'play_shopConfirm'      ,'html' , 48, 0, 8, 'play_shopConfirm'     , 'html');\p\g
insert into ice_documents values(21, 3, 'play_shopAction_h'     ,'html' , 48, 0, 8, 'play_shopAction_h'    , 'html');\p\g
insert into ice_documents values(22, 3, 'play_newProduct'       ,'html' , 48, 0, 8, 'play_newProduct'      , 'html');\p\g
insert into ice_documents values(23, 3, 'play_newProductInsert' ,'html' , 48, 0, 8, 'play_newProductInsert', 'html');\p\g
insert into ice_documents values(24, 3, 'OldGlobe'              ,'gif'  , 72, 0, 0, ''                     , ''    );\p\g
insert into ice_documents values(25, 3, 'tragedy'               ,'gif'  , 72, 0, 0, ''                     , ''    );\p\g
insert into ice_documents values(26, 3, 'history'               ,'gif'  , 72, 0, 0, ''                     , ''    );\p\g
insert into ice_documents values(28, 3, 'comedy'                ,'gif'  , 72, 0, 0, ''                     , ''    );\p\g
insert into ice_documents values(29, 3, 'bgpaper'               ,'gif'  , 72, 0, 0, ''                     , ''    );\p\g
insert into ice_documents values(30, 3, 'logout'                ,'gif'  , 72, 0, 0, ''                     , ''    );\p\g
insert into ice_documents values(31, 3, 'play_styleSheet'       ,'css'  , 72, 0, 0, ''                     , ''    );\p\g
