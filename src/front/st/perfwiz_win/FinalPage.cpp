/*
**
**  Name: FinalPage.cpp
**
**  Description:
**	This file contains the routines necessary for the Final Page Property
**	Page, which allows users to select counters to watch.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
**	15-oct-1999 (somsa01)
**	    Moved selection of new chart file name to here from the Intro
**	    Property Page. Also, the Server selection page is now the final
**	    page. Also, repaired OnButtonDeselect() and OnButtonSelect() to
**	    work properly. Also, in CreatePerfChart(), the chart file must
**	    be opened in binary mode, and properly set the default scale
**	    value.
**	21-oct-1999 (somsa01)
**	    Moved the actual creation of the chart file to the server selection
**	    page as the final step.
**	29-oct-1999 (somsa01)
**	    Added option for choosing personal counters.
**	28-jan-2000 (somsa01)
**	    Added Sleep() call while waiting for threads.
**	25-oct-2001 (somsa01)
**	    In OnWizardNext(), if we are supplied with a chart name which is
**	    not an absolute path, default the path to "My Documents".
**	03-dec-2001 (somsa01)
**	    In WIN32 build environments, UINT_PTR is defined to be
**	    "unsigned long" rather than "unsigned int". Therefore, use
**	    UINT_PTR in WIN64 build environments and UINT32 in WIN32 build
**	    environments.
**	07-dec-2001 (somsa01)
**	    Cleaned up some Win64 compiler warnings.
**	16-oct-2002 (somsa01)
**	    After retrieving the chart name, make sure we append the .pmc
**	    extension to the name, if it needs it.
**	07-May-2008 (drivi01)
**	    Apply Wizard97 template to this utility.
*/

#include "stdafx.h"
#include "perfwiz.h"
#include "FinalPage.h"
#include "WaitDlg.h"
#include <cderr.h>
#include <dlgs.h>
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define NOCHECK 1
#define CHECK 2

static COLORREF LineColors[] =
{
    RGB (0xff, 0x00, 0x00),
    RGB (0x00, 0x80, 0x00),
    RGB (0x00, 0x00, 0xff),
    RGB (0xff, 0xff, 0x00),
    RGB (0xff, 0x00, 0xff),
    RGB (0x00, 0xff, 0xff),
    RGB (0x80, 0x00, 0x00),
    RGB (0x40, 0x40, 0x40),
    RGB (0x00, 0x00, 0x80),
    RGB (0x80, 0x80, 0x00),
    RGB (0x80, 0x00, 0x80),
    RGB (0x00, 0x80, 0x80),
    RGB (0x40, 0x00, 0x00),
    RGB (0x00, 0x40, 0x00),
    RGB (0x00, 0x00, 0x40),
    RGB (0x00, 0x00, 0x00)
};

#define NumColors   (sizeof(LineColors)/sizeof(LineColors[0]))
#define NumWidth    5
#define NumStyle    4

static int  ObjectIndex;

struct Counters
{
    int		    CounterID, GroupID, *DefScale;
    LPWSTR	    CounterName, InstanceName, GroupName;
    struct Counters *next;
};

struct InstanceName
{
    LPCWSTR szInstanceName;
};

static const struct InstanceName CacheInstances[] =
{
    {L"2K pages "},
    {L"4K pages "},
    {L"8K pages "},
    {L"16K pages"},
    {L"32K pages"},
    {L"64K pages"}
};

static const struct InstanceName LockInstances[] =
{
    {L"Waiting"}
};

static const struct InstanceName ThreadInstances[] =
{
    {L"Cs_Intrnl_Thread"},
    {L"Normal          "},
    {L"Monitor         "},
    {L"Fast_Commit     "},
    {L"Write_Behind    "},
    {L"Server_Init     "},
    {L"Event           "},
    {L"2_Phase_Commit  "},
    {L"Recovery        "},
    {L"Logwriter       "},
    {L"Check_Dead      "},
    {L"Group_Commit    "},
    {L"Force_Abort     "},
    {L"Audit           "},
    {L"Cp_Timer        "},
    {L"Check_Term      "},
    {L"Secaud_Writer   "},
    {L"Write_Along     "},
    {L"Read_Ahead      "},
    {L"Rep_Qman        "},
    {L"Lkcallback      "},
    {L"Lkdeadlock      "},
    {L"Sampler         "},
    {L"Sort            "},
    {L"Factotum        "},
    {L"License         "},
    {L"<unknown>       "}
};

static const struct InstanceName SamplerInstances[] =
{
    {L"Sampler"}
};

static const struct InstanceName PersonalInstances[] =
{
    {L"Personal"}
};

static PBYTE DiskStringCopy(DISKSTRING *pDS, LPWSTR lpszText, PBYTE pNextFree);

int  CreatePerfChart(CString ChartName);

static void WriteCounterLine(LINESTRUCT *pLine, FILE *fptr);
static PPERF_COUNTER_DEFINITION	GetCounterDefinition(
						PPERF_DATA_BLOCK PerfData,
						DWORD ObjectIndex,
						LPCWSTR InstanceName,
						DWORD CounterIndex);

extern "C" void	ListDestructor();
extern int	ReadInitFile();

VOID ProcessCDError(DWORD dwErrorCode, HWND hWnd);
INT LocationSelect(HWND hDlg, CString *InputFile);
#ifdef WIN64
UINT_PTR CALLBACK LocSelHookProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
#else
UINT32 CALLBACK LocSelHookProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
#endif  /* WIN64 */

/*
** CFinalPage property page
*/
IMPLEMENT_DYNCREATE(CFinalPage, CPropertyPage)

CFinalPage::CFinalPage() : CPropertyPage(CFinalPage::IDD)
{
    //{{AFX_DATA_INIT(CFinalPage)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CFinalPage::CFinalPage(CPropertySheet *ps) : CPropertyPage(CFinalPage::IDD)
{
    m_propertysheet = ps;
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CFinalPage::~CFinalPage()
{
}

void CFinalPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFinalPage)
    DDX_Control(pDX, IDC_CHARTLOC_COMBO, m_ChartLoc);
    DDX_Control(pDX, IDC_OBJECTS_COMBO, m_ObjectList);
    DDX_Control(pDX, IDC_COUNTER_LIST, m_CounterList);
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFinalPage, CPropertyPage)
    //{{AFX_MSG_MAP(CFinalPage)
    ON_WM_DRAWITEM()
    ON_CBN_SELCHANGE(IDC_OBJECTS_COMBO, OnSelchangeObjectsCombo)
    ON_NOTIFY(NM_CLICK, IDC_COUNTER_LIST, OnClickCounterList)
    ON_NOTIFY(LVN_KEYDOWN, IDC_COUNTER_LIST, OnKeydownCounterList)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_COUNTER_LIST, OnItemchangedCounterList)
    ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)
    ON_BN_CLICKED(IDC_BUTTON_DESELECT, OnButtonDeselect)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE2, OnButtonBrowse2)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CFinalPage message handlers
*/
BOOL
CFinalPage::OnSetActive() 
{
    int			i;
    struct grouplist	*gptr;

    if (!InitFileRead)
    {
	CWaitDlg    wait;
	HANDLE	    hThread;
	DWORD	    ThreadID, status = 0;

	wait.m_WaitHeading.LoadString(IDS_READINIT_WAIT);
	MainWnd = NULL;
	hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)ReadInitFile,
				NULL, 0, &ThreadID );
	wait.DoModal();
	while (WaitForSingleObject(hThread, 0) != WAIT_OBJECT_0)
		Sleep(200);
	GetExitCodeThread(hThread, &status);

	if (status)
	{
	    CString Message, Message2;
	    char    outmsg[2048];

	    Message.LoadString(IDS_READINIT_ERR);
	    sprintf(outmsg, Message, status);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(outmsg, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return(-1);
	}
    }

    m_ObjectList.ResetContent();
    gptr = GroupList;
    while (gptr)
    {
	m_ObjectList.AddString(gptr->GroupName);
	gptr = gptr->next;
    }
    for (i=0; i<Num_Groups; i++)
	m_ObjectList.AddString(GroupNames[i].Name);
    m_ObjectList.SetCurSel(0);
    ObjectIndex = 0;

    m_CounterList.DeleteAllItems();
    InitCounterList();

    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);	
    return CPropertyPage::OnSetActive();
}

