/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdomc02.cpp, Implementation file 
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : 
**    Purpose  : Modeless Dialog Box that will appear at the right pane of the 
**               DOM Frame (CChildFrame)
**               It contains a TabCtrl and maintains the pages (Modeless Dialogs)
**               that will appeare at each tab. It also keeps track of the current 
**               object being displayed. When we get the new object to display,
**               we check if it is the same Class of the Current Object for the 
**               purpose of just updating the data of the current page.
**
** History after 12-Sep-2000:
**
** 12-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**    Show/Hide the control m_staticHeader depending on 
**    CMainMfcApp::GetShowRightPaneHeader()
** 27-Mar-2001 (noifr01)
**      (sir 104270) removal of code for managing Ingres/Desktop
** 04-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 30-May-2001 (uk$so01)
**    SIR #104736, Integrate IIA and Cleanup old code of populate.
** 13-Dec-2001 (noifr01)
**    restored the end of the file that seemed to have disappeared
** 21-Mar-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
** 02-Apr-2003 (schph01)
**    SIR #107523, Add Properties panes for Sequences.
** 28-Apr-2004 (uk$so01)
**    BUG #112221 / ISSUE 13403162
**    VC7 (.Net) does not call OnInitialUpdate first for this Frame/View/Doc but it does call
**    OnUpdate. So I write void InitUpdate(); that is called once either in OnUpdate or
**    OnInitialUpdate() to resolve this problem.
** 08-Nov-2004 (uk$so01)
**    BUG #113400 / ISSUE #13748992.
**    Avoid reseizing the window if its width or height is 0.
**/

// DgDomC02.cpp : implementation file
//

#include "stdafx.h"
#include "rcdepend.h"
#include "vtree.h"
#include "dgdomc02.h"

#include "maindoc.h"
#include "mainview.h"
#include "childfrm.h"

#include "dlgvfrm.h"    // CuDlgViewFrame: for scrollables right panes

#include "dgdploc.h"    // CuDlgDomPropLocation
#include "dgdpusr.h"    // CuDlgDomPropUser
#include "dgdprol.h"    // CuDlgDomPropRole
#include "dgdppro.h"    // CuDlgDomPropProfile
#include "dgdpidx.h"    // CuDlgDomPropIndex
#include "dgdpint.h"    // CuDlgDomPropIntegrity
#include "dgdprul.h"    // CuDlgDomPropRule
#include "dgdpprc.h"    // CuDlgDomPropProcedure (Prc)
#include "dgdpgrp.h"    // CuDlgDomPropGroup
#include "dgdpdbal.h"   // CuDlgDomPropDbAlarms
#include "dgdptbal.h"   // CuDlgDomPropTableAlarms
#include "dgdptbgr.h"   // CuDlgDomPropTableGrantees
#include "dgdpdbgr.h"   // CuDlgDomPropDbGrantees
#include "dgdpdb.h"     // CuDlgDomPropDb
#include "dgdptb.h"     // CuDlgDomPropTable
#include "dgdpvigr.h"   // CuDlgDomPropViewGrantees
#include "dgdpvi.h"     // CuDlgDomPropView
#include "dgdpcd.h"     // CuDlgDomPropCdds
#include "dgdpcdtb.h"   // CuDlgDomPropCddsTables
#include "dgdplocs.h"   // CuDlgDomPropLocationSpace
#include "dgdplocd.h"   // CuDlgDomPropLocationDbs
#include "dgdpevgr.h"   // CuDlgDomPropDbEventGrantees
#include "dgdpprcg.h"   // CuDlgDomPropProcGrantees
#include "dgdptbco.h"   // CuDlgDomPropTableColumns

#include "dgdprpcd.h"   // CuDlgDomPropReplicCdds
#include "dgdprpco.h"   // CuDlgDomPropReplicConn
#include "dgdprpmu.h"   // CuDlgDomPropReplicMailU
#include "dgdprprt.h"   // CuDlgDomPropReplicRegTbl

#include "dgdpdbtb.h"   // CuDlgDomPropDbTbl
#include "dgdpdbvi.h"   // CuDlgDomPropDbView
#include "dgdpdbpr.h"   // CuDlgDomPropDbProc
#include "dgdpdbsc.h"   // CuDlgDomPropDbSchema
#include "dgdpdbsy.h"   // CuDlgDomPropDbSyn
#include "dgdpdbev.h"   // CuDlgDomPropDbEvent

#include "dgdptbig.h"   // CuDlgDomPropTableInteg
#include "dgdptbru.h"   // CuDlgDomPropTableRule
#include "dgdptbix.h"   // CuDlgDomPropTableIdx
#include "dgdptblo.h"   // CuDlgDomPropTableLoc
#include "dgdptrow.h"   // CuDlgDomPropTableRows
#include "dgdptbst.h"   // CuDlgDomPropTableStatistic

#include "dgdpusxa.h"   // CuDlgDomPropUserXAlarm (X like Cross-Ref)

#include "dgdpusxd.h"   // CuDlgDomPropUserXGrantedDb
#include "dgdpusxt.h"   // CuDlgDomPropUserXGrantedTbl
#include "dgdpusxv.h"   // CuDlgDomPropUserXGrantedView
#include "dgdpusxe.h"   // CuDlgDomPropUserXGrantedEvent
#include "dgdpusxp.h"   // CuDlgDomPropUserXGrantedProc
#include "dgdpusxr.h"   // CuDlgDomPropUserXGrantedRole
#include "dgdpusxs.h"   // CuDlgDomPropUserXGrantedSeq

#include "dgdpgrxd.h"   // CuDlgDomPropGroupXGrantedDb
#include "dgdpgrxt.h"   // CuDlgDomPropGroupXGrantedTbl
#include "dgdpgrxv.h"   // CuDlgDomPropGroupXGrantedView
#include "dgdpgrxe.h"   // CuDlgDomPropGroupXGrantedEvent
#include "dgdpgrxp.h"   // CuDlgDomPropGroupXGrantedProc
#include "dgdpgrxs.h"   // CuDlgDomPropGroupXGrantedSeq

#include "dgdproxd.h"   // CuDlgDomPropRoleXGrantedDb
#include "dgdproxt.h"   // CuDlgDomPropRoleXGrantedTbl
#include "dgdproxv.h"   // CuDlgDomPropRoleXGrantedView
#include "dgdproxe.h"   // CuDlgDomPropRoleXGrantedEvent
#include "dgdproxp.h"   // CuDlgDomPropRoleXGrantedProc
#include "dgdproxs.h"   // CuDlgDomPropRoleXGrantedSeq

#include "dgdplosu.h"   // CuDlgDomPropLocations (Su like Summary)

// Distributed objects
#include "dgdpdd.h"     // CuDlgDomPropDDb
#include "dgdpddtb.h"   // CuDlgDomPropDDbTbl
#include "dgdpddvi.h"   // CuDlgDomPropDDbView
#include "dgdpddpr.h"   // CuDlgDomPropDDbProc

#include "dgdpdvn.h"    // CuDlgDomPropViewNative
#include "dgdpdvl.h"    // CuDlgDomPropViewLink
// #include "dgdpdpl.h"    // CuDlgDomPropProcLink
#include "dgdpdtlx.h"   // CuDlgDomPropTableLinkIdx

#include "dgdpsctb.h"   // CuDlgDomPropSchemaTbl
#include "dgdpscvi.h"   // CuDlgDomPropSchemaView
#include "dgdpscpr.h"   // CuDlgDomPropSchemaProc

#include "dgdptbpg.h"   // CuDlgDomPropTblPages
//#include "dgdpixpg.h"   // CuDlgDomPropIndexPages

#include "dgdpdtl.h"    // CuDlgDomPropTableLink
#include "dgstarco.h"   // CuDlgDomPropStarDbComponent

#include "dgdptrna.h"   // CuDlgDomPropTableRowsNA

#include "dgdpstdb.h"   // CuDlgDomPropStaticDb
#include "dgdpstpr.h"   // CuDlgDomPropStaticProfile
#include "dgdpstus.h"   // CuDlgDomPropStaticUser
#include "dgdpstgr.h"   // CuDlgDomPropStaticGroup
#include "dgdpstro.h"   // CuDlgDomPropStaticRole
#include "dgdpstlo.h"   // CuDlgDomPropStaticLocation
#include "dgdpstgd.h"   // CuDlgDomPropStaticGwDb

#include "dgdpwsro.h"   // CuDlgDomPropIceSecRoles
#include "dgdpwsdu.h"   // CuDlgDomPropIceSecDbusers
#include "dgdpwsco.h"   // CuDlgDomPropIceSecDbconns
#include "dgdpwswu.h"   // CuDlgDomPropIceSecWebusers
#include "dgdpwspr.h"   // CuDlgDomPropIceSecProfiles

#include "dgdpwwro.h"   // CuDlgDomPropIceSecWebuserRoles
#include "dgdpwwco.h"   // CuDlgDomPropIceSecWebuserDbconns
#include "dgdpwpro.h"   // CuDlgDomPropIceSecProfileRoles
#include "dgdpwpco.h"   // CuDlgDomPropIceSecProfileDbconns

#include "dgdpwvap.h"   // CuDlgDomPropIceServerApplications
#include "dgdpwvlo.h"   // CuDlgDomPropIceServerLocations
#include "dgdpwvva.h"   // CuDlgDomPropIceServerVariables

#include "dgdpauus.h"   // Adit All User:   CuDlgDomPropInstallationLevelAuditAllUsers
#include "dgdpaulo.h"   // Adit Log:        CuDlgDomPropInstallationLevelAuditLog
#include "dgdpenle.h"   // Enabled Level:   CuDlgDomPropInstallationLevelEnabledLevel

#include "dgdpwb.h"     // CuDlgDomPropIceBunits
#include "dgdpwf.h"     // CuDlgDomPropIceFacets
#include "dgdpwp.h"     // CuDlgDomPropIcePages
#include "dgdpwl.h"     // CuDlgDomPropIceLocations
#include "dgdpwbro.h"   // CuDlgDomPropIceBunitRoles
#include "dgdpwbus.h"   // CuDlgDomPropIceBunitUsers

#include "dgdpwfpr.h"   // CuDlgDomPropIceFacetNPageRoles
#include "dgdpwfpu.h"   // CuDlgDomPropIceFacetNPageUsers

#include "dgdpihtm.h"   // CuDlgDomPropIceHtml
#include "dgdpseq.h"    // CuDlgDomPropSeq

#include "dgdpdbse.h"   // CuDlgDomPropDbSeq
#include "dgdpseqg.h"   // CuDlgDomPropSeqGrantees

