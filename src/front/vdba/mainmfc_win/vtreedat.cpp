// vTreeDat.cpp - Emmanuel Blattes - started Feb. 7, 97
// 

#include "stdafx.h"

#include "vtreedat.h"

extern "C" {
#include "monitor.h"
};

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// declaration for serialization
IMPLEMENT_SERIAL ( CTreeItemData, CObject, 3)

// "normal" constructor
CTreeItemData::CTreeItemData(void *pstruct, int structsize, int hNode, CString name)
{
  ASSERT (pstruct);
  ASSERT (structsize);

  m_name = name;
  m_size = structsize;
  m_pstruct = NULL;
  if (pstruct && structsize) {
    m_pstruct = new char[structsize];
    ASSERT(m_pstruct);
    if (m_pstruct)
      memcpy(m_pstruct, pstruct, structsize);
  }
  m_hNode = hNode;
}

// copy constructor
CTreeItemData::CTreeItemData(const CTreeItemData &itemData)
{
  m_size = itemData.m_size;
  m_pstruct = new char[m_size];
  ASSERT(m_pstruct);
  if (m_pstruct)
    memcpy(m_pstruct, itemData.m_pstruct, m_size);
  m_hNode = itemData.m_hNode;
  m_name  = itemData.m_name;
}

// assignment operator
CTreeItemData &CTreeItemData::operator=( CTreeItemData &itemData )
{
  m_size = itemData.m_size;
  m_pstruct = new char[m_size];
  ASSERT(m_pstruct);
  if (m_pstruct)
    memcpy(m_pstruct, itemData.m_pstruct, m_size);
  m_hNode = itemData.m_hNode;
  m_name  = itemData.m_name;

  return *this;  // Assignment operator returns left side.
}
  
// serialization constructor
CTreeItemData::CTreeItemData()
{
}

// destructor
CTreeItemData::~CTreeItemData()
{
  if (m_pstruct)
    delete m_pstruct;
}

// serialization method
void CTreeItemData::Serialize (CArchive& ar)
{
  if (ar.IsStoring()) {
    ar << m_hNode;
    ar << m_size;
    ar.Write(m_pstruct, m_size);
    ar << m_name;
  }
  else {
    ar >> m_hNode;
    ar >> m_size;
    m_pstruct = new char[m_size];   // will throw a memory exception if out of memory
    ar.Read(m_pstruct, m_size);
    ar >> m_name;
  }
}


#include "vtree3.h"

CString CTreeItemData::GetItemName(int objType)
{
  return m_name;

/********************
  // Was really difficult to manage this for star tree! architecture intensively revized.
  // Old code :
  //
  //ASSERT(m_size == size);
  //
  //char    name[MAXMONINFONAME];
  //GetMonInfoName(GetHNode(),objType, m_pstruct, name, sizeof(name));
  //CString zeName = name;
  //return zeName;

  // trial replacement code, didn't work for OT_NODE which can be both star and non star
  int size = 0;
  if (objType != OT_VIEW && objType != OT_PROCEDURE)
    size = GetMonInfoStructSize(objType);

  if (m_size == size) {
    ASSERT (FALSE);		// Obsolete car déplacé - 
    char    name[MAXMONINFONAME];
    GetMonInfoName(GetHNode(),objType, m_pstruct, name, sizeof(name));
    CString zeName = name;
    return zeName;
  }
  else {
    LPSTARITEM pStar = (LPSTARITEM)m_pstruct;
    CString zeName = pStar->objName;
    return zeName;
  }
  ***************/
}

// replace structure "as is" - no delete/new to keep same address,
// which is needed for right pane refresh in monitor window
BOOL CTreeItemData::SetNewStructure(void *newpstruct)
{
  ASSERT (newpstruct);
  ASSERT (m_size);
  ASSERT(m_pstruct);

  if (!newpstruct)
    return FALSE;
  if (!m_size)
    return FALSE;
  if (!m_pstruct)
    return FALSE;

  memcpy(m_pstruct, newpstruct, m_size);
  return TRUE;
}

