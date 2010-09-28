/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : dba.h
//    general info (obj type, ...)
//
//  History:
//	14-Feb-2000 (somsa01)
//	    Removed redefinitions of system types.
**  26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**    21-Jun-2001 (schph01)
**      (SIR 103694) STEP 2 support of UNICODE datatypes.
**  23-Nov-2001 (noifr01)
**     added #if[n]def's for the libmon project
**  10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
**  18-Mar-2003 (schph01 and noifr01)
**   (sir 107523) added OT_SEQUENCE and other related new object 
**   types (grantees)
**  12-May-2004 (schph01)
**   (SIR 111507)Add new datatype BigInt
**  10-Nov-2004 (schph01)
**   (bug 113426) Add OT_ICE_MAIN for managing on Force Refresh or Expand All
**  11-May-2010 (drivi01)
**    Added function CanObjectExistInVectorWise which returns
**    TRUE or FALSE depending on if the object supported by
**    VectorWise.
**  25-May-2010 (drivi01) Bug 123817
**    Fix the object length to allow for long IDs.
**    Remove hard coded constants, use DB_MAXNAME instead.
** 30-Jun-2010 (drivi01)
**    Bug #124006
**    Add new BOOLEAN datatype.
********************************************************************/

#ifndef DBA_HEADER
#define DBA_HEADER


#include <compat.h>
#include <cm.h>
#include <st.h>
#include <iicommon.h>

#ifndef LIBMON_HEADER
//#ifndef UNIX
#ifndef OBSOLETE
#include <windows.h>
#include <string.h>
typedef HWND DISPWND;
#else
#ifndef BOOL
typedef int BOOL;
#endif
#ifndef FALSE
#define FALSE       0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif
#ifndef UINT
typedef unsigned int UINT;   
#endif 
#endif 

#include "port.h"

// Internal command handlers
#ifdef WIN32
#define BOOL_HANDLE_WM_COMMAND(hwnd, wParam, lParam, fn) \
    case WM_COMMAND: return (LRESULT)(BOOL)(fn)((hwnd), (int)(LOWORD(wParam)), (HWND)(lParam), (UINT)HIWORD(wParam))
#else
#define BOOL_HANDLE_WM_COMMAND(hwnd, wParam, lParam, fn) \
    case WM_COMMAND: return (LRESULT)(BOOL)(fn)((hwnd), (int)(wParam), (HWND)LOWORD(lParam), (UINT)HIWORD(lParam))
#endif

#endif /* LIBMON_HEADER */

typedef unsigned char * LPUCHAR;

#include "tools.h"
#include "esltools.h"

#define MAXSESSIONS 40

//#define MAXOBJECTNAME   (32 + 1)
#define MAXOBJECTNAME   (DB_MAXNAME*2 + 3)
#define MAXOBJECTNAMENOSCHEMA  DB_MAXNAME + 1
#define MAXDBNAME       (MAXOBJECTNAME)   /* but must be unique on 24 chars*/
#define EXTRADATASIZE   (MAXOBJECTNAME*2)
#define MAXPLEVEL 3
#define MAXAREANAME     (128+1)
#define MAXDATELEN      32                
#define MAXCHILDRENTYPE 40 
#define MAXLIMITSECLBL  (128+1)
#define MARKENDOFLIST ((unsigned char)0xFF)

#define STEPSMALLOBJ 8

#define STEPELEM 2  

#define OT_VIRTNODE         0
#define OT_DATABASE         1
#define OT_PROFILE          2
#define OT_USER             3
#define OT_GROUP            4
#define OT_GROUPUSER        5
#define OT_ROLE             6
#define OT_LOCATION         7
#define OT_SEQUENCE         8
#define OT_TABLE           10
#define OT_TABLELOCATION   11
#define OT_VIEW            12
#define OT_VIEWTABLE       13
#define OT_INDEX           14
#define OT_INTEGRITY       15
#define OT_PROCEDURE       16
#define OT_RULE            17
#define OT_RULEWITHPROC    18
#define OT_RULEPROC        19
#define OT_SCHEMAUSER      20
#define OT_SYNONYM         21
#define OT_SYNONYMOBJECT   22 /* if type undefined (table/view/index) */
#define OT_DBEVENT         23

