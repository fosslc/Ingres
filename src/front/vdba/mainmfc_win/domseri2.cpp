/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DomSeri2.cpp, Code File                                               //
//               Splitted from domseri.cpp                                             //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : Emmanuel Blattes                                                      //
//                                                                                     //
//    Purpose  : Serialization of DOM right pane property pages (modeless dialog)      //
**
**  History:
**
**  27-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**  10-May-2001 (noifr01)
**      (bug 104694) manage the missing member variables in the CuDomPropDataPages class
**  02-Apr-2003 (schph01)
**     SIR #107523, Add class to serialize the list of Sequences.
****************************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "domseri2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// REPLICATOR CDDS
//

IMPLEMENT_SERIAL (CuDomPropDataReplicCdds, CuIpmPropertyData, 1)

CuDomPropDataReplicCdds::CuDomPropDataReplicCdds()
    :CuIpmPropertyData("CuDomPropDataReplicCdds")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataReplicCdds::CuDomPropDataReplicCdds(const CuDomPropDataReplicCdds & refPropDataReplicCdds)
    :CuIpmPropertyData("CuDomPropDataReplicCdds")
{
  m_acsReplicCdds.Copy(refPropDataReplicCdds.m_acsReplicCdds);
}

// '=' operator overloading
CuDomPropDataReplicCdds CuDomPropDataReplicCdds::operator = (const CuDomPropDataReplicCdds & refPropDataReplicCdds)
{
  if (this == &refPropDataReplicCdds)
    ASSERT (FALSE);

  m_acsReplicCdds.Copy(refPropDataReplicCdds.m_acsReplicCdds);

  return *this;
}

void CuDomPropDataReplicCdds::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataReplicCdds::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_acsReplicCdds.Serialize(ar);
}

//
// REPLICATOR CONNECTIONS
//

IMPLEMENT_SERIAL (CuDomPropDataReplicConn, CuIpmPropertyData, 1)

CuDomPropDataReplicConn::CuDomPropDataReplicConn()
    :CuIpmPropertyData("CuDomPropDataReplicConn")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataReplicConn::CuDomPropDataReplicConn(const CuDomPropDataReplicConn & refPropDataReplicConn)
    :CuIpmPropertyData("CuDomPropDataReplicConn")
{
  m_uaReplicConn.Copy(refPropDataReplicConn.m_uaReplicConn);
}

// '=' operator overloading
CuDomPropDataReplicConn CuDomPropDataReplicConn::operator = (const CuDomPropDataReplicConn & refPropDataReplicConn)
{
  if (this == &refPropDataReplicConn)
    ASSERT (FALSE);

  m_uaReplicConn.Copy(refPropDataReplicConn.m_uaReplicConn);

  return *this;
}

void CuDomPropDataReplicConn::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataReplicConn::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaReplicConn.Serialize(ar);
}

//
// REPLICATOR MAIL ALERT USERS
//

IMPLEMENT_SERIAL (CuDomPropDataReplicMailU, CuIpmPropertyData, 1)

CuDomPropDataReplicMailU::CuDomPropDataReplicMailU()
    :CuIpmPropertyData("CuDomPropDataReplicMailU")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataReplicMailU::CuDomPropDataReplicMailU(const CuDomPropDataReplicMailU & refPropDataReplicMailU)
    :CuIpmPropertyData("CuDomPropDataReplicMailU")
{
  m_acsReplicMailU.Copy(refPropDataReplicMailU.m_acsReplicMailU);
}

// '=' operator overloading
CuDomPropDataReplicMailU CuDomPropDataReplicMailU::operator = (const CuDomPropDataReplicMailU & refPropDataReplicMailU)
{
  if (this == &refPropDataReplicMailU)
    ASSERT (FALSE);

  m_acsReplicMailU.Copy(refPropDataReplicMailU.m_acsReplicMailU);

  return *this;
}

void CuDomPropDataReplicMailU::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataReplicMailU::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_acsReplicMailU.Serialize(ar);
}

//
// REPLICATOR REGISTERED TABLES
//

IMPLEMENT_SERIAL (CuDomPropDataReplicRegTbl, CuIpmPropertyData, 1)

CuDomPropDataReplicRegTbl::CuDomPropDataReplicRegTbl()
    :CuIpmPropertyData("CuDomPropDataReplicRegTbl")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataReplicRegTbl::CuDomPropDataReplicRegTbl(const CuDomPropDataReplicRegTbl & refPropDataReplicRegTbl)
    :CuIpmPropertyData("CuDomPropDataReplicRegTbl")
{
  m_uaReplicRegTbl.Copy(refPropDataReplicRegTbl.m_uaReplicRegTbl);
}

// '=' operator overloading
CuDomPropDataReplicRegTbl CuDomPropDataReplicRegTbl::operator = (const CuDomPropDataReplicRegTbl & refPropDataReplicRegTbl)
{
  if (this == &refPropDataReplicRegTbl)
    ASSERT (FALSE);

  m_uaReplicRegTbl.Copy(refPropDataReplicRegTbl.m_uaReplicRegTbl);

  return *this;
}

void CuDomPropDataReplicRegTbl::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataReplicRegTbl::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaReplicRegTbl.Serialize(ar);
}

//
// DB TABLES
//

IMPLEMENT_SERIAL (CuDomPropDataDbTbl, CuIpmPropertyData, 2)

CuDomPropDataDbTbl::CuDomPropDataDbTbl()
    :CuIpmPropertyData("CuDomPropDataDbTbl")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataDbTbl::CuDomPropDataDbTbl(const CuDomPropDataDbTbl & refPropDataDbTbl)
    :CuIpmPropertyData("CuDomPropDataDbTbl")
{
  m_uaDbTbl.Copy(refPropDataDbTbl.m_uaDbTbl);
  m_objType = refPropDataDbTbl.m_objType;
}

// '=' operator overloading
CuDomPropDataDbTbl CuDomPropDataDbTbl::operator = (const CuDomPropDataDbTbl & refPropDataDbTbl)
{
  if (this == &refPropDataDbTbl)
    ASSERT (FALSE);

  m_uaDbTbl.Copy(refPropDataDbTbl.m_uaDbTbl);
  m_objType = refPropDataDbTbl.m_objType;

  return *this;
}

void CuDomPropDataDbTbl::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataDbTbl::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaDbTbl.Serialize(ar);
}

//
// DB VIEWS
//

IMPLEMENT_SERIAL (CuDomPropDataDbView, CuIpmPropertyData, 2)

CuDomPropDataDbView::CuDomPropDataDbView()
    :CuIpmPropertyData("CuDomPropDataDbView")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataDbView::CuDomPropDataDbView(const CuDomPropDataDbView & refPropDataDbView)
    :CuIpmPropertyData("CuDomPropDataDbView")
{
  m_uaDbView.Copy(refPropDataDbView.m_uaDbView);
  m_objType = refPropDataDbView.m_objType;
}

// '=' operator overloading
CuDomPropDataDbView CuDomPropDataDbView::operator = (const CuDomPropDataDbView & refPropDataDbView)
{
  if (this == &refPropDataDbView)
    ASSERT (FALSE);

  m_uaDbView.Copy(refPropDataDbView.m_uaDbView);
  m_objType = refPropDataDbView.m_objType;

  return *this;
}

void CuDomPropDataDbView::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataDbView::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaDbView.Serialize(ar);
}

//
// DB PROCEDURES
//

IMPLEMENT_SERIAL (CuDomPropDataDbProc, CuIpmPropertyData, 2)

CuDomPropDataDbProc::CuDomPropDataDbProc()
    :CuIpmPropertyData("CuDomPropDataDbProc")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataDbProc::CuDomPropDataDbProc(const CuDomPropDataDbProc & refPropDataDbProc)
    :CuIpmPropertyData("CuDomPropDataDbProc")
{
  m_uaDbProc.Copy(refPropDataDbProc.m_uaDbProc);
  m_objType = refPropDataDbProc.m_objType;
}

// '=' operator overloading
CuDomPropDataDbProc CuDomPropDataDbProc::operator = (const CuDomPropDataDbProc & refPropDataDbProc)
{
  if (this == &refPropDataDbProc)
    ASSERT (FALSE);

  m_uaDbProc.Copy(refPropDataDbProc.m_uaDbProc);
  m_objType = refPropDataDbProc.m_objType;

  return *this;
}