extern "C"
{

#include "dba.h"
#include "dbaginfo.h"

#include "main.h"
#include "dom.h"

// for dom tree access
#include "tree.h"
#include "treelb.e"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


///////////////////////////////////////////////////////////////////////
// static utility functions

inline static int compare (const void* e1, const void* e2)
{
    UINT o1 = *((UINT*)e1);
    UINT o2 = *((UINT*)e2);
    return (o1 > o2)? 1: (o1 < o2)? -1: 0;
}

static int Find (UINT nIDD, DOMDETAILDLGSTRUCT* pArray, int first, int last)
{
    if (first <= last)
    {
        int mid = (first + last) / 2;
        if (pArray [mid].nIDD == nIDD)
            return mid;
        else
        if (pArray [mid].nIDD > nIDD)
            return Find (nIDD, pArray, first, mid-1);
        else
            return Find (nIDD, pArray, mid+1, last);
    }
    return -1;
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomTabCtrl dialog

CuDlgDomTabCtrl::CuDlgDomTabCtrl(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomTabCtrl::IDD, pParent)
{
    UINT nArrayIDD [NUMBEROFDOMPAGES] = 
    {
      IDD_DOMPROP_LOCATION,
      IDD_DOMPROP_USER,
      IDD_DOMPROP_ROLE,
      IDD_DOMPROP_PROF,
      IDD_DOMPROP_INDEX,
      IDD_DOMPROP_INTEG,
      IDD_DOMPROP_RULE,
      IDD_DOMPROP_PROC,
      IDD_DOMPROP_SEQ,
      IDD_DOMPROP_GROUP,
      IDD_DOMPROP_DB_ALARMS,
      IDD_DOMPROP_TABLE_ALARMS,
      IDD_DOMPROP_TABLE_GRANTEES,
      IDD_DOMPROP_DB_GRANTEES,
      IDD_DOMPROP_DB,
      IDD_DOMPROP_TABLE,
      IDD_DOMPROP_VIEW_GRANTEES,
      IDD_DOMPROP_VIEW,
      IDD_DOMPROP_CDDS,
      IDD_DOMPROP_CDDS_TBL,
      IDD_IPMPAGE_LOCATION,   // clone from ipm - branched to CuDlgDomPropLocationSpace
      IDD_IPMDETAIL_LOCATION, // clone from ipm - branched to CuDlgDomPropLocationDbs
      IDD_DOMPROP_DBEV_GRANTEES,
      IDD_DOMPROP_PROC_GRANTEES,
      IDD_DOMPROP_SEQ_GRANTEES,
      IDD_DOMPROP_TABLE_COLUMNS,

      IDD_DOMPROP_REPL_CDDS,
      IDD_DOMPROP_REPL_CONN,
      IDD_DOMPROP_REPL_MAILU,
      IDD_DOMPROP_REPL_REGTBL,

      IDD_DOMPROP_DB_TBLS,
      IDD_DOMPROP_DB_VIEWS,
      IDD_DOMPROP_DB_PROCS,
      IDD_DOMPROP_DB_SEQS,
      IDD_DOMPROP_DB_SCHEMAS,
      IDD_DOMPROP_DB_SYNS,
      IDD_DOMPROP_DB_EVTS,

      IDD_DOMPROP_TABLE_INTEG,
      IDD_DOMPROP_TABLE_RULE,
      IDD_DOMPROP_TABLE_IDX,
      IDD_DOMPROP_TABLE_LOC,
      IDD_DOMPROP_TABLE_ROWS,
      IDD_DOMPROP_TABLE_STATISTIC,

      IDD_DOMPROP_USER_XALARMS,

      IDD_DOMPROP_USER_XGRANTED_DB,
      IDD_DOMPROP_USER_XGRANTED_TBL,
      IDD_DOMPROP_USER_XGRANTED_VIEW,
      IDD_DOMPROP_USER_XGRANTED_DBEV,
      IDD_DOMPROP_USER_XGRANTED_PROC,
      IDD_DOMPROP_USER_XGRANTED_ROLE,
      IDD_DOMPROP_USER_XGRANTED_SEQ,

      IDD_DOMPROP_GROUP_XGRANTED_DB,
      IDD_DOMPROP_GROUP_XGRANTED_TBL,
      IDD_DOMPROP_GROUP_XGRANTED_VIEW,
      IDD_DOMPROP_GROUP_XGRANTED_DBEV,
      IDD_DOMPROP_GROUP_XGRANTED_PROC,
      IDD_DOMPROP_GROUP_XGRANTED_SEQ,

      IDD_DOMPROP_ROLE_XGRANTED_DB,
      IDD_DOMPROP_ROLE_XGRANTED_TBL,
      IDD_DOMPROP_ROLE_XGRANTED_VIEW,
      IDD_DOMPROP_ROLE_XGRANTED_DBEV,
      IDD_DOMPROP_ROLE_XGRANTED_PROC,
      IDD_DOMPROP_ROLE_XGRANTED_SEQ,

      IDD_IPMDETAIL_LOCATIONS, // clone from ipm - branched to CuDlgDomPropLocations

      IDD_DOMPROP_CONNECTION,

      IDD_DOMPROP_DDB,
      IDD_DOMPROP_DDB_TBLS,
      IDD_DOMPROP_DDB_VIEWS,
      IDD_DOMPROP_DDB_PROCS,
      IDD_DOMPROP_STARDB_COMPONENT,

      IDD_DOMPROP_STARTBL_L_IDX,
      IDD_DOMPROP_STARPROC_L,
      IDD_DOMPROP_STARVIEW_L,
      IDD_DOMPROP_STARVIEW_N,

      IDD_DOMPROP_SCHEMA_TBLS,
      IDD_DOMPROP_SCHEMA_VIEWS,
      IDD_DOMPROP_SCHEMA_PROCS,

      IDD_DOMPROP_TABLE_PAGES,
      IDD_DOMPROP_INDEX_PAGES,

      IDD_DOMPROP_STARTBL_L,

      IDD_DOMPROP_INDEX_L_SOURCE,
      IDD_DOMPROP_PROC_L_SOURCE,
      IDD_DOMPROP_VIEW_L_SOURCE,
      IDD_DOMPROP_VIEW_N_SOURCE,

      IDD_DOMPROP_TABLE_ROWSNA,

      IDD_DOMPROP_ST_DB,
      IDD_DOMPROP_ST_PROFILE,
      IDD_DOMPROP_ST_USER,
      IDD_DOMPROP_ST_GROUP,
      IDD_DOMPROP_ST_ROLE,
      IDD_DOMPROP_ST_LOCATION,
      IDD_DOMPROP_ST_GWDB,

      IDD_DOMPROP_ICE_DBCONNECTION,
      IDD_DOMPROP_ICE_DBUSER,
      IDD_DOMPROP_ICE_PROFILE,
      IDD_DOMPROP_ICE_ROLE,
      IDD_DOMPROP_ICE_WEBUSER,
      IDD_DOMPROP_ICE_SERVER_LOCATION,
      IDD_DOMPROP_ICE_SERVER_VARIABLE,
      IDD_DOMPROP_ICE_BUNIT,
      IDD_DOMPROP_ICE_SEC_ROLES,
      IDD_DOMPROP_ICE_SEC_DBUSERS,
      IDD_DOMPROP_ICE_SEC_DBCONNS,
      IDD_DOMPROP_ICE_SEC_WEBUSERS,
      IDD_DOMPROP_ICE_SEC_PROFILES,
      IDD_DOMPROP_ICE_WEBUSER_ROLES,
      IDD_DOMPROP_ICE_WEBUSER_CONNECTIONS,
      IDD_DOMPROP_ICE_PROFILE_ROLES,
      IDD_DOMPROP_ICE_PROFILE_CONNECTIONS,
      IDD_DOMPROP_ICE_SERVER_APPLICATIONS,
      IDD_DOMPROP_ICE_SERVER_LOCATIONS,
      IDD_DOMPROP_ICE_SERVER_VARIABLES,

      IDD_DOMPROP_INSTLEVEL_AUDITUSERS,
      IDD_DOMPROP_INSTLEVEL_ENABLEDLEVEL,
      IDD_DOMPROP_INSTLEVEL_AUDITLOG,

      IDD_DOMPROP_ICE_FACETNPAGE,
      IDD_DOMPROP_ICE_BUNIT_ACCESSDEF,
      IDD_DOMPROP_ICE_BUNITS,
      IDD_DOMPROP_ICE_SEC_ROLE,
      IDD_DOMPROP_ICE_SEC_DBUSER,
      IDD_DOMPROP_ICE_FACETS,
      IDD_DOMPROP_ICE_PAGES,
      IDD_DOMPROP_ICE_LOCATIONS,

      IDD_DOMPROP_ICE_BUNIT_ROLES,
      IDD_DOMPROP_ICE_BUNIT_USERS,

      IDD_DOMPROP_ICE_FACETNPAGE_ROLES,
      IDD_DOMPROP_ICE_FACETNPAGE_USERS,

      IDD_DOMPROP_ICE_HTML_IMAGE,
      IDD_DOMPROP_ICE_HTML_SOURCE,

    };

    qsort ((void*)nArrayIDD, (size_t)NUMBEROFDOMPAGES, (size_t)sizeof(UINT), compare);
    for (int i=0; i<NUMBEROFDOMPAGES; i++)
    {
        m_arrayDlg[i].nIDD = nArrayIDD [i];
        m_arrayDlg[i].pPage= NULL;
    }
    m_pCurrentProperty = NULL;
    m_pCurrentPage = NULL;
    m_bIsLoading   = FALSE;
    //{{AFX_DATA_INIT(CuDlgDomTabCtrl)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomTabCtrl::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CuDlgDomTabCtrl)
    //}}AFX_DATA_MAP
}

CWnd* CuDlgDomTabCtrl::ConstructPage (UINT nIDD)
{
  switch (nIDD) {
    case IDD_DOMPROP_TABLE:           return DomPropPage_Table(nIDD);
    case IDD_DOMPROP_LOCATION:        return DomPropPage_Location(nIDD);
    case IDD_DOMPROP_USER:            return DomPropPage_User(nIDD);
    case IDD_DOMPROP_ROLE:            return DomPropPage_Role(nIDD);
    case IDD_DOMPROP_PROF:            return DomPropPage_Profile(nIDD);
    case IDD_DOMPROP_INDEX:           return DomPropPage_Index(nIDD);
    case IDD_DOMPROP_INTEG:           return DomPropPage_Integrity(nIDD);
    case IDD_DOMPROP_RULE:            return DomPropPage_Rule(nIDD);
    case IDD_DOMPROP_PROC:            return DomPropPage_Procedure(nIDD);
    case IDD_DOMPROP_SEQ:             return DomPropPage_Sequence(nIDD);
    case IDD_DOMPROP_GROUP:           return DomPropPage_Group(nIDD);
    case IDD_DOMPROP_DB_ALARMS:       return DomPropPage_DbAlarms(nIDD);
    case IDD_DOMPROP_TABLE_ALARMS:    return DomPropPage_TableAlarms(nIDD);
    case IDD_DOMPROP_TABLE_GRANTEES:  return DomPropPage_TableGrantees(nIDD);
    case IDD_DOMPROP_DB_GRANTEES:     return DomPropPage_DbGrantees(nIDD);
    case IDD_DOMPROP_DB:              return DomPropPage_Db(nIDD);
    case IDD_DOMPROP_VIEW_GRANTEES:   return DomPropPage_ViewGrantees(nIDD);
    case IDD_DOMPROP_VIEW:            return DomPropPage_View(nIDD);
    case IDD_DOMPROP_CDDS:            return DomPropPage_Cdds(nIDD);
    case IDD_DOMPROP_CDDS_TBL:        return DomPropPage_CddsTables(nIDD);
    case IDD_IPMPAGE_LOCATION:        return DomPropPage_LocationSpace(nIDD);
    case IDD_IPMDETAIL_LOCATION:      return DomPropPage_LocationDbs(nIDD);
    case IDD_DOMPROP_DBEV_GRANTEES:   return DomPropPage_DbEventGrantees(nIDD);
    case IDD_DOMPROP_PROC_GRANTEES:   return DomPropPage_ProcGrantees(nIDD);
    case IDD_DOMPROP_SEQ_GRANTEES:    return DomPropPage_SeqGrantees(nIDD);
    case IDD_DOMPROP_TABLE_COLUMNS:   return DomPropPage_TableColumns(nIDD);

    case IDD_DOMPROP_REPL_CDDS:       return DomPropPage_ReplicCdds(nIDD);
    case IDD_DOMPROP_REPL_CONN:       return DomPropPage_ReplicConn(nIDD);
    case IDD_DOMPROP_REPL_MAILU:      return DomPropPage_ReplicMailU(nIDD);
    case IDD_DOMPROP_REPL_REGTBL:     return DomPropPage_ReplicRegTbl(nIDD);
                                      
    case IDD_DOMPROP_DB_TBLS:         return DomPropPage_DbTbls(nIDD);
    case IDD_DOMPROP_DB_VIEWS:        return DomPropPage_DbViews(nIDD);
    case IDD_DOMPROP_DB_PROCS:        return DomPropPage_DbProcs(nIDD);
    case IDD_DOMPROP_DB_SEQS:         return DomPropPage_DbSeqs(nIDD);
    case IDD_DOMPROP_DB_SCHEMAS:      return DomPropPage_DbSchemas(nIDD);
    case IDD_DOMPROP_DB_SYNS:         return DomPropPage_DbSynonyms(nIDD);
    case IDD_DOMPROP_DB_EVTS:         return DomPropPage_DbEvents(nIDD);

    case IDD_DOMPROP_TABLE_INTEG:     return DomPropPage_TableInteg(nIDD);
    case IDD_DOMPROP_TABLE_RULE:      return DomPropPage_TableRule(nIDD);
    case IDD_DOMPROP_TABLE_IDX:       return DomPropPage_TableIdx(nIDD);
    case IDD_DOMPROP_TABLE_LOC:       return DomPropPage_TableLoc(nIDD);
    case IDD_DOMPROP_TABLE_ROWS:      return DomPropPage_TableRows(nIDD);
    case IDD_DOMPROP_TABLE_STATISTIC: return DomPropPage_TableStatistic(nIDD);

    case IDD_DOMPROP_USER_XALARMS:    return DomPropPage_UserXAlarms(nIDD);

    case IDD_DOMPROP_USER_XGRANTED_DB:   return DomPropPage_UserXGrantedDb(nIDD);
    case IDD_DOMPROP_USER_XGRANTED_TBL:  return DomPropPage_UserXGrantedTbl(nIDD);
    case IDD_DOMPROP_USER_XGRANTED_VIEW: return DomPropPage_UserXGrantedView(nIDD);
    case IDD_DOMPROP_USER_XGRANTED_DBEV: return DomPropPage_UserXGrantedEvent(nIDD);
    case IDD_DOMPROP_USER_XGRANTED_PROC: return DomPropPage_UserXGrantedProc(nIDD);
    case IDD_DOMPROP_USER_XGRANTED_ROLE: return DomPropPage_UserXGrantedRole(nIDD);
    case IDD_DOMPROP_USER_XGRANTED_SEQ:  return DomPropPage_UserXGrantedSeq(nIDD);

    case IDD_DOMPROP_GROUP_XGRANTED_DB:   return DomPropPage_GroupXGrantedDb(nIDD);
    case IDD_DOMPROP_GROUP_XGRANTED_TBL:  return DomPropPage_GroupXGrantedTbl(nIDD);
    case IDD_DOMPROP_GROUP_XGRANTED_VIEW: return DomPropPage_GroupXGrantedView(nIDD);
    case IDD_DOMPROP_GROUP_XGRANTED_DBEV: return DomPropPage_GroupXGrantedEvent(nIDD);
    case IDD_DOMPROP_GROUP_XGRANTED_PROC: return DomPropPage_GroupXGrantedProc(nIDD);
    case IDD_DOMPROP_GROUP_XGRANTED_SEQ:  return DomPropPage_GroupXGrantedSeq(nIDD);

    case IDD_DOMPROP_ROLE_XGRANTED_DB:   return DomPropPage_RoleXGrantedDb(nIDD);
    case IDD_DOMPROP_ROLE_XGRANTED_TBL:  return DomPropPage_RoleXGrantedTbl(nIDD);
    case IDD_DOMPROP_ROLE_XGRANTED_VIEW: return DomPropPage_RoleXGrantedView(nIDD);
    case IDD_DOMPROP_ROLE_XGRANTED_DBEV: return DomPropPage_RoleXGrantedEvent(nIDD);
    case IDD_DOMPROP_ROLE_XGRANTED_PROC: return DomPropPage_RoleXGrantedProc(nIDD);
    case IDD_DOMPROP_ROLE_XGRANTED_SEQ:  return DomPropPage_RoleXGrantedSeq(nIDD);

    case IDD_IPMDETAIL_LOCATIONS:        return DomPropPage_Locations(nIDD);

    case IDD_DOMPROP_CONNECTION:         return DomPropPage_Connection(nIDD);

    // distributed objects
    case IDD_DOMPROP_DDB:              return DomPropPage_DDb(nIDD);
    case IDD_DOMPROP_DDB_TBLS:         return DomPropPage_DDbTbls(nIDD);
    case IDD_DOMPROP_DDB_VIEWS:        return DomPropPage_DDbViews(nIDD);
    case IDD_DOMPROP_DDB_PROCS:        return DomPropPage_DDbProcs(nIDD);
    case IDD_DOMPROP_STARDB_COMPONENT: return DomPropPage_StarDbComponent(nIDD);

    case IDD_DOMPROP_STARTBL_L_IDX:    return DomPropPage_StarTableLinkIndexes(nIDD);
    case IDD_DOMPROP_STARPROC_L:       return DomPropPage_StarProcLink(nIDD);
    case IDD_DOMPROP_STARVIEW_L:       return DomPropPage_StarViewLink(nIDD);
    case IDD_DOMPROP_STARVIEW_N:       return DomPropPage_StarViewNative(nIDD);

    case IDD_DOMPROP_SCHEMA_TBLS:      return DomPropPage_SchemaTbls(nIDD);
    case IDD_DOMPROP_SCHEMA_VIEWS:     return DomPropPage_SchemaViews(nIDD);
    case IDD_DOMPROP_SCHEMA_PROCS:     return DomPropPage_SchemaProcs(nIDD);

    case IDD_DOMPROP_TABLE_PAGES:      return DomPropPage_TablePages(nIDD);
    case IDD_DOMPROP_INDEX_PAGES:      return DomPropPage_IndexPages(nIDD);

    case IDD_DOMPROP_STARTBL_L:        return DomPropPage_StarTableLink(nIDD);

    case IDD_DOMPROP_INDEX_L_SOURCE:   return DomPropPage_StarIndexLinkSource(nIDD);
    case IDD_DOMPROP_PROC_L_SOURCE:    return DomPropPage_StarProcLinkSource(nIDD);
    case IDD_DOMPROP_VIEW_L_SOURCE:    return DomPropPage_StarViewLinkSource(nIDD);
    case IDD_DOMPROP_VIEW_N_SOURCE:    return DomPropPage_StarViewNativeSource(nIDD);

    case IDD_DOMPROP_TABLE_ROWSNA:     return DomPropPage_TableRowsNA(nIDD);

    // Static root items
    case IDD_DOMPROP_ST_DB:            return DomPropPage_StaticDb(nIDD);
    case IDD_DOMPROP_ST_PROFILE:       return DomPropPage_StaticProfile(nIDD);
    case IDD_DOMPROP_ST_USER:          return DomPropPage_StaticUser(nIDD);
    case IDD_DOMPROP_ST_GROUP:         return DomPropPage_StaticGroup(nIDD);
    case IDD_DOMPROP_ST_ROLE:          return DomPropPage_StaticRole(nIDD);
    case IDD_DOMPROP_ST_LOCATION:      return DomPropPage_StaticLocation(nIDD);
    case IDD_DOMPROP_ST_GWDB:          return DomPropPage_StaticGwDb(nIDD);

    // Ice items
    case IDD_DOMPROP_ICE_DBCONNECTION:        return DomPropPage_IceDbconnection(nIDD);
    case IDD_DOMPROP_ICE_DBUSER:              return DomPropPage_IceDbuser(nIDD);
    case IDD_DOMPROP_ICE_PROFILE:             return DomPropPage_IceProfile(nIDD);
    case IDD_DOMPROP_ICE_ROLE:                return DomPropPage_IceRole(nIDD);
    case IDD_DOMPROP_ICE_WEBUSER:             return DomPropPage_IceWebuser(nIDD);
    case IDD_DOMPROP_ICE_SERVER_LOCATION:     return DomPropPage_IceLocation(nIDD);
    case IDD_DOMPROP_ICE_SERVER_VARIABLE:     return DomPropPage_IceVariable(nIDD);
    case IDD_DOMPROP_ICE_BUNIT:               return DomPropPage_IceBunit(nIDD);
    case IDD_DOMPROP_ICE_SEC_ROLES:           return DomPropPage_IceSecRoles(nIDD);
    case IDD_DOMPROP_ICE_SEC_DBUSERS:         return DomPropPage_IceSecDbusers(nIDD);
    case IDD_DOMPROP_ICE_SEC_DBCONNS:         return DomPropPage_IceSecDbconns(nIDD);
    case IDD_DOMPROP_ICE_SEC_WEBUSERS:        return DomPropPage_IceSecWebusers(nIDD);
    case IDD_DOMPROP_ICE_SEC_PROFILES:        return DomPropPage_IceSecProfiles(nIDD);
    case IDD_DOMPROP_ICE_WEBUSER_ROLES:       return DomPropPage_IceSecWebuserRoles(nIDD);
    case IDD_DOMPROP_ICE_WEBUSER_CONNECTIONS: return DomPropPage_IceSecWebuserDbconns(nIDD);
    case IDD_DOMPROP_ICE_PROFILE_ROLES:       return DomPropPage_IceSecProfileRoles(nIDD);
    case IDD_DOMPROP_ICE_PROFILE_CONNECTIONS: return DomPropPage_IceSecProfileDbconns(nIDD);
    case IDD_DOMPROP_ICE_SERVER_APPLICATIONS: return DomPropPage_IceServerApplications(nIDD);
    case IDD_DOMPROP_ICE_SERVER_LOCATIONS:    return DomPropPage_IceServerLocations(nIDD);
    case IDD_DOMPROP_ICE_SERVER_VARIABLES:    return DomPropPage_IceServerVariables(nIDD);

    case IDD_DOMPROP_INSTLEVEL_AUDITUSERS:  return DomPropPage_InstallLevelAditAllUsers(nIDD);
    case IDD_DOMPROP_INSTLEVEL_ENABLEDLEVEL:return DomPropPage_InstallLevelEnabledLevel(nIDD);
    case IDD_DOMPROP_INSTLEVEL_AUDITLOG:    return DomPropPage_InstallLevelAuditLog(nIDD);

    case IDD_DOMPROP_ICE_FACETNPAGE:        return DomPropPage_IceBunitFacetAndPage(nIDD);
    case IDD_DOMPROP_ICE_BUNIT_ACCESSDEF:   return DomPropPage_IceBunitAccessDef(nIDD);
    case IDD_DOMPROP_ICE_BUNITS:            return DomPropPage_IceBunits(nIDD);
    case IDD_DOMPROP_ICE_SEC_ROLE:          return DomPropPage_IceBunitRole(nIDD);
    case IDD_DOMPROP_ICE_SEC_DBUSER:        return DomPropPage_IceBunitDbuser(nIDD);
    case IDD_DOMPROP_ICE_FACETS:            return DomPropPage_IceFacets(nIDD);
    case IDD_DOMPROP_ICE_PAGES:             return DomPropPage_IcePages(nIDD);
    case IDD_DOMPROP_ICE_LOCATIONS:         return DomPropPage_IceLocations(nIDD);

    case IDD_DOMPROP_ICE_BUNIT_ROLES:       return DomPropPage_IceBunitRoles(nIDD);
    case IDD_DOMPROP_ICE_BUNIT_USERS:       return DomPropPage_IceBunitUsers(nIDD);
    case IDD_DOMPROP_ICE_FACETNPAGE_ROLES:  return DomPropPage_IceFacetNPageRoles(nIDD);
    case IDD_DOMPROP_ICE_FACETNPAGE_USERS:  return DomPropPage_IceFacetNPageUsers(nIDD);

    case IDD_DOMPROP_ICE_HTML_IMAGE:        return DomPropPage_IceFacetGif(nIDD);
    case IDD_DOMPROP_ICE_HTML_SOURCE:       return DomPropPage_IcePageHtml(nIDD);

    default:
      ASSERT(FALSE);
      return NULL;
  }

  return NULL;
}

CWnd* CuDlgDomTabCtrl::GetPage (UINT nIDD)
{
    int index = Find (nIDD, m_arrayDlg, 0, NUMBEROFDOMPAGES -1);
    if (index == -1)  {
      ASSERT (FALSE);   // Forgot to add the page in the array!
      return NULL;
    }
    else
    {
        if (m_arrayDlg[index].pPage)
            return m_arrayDlg[index].pPage;
        else
        {
            m_arrayDlg[index].pPage = ConstructPage (nIDD);
            return m_arrayDlg[index].pPage;
        }
    }

    for (int i=0; i<NUMBEROFDOMPAGES; i++)
    {
        if (m_arrayDlg[i].nIDD == nIDD)
        {
            if (m_arrayDlg[i].pPage)
                return m_arrayDlg[i].pPage;
            m_arrayDlg[i].pPage = ConstructPage (nIDD);
            return m_arrayDlg[i].pPage;
        }
    }
    //
    // Call GetPage, you must define some dialog box...
    ASSERT (FALSE);
    return NULL;
}

CWnd* CuDlgDomTabCtrl::GetCreatedPage (UINT nIDD)
{
	int index = Find (nIDD, m_arrayDlg, 0, NUMBEROFDOMPAGES -1);
	if (index == -1)
	{
		ASSERT (FALSE);// Forgot to add the page in the array!
		return NULL;
	}
	else
	{
		if (m_arrayDlg[index].pPage)
			return m_arrayDlg[index].pPage;
		return NULL;
	}

	return NULL;
}

void CuDlgDomTabCtrl::DisplayPage (CuDomPageInformation* pPageInfo, BOOL bUpdate, int initialSel)
{
    CRect   r;
    TC_ITEM item;
    int     i, nPages;
    UINT*   nTabID;
    UINT*   nDlgID;
    CString strTab;
    CString strTitle;
    UINT    nIDD; 
    CWnd* pDlg;
    CView*   pView = (CView*)GetParent();
    ASSERT (pView);
    CChildFrame* pFrame = (CChildFrame*)pView->GetParent();
    ASSERT (pFrame);
    HWND hWndDomDoc = GetVdbaDocumentHandle(pFrame->m_hWnd);
    ASSERT (hWndDomDoc);
    LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hWndDomDoc, 0L);
    ASSERT (lpDomData);
    int hDomNode = lpDomData->psDomNode->nodeHandle;
    ASSERT (hDomNode != -1);

    if (m_bIsLoading)
    {
        m_bIsLoading = FALSE;
        // Masqued Emb April 9, 97 for bug fix
        // return;
    }
    if (!pPageInfo)
    {
        if (m_pCurrentPage)
            m_pCurrentPage->ShowWindow (SW_HIDE);
        m_cTab1.DeleteAllItems();
        if (m_pCurrentProperty)
        {
            m_pCurrentProperty->SetDomPageInfo (NULL);
            delete m_pCurrentProperty;
            m_pCurrentProperty = NULL;
            m_pCurrentPage     = NULL;
        }
        m_staticHeader.SetWindowText ("");
        UINT uFlags = SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER;
        m_staticHeader.ResetBitmap (NULL);
        m_staticHeader.SetWindowPos(NULL, 0, 0, 0, 0, uFlags);
        return;
    }
    if (!m_pCurrentProperty)
    {
        try
        {
            m_pCurrentProperty = new CuDomProperty (0, pPageInfo);
        }
        catch (CMemoryException* e)
        {
            VDBA_OutOfMemoryMessage();
            e->Delete();
            m_pCurrentPage = NULL;
            m_pCurrentProperty = NULL;
            return;
        }
        //
        // Set up the Title:
        pPageInfo->GetTitle (strTitle);
        m_staticHeader.SetWindowText (strTitle);

        UINT uFlags = SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER;
        if (!strTitle.IsEmpty())
        {
            switch (pPageInfo->GetImageType())
            {
            case 0:
                m_staticHeader.ResetBitmap (pPageInfo->GetIconImage());
                break;
            case 1:
                m_staticHeader.ResetBitmap (pPageInfo->GetBitmapImage(), pPageInfo->GetImageMask());
                break;
            default:
                m_staticHeader.ResetBitmap (NULL);
            }
        }
        else
            m_staticHeader.ResetBitmap (NULL);
        m_staticHeader.SetWindowPos(NULL, 0, 0, 0, 0, uFlags);

        nPages = pPageInfo->GetNumberOfPage();
        if (nPages < 1)
        {
            m_pCurrentPage = NULL;
            return;
        }
        //
        // Construct the Tab(s)
        nTabID = pPageInfo->GetTabID();
        nDlgID = pPageInfo->GetDlgID();
        memset (&item, 0, sizeof (item));
        item.mask       = TCIF_TEXT;
        item.cchTextMax = 32;
        m_cTab1.DeleteAllItems();
        for (i=0; i<nPages; i++)
        {
            strTab.LoadString (nTabID [i]);
            item.pszText = (LPTSTR)(LPCTSTR)strTab;
            m_cTab1.InsertItem (i, &item);
        }
        //
        // Display the initial selection, defaults to the first page.
        nIDD    = pPageInfo->GetDlgID (initialSel);
        try 
        {
            pDlg= GetPage (nIDD);
        }
        catch (CMemoryException* e)
        {
            VDBA_OutOfMemoryMessage ();
            e->Delete();
            return;
        }
        catch (CResourceException* e)
        {
            BfxMessageBox (VDBA_MfcResourceString(IDS_E_LOAD_RESOURCE));//"Load resource failed"
            e->Delete();
            return;
        }
        m_cTab1.SetCurSel (initialSel);
        lpDomData->detailSelBeforeRClick = initialSel;
        m_pCurrentProperty->SetCurSel(initialSel);    // Otherwise Save followed by Load will lead to crash since assuming first property page loaded
        m_cTab1.GetClientRect (r);
        m_cTab1.AdjustRect (FALSE, r);
        ASSERT (pDlg);    // forgotten dialogs trap here
        pDlg->MoveWindow (r);
        pDlg->ShowWindow(SW_SHOW);
        m_pCurrentPage = pDlg;
        if (bUpdate)
            m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)hDomNode, (LPARAM)pPageInfo->GetUpdateParam());
    }
    else
    {
        CuDomPageInformation* pCurrentPageInfo = m_pCurrentProperty->GetDomPageInfo();
        pPageInfo->GetTitle (strTitle);
        m_staticHeader.SetWindowText (strTitle);
        UINT uFlags = SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER;
        if (!strTitle.IsEmpty())
        {
            switch (pPageInfo->GetImageType())
            {
            case 0:
                m_staticHeader.ResetBitmap (pPageInfo->GetIconImage());
                break;
            case 1:
                m_staticHeader.ResetBitmap (pPageInfo->GetBitmapImage(), pPageInfo->GetImageMask());
                break;
            default:
                m_staticHeader.ResetBitmap (NULL);
            }
        }
        else
            m_staticHeader.ResetBitmap (NULL);
        m_staticHeader.SetWindowPos(NULL, 0, 0, 0, 0, uFlags);

        // New code as of Dec. 23, 1997: pCurrentPageInfo can have become NULL
        // example: current selection was system obj, and system checkbox has been unchecked
        BOOL bNoMoreCurrentClassName = FALSE;
        CString CurrentClassName;
        if (pCurrentPageInfo)
          CurrentClassName = pCurrentPageInfo->GetClassName();
        else
        {
          bNoMoreCurrentClassName = TRUE;
          CurrentClassName = "No More Current Class Name!!!";
        }
        CString ClassName        = pPageInfo->GetClassName();
        if ((CurrentClassName == ClassName) && (!bNoMoreCurrentClassName) )
        {
            m_pCurrentProperty->SetDomPageInfo (pPageInfo);
            //
            // wParam, and lParam will contain the information needed
            // by the dialog to refresh itself.
            if (m_pCurrentPage) 
                if (bUpdate)
                    m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)hDomNode, (LPARAM)pPageInfo->GetUpdateParam());
        }
        else
        {
            if (m_pCurrentPage) 
                m_pCurrentPage->ShowWindow (SW_HIDE);
            m_cTab1.DeleteAllItems();
            nPages = pPageInfo->GetNumberOfPage();
            m_pCurrentProperty->SetDomPageInfo (pPageInfo);
            m_pCurrentProperty->SetCurSel (0);
            lpDomData->detailSelBeforeRClick = 0;

            if (nPages < 1)
            {
                m_pCurrentPage = NULL;
                return;
            }
            UINT nIDD; 
            CWnd* pDlg;
            //
            // Construct the Tab(s)
            nTabID = pPageInfo->GetTabID();
            nDlgID = pPageInfo->GetDlgID();
            memset (&item, 0, sizeof (item));
            item.mask       = TCIF_TEXT;
            item.cchTextMax = 32;
            m_cTab1.DeleteAllItems();
            for (i=0; i<nPages; i++)
            {
                strTab.LoadString (nTabID [i]);
                item.pszText = (LPTSTR)(LPCTSTR)strTab;
                m_cTab1.InsertItem (i, &item);
            }
            //
            // Display the default (the first) page.
            nIDD    = pPageInfo->GetDlgID (0); 
            try 
            {
                pDlg= GetPage (nIDD);
            }
            catch (CMemoryException* e)
            {
                VDBA_OutOfMemoryMessage();
                e->Delete();
                return;
            }
            catch (CResourceException* e)
            {
                BfxMessageBox (VDBA_MfcResourceString(IDS_E_LOAD_RESOURCE));//"Load resource failed"
                e->Delete();
                return;
            }
            m_cTab1.SetCurSel (0);
            lpDomData->detailSelBeforeRClick = 0;
            m_cTab1.GetClientRect (r);
            m_cTab1.AdjustRect (FALSE, r);
            pDlg->MoveWindow (r);
            pDlg->ShowWindow(SW_SHOW);
            m_pCurrentPage = pDlg;
            if (bUpdate)
                m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)hDomNode, (LPARAM)pPageInfo->GetUpdateParam());
        }
    }
}