#define OT_S_ALARM_SELSUCCESS_USER 24
#define OT_S_ALARM_SELFAILURE_USER 25
#define OT_S_ALARM_DELSUCCESS_USER 26
#define OT_S_ALARM_DELFAILURE_USER 27
#define OT_S_ALARM_INSSUCCESS_USER 28
#define OT_S_ALARM_INSFAILURE_USER 29
#define OT_S_ALARM_UPDSUCCESS_USER 30
#define OT_S_ALARM_UPDFAILURE_USER 31

#define OT_DBGRANT_ACCESY_USER     32   // equivalent to CONNECT for OIDT
#define OT_DBGRANT_ACCESN_USER     33
#define OT_DBGRANT_CREPRY_USER     34
#define OT_DBGRANT_CREPRN_USER     35
#define OT_DBGRANT_CRETBY_USER     36   // equivalent to RESOURCE for OIDT
#define OT_DBGRANT_CRETBN_USER     37
#define OT_DBGRANT_DBADMY_USER     38   // equivalent to DBA for OIDT
#define OT_DBGRANT_DBADMN_USER     39
#define OT_DBGRANT_LKMODY_USER     40
#define OT_DBGRANT_LKMODN_USER     41
#define OT_DBGRANT_QRYIOY_USER     42
#define OT_DBGRANT_QRYION_USER     43
#define OT_DBGRANT_QRYRWY_USER     44
#define OT_DBGRANT_QRYRWN_USER     45
#define OT_DBGRANT_UPDSCY_USER     46
#define OT_DBGRANT_UPDSCN_USER     47 

// new section for new DB GRANTS in OpenIngres
#define OT_DBGRANT_SELSCY_USER     48 
#define OT_DBGRANT_SELSCN_USER     49 
#define OT_DBGRANT_CNCTLY_USER     50 
#define OT_DBGRANT_CNCTLN_USER     51 
#define OT_DBGRANT_IDLTLY_USER     52 
#define OT_DBGRANT_IDLTLN_USER     53 
#define OT_DBGRANT_SESPRY_USER     54 
#define OT_DBGRANT_SESPRN_USER     55 
#define OT_DBGRANT_TBLSTY_USER     56 
#define OT_DBGRANT_TBLSTN_USER     57 
// end of new section for new DB GRANTS in OpenIngres

// new section for previously undocumented ids
#define OT_DBGRANT_QRYCPN_USER     700 
#define OT_DBGRANT_QRYCPY_USER     701 
#define OT_DBGRANT_QRYPGN_USER     702 
#define OT_DBGRANT_QRYPGY_USER     703 
#define OT_DBGRANT_QRYCON_USER     704 
#define OT_DBGRANT_QRYCOY_USER     705 

#define OT_DBGRANT_SEQCRN_USER     58
#define OT_DBGRANT_SEQCRY_USER     59

#define OT_SEQUGRANT_NEXT_USER     60

#define OT_TABLEGRANT_SEL_USER     61
#define OT_TABLEGRANT_INS_USER     62
#define OT_TABLEGRANT_UPD_USER     63
#define OT_TABLEGRANT_DEL_USER     64
#define OT_TABLEGRANT_REF_USER     65
#define OT_TABLEGRANT_CPI_USER     66
#define OT_TABLEGRANT_CPF_USER     67
#define OT_TABLEGRANT_ALL_USER     68

#define OT_TABLEGRANT_INDEX_USER   69   // OI Desktop
#define OT_TABLEGRANT_ALTER_USER   70   // OI Desktop