void CuDomPropDataDbProc::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataDbProc::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaDbProc.Serialize(ar);
}

//
// DB SEQUENCES
//

IMPLEMENT_SERIAL (CuDomPropDataDbSeq, CuIpmPropertyData, 2)

CuDomPropDataDbSeq::CuDomPropDataDbSeq()
    :CuIpmPropertyData("CuDomPropDataDbSeq")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataDbSeq::CuDomPropDataDbSeq(const CuDomPropDataDbSeq & refPropDataDbSeq)
    :CuIpmPropertyData("CuDomPropDataDbSeq")
{
  m_uaDbSeq.Copy(refPropDataDbSeq.m_uaDbSeq);
  m_objType = refPropDataDbSeq.m_objType;
}

// '=' operator overloading
CuDomPropDataDbSeq CuDomPropDataDbSeq::operator = (const CuDomPropDataDbSeq & refPropDataDbSeq)
{
  if (this == &refPropDataDbSeq)
    ASSERT (FALSE);

  m_uaDbSeq.Copy(refPropDataDbSeq.m_uaDbSeq);
  m_objType = refPropDataDbSeq.m_objType;

  return *this;
}

void CuDomPropDataDbSeq::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataDbSeq::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaDbSeq.Serialize(ar);
}

//
// DB SCHEMAS
//

IMPLEMENT_SERIAL (CuDomPropDataDbSchema, CuIpmPropertyData, 2)

CuDomPropDataDbSchema::CuDomPropDataDbSchema()
    :CuIpmPropertyData("CuDomPropDataDbSchema")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataDbSchema::CuDomPropDataDbSchema(const CuDomPropDataDbSchema & refPropDataDbSchema)
    :CuIpmPropertyData("CuDomPropDataDbSchema")
{
  m_uaDbSchema.Copy(refPropDataDbSchema.m_uaDbSchema);
  m_objType = refPropDataDbSchema.m_objType;
}

// '=' operator overloading
CuDomPropDataDbSchema CuDomPropDataDbSchema::operator = (const CuDomPropDataDbSchema & refPropDataDbSchema)
{
  if (this == &refPropDataDbSchema)
    ASSERT (FALSE);

  m_uaDbSchema.Copy(refPropDataDbSchema.m_uaDbSchema);
  m_objType = refPropDataDbSchema.m_objType;

  return *this;
}

void CuDomPropDataDbSchema::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataDbSchema::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaDbSchema.Serialize(ar);
}

//
// DB SYNONYMS
//

IMPLEMENT_SERIAL (CuDomPropDataDbSyn, CuIpmPropertyData, 2)

CuDomPropDataDbSyn::CuDomPropDataDbSyn()
    :CuIpmPropertyData("CuDomPropDataDbSyn")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataDbSyn::CuDomPropDataDbSyn(const CuDomPropDataDbSyn & refPropDataDbSyn)
    :CuIpmPropertyData("CuDomPropDataDbSyn")
{
  m_uaDbSyn.Copy(refPropDataDbSyn.m_uaDbSyn);
  m_objType = refPropDataDbSyn.m_objType;
}

// '=' operator overloading
CuDomPropDataDbSyn CuDomPropDataDbSyn::operator = (const CuDomPropDataDbSyn & refPropDataDbSyn)
{
  if (this == &refPropDataDbSyn)
    ASSERT (FALSE);

  m_uaDbSyn.Copy(refPropDataDbSyn.m_uaDbSyn);
  m_objType = refPropDataDbSyn.m_objType;

  return *this;
}

void CuDomPropDataDbSyn::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataDbSyn::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaDbSyn.Serialize(ar);
}

//
// DB EVENTS
//

IMPLEMENT_SERIAL (CuDomPropDataDbEvt, CuIpmPropertyData, 2)

CuDomPropDataDbEvt::CuDomPropDataDbEvt()
    :CuIpmPropertyData("CuDomPropDataDbEvt")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataDbEvt::CuDomPropDataDbEvt(const CuDomPropDataDbEvt & refPropDataDbEvt)
    :CuIpmPropertyData("CuDomPropDataDbEvt")
{
  m_uaDbEvt.Copy(refPropDataDbEvt.m_uaDbEvt);
  m_objType = refPropDataDbEvt.m_objType;
}

// '=' operator overloading
CuDomPropDataDbEvt CuDomPropDataDbEvt::operator = (const CuDomPropDataDbEvt & refPropDataDbEvt)
{
  if (this == &refPropDataDbEvt)
    ASSERT (FALSE);

  m_uaDbEvt.Copy(refPropDataDbEvt.m_uaDbEvt);
  m_objType = refPropDataDbEvt.m_objType;

  return *this;
}

void CuDomPropDataDbEvt::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataDbEvt::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaDbEvt.Serialize(ar);
}

//
// TABLE INTEGRITIES
//

IMPLEMENT_SERIAL (CuDomPropDataTblInteg, CuIpmPropertyData, 2)

CuDomPropDataTblInteg::CuDomPropDataTblInteg()
    :CuIpmPropertyData("CuDomPropDataTblInteg")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataTblInteg::CuDomPropDataTblInteg(const CuDomPropDataTblInteg & refPropDataTblInteg)
    :CuIpmPropertyData("CuDomPropDataTblInteg")
{
  m_uaTblInteg.Copy(refPropDataTblInteg.m_uaTblInteg);
  m_objType = refPropDataTblInteg.m_objType;
}

// '=' operator overloading
CuDomPropDataTblInteg CuDomPropDataTblInteg::operator = (const CuDomPropDataTblInteg & refPropDataTblInteg)
{
  if (this == &refPropDataTblInteg)
    ASSERT (FALSE);

  m_uaTblInteg.Copy(refPropDataTblInteg.m_uaTblInteg);
  m_objType = refPropDataTblInteg.m_objType;

  return *this;
}

void CuDomPropDataTblInteg::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataTblInteg::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaTblInteg.Serialize(ar);
}

//
// TABLE RULES
//

IMPLEMENT_SERIAL (CuDomPropDataTblRule, CuIpmPropertyData, 2)

CuDomPropDataTblRule::CuDomPropDataTblRule()
    :CuIpmPropertyData("CuDomPropDataTblRule")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataTblRule::CuDomPropDataTblRule(const CuDomPropDataTblRule & refPropDataTblRule)
    :CuIpmPropertyData("CuDomPropDataTblRule")
{
  m_uaTblRule.Copy(refPropDataTblRule.m_uaTblRule);
  m_objType = refPropDataTblRule.m_objType;
}

// '=' operator overloading
CuDomPropDataTblRule CuDomPropDataTblRule::operator = (const CuDomPropDataTblRule & refPropDataTblRule)
{
  if (this == &refPropDataTblRule)
    ASSERT (FALSE);

  m_uaTblRule.Copy(refPropDataTblRule.m_uaTblRule);
  m_objType = refPropDataTblRule.m_objType;

  return *this;
}

void CuDomPropDataTblRule::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataTblRule::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaTblRule.Serialize(ar);
}

//
// TABLE INDEXES
//

IMPLEMENT_SERIAL (CuDomPropDataTblIdx, CuIpmPropertyData, 2)

CuDomPropDataTblIdx::CuDomPropDataTblIdx()
    :CuIpmPropertyData("CuDomPropDataTblIdx")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataTblIdx::CuDomPropDataTblIdx(const CuDomPropDataTblIdx & refPropDataTblIdx)
    :CuIpmPropertyData("CuDomPropDataTblIdx")
{
  m_uaTblIdx.Copy(refPropDataTblIdx.m_uaTblIdx);
  m_objType = refPropDataTblIdx.m_objType;
}

// '=' operator overloading
CuDomPropDataTblIdx CuDomPropDataTblIdx::operator = (const CuDomPropDataTblIdx & refPropDataTblIdx)
{
  if (this == &refPropDataTblIdx)
    ASSERT (FALSE);

  m_uaTblIdx.Copy(refPropDataTblIdx.m_uaTblIdx);
  m_objType = refPropDataTblIdx.m_objType;

  return *this;
}

