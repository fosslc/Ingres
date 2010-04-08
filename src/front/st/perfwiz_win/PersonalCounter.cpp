/*
**
**  Name: PersonalCounter.cpp
**
**  Description:
**	This file contains the routines necessary for the Personal Counter Property
**	Page, which allows users to add counters to their personal groups.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
**	29-oct-1999 (somsa01)
**	    Added the ability to select personal counters.
**	07-May-2008 (drivi01)
**	    Apply Wizard97 template to this utility.
*/

#include "stdafx.h"
#include "perfwiz.h"
#include "PersonalCounter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int  ObjCtrIndex, PersonalCtrIndex, GroupIndex, ObjectIndex;

/*
** CPersonalCounter property page
*/
IMPLEMENT_DYNCREATE(CPersonalCounter, CPropertyPage)

CPersonalCounter::CPersonalCounter() : CPropertyPage(CPersonalCounter::IDD)
{
    //{{AFX_DATA_INIT(CPersonalCounter)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CPersonalCounter::CPersonalCounter(CPropertySheet *ps) : CPropertyPage(CPersonalCounter::IDD)
{
    m_propertysheet = ps;
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CPersonalCounter::~CPersonalCounter()
{
}

void CPersonalCounter::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPersonalCounter)
    DDX_Control(pDX, IDC_PERSONAL_COUNTER_LIST, m_PersonalCtrList);
    DDX_Control(pDX, IDC_COUNTER_LIST, m_CounterList);
    DDX_Control(pDX, IDC_COMBO_OBJECTS, m_ObjectList);
    DDX_Control(pDX, IDC_COMBO_GROUPS, m_GroupList);
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPersonalCounter, CPropertyPage)
    //{{AFX_MSG_MAP(CPersonalCounter)
    ON_CBN_SELCHANGE(IDC_COMBO_OBJECTS, OnSelchangeComboObjects)
    ON_CBN_SELCHANGE(IDC_COMBO_GROUPS, OnSelchangeComboGroups)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
    ON_NOTIFY(NM_CLICK, IDC_COUNTER_LIST, OnClickCounterList)
    ON_NOTIFY(NM_CLICK, IDC_PERSONAL_COUNTER_LIST, OnClickPersonalCounterList)
    ON_WM_DRAWITEM()
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_COUNTER_LIST, OnItemchangedCounterList)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_PERSONAL_COUNTER_LIST, OnItemchangedPersonalCounterList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CPersonalCounter message handlers
*/
BOOL
CPersonalCounter::OnSetActive() 
{
    struct grouplist	*gptr = GroupList;

    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    m_GroupList.ResetContent();
    while (gptr)
    {
	m_GroupList.AddString(gptr->GroupName);
	gptr = gptr->next;
    }
    m_GroupList.SetCurSel(0);
    GroupIndex = 0;
    ObjectIndex = m_ObjectList.GetCurSel();

    m_CounterList.DeleteAllItems();
    m_PersonalCtrList.DeleteAllItems();
    InitCounterLists();

    return CPropertyPage::OnSetActive();
}

BOOL
CPersonalCounter::OnInitDialog() 
{
    CString Message;
    int	    i;

    CPropertyPage::OnInitDialog();

    Message.LoadString(IDS_PERSONAL_COUNTER_HEADING);
    SetDlgItemText(IDC_PERSONAL_COUNTER_HEADING, Message);
    Message.LoadString(IDS_PERSONAL_GROUP_NAME);
    SetDlgItemText(IDC_PERSONAL_GROUP_NAME, Message);
    Message.LoadString(IDS_PERSONAL_COUNTER_COMBO);
    SetDlgItemText(IDC_PERSONAL_COUNTER_COMBO, Message);
    Message.LoadString(IDS_INGRES_OBJECT);
    SetDlgItemText(IDC_INGRES_OBJECT, Message);

    InitListViewControls();
    for (i=0; i<Num_Groups; i++)
	m_ObjectList.AddString(GroupNames[i].Name);
    m_ObjectList.SetCurSel(0);
    ObjectIndex = 0;

    ObjCtrIndex = PersonalCtrIndex = -1;
    return TRUE;
}