#define OT_VIEWGRANT_SEL_USER      71
#define OT_VIEWGRANT_INS_USER      72
#define OT_VIEWGRANT_UPD_USER      73
#define OT_VIEWGRANT_DEL_USER      74

#define OT_DBEGRANT_RAISE_USER     75
#define OT_DBEGRANT_REGTR_USER     76

#define OT_PROCGRANT_EXEC_USER     77

#define OT_ROLEGRANT_USER          78

#define OTR_PROC_RULE              79
#define OTR_LOCATIONTABLE          80
#define OTR_TABLEVIEW              81
#define OTR_USERGROUP              82
#define OTR_USERSCHEMA             83

#define OTR_INDEXSYNONYM           84
#define OTR_TABLESYNONYM           85 
#define OTR_VIEWSYNONYM            86
                                   
#define OTR_DBGRANT_ACCESY_DB      87
#define OTR_DBGRANT_ACCESN_DB      88
#define OTR_DBGRANT_CREPRY_DB      89
#define OTR_DBGRANT_CREPRN_DB      90
#define OTR_DBGRANT_CRETBY_DB      91
#define OTR_DBGRANT_CRETBN_DB      92
#define OTR_DBGRANT_DBADMY_DB      93
#define OTR_DBGRANT_DBADMN_DB      94
#define OTR_DBGRANT_LKMODY_DB      95
#define OTR_DBGRANT_LKMODN_DB      96
#define OTR_DBGRANT_QRYIOY_DB      97
#define OTR_DBGRANT_QRYION_DB      98
#define OTR_DBGRANT_QRYRWY_DB      99
#define OTR_DBGRANT_QRYRWN_DB     100
#define OTR_DBGRANT_UPDSCY_DB     101
#define OTR_DBGRANT_UPDSCN_DB     102 

// new section for new DB GRANTS in OpenIngres
#define OTR_DBGRANT_SELSCY_DB     103 
#define OTR_DBGRANT_SELSCN_DB     104 
#define OTR_DBGRANT_CNCTLY_DB     105 
#define OTR_DBGRANT_CNCTLN_DB     106 
#define OTR_DBGRANT_IDLTLY_DB     107 
#define OTR_DBGRANT_IDLTLN_DB     108 
#define OTR_DBGRANT_SESPRY_DB     109 
#define OTR_DBGRANT_SESPRN_DB     110 
#define OTR_DBGRANT_TBLSTY_DB     111 
#define OTR_DBGRANT_TBLSTN_DB     112 

// new section for previously undocumented ids
#define OTR_DBGRANT_QRYCPN_DB     750 
#define OTR_DBGRANT_QRYCPY_DB     751 
#define OTR_DBGRANT_QRYPGN_DB     752 
#define OTR_DBGRANT_QRYPGY_DB     753 
#define OTR_DBGRANT_QRYCON_DB     754 
#define OTR_DBGRANT_QRYCOY_DB     755 

// new section for DB GRANTS for sequences
#define OTR_DBGRANT_SEQCRN_DB     756
#define OTR_DBGRANT_SEQCRY_DB     757 

// end of new section for new DB GRANTS in OpenIngres

#define OTR_GRANTEE_SEL_TABLE     113
#define OTR_GRANTEE_INS_TABLE     114
#define OTR_GRANTEE_UPD_TABLE     115
#define OTR_GRANTEE_DEL_TABLE     116
#define OTR_GRANTEE_REF_TABLE     117
#define OTR_GRANTEE_CPI_TABLE     118
#define OTR_GRANTEE_CPF_TABLE     119
#define OTR_GRANTEE_ALL_TABLE     120
#define OTR_GRANTEE_SEL_VIEW      121
#define OTR_GRANTEE_INS_VIEW      122
#define OTR_GRANTEE_UPD_VIEW      123
#define OTR_GRANTEE_DEL_VIEW      124
#define OTR_GRANTEE_RAISE_DBEVENT 125
#define OTR_GRANTEE_REGTR_DBEVENT 126
#define OTR_GRANTEE_EXEC_PROC     127
#define OTR_GRANTEE_ROLE          128
#define OTR_GRANTEE_NEXT_SEQU    2010