BEGIN_MESSAGE_MAP(CuDlgDomTabCtrl, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomTabCtrl)
    ON_WM_SIZE()
    ON_NOTIFY(TCN_SELCHANGE, IDC_MFC_TAB1, OnSelchangeTab1)
    ON_WM_DESTROY()
	ON_NOTIFY(TCN_SELCHANGING, IDC_MFC_TAB1, OnSelchangingTab1)
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_QUERY_OPEN_CURSOR, OnQueryOpenCursor)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CuDlgDomTabCtrl message handlers

LONG CuDlgDomTabCtrl::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
  //
  // lParam containts the hint about the change.
  // Its value can be FILTER_DOM_SYSTEMOBJECTS, FILTER_DOM_XYZ...
  if (m_pCurrentPage) {
    FilterCause cause = (FilterCause)(int)lParam;
    if (cause == FILTER_DOM_BKREFRESH || cause == FILTER_DOM_BKREFRESH_DETAIL ) {
      DomPageType pageType = (DomPageType)m_pCurrentPage->SendMessage(WM_USER_DOMPAGE_QUERYTYPE, 0, 0);
      if (pageType == DOMPAGE_ERROR) {
        ASSERT (FALSE);   // MISSING HANDLER FOR WM_USER_DOMPAGE_QUERYTYPE IN CURRENT PAGE!!!
        return 0L;
      }
      if (cause == FILTER_DOM_BKREFRESH)
        if (pageType != DOMPAGE_LIST)
          return 0L;
      if (cause == FILTER_DOM_BKREFRESH_DETAIL)
        if (pageType != DOMPAGE_DETAIL && pageType != DOMPAGE_BOTH)
          return 0L;
    }
    //
    // SQLAct Setting changes - Enable/Disable Row Grid:
    int nSetting = -1;
    if (cause == FILTER_SETTING_CHANGE)
    {
        switch (GetCurrentPageID())
        {
        case IDD_DOMPROP_TABLE_ROWS:
        case IDD_DOMPROP_INSTLEVEL_AUDITLOG:
            nSetting = FILTER_SETTING_CHANGE;
            break;
        }
    }

    CuDomPageInformation* pPageInfo = m_pCurrentProperty->GetDomPageInfo();
    ASSERT (pPageInfo);
    LPIPMUPDATEPARAMS pUps = pPageInfo->GetUpdateParam();
    pUps->nIpmHint = (nSetting == -1)? (int)lParam: nSetting;
    m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_UPDATEING, wParam, (LPARAM)pUps);
  }
  return 0L;
}