LPCTSTR
CFinalPage::MakeShortString(CDC *pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
{
    static const _TCHAR	szThreeDots[]=_T("...");
    int			nStringLen=lstrlen(lpszLong);
    static _TCHAR	szShort[MAX_PATH];
    int			nAddLen;

    if (nStringLen==0 ||
	pDC->GetTextExtent(lpszLong,nStringLen).cx+nOffset<=nColumnLen)
	return(lpszLong);

    lstrcpy(szShort,lpszLong);
    nAddLen=pDC->GetTextExtent(szThreeDots,sizeof(szThreeDots)).cx;

    for(int i=nStringLen-1; i>0; i--)
    {
	szShort[i]=0;
	if(pDC->GetTextExtent(szShort,i).cx+nOffset+nAddLen<=nColumnLen)
	    break;
    }

    lstrcat(szShort,szThreeDots);

    return(szShort);
}

void
CFinalPage::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
    int			m_cxClient=0, nColumn, m_cxStateImageOffset=0, nRetLen;
    BOOL		m_bClientWidthSel=TRUE;
    COLORREF		m_inactive=::GetSysColor(COLOR_INACTIVEBORDER);
    COLORREF		m_clrTextBk=::GetSysColor(COLOR_WINDOW);
    COLORREF		m_clrBkgnd=::GetSysColor(COLOR_WINDOW);
    CListCtrl		&ListCtrl=m_CounterList;
    CDC			*pDC=CDC::FromHandle(lpDrawItemStruct->hDC);
    CRect		rcItem(lpDrawItemStruct->rcItem);
    UINT		uiFlags=ILD_TRANSPARENT, nStateImageMask, nJustify;
    CImageList		*pImageList;
    int			nItem=lpDrawItemStruct->itemID;
    BOOL		bFocus=(GetFocus()==this);
    COLORREF		clrTextSave, clrBkSave;
    COLORREF		clrImage=m_clrBkgnd;
    static _TCHAR	szBuff[MAX_PATH];
    LPCTSTR		pszText;
    LV_ITEM		lvi;
    LV_COLUMN		lvc;
    BOOL		bSelected;
    CRect		rcAllLabels, rcLabel, rcIcon, rcRect;

    /* get item data */
    lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvi.iItem=nItem;
    lvi.iSubItem=0;
    lvi.pszText=szBuff;
    lvi.cchTextMax=sizeof(szBuff);
    lvi.stateMask=0xFFFF;	/* get all state flags */
    ListCtrl.GetItem(&lvi);

    bSelected=(lvi.state & LVIS_SELECTED);

    /* set colors if item is selected */
    ListCtrl.GetItemRect(nItem,rcAllLabels,LVIR_BOUNDS);
    ListCtrl.GetItemRect(nItem,rcLabel,LVIR_LABEL);
    rcAllLabels.left=rcLabel.left - OFFSET_FIRST;
    if (m_bClientWidthSel && rcAllLabels.right<m_cxClient)
	rcAllLabels.right=m_cxClient;

    if(bSelected && ListCtrl.IsWindowEnabled())
    {
	clrTextSave=pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
	clrBkSave=pDC->SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));

	CBrush brush(::GetSysColor(COLOR_HIGHLIGHT));
	pDC->FillRect(rcAllLabels, &brush);
    }
    else
    {
	CBrush brush1(m_clrTextBk), brush2(m_inactive);

	if (ListCtrl.IsWindowEnabled())
	    pDC->FillRect(rcAllLabels, &brush1);
	else
	    pDC->FillRect(rcAllLabels, &brush2);
    }

    /* set color and mask for the icon */
    if(lvi.state & LVIS_CUT)
    {
	clrImage=m_clrBkgnd;
	uiFlags|=ILD_BLEND50;
    }
    else if(bSelected && ListCtrl.IsWindowEnabled())
    {
	clrImage=::GetSysColor(COLOR_HIGHLIGHT);
	uiFlags|=ILD_BLEND50;
    }

    /* draw state icon */
    nStateImageMask=lvi.state & LVIS_STATEIMAGEMASK;
    if (nStateImageMask)
    {
	int nImage=(nStateImageMask>>12)-1;

	pImageList=ListCtrl.GetImageList(LVSIL_STATE);
	if(pImageList)
	    pImageList->Draw(pDC,nImage,CPoint(rcItem.left,rcItem.top),
			ILD_TRANSPARENT);
    }

    /* draw normal and overlay icon */
    ListCtrl.GetItemRect(nItem,rcIcon,LVIR_ICON);

    pImageList=ListCtrl.GetImageList(LVSIL_SMALL);
    if (pImageList)
    {
	UINT nOvlImageMask=lvi.state & LVIS_OVERLAYMASK;

	if(rcItem.left<rcItem.right-1)
	    ImageList_DrawEx(pImageList->m_hImageList,lvi.iImage,
			     pDC->m_hDC,rcIcon.left,rcIcon.top,16,16,
			     m_clrBkgnd,clrImage,uiFlags | nOvlImageMask);
    }

    /* draw item label */
    ListCtrl.GetItemRect(nItem,rcItem,LVIR_LABEL);
    rcItem.right-=m_cxStateImageOffset;

    pszText=MakeShortString(pDC,szBuff,rcItem.right-rcItem.left,
			    2*OFFSET_FIRST);

    rcLabel=rcItem;
    rcLabel.left+=OFFSET_FIRST;
    rcLabel.right-=OFFSET_FIRST;

    pDC->DrawText(pszText,-1,rcLabel,
		DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);

    /* draw labels for extra columns */
    lvc.mask=LVCF_FMT | LVCF_WIDTH;

    for (nColumn=1; ListCtrl.GetColumn(nColumn,&lvc); nColumn++)
    {
	rcItem.left=rcItem.right;
	rcItem.right+=lvc.cx;

	nRetLen=ListCtrl.GetItemText(nItem,nColumn,szBuff,sizeof(szBuff));
	if(nRetLen==0)
	    continue;

	pszText=MakeShortString(pDC,szBuff,rcItem.right-rcItem.left,
				2*OFFSET_OTHER);

	nJustify=DT_LEFT;

	if (pszText==szBuff)
	{
	    switch (lvc.fmt & LVCFMT_JUSTIFYMASK)
	    {
		case LVCFMT_RIGHT:
		    nJustify=DT_RIGHT;
		    break;

		case LVCFMT_CENTER:
		    nJustify=DT_CENTER;
		    break;

		default:
		    break;
	    }
	}

	rcLabel=rcItem;
	rcLabel.left+=OFFSET_OTHER;
	rcLabel.right-=OFFSET_OTHER;

	pDC->DrawText(pszText,-1,rcLabel,
		nJustify|DT_SINGLELINE|DT_NOPREFIX|DT_NOCLIP|DT_VCENTER);

        /* Draw a line separating each column... */
        CRect rcRect=rcItem;
        rcRect.right = rcRect.left;
        rcRect.left -= 1;
        
	pDC->FillRect(rcRect, 
		CBrush::FromHandle(GetSysColorBrush(COLOR_INACTIVEBORDER)));
    }
    rcRect = rcItem;
    rcRect.left = rcItem.right - 1;
    pDC->FillRect(rcRect, 
		CBrush::FromHandle(GetSysColorBrush(COLOR_INACTIVEBORDER)));

    /* Draw horizontal lines between each row ... */
    rcRect = rcItem;
    rcRect.top = rcRect.bottom - 1;
    rcRect.left = 0;
    pDC->FillRect(rcRect, 
		CBrush::FromHandle(GetSysColorBrush(COLOR_INACTIVEBORDER)));

    /* draw focus rectangle if item has focus */
    if (lvi.state & LVIS_FOCUSED && bFocus)
	pDC->DrawFocusRect(rcAllLabels);

    /* set original colors if item was selected */
    if (bSelected && ListCtrl.IsWindowEnabled())
    {
	pDC->SetTextColor(clrTextSave);
	pDC->SetBkColor(clrBkSave);
    }
}

void
CFinalPage::InitListViewControl()
{
    LV_COLUMN	col;
    RECT	rc;
    CString	ColumnHeader;

    m_StateImageList = new (CImageList);
    m_StateImageList->Create(IDB_LV_STATEIMAGE, 16, 1, RGB(255,0,0));
    m_CounterList.SetImageList(m_StateImageList, LVSIL_STATE);

    ColumnHeader.LoadString(IDS_COUNTLIST_COLNAME);
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    m_CounterList.GetClientRect(&rc);
    col.cx = rc.right;
    col.fmt = LVCFMT_LEFT;
    col.pszText = ColumnHeader.GetBuffer(256);
    m_CounterList.InsertColumn(0, &col);
}

BOOL
CFinalPage::OnInitDialog() 
{
    CString Message, HistFile;
    FILE    *fptr;
    char    mline[2048];

    CPropertyPage::OnInitDialog();

    Message.LoadString(IDS_FINAL_HEADING);
    SetDlgItemText(IDC_FINAL_HEADING, Message);
    Message.LoadString(IDS_INGRES_OBJECT);
    SetDlgItemText(IDC_INGRES_OBJECT, Message);
    Message.LoadString(IDS_HELP_DESC2);
    SetDlgItemText(IDC_HELP_DESC2, Message);
    Message.LoadString(IDS_CHART_STRING);
    SetDlgItemText(IDC_CHART_STRING, Message);
    Message.LoadString(IDS_BUTTON_SELECT);
    SetDlgItemText(IDC_BUTTON_SELECT, Message);
    Message.LoadString(IDS_BUTTON_DESELECT);
    SetDlgItemText(IDC_BUTTON_DESELECT, Message);
    Message.LoadString(IDS_BUTTON_BROWSE);
    SetDlgItemText(IDC_BUTTON_BROWSE2, Message);

    InitListViewControl();

    /*
    ** Fill up the destination combo box with the history file.
    */
    HistFile = CString(getenv("II_SYSTEM")) +
	       CString("\\ingres\\files\\perfwiz.hst");
    if ((fptr = fopen(HistFile, "r")) != NULL)
    {
	while ((fgets(mline, sizeof(mline)-1, fptr) != NULL))
	{
	    mline[strlen(mline)-1] = '\0';
	    m_ChartLoc.AddString((LPTSTR)&mline);
	}
	fclose (fptr);
    }
	
    return TRUE;
}

