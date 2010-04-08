// DomPgInf.h: interface for the CuDomPageInformation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DOMPGINF_H__32445501_663E_11D1_A326_00609725DBFA__INCLUDED_)
#define AFX_DOMPGINF_H__32445501_663E_11D1_A326_00609725DBFA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "ipmdisp.h"

class CuDomUpdateParam: public CObject
{

public:
  CuDomUpdateParam();
  CuDomUpdateParam(int testDummy);
  CuDomUpdateParam(const CuDomUpdateParam& refParam);
  CuDomUpdateParam& operator= (const CuDomUpdateParam& refParam);
  virtual ~CuDomUpdateParam();

  virtual void Serialize (CArchive& ar);

  DECLARE_SERIAL (CuDomUpdateParam)

private:
  int m_testDummy;
};

class CuDomPageInformation : public CuPageInformation
{
  DECLARE_SERIAL (CuDomPageInformation)

public:
  CuDomPageInformation();
  CuDomPageInformation(CString strClass, int nSize, UINT* tabArray,UINT* dlgArray, UINT nIDTitle = 0);
  CuDomPageInformation(CString strClass);
	virtual ~CuDomPageInformation();

  // new data for dom
  void SetDomUpdateParam (CuDomUpdateParam DomUpdateParam);
  CuDomUpdateParam GetDomUpdateParam (){return m_DomUpdateParam;}

  virtual void Serialize (CArchive& ar);

  // virtual int MysteriousCrash() { return -1; }

private:
  CuDomUpdateParam m_DomUpdateParam;  // Utility questionable
};

class CuDomProperty: public CuIpmProperty
{
public:
    CuDomProperty(int nCurSel, CuDomPageInformation*  pPageInfo);
    ~CuDomProperty();

    void SetDomPageInfo (CuDomPageInformation*  pPageInfo)  {m_pDomPageInfo = pPageInfo;}
    CuDomPageInformation* GetDomPageInfo()  { return m_pDomPageInfo; }

    virtual void Serialize (CArchive& ar);

protected:
    DECLARE_SERIAL (CuDomProperty)
    CuDomProperty();

private:
    CuDomPageInformation*  m_pDomPageInfo;    // Current object displayed in the TabCtrl.
};


#endif // !defined(AFX_DOMPGINF_H__32445501_663E_11D1_A326_00609725DBFA__INCLUDED_)