void CuDomPropDataTblIdx::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataTblIdx::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaTblIdx.Serialize(ar);
}

//
// TABLE LOCATIONS
//

IMPLEMENT_SERIAL (CuDomPropDataTblLoc, CuIpmPropertyData, 2)

CuDomPropDataTblLoc::CuDomPropDataTblLoc()
    :CuIpmPropertyData("CuDomPropDataTblLoc")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataTblLoc::CuDomPropDataTblLoc(const CuDomPropDataTblLoc & refPropDataTblLoc)
    :CuIpmPropertyData("CuDomPropDataTblLoc")
{
  m_uaTblLoc.Copy(refPropDataTblLoc.m_uaTblLoc);
  m_objType = refPropDataTblLoc.m_objType;
}

// '=' operator overloading
CuDomPropDataTblLoc CuDomPropDataTblLoc::operator = (const CuDomPropDataTblLoc & refPropDataTblLoc)
{
  if (this == &refPropDataTblLoc)
    ASSERT (FALSE);

  m_uaTblLoc.Copy(refPropDataTblLoc.m_uaTblLoc);
  m_objType = refPropDataTblLoc.m_objType;

  return *this;
}

void CuDomPropDataTblLoc::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataTblLoc::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaTblLoc.Serialize(ar);
}

//
// USER RELATED TABLE SECURITY ALARMS (XALARMS)
//

IMPLEMENT_SERIAL (CuDomPropDataUserXAlarm, CuIpmPropertyData, 1)

CuDomPropDataUserXAlarm::CuDomPropDataUserXAlarm()
    :CuIpmPropertyData("CuDomPropDataUserXAlarm")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataUserXAlarm::CuDomPropDataUserXAlarm(const CuDomPropDataUserXAlarm & refPropDataUserXAlarm)
    :CuIpmPropertyData("CuDomPropDataUserXAlarm")
{
  m_uaUserXAlarm.Copy(refPropDataUserXAlarm.m_uaUserXAlarm);
}

// '=' operator overloading
CuDomPropDataUserXAlarm CuDomPropDataUserXAlarm::operator = (const CuDomPropDataUserXAlarm & refPropDataUserXAlarm)
{
  if (this == &refPropDataUserXAlarm)
    ASSERT (FALSE);

  m_uaUserXAlarm.Copy(refPropDataUserXAlarm.m_uaUserXAlarm);

  return *this;
}

void CuDomPropDataUserXAlarm::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataUserXAlarm::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaUserXAlarm.Serialize(ar);
}

//
// USER RELATED GRANTED DATABASES
//

IMPLEMENT_SERIAL (CuDomPropDataUserXGrantedDb, CuIpmPropertyData, 1)

CuDomPropDataUserXGrantedDb::CuDomPropDataUserXGrantedDb()
    :CuIpmPropertyData("CuDomPropDataUserXGrantedDb")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataUserXGrantedDb::CuDomPropDataUserXGrantedDb(const CuDomPropDataUserXGrantedDb & refPropDataUserXGrantedDb)
    :CuIpmPropertyData("CuDomPropDataUserXGrantedDb")
{
  m_uaUserXGrantedDb.Copy(refPropDataUserXGrantedDb.m_uaUserXGrantedDb);
}

// '=' operator overloading
CuDomPropDataUserXGrantedDb CuDomPropDataUserXGrantedDb::operator = (const CuDomPropDataUserXGrantedDb & refPropDataUserXGrantedDb)
{
  if (this == &refPropDataUserXGrantedDb)
    ASSERT (FALSE);

  m_uaUserXGrantedDb.Copy(refPropDataUserXGrantedDb.m_uaUserXGrantedDb);

  return *this;
}

void CuDomPropDataUserXGrantedDb::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataUserXGrantedDb::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaUserXGrantedDb.Serialize(ar);
}

//
// USER RELATED GRANTED TABLES
//

IMPLEMENT_SERIAL (CuDomPropDataUserXGrantedTbl, CuIpmPropertyData, 2)

CuDomPropDataUserXGrantedTbl::CuDomPropDataUserXGrantedTbl()
    :CuIpmPropertyData("CuDomPropDataUserXGrantedTbl")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataUserXGrantedTbl::CuDomPropDataUserXGrantedTbl(const CuDomPropDataUserXGrantedTbl & refPropDataUserXGrantedTbl)
    :CuIpmPropertyData("CuDomPropDataUserXGrantedTbl")
{
  m_uaUserXGrantedTbl.Copy(refPropDataUserXGrantedTbl.m_uaUserXGrantedTbl);
}

// '=' operator overloading
CuDomPropDataUserXGrantedTbl CuDomPropDataUserXGrantedTbl::operator = (const CuDomPropDataUserXGrantedTbl & refPropDataUserXGrantedTbl)
{
  if (this == &refPropDataUserXGrantedTbl)
    ASSERT (FALSE);

  m_uaUserXGrantedTbl.Copy(refPropDataUserXGrantedTbl.m_uaUserXGrantedTbl);

  return *this;
}

void CuDomPropDataUserXGrantedTbl::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataUserXGrantedTbl::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaUserXGrantedTbl.Serialize(ar);
}

//
// USER RELATED GRANTED VIEWS
//

IMPLEMENT_SERIAL (CuDomPropDataUserXGrantedView, CuIpmPropertyData, 1)

CuDomPropDataUserXGrantedView::CuDomPropDataUserXGrantedView()
    :CuIpmPropertyData("CuDomPropDataUserXGrantedView")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataUserXGrantedView::CuDomPropDataUserXGrantedView(const CuDomPropDataUserXGrantedView & refPropDataUserXGrantedView)
    :CuIpmPropertyData("CuDomPropDataUserXGrantedView")
{
  m_uaUserXGrantedView.Copy(refPropDataUserXGrantedView.m_uaUserXGrantedView);
}

// '=' operator overloading
CuDomPropDataUserXGrantedView CuDomPropDataUserXGrantedView::operator = (const CuDomPropDataUserXGrantedView & refPropDataUserXGrantedView)
{
  if (this == &refPropDataUserXGrantedView)
    ASSERT (FALSE);

  m_uaUserXGrantedView.Copy(refPropDataUserXGrantedView.m_uaUserXGrantedView);

  return *this;
}

void CuDomPropDataUserXGrantedView::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataUserXGrantedView::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaUserXGrantedView.Serialize(ar);
}

//
// USER RELATED GRANTED DBEVENTS
//

IMPLEMENT_SERIAL (CuDomPropDataUserXGrantedEvent, CuIpmPropertyData, 1)

CuDomPropDataUserXGrantedEvent::CuDomPropDataUserXGrantedEvent()
    :CuIpmPropertyData("CuDomPropDataUserXGrantedEvent")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataUserXGrantedEvent::CuDomPropDataUserXGrantedEvent(const CuDomPropDataUserXGrantedEvent & refPropDataUserXGrantedEvent)
    :CuIpmPropertyData("CuDomPropDataUserXGrantedEvent")
{
  m_uaUserXGrantedEvent.Copy(refPropDataUserXGrantedEvent.m_uaUserXGrantedEvent);
}

// '=' operator overloading
CuDomPropDataUserXGrantedEvent CuDomPropDataUserXGrantedEvent::operator = (const CuDomPropDataUserXGrantedEvent & refPropDataUserXGrantedEvent)
{
  if (this == &refPropDataUserXGrantedEvent)
    ASSERT (FALSE);

  m_uaUserXGrantedEvent.Copy(refPropDataUserXGrantedEvent.m_uaUserXGrantedEvent);

  return *this;
}

void CuDomPropDataUserXGrantedEvent::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataUserXGrantedEvent::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaUserXGrantedEvent.Serialize(ar);
}

//
// USER RELATED GRANTED PROCEDURES
//

IMPLEMENT_SERIAL (CuDomPropDataUserXGrantedProc, CuIpmPropertyData, 1)