void
CFinalPage::InitCounterList()
{
    CString		ObjectName;
    struct grouplist	*gptr = GroupList;
    int			i, j, GroupIndex, groupidx;

    m_ObjectList.GetLBText(m_ObjectList.GetCurSel(), ObjectName);
    while (gptr && strcmp(gptr->GroupName, ObjectName))
	gptr = gptr->next;
    if (gptr)
    {
	for (i=0; i<NUM_PERSONAL_COUNTERS; i++)
	{
	    if (gptr->CounterList[i].selected)
	    {
		ListViewInsert( gptr->CounterList[i].UserName,
				gptr->CounterList[i].UserSelected );
	    }
	}

	/*
	** Add any personal counters.
	*/
	if (gptr->PersCtrs)
	{
	    struct PersCtrList  *PCPtr = gptr->PersCtrs;

	    while (PCPtr)
	    {
		ListViewInsert( PCPtr->PersCtr.UserName,
				PCPtr->PersCtr.UserSelected );
		PCPtr = PCPtr->next;
	    }
	}
    }
    else
    {
	for (i=0; i<Num_Groups; i++)
	{
	    if (!strcmp(GroupNames[i].Name, ObjectName))
		break;
	}
	for (j=i;
	     j>=0 && GroupNames[j].classid == GroupNames[i].classid;
	     j--);
	if (j < 0)
	    groupidx = i;
	else
	    groupidx = i - j - 1;

	switch (GroupNames[i].classid)
	{
	    case CacheClass:
		for (j=0; j<Num_Cache_Counters; j++)
		{
		    GroupIndex = Num_Cache_Counters * groupidx + j;
		    ListViewInsert(CacheGroup[GroupIndex].Name,
				   CacheGroup[GroupIndex].UserSelected);
		}
		break;

	    case LockClass:
		for (j=0; j<Num_Lock_Counters; j++)
		{
		    GroupIndex = Num_Lock_Counters * groupidx + j;
		    ListViewInsert(LockGroup[GroupIndex].Name,
				   LockGroup[GroupIndex].UserSelected);
		}
		break;

	    case ThreadClass:
		for (j=0; j<Num_Thread_Counters; j++)
		{
		    GroupIndex = Num_Thread_Counters * groupidx + j;
		    ListViewInsert(ThreadGroup[GroupIndex].Name,
				   ThreadGroup[GroupIndex].UserSelected);
		}
		break;

	    case SamplerClass:
		for (j=0; j<Num_Sampler_Counters; j++)
		{
		    GroupIndex = Num_Sampler_Counters * groupidx + j;
		    ListViewInsert(SamplerGroup[GroupIndex].Name,
				   SamplerGroup[GroupIndex].UserSelected);
		}
		break;
	}
    }
}

void
CFinalPage::ListViewInsert(char *CounterName, bool selected)
{
    LV_ITEM lvitem;

    lvitem.iItem = m_CounterList.GetItemCount();
    lvitem.mask = LVIF_TEXT;
    lvitem.iSubItem = 0;
    lvitem.pszText = CounterName;
    m_CounterList.InsertItem(&lvitem);

    if (selected)
	m_CounterList.SetItemState( lvitem.iItem, INDEXTOSTATEIMAGEMASK(CHECK),
				    LVIS_STATEIMAGEMASK );
    else
	m_CounterList.SetItemState( lvitem.iItem, INDEXTOSTATEIMAGEMASK(NOCHECK),
				    LVIS_STATEIMAGEMASK );
}

void
CFinalPage::OnSelchangeObjectsCombo() 
{
    CButton		*pEdit = (CButton *)GetDlgItem(IDC_COUNTER_DESC);

    if (m_ObjectList.GetCurSel() != ObjectIndex)
    {
	m_CounterList.DeleteAllItems();
	InitCounterList();
	ObjectIndex = m_ObjectList.GetCurSel();
	pEdit->SetWindowText("");
    }	
}

void
CFinalPage::OnClickCounterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_HITTESTINFO FAR	pinfo;
    DWORD		dwpos;
    int			rc, i, j, GroupIndex, groupidx;
    UINT		state; 
    POINT		point;
    struct grouplist	*gptr;
    CString		ObjectName, CounterName;

    /* Find out where the cursor was */
    dwpos = GetMessagePos();

    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_CounterList.m_hWnd, &point, 1);
    pinfo.pt = point;

    rc = m_CounterList.HitTest (&pinfo);
    if (pinfo.flags & LVHT_ONITEM)
    {
	/* Set the state to the opposite of what it is set to. */
	state = m_CounterList.GetItemState(pinfo.iItem, LVIS_STATEIMAGEMASK);
	state = (state == INDEXTOSTATEIMAGEMASK (NOCHECK)) ? 
			  INDEXTOSTATEIMAGEMASK (CHECK) : 
			  INDEXTOSTATEIMAGEMASK (NOCHECK);

	if (state == INDEXTOSTATEIMAGEMASK(CHECK))
	    NumCountersSelected++;
	else
	    NumCountersSelected--;

	m_CounterList.SetItemState(pinfo.iItem, state, LVIS_STATEIMAGEMASK);

	CounterName = m_CounterList.GetItemText(pinfo.iItem, 0);
	m_ObjectList.GetLBText(m_ObjectList.GetCurSel(), ObjectName);
	gptr = GroupList;
	while (gptr && strcmp(gptr->GroupName, ObjectName))
	    gptr = gptr->next;
	if (gptr)
	{
	    for (i=0; i<NUM_PERSONAL_COUNTERS; i++)
	    {
		if (!strcmp(gptr->CounterList[i].UserName, CounterName))
		    break;
	    }

	    if (i < NUM_PERSONAL_COUNTERS)
	    {
		if (state == INDEXTOSTATEIMAGEMASK (NOCHECK))
		    gptr->CounterList[i].UserSelected = FALSE;
		else
		    gptr->CounterList[i].UserSelected = TRUE;
	    }
	    else
	    {
		struct PersCtrList  *PCPtr = gptr->PersCtrs;

		/*
		** This is a personal counter.
		*/
		while (strcmp(PCPtr->PersCtr.UserName, CounterName) != 0)
		    PCPtr = PCPtr->next;

		if (state == INDEXTOSTATEIMAGEMASK (NOCHECK))
		    PCPtr->PersCtr.UserSelected = FALSE;
		else
		    PCPtr->PersCtr.UserSelected = TRUE;
	    }
	}
	else
	{
	    for (i=0; i<Num_Groups; i++)
	    {
		if (!strcmp(GroupNames[i].Name, ObjectName))
		    break;
	    }
	    for (j=i;
		 j>=0 && GroupNames[j].classid == GroupNames[i].classid;
		 j--);
	    if (j < 0)
		groupidx = i;
	    else
		groupidx = i - j - 1;

	    switch (GroupNames[i].classid)
	    {
		case CacheClass:
		    for (j=0; j<Num_Cache_Counters; j++)
		    {
			GroupIndex = Num_Cache_Counters * groupidx + j;
			if (!strcmp(CacheGroup[GroupIndex].Name, CounterName))
			    break;
		    }
		    if (state == INDEXTOSTATEIMAGEMASK (NOCHECK))
			CacheGroup[GroupIndex].UserSelected = FALSE;
		    else
			CacheGroup[GroupIndex].UserSelected = TRUE;

		    break;

		case LockClass:
		    for (j=0; j<Num_Lock_Counters; j++)
		    {
			GroupIndex = Num_Lock_Counters * groupidx + j;
			if (!strcmp(LockGroup[GroupIndex].Name, CounterName))
			    break;
		    }
		    if (state == INDEXTOSTATEIMAGEMASK (NOCHECK))
			LockGroup[GroupIndex].UserSelected = FALSE;
		    else
			LockGroup[GroupIndex].UserSelected = TRUE;

		    break;

		case ThreadClass:
		    for (j=0; j<Num_Thread_Counters; j++)
		    {
			GroupIndex = Num_Thread_Counters * groupidx + j;
			if (!strcmp(ThreadGroup[GroupIndex].Name, CounterName))
			    break;
		    }
		    if (state == INDEXTOSTATEIMAGEMASK (NOCHECK))
			ThreadGroup[GroupIndex].UserSelected = FALSE;
		    else
			ThreadGroup[GroupIndex].UserSelected = TRUE;

		    break;

		case SamplerClass:
		    for (j=0; j<Num_Sampler_Counters; j++)
		    {
			GroupIndex = Num_Cache_Counters * groupidx + j;
			if (!strcmp(SamplerGroup[GroupIndex].Name, CounterName))
			    break;
		    }
		    if (state == INDEXTOSTATEIMAGEMASK (NOCHECK))
			SamplerGroup[GroupIndex].UserSelected = FALSE;
		    else
			SamplerGroup[GroupIndex].UserSelected = TRUE;

		    break;
	    }
	}
    }

    *pResult = 0;
}