#define OTR_ALARM_SELSUCCESS_TABLE  129 
#define OTR_ALARM_SELFAILURE_TABLE  130 
#define OTR_ALARM_DELSUCCESS_TABLE  131 
#define OTR_ALARM_DELFAILURE_TABLE  132 
#define OTR_ALARM_INSSUCCESS_TABLE  133 
#define OTR_ALARM_INSFAILURE_TABLE  134
#define OTR_ALARM_UPDSUCCESS_TABLE  135
#define OTR_ALARM_UPDFAILURE_TABLE  136

#define OTLL_SECURITYALARM        137
#define OTLL_GRANTEE              138
#define OTLL_DBGRANTEE            139
#define OTLL_GRANT                140
                                  
#define OT_GRANTEE                141

#define OT_UNKNOWN                142
#define OT_COLUMN                 143
#define OT_VIEWCOLUMN             144
#define OT_PUBLIC                 145


#define OT_REPLIC_CONNECTION      146
#define OT_REPLIC_CDDS            147
#define OT_REPLIC_CDDS_DETAIL     148
#define OT_REPLIC_REGTABLE        149
#define OT_REPLIC_MAILUSER        150
#define OTR_REPLIC_CDDS_TABLE     152
#define OTR_REPLIC_TABLE_CDDS     153
#define OTLL_REPLIC_CDDS          154
                                     
// JFS BEGIN
#define OT_REPLIC_DISTRIBUTION    155
#define OT_REPLIC_REGTBLS         156
#define OT_REPLIC_REGCOLS         157
// JFS END
                     
#define OT_LOCATIONWITHUSAGES     158
                
#define OTLL_REVOKE               159
                                  
#define OT_S_ALARM_CO_SUCCESS_USER 162
#define OT_S_ALARM_CO_FAILURE_USER 163
#define OT_S_ALARM_DI_SUCCESS_USER 164
#define OT_S_ALARM_DI_FAILURE_USER 165
#define OT_ALARMEE                 166
#define OT_S_ALARM_EVENT           167

#define OTR_ALARM_CONNECT_DB      168
#define OTR_ALARM_DISCONNECT_DB   169 
#define OTR_DBEVENT_CNCT_ALARM    170
#define OTR_DBEVENT_DISC_ALARM    171
#define OTR_DBEVENT_SELS_ALARM    172
#define OTR_DBEVENT_SELF_ALARM    173
#define OTR_DBEVENT_DELS_ALARM    174
#define OTR_DBEVENT_DELF_ALARM    175
#define OTR_DBEVENT_INSS_ALARM    176
#define OTR_DBEVENT_INSF_ALARM    177
#define OTR_DBEVENT_UPDS_ALARM    178
#define OTR_DBEVENT_UPDF_ALARM    179
#define OTLL_DBSECALARM           180

// version 1.1 of Replicator section

#define OT_REPLIC_CONNECTION_V11   181
#define OT_REPLIC_CDDS_V11         182
#define OT_REPLIC_CDDS_DETAIL_V11  183
#define OT_REPLIC_REGTABLE_V11     184
#define OT_REPLIC_MAILUSER_V11     185
#define OTR_REPLIC_CDDS_TABLE_V11  187
#define OTR_REPLIC_TABLE_CDDS_V11  188
#define OTLL_REPLIC_CDDS_V11       189
                                     
// JFS BEGIN
#define OT_REPLIC_DISTRIBUTION_V11 190
#define OT_REPLIC_REGTBLS_V11      191
#define OT_REPLIC_REGCOLS_V11      192
// JFS END
#define OT_REPLIC_DBMS_TYPE_V11    193
#define OT_REPLIC_CDDSDBINFO_V11   194