void CuDlgDomTabCtrl::PostNcDestroy() 
{
    delete this;    
    CDialog::PostNcDestroy();
}

BOOL CuDlgDomTabCtrl::OnInitDialog() 
{
    CDialog::OnInitDialog();
    VERIFY (m_cTab1.SubclassDlgItem (IDC_MFC_TAB1, this));
    VERIFY (m_staticHeader.SubclassDlgItem (IDC_MFC_STATIC, this));
    m_staticHeader.SetImage (-1);
    //
    // When the document is Loaded ...
    CWnd* pParent1 = GetParent();    // The view: CImView2
    ASSERT (pParent1);
    CMainMfcDoc* pDoc = (CMainMfcDoc*)((CView*)pParent1)->GetDocument();   
    ASSERT (pDoc);
    pDoc->SetTabDialog (this);
    if (!pDoc->m_pCurrentProperty)
        return TRUE;    // not loaded, or no current property (empty scratchpad, treeitem without property, ...)
    try
    {
        CSplitterWnd* pParent2 = (CSplitterWnd*)pParent1->GetParent();    // The Splitter Window
        ASSERT (pParent2);
        CMainMfcView*    pMainMfcView= (CMainMfcView*)pParent2->GetPane (0,0);
        ASSERT (pMainMfcView);

        // access LPDOMDATA
        HWND hwndDoc = GetVdbaDocumentHandle(pMainMfcView->m_hWnd);
        ASSERT (hwndDoc);
        LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwndDoc, 0);
        ASSERT (lpDomData);

        // pick info on current selection in dom tree
        DWORD dwCurSel = GetCurSel(lpDomData);
        ASSERT (dwCurSel != 0xFFFFFFFF);
        if (dwCurSel == 0xFFFFFFFF)
          return TRUE;
        BOOL  bCurSelStatic = IsCurSelObjStatic(lpDomData);
        int CurItemObjType = GetItemObjType(lpDomData, dwCurSel);
        LPTREERECORD lpRecord = (LPTREERECORD)::SendMessage(lpDomData->hwndTreeLb,
                                                           LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
        ASSERT (lpRecord);
        // useless: BOOL bHasProperties = HasProperties4display(CurItemObjType);

        // update updateparams structure
        CuDomPageInformation* pPageInfo = pDoc->m_pCurrentProperty->GetDomPageInfo();
        ASSERT (pPageInfo);
        LPIPMUPDATEPARAMS pUps = pPageInfo->GetUpdateParam();
        ASSERT (pUps->nType   == CurItemObjType); // Must be the same
        pUps->pStruct = lpRecord;                 // Must be updated since reallocated
        pUps->pSFilter = pDoc->GetLpFilter();

        // Flag loading in progress BEFORE call to LoadPage() (see test in DomPropPage_GenericWithScrollbars() )
        m_bIsLoading = TRUE;
        //
        //  Get the Image (Icon) of the current selected Tree Item:
        HICON hIcon = CChildFrame::DOMTreeGetCurrentIcon(lpDomData->hwndTreeLb);
        pPageInfo->SetImage  (hIcon);
        // load the page
        LoadPage (pDoc->m_pCurrentProperty);

        // update lpRecord according to the load result,
        // so that we won't have free chunks at program termination
        lpRecord->bPageInfoCreated = TRUE;
        lpRecord->m_pPageInfo = (DWORD)pPageInfo;
    }
    catch (CMemoryException* e)
    {
        VDBA_OutOfMemoryMessage ();
        e->Delete();
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomTabCtrl::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cTab1.m_hWnd) || !IsWindow (m_staticHeader.m_hWnd))
		return;
	CRect r, r2;
	GetClientRect (r);
	if (theApp.GetShowRightPaneHeader())
	{
		m_staticHeader.GetWindowRect (r2);
		ScreenToClient (r2);
		r2.top   = r.top;
		r2.left  = r.left;
		r2.right = r.right;
		m_staticHeader.MoveWindow (r2);
	}
	m_cTab1.GetWindowRect (r2);
	ScreenToClient (r2);
	if (!theApp.GetShowRightPaneHeader())
	{
		r2.top = r.top;
		m_staticHeader.ShowWindow (SW_HIDE);
	}

	r2.left  = r.left;
	r2.right = r.right;
	r2.bottom= r.bottom;
	if (r2.Width() > 0 && r2.Height() > 0)
		m_cTab1.MoveWindow (r2);
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	if (m_pCurrentPage)
		m_pCurrentPage->MoveWindow (r);
}