CuDomPropDataUserXGrantedProc::CuDomPropDataUserXGrantedProc()
    :CuIpmPropertyData("CuDomPropDataUserXGrantedProc")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataUserXGrantedProc::CuDomPropDataUserXGrantedProc(const CuDomPropDataUserXGrantedProc & refPropDataUserXGrantedProc)
    :CuIpmPropertyData("CuDomPropDataUserXGrantedProc")
{
  m_uaUserXGrantedProc.Copy(refPropDataUserXGrantedProc.m_uaUserXGrantedProc);
}

// '=' operator overloading
CuDomPropDataUserXGrantedProc CuDomPropDataUserXGrantedProc::operator = (const CuDomPropDataUserXGrantedProc & refPropDataUserXGrantedProc)
{
  if (this == &refPropDataUserXGrantedProc)
    ASSERT (FALSE);

  m_uaUserXGrantedProc.Copy(refPropDataUserXGrantedProc.m_uaUserXGrantedProc);

  return *this;
}

void CuDomPropDataUserXGrantedProc::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataUserXGrantedProc::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaUserXGrantedProc.Serialize(ar);
}

//
// USER RELATED GRANTED SEQUENCES
//

IMPLEMENT_SERIAL (CuDomPropDataUserXGrantedSeq, CuIpmPropertyData, 1)

CuDomPropDataUserXGrantedSeq::CuDomPropDataUserXGrantedSeq()
    :CuIpmPropertyData("CuDomPropDataUserXGrantedSeq")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataUserXGrantedSeq::CuDomPropDataUserXGrantedSeq(const CuDomPropDataUserXGrantedSeq & refPropDataUserXGrantedSeq)
    :CuIpmPropertyData("CuDomPropDataUserXGrantedSeq")
{
  m_uaUserXGrantedSeq.Copy(refPropDataUserXGrantedSeq.m_uaUserXGrantedSeq);
}

// '=' operator overloading
CuDomPropDataUserXGrantedSeq CuDomPropDataUserXGrantedSeq::operator = (const CuDomPropDataUserXGrantedSeq & refPropDataUserXGrantedSeq)
{
  if (this == &refPropDataUserXGrantedSeq)
    ASSERT (FALSE);

  m_uaUserXGrantedSeq.Copy(refPropDataUserXGrantedSeq.m_uaUserXGrantedSeq);

  return *this;
}

void CuDomPropDataUserXGrantedSeq::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataUserXGrantedSeq::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaUserXGrantedSeq.Serialize(ar);
}

//
// USER RELATED GRANTED ROLES
//

IMPLEMENT_SERIAL (CuDomPropDataUserXGrantedRole, CuIpmPropertyData, 1)

CuDomPropDataUserXGrantedRole::CuDomPropDataUserXGrantedRole()
    :CuIpmPropertyData("CuDomPropDataUserXGrantedRole")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataUserXGrantedRole::CuDomPropDataUserXGrantedRole(const CuDomPropDataUserXGrantedRole & refPropDataUserXGrantedRole)
    :CuIpmPropertyData("CuDomPropDataUserXGrantedRole")
{
  m_uaUserXGrantedRole.Copy(refPropDataUserXGrantedRole.m_uaUserXGrantedRole);
}

// '=' operator overloading
CuDomPropDataUserXGrantedRole CuDomPropDataUserXGrantedRole::operator = (const CuDomPropDataUserXGrantedRole & refPropDataUserXGrantedRole)
{
  if (this == &refPropDataUserXGrantedRole)
    ASSERT (FALSE);

  m_uaUserXGrantedRole.Copy(refPropDataUserXGrantedRole.m_uaUserXGrantedRole);

  return *this;
}

void CuDomPropDataUserXGrantedRole::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataUserXGrantedRole::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaUserXGrantedRole.Serialize(ar);
}

//
// GROUP RELATED GRANTED DATABASES
//

IMPLEMENT_SERIAL (CuDomPropDataGroupXGrantedDb, CuIpmPropertyData, 1)

CuDomPropDataGroupXGrantedDb::CuDomPropDataGroupXGrantedDb()
    :CuIpmPropertyData("CuDomPropDataGroupXGrantedDb")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataGroupXGrantedDb::CuDomPropDataGroupXGrantedDb(const CuDomPropDataGroupXGrantedDb & refPropDataGroupXGrantedDb)
    :CuIpmPropertyData("CuDomPropDataGroupXGrantedDb")
{
  m_uaGroupXGrantedDb.Copy(refPropDataGroupXGrantedDb.m_uaGroupXGrantedDb);
}

// '=' operator overloading
CuDomPropDataGroupXGrantedDb CuDomPropDataGroupXGrantedDb::operator = (const CuDomPropDataGroupXGrantedDb & refPropDataGroupXGrantedDb)
{
  if (this == &refPropDataGroupXGrantedDb)
    ASSERT (FALSE);

  m_uaGroupXGrantedDb.Copy(refPropDataGroupXGrantedDb.m_uaGroupXGrantedDb);

  return *this;
}

void CuDomPropDataGroupXGrantedDb::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataGroupXGrantedDb::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaGroupXGrantedDb.Serialize(ar);
}

//
// GROUP RELATED GRANTED TABLES
//

IMPLEMENT_SERIAL (CuDomPropDataGroupXGrantedTbl, CuIpmPropertyData, 2)

CuDomPropDataGroupXGrantedTbl::CuDomPropDataGroupXGrantedTbl()
    :CuIpmPropertyData("CuDomPropDataGroupXGrantedTbl")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataGroupXGrantedTbl::CuDomPropDataGroupXGrantedTbl(const CuDomPropDataGroupXGrantedTbl & refPropDataGroupXGrantedTbl)
    :CuIpmPropertyData("CuDomPropDataGroupXGrantedTbl")
{
  m_uaGroupXGrantedTbl.Copy(refPropDataGroupXGrantedTbl.m_uaGroupXGrantedTbl);
}

// '=' operator overloading
CuDomPropDataGroupXGrantedTbl CuDomPropDataGroupXGrantedTbl::operator = (const CuDomPropDataGroupXGrantedTbl & refPropDataGroupXGrantedTbl)
{
  if (this == &refPropDataGroupXGrantedTbl)
    ASSERT (FALSE);

  m_uaGroupXGrantedTbl.Copy(refPropDataGroupXGrantedTbl.m_uaGroupXGrantedTbl);

  return *this;
}

void CuDomPropDataGroupXGrantedTbl::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataGroupXGrantedTbl::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaGroupXGrantedTbl.Serialize(ar);
}

//
// GROUP RELATED GRANTED VIEWS
//

IMPLEMENT_SERIAL (CuDomPropDataGroupXGrantedView, CuIpmPropertyData, 1)

CuDomPropDataGroupXGrantedView::CuDomPropDataGroupXGrantedView()
    :CuIpmPropertyData("CuDomPropDataGroupXGrantedView")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataGroupXGrantedView::CuDomPropDataGroupXGrantedView(const CuDomPropDataGroupXGrantedView & refPropDataGroupXGrantedView)
    :CuIpmPropertyData("CuDomPropDataGroupXGrantedView")
{
  m_uaGroupXGrantedView.Copy(refPropDataGroupXGrantedView.m_uaGroupXGrantedView);
}

// '=' operator overloading
CuDomPropDataGroupXGrantedView CuDomPropDataGroupXGrantedView::operator = (const CuDomPropDataGroupXGrantedView & refPropDataGroupXGrantedView)
{
  if (this == &refPropDataGroupXGrantedView)
    ASSERT (FALSE);

  m_uaGroupXGrantedView.Copy(refPropDataGroupXGrantedView.m_uaGroupXGrantedView);

  return *this;
}

void CuDomPropDataGroupXGrantedView::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataGroupXGrantedView::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaGroupXGrantedView.Serialize(ar);
}

//
// GROUP RELATED GRANTED DBEVENTS
//

IMPLEMENT_SERIAL (CuDomPropDataGroupXGrantedEvent, CuIpmPropertyData, 1)

CuDomPropDataGroupXGrantedEvent::CuDomPropDataGroupXGrantedEvent()
    :CuIpmPropertyData("CuDomPropDataGroupXGrantedEvent")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataGroupXGrantedEvent::CuDomPropDataGroupXGrantedEvent(const CuDomPropDataGroupXGrantedEvent & refPropDataGroupXGrantedEvent)
    :CuIpmPropertyData("CuDomPropDataGroupXGrantedEvent")
{
  m_uaGroupXGrantedEvent.Copy(refPropDataGroupXGrantedEvent.m_uaGroupXGrantedEvent);
}