void
CPersonalCounter::InitListViewControls()
{
    LV_COLUMN	col;
    RECT	rc;
    CString	ColumnHeader;

    ColumnHeader.LoadString(IDS_COUNTLIST_COLNAME);
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    m_CounterList.GetClientRect(&rc);
    col.cx = rc.right;
    col.fmt = LVCFMT_LEFT;
    col.pszText = ColumnHeader.GetBuffer(256);
    m_CounterList.InsertColumn(0, &col);

    ColumnHeader.LoadString(IDS_PERSLIST_COLNAME);
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    m_PersonalCtrList.GetClientRect(&rc);
    col.cx = rc.right;
    col.fmt = LVCFMT_LEFT;
    col.pszText = ColumnHeader.GetBuffer(256);
    m_PersonalCtrList.InsertColumn(0, &col);
}

void
CPersonalCounter::OnSelchangeComboObjects() 
{
    if (m_ObjectList.GetCurSel() != ObjectIndex)
    {
	m_CounterList.DeleteAllItems();
	m_PersonalCtrList.DeleteAllItems();
	InitCounterLists();
	ObjectIndex = m_ObjectList.GetCurSel();
    }
}

void
CPersonalCounter::OnSelchangeComboGroups() 
{
    if (m_GroupList.GetCurSel() != GroupIndex)
    {
	m_CounterList.DeleteAllItems();
	m_PersonalCtrList.DeleteAllItems();
	InitCounterLists();
	GroupIndex = m_GroupList.GetCurSel();
    }
}

void
CPersonalCounter::InitCounterLists()
{
    struct grouplist	*gptr = GroupList;
    int			i, j, k, ObjID = m_ObjectList.GetCurSel();
    int			GroupIndex;
    CString		GroupName;

    m_GroupList.GetLBText(m_GroupList.GetCurSel(), GroupName);
    while (strcmp(gptr->GroupName, GroupName))
	gptr = gptr->next;

    /*
    ** Place the counters in the counter browse list box for the object
    ** if the user group selected has not chosen it already. Otherwise,
    ** place it in the counters chosen list box.
    */
    for (i=0, j=0, k=0; i<NUM_PERSONAL_COUNTERS; i++)
    {
	if (gptr->CounterList[i].selected)
	    m_PersonalCtrList.InsertItem(j++, gptr->CounterList[i].PCounterName);
	else
	{
	    if (gptr->CounterList[i].groupid == ObjID)
	    {
		switch (gptr->CounterList[i].classid)
		{
		    case CacheClass:
			GroupIndex = Num_Cache_Counters *
				     gptr->CounterList[i].groupidx +
				     gptr->CounterList[i].ctrid;
			m_CounterList.InsertItem(k++,
				CacheGroup[GroupIndex].Name);
			break;

		    case LockClass:
			GroupIndex = Num_Lock_Counters *
				     gptr->CounterList[i].groupidx +
				     gptr->CounterList[i].ctrid;
			m_CounterList.InsertItem(k++,
				LockGroup[GroupIndex].Name);
			break;

		    case ThreadClass:
			GroupIndex = Num_Thread_Counters *
				     gptr->CounterList[i].groupidx +
				     gptr->CounterList[i].ctrid;
			m_CounterList.InsertItem(k++,
				ThreadGroup[GroupIndex].Name);
			break;

		    case SamplerClass:
			GroupIndex = Num_Sampler_Counters *
				     gptr->CounterList[i].groupidx +
				     gptr->CounterList[i].ctrid;
			m_CounterList.InsertItem(k++,
				SamplerGroup[GroupIndex].Name);
			break;
		}
	    }
	}
    }

    /*
    ** Now, place any personal counters for the personal group in the
    ** list box.
    */
    if (gptr->PersCtrs)
    {
	struct PersCtrList  *PCPtr = gptr->PersCtrs;
	int		    Index = m_PersonalCtrList.GetItemCount();

	while (PCPtr)
	{
	    m_PersonalCtrList.InsertItem(Index++,
					 PCPtr->PersCtr.PCounterName);
	    PCPtr = PCPtr->next;
	}
    }
}