#define OTLL_OIDTDBGRANTEE         195

// end of version 1.1 of Replicator section


#define OT_VIEWALL                196                                 
#define OT_ERROR                  197

// Needed because of new icons behaviour as of 27/05/97, in case replicator not installed
#define OT_REPLICATOR             198

// 3 types for schema subbranches: tables, views and procedures
// values : see comment in winmain\tree.h
#define OT_SCHEMAUSER_TABLE       617
#define OT_SCHEMAUSER_VIEW        618
#define OT_SCHEMAUSER_PROCEDURE   619
// Note: 620 to 623 used in winmain\tree.h


// MAKE SURE NEW OT_xxx VALUES DO NOT OVERLAP WITH OT_yyy VALUES IN TREE.H !

#define OTR_CDB						2000  //Coordinator database
//
// MONITORING
//

// MAKE SURE NEW OT_xxx VALUES DO NOT OVERLAP WITH OT_yyy VALUES IN TREE.H !
#define NOSESSION    0L

#define OT_TOBESOLVED              999

#define OT_MON_ALL                 1000

#define OTLL_MON_RESOURCE          1001

#define OT_MON_SERVER              1002
#define OT_MON_SESSION             1003

#define OT_MON_CLIENTINFO          1004  // for GetDetailInfo only
#define OT_MON_LOCKINFO            1005  // for GetDetailInfo only

#define OT_MON_LOCKLIST            1006
#define OT_MON_LOCKLIST_LOCK       1007
#define OT_MON_LOCKLIST_BL_LOCK    1008 // blocking lock
#define OT_MON_LOCK                1009
#define OT_MON_LOCK_RESOURCE       1010
#define OT_MON_LOCK_LOCKLIST       1011
#define OT_MON_LOCK_SESSION        1012
#define OT_MON_LOCK_SERVER         1013

#define OT_MON_LOGINFO             1014  // for GetDetailInfo only
#define OT_MON_LOGHEADER           1015  // for GetDetailInfo only
#define OT_MON_TRANSACTION         1016

#define OT_MON_RES_DB              1017
#define OT_MON_RES_TABLE           1018
#define OT_MON_RES_PAGE            1019
#define OT_MON_RES_OTHER           1020

#define OT_MON_RES_LOCK            1021

#define OT_MON_LOCATION_DB         1022

#define OT_MON_LOGPROCESS          1023
#define OT_MON_LOGDATABASE         1024

#define OT_MON_DB_SERVER           1025

#define OT_MON_ACTIVE_USER         1026
#define OT_MON_ACTIVE_USR_SESSION  1027

#define OT_LOCATION_NODOUBLE       1028
#define OT_MON_RESOURCE_DTPO       1029
#define OT_MON_SERVERWITHSESS      1030

#define OT_MON_REPLIC_SERVER       1031
#define OT_MON_REPLIC_CDDS_ASSIGN  1032

#define OT_MON_BLOCKING_SESSION    1033


// New types for ICE as of Sep. 10, 98
#define OT_MON_ICE_CONNECTED_USER      1034
#define OT_MON_ICE_TRANSACTION         1035
#define OT_MON_ICE_CURSOR              1036
#define OT_MON_ICE_ACTIVE_USER         1037
#define OT_MON_ICE_FILEINFO            1038
#define OT_MON_ICE_HTTP_CONNECTION     1042
#define OT_MON_ICE_DB_CONNECTION       1043

//
// NODES
//

#ifndef LIBMON_HEADER
#define OT_NODE                    1051
#define OT_NODE_OPENWIN            1052
#define OT_NODE_LOGINPSW           1053
#define OT_NODE_CONNECTIONDATA     1054
#define OT_NODE_SERVERCLASS        1055
#define OT_NODE_ATTRIBUTE          1056
#endif