// Gère le changement d'onglet actif!
void CuDlgDomTabCtrl::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
    int nSel;
    CRect r;
    CWnd* pNewPage;
    CuDomPageInformation* pPageInfo;
    CWnd* pParent1 = GetParent();    // The view: CImView2
    ASSERT (pParent1);
    CSplitterWnd* pParent2 = (CSplitterWnd*)pParent1->GetParent();  // The Splitter
    ASSERT (pParent2);
    CChildFrame* pFrame = (CChildFrame*)pParent2->GetParent();      // The Frame Window
    ASSERT (pFrame);
    HWND hWndDomDoc = GetVdbaDocumentHandle(pFrame->m_hWnd);
    ASSERT (hWndDomDoc);
    LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hWndDomDoc, 0L);
    ASSERT (lpDomData);
    int hDomNode = lpDomData->psDomNode->nodeHandle;
    ASSERT (hDomNode != -1);

    if (!pFrame->IsAllViewCreated())
        return;
    if (!m_pCurrentProperty)
        return;
    AfxGetApp()->DoWaitCursor (1);
    m_cTab1.GetClientRect (r);
    m_cTab1.AdjustRect (FALSE, r);
    nSel = m_cTab1.GetCurSel();
    m_pCurrentProperty->SetCurSel(nSel);
    lpDomData->detailSelBeforeRClick = nSel;
    pPageInfo = m_pCurrentProperty->GetDomPageInfo();
    try
    {
        pNewPage  = GetPage (pPageInfo->GetDlgID (nSel));
    }
    catch (CMemoryException* e)
    {
        AfxGetApp()->DoWaitCursor (-1);
        VDBA_OutOfMemoryMessage();
        e->Delete();
        return;
    }
    catch (CResourceException* e)
    {
        AfxGetApp()->DoWaitCursor (-1);
        BfxMessageBox (VDBA_MfcResourceString(IDS_E_LOAD_RESOURCE));//"Fail to load resource"
        e->Delete();
        return;
    }
    if (m_pCurrentPage)
        m_pCurrentPage->ShowWindow (SW_HIDE);
    m_pCurrentPage = pNewPage;
    m_pCurrentPage->MoveWindow (r);
    m_pCurrentPage->ShowWindow(SW_SHOW);

    // Reset Hint HERE!
    LPIPMUPDATEPARAMS pUps = pPageInfo->GetUpdateParam();
    ASSERT (pUps);
    pUps->nIpmHint = 0;
    //
    // Special management for the Rows:
    // Inform the page that the WM_USER_IPMPAGE_UPDATEING message results from the
    // Selection change of the tab control in the right pane (set the pUps->nIpmHint to -1):
    int nOldHint = pUps->nIpmHint;
    m_pCurrentPage->SendMessage (WM_QUERY_UPDATING, (WPARAM)0, (LPARAM)pUps);
    m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)hDomNode, (LPARAM)pUps);
    pUps->nIpmHint = nOldHint;
    *pResult = 0;
    AfxGetApp()->DoWaitCursor (-1);
}



