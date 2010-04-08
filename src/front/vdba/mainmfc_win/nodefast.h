// nodefast.h   : header file for fast access to an item in the nodes window,
//                for unicenter-driven feature
//

#ifndef NODEFAST_INCLUDED
#define NODEFAST_INCLUDED

#include "resmfc.h"

class CuNodeTreeFastItem: public CObject
{
public:
  CuNodeTreeFastItem(BOOL bStatic, int type, LPCTSTR lpszName = NULL);
  virtual ~CuNodeTreeFastItem();

protected:
  // CuNodeTreeFastItem();    // serialization
  // DECLARE_SERIAL (CuNodeTreeFastItem)

public:
  BOOL    GetBStatic() { return m_bStatic; }
  int     GetType()    { return m_type; }
  CString GetName()    { return m_csName; }

private:
  BOOL    m_bStatic;
  int     m_type;
  CString m_csName;
};

BOOL ExpandAndSelectUpToSearchedItem(CMainFrame* pMainFrame, CTypedPtrList<CObList, CuNodeTreeFastItem*>& rNodeTreeFastItemList);

#endif  // NODEFAST_INCLUDED