// '=' operator overloading
CuDomPropDataGroupXGrantedEvent CuDomPropDataGroupXGrantedEvent::operator = (const CuDomPropDataGroupXGrantedEvent & refPropDataGroupXGrantedEvent)
{
  if (this == &refPropDataGroupXGrantedEvent)
    ASSERT (FALSE);

  m_uaGroupXGrantedEvent.Copy(refPropDataGroupXGrantedEvent.m_uaGroupXGrantedEvent);

  return *this;
}

void CuDomPropDataGroupXGrantedEvent::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataGroupXGrantedEvent::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaGroupXGrantedEvent.Serialize(ar);
}

//
// GROUP RELATED GRANTED PROCEDURES
//

IMPLEMENT_SERIAL (CuDomPropDataGroupXGrantedProc, CuIpmPropertyData, 1)

CuDomPropDataGroupXGrantedProc::CuDomPropDataGroupXGrantedProc()
    :CuIpmPropertyData("CuDomPropDataGroupXGrantedProc")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataGroupXGrantedProc::CuDomPropDataGroupXGrantedProc(const CuDomPropDataGroupXGrantedProc & refPropDataGroupXGrantedProc)
    :CuIpmPropertyData("CuDomPropDataGroupXGrantedProc")
{
  m_uaGroupXGrantedProc.Copy(refPropDataGroupXGrantedProc.m_uaGroupXGrantedProc);
}

// '=' operator overloading
CuDomPropDataGroupXGrantedProc CuDomPropDataGroupXGrantedProc::operator = (const CuDomPropDataGroupXGrantedProc & refPropDataGroupXGrantedProc)
{
  if (this == &refPropDataGroupXGrantedProc)
    ASSERT (FALSE);

  m_uaGroupXGrantedProc.Copy(refPropDataGroupXGrantedProc.m_uaGroupXGrantedProc);

  return *this;
}

void CuDomPropDataGroupXGrantedProc::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataGroupXGrantedProc::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaGroupXGrantedProc.Serialize(ar);
}

//
// GROUP RELATED GRANTED SEQUENCES
//

IMPLEMENT_SERIAL (CuDomPropDataGroupXGrantedSeq, CuIpmPropertyData, 1)

CuDomPropDataGroupXGrantedSeq::CuDomPropDataGroupXGrantedSeq()
    :CuIpmPropertyData("CuDomPropDataGroupXGrantedSeq")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataGroupXGrantedSeq::CuDomPropDataGroupXGrantedSeq(const CuDomPropDataGroupXGrantedSeq & refPropDataGroupXGrantedSeq)
    :CuIpmPropertyData("CuDomPropDataGroupXGrantedSeq")
{
  m_uaGroupXGrantedSeq.Copy(refPropDataGroupXGrantedSeq.m_uaGroupXGrantedSeq);
}

// '=' operator overloading
CuDomPropDataGroupXGrantedSeq CuDomPropDataGroupXGrantedSeq::operator = (const CuDomPropDataGroupXGrantedSeq & refPropDataGroupXGrantedSeq)
{
  if (this == &refPropDataGroupXGrantedSeq)
    ASSERT (FALSE);

  m_uaGroupXGrantedSeq.Copy(refPropDataGroupXGrantedSeq.m_uaGroupXGrantedSeq);

  return *this;
}

void CuDomPropDataGroupXGrantedSeq::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataGroupXGrantedSeq::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaGroupXGrantedSeq.Serialize(ar);
}

//
// ROLE RELATED GRANTED DATABASES
//

IMPLEMENT_SERIAL (CuDomPropDataRoleXGrantedDb, CuIpmPropertyData, 1)

CuDomPropDataRoleXGrantedDb::CuDomPropDataRoleXGrantedDb()
    :CuIpmPropertyData("CuDomPropDataRoleXGrantedDb")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataRoleXGrantedDb::CuDomPropDataRoleXGrantedDb(const CuDomPropDataRoleXGrantedDb & refPropDataRoleXGrantedDb)
    :CuIpmPropertyData("CuDomPropDataRoleXGrantedDb")
{
  m_uaRoleXGrantedDb.Copy(refPropDataRoleXGrantedDb.m_uaRoleXGrantedDb);
}

// '=' operator overloading
CuDomPropDataRoleXGrantedDb CuDomPropDataRoleXGrantedDb::operator = (const CuDomPropDataRoleXGrantedDb & refPropDataRoleXGrantedDb)
{
  if (this == &refPropDataRoleXGrantedDb)
    ASSERT (FALSE);

  m_uaRoleXGrantedDb.Copy(refPropDataRoleXGrantedDb.m_uaRoleXGrantedDb);

  return *this;
}

void CuDomPropDataRoleXGrantedDb::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataRoleXGrantedDb::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaRoleXGrantedDb.Serialize(ar);
}

//
// ROLE RELATED GRANTED TABLES
//

IMPLEMENT_SERIAL (CuDomPropDataRoleXGrantedTbl, CuIpmPropertyData, 2)

CuDomPropDataRoleXGrantedTbl::CuDomPropDataRoleXGrantedTbl()
    :CuIpmPropertyData("CuDomPropDataRoleXGrantedTbl")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataRoleXGrantedTbl::CuDomPropDataRoleXGrantedTbl(const CuDomPropDataRoleXGrantedTbl & refPropDataRoleXGrantedTbl)
    :CuIpmPropertyData("CuDomPropDataRoleXGrantedTbl")
{
  m_uaRoleXGrantedTbl.Copy(refPropDataRoleXGrantedTbl.m_uaRoleXGrantedTbl);
}

// '=' operator overloading
CuDomPropDataRoleXGrantedTbl CuDomPropDataRoleXGrantedTbl::operator = (const CuDomPropDataRoleXGrantedTbl & refPropDataRoleXGrantedTbl)
{
  if (this == &refPropDataRoleXGrantedTbl)
    ASSERT (FALSE);

  m_uaRoleXGrantedTbl.Copy(refPropDataRoleXGrantedTbl.m_uaRoleXGrantedTbl);

  return *this;
}

void CuDomPropDataRoleXGrantedTbl::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataRoleXGrantedTbl::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaRoleXGrantedTbl.Serialize(ar);
}

//
// ROLE RELATED GRANTED VIEWS
//

IMPLEMENT_SERIAL (CuDomPropDataRoleXGrantedView, CuIpmPropertyData, 1)

CuDomPropDataRoleXGrantedView::CuDomPropDataRoleXGrantedView()
    :CuIpmPropertyData("CuDomPropDataRoleXGrantedView")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataRoleXGrantedView::CuDomPropDataRoleXGrantedView(const CuDomPropDataRoleXGrantedView & refPropDataRoleXGrantedView)
    :CuIpmPropertyData("CuDomPropDataRoleXGrantedView")
{
  m_uaRoleXGrantedView.Copy(refPropDataRoleXGrantedView.m_uaRoleXGrantedView);
}

// '=' operator overloading
CuDomPropDataRoleXGrantedView CuDomPropDataRoleXGrantedView::operator = (const CuDomPropDataRoleXGrantedView & refPropDataRoleXGrantedView)
{
  if (this == &refPropDataRoleXGrantedView)
    ASSERT (FALSE);

  m_uaRoleXGrantedView.Copy(refPropDataRoleXGrantedView.m_uaRoleXGrantedView);

  return *this;
}

void CuDomPropDataRoleXGrantedView::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataRoleXGrantedView::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaRoleXGrantedView.Serialize(ar);
}

//
// ROLE RELATED GRANTED DBEVENTS
//

IMPLEMENT_SERIAL (CuDomPropDataRoleXGrantedEvent, CuIpmPropertyData, 1)

CuDomPropDataRoleXGrantedEvent::CuDomPropDataRoleXGrantedEvent()
    :CuIpmPropertyData("CuDomPropDataRoleXGrantedEvent")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataRoleXGrantedEvent::CuDomPropDataRoleXGrantedEvent(const CuDomPropDataRoleXGrantedEvent & refPropDataRoleXGrantedEvent)
    :CuIpmPropertyData("CuDomPropDataRoleXGrantedEvent")
{
  m_uaRoleXGrantedEvent.Copy(refPropDataRoleXGrantedEvent.m_uaRoleXGrantedEvent);
}

