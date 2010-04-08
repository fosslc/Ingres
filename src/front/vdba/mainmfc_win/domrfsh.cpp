/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DomRfsh.cpp                                                           //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
// Utility classes for dom refresh params management of detail panes                   //
//                                                                                     //
****************************************************************************************/

#include "stdafx.h"

#include "domrfsh.h"

extern "C" {
#include "settings.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SERIAL (CuDomRefreshParams, CObject, 1)

CuDomRefreshParams::CuDomRefreshParams()
{
  m_type = -1;
  memset (&m_sDomRefreshParams, 0, sizeof(m_sDomRefreshParams));
}

CuDomRefreshParams::CuDomRefreshParams(int type)
{
  m_type = type;
  memset (&m_sDomRefreshParams, 0, sizeof(m_sDomRefreshParams));
}

CuDomRefreshParams::CuDomRefreshParams(const CuDomRefreshParams & refRefreshParams)
{
  m_type = refRefreshParams.m_type;
  memcpy (&m_sDomRefreshParams, &(refRefreshParams.m_sDomRefreshParams), sizeof(m_sDomRefreshParams));
}

// '=' operator overloading
CuDomRefreshParams CuDomRefreshParams::operator = (const CuDomRefreshParams & refRefreshParams)
{
  if (this == &refRefreshParams)
    ASSERT (FALSE);

  m_type = refRefreshParams.m_type;
  memcpy (&m_sDomRefreshParams, &(refRefreshParams.m_sDomRefreshParams), sizeof(m_sDomRefreshParams));

  return *this;
}

void CuDomRefreshParams::Serialize (CArchive& ar)
{
  CObject::Serialize(ar);

  if (ar.IsStoring()) {
    ASSERT (m_type != -1);
    ar << m_type;
    ar.Write(&m_sDomRefreshParams, sizeof(m_sDomRefreshParams));
  }
  else {
    ar >> m_type;
    ar.Read(&m_sDomRefreshParams, sizeof(m_sDomRefreshParams));
  }
}

void CuDomRefreshParams::InitializeRefreshParamsType(int type)
{
  ASSERT (type != -1);
  m_type = type;
}

void CuDomRefreshParams::UpdateRefreshParams()
{
  ASSERT (m_type != -1);

  UpdRefreshParams(&m_sDomRefreshParams, m_type);
}

BOOL CuDomRefreshParams::MustRefresh(BOOL bOnLoad, time_t refreshtime)
{
  if (m_type == -1) {
    ASSERT (FALSE);
    return FALSE;       // Not initialized yet
  }

  // If type null, means data unavailable ---> do NOT call time elapse functions, always say no need to refresh
  if (m_sDomRefreshParams.iobjecttype == 0)
    return FALSE;

  ASSERT (m_sDomRefreshParams.iobjecttype == m_type);

  BOOL bMustRefresh = FALSE;

  // extracted from fnn's function RefreshPropWindows(), previously in dom3.c
  if (!bOnLoad && DOMIsTimeElapsed(refreshtime, &m_sDomRefreshParams, FALSE))
    bMustRefresh = TRUE;
  if (bOnLoad) {
    LPFREQUENCY lpfreq = GetSettingsFrequencyParams(m_sDomRefreshParams.iobjecttype);
    if (lpfreq->bOnLoad)
      bMustRefresh = TRUE;
  }

  return bMustRefresh;
}

BOOL CALLBACK EnumUpdateChildProc(HWND hwnd, LPARAM lParam)
{
  UpdateWindow(hwnd);
  return TRUE;
}

// Force immediate update of all children controls
void RedrawAllChildWindows(HWND hwnd)
{
  // In case of hidden controls in the property page
  UpdateWindow(hwnd);

  BOOL bSuccess = EnumChildWindows(hwnd, EnumUpdateChildProc, 0L);
  ASSERT (bSuccess);
}
