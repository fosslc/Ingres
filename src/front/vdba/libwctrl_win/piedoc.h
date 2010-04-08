/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : piedoc.h : Header file.  (Document Window) 
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Document for Drawing the statistic pie
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
*/


#ifndef PIECHARTDOC_HEADER
#define PIECHARTDOC_HEADER
#include "statfrm.h"

class CdStatisticPieChartDoc : public CDocument
{
	DECLARE_DYNCREATE(CdStatisticPieChartDoc)
public:
	CdStatisticPieChartDoc();
	void SetPieInfo (CaPieInfoData* pPie);
	CaPieInfoData* GetPieInfo() {return m_pPieInfo;}

	//
	// Common data for Pie & Chart:
	CaPieChartCreationData m_PieChartCreationData;

	//
	// Specific Data for Pie:
	CaPieInfoData* m_pPieInfo;

	//
	// Specific Data for Chart:
	CaBarInfoData* m_pBarInfo;
	void SetDiskInfo (CaBarInfoData* pBarInfo);
	CaBarInfoData* GetDiskInfo (){return m_pBarInfo;}


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdStatisticPieChartDoc)
	public:
	virtual void Serialize(CArchive& ar);	// overridden for document i/o
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CdStatisticPieChartDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CdStatisticPieChartDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // PIECHARTDOC_HEADER