// '=' operator overloading
CuDomPropDataRoleXGrantedEvent CuDomPropDataRoleXGrantedEvent::operator = (const CuDomPropDataRoleXGrantedEvent & refPropDataRoleXGrantedEvent)
{
  if (this == &refPropDataRoleXGrantedEvent)
    ASSERT (FALSE);

  m_uaRoleXGrantedEvent.Copy(refPropDataRoleXGrantedEvent.m_uaRoleXGrantedEvent);

  return *this;
}

void CuDomPropDataRoleXGrantedEvent::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataRoleXGrantedEvent::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaRoleXGrantedEvent.Serialize(ar);
}

//
// ROLE RELATED GRANTED PROCEDURES
//

IMPLEMENT_SERIAL (CuDomPropDataRoleXGrantedProc, CuIpmPropertyData, 1)

CuDomPropDataRoleXGrantedProc::CuDomPropDataRoleXGrantedProc()
    :CuIpmPropertyData("CuDomPropDataRoleXGrantedProc")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataRoleXGrantedProc::CuDomPropDataRoleXGrantedProc(const CuDomPropDataRoleXGrantedProc & refPropDataRoleXGrantedProc)
    :CuIpmPropertyData("CuDomPropDataRoleXGrantedProc")
{
  m_uaRoleXGrantedProc.Copy(refPropDataRoleXGrantedProc.m_uaRoleXGrantedProc);
}

// '=' operator overloading
CuDomPropDataRoleXGrantedProc CuDomPropDataRoleXGrantedProc::operator = (const CuDomPropDataRoleXGrantedProc & refPropDataRoleXGrantedProc)
{
  if (this == &refPropDataRoleXGrantedProc)
    ASSERT (FALSE);

  m_uaRoleXGrantedProc.Copy(refPropDataRoleXGrantedProc.m_uaRoleXGrantedProc);

  return *this;
}

void CuDomPropDataRoleXGrantedProc::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataRoleXGrantedProc::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaRoleXGrantedProc.Serialize(ar);
}

//
// ROLE RELATED GRANTED SEQUENCES
//

IMPLEMENT_SERIAL (CuDomPropDataRoleXGrantedSeq, CuIpmPropertyData, 1)

CuDomPropDataRoleXGrantedSeq::CuDomPropDataRoleXGrantedSeq()
    :CuIpmPropertyData("CuDomPropDataRoleXGrantedSeq")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataRoleXGrantedSeq::CuDomPropDataRoleXGrantedSeq(const CuDomPropDataRoleXGrantedSeq & refPropDataRoleXGrantedSeq)
    :CuIpmPropertyData("CuDomPropDataRoleXGrantedSeq")
{
  m_uaRoleXGrantedSeq.Copy(refPropDataRoleXGrantedSeq.m_uaRoleXGrantedSeq);
}

// '=' operator overloading
CuDomPropDataRoleXGrantedSeq CuDomPropDataRoleXGrantedSeq::operator = (const CuDomPropDataRoleXGrantedSeq & refPropDataRoleXGrantedSeq)
{
  if (this == &refPropDataRoleXGrantedSeq)
    ASSERT (FALSE);

  m_uaRoleXGrantedSeq.Copy(refPropDataRoleXGrantedSeq.m_uaRoleXGrantedSeq);

  return *this;
}

void CuDomPropDataRoleXGrantedSeq::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataRoleXGrantedSeq::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaRoleXGrantedSeq.Serialize(ar);
}


//
// CONNECTION
//

IMPLEMENT_SERIAL (CuDomPropDataConnection, CuIpmPropertyData, 2)

CuDomPropDataConnection::CuDomPropDataConnection()
    :CuIpmPropertyData("CuDomPropDataConnection")
{
  m_csDescription = _T("");
  m_nNumber       = -1;
  m_csVnode       = _T("");
  m_csDb          = _T("");
  m_csType        = _T("");
  m_bLocal        = FALSE;
}

CuDomPropDataConnection::CuDomPropDataConnection(const CuDomPropDataConnection & refPropDataConnection)
    :CuIpmPropertyData("CuDomPropDataConnection")
{
  m_csDescription = refPropDataConnection.m_csDescription;
  m_nNumber       = refPropDataConnection.m_nNumber      ;
  m_csVnode       = refPropDataConnection.m_csVnode      ;
  m_csDb          = refPropDataConnection.m_csDb         ;
  m_csType        = refPropDataConnection.m_csType       ;
  m_bLocal        = refPropDataConnection.m_bLocal       ;

  m_refreshParams = refPropDataConnection.m_refreshParams;
}

CuDomPropDataConnection CuDomPropDataConnection::operator = (const CuDomPropDataConnection & refPropDataConnection)
{
  if (this == &refPropDataConnection)
    ASSERT (FALSE);

  m_csDescription = refPropDataConnection.m_csDescription;
  m_nNumber       = refPropDataConnection.m_nNumber      ;
  m_csVnode       = refPropDataConnection.m_csVnode      ;
  m_csDb          = refPropDataConnection.m_csDb         ;
  m_csType        = refPropDataConnection.m_csType       ;
  m_bLocal        = refPropDataConnection.m_bLocal       ;

  m_refreshParams = refPropDataConnection.m_refreshParams;

  return *this;
}

void CuDomPropDataConnection::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataConnection::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csDescription;
    ar << m_nNumber      ;
    ar << m_csVnode      ;
    ar << m_csDb         ;
    ar << m_csType       ;
    ar << m_bLocal       ;
  }
  else {
    ar >> m_csDescription;
    ar >> m_nNumber      ;
    ar >> m_csVnode      ;
    ar >> m_csDb         ;
    ar >> m_csType       ;
    ar >> m_bLocal       ;
  }
  m_refreshParams.Serialize(ar);
}

//
// SCHEMA TABLES
//

IMPLEMENT_SERIAL (CuDomPropDataSchemaTbl, CuIpmPropertyData, 1)

CuDomPropDataSchemaTbl::CuDomPropDataSchemaTbl()
    :CuIpmPropertyData("CuDomPropDataSchemaTbl")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataSchemaTbl::CuDomPropDataSchemaTbl(const CuDomPropDataSchemaTbl & refPropDataSchemaTbl)
    :CuIpmPropertyData("CuDomPropDataSchemaTbl")
{
  m_uaSchemaTbl.Copy(refPropDataSchemaTbl.m_uaSchemaTbl);
}

// '=' operator overloading
CuDomPropDataSchemaTbl CuDomPropDataSchemaTbl::operator = (const CuDomPropDataSchemaTbl & refPropDataSchemaTbl)
{
  if (this == &refPropDataSchemaTbl)
    ASSERT (FALSE);

  m_uaSchemaTbl.Copy(refPropDataSchemaTbl.m_uaSchemaTbl);

  return *this;
}

void CuDomPropDataSchemaTbl::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataSchemaTbl::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaSchemaTbl.Serialize(ar);
}

//
// SCHEMA VIEWS
//

IMPLEMENT_SERIAL (CuDomPropDataSchemaView, CuIpmPropertyData, 1)

CuDomPropDataSchemaView::CuDomPropDataSchemaView()
    :CuIpmPropertyData("CuDomPropDataSchemaView")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataSchemaView::CuDomPropDataSchemaView(const CuDomPropDataSchemaView & refPropDataSchemaView)
    :CuIpmPropertyData("CuDomPropDataSchemaView")
{
  m_uaSchemaView.Copy(refPropDataSchemaView.m_uaSchemaView);
}

// '=' operator overloading
CuDomPropDataSchemaView CuDomPropDataSchemaView::operator = (const CuDomPropDataSchemaView & refPropDataSchemaView)
{
  if (this == &refPropDataSchemaView)
    ASSERT (FALSE);

  m_uaSchemaView.Copy(refPropDataSchemaView.m_uaSchemaView);

  return *this;
}