//
// ICE
//
// Under "Security"
#define OT_ICE_ROLE                 1060
#define OT_ICE_DBUSER               1061
#define OT_ICE_DBCONNECTION         1062
#define OT_ICE_WEBUSER              1063
#define OT_ICE_WEBUSER_ROLE         1064
#define OT_ICE_WEBUSER_CONNECTION   1065
#define OT_ICE_PROFILE              1066
#define OT_ICE_PROFILE_ROLE         1067
#define OT_ICE_PROFILE_CONNECTION   1068
// Under "Bussiness unit" (BUNIT)
#define OT_ICE_BUNIT                1069
#define OT_ICE_BUNIT_SEC_ROLE       1070
#define OT_ICE_BUNIT_SEC_USER       1071
#define OT_ICE_BUNIT_FACET          1072
#define OT_ICE_BUNIT_PAGE           1073
#define OT_ICE_BUNIT_FACET_ROLE     1074
#define OT_ICE_BUNIT_FACET_USER     1075
#define OT_ICE_BUNIT_PAGE_ROLE      1076
#define OT_ICE_BUNIT_PAGE_USER      1077
#define OT_ICE_BUNIT_LOCATION       1078
// Under "Server"
#define OT_ICE_SERVER_APPLICATION   1079
#define OT_ICE_SERVER_LOCATION      1080
#define OT_ICE_SERVER_VARIABLE      1081
// generic for "background refresh"
#define OT_ICE_GENERIC              1082
#define OT_ICE_MAIN                 1083



#define OT_STARDB_COMPONENT        1100

// MAKE SURE NEW OT_xxx VALUES DO NOT OVERLAP WITH OT_yyy VALUES IN TREE.H !

#define RES_ERR                 0
#define RES_SUCCESS             1
#define RES_TIMEOUT             2
#define RES_NOGRANT             3
#define RES_ENDOFDATA           4
#define RES_ALREADYEXIST        5
#define RES_30100_TABLENOTFOUND 6  // table, view or index not found

#define RES_FILENOEXIST         7
#define RES_FILELOCKED          8
#define RES_SQLNOEXEC           9
#define RES_DDPROPAG_WRONGTYPE 10
#define RES_USER_CANCEL        11 
#define RES_NOTDBA             12

#define RES_WRONGLOCALNODE     14
#define RES_MOREDBEVENT        15
#define RES_NOT_OI             16
#define RES_CANNOT_SOLVE       17
#define RES_MULTIPLE_GRANTS    18
#define RES_PAGE_SIZE_NOT_ENOUGH 19
#define RES_CHANGE_PAGESIZE_NEEDED 20

#define INGTYPE_ERROR         0

#define INGTYPE_C             1
#define INGTYPE_CHAR          2
#define INGTYPE_TEXT          3
#define INGTYPE_VARCHAR       4
#define INGTYPE_LONGVARCHAR   5
#define INGTYPE_INT4          6
#define INGTYPE_INT2          7
#define INGTYPE_INT1          8
#define INGTYPE_DECIMAL       9 
#define INGTYPE_FLOAT8       10
#define INGTYPE_FLOAT4       11
#define INGTYPE_DATE         12
#define INGTYPE_MONEY        13
#define INGTYPE_BYTE         14
#define INGTYPE_BYTEVAR      15
#define INGTYPE_LONGBYTE     16

#define INGTYPE_OBJKEY       17
#define INGTYPE_TABLEKEY     18
#define INGTYPE_SECURITYLBL  19
#define INGTYPE_SHORTSECLBL  20

#define INGTYPE_FLOAT        21
#define INGTYPE_INTEGER      22     // equivalent to INGTYPE_INT4

#define INGTYPE_UNICODE_NCHR   23   // unicode nchar
#define INGTYPE_UNICODE_NVCHR  24   // unicode nvarchar
#define INGTYPE_UNICODE_LNVCHR 25   // unicode long nvarchar

#define INGTYPE_INT8         26
#define INGTYPE_BIGINT       27