void CuDlgDomTabCtrl::OnDestroy() 
{
    if (m_pCurrentProperty)
    {
        m_pCurrentProperty->SetDomPageInfo (NULL);
        delete m_pCurrentProperty;
    }
    CDialog::OnDestroy();
}


void CuDlgDomTabCtrl::LoadPage (CuDomProperty* pCurrentProperty)
{
    CRect   r;
    TC_ITEM item;
    int     i, nPages;
    UINT*   nTabID;
    UINT*   nDlgID;
    CString strTab;
    CString strTitle;
    UINT    nIDD; 
    CWnd*   pDlg;
    CuDomPageInformation* pPageInfo = NULL;

    CView*   pView = (CView*)GetParent();
    ASSERT (pView);
    CChildFrame* pFrame = (CChildFrame*)pView->GetParent();
    ASSERT (pFrame);
    HWND hWndDomDoc = GetVdbaDocumentHandle(pFrame->m_hWnd);
    ASSERT (hWndDomDoc);
    LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hWndDomDoc, 0L);
    ASSERT (lpDomData);

    if (m_pCurrentProperty)
    {
        m_pCurrentProperty->SetDomPageInfo (NULL);
        delete m_pCurrentProperty;
    }
    try
    {
        pPageInfo = pCurrentProperty->GetDomPageInfo();
        m_pCurrentProperty = new CuDomProperty (pCurrentProperty->GetCurSel(), pPageInfo);
    }
    catch (CMemoryException* e)
    {
        VDBA_OutOfMemoryMessage();
        e->Delete();
        m_pCurrentPage     = NULL;
        m_pCurrentProperty = NULL;
        return;
    }
    
    //
    // Set up the Title:
    pPageInfo->GetTitle (strTitle);
    m_staticHeader.SetWindowText (strTitle);
    UINT uFlags = SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER;
    if (!strTitle.IsEmpty())
    {
        switch (pPageInfo->GetImageType())
        {
        case 0:
            m_staticHeader.ResetBitmap (pPageInfo->GetIconImage());
            break;
        case 1:
            m_staticHeader.ResetBitmap (pPageInfo->GetBitmapImage(), pPageInfo->GetImageMask());
            break;
        default:
            m_staticHeader.ResetBitmap (NULL);
        }
    }
    else
        m_staticHeader.ResetBitmap (NULL);
    m_staticHeader.SetWindowPos(NULL, 0, 0, 0, 0, uFlags);

    nPages = pPageInfo->GetNumberOfPage();
    if (nPages < 1)
    {
        m_pCurrentPage = NULL;
        return;
    }
    //
    // Construct the Tab(s)
    nTabID = pPageInfo->GetTabID();
    nDlgID = pPageInfo->GetDlgID();
    memset (&item, 0, sizeof (item));
    item.mask       = TCIF_TEXT;
    item.cchTextMax = 32;
    m_cTab1.DeleteAllItems();
    for (i=0; i<nPages; i++)
    {
        strTab.LoadString (nTabID [i]);
        item.pszText = (LPTSTR)(LPCTSTR)strTab;
        m_cTab1.InsertItem (i, &item);
    }
    //
    // Display the selected page.
    nIDD = pPageInfo->GetDlgID (pCurrentProperty->GetCurSel()); 
    try 
    {
        pDlg = GetPage (nIDD);
    }
    catch (CMemoryException* e)
    {
        VDBA_OutOfMemoryMessage();
        e->Delete();
        return;
    }
    catch (CResourceException* e)
    {
        BfxMessageBox (VDBA_MfcResourceString(IDS_E_LOAD_RESOURCE));//"Load resource failed"
        e->Delete();
        return;
    }
    m_cTab1.SetCurSel (pCurrentProperty->GetCurSel());
    lpDomData->detailSelBeforeRClick = pCurrentProperty->GetCurSel();
    m_cTab1.GetClientRect (r);
    m_cTab1.AdjustRect (FALSE, r);
    pDlg->MoveWindow (r);
    pDlg->ShowWindow(SW_SHOW);
    m_pCurrentPage = pDlg;
    //
    // Display the content (data) of this page.
    CuIpmPropertyData* pData = pCurrentProperty->GetPropertyData();
    pData->NotifyLoad (m_pCurrentPage);
}

CuIpmPropertyData* CuDlgDomTabCtrl::GetDialogData()
{
    if (m_pCurrentPage)
        return (CuIpmPropertyData*)(LRESULT)m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_GETDATA, 0, GETDIALOGDATA);
    return NULL;
}

CuIpmPropertyData* CuDlgDomTabCtrl::GetDialogDataWithDup()
{
    if (m_pCurrentPage)
        return (CuIpmPropertyData*)(LRESULT)m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_GETDATA, 0, GETDIALOGDATAWITHDUP);
    return NULL;
}

UINT CuDlgDomTabCtrl::GetCurrentPageID()
{
    if (!m_pCurrentProperty)
        return 0;
    CuDomPageInformation* pPageInfo = m_pCurrentProperty->GetDomPageInfo();
    if (!pPageInfo)
        return 0;
    if (pPageInfo->GetNumberOfPage() == 0)
        return 0;
    UINT* pArrayID = pPageInfo->GetDlgID();
    if (!pArrayID)
        return 0;
    int index = m_pCurrentProperty->GetCurSel();
    if (index >= 0 && index < pPageInfo->GetNumberOfPage())
        return pArrayID [index];
    return 0;
}

//
// Special Help ID management:
// Need ID to be distinct from one pane to anoter.
UINT CuDlgDomTabCtrl::GetCurrentHelpID()
{
    UINT nID = GetCurrentPageID();
    if (nID > 0 && nID == IDD_DOMPROP_TABLE_ROWS && m_pCurrentProperty)
    {
        CuDomPageInformation* pPageInfo = m_pCurrentProperty->GetDomPageInfo();
        if (!pPageInfo)
            return 0;
        if (pPageInfo->GetClassName() == _T("CuDomView"))
            return HELPID_DOM_VIEW_ROWS;
        else
        if (pPageInfo->GetClassName() == _T("CuDomStarLinkTable"))
            return HELPID_DOM_TABLE_STAR_ROWS;
        else
        if (pPageInfo->GetClassName() == _T("CuDomStarLinkView"))
            return HELPID_DOM_VIEW_STAR_ROWS;
        else
            return nID;
    }
    else
    if (nID > 0 && nID == IDD_DOMPROP_LOCATION && m_pCurrentProperty)
    {
        CuDomPageInformation* pPageInfo = m_pCurrentProperty->GetDomPageInfo();
        if (!pPageInfo)
            return 0;
        if (pPageInfo->GetClassName() == _T("CuDomLocationLocal"))
            return HELPID_DOM_LOCATION_LOCAL;
        else
        if (pPageInfo->GetClassName() == _T("CuDomLocationRemote"))
            return HELPID_DOM_LOCATION_REMOTE;
        else
            return nID;
    }
    else
    if (nID == IDD_DOMPROP_DB_GRANTEES && m_pCurrentProperty)
    {
        CuDomPageInformation* pPageInfo = m_pCurrentProperty->GetDomPageInfo();
        if (!pPageInfo)
            return nID;
        if (pPageInfo->GetClassName() == _T("CuDomStaticInstallationGrantees"))
            return HELPID_DOM_INSTALL_GRANTEES;
        else
            return nID;
    }
    else
    if (nID == IDD_DOMPROP_DB_ALARMS && m_pCurrentProperty)
    {
        CuDomPageInformation* pPageInfo = m_pCurrentProperty->GetDomPageInfo();
        if (!pPageInfo)
            return nID;
        if (pPageInfo->GetClassName() == _T("CuDomStaticInstallationAlarms"))
            return HELPID_DOM_INSTALL_ALARMS;
        else
            return nID;
    }
    else
    if (nID == IDD_DOMPROP_TABLE_PAGES && m_pCurrentProperty)
    {
        CuDomPageInformation* pPageInfo = m_pCurrentProperty->GetDomPageInfo();
        if (!pPageInfo)
            return nID;
        if (pPageInfo->GetClassName() == _T("CuDomIndex"))
            return IDD_DOMPROP_INDEX_PAGES;
        else
            return nID;
    }
    else
    if (nID == IDD_DOMPROP_TABLE_STATISTIC && m_pCurrentProperty)
    {
        CuDomPageInformation* pPageInfo = m_pCurrentProperty->GetDomPageInfo();
        if (!pPageInfo)
            return nID;
        if (pPageInfo->GetClassName() == _T("CuDomIndex"))
            return HELPID_STATISIC_INDEX;
        else
            return nID;
    }


    return nID;
}