void CuDomPropDataSchemaView::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataSchemaView::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaSchemaView.Serialize(ar);
}

//
// SCHEMA PROCEDURES
//

IMPLEMENT_SERIAL (CuDomPropDataSchemaProc, CuIpmPropertyData, 1)

CuDomPropDataSchemaProc::CuDomPropDataSchemaProc()
    :CuIpmPropertyData("CuDomPropDataSchemaProc")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataSchemaProc::CuDomPropDataSchemaProc(const CuDomPropDataSchemaProc & refPropDataSchemaProc)
    :CuIpmPropertyData("CuDomPropDataSchemaProc")
{
  m_uaSchemaProc.Copy(refPropDataSchemaProc.m_uaSchemaProc);
}

// '=' operator overloading
CuDomPropDataSchemaProc CuDomPropDataSchemaProc::operator = (const CuDomPropDataSchemaProc & refPropDataSchemaProc)
{
  if (this == &refPropDataSchemaProc)
    ASSERT (FALSE);

  m_uaSchemaProc.Copy(refPropDataSchemaProc.m_uaSchemaProc);

  return *this;
}

void CuDomPropDataSchemaProc::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataSchemaProc::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaSchemaProc.Serialize(ar);
}

//
// TABLE AND INDEX PAGES
//


IMPLEMENT_SERIAL (CuDomPropDataPages, CuIpmPropertyData, 5)

CuDomPropDataPages::CuDomPropDataPages()
    :CuIpmPropertyData("CuDomPropDataPages")
{
  // nothing to do for CStringArrays

  m_bMustDeleteArray = FALSE;   // used in destructor

  m_lTotal        = -1L;
  m_lInUse        = -1L;
  m_lOverflow     = -1L;
  m_lFreed        = -1L;
  m_lNeverUsed    = -1L;

  m_lbuckets_no_ovf = -1;
  m_lemptybuckets_no_ovf = -1;
  m_lbuck_with_ovf = -1;
  m_lemptybuckets_with_ovf = -1;
  m_loverflow_not_empty = -1;
  m_loverflow_empty = -1;

  m_lItemsPerByte = -1L;
  m_nbElements    = -1L;
  m_pByteArray    = NULL;

  m_lCurPgPerCell = -1L;

  m_bHashed       = FALSE;

  m_llongest_ovf_chain = -1L;
  m_lbuck_longestovfchain = -1L;
  m_favg_ovf_chain = -1.0;

}

// copy constructor
CuDomPropDataPages::CuDomPropDataPages(const CuDomPropDataPages & refPropDataPages)
    :CuIpmPropertyData("CuDomPropDataPages")
{
  m_bMustDeleteArray = TRUE;   // used in destructor

  m_lTotal        = refPropDataPages.m_lTotal        ;
  m_lInUse        = refPropDataPages.m_lInUse        ;
  m_lOverflow     = refPropDataPages.m_lOverflow     ;
  m_lFreed        = refPropDataPages.m_lFreed        ;
  m_lNeverUsed    = refPropDataPages.m_lNeverUsed    ;

  m_lItemsPerByte = refPropDataPages.m_lItemsPerByte ;
  m_nbElements    = refPropDataPages.m_nbElements    ;

  m_lbuckets_no_ovf = refPropDataPages.m_lbuckets_no_ovf;
  m_lemptybuckets_no_ovf = refPropDataPages.m_lemptybuckets_no_ovf;
  m_lbuck_with_ovf = refPropDataPages.m_lbuck_with_ovf;
  m_lemptybuckets_with_ovf = refPropDataPages.m_lemptybuckets_with_ovf;
  m_loverflow_not_empty = refPropDataPages.m_loverflow_not_empty;
  m_loverflow_empty = refPropDataPages.m_loverflow_empty;


  // Need to duplicate array, otherwize tear-out/new window problems at close time
  m_pByteArray = new CByteArray();
  m_pByteArray->SetSize(refPropDataPages.m_pByteArray->GetSize());
  m_pByteArray->Copy(*refPropDataPages.m_pByteArray);

  m_lCurPgPerCell = refPropDataPages.m_lCurPgPerCell ;

  m_bHashed       = refPropDataPages.m_bHashed;

  m_refreshParams = refPropDataPages.m_refreshParams;

  m_llongest_ovf_chain    = refPropDataPages.m_llongest_ovf_chain;
  m_lbuck_longestovfchain = refPropDataPages.m_lbuck_longestovfchain;
  m_favg_ovf_chain        = refPropDataPages.m_favg_ovf_chain;

}

// '=' operator overloading
CuDomPropDataPages CuDomPropDataPages::operator = (const CuDomPropDataPages & refPropDataPages)
{
  if (this == &refPropDataPages)
    ASSERT (FALSE);

  m_lTotal        = refPropDataPages.m_lTotal        ;
  m_lInUse        = refPropDataPages.m_lInUse        ;
  m_lOverflow     = refPropDataPages.m_lOverflow     ;
  m_lFreed        = refPropDataPages.m_lFreed        ;
  m_lNeverUsed    = refPropDataPages.m_lNeverUsed    ;

  m_lItemsPerByte = refPropDataPages.m_lItemsPerByte ;
  m_nbElements    = refPropDataPages.m_nbElements    ;

  // Need to duplicate array, otherwise tear-out/new window problems at close time
  m_pByteArray = new CByteArray();
  m_pByteArray->SetSize(refPropDataPages.m_pByteArray->GetSize());
  m_pByteArray->Copy(*refPropDataPages.m_pByteArray);

  m_lCurPgPerCell = refPropDataPages.m_lCurPgPerCell ;

  m_bHashed       = refPropDataPages.m_bHashed;

  m_refreshParams = refPropDataPages.m_refreshParams;

  m_lbuckets_no_ovf = refPropDataPages.m_lbuckets_no_ovf;
  m_lemptybuckets_no_ovf = refPropDataPages.m_lemptybuckets_no_ovf;
  m_lbuck_with_ovf = refPropDataPages.m_lbuck_with_ovf;
  m_lemptybuckets_with_ovf = refPropDataPages.m_lemptybuckets_with_ovf;
  m_loverflow_not_empty = refPropDataPages.m_loverflow_not_empty;
  m_loverflow_empty = refPropDataPages.m_loverflow_empty;

  m_llongest_ovf_chain    = refPropDataPages.m_llongest_ovf_chain;
  m_lbuck_longestovfchain = refPropDataPages.m_lbuck_longestovfchain;
  m_favg_ovf_chain        = refPropDataPages.m_favg_ovf_chain;

  return *this;
}

CuDomPropDataPages::~CuDomPropDataPages()
{
  // If created with copy constructor or after a load,
  // it is our responsibility to delete pByteArray  in order to avoid free chunks
  if (m_bMustDeleteArray)
    if (m_pByteArray)
      delete m_pByteArray;
}

void CuDomPropDataPages::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataPages::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_lTotal        ;
    ar << m_lInUse        ;
    ar << m_lOverflow     ;
    ar << m_lFreed        ;
    ar << m_lNeverUsed    ;

    ar << m_lItemsPerByte ;
    ar << m_nbElements    ;
    ar << m_pByteArray    ;

    ar << m_lCurPgPerCell ;

    ar << m_bHashed;

    ar << m_lbuckets_no_ovf;
    ar << m_lemptybuckets_no_ovf;
    ar << m_lbuck_with_ovf;
    ar << m_lemptybuckets_with_ovf;
    ar << m_loverflow_not_empty;
    ar << m_loverflow_empty;

    ar << m_llongest_ovf_chain;
    ar << m_lbuck_longestovfchain;
    ar << m_favg_ovf_chain;

  }
  else {
    ar >> m_lTotal        ;
    ar >> m_lInUse        ;
    ar >> m_lOverflow     ;
    ar >> m_lFreed        ;
    ar >> m_lNeverUsed    ;

    ar >> m_lItemsPerByte ;
    ar >> m_nbElements    ;
    ar >> m_pByteArray    ;

    ar >> m_lCurPgPerCell ;

    ar >> m_bHashed;

    ar >> m_lbuckets_no_ovf;
    ar >> m_lemptybuckets_no_ovf;
    ar >> m_lbuck_with_ovf;
    ar >> m_lemptybuckets_with_ovf;
    ar >> m_loverflow_not_empty;
    ar >> m_loverflow_empty;

    ar >> m_llongest_ovf_chain;
    ar >> m_lbuck_longestovfchain;
    ar >> m_favg_ovf_chain;
  }
  m_refreshParams.Serialize(ar);
}