void
CPersonalCounter::OnButtonAdd() 
{
    CString		ObjName, CtrName, PersonalGroup;
    struct grouplist	*gptr = GroupList;
    int			Index;
    POSITION		pos;

    if (ObjCtrIndex != -1)
    {
	/*
	** First, find the counter in the personal group and "turn it on".
	*/
	pos = m_CounterList.GetFirstSelectedItemPosition();
	if (!pos)
	{
	    ObjCtrIndex = -1;
	    return;
	}

	while (pos)
	{
	    ObjCtrIndex  = m_CounterList.GetNextSelectedItem(pos);

	    m_ObjectList.GetLBText(m_ObjectList.GetCurSel(), ObjName);
	    CtrName = m_CounterList.GetItemText(ObjCtrIndex, 0);
	    m_GroupList.GetLBText(m_GroupList.GetCurSel(), PersonalGroup);
	    while (strcmp(gptr->GroupName, PersonalGroup))
		gptr = gptr->next;

	    for (Index = 0;
		 strcmp(GroupNames[gptr->CounterList[Index].groupid].Name, ObjName) !=0 ||
		 strcmp(gptr->CounterList[Index].CounterName, CtrName) !=0;
		 Index++);
	    gptr->CounterList[Index].selected = TRUE;
	    gptr->NumCtrsSelected++;
	}

	/*
	** Now, refresh the personal and source counter list views.
	*/
	ObjCtrIndex = -1;
	m_CounterList.DeleteAllItems();
	m_PersonalCtrList.DeleteAllItems();
	InitCounterLists();
    }
}

void
CPersonalCounter::OnButtonDelete() 
{
    CString		CtrName, PersonalGroup;
    struct grouplist	*gptr = GroupList;
    int			Index;
    POSITION		pos;

    if (PersonalCtrIndex != -1)
    {
	/*
	** First, find the counter in the personal group and "turn it off".
	*/
	pos = m_PersonalCtrList.GetFirstSelectedItemPosition();
	if (!pos)
	{
	    PersonalCtrIndex = -1;
	    return;
	}

	while (pos)
	{
	    PersonalCtrIndex  = m_PersonalCtrList.GetNextSelectedItem(pos);

	    CtrName = m_PersonalCtrList.GetItemText(PersonalCtrIndex, 0);
	    m_GroupList.GetLBText(m_GroupList.GetCurSel(), PersonalGroup);
	    while (strcmp(gptr->GroupName, PersonalGroup))
		gptr = gptr->next;
	    Index = 0;
	    while ( (Index < NUM_PERSONAL_COUNTERS) &&
		    (strcmp(gptr->CounterList[Index].PCounterName, CtrName)) )
		Index++;

	    if (Index < NUM_PERSONAL_COUNTERS)
	    {
		gptr->CounterList[Index].selected = FALSE;

		/*
		** Reset the User Name and Help text back to the default.
		*/
		free(gptr->CounterList[Index].UserName);
		gptr->CounterList[Index].UserName = (char *)malloc(
			strlen(gptr->CounterList[Index].PCounterName)+1);
		strcpy( gptr->CounterList[Index].UserName,
			gptr->CounterList[Index].PCounterName );
		free(gptr->CounterList[Index].UserHelp);
		gptr->CounterList[Index].UserHelp = (char *)malloc(
			strlen(gptr->CounterList[Index].CounterHelp)+1);
		strcpy( gptr->CounterList[Index].UserHelp,
			gptr->CounterList[Index].CounterHelp );

		gptr->NumCtrsSelected--;
	    }
	    else
	    {
		CString	Message, Message2;

		/*
		** This is a personal counter. We cannot "delete" it
		** from the user list from this screen
		*/
		Message.LoadString(IDS_CANNOT_DELETE_CTR);
		Message2.LoadString(IDS_TITLE);
		MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
		return;
	    }
	}

	/*
	** Now, refresh the personal and source counter list views.
	*/
	PersonalCtrIndex = -1;
	m_CounterList.DeleteAllItems();
	m_PersonalCtrList.DeleteAllItems();
	InitCounterLists();
    }
}

void
CPersonalCounter::OnClickCounterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_HITTESTINFO FAR	pinfo;
    DWORD		dwpos;
    POINT		point;
    LV_ITEM		lvi;

    /* Find out where the cursor was */
    dwpos = GetMessagePos();

    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_CounterList.m_hWnd, &point, 1);
    pinfo.pt = point;

    m_CounterList.HitTest(&pinfo);
    if (pinfo.flags & LVHT_ONITEM)
	ObjCtrIndex = pinfo.iItem;

    /* Unset any item set in the other list box. */
    if (PersonalCtrIndex != -1)
    {
	lvi.mask=LVIF_STATE;
	lvi.iItem=PersonalCtrIndex;
	lvi.iSubItem=0;
	lvi.stateMask=0xFFFF;	/* get all state flags */
	m_PersonalCtrList.GetItem(&lvi);

	lvi.state &= ~(LVIS_SELECTED);
	m_PersonalCtrList.SetItem(&lvi);

	PersonalCtrIndex = -1;
    }

    *pResult = 0;
}

