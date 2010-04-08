/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : statfrm.h, Header file.  (Frame Window)
**    Project  : CA-OpenIngres/Monitoring
**    Author   : Uk Sotheavut (uk$so01)
**    Purpose  : Drawing the statistic bar
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
** 22-Aug-2008 (whiro01)
**    Removed redundant <afx...> include which is already included in every "stdafx.h"
*/


#ifndef STATFRM_HEADER
#define STATFRM_HEADER

//
// Location definition
class CaBarPieItem: public CObject
{
	DECLARE_SERIAL (CaBarPieItem)
public:
	CaBarPieItem ();
	CaBarPieItem (LPCTSTR lpszBar, COLORREF color);
	CaBarPieItem (const CaBarPieItem* pRefLocationData);
	CaBarPieItem (LPCTSTR lpszBar, COLORREF color, BOOL bHatch, COLORREF colorHatch);

	virtual ~CaBarPieItem(){}
	virtual void Serialize (CArchive& ar);
	CString GetName(){return m_strName;}

	CString  m_strName;      // Location Name
	COLORREF m_colorRGB;     // Color of the location

	BOOL m_bHatch;
	COLORREF m_colorHatch;
};


//
// Occupant definition
class CaOccupantData: public CaBarPieItem
{
	DECLARE_SERIAL (CaOccupantData)
public:
	CaOccupantData ();
	CaOccupantData (LPCTSTR lpszOccupant, COLORREF color, double fPercentage);
	CaOccupantData (LPCTSTR lpszOccupant, COLORREF color, double fPercentage, BOOL bHatch, COLORREF colorHatch);
	CaOccupantData (const CaOccupantData* pRefOccupantData);
	virtual ~CaOccupantData(){}
	virtual void Serialize (CArchive& ar);

	double m_fPercentage;
	CRect  m_rcOccupant;
	BOOL   m_bFocus;
};


//
// BAR Definition
class CaBarData: public CObject
{
	DECLARE_SERIAL (CaBarData)
public:
	CaBarData();
	CaBarData(LPCTSTR lpszBarUnit, double dCapacity);
	CaBarData(const CaBarData* pRefBarData);
	virtual ~CaBarData();
	virtual void Serialize (CArchive& ar);

	BOOL    m_bFocus;
	CRect   m_rcCaption;
	CRect   m_rcBarUnit;
	CString m_strBarUnit;  // Ex: For the Disk Unit it can be C, D, E
	double  m_nCapacity;   // Ex: Storage capacity in Mega Bytes (Mb)
	CTypedPtrList<CObList, CaOccupantData*> m_listOccupant; // List of Occupants of the Bar

};

//
// Disk Space Statistic of Occupation
// **********************************
// Usage:
//    1) Construct the Object.
//    2) Call the member create, after that, the object is ready to be used.
// 
class CaBarInfoData: public CObject
{
	DECLARE_SERIAL (CaBarInfoData)
public:
	CaBarInfoData();
	CaBarInfoData(const CaBarInfoData* pRefDiskInfoData);
	virtual ~CaBarInfoData();
	virtual void Serialize (CArchive& ar);
	double GetMaxCapacity ();
	UINT   GetBarUnitCount ()    {return m_listUnit.GetCount();};
	UINT   GetBarCount (){return m_listBar.GetCount();};
	void   Cleanup();
	CTypedPtrList<CObList, CaBarPieItem*>& GetListBar(){return m_listBar;}
	//
	// All function Add ... will throw CMemoryException, when allocating
	// memory failed.
	void AddBar (LPCTSTR lpszBar);
	void AddBar (LPCTSTR lpszBar, COLORREF color);
	void AddBar (LPCTSTR lpszBar, COLORREF color, BOOL bHatch, COLORREF colorHatch);

	void AddBarUnit (LPCTSTR lpszBar, double dCapacity);
	void AddOccupant (LPCTSTR lpszDisk, LPCTSTR lpszOccupant, double fPercentage);
	
	void DropBar(LPCTSTR lpszBar);
	void DropBarUnit(LPCTSTR lpszDisk);
	void DropOccupant(LPCTSTR lpszDisk, LPCTSTR lpszOccupant);
	
	void SetTrackerSelectObject (CRect rcObject, BOOL bSelect = TRUE);
	double GetBarCapacity(LPCTSTR lpszDisk);
	//
	// Call this member to create the list of locations and
	// the list of Disk Unit and Initialize other data.
	BOOL Create( int nNodeHandle );
	
	CTypedPtrList<CObList, CaBarPieItem*> m_listBar; // List of bar
	CTypedPtrList<CObList, CaBarData*>    m_listUnit;// List of bar Units
	
