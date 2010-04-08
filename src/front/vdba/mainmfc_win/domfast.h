// domfast.h   : header file for fast access to an item.
//               thanks to double-click in right pane changing selection in the dom tree
//

#ifndef DOMFAST_INCLUDED
#define DOMFAST_INCLUDED

#include "childfrm.h"

class CuDomTreeFastItem: public CObject
{
public:
  CuDomTreeFastItem(int type, LPCTSTR szName = NULL, LPCTSTR szOwner = NULL);
  virtual ~CuDomTreeFastItem();

protected:
  // CuDomTreeFastItem();    // serialization
  // DECLARE_SERIAL (CuDomTreeFastItem)

public:
  int     GetType()   { return m_type; }
  LPCTSTR GetName()   { return m_csName;  }
  LPCTSTR GetOwner()  { return m_csOwner; }

private:
  int m_type;
  CString m_csName;
  CString m_csOwner;
};

BOOL ExpandUpToSearchedItem(CWnd* pPropWnd, CTypedPtrList<CObList, CuDomTreeFastItem*>& rDomTreeFastItemList, BOOL bAutomated = FALSE);

#endif  // DOMFAST_INCLUDED