///////////////////////////////////////////////////////////////////////
// Utility code-saver method for scrollable pages
//
CWnd* CuDlgDomTabCtrl::DomPropPage_GenericWithScrollbars(UINT nIDD)
{
  CRect r;
  m_cTab1.GetClientRect (r);
  m_cTab1.AdjustRect (FALSE, r);

  CuDlgViewFrame* pDlgFrame = new CuDlgViewFrame (nIDD);
  BOOL bCreated = pDlgFrame->Create (
      NULL,
      NULL,
      WS_CHILD,
      r,
      &m_cTab1
      );
  if (!bCreated)
      AfxThrowResourceException();

  // Call InitialUpdateFrame() ONLY IF document is not loaded,
  // (if loaded, pTemplate->OpenDocumentFile() will call it)
  if (m_bIsLoading)
    pDlgFrame->InitialUpdateFrame (NULL, FALSE);  // OnInitialUpdate() will not be called
  else
    pDlgFrame->InitialUpdateFrame (NULL, TRUE);   // Second parameter to TRUE,
                                                  // so that OnInitialUpdate() is called in the FormView
  return (CWnd*)pDlgFrame;
}

///////////////////////////////////////////////////////////////////////
// methods for construction of the Individual Pages
//
// CAUTION!!!: DIFFERENT CODE FOR CFormView OR FOR CModelessDlg!!!