	CRectTracker m_Tracker;
	CString m_strBarCaption; // Caption text displayed under each bar.
	CString m_strUnit;       // Unit text drawn at the top of the vertical axis.
	CString m_strSmallUnit;
};




//
// A pie is defined by:
//  - Name (Legend)
//  - Percentage
//  - Color.
class CaLegendData: public CaBarPieItem
{
	DECLARE_SERIAL (CaLegendData)
public:
	CaLegendData (): CaBarPieItem(), m_bFocus(FALSE) {}
	CaLegendData (LPCTSTR lpszPie, double dPercentage, COLORREF color);
	CaLegendData (LPCTSTR lpszLegend, COLORREF color);

	CaLegendData (const CaLegendData* refLegendData);
	
	virtual ~CaLegendData(){}
	virtual void Serialize (CArchive& ar);
	
	void SetSector  (double degStart, double degEnd, BOOL bAssert = TRUE);
	double m_dPercentage;
	double m_dValue;      // m_dPercentage * Capacity
	double m_dDegStart;
	double m_dDegEnd;
	
	BOOL   m_bFocus;
};


//
// Pie Chart Statistic
// *******************
// Usage:
//    1) Construct the Object.
//    2) Call the member create, after that, the object is ready to be used.
// 
// Implementation:
//    A pie of 1% is equivalent to a sector of an ellipse of 3.6 degrees.
//
class CaPieInfoData: public CObject
{
	DECLARE_SERIAL (CaPieInfoData)
public:
	CaPieInfoData();
	CaPieInfoData(double dCapacity, LPCTSTR lpszUnit);
	CaPieInfoData(const CaPieInfoData* pRefPieInfoData);
	virtual ~CaPieInfoData();
	virtual void Serialize (CArchive& ar);
	UINT    GetPieCount () {return m_listPie.GetCount();};
	void    Cleanup();
	void    SetCapacity(double dCapacity, LPCTSTR lpszUnit);
	double  GetCapacity(){return m_dCapacity;}
	CString GetUnit()    {return m_strUnit;}
	void    SetTitle(LPCTSTR lpszTitle, LPCTSTR lpszMainItem = NULL) 
	{
		m_strTitle = lpszTitle;
		if (lpszMainItem)
			m_strMainItem = lpszMainItem;
	}
	void SetSmallUnit(LPCTSTR lpszUnit){m_strSmallUnit = lpszUnit;} // Ex: _T("Kb");
	void SetError(BOOL bSet){m_bError = bSet;}
	BOOL GetError(){return m_bError;}

	CTypedPtrList<CObList, CaLegendData*>& GetListLegend(){return m_listLegend;}

	//
	// All function Add ... will throw CMemoryException, when allocating
	// memory failed.
	void AddPie    (LPCTSTR lpszPie,    double dPercentage);
	void AddPie    (LPCTSTR lpszPie,    double dPercentage, COLORREF color);
	//
	// AddPie for Log File (Circular Buffer)
	void AddPie    (LPCTSTR lpszPie,    double dPercentage, COLORREF color, double dDegStart);

	void AddPie2    (LPCTSTR lpszPie,    double dPercentage, double dValue);
	void AddPie2    (LPCTSTR lpszPie,    double dPercentage, COLORREF color, double dValue);

	void AddLegend (LPCTSTR lpszLegend, double dPercentage = 0.0, double dValue = 0.0);
	void AddLegend (LPCTSTR lpszLegend, COLORREF color, double dPercentage = 0.0, double dValue = 0.0);

	double  m_dCapacity;
	CString m_strUnit;
	CString m_strSmallUnit;
	CString m_strTitle;
	CString m_strMainItem;
	CString m_strFormat;
	BOOL    m_bError;
	//
	// Specific for Log File:
	BOOL m_bDrawOrientation;
	double m_degStartNextCP;
	int m_nBlockNextCP;
	int m_nBlockPrevCP;
	int m_nBlockBOF;
	int m_nBlockEOF;
	int m_nBlockTotal;
	//
	// Remark: Some elements of m_listLegend might not in the list m_listPie.
	CTypedPtrList<CObList, CaLegendData*> m_listLegend;     // List of legends,
	CTypedPtrList<CObList, CaLegendData*> m_listPie;        // List of locations, and Other.
};


//
// Creation Data (CaPieChartCreationData)
// ************************************************************************************************
typedef BOOL (CALLBACK* PfnCreatePalette)(CPalette* pPalette);
class CaPieChartCreationData: public CObject
{
public:
	CaPieChartCreationData();
	CaPieChartCreationData(const CaPieChartCreationData& c)
	{
		Copy (c);
	}

	CaPieChartCreationData operator = (const CaPieChartCreationData& c);
	void Copy (const CaPieChartCreationData& c);
	virtual ~CaPieChartCreationData(){}