int
CreatePerfChart(CString ChartName)
{
    PERFFILEHEADER		FileHeader;
    DISKCHART			DiskChart;
    LINESTRUCT			*pline, *LineList=NULL, *lptr;
    FILE			*fptr;
    int				i, j, wsize;
    char			hostname[MAX_COMPUTERNAME_LENGTH + 1];
    char			hostname2[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR			whostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD			size = MAX_COMPUTERNAME_LENGTH + 1;
    struct Counters		*CounterList=NULL, *node, *cptr;
    struct grouplist		*gptr;
    int				ColorIndex=-1, StyleIndex=-1, WidthIndex=-1;
    DWORD			AllocSize, BufferSize;
    PPERF_DATA_BLOCK		PerfData;
    PPERF_COUNTER_DEFINITION	CounterDef;
    struct PersCtrList		*PCPtr;

    /*
    ** Create a list of counter ids selected.
    */
    for (i=0; i<NUM_CACHE_GROUPS*Num_Cache_Counters; i++)
    {
	if (CacheGroup[i].UserSelected)
	{
	    node = (struct Counters *)malloc(sizeof(struct Counters));
	    node->next = NULL;
	    node->DefScale = NULL;
	    node->CounterID = CacheGroup[i].CounterID;
	    wsize = sizeof(WCHAR) * ((int)strlen(CacheGroup[i].Name) + 1);
	    node->CounterName = (WCHAR *)malloc(wsize);
	    MultiByteToWideChar(CP_ACP, 0, CacheGroup[i].Name, -1,
				node->CounterName, wsize);

	    j = i / Num_Cache_Counters;
	    node->GroupID = GroupNames[j].CounterID;
	    wsize = sizeof(WCHAR) * ((int)strlen(GroupNames[j].Name) + 1);
	    node->GroupName = (WCHAR *)malloc(wsize);
	    MultiByteToWideChar(CP_ACP, 0, GroupNames[j].Name, -1,
				node->GroupName, wsize);

	    wsize = sizeof(WCHAR) * ((int)wcslen(CacheInstances[j].szInstanceName) + 1);
	    node->InstanceName = (WCHAR *)malloc(wsize);
	    wcscpy(node->InstanceName, CacheInstances[j].szInstanceName);

	    if (!CounterList)
		CounterList = node;
	    else
	    {
		cptr = CounterList;
		while (cptr->next)
		    cptr = cptr->next;
		cptr->next = node;
	    }
	}
    }
    for (i=0; i<NUM_LOCK_GROUPS*Num_Lock_Counters; i++)
    {
	if (LockGroup[i].UserSelected)
	{
	    node = (struct Counters *)malloc(sizeof(struct Counters));
	    node->next = NULL;
	    node->DefScale = NULL;
	    node->CounterID = LockGroup[i].CounterID;
	    wsize = sizeof(WCHAR) * ((int)strlen(LockGroup[i].Name) + 1);
	    node->CounterName = (WCHAR *)malloc(wsize);
	    MultiByteToWideChar(CP_ACP, 0, LockGroup[i].Name, -1,
				node->CounterName, wsize);

	    j = (i / Num_Lock_Counters) + NUM_CACHE_GROUPS;
	    node->GroupID = GroupNames[j].CounterID;
	    wsize = sizeof(WCHAR) * ((int)strlen(GroupNames[j].Name) + 1);
	    node->GroupName = (WCHAR *)malloc(wsize);
	    MultiByteToWideChar(CP_ACP, 0, GroupNames[j].Name, -1,
				node->GroupName, wsize);

	    j -= NUM_CACHE_GROUPS;
	    wsize = sizeof(WCHAR) * ((int)wcslen(LockInstances[j].szInstanceName) + 1);
	    node->InstanceName = (WCHAR *)malloc(wsize);
	    wcscpy(node->InstanceName, LockInstances[j].szInstanceName);

	    if (!CounterList)
		CounterList = node;
	    else
	    {
		cptr = CounterList;
		while (cptr->next)
		    cptr = cptr->next;
		cptr->next = node;
	    }
	}
    }
    for (i=0; i<NUM_THREAD_GROUPS*Num_Thread_Counters; i++)
    {
	if (ThreadGroup[i].UserSelected)
	{
	    node = (struct Counters *)malloc(sizeof(struct Counters));
	    node->next = NULL;
	    node->DefScale = NULL;
	    node->CounterID = ThreadGroup[i].CounterID;
	    wsize = sizeof(WCHAR) * ((int)strlen(ThreadGroup[i].Name) + 1);
	    node->CounterName = (WCHAR *)malloc(wsize);
	    MultiByteToWideChar(CP_ACP, 0, ThreadGroup[i].Name, -1,
				node->CounterName, wsize);

	    j = (i / Num_Thread_Counters) + NUM_CACHE_GROUPS + NUM_LOCK_GROUPS;
	    node->GroupID = GroupNames[j].CounterID;
	    wsize = sizeof(WCHAR) * ((int)strlen(GroupNames[j].Name) + 1);
	    node->GroupName = (WCHAR *)malloc(wsize);
	    MultiByteToWideChar(CP_ACP, 0, GroupNames[j].Name, -1,
				node->GroupName, wsize);

	    j -= NUM_CACHE_GROUPS + NUM_LOCK_GROUPS;
	    wsize = sizeof(WCHAR) * ((int)wcslen(ThreadInstances[j].szInstanceName) + 1);
	    node->InstanceName = (WCHAR *)malloc(wsize);
	    wcscpy(node->InstanceName, ThreadInstances[j].szInstanceName);

	    if (!CounterList)
		CounterList = node;
	    else
	    {
		cptr = CounterList;
		while (cptr->next)
		    cptr = cptr->next;
		cptr->next = node;
	    }
	}
    }
    for (i=0; i<NUM_SAMPLER_GROUPS*Num_Sampler_Counters; i++)
    {
	if (SamplerGroup[i].UserSelected)
	{
	    node = (struct Counters *)malloc(sizeof(struct Counters));
	    node->next = NULL;
	    node->DefScale = NULL;
	    node->CounterID = SamplerGroup[i].CounterID;
	    wsize = sizeof(WCHAR) * ((int)strlen(SamplerGroup[i].Name) + 1);
	    node->CounterName = (WCHAR *)malloc(wsize);
	    MultiByteToWideChar(CP_ACP, 0, SamplerGroup[i].Name, -1,
				node->CounterName, wsize);

	    j = (i / Num_Sampler_Counters) + NUM_CACHE_GROUPS + NUM_LOCK_GROUPS +
		NUM_THREAD_GROUPS;
	    node->GroupID = GroupNames[j].CounterID;
	    wsize = sizeof(WCHAR) * ((int)strlen(GroupNames[j].Name) + 1);
	    node->GroupName = (WCHAR *)malloc(wsize);
	    MultiByteToWideChar(CP_ACP, 0, GroupNames[j].Name, -1,
				node->GroupName, wsize);

	    j -= NUM_CACHE_GROUPS + NUM_LOCK_GROUPS + NUM_THREAD_GROUPS;
	    wsize = sizeof(WCHAR) * ((int)wcslen(SamplerInstances[j].szInstanceName) + 1);
	    node->InstanceName = (WCHAR *)malloc(wsize);
	    wcscpy(node->InstanceName, SamplerInstances[j].szInstanceName);

	    if (!CounterList)
		CounterList = node;
	    else
	    {
		cptr = CounterList;
		while (cptr->next)
		    cptr = cptr->next;
		cptr->next = node;
	    }
	}
    }
    gptr = GroupList;
    while (gptr)
    {
	for (i=0; i<NUM_PERSONAL_COUNTERS; i++)
	{
	    if (gptr->CounterList[i].UserSelected)
	    {
		node = (struct Counters *)malloc(sizeof(struct Counters));
		node->next = NULL;
		node->DefScale = NULL;
		node->CounterID = gptr->CounterList[i].CounterID;
		wsize = sizeof(WCHAR) * ((int)strlen(gptr->CounterList[i].UserName) + 1);
		node->CounterName = (WCHAR *)malloc(wsize);
		MultiByteToWideChar(CP_ACP, 0, gptr->CounterList[i].UserName,
				    -1, node->CounterName, wsize);

		node->GroupID = gptr->CounterID;
		wsize = sizeof(WCHAR) * ((int)strlen(gptr->GroupName) + 1);
		node->GroupName = (WCHAR *)malloc(wsize);
		MultiByteToWideChar(CP_ACP, 0, gptr->GroupName, -1,
				    node->GroupName, wsize);

		wsize = sizeof(WCHAR) * ((int)wcslen(PersonalInstances[0].szInstanceName) + 1);
		node->InstanceName = (WCHAR *)malloc(wsize);
		wcscpy(node->InstanceName, PersonalInstances[0].szInstanceName);

		if (!CounterList)
		    CounterList = node;
		else
		{
		    cptr = CounterList;
		    while (cptr->next)
			cptr = cptr->next;
		    cptr->next = node;
		}
	    }
	}

	/*
	** Check the personal counters.
	*/
	PCPtr = gptr->PersCtrs;
	while (PCPtr)
	{
	    if (PCPtr->PersCtr.UserSelected)
	    {
		node = (struct Counters *)malloc(sizeof(struct Counters));
		node->next = NULL;
		node->DefScale = (int *)malloc(sizeof(PCPtr->scale));
		*(node->DefScale) = PCPtr->scale;
		node->CounterID = PCPtr->PersCtr.CounterID;
		wsize = sizeof(WCHAR) * ((int)strlen(PCPtr->PersCtr.UserName) + 1);
		node->CounterName = (WCHAR *)malloc(wsize);
		MultiByteToWideChar(CP_ACP, 0, PCPtr->PersCtr.UserName,
				    -1, node->CounterName, wsize);

		node->GroupID = gptr->CounterID;
		wsize = sizeof(WCHAR) * ((int)strlen(gptr->GroupName) + 1);
		node->GroupName = (WCHAR *)malloc(wsize);
		MultiByteToWideChar(CP_ACP, 0, gptr->GroupName, -1,
				    node->GroupName, wsize);

		wsize = sizeof(WCHAR) * ((int)wcslen(PersonalInstances[0].szInstanceName) + 1);
		node->InstanceName = (WCHAR *)malloc(wsize);
		wcscpy(node->InstanceName, PersonalInstances[0].szInstanceName);

		if (!CounterList)
		    CounterList = node;
		else
		{
		    cptr = CounterList;
		    while (cptr->next)
			cptr = cptr->next;
		    cptr->next = node;
		}
	    }

	    PCPtr = PCPtr->next;
	}

	gptr = gptr->next;
    }

    /*
    ** Obtain performance data from the registry.
    */
    AllocSize = BufferSize = 4096;
    PerfData = (PPERF_DATA_BLOCK)malloc(AllocSize);
    while (RegQueryValueEx( HKEY_PERFORMANCE_DATA,
			    "Global",
			    NULL, NULL, (LPBYTE)PerfData,
			    &BufferSize ) == ERROR_MORE_DATA)
    {
	AllocSize += 4096;
	PerfData = (PPERF_DATA_BLOCK)realloc(PerfData, AllocSize);
	BufferSize = AllocSize;
    }
    BufferSize = AllocSize;
    RegCloseKey(HKEY_PERFORMANCE_DATA);

    /*
    ** Formulate the file header and write it out.
    */
    if ((fptr = fopen(ChartName, "wb")) == NULL)
    {
	free(PerfData);
	while (CounterList)
	{
	    cptr = CounterList->next;
	    free(CounterList->CounterName);
	    free(CounterList->GroupName);
	    free(CounterList->InstanceName);
	    CounterList = cptr;
	}

	return(1);
    }

    memset(&FileHeader, 0, sizeof(FileHeader));
    wcscpy(FileHeader.szSignature, szPerfChartSignature);
    FileHeader.dwMajorVersion = ChartMajorVersion;
    FileHeader.dwMinorVersion = ChartMinorVersion;
    fwrite(&FileHeader, sizeof(FileHeader), 1, fptr);

    /*
    ** Formulate the counter header of the chart and write it out.
    */
    DiskChart.dwNumLines = NumCountersSelected;
    DiskChart.gMaxValues = DEFAULT_MAX_VALUES;
    DiskChart.bManualRefresh = 0;

    DiskChart.perfmonOptions.bMenubar = 1;
    DiskChart.perfmonOptions.bToolbar = 1;
    DiskChart.perfmonOptions.bStatusbar = 1;
    DiskChart.perfmonOptions.bAlwaysOnTop = 0;

    DiskChart.gOptions.bLegendChecked = 1;
    DiskChart.gOptions.bMenuChecked = 1;
    DiskChart.gOptions.bLabelsChecked = 1;
    DiskChart.gOptions.bVertGridChecked = 0;
    DiskChart.gOptions.bHorzGridChecked = 0;
    DiskChart.gOptions.bStatusBarChecked = 1;
    DiskChart.gOptions.iVertMax = 100;
    DiskChart.gOptions.eTimeInterval = 1.00000;
    DiskChart.gOptions.iGraphOrHistogram = 1;
    DiskChart.gOptions.GraphVGrid = 1;
    DiskChart.gOptions.GraphHGrid = 1;
    DiskChart.gOptions.HistVGrid = 1;
    DiskChart.gOptions.HistHGrid = 1;

    DiskChart.Visual.crColor = 255;
    DiskChart.Visual.iColorIndex = 1;
    DiskChart.Visual.iStyle = 0;
    DiskChart.Visual.iStyleIndex = 0;
    DiskChart.Visual.iWidth = 1;
    DiskChart.Visual.iWidthIndex = 0;

    fwrite(&DiskChart, sizeof(DiskChart), 1, fptr);

    /*
    ** Formulate the individual lines and write it out.
    */
    GetComputerName(hostname, &size);
    sprintf(hostname2, "\\\\%s", hostname);
    MultiByteToWideChar(CP_ACP, 0, hostname2, -1,
		        whostname, (int)strlen(hostname2)+1);

    cptr = CounterList;
    while (cptr)
    {
	if (!cptr->DefScale)
	{
	    /*
	    ** Get the counter definition.
	    */
	    CounterDef = GetCounterDefinition(PerfData, cptr->GroupID,
					      cptr->InstanceName,
					      cptr->CounterID);
	    if (!CounterDef)
	    {
		free(PerfData);
		while (LineList)
		{
		    lptr = LineList->pLineNext;
		    free(LineList);
		    LineList = lptr;
		}
		while (CounterList)
		{
		    cptr = CounterList->next;
		    free(CounterList->CounterName);
		    free(CounterList->GroupName);
		    free(CounterList->InstanceName);
		    CounterList = cptr;
		}

		fclose(fptr);
		DeleteFile(ChartName);

		return(2);
	    }
	}

	pline = (LINESTRUCT *)malloc(sizeof(LINESTRUCT));
	memset(pline, 0, sizeof(LINESTRUCT));
	pline->iLineType = 1;
	pline->lnSystemName = whostname;
	pline->lnUniqueID = (DWORD)-1;
	pline->lnPerfFreq = 199450000;

	if (cptr->DefScale)
	{
	    pline->eScale = (float)pow((double)10.0,
				       (double)*cptr->DefScale);
	}
	else
	{
	    pline->eScale = (float)pow((double)10.0,
				       (double)CounterDef->DefaultScale);
	}

	pline->lnaOldCounterValue[0] = pline->lnaCounterValue[0] =
	    pline->lnaOldCounterValue[1] = pline->lnaCounterValue[1] = 
	    (LONGLONG)0;
	pline->pLineNext = 0;

	pline->lnObjectName = cptr->GroupName;
	pline->lnCounterName = cptr->CounterName;
	pline->lnInstanceName = cptr->InstanceName;
	pline->Visual.iColorIndex = (++ColorIndex) % NumColors;
	pline->Visual.crColor = LineColors[pline->Visual.iColorIndex];
	if (ColorIndex == 0)
	    pline->Visual.iStyleIndex = (++StyleIndex) % NumStyle;
	else
	    pline->Visual.iStyleIndex = StyleIndex % NumStyle;
	pline->Visual.iStyle = pline->Visual.iStyleIndex;
	if ((ColorIndex == 0) && (StyleIndex == 0))
	    pline->Visual.iWidthIndex = (++WidthIndex) % NumWidth;
	else
	    pline->Visual.iWidthIndex = WidthIndex % NumWidth;
	pline->Visual.iWidth = (pline->Visual.iWidthIndex*2) + 1;

	if (!LineList)
	    LineList = pline;
	else
	{
	    lptr = LineList;
	    while (lptr->pLineNext)
		lptr = lptr->pLineNext;
	    lptr->pLineNext = pline;
	}

	cptr = cptr->next;
    }

    for (lptr = LineList; lptr; lptr = lptr->pLineNext)
	WriteCounterLine(lptr, fptr);

    fclose(fptr);

    /*
    ** Free up the link lists.
    */
    free(PerfData);
    while (LineList)
    {
	lptr = LineList->pLineNext;
	free(LineList);
	LineList = lptr;
    }
    while (CounterList)
    {
	cptr = CounterList->next;
	free(CounterList->CounterName);
	free(CounterList->GroupName);
	free(CounterList->InstanceName);
	CounterList = cptr;
    }

    return(0);
}

void
CFinalPage::OnKeydownCounterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;

    if (pLVKeyDow->wVKey == VK_SPACE)
    {
	int		    i, j, GroupIndex, groupidx;
	UINT		    state; 
	struct grouplist    *gptr;
	CString		    ObjectName, CounterName;
	DWORD		    iItem;

	iItem = m_CounterList.GetSelectionMark();

	/* Set the state to the opposite of what it is set to. */
	state = m_CounterList.GetItemState(iItem, LVIS_STATEIMAGEMASK);
	state = (state == INDEXTOSTATEIMAGEMASK (NOCHECK)) ? 
			  INDEXTOSTATEIMAGEMASK (CHECK) : 
			  INDEXTOSTATEIMAGEMASK (NOCHECK);

	if (state == INDEXTOSTATEIMAGEMASK(CHECK))
	    NumCountersSelected++;
	else
	    NumCountersSelected--;

	m_CounterList.SetItemState(iItem, state, LVIS_STATEIMAGEMASK);

	CounterName = m_CounterList.GetItemText(iItem, 0);
	m_ObjectList.GetLBText(m_ObjectList.GetCurSel(), ObjectName);
	gptr = GroupList;
	while (gptr && strcmp(gptr->GroupName, ObjectName))
	    gptr = gptr->next;
	if (gptr)
	{
	    for (i=0; i<NUM_PERSONAL_COUNTERS; i++)
	    {
		if (!strcmp(gptr->CounterList[i].UserName, CounterName))
		    break;
	    }

	    if (state == INDEXTOSTATEIMAGEMASK (NOCHECK))
		gptr->CounterList[i].UserSelected = FALSE;
	    else
		gptr->CounterList[i].UserSelected = TRUE;
	}
	else
	{
	    for (i=0; i<Num_Groups; i++)
	    {
		if (!strcmp(GroupNames[i].Name, ObjectName))
		    break;
	    }
	    for (j=i;
		 j>=0 && GroupNames[j].classid == GroupNames[i].classid;
		 j--);
	    if (j < 0)
		groupidx = i;
	    else
		groupidx = i - j - 1;

	    switch (GroupNames[i].classid)
	    {
		case CacheClass:
		    for (j=0; j<Num_Cache_Counters; j++)
		    {
			GroupIndex = Num_Cache_Counters * groupidx + j;
			if (!strcmp(CacheGroup[GroupIndex].Name, CounterName))
			    break;
		    }
		    if (state == INDEXTOSTATEIMAGEMASK (NOCHECK))
			CacheGroup[GroupIndex].UserSelected = FALSE;
		    else
			CacheGroup[GroupIndex].UserSelected = TRUE;

		    break;

		case LockClass:
		    for (j=0; j<Num_Lock_Counters; j++)
		    {
			GroupIndex = Num_Lock_Counters * groupidx + j;
			if (!strcmp(LockGroup[GroupIndex].Name, CounterName))
			    break;
		    }
		    if (state == INDEXTOSTATEIMAGEMASK (NOCHECK))
			LockGroup[GroupIndex].UserSelected = FALSE;
		    else
			LockGroup[GroupIndex].UserSelected = TRUE;

		    break;

		case ThreadClass:
		    for (j=0; j<Num_Thread_Counters; j++)
		    {
			GroupIndex = Num_Thread_Counters * groupidx + j;
			if (!strcmp(ThreadGroup[GroupIndex].Name, CounterName))
			    break;
		    }
		    if (state == INDEXTOSTATEIMAGEMASK (NOCHECK))
			ThreadGroup[GroupIndex].UserSelected = FALSE;
		    else
			ThreadGroup[GroupIndex].UserSelected = TRUE;

		    break;

		case SamplerClass:
		    for (j=0; j<Num_Sampler_Counters; j++)
		    {
			GroupIndex = Num_Cache_Counters * groupidx + j;
			if (!strcmp(SamplerGroup[GroupIndex].Name, CounterName))
			    break;
		    }
		    if (state == INDEXTOSTATEIMAGEMASK (NOCHECK))
			SamplerGroup[GroupIndex].UserSelected = FALSE;
		    else
			SamplerGroup[GroupIndex].UserSelected = TRUE;

		    break;
	    }
	}
    }
    
    *pResult = 0;
}