#define INGTYPE_ADTE         28     // ansidate
#define INGTYPE_TMWO         29     // time without time zone
#define INGTYPE_TMW          30     // time with time zone
#define INGTYPE_TME          31     // time with local time zone
#define INGTYPE_TSWO         32     // timestamp without time zone
#define INGTYPE_TSW          33     // timestamp with time zone
#define INGTYPE_TSTMP        34     // timestamp with local time zone
#define INGTYPE_INYM         35     // interval year to month
#define INGTYPE_INDS         36     // interval day to second
#define INGTYPE_IDTE         37     // ingresdate
#define INGTYPE_BOOLEAN	     38     // boolean


#define REPLIC_FULLPEER      1   /* these values should not be changed */
#define REPLIC_PROT_RO       2   /* defined by Ingres                  */
#define REPLIC_UNPROT_RO     3
#define REPLIC_EACCESS       4
#define REPLIC_TYPE_UNDEFINED 100 /* this values is only for replicator 1.1 */
                                  /* when the replicator type is not define */

//ps 05/03/96 for new replicator version
#define REPLIC_ERROR_SKIP_TRANSACTION 0
#define REPLIC_ERROR_SKIP_ROW         1
#define REPLIC_ERROR_QUIET_CDDS       2
#define REPLIC_ERROR_QUIET_DATABASE   3
#define REPLIC_ERROR_QUIET_SERVER     4

//ps 05/03/96 for new replicator version
#define REPLIC_COLLISION_PASSIVE    0
#define REPLIC_COLLISION_ACTIVE     1
#define REPLIC_COLLISION_BEGNIN     2
#define REPLIC_COLLISION_PRIORITY   3
#define REPLIC_COLLISION_LASTWRITE  4

#define ATTACHED_UNKNOWN  (char) 0
#define ATTACHED_YES      (char) 1
#define ATTACHED_NO       (char) 2

extern int imaxsessions;

BOOL CanObjectHaveSchema(int iobjecttype);
BOOL IsSystemObject    (int iobjecttype,LPUCHAR lpobjectname,LPUCHAR lpobjectowner);
BOOL HasSystemObjects  (int iobjecttype);
BOOL CanObjectBeDeleted(int iobjecttype,LPUCHAR lpobjectname,LPUCHAR lpobjectowner);
BOOL CanObjectBeAltered(int iobjecttype,LPUCHAR lpobjectname,LPUCHAR lpobjectowner);
BOOL HasProperties4display(int iobjecttype);
BOOL CanObjectBeAdded(int iobjecttype);
BOOL CanObjectBeCopied(int iobjecttype, LPUCHAR lpobjectname, LPUCHAR lpobjectowner);
BOOL CanObjectStructureBeModified(int iobjecttype);
BOOL CanObjectExistInVectorWise(int iobjecttype);
BOOL HasOwner(int iobjecttype);
BOOL NonDBObjectHasOwner(int iobjecttype);
BOOL HasExtraDisplayString(int iobjecttype);
int  IngGetStringType(LPUCHAR lpstring, int ilen);
int  DOMTreeCmpStr(LPUCHAR p1, LPUCHAR powner1, LPUCHAR p2, LPUCHAR powner2, int iobjecttype, BOOL bEqualIfMixed);
int DOMTreeCmpTableNames(LPUCHAR p1,LPUCHAR powner1,LPUCHAR p2,LPUCHAR powner2);
int DOMTreeCmpllGrants(LPUCHAR p1,LPUCHAR powner1,LPUCHAR pextra1,LPUCHAR p2,LPUCHAR powner2,LPUCHAR pextra2);
int  DOMTreeCmpDoubleStr(LPUCHAR p11,LPUCHAR p12,LPUCHAR p21,LPUCHAR p22);
       /* DOMTreeCmpDoubleStr compares from left to right */
       /* p11 p12: first object,  p21 p22 second object   */
