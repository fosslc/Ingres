// DomPgInf.cpp: implementation of custom classes for dom splitted right pane
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "mainmfc.h"
#include "dompginf.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// CuDomUpdateParam
//

IMPLEMENT_SERIAL (CuDomUpdateParam, CObject, 1)

CuDomUpdateParam::CuDomUpdateParam()
{
  m_testDummy = 0;
}

CuDomUpdateParam::CuDomUpdateParam(int testDummy)
{
  m_testDummy = testDummy;
}

CuDomUpdateParam::CuDomUpdateParam(const CuDomUpdateParam& refParam)
{
  m_testDummy = refParam.m_testDummy;
}

CuDomUpdateParam& CuDomUpdateParam::operator= (const CuDomUpdateParam& refParam)
{
  m_testDummy = refParam.m_testDummy;
  return *this;
}

CuDomUpdateParam::~CuDomUpdateParam()
{
}

void CuDomUpdateParam::Serialize (CArchive& ar)
{
  if (ar.IsStoring()) {
    ar << m_testDummy;
  }
  else {
    ar >> m_testDummy;
  }
}


//////////////////////////////////////////////////////////////////////
// CuDomPageInformation Implementation
//

IMPLEMENT_SERIAL (CuDomPageInformation, CuPageInformation, 1)

CuDomPageInformation::CuDomPageInformation()
  :CuPageInformation()
{
}

CuDomPageInformation::CuDomPageInformation(CString strClass, int nSize, UINT* tabArray, UINT* dlgArray, UINT nIDTitle)
  :CuPageInformation(strClass, nSize, tabArray, dlgArray, nIDTitle)
{
}


CuDomPageInformation::CuDomPageInformation(CString strClass)
  :CuPageInformation(strClass)
{
}
    
CuDomPageInformation::~CuDomPageInformation()
{
}

void CuDomPageInformation::SetDomUpdateParam (CuDomUpdateParam DomUpdateParam)
{
  m_DomUpdateParam = DomUpdateParam;
}


void CuDomPageInformation::Serialize (CArchive& ar)
{
  m_DomUpdateParam.Serialize(ar);

  CuPageInformation::Serialize(ar);
}


//////////////////////////////////////////////////////////////////////
// CuDomProperty Implementation
//

IMPLEMENT_SERIAL (CuDomProperty, CuIpmProperty, 2)

CuDomProperty::CuDomProperty()
  :CuIpmProperty()
{
  m_pDomPageInfo = NULL;
}

CuDomProperty::CuDomProperty(int nCurSel, CuDomPageInformation*  pPageInfo)
  :CuIpmProperty(nCurSel, NULL)
{
  // Null page info for CuIpmProperty,
  // since useful pointer is m_pDomPageInformation
  m_pDomPageInfo = pPageInfo;
}

CuDomProperty::~CuDomProperty()
{
}

void CuDomProperty::Serialize (CArchive& ar)
{
  CuIpmProperty::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_pDomPageInfo;
  }
  else {
    ar >> m_pDomPageInfo;
  }
}