static void
WriteCounterLine(LINESTRUCT *pLine, FILE *fptr)
{
    DWORD	Signature;
    size_t	size;
    WCHAR	LOCAL_SYS_CODE_NAME[] = L"....";
    DISKLINE	*pDL;
    PBYTE	NextFree;

    Signature = dwLineSignature;
    fwrite(&Signature, sizeof(Signature), 1, fptr);

    size = sizeof(DISKLINE) + ((DWORD)wcslen(LOCAL_SYS_CODE_NAME) * sizeof(WCHAR));
    size += (pLine->lnObjectName ?
		wcslen(pLine->lnObjectName)*sizeof(WCHAR) : 0);
    size += (pLine->lnCounterName ?
		wcslen(pLine->lnCounterName)*sizeof(WCHAR) : 0);
    size += (pLine->lnInstanceName ?
		wcslen(pLine->lnInstanceName)*sizeof(WCHAR) : 0);
    size += (pLine->lnPINName ?
		wcslen(pLine->lnPINName)*sizeof(WCHAR) : 0);
    size += (pLine->lnParentObjName ?
		wcslen(pLine->lnParentObjName)*sizeof(WCHAR) : 0);
    size += (pLine->lpszAlertProgram ?
		wcslen(pLine->lpszAlertProgram)*sizeof(WCHAR) : 0);
    fwrite(&size, sizeof(size), 1, fptr);

    pDL = (DISKLINE *)malloc(size);
    NextFree = (PBYTE)pDL + sizeof(DISKLINE);

    /* Convert the fixed size fields. */
    pDL->iLineType = pLine->iLineType ;
    pDL->dwUniqueID = pLine->lnUniqueID ;
    pDL->Visual = pLine->Visual ;
    pDL->iScaleIndex = pLine->iScaleIndex ;
    pDL->eScale = pLine->eScale ;
    pDL->bAlertOver = pLine->bAlertOver ;
    pDL->eAlertValue = pLine->eAlertValue ;
    pDL->bEveryTime = pLine->bEveryTime ;

    /* Copy the disk string fields. */
    NextFree = DiskStringCopy( &pDL->dsSystemName, LOCAL_SYS_CODE_NAME,
				NextFree );
    NextFree = DiskStringCopy( &pDL->dsObjectName, pLine->lnObjectName,
				NextFree );
    NextFree = DiskStringCopy( &pDL->dsCounterName, pLine->lnCounterName,
				NextFree );
    NextFree = DiskStringCopy( &pDL->dsParentObjName, pLine->lnParentObjName,
				NextFree );
    NextFree = DiskStringCopy( &pDL->dsInstanceName, pLine->lnInstanceName,
				NextFree );
    NextFree = DiskStringCopy( &pDL->dsPINName, pLine->lnPINName,
				NextFree );
    NextFree = DiskStringCopy( &pDL->dsAlertProgram, pLine->lpszAlertProgram,
				NextFree) ;
    fwrite(pDL, size, 1, fptr);

    free(pDL);
}