int DOMTreeCmpTablesWithDB(LPUCHAR p11,LPUCHAR p12,LPUCHAR p13,LPUCHAR p21,LPUCHAR p22,LPUCHAR p23);
int  DOMTreeCmpSecAlarms(LPUCHAR p1, long l1, LPUCHAR p2, long l2);
int DOMTreeCmpRelSecAlarms(LPUCHAR p11,LPUCHAR p12,LPUCHAR p1owner,long l1,LPUCHAR p21,LPUCHAR p22,LPUCHAR p2owner,long l2);
int DOMTreeCmpllSecAlarms (LPUCHAR p11,LPUCHAR p1owner, LPUCHAR usr1, long l1, LPUCHAR p21, LPUCHAR p2owner, LPUCHAR usr2, long l2);
       /* DOMTreeCmpSecAlarms compares from left to right : */
       /* p11 p12: first object,  p21 p22 second object, l1 l2 values */
int DOMTreeCmp4ints(int i11,int i12,int i13,int i14,int i21,int i22,int i23,int i24);
LPUCHAR GetExtraDisplayStringCaption(LPUCHAR lpstring, int iobjType);
int  GetParentLevelFromObjType(int iobjecttype);
LPUCHAR lppublicdispstring(void);
LPUCHAR lppublicsysstring(void);
int DBAGetFirstSubType(int iobjecttype);
int DBAGetNextSubType(void);
#ifndef LIBMON_HEADER
BOOL DBAGetSessionUser(LPUCHAR lpVirtNode, LPUCHAR lpresultusername);
BOOL DBAGetSessionUserWithCheck(LPUCHAR lpVirtNode, LPUCHAR lpresultusername, BOOL bCheck);
#endif
BOOL DBAGetCurrentUser(LPUCHAR lpresultusername);
int isDBGranted  (int hnodestruct,LPUCHAR lpDBName,LPUCHAR lpgranteename,BOOL * pReturn);
int HasDBNoaccess(int hnodestruct,LPUCHAR lpDBName,LPUCHAR lpgranteename,BOOL * pReturn);
void checkgcnnodes();
void vcheckgcnnodes();
void checknogcnnodes();
void vchecknogcnnodes();
BOOL IsQueryBlockingDefined();

#ifndef LIBMON_HEADER
// "Thread safe" redirection of MessageBox() and GetFocus(). Should be called instead of MessageBox
// when the caller may be executed in a secondary thread
int TS_MessageBox( HWND hWnd, LPCTSTR pTxt, LPCTSTR pTitle, UINT uType); 
HWND TS_GetFocus() ;
LRESULT TS_SendMessage( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam );
BOOL CheckConnection(LPUCHAR lpVirtNode, BOOL bForceSession,BOOL bDisplayErrorMessage);
#endif



//
//  STAR SUPPORT
//

// For Star support: Database types - used for OT_DATABASE
#define DBTYPE_REGULAR      0
#define DBTYPE_DISTRIBUTED  1
#define DBTYPE_COORDINATOR  2
#define DBTYPE_ERROR        3

// For Star support: Objects types - used for OT_TABLE and OT_VIEW
#define OBJTYPE_NOTSTAR     0
#define OBJTYPE_STARNATIVE  1
#define OBJTYPE_STARLINK    2
#define OBJTYPE_UNKNOWN     3

BOOL IsStarDatabase(int hnodestruct,LPUCHAR lpDBName);      // TRUE if DBTYPE_DISTRIBUTED only
int  GetDatabaseStarType(int hnodestruct,LPUCHAR lpDBName); // DBTYPE_???

//
//  OpenIngres tools catalog support
//
#define MAX_TOOL_PRODUCTS  4
typedef enum {
	TOOLS_INGRES=0,
	TOOLS_INGRESDBD,
	TOOLS_VISION,
	TOOLS_W4GL
} IDX_PRODUCTS;
#endif  /* #define DBA_HEADER */