	BOOL m_bDBLocation;     // Use for database location (TRUE by default);

	BOOL m_bUseLegend;      // Control contains the list of legends (default)
	BOOL m_bLegendRight;    // Default (FALSE), legend at bottom
	BOOL m_bUseLButtonDown; // Default (TRUE), show popup info on LButtonDown

	//
	// Note: If both m_bShowPercentage and m_bShowValue are true
	// The legend item will be shown as: item (percent%, value).
	BOOL m_bShowPercentage; // Default (FALSE), the legend has the percentage at the right.
	BOOL m_bShowValue;      // Default (FALSE), the legend has the value at the right.
	BOOL m_bValueInteger;   // Default (FALSE), If true, the double value will be casted to integer.
	//
	// The control uses these members to convert the width or height of the list of legends
	// into pixels:
	int  m_nLines;          // When m_bLegendRight = FALSE, number of lines in the list of legends
	int  m_nColumns;        // When m_bLegendRight = TRUE, number of columns in the list of legends

	//
	// For bars only:
	BOOL m_b3D;             // Default = FALSE
	BOOL m_bShowBarTitle;   // Default = TRUE;
	BOOL m_bDrawAxis;       // Default = TRUE;
	CString m_strBarWidth;  // Default = _T("EEEE"). Bar width =  text extend cx of "EEEE".
	//
	// Free color, need if the sum (stacks) < total (bar capacity)
	// Hint to draw the stacks !. Draw the initial bar with color 'm_crFree'.
	// Then all the successive stacks will be drawn on the initial bar and arase
	// some parts of free area:
	COLORREF m_crFree;
	CPalette* m_pPalette;

	//
	// User can specify the use of specific palette by giving
	// the function that create the palette:
	PfnCreatePalette m_pfnCreatePalette;

	//
	// String IDS:
	// User can pass the ids for the resource strings:
	UINT m_uIDS_IncreasePaneSize;  // "Please increase pane size"
	// 
	// m_uIDS_PieIsEmpty. When there is no pie to draw. Default string should be "<Name> %1 is empty."
	//                    Ex: "Location %1 is empty". (only if m_bDBLocation = TRUE)
	UINT m_uIDS_PieIsEmpty;
	// 
	// m_uIDS_LegendCount.When the list of legends is empty. Default string should be "No <Name> in <Item> %1
	//                    Ex: "No Databases in Location %1". (only if m_bDBLocation = TRUE)
	UINT m_uIDS_LegendCount;

};

//
// Legeng Control (CListBox)
// ************************************************************************************************
class CuStatisticBarLegendList : public CListBox
{
public:
	CuStatisticBarLegendList();
	int AddItem (LPCTSTR lpszItem, COLORREF color);
	//
	// dwItemData must be a pointer to CaBarPieItem
	int AddItem2(LPCTSTR lpszItem, DWORD dwItemData);
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuStatisticBarLegendList)
	public:
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL
	
	afx_msg void OnSysColorChange();

	// Implementation
public:
	virtual ~CuStatisticBarLegendList();

	// Generated message map functions
protected:
	int m_nXHatchWidth;
	int m_nXHatchHeight;
	BOOL m_bHatch;
	int  m_nRectColorWidth;
	COLORREF m_colorText;
	COLORREF m_colorTextBk;

	//{{AFX_MSG(CuStatisticBarLegendList)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};



//
// Frame Window (CfStatisticFrame derived fromCFrameWnd)
// ************************************************************************************************
class CfStatisticFrame : public CFrameWnd
{
public:
	CfStatisticFrame(int nType);

	void Refresh (LPCTSTR strT);
	void DrawLegend (CuStatisticBarLegendList* pList, BOOL bShowPercentage = FALSE);
	CDocument* GetDoc (){return GetActiveDocument();}
	enum {nTypeBar = 1, nTypePie};

	BOOL Create(CWnd* pParent, CRect r, CaPieChartCreationData* pCreationData);
	void SetPieInformation(CaPieInfoData* pData);
	void SetBarInformation(CaBarInfoData* pData);
	CaPieInfoData* GetPieInformation();
	CaBarInfoData* GetBarInformation();
	void UpdateLegend();
	void UpdatePieChart();
	BOOL IsLegendUsed();
	CWnd* GetCurrentView();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfStatisticFrame)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	int m_nType;
	CDocument* m_pPieChartDoc;
	CSplitterWnd* m_pSplitter;
	CView* m_pViewLegend;
	BOOL m_bAllCreated;
	virtual ~CfStatisticFrame();
	void ResizePanes();

	// Generated message map functions
	//{{AFX_MSG(CfStatisticFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // STATFRM_HEADER