static PBYTE
DiskStringCopy(DISKSTRING *pDS, LPWSTR lpszText, PBYTE pNextFree)
{
    if (!lpszText)
    {
	pDS->dwOffset = 0;
	pDS->dwLength = 0;
	return (pNextFree);
    }
    else
    {
	pDS->dwOffset = pNextFree - (PBYTE) pDS;
	pDS->dwLength = (lpszText ? wcslen(lpszText) : 0);
	wcsncpy((WCHAR *)pNextFree, (WCHAR *)lpszText, pDS->dwLength);
	return (pNextFree + pDS->dwLength * sizeof(WCHAR));
    }
}

void CFinalPage::OnItemchangedCounterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_LISTVIEW*	pNMListView = (NM_LISTVIEW*)pNMHDR;
    CString		ObjectName, CounterName;
    int			Index, i, j, GroupIndex;
    struct grouplist    *gptr = GroupList;
    CButton		*pEdit = (CButton *)GetDlgItem(IDC_COUNTER_DESC);

    *pResult = 0;

    CounterName = m_CounterList.GetItemText(m_CounterList.GetSelectionMark(), 0);
    if (CounterName == "")
	return;

    /* First, find the right group pointer. */
    m_ObjectList.GetLBText(m_ObjectList.GetCurSel(), ObjectName);
    while (gptr && strcmp(gptr->GroupName, ObjectName))
	gptr = gptr->next;

    if (gptr)
    {
	/* Now, find the right counter. */
	Index = 0;
	while ( (Index < NUM_PERSONAL_COUNTERS) &&
		(strcmp(gptr->CounterList[Index].UserName, CounterName)) )
	    Index++;

	if (Index < NUM_PERSONAL_COUNTERS)
	    pEdit->SetWindowText(gptr->CounterList[Index].UserHelp);
	else
	{
	    struct PersCtrList  *PCPtr = gptr->PersCtrs;

	    /*
	    ** This is a personal counter.
	    */
	    while (strcmp(PCPtr->PersCtr.UserName, CounterName))
		PCPtr = PCPtr->next;
	    pEdit->SetWindowText(PCPtr->PersCtr.UserHelp);
	}
    }
    else
    {
	/* First, find the right group pointer. */
	for (i=0; i<Num_Groups; i++)
	{
	    if (!strcmp(GroupNames[i].Name, ObjectName))
		break;
	}
	for (j=i;
	     j>=0 && GroupNames[j].classid == GroupNames[i].classid;
	     j--);
	if (j < 0)
	    Index = i;
	else
	    Index = i - j - 1;

	/* Now, find the right counter. */
	switch (GroupNames[i].classid)
	{
	    case CacheClass:
		for (j=0; j<Num_Cache_Counters; j++)
		{
		    GroupIndex = Num_Cache_Counters * Index + j;
		    if (!strcmp(CacheGroup[GroupIndex].Name, CounterName))
			break;
		}
		pEdit->SetWindowText(CacheGroup[GroupIndex].Help);

		break;

	    case LockClass:
		for (j=0; j<Num_Lock_Counters; j++)
		{
		    GroupIndex = Num_Lock_Counters * Index + j;
		    if (!strcmp(LockGroup[GroupIndex].Name, CounterName))
			break;
		}
		pEdit->SetWindowText(LockGroup[GroupIndex].Help);

		break;

	    case ThreadClass:
		for (j=0; j<Num_Thread_Counters; j++)
		{
		    GroupIndex = Num_Thread_Counters * Index + j;
		    if (!strcmp(ThreadGroup[GroupIndex].Name, CounterName))
			break;
		}
		pEdit->SetWindowText(ThreadGroup[GroupIndex].Help);

		break;

	    case SamplerClass:
		for (j=0; j<Num_Sampler_Counters; j++)
		{
		    GroupIndex = Num_Cache_Counters * Index + j;
		    if (!strcmp(SamplerGroup[GroupIndex].Name, CounterName))
			break;
		}
		pEdit->SetWindowText(SamplerGroup[GroupIndex].Help);

		break;
	}
    }
}

void CFinalPage::OnButtonSelect() 
{
    int			i, j, groupidx, GroupIndex;
    int			NumCtrs = m_CounterList.GetItemCount();
    CString		ObjectName;
    struct grouplist	*gptr;

    for (i = 0; i < NumCtrs; i++)
    {
	m_CounterList.SetItemState( i, INDEXTOSTATEIMAGEMASK(CHECK),
				    LVIS_STATEIMAGEMASK );
    }

    m_ObjectList.GetLBText(m_ObjectList.GetCurSel(), ObjectName);
    gptr = GroupList;
    while (gptr && strcmp(gptr->GroupName, ObjectName))
	gptr = gptr->next;
    if (gptr)
    {
	for (i=0; i<NUM_PERSONAL_COUNTERS; i++)
	{
	    if (!gptr->CounterList[i].UserSelected && gptr->CounterList[i].selected)
	    {
		NumCountersSelected++;
		gptr->CounterList[i].UserSelected = TRUE;
	    }
	}

	/*
	** Add any personal counters.
	*/
	if (gptr->PersCtrs)
	{
	    struct PersCtrList  *PCPtr = gptr->PersCtrs;

	    while (PCPtr)
	    {
		if (!PCPtr->PersCtr.UserSelected && PCPtr->PersCtr.selected)
		{
		    NumCountersSelected++;
		    PCPtr->PersCtr.UserSelected = TRUE;
		}

		PCPtr = PCPtr->next;
	    }
	}
    }
    else
    {
	for (i=0; i<Num_Groups; i++)
	{
	    if (!strcmp(GroupNames[i].Name, ObjectName))
		break;
	}
	for (j=i;
	     j>=0 && GroupNames[j].classid == GroupNames[i].classid;
	     j--);
	if (j < 0)
	    groupidx = i;
	else
	    groupidx = i - j - 1;

	switch (GroupNames[i].classid)
	{
	    case CacheClass:
		for (j=0; j<Num_Cache_Counters; j++)
		{
		    GroupIndex = Num_Cache_Counters * groupidx + j;
		    if (!CacheGroup[GroupIndex].UserSelected)
		    {
			NumCountersSelected++;
			CacheGroup[GroupIndex].UserSelected = TRUE;
		    }
		}

		break;

	    case LockClass:
		for (j=0; j<Num_Lock_Counters; j++)
		{
		    GroupIndex = Num_Lock_Counters * groupidx + j;
		    if (!LockGroup[GroupIndex].UserSelected)
		    {
			NumCountersSelected++;
			LockGroup[GroupIndex].UserSelected = TRUE;
		    }
		}

		break;

	    case ThreadClass:
		for (j=0; j<Num_Thread_Counters; j++)
		{
		    GroupIndex = Num_Thread_Counters * groupidx + j;
		    if (!ThreadGroup[GroupIndex].UserSelected)
		    {
			NumCountersSelected++;
			ThreadGroup[GroupIndex].UserSelected = TRUE;
		    }
		}

		break;

	    case SamplerClass:
		for (j=0; j<Num_Sampler_Counters; j++)
		{
		    GroupIndex = Num_Cache_Counters * groupidx + j;
		    if (!SamplerGroup[GroupIndex].UserSelected)
		    {
			NumCountersSelected++;
			SamplerGroup[GroupIndex].UserSelected = TRUE;
		    }
		}

		break;
	}
    }
}