CWnd* CuDlgDomTabCtrl::DomPropPage_Location(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_User(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_Role(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_Profile(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_Index(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_Integrity(UINT nIDD)
{
  CuDlgDomPropIntegrity* pDld = new CuDlgDomPropIntegrity(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_Rule(UINT nIDD)
{
  CuDlgDomPropRule* pDld = new CuDlgDomPropRule(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_Procedure(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_Sequence(UINT nIDD)
{
  ASSERT (nIDD == IDD_DOMPROP_SEQ);
  CuDlgDomPropSeq* pDld = new CuDlgDomPropSeq(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_Group(UINT nIDD)
{
  ASSERT (nIDD == IDD_DOMPROP_GROUP);
  CuDlgDomPropGroup* pDld = new CuDlgDomPropGroup(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DbAlarms(UINT nIDD)
{
  CuDlgDomPropDbAlarms* pDld = new CuDlgDomPropDbAlarms(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_TableAlarms(UINT nIDD)
{
  CuDlgDomPropTableAlarms* pDld = new CuDlgDomPropTableAlarms(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_TableGrantees(UINT nIDD)
{
  CuDlgDomPropTableGrantees* pDld = new CuDlgDomPropTableGrantees(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DbGrantees(UINT nIDD)
{
  CuDlgDomPropDbGrantees* pDld = new CuDlgDomPropDbGrantees(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_Db(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_Table(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_ViewGrantees(UINT nIDD)
{
  CuDlgDomPropViewGrantees* pDld = new CuDlgDomPropViewGrantees(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_View(UINT nIDD)
{
  CuDlgDomPropView* pDld = new CuDlgDomPropView(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_Cdds(UINT nIDD)
{
  CuDlgDomPropCdds* pDld = new CuDlgDomPropCdds(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_CddsTables(UINT nIDD)
{
  CuDlgDomPropCddsTables* pDld = new CuDlgDomPropCddsTables(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_LocationSpace(UINT nIDD)
{
  CuDlgDomPropLocationSpace* pDld = new CuDlgDomPropLocationSpace(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_LocationDbs(UINT nIDD)
{
  CuDlgDomPropLocationDbs* pDld = new CuDlgDomPropLocationDbs(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DbEventGrantees(UINT nIDD)
{
  CuDlgDomPropDbEventGrantees* pDld = new CuDlgDomPropDbEventGrantees(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_ProcGrantees(UINT nIDD)
{
  CuDlgDomPropProcGrantees* pDld = new CuDlgDomPropProcGrantees(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_SeqGrantees(UINT nIDD)
{
  CuDlgDomPropSeqGrantees* pDld = new CuDlgDomPropSeqGrantees(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_TableColumns(UINT nIDD)
{
  CuDlgDomPropTableColumns* pDld = new CuDlgDomPropTableColumns(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_ReplicCdds(UINT nIDD)
{
  CuDlgDomPropReplicCdds* pDld = new CuDlgDomPropReplicCdds(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_ReplicConn(UINT nIDD)
{
  CuDlgDomPropReplicConn* pDld = new CuDlgDomPropReplicConn(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_ReplicMailU(UINT nIDD)
{
  CuDlgDomPropReplicMailU* pDld = new CuDlgDomPropReplicMailU(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_ReplicRegTbl(UINT nIDD)
{
  CuDlgDomPropReplicRegTbl* pDld = new CuDlgDomPropReplicRegTbl(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DbTbls(UINT nIDD)
{
  CuDlgDomPropDbTbl* pDld = new CuDlgDomPropDbTbl(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DbViews(UINT nIDD)
{
  CuDlgDomPropDbView* pDld = new CuDlgDomPropDbView(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DbProcs(UINT nIDD)
{
  CuDlgDomPropDbProc* pDld = new CuDlgDomPropDbProc(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DbSeqs(UINT nIDD)
{
  CuDlgDomPropDbSeq* pDld = new CuDlgDomPropDbSeq(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DbSchemas(UINT nIDD)
{
  CuDlgDomPropDbSchema* pDld = new CuDlgDomPropDbSchema(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DbSynonyms(UINT nIDD)
{
  CuDlgDomPropDbSyn* pDld = new CuDlgDomPropDbSyn(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DbEvents(UINT nIDD)
{
  CuDlgDomPropDbEvt* pDld = new CuDlgDomPropDbEvt(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_TableInteg(UINT nIDD)
{
  CuDlgDomPropTblInteg* pDld = new CuDlgDomPropTblInteg(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_TableRule(UINT nIDD)
{
  CuDlgDomPropTblRule* pDld = new CuDlgDomPropTblRule(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_TableIdx(UINT nIDD)
{
  CuDlgDomPropTblIdx* pDld = new CuDlgDomPropTblIdx(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_TableLoc(UINT nIDD)
{
  CuDlgDomPropTblLoc* pDld = new CuDlgDomPropTblLoc(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_TableRows(UINT nIDD)
{
  CuDlgDomPropTableRows* pDld = new CuDlgDomPropTableRows(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_TableStatistic(UINT nIDD)
{
  CuDlgDomPropTableStatistic* pDld = new CuDlgDomPropTableStatistic(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_UserXAlarms(UINT nIDD)
{
  CuDlgDomPropUserXAlarms* pDld = new CuDlgDomPropUserXAlarms(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_UserXGrantedDb(UINT nIDD)
{
  CuDlgDomPropUserXGrantedDb* pDld = new CuDlgDomPropUserXGrantedDb(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_UserXGrantedTbl(UINT nIDD)
{
  CuDlgDomPropUserXGrantedTbl* pDld = new CuDlgDomPropUserXGrantedTbl(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_UserXGrantedView(UINT nIDD)
{
  CuDlgDomPropUserXGrantedView* pDld = new CuDlgDomPropUserXGrantedView(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_UserXGrantedEvent(UINT nIDD)
{
  CuDlgDomPropUserXGrantedEvent* pDld = new CuDlgDomPropUserXGrantedEvent(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_UserXGrantedProc(UINT nIDD)
{
  CuDlgDomPropUserXGrantedProc* pDld = new CuDlgDomPropUserXGrantedProc(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_UserXGrantedRole(UINT nIDD)
{
  CuDlgDomPropUserXGrantedRole* pDld = new CuDlgDomPropUserXGrantedRole(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_UserXGrantedSeq(UINT nIDD)
{
  CuDlgDomPropUserXGrantedSeq* pDld = new CuDlgDomPropUserXGrantedSeq(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_GroupXGrantedDb(UINT nIDD)
{
  CuDlgDomPropGroupXGrantedDb* pDld = new CuDlgDomPropGroupXGrantedDb(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_GroupXGrantedTbl(UINT nIDD)
{
  CuDlgDomPropGroupXGrantedTbl* pDld = new CuDlgDomPropGroupXGrantedTbl(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_GroupXGrantedView(UINT nIDD)
{
  CuDlgDomPropGroupXGrantedView* pDld = new CuDlgDomPropGroupXGrantedView(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_GroupXGrantedEvent(UINT nIDD)
{
  CuDlgDomPropGroupXGrantedEvent* pDld = new CuDlgDomPropGroupXGrantedEvent(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_GroupXGrantedProc(UINT nIDD)
{
  CuDlgDomPropGroupXGrantedProc* pDld = new CuDlgDomPropGroupXGrantedProc(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_GroupXGrantedSeq(UINT nIDD)
{
  CuDlgDomPropGroupXGrantedSeq* pDld = new CuDlgDomPropGroupXGrantedSeq(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_RoleXGrantedDb(UINT nIDD)
{
  CuDlgDomPropRoleXGrantedDb* pDld = new CuDlgDomPropRoleXGrantedDb(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_RoleXGrantedTbl(UINT nIDD)
{
  CuDlgDomPropRoleXGrantedTbl* pDld = new CuDlgDomPropRoleXGrantedTbl(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_RoleXGrantedView(UINT nIDD)
{
  CuDlgDomPropRoleXGrantedView* pDld = new CuDlgDomPropRoleXGrantedView(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_RoleXGrantedEvent(UINT nIDD)
{
  CuDlgDomPropRoleXGrantedEvent* pDld = new CuDlgDomPropRoleXGrantedEvent(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_RoleXGrantedProc(UINT nIDD)
{
  CuDlgDomPropRoleXGrantedProc* pDld = new CuDlgDomPropRoleXGrantedProc(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_RoleXGrantedSeq(UINT nIDD)
{
  CuDlgDomPropRoleXGrantedSeq* pDld = new CuDlgDomPropRoleXGrantedSeq(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_Locations(UINT nIDD)
{
  CuDlgDomPropLocations* pDld = new CuDlgDomPropLocations(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_Connection(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DDb(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DDbTbls(UINT nIDD)
{
  CuDlgDomPropDDbTbl* pDld = new CuDlgDomPropDDbTbl(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DDbViews(UINT nIDD)
{
  CuDlgDomPropDDbView* pDld = new CuDlgDomPropDDbView(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_DDbProcs(UINT nIDD)
{
  CuDlgDomPropDDbProc* pDld = new CuDlgDomPropDDbProc(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StarProcLink(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StarViewLink(UINT nIDD)
{
  CuDlgDomPropViewLink* pDld = new CuDlgDomPropViewLink(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StarViewNative(UINT nIDD)
{
  CuDlgDomPropViewNative* pDld = new CuDlgDomPropViewNative(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StarTableLinkIndexes(UINT nIDD)
{
  CuDlgDomPropTableLinkIdx* pDld = new CuDlgDomPropTableLinkIdx(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_SchemaTbls(UINT nIDD)
{
  CuDlgDomPropSchemaTbl* pDld = new CuDlgDomPropSchemaTbl(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_SchemaViews(UINT nIDD)
{
  CuDlgDomPropSchemaView* pDld = new CuDlgDomPropSchemaView(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_SchemaProcs(UINT nIDD)
{
  CuDlgDomPropSchemaProc* pDld = new CuDlgDomPropSchemaProc(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_TablePages(UINT nIDD)
{
  CuDlgDomPropTblPages* pDld = new CuDlgDomPropTblPages(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IndexPages(UINT nIDD)
{
  ASSERT (FALSE);
  return NULL;
  /*CuDlgDomPropIndexPages* pDld = new CuDlgDomPropIndexPages(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
  */
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StarTableLink(UINT nIDD)
{
  CuDlgDomPropTableLink* pDld = new CuDlgDomPropTableLink(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StarDbComponent(UINT nIDD)
{
    CuDlgDomPropStarDbComponent* pDld = new CuDlgDomPropStarDbComponent(&m_cTab1);
    if (!pDld->Create (nIDD, &m_cTab1))
        AfxThrowResourceException();
    return (CWnd*)pDld;
}


CWnd* CuDlgDomTabCtrl::DomPropPage_StarIndexLinkSource(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StarProcLinkSource(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StarViewLinkSource(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StarViewNativeSource(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_TableRowsNA(UINT nIDD)
{
  CuDlgDomPropTableRowsNA* pDld = new CuDlgDomPropTableRowsNA(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StaticDb(UINT nIDD)
{
  CuDlgDomPropStaticDb* pDld = new CuDlgDomPropStaticDb(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StaticProfile(UINT nIDD)
{
  CuDlgDomPropStaticProfile* pDld = new CuDlgDomPropStaticProfile(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StaticUser(UINT nIDD)
{
  CuDlgDomPropStaticUser* pDld = new CuDlgDomPropStaticUser(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StaticGroup(UINT nIDD)
{
  CuDlgDomPropStaticGroup* pDld = new CuDlgDomPropStaticGroup(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StaticRole(UINT nIDD)
{
  CuDlgDomPropStaticRole* pDld = new CuDlgDomPropStaticRole(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StaticLocation(UINT nIDD)
{
  CuDlgDomPropStaticLocation* pDld = new CuDlgDomPropStaticLocation(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_StaticGwDb(UINT nIDD)
{
  CuDlgDomPropStaticGwDb* pDld = new CuDlgDomPropStaticGwDb(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceDbconnection(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceDbuser(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceProfile(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceRole(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceWebuser(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceLocation(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceVariable(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceBunit(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceSecRoles(UINT nIDD)
{
  CuDlgDomPropIceSecRoles* pDld = new CuDlgDomPropIceSecRoles(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceSecDbusers(UINT nIDD)
{
  CuDlgDomPropIceSecDbusers* pDld = new CuDlgDomPropIceSecDbusers(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceSecDbconns(UINT nIDD)
{
  CuDlgDomPropIceSecDbconns* pDld = new CuDlgDomPropIceSecDbconns(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceSecWebusers(UINT nIDD)
{
  CuDlgDomPropIceSecWebusers* pDld = new CuDlgDomPropIceSecWebusers(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceSecProfiles(UINT nIDD)
{
  CuDlgDomPropIceSecProfiles* pDld = new CuDlgDomPropIceSecProfiles(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceSecWebuserRoles(UINT nIDD)
{
  CuDlgDomPropIceWebuserRoles* pDld = new CuDlgDomPropIceWebuserRoles(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceSecWebuserDbconns(UINT nIDD)
{
  CuDlgDomPropIceWebuserDbconns* pDld = new CuDlgDomPropIceWebuserDbconns(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceSecProfileRoles(UINT nIDD)
{
  CuDlgDomPropIceProfileRoles* pDld = new CuDlgDomPropIceProfileRoles(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceSecProfileDbconns(UINT nIDD)
{
  CuDlgDomPropIceProfileDbconns* pDld = new CuDlgDomPropIceProfileDbconns(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceServerApplications(UINT nIDD)
{
  CuDlgDomPropIceServerApplications* pDld = new CuDlgDomPropIceServerApplications(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceServerLocations(UINT nIDD)
{
  CuDlgDomPropIceServerLocations* pDld = new CuDlgDomPropIceServerLocations(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceServerVariables(UINT nIDD)
{
  CuDlgDomPropIceServerVariables* pDld = new CuDlgDomPropIceServerVariables(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}


CWnd* CuDlgDomTabCtrl::DomPropPage_InstallLevelAditAllUsers (UINT nIDD)
{
  CuDlgDomPropInstallationLevelAuditAllUsers* pDld = new CuDlgDomPropInstallationLevelAuditAllUsers(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_InstallLevelEnabledLevel(UINT nIDD)
{
  CuDlgDomPropInstallationLevelEnabledLevel* pDld = new CuDlgDomPropInstallationLevelEnabledLevel(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_InstallLevelAuditLog (UINT nIDD)
{
  CuDlgDomPropInstallationLevelAuditLog* pDld = new CuDlgDomPropInstallationLevelAuditLog(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceBunitFacetAndPage(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}


CWnd* CuDlgDomTabCtrl::DomPropPage_IceBunitAccessDef(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceBunits(UINT nIDD)
{
  CuDlgDomPropIceBunits* pDld = new CuDlgDomPropIceBunits(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceBunitRole(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceBunitDbuser(UINT nIDD)
{
  return DomPropPage_GenericWithScrollbars(nIDD);
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceFacets(UINT nIDD)
{
  CuDlgDomPropIceFacets* pDld = new CuDlgDomPropIceFacets(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IcePages(UINT nIDD)
{
  CuDlgDomPropIcePages* pDld = new CuDlgDomPropIcePages(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceLocations(UINT nIDD)
{
  CuDlgDomPropIceLocations* pDld = new CuDlgDomPropIceLocations(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceBunitRoles(UINT nIDD)
{
  CuDlgDomPropIceBunitRoles* pDld = new CuDlgDomPropIceBunitRoles(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceBunitUsers(UINT nIDD)
{
  CuDlgDomPropIceBunitUsers* pDld = new CuDlgDomPropIceBunitUsers(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceFacetNPageRoles(UINT nIDD)
{
  CuDlgDomPropIceFacetNPageRoles* pDld = new CuDlgDomPropIceFacetNPageRoles(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceFacetNPageUsers(UINT nIDD)
{
  CuDlgDomPropIceFacetNPageUsers* pDld = new CuDlgDomPropIceFacetNPageUsers(&m_cTab1);
  if (!pDld->Create (nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IcePageHtml(UINT nIDD)
{
  CuDlgDomPropIceHtml* pDld = new CuDlgDomPropIceHtml(0, &m_cTab1);
  if (!pDld->Create (pDld->m_nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

CWnd* CuDlgDomTabCtrl::DomPropPage_IceFacetGif(UINT nIDD)
{
  CuDlgDomPropIceHtml* pDld = new CuDlgDomPropIceHtml(1, &m_cTab1);
  if (!pDld->Create (pDld->m_nIDD, &m_cTab1))
    AfxThrowResourceException();
  return (CWnd*)pDld;
}

void CuDlgDomTabCtrl::NotifyPageSelChanging(BOOL bUpdate)
{
    //
    // Convention: lParam = 0: Left pane selection changes.
    //             lParam = 1: Right pane selection changes.
    //             lParam = 2: Restore right pane visibility after exiting contextmenu by escape
    // This convention can be changed if lParam is needed for other purposes later.
    LPARAM lParam = (bUpdate? 0: 2);
    if (m_pCurrentProperty)
    {
        //
        // For the current active page:
        if (m_pCurrentPage)
            m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_LEAVINGPAGE, (WPARAM)LEAVINGPAGE_CHANGELEFTSEL, lParam);
    }
    //
    // Special handling:
    // The row page, even if it is not active, tell it to destroy the cursor:
    int index = Find (IDD_DOMPROP_TABLE_ROWS, m_arrayDlg, 0, NUMBEROFDOMPAGES -1);
    if (index != -1 && m_arrayDlg[index].pPage)
    {
        m_arrayDlg[index].pPage->SendMessage (WM_USER_IPMPAGE_LEAVINGPAGE, (WPARAM)LEAVINGPAGE_CHANGELEFTSEL, lParam);
    }
}


void CuDlgDomTabCtrl::OnSelchangingTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
    //
    // Convention: lParam = 0: Left pane selection changes.
    //             lParam = 1: Right pane selection changes.
    //             lParam = 2: Restore right pane visibility after exiting contextmenu by escape
    // This convention can be changed if lParam is needed for other purposes later.
    if (m_pCurrentProperty)
        if (m_pCurrentPage)
            m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_LEAVINGPAGE, (WPARAM)LEAVINGPAGE_CHANGELEFTSEL, 1L);
    *pResult = 0;
}

void CuDlgDomTabCtrl::UpdateBitmapTitle(HICON hIcon)
{
	if (!m_pCurrentProperty)
		return;
	CuDomPageInformation* pPageInfo = m_pCurrentProperty->GetDomPageInfo();
	if (!pPageInfo)
		return;
	pPageInfo->SetImage (hIcon);
	UINT uFlags = SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER;
	CString strTitle;
	pPageInfo->GetTitle(strTitle);
	if (!strTitle.IsEmpty())
	{
		switch (pPageInfo->GetImageType())
		{
		case 0:
			m_staticHeader.ResetBitmap (pPageInfo->GetIconImage());
			break;
		case 1:
			m_staticHeader.ResetBitmap (pPageInfo->GetBitmapImage(), pPageInfo->GetImageMask());
			break;
		default:
			m_staticHeader.ResetBitmap (NULL);
		}
	}
	else
		m_staticHeader.ResetBitmap (NULL);
	m_staticHeader.InvalidateRect (NULL);
//	m_staticHeader.SetWindowPos(NULL, 0, 0, 0, 0, uFlags);
}

long CuDlgDomTabCtrl::OnQueryOpenCursor(WPARAM wParam, LPARAM lParam)
{
	BOOL bExit = FALSE;
	//
	// If exist, it must be the last execution (index 0):

	int index = Find (IDD_DOMPROP_TABLE_ROWS, m_arrayDlg, 0, NUMBEROFDOMPAGES -1);
	if (index == -1)  
	{
		//
		// Forgot to add the page in the array!
		ASSERT (FALSE);
		return (long)FALSE;
	}
	else
	{
		if (m_arrayDlg[index].pPage)
		{
			CWnd* pWnd = m_arrayDlg[index].pPage;
			bExit = (BOOL)pWnd->SendMessage (WM_QUERY_OPEN_CURSOR, wParam, lParam);
		}
	}

	return (BOOL)bExit;
}
