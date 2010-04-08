/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : DomRfsh.h                                                             //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
// Utility classes for dom refresh params management of detail panes                   //
//                                                                                     //
****************************************************************************************/

#ifndef DOMREFRESH_HEADER
#define DOMREFRESH_HEADER

extern "C" {
#include "dbatime.h"
}

//////////////////////////////////////////////////////////////////
BOOL CALLBACK EnumUpdateChildProc(HWND hwnd, LPARAM lParam);
void RedrawAllChildWindows(HWND hwnd);

class CuDomRefreshParams: public CObject
{
  DECLARE_SERIAL (CuDomRefreshParams)

public:
  CuDomRefreshParams(int type);
  virtual ~CuDomRefreshParams() {}

  // copy constructor and '=' operator overloading
  CuDomRefreshParams(const CuDomRefreshParams& refRefreshParams);
  CuDomRefreshParams operator = (const CuDomRefreshParams & refPropDataUser);

  void InitializeRefreshParamsType(int type);
  void UpdateRefreshParams();
  BOOL MustRefresh(BOOL bOnLoad, time_t refreshtime);

// for serialisation
  CuDomRefreshParams();
  virtual void Serialize (CArchive& ar);

private:
  int m_type;
  DOMREFRESHPARAMS m_sDomRefreshParams;
};

#endif  // DOMREFRESH_HEADER