void CFinalPage::OnButtonDeselect() 
{
    int			i, j, groupidx, GroupIndex;
    CString		ObjectName;
    struct grouplist	*gptr;

    for (i = 0; i < m_CounterList.GetItemCount(); i++)
    {
	m_CounterList.SetItemState( i, INDEXTOSTATEIMAGEMASK(NOCHECK),
				    LVIS_STATEIMAGEMASK );
    }

    m_ObjectList.GetLBText(m_ObjectList.GetCurSel(), ObjectName);
    gptr = GroupList;
    while (gptr && strcmp(gptr->GroupName, ObjectName))
	gptr = gptr->next;
    if (gptr)
    {
	for (i=0; i<NUM_PERSONAL_COUNTERS; i++)
	{
	    if (gptr->CounterList[i].UserSelected)
	    {
		NumCountersSelected--;
		gptr->CounterList[i].UserSelected = FALSE;
	    }
	}

	/*
	** Take off any personal counters.
	*/
	if (gptr->PersCtrs)
	{
	    struct PersCtrList  *PCPtr = gptr->PersCtrs;

	    while (PCPtr)
	    {
		if (PCPtr->PersCtr.UserSelected)
		{
		    NumCountersSelected--;
		    PCPtr->PersCtr.UserSelected = FALSE;
		}

		PCPtr = PCPtr->next;
	    }
	}
    }
    else
    {
	for (i=0; i<Num_Groups; i++)
	{
	    if (!strcmp(GroupNames[i].Name, ObjectName))
		break;
	}
	for (j=i;
	     j>=0 && GroupNames[j].classid == GroupNames[i].classid;
	     j--);
	if (j < 0)
	    groupidx = i;
	else
	    groupidx = i - j - 1;

	switch (GroupNames[i].classid)
	{
	    case CacheClass:
		for (j=0; j<Num_Cache_Counters; j++)
		{
		    GroupIndex = Num_Cache_Counters * groupidx + j;
		    if (CacheGroup[GroupIndex].UserSelected)
		    {
			NumCountersSelected--;
			CacheGroup[GroupIndex].UserSelected = FALSE;
		    }
		}

		break;

	    case LockClass:
		for (j=0; j<Num_Lock_Counters; j++)
		{
		    GroupIndex = Num_Lock_Counters * groupidx + j;
		    if (LockGroup[GroupIndex].UserSelected)
		    {
			NumCountersSelected--;
			LockGroup[GroupIndex].UserSelected = FALSE;
		    }
		}

		break;

	    case ThreadClass:
		for (j=0; j<Num_Thread_Counters; j++)
		{
		    GroupIndex = Num_Thread_Counters * groupidx + j;
		    if (ThreadGroup[GroupIndex].UserSelected)
		    {
			NumCountersSelected--;
			ThreadGroup[GroupIndex].UserSelected = FALSE;
		    }
		}

		break;

	    case SamplerClass:
		for (j=0; j<Num_Sampler_Counters; j++)
		{
		    GroupIndex = Num_Cache_Counters * groupidx + j;
		    if (SamplerGroup[GroupIndex].UserSelected)
		    {
			NumCountersSelected--;
			SamplerGroup[GroupIndex].UserSelected = FALSE;
		    }
		}

		break;
	}
    }
}

void
Cleanup()
{
    struct grouplist	*gptr;
    struct PersCtrList	*PCPtr;
    int			i;

    /*
    ** Destroy the vnode list.
    */
    ListDestructor();

    /*
    ** Delete the personal group list.
    */
    while (GroupList)
    {
	/*
	** Free up its members.
	*/
	gptr = GroupList->next;
	FreePersonalCounters(GroupList);
	if (GroupList->GroupDescription)
	    free(GroupList->GroupDescription);
	if (GroupList->GroupName)
	    free(GroupList->GroupName);
	if (GroupList->PersCtrs)
	{
	    while (GroupList->PersCtrs)
	    {
		PCPtr = GroupList->PersCtrs->next;

		free(GroupList->PersCtrs->dbname);
		free(GroupList->PersCtrs->qry);
		free(GroupList->PersCtrs->PersCtr.CounterName);
		free(GroupList->PersCtrs->PersCtr.PCounterName);
		free(GroupList->PersCtrs->PersCtr.CounterHelp);
		free(GroupList->PersCtrs->PersCtr.UserName);
		free(GroupList->PersCtrs->PersCtr.UserHelp);
		if (GroupList->PersCtrs->PersCtr.DefineName)
		    free(GroupList->PersCtrs->PersCtr.DefineName);

		free(GroupList->PersCtrs);
		GroupList->PersCtrs = PCPtr;
	    }
	}

	free(GroupList);
	GroupList = gptr;
    }

    /*
    ** Free up allocated memory for the different group lists.
    */
    for (i=0; i<Num_Groups; i++)
    {
	if (GroupNames[i].Name)
	    free(GroupNames[i].Name);
	if (GroupNames[i].Help)
	free(GroupNames[i].Help);
    }
    for (i=0; i<NUM_CACHE_GROUPS*Num_Cache_Counters; i++)
    {
	if (CacheGroup[i].Name)
	    free(CacheGroup[i].Name);
	if (CacheGroup[i].Name)
	    free(CacheGroup[i].Help);
    }
    for (i=0; i<NUM_LOCK_GROUPS*Num_Lock_Counters; i++)
    {
	if (LockGroup[i].Name)
	    free(LockGroup[i].Name);
	if (LockGroup[i].Help)
	    free(LockGroup[i].Help);
    }
    for (i=0; i<NUM_THREAD_GROUPS*Num_Thread_Counters; i++)
    {
	if (ThreadGroup[i].Name)
	    free(ThreadGroup[i].Name);
	if (ThreadGroup[i].Help)
	    free(ThreadGroup[i].Help);
    }
    for (i=0; i<NUM_SAMPLER_GROUPS*Num_Sampler_Counters; i++)
    {
	if (SamplerGroup[i].Name)
	    free(SamplerGroup[i].Name);
	if (SamplerGroup[i].Help)
	    free(SamplerGroup[i].Help);
    }
    for (i=0; i<NUM_PERSONAL_COUNTERS; i++)
    {
	if (PersonalCtr_Init[i].CounterName)
	    free(PersonalCtr_Init[i].CounterName);
	if (PersonalCtr_Init[i].PCounterName)
	    free(PersonalCtr_Init[i].PCounterName);
	if (PersonalCtr_Init[i].CounterHelp)
	    free(PersonalCtr_Init[i].CounterHelp);
	if (PersonalCtr_Init[i].UserName)
	    free(PersonalCtr_Init[i].UserName);
	if (PersonalCtr_Init[i].UserHelp)
	    free(PersonalCtr_Init[i].UserHelp);
	if (PersonalCtr_Init[i].DefineName)
	    free(PersonalCtr_Init[i].DefineName);
    }
}

#ifdef WIN64
UINT_PTR CALLBACK
#else
UINT32 CALLBACK
#endif  /* WIN64 */
LocSelHookProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
    BOOL	rc;
    CWnd	Parent;
    CHAR	Filter[256];

    rc=TRUE;
    switch (messg)
    {
	case WM_INITDIALOG:          /* Set Default Values for Entry Fields */
	    Parent.Attach(::GetParent(hDlg));
	    Parent.CenterWindow();
	    Parent.Detach();
	    CommDlg_OpenSave_SetControlText(::GetParent(hDlg), IDOK, "OK");
	    rc=TRUE;
	    break;

	case WM_NOTIFY:
	    if (((LPOFNOTIFY)lParam)->hdr.code == CDN_FILEOK)
	    {
		GetDlgItemText( ::GetParent(hDlg), cmb1, (LPTSTR)&Filter,
				sizeof(Filter));
		if ((strstr(Filter, "Performance")) &&
		    (!strstr(((LPOFNOTIFY)lParam)->lpOFN->lpstrFileTitle,".pmc")))
		{
		    ((LPOFNOTIFY)lParam)->lpOFN->lCustData = 1;
		}
	    }
	    rc=TRUE;
	    break;

	default:
	    rc=FALSE;
	    break;
    }
    return rc;
}

INT
LocationSelect(HWND hDlg, CString *InputFile)
{
    CHAR	Filter[] =
	"Performance Monitor Chart File (*.pmc)\0*.pmc\0All Files (*.*)\0*.*\0\0";
    DWORD	status;
    CHAR	FileTitle[256], FileName[2048], OldFileName[256];
    OPENFILENAME OpenFileName;

    strcpy(OldFileName, *InputFile);
    *InputFile = "";
    strcpy(FileName, "");
    strcpy(FileTitle, "");

    OpenFileName.lStructSize       = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner         = hDlg;
    OpenFileName.lpstrCustomFilter = (LPSTR) NULL;
    OpenFileName.nMaxCustFilter    = 0L;
    OpenFileName.nFilterIndex      = 1L;
    OpenFileName.lpstrFilter       = Filter;
    OpenFileName.lpstrFile         = FileName;
    OpenFileName.nMaxFile          = sizeof(FileName);
    OpenFileName.lpstrDefExt       = NULL;
    OpenFileName.Flags             = OFN_ENABLEHOOK | OFN_HIDEREADONLY |
				     OFN_EXPLORER;
    OpenFileName.lpfnHook          = LocSelHookProc;

    OpenFileName.lpstrFileTitle    = FileTitle;
    OpenFileName.nMaxFileTitle     = sizeof(FileTitle);
    OpenFileName.lpstrInitialDir   = NULL;
    OpenFileName.lpstrTitle        = "Ingres Performance Monitor Wizard";
    OpenFileName.nFileOffset       = 0;
    OpenFileName.nFileExtension    = 0;
    OpenFileName.lCustData         = 0;

    if (!GetOpenFileName(&OpenFileName))
    {
	status = CommDlgExtendedError();

	if (status)
	{
	    ProcessCDError(status, hDlg);
	    return FALSE;
	}
	else
	{
	    *InputFile = CString(OldFileName);
	}
    }
    else
    {
	*InputFile = CString(FileName);
	if (OpenFileName.lCustData == 1)
	    *InputFile += CString(".pmc");
    }

    return TRUE;
}