void
CPersonalCounter::OnClickPersonalCounterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_HITTESTINFO FAR	pinfo;
    DWORD		dwpos;
    POINT		point;
    LV_ITEM		lvi;

    /* Find out where the cursor was */
    dwpos = GetMessagePos();

    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_PersonalCtrList.m_hWnd, &point, 1);
    pinfo.pt = point;

    m_PersonalCtrList.HitTest(&pinfo);
    if (pinfo.flags & LVHT_ONITEM)
	PersonalCtrIndex = pinfo.iItem;

    /* Unset any item set in the other list box. */
    if (ObjCtrIndex != -1)
    {
	lvi.mask=LVIF_STATE;
	lvi.iItem=ObjCtrIndex;
	lvi.iSubItem=0;
	lvi.stateMask=0xFFFF;	/* get all state flags */
	m_CounterList.GetItem(&lvi);

	lvi.state &= ~(LVIS_SELECTED);
	m_CounterList.SetItem(&lvi);

	ObjCtrIndex = -1;
    }

    *pResult = 0;
}

LPCTSTR
CPersonalCounter::MakeShortString(CDC *pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
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
CPersonalCounter::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
    int			m_cxClient=0, nColumn, m_cxStateImageOffset=0, nRetLen;
    BOOL		m_bClientWidthSel=TRUE;
    COLORREF		m_inactive=::GetSysColor(COLOR_INACTIVEBORDER);
    COLORREF		m_clrTextBk=::GetSysColor(COLOR_WINDOW);
    COLORREF		m_clrBkgnd=::GetSysColor(COLOR_WINDOW);
    CListCtrl		&ListCtrl=
			((lpDrawItemStruct->CtlID == IDC_COUNTER_LIST) ?
			m_CounterList : m_PersonalCtrList);
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

LRESULT
CPersonalCounter::OnWizardBack() 
{
    ObjCtrIndex = PersonalCtrIndex = -1;
	
    return CPropertyPage::OnWizardBack();
}

LRESULT
CPersonalCounter::OnWizardNext() 
{
    struct grouplist	*gptr = GroupList;
    CString		Message, Message2;
    char		outmsg[1024];

    /*
    ** Make sure the user has chosen at least one counter per
    ** personal group.
    */
    while (gptr)
    {
	if (!gptr->NumCtrsSelected && !gptr->PersCtrs)
	{
	    Message.LoadString(IDS_NOCTRS_SELECTED);
	    sprintf(outmsg, Message, gptr->GroupName);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(outmsg, Message2, MB_OK | MB_ICONEXCLAMATION);
	    m_GroupList.SetCurSel(
		    m_GroupList.FindStringExact(-1, gptr->GroupName));
	    OnSelchangeComboGroups();
	    return (-1);
	}

	gptr = gptr->next;
    }

    return CPropertyPage::OnWizardNext();
}

void
CPersonalCounter::OnItemchangedCounterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_ITEM		lvi;

    ObjCtrIndex = m_CounterList.GetSelectionMark();

    /* Unset any item set in the other list box. */
    if (PersonalCtrIndex != -1)
    {
	lvi.mask=LVIF_STATE;
	lvi.iItem=PersonalCtrIndex;
	lvi.iSubItem=0;
	lvi.stateMask=0xFFFF;	/* get all state flags */
	m_PersonalCtrList.GetItem(&lvi);

	lvi.state &= ~(LVIS_SELECTED);
	m_PersonalCtrList.SetItem(&lvi);

	PersonalCtrIndex = -1;
    }

    *pResult = 0;
}

void
CPersonalCounter::OnItemchangedPersonalCounterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_ITEM		lvi;

    PersonalCtrIndex = m_PersonalCtrList.GetSelectionMark();

    /* Unset any item set in the other list box. */
    if (ObjCtrIndex != -1)
    {
	lvi.mask=LVIF_STATE;
	lvi.iItem=ObjCtrIndex;
	lvi.iSubItem=0;
	lvi.stateMask=0xFFFF;	/* get all state flags */
	m_CounterList.GetItem(&lvi);

	lvi.state &= ~(LVIS_SELECTED);
	m_CounterList.SetItem(&lvi);

	ObjCtrIndex = -1;
    }

    *pResult = 0;
}