//
// STATIC DATABASES
//

IMPLEMENT_SERIAL (CuDomPropDataStaticDb, CuIpmPropertyData, 2)

CuDomPropDataStaticDb::CuDomPropDataStaticDb()
    :CuIpmPropertyData("CuDomPropDataStaticDb")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataStaticDb::CuDomPropDataStaticDb(const CuDomPropDataStaticDb & refPropDataStaticDb)
    :CuIpmPropertyData("CuDomPropDataStaticDb")
{
  m_uaStaticDb.Copy(refPropDataStaticDb.m_uaStaticDb);
  m_awDbType.Copy(refPropDataStaticDb.m_awDbType);
}

// '=' operator overloading
CuDomPropDataStaticDb CuDomPropDataStaticDb::operator = (const CuDomPropDataStaticDb & refPropDataStaticDb)
{
  if (this == &refPropDataStaticDb)
    ASSERT (FALSE);

  m_uaStaticDb.Copy(refPropDataStaticDb.m_uaStaticDb);
  m_awDbType.Copy(refPropDataStaticDb.m_awDbType);

  return *this;
}

void CuDomPropDataStaticDb::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataStaticDb::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaStaticDb.Serialize(ar);
  m_awDbType.Serialize(ar);
}

//
// STATIC PROFILES
//

IMPLEMENT_SERIAL (CuDomPropDataStaticProfile, CuIpmPropertyData, 1)

CuDomPropDataStaticProfile::CuDomPropDataStaticProfile()
    :CuIpmPropertyData("CuDomPropDataStaticProfile")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataStaticProfile::CuDomPropDataStaticProfile(const CuDomPropDataStaticProfile & refPropDataStaticProfile)
    :CuIpmPropertyData("CuDomPropDataStaticProfile")
{
  m_uaStaticProfile.Copy(refPropDataStaticProfile.m_uaStaticProfile);
}

// '=' operator overloading
CuDomPropDataStaticProfile CuDomPropDataStaticProfile::operator = (const CuDomPropDataStaticProfile & refPropDataStaticProfile)
{
  if (this == &refPropDataStaticProfile)
    ASSERT (FALSE);

  m_uaStaticProfile.Copy(refPropDataStaticProfile.m_uaStaticProfile);

  return *this;
}

void CuDomPropDataStaticProfile::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataStaticProfile::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaStaticProfile.Serialize(ar);
}

//
// STATIC USERS
//

IMPLEMENT_SERIAL (CuDomPropDataStaticUser, CuIpmPropertyData, 1)

CuDomPropDataStaticUser::CuDomPropDataStaticUser()
    :CuIpmPropertyData("CuDomPropDataStaticUser")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataStaticUser::CuDomPropDataStaticUser(const CuDomPropDataStaticUser & refPropDataStaticUser)
    :CuIpmPropertyData("CuDomPropDataStaticUser")
{
  m_uaStaticUser.Copy(refPropDataStaticUser.m_uaStaticUser);
}

// '=' operator overloading
CuDomPropDataStaticUser CuDomPropDataStaticUser::operator = (const CuDomPropDataStaticUser & refPropDataStaticUser)
{
  if (this == &refPropDataStaticUser)
    ASSERT (FALSE);

  m_uaStaticUser.Copy(refPropDataStaticUser.m_uaStaticUser);

  return *this;
}

void CuDomPropDataStaticUser::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataStaticUser::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaStaticUser.Serialize(ar);
}

//
// STATIC GROUPS
//

IMPLEMENT_SERIAL (CuDomPropDataStaticGroup, CuIpmPropertyData, 1)

CuDomPropDataStaticGroup::CuDomPropDataStaticGroup()
    :CuIpmPropertyData("CuDomPropDataStaticGroup")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataStaticGroup::CuDomPropDataStaticGroup(const CuDomPropDataStaticGroup & refPropDataStaticGroup)
    :CuIpmPropertyData("CuDomPropDataStaticGroup")
{
  m_uaStaticGroup.Copy(refPropDataStaticGroup.m_uaStaticGroup);
}

// '=' operator overloading
CuDomPropDataStaticGroup CuDomPropDataStaticGroup::operator = (const CuDomPropDataStaticGroup & refPropDataStaticGroup)
{
  if (this == &refPropDataStaticGroup)
    ASSERT (FALSE);

  m_uaStaticGroup.Copy(refPropDataStaticGroup.m_uaStaticGroup);

  return *this;
}

void CuDomPropDataStaticGroup::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataStaticGroup::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaStaticGroup.Serialize(ar);
}

//
// STATIC ROLES
//

IMPLEMENT_SERIAL (CuDomPropDataStaticRole, CuIpmPropertyData, 1)

CuDomPropDataStaticRole::CuDomPropDataStaticRole()
    :CuIpmPropertyData("CuDomPropDataStaticRole")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataStaticRole::CuDomPropDataStaticRole(const CuDomPropDataStaticRole & refPropDataStaticRole)
    :CuIpmPropertyData("CuDomPropDataStaticRole")
{
  m_uaStaticRole.Copy(refPropDataStaticRole.m_uaStaticRole);
}

// '=' operator overloading
CuDomPropDataStaticRole CuDomPropDataStaticRole::operator = (const CuDomPropDataStaticRole & refPropDataStaticRole)
{
  if (this == &refPropDataStaticRole)
    ASSERT (FALSE);

  m_uaStaticRole.Copy(refPropDataStaticRole.m_uaStaticRole);

  return *this;
}

void CuDomPropDataStaticRole::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataStaticRole::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaStaticRole.Serialize(ar);
}

//
// STATIC LOCATIONS
//

IMPLEMENT_SERIAL (CuDomPropDataStaticLocation, CuIpmPropertyData, 1)

CuDomPropDataStaticLocation::CuDomPropDataStaticLocation()
    :CuIpmPropertyData("CuDomPropDataStaticLocation")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataStaticLocation::CuDomPropDataStaticLocation(const CuDomPropDataStaticLocation & refPropDataStaticLocation)
    :CuIpmPropertyData("CuDomPropDataStaticLocation")
{
  m_uaStaticLocation.Copy(refPropDataStaticLocation.m_uaStaticLocation);
}

// '=' operator overloading
CuDomPropDataStaticLocation CuDomPropDataStaticLocation::operator = (const CuDomPropDataStaticLocation & refPropDataStaticLocation)
{
  if (this == &refPropDataStaticLocation)
    ASSERT (FALSE);

  m_uaStaticLocation.Copy(refPropDataStaticLocation.m_uaStaticLocation);

  return *this;
}

void CuDomPropDataStaticLocation::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataStaticLocation::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaStaticLocation.Serialize(ar);
}


//
// STATIC DATABASES ON GATEWAY ( IDMS, DATACOMM, ETC... )
//

IMPLEMENT_SERIAL (CuDomPropDataStaticGwDb, CuIpmPropertyData, 1)

CuDomPropDataStaticGwDb::CuDomPropDataStaticGwDb()
    :CuIpmPropertyData("CuDomPropDataStaticGwDb")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataStaticGwDb::CuDomPropDataStaticGwDb(const CuDomPropDataStaticGwDb & refPropDataStaticGwDb)
    :CuIpmPropertyData("CuDomPropDataStaticGwDb")
{
  m_uaStaticGwDb.Copy(refPropDataStaticGwDb.m_uaStaticGwDb);
}

// '=' operator overloading
CuDomPropDataStaticGwDb CuDomPropDataStaticGwDb::operator = (const CuDomPropDataStaticGwDb & refPropDataStaticGwDb)
{
  if (this == &refPropDataStaticGwDb)
    ASSERT (FALSE);

  m_uaStaticGwDb.Copy(refPropDataStaticGwDb.m_uaStaticGwDb);

  return *this;
}

void CuDomPropDataStaticGwDb::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataStaticGwDb::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaStaticGwDb.Serialize(ar);
}