VOID
ProcessCDError(DWORD dwErrorCode, HWND hWnd)
{
    WORD	wStringID;
    CString	buf, Message;

    switch(dwErrorCode)
    {
	case CDERR_DIALOGFAILURE:   wStringID=IDS_DIALOGFAILURE;   break;
	case CDERR_STRUCTSIZE:      wStringID=IDS_STRUCTSIZE;      break;
	case CDERR_INITIALIZATION:  wStringID=IDS_INITIALIZATION;  break;
	case CDERR_NOTEMPLATE:      wStringID=IDS_NOTEMPLATE;      break;
	case CDERR_NOHINSTANCE:     wStringID=IDS_NOHINSTANCE;     break;
	case CDERR_LOADSTRFAILURE:  wStringID=IDS_LOADSTRFAILURE;  break;
	case CDERR_FINDRESFAILURE:  wStringID=IDS_FINDRESFAILURE;  break;
	case CDERR_LOADRESFAILURE:  wStringID=IDS_LOADRESFAILURE;  break;
	case CDERR_LOCKRESFAILURE:  wStringID=IDS_LOCKRESFAILURE;  break;
	case CDERR_MEMALLOCFAILURE: wStringID=IDS_MEMALLOCFAILURE; break;
	case CDERR_MEMLOCKFAILURE:  wStringID=IDS_MEMLOCKFAILURE;  break;
	case CDERR_NOHOOK:          wStringID=IDS_NOHOOK;          break;
	case PDERR_SETUPFAILURE:    wStringID=IDS_SETUPFAILURE;    break;
	case PDERR_PARSEFAILURE:    wStringID=IDS_PARSEFAILURE;    break;
	case PDERR_RETDEFFAILURE:   wStringID=IDS_RETDEFFAILURE;   break;
	case PDERR_LOADDRVFAILURE:  wStringID=IDS_LOADDRVFAILURE;  break;
	case PDERR_GETDEVMODEFAIL:  wStringID=IDS_GETDEVMODEFAIL;  break;
	case PDERR_INITFAILURE:     wStringID=IDS_INITFAILURE;     break;
	case PDERR_NODEVICES:       wStringID=IDS_NODEVICES;       break;
	case PDERR_NODEFAULTPRN:    wStringID=IDS_NODEFAULTPRN;    break;
	case PDERR_DNDMMISMATCH:    wStringID=IDS_DNDMMISMATCH;    break;
	case PDERR_CREATEICFAILURE: wStringID=IDS_CREATEICFAILURE; break;
	case PDERR_PRINTERNOTFOUND: wStringID=IDS_PRINTERNOTFOUND; break;
	case CFERR_NOFONTS:         wStringID=IDS_NOFONTS;         break;
	case FNERR_SUBCLASSFAILURE: wStringID=IDS_SUBCLASSFAILURE; break;
	case FNERR_INVALIDFILENAME: wStringID=IDS_INVALIDFILENAME; break;
	case FNERR_BUFFERTOOSMALL:  wStringID=IDS_BUFFERTOOSMALL;  break;

	case 0:   /* User may have hit CANCEL or we got a *very* random error */
	    return;

	default:
	    wStringID=IDS_UNKNOWNERROR;
    }

    buf.LoadString(wStringID);
    Message.LoadString(IDS_TITLE);
    MessageBox(hWnd, buf, Message, MB_APPLMODAL | MB_OK);
    return;
}

void CFinalPage::OnButtonBrowse2() 
{
    GetDlgItemText(IDC_CHARTLOC_COMBO, ChartName);
    LocationSelect(m_hWnd, &ChartName);
    SetDlgItemText(IDC_CHARTLOC_COMBO, ChartName);	
}

LRESULT CFinalPage::OnWizardNext() 
{
    CString		HistFile, TempFile, CmdLine, Message, Message2;
    char		mline[2048];
    FILE		*outptr, *inptr;
    int			count;
    DWORD		status = 0;
    LPITEMIDLIST	pidlPersonal = NULL;
    char		szPath[MAX_PATH];

    if (!NumCountersSelected)
    {
	Message.LoadString(IDS_NO_COUNTERS);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	return -1;
    }

    GetDlgItemText(IDC_CHARTLOC_COMBO, ChartName);

    ChartName.TrimRight();
    if (ChartName.Right(4) != CString(".pmc"))
	ChartName = ChartName + CString(".pmc");

    if (ChartName.Find('\\') == -1)
    {
	/*
	** If we've just been given a file name, default it's location
	** to "My Documents".
	*/
	SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidlPersonal);
	if (pidlPersonal)
	{
	    SHGetPathFromIDList(pidlPersonal, szPath);
	    ChartName = CString(szPath) + CString("\\") + ChartName;
	    SetDlgItemText(IDC_CHARTLOC_COMBO, ChartName);
	}
    }

    if (ChartName == "")
    {
	Message.LoadString(IDS_NEED_CHARTFILE);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	return -1;
    }

    if (GetFileAttributes(ChartName) != -1)
    {
	Message.LoadString(IDS_CHART_EXISTS);
	Message2.LoadString(IDS_TITLE);
	status = MessageBox(Message, Message2, MB_YESNO | MB_ICONINFORMATION);
	if (status == IDNO)
	    return -1;
    }

    /*
    ** Write out the history file. We'll only keep the last 15
    ** unique recent files.
    */
    HistFile = CString(getenv("II_SYSTEM")) +
	       CString("\\ingres\\files\\perfwiz.hst");
    TempFile = CString(getenv("II_SYSTEM")) +
	       CString("\\ingres\\files\\perfwiz.tmp");
    if ((outptr = fopen(TempFile, "w")) != NULL)
    {
	fprintf(outptr, "%s\n", ChartName);
	if ((inptr = fopen(HistFile, "r")) != NULL)
	{
	    count = 0;
	    while ( (fgets(mline, sizeof(mline)-1, inptr) != NULL) &&
		    (count < 15))
	    {
		mline[strlen(mline)-1] = '\0';
		if (strcmp(mline, ChartName))
		{
		    fprintf(outptr, "%s\n", mline);
		    count++;
		}
	    }
	    fclose (inptr);
	}
	fclose (outptr);
	CopyFile(TempFile, HistFile, FALSE);
	DeleteFile(TempFile);
    }

    return CPropertyPage::OnWizardNext();
}

LRESULT CFinalPage::OnWizardBack() 
{
    if (PersonalGroup)
	return IDD_PERSONAL_HELP_PAGE;
    else
	return IDD_INTRO_PAGE;
}

static PPERF_COUNTER_DEFINITION	GetCounterDefinition(
						PPERF_DATA_BLOCK PerfData,
						DWORD ObjectIndex,
						LPCWSTR InstanceName,
						DWORD CounterIndex)
{
    PPERF_OBJECT_TYPE		PerfObj;
    PPERF_INSTANCE_DEFINITION	PerfInst;
    PPERF_COUNTER_DEFINITION	PerfCntr, CurCntr;
    PPERF_COUNTER_BLOCK		PtrToCntr;
    DWORD			i;

    /*
    ** Get the object that we're interested in.
    */
    PerfObj = (PPERF_OBJECT_TYPE)((PBYTE)PerfData + PerfData->HeaderLength);
    i = 0;
    while ( (PerfObj->ObjectNameTitleIndex != ObjectIndex) &&
	    (i < PerfData->NumObjectTypes) )
    {
	PerfObj = (PPERF_OBJECT_TYPE)((PBYTE)PerfObj +
			PerfObj->TotalByteLength);
	i++;
    }

    if (PerfObj->ObjectNameTitleIndex != ObjectIndex)
	return(0);

    /*
    ** Now, get the counter that we're interested in.
    */
    PerfCntr = (PPERF_COUNTER_DEFINITION)((PBYTE)PerfObj +
		    PerfObj->HeaderLength);

    if (PerfObj->NumInstances > 0)
    {
	/*
	** Find the instance that we're interested in.
	*/
	PerfInst = (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfObj +
			PerfObj->DefinitionLength);
	i = 0;
	while ( (!wcsstr((WCHAR *)((PBYTE)PerfInst + PerfInst->NameOffset),
			 InstanceName)) &&
		(i < (DWORD)PerfObj->NumInstances) )
	{
	    PerfInst = (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfInst +
			    PerfInst->ByteLength);
	    i++;
	}

	if (!wcsstr((WCHAR *)((PBYTE)PerfInst + PerfInst->NameOffset),
		   InstanceName))
	    return(0);

	/*
	** Now find the counter that we want.
	*/
	CurCntr = PerfCntr;
	PtrToCntr = (PPERF_COUNTER_BLOCK)((PBYTE)PerfInst +
			PerfInst->ByteLength);
	i = 0;
	while ( (CurCntr->CounterNameTitleIndex != CounterIndex) &&
		(i < PerfObj->NumCounters) )
	{
	    CurCntr = (PPERF_COUNTER_DEFINITION)((PBYTE)CurCntr +
			    CurCntr->ByteLength);
	    i++;
	}

	if (CurCntr->CounterNameTitleIndex != CounterIndex)
	    return(0);
	else
	    return(CurCntr);
    }
    else
    {
	PtrToCntr = (PPERF_COUNTER_BLOCK)((PBYTE)PerfObj +
			PerfObj->DefinitionLength);
	i = 0;
	while ( (PerfCntr->CounterNameTitleIndex != CounterIndex) &&
		(i < PerfObj->NumCounters) )
	{
	    PerfCntr = (PPERF_COUNTER_DEFINITION)((PBYTE)PerfCntr +
			    PerfCntr->ByteLength);
	    i++;
	}

	if (PerfCntr->CounterNameTitleIndex != CounterIndex)
	    return(0);
	else
	    return(PerfCntr);
    }
}
