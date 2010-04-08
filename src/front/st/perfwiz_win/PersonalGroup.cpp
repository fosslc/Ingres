/*
**
**  Name: PersonalGroup.cpp
**
**  Description:
**	This file contains the routines necessary for the Personal Group
**	Property Page, which allows users to enter personal groups.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
**	15-oct-1999 (somsa01)
**	    The Server selection page is now the final page.
**	18-oct-1999 (somsa01)
**	    Fixed typos in ReadInitFile().
**	25-oct-1999 (somsa01)
**	    Added the ability to read in personal counters. Also, cleaned
**	    up counter id setup.
**	28-jan-2000 (somsa01)
**	    In ReadInitFile(), if the Ingres performance stuff is not in
**	    the registry, then add it.
**	07-May-2008 (drivi01)
**	    Apply Wizard97 template to this utility.
**	29-May-2008 (whiro01) Bug 119642, 115719; SD issue 122646
**	    Make ReadInitFile also register our counter DLL if necessary.
*/

#include "stdafx.h"
#include "perfwiz.h"
#include "PersonalGroup.h"
#include "EditObject.h"
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int  GroupIndex;
static void InitPersonalCounters(grouplist *glptr);

int  ReadInitFile();
void UpdateCounterIDs();

/*
** CPersonalGroup property page
*/
IMPLEMENT_DYNCREATE(CPersonalGroup, CPropertyPage)

CPersonalGroup::CPersonalGroup() : CPropertyPage(CPersonalGroup::IDD)
{
    //{{AFX_DATA_INIT(CPersonalGroup)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CPersonalGroup::CPersonalGroup(CPropertySheet *ps) : CPropertyPage(CPersonalGroup::IDD)
{
    m_propertysheet = ps;
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CPersonalGroup::~CPersonalGroup()
{
}

void CPersonalGroup::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPersonalGroup)
    DDX_Control(pDX, IDC_PERSONAL_GROUP_LIST, m_PersonalGroupList);
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPersonalGroup, CPropertyPage)
    //{{AFX_MSG_MAP(CPersonalGroup)
    ON_BN_CLICKED(IDC_BUTTON_ADD_GROUP, OnButtonAddGroup)
    ON_BN_CLICKED(IDC_BUTTON_DELETE_GROUP, OnButtonDeleteGroup)
    ON_BN_CLICKED(IDC_BUTTON_EDIT_GROUP, OnButtonEditGroup)
    ON_NOTIFY(LVN_KEYDOWN, IDC_PERSONAL_GROUP_LIST, OnKeydownPersonalGroupList)
    ON_NOTIFY(NM_DBLCLK, IDC_PERSONAL_GROUP_LIST, OnDblclkPersonalGroupList)
    ON_WM_DRAWITEM()
    ON_NOTIFY(NM_CLICK, IDC_PERSONAL_GROUP_LIST, OnClickPersonalGroupList)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_PERSONAL_GROUP_LIST, OnItemchangedPersonalGroupList)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CPersonalGroup message handlers
*/
BOOL
CPersonalGroup::OnSetActive() 
{
    m_PersonalGroupList.DeleteAllItems();
    GroupIndex = -1;

    if (GroupList)
    {
	LV_ITEM	lvitem;
	struct grouplist    *gptr = GroupList;

	while (gptr)
	{
	    lvitem.iItem = m_PersonalGroupList.GetItemCount();
	    lvitem.mask = LVIF_TEXT;
	    lvitem.iSubItem = 0;
	    lvitem.pszText = gptr->GroupName;
	    lvitem.iItem = m_PersonalGroupList.InsertItem(&lvitem);

	    gptr = gptr->next;
	}
    }

    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
    return CPropertyPage::OnSetActive();
}

LRESULT
CPersonalGroup::OnWizardBack() 
{
    GroupIndex = -1;
    return IDD_INTRO_PAGE;
}

void
CPersonalGroup::InitListViewControl()
{
    LV_COLUMN  col;
    CString ColumnHeader;
    RECT rc;

    ColumnHeader.LoadString(IDS_PGLIST_COLUMN);
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    m_PersonalGroupList.GetClientRect(&rc);
    col.cx = rc.right;
    col.fmt = LVCFMT_LEFT;
    col.iSubItem = 0;
    col.pszText = ColumnHeader.GetBuffer(256);
    m_PersonalGroupList.InsertColumn(0, &col);
}

BOOL
CPersonalGroup::OnInitDialog() 
{
    CString	Message;

    CPropertyPage::OnInitDialog();
    
    Message.LoadString(IDS_PERSONAL_GROUP_HEADING);
    SetDlgItemText(IDC_PERSONAL_GROUP_HEADING, Message);
    Message.LoadString(IDS_BUTTON_ADD_GROUP);
    SetDlgItemText(IDC_BUTTON_ADD_GROUP, Message);
    Message.LoadString(IDS_BUTTON_EDIT_GROUP);
    SetDlgItemText(IDC_BUTTON_EDIT_GROUP, Message);
    Message.LoadString(IDS_BUTTON_DELETE_GROUP);
    SetDlgItemText(IDC_BUTTON_DELETE_GROUP, Message);

    InitListViewControl();
    GroupIndex = -1;

    if (GroupList)
    {
	LV_ITEM	lvitem;
	struct grouplist    *gptr = GroupList;

	while (gptr)
	{
	    lvitem.iItem = m_PersonalGroupList.GetItemCount();
	    lvitem.mask = LVIF_TEXT;
	    lvitem.iSubItem = 0;
	    lvitem.pszText = gptr->GroupName;
	    lvitem.iItem = m_PersonalGroupList.InsertItem(&lvitem);

	    gptr = gptr->next;
	}
    }

    return TRUE;
}

void
CPersonalGroup::OnButtonAddGroup() 
{
    CEditObject		eoDlg;
    LV_ITEM		lvitem;
    LVFINDINFO		info;
    CString		GroupName, Message, Message2;
    struct grouplist	*gptr, *node;

    eoDlg.m_ObjectDescription = eoDlg.m_ObjectName = CString("");
    while (eoDlg.DoModal() == IDOK)
    {
	info.flags = LVFI_STRING;
	info.psz = eoDlg.m_ObjectName.GetBuffer(0);
	if (m_PersonalGroupList.FindItem(&info) != -1)
	{
	    /*
	    ** Duplicate Group Name!
	    */
	    Message.LoadString(IDS_DUPLICATE_GROUP);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONINFORMATION);
	    continue;
	}

	node = (struct grouplist *)malloc(sizeof(struct grouplist));
	node->next = NULL;
	node->NumCtrsSelected = 0;
	node->PersCtrs = NULL;
	InitPersonalCounters(node);

	node->GroupDescription = (char *)malloc(
				 eoDlg.m_ObjectDescription.GetLength()+1);
	strcpy(node->GroupDescription, eoDlg.m_ObjectDescription);
	node->GroupName = (char *)malloc(
			  eoDlg.m_ObjectName.GetLength()+1);
	strcpy(node->GroupName, eoDlg.m_ObjectName);

	if (!GroupList)
	    GroupList = node;
	else
	{
	    gptr = GroupList;
	    while (gptr->next)
		gptr = gptr->next;
	    gptr->next = node;
	}

	lvitem.iItem = m_PersonalGroupList.GetItemCount();
	lvitem.mask = LVIF_TEXT;
	lvitem.iSubItem = 0;
	lvitem.pszText = node->GroupName;
	lvitem.iItem = m_PersonalGroupList.InsertItem(&lvitem);

	break;
    }
}

void
CPersonalGroup::OnButtonDeleteGroup() 
{
    CString		Group;
    struct grouplist	*gptr, *gptr2;
  
    if (GroupIndex != -1)
    {
        Group = m_PersonalGroupList.GetItemText(GroupIndex, 0);
	gptr = GroupList;
	gptr2 = NULL;
	while (strcmp(Group, gptr->GroupName))
	{
	    gptr2 = gptr;
	    gptr = gptr->next;
	}

	if (gptr2)
	    gptr2->next = gptr->next;
	else
	    GroupList = gptr->next;

	/*
	** Free up its members.
	*/
	FreePersonalCounters(gptr);
	if (gptr->GroupDescription)
	    free(gptr->GroupDescription);
	if (gptr->GroupName)
	    free(gptr->GroupName);
	free(gptr);

	m_PersonalGroupList.DeleteItem(GroupIndex);
	GroupIndex = -1;
    }
}

void
CPersonalGroup::OnButtonEditGroup() 
{
    CEditObject		eoDlg;
    int			Index;
    struct grouplist	*gptr;
    CString		Group, Message, Message2;
    LVFINDINFO		info;
  
    if (GroupIndex != -1)
    {
	Group = m_PersonalGroupList.GetItemText(GroupIndex, 0);
	gptr = GroupList;
	while (strcmp(gptr->GroupName, Group))
	    gptr = gptr->next;

	eoDlg.m_ObjectDescription = gptr->GroupDescription;
	eoDlg.m_ObjectName = gptr->GroupName;
	while (eoDlg.DoModal() == IDOK)
	{
	    info.flags = LVFI_STRING;
	    info.psz = eoDlg.m_ObjectName.GetBuffer(0);
	    if (((Index = m_PersonalGroupList.FindItem(&info)) != -1) &&
		(Index != GroupIndex))
	    {
		/*
		** Duplicate Group Name!
		*/
		Message.LoadString(IDS_DUPLICATE_GROUP);
		Message2.LoadString(IDS_TITLE);
		MessageBox(Message, Message2, MB_OK | MB_ICONINFORMATION);
		eoDlg.m_ObjectName = gptr->GroupName;
		continue;
	    }

	    /*
	    ** Reset the name and description.
	    */
	    free(gptr->GroupDescription);
	    gptr->GroupDescription = (char *)malloc(
				     eoDlg.m_ObjectDescription.GetLength()+1);
	    strcpy(gptr->GroupDescription, eoDlg.m_ObjectDescription);

	    free(gptr->GroupName);
	    gptr->GroupName = (char *)malloc(
			      eoDlg.m_ObjectName.GetLength()+1);
	    strcpy(gptr->GroupName, eoDlg.m_ObjectName);

	    m_PersonalGroupList.SetItemText(GroupIndex, 0, gptr->GroupName);

	    break;
	}
    }
}

void
CPersonalGroup::OnKeydownPersonalGroupList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;

    if (pLVKeyDow->wVKey == VK_DELETE)
	OnButtonDeleteGroup();
    
    *pResult = 0;
}

void
CPersonalGroup::OnDblclkPersonalGroupList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    OnButtonEditGroup();

    *pResult = 0;
}

LPCTSTR
CPersonalGroup::MakeShortString(CDC *pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
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
CPersonalGroup::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
    int			m_cxClient=0, nColumn, m_cxStateImageOffset=0, nRetLen;
    BOOL		m_bClientWidthSel=TRUE;
    COLORREF		m_inactive=::GetSysColor(COLOR_INACTIVEBORDER);
    COLORREF		m_clrTextBk=::GetSysColor(COLOR_WINDOW);
    COLORREF		m_clrBkgnd=::GetSysColor(COLOR_WINDOW);
    CListCtrl		&ListCtrl=m_PersonalGroupList;
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
CPersonalGroup::OnClickPersonalGroupList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_HITTESTINFO FAR	pinfo;
    DWORD		dwpos;
    POINT		point;

    /* Find out where the cursor was */
    dwpos = GetMessagePos();

    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_PersonalGroupList.m_hWnd, &point, 1);
    pinfo.pt = point;

    m_PersonalGroupList.HitTest(&pinfo);
    if (pinfo.flags & LVHT_ONITEM)
	GroupIndex = pinfo.iItem;
    *pResult = 0;
}

LRESULT
CPersonalGroup::OnWizardNext() 
{
    CString Message, Message2;

    if (!m_PersonalGroupList.GetItemCount())
    {
	Message.LoadString(IDS_NO_GROUPS);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	return(-1);
    }

    return CPropertyPage::OnWizardNext();
}

void
FreePersonalCounters(struct grouplist *glptr)
{
    int	i;

    for (i=0; i<NUM_PERSONAL_COUNTERS; i++)
    {
	free(glptr->CounterList[i].CounterName);
	free(glptr->CounterList[i].PCounterName);
	free(glptr->CounterList[i].CounterHelp);
	free(glptr->CounterList[i].UserName);
	free(glptr->CounterList[i].UserHelp);
	free(glptr->CounterList[i].DefineName);
    }
}

static void
InitPersonalCounters(grouplist *glptr)
{
    int	    i, GroupIndex;
    CString Name;

    glptr->CounterID = CounterID;
    for (i=0; i<NUM_PERSONAL_COUNTERS; i++)
    {
	glptr->CounterList[i] = PersonalCtr_Init[i];

	glptr->CounterList[i].DefineName = (char *)malloc(
		strlen(PersonalCtr_Init[i].DefineName)+1);
	strcpy(glptr->CounterList[i].DefineName, PersonalCtr_Init[i].DefineName);
	glptr->CounterList[i].CounterID = CounterID;

	switch (glptr->CounterList[i].classid)
	{
	    case CacheClass:
		GroupIndex = Num_Cache_Counters * glptr->CounterList[i].groupidx +
			     glptr->CounterList[i].ctrid;
		Name = CString(GroupNames[glptr->CounterList[i].groupid].Name) +
		       CString(" - ") +
		       CString(CacheGroup[GroupIndex].Name);
		glptr->CounterList[i].PCounterName = (char *)malloc(Name.GetLength()+1);
		strcpy(glptr->CounterList[i].PCounterName, Name);

		glptr->CounterList[i].UserName = (char *)malloc(Name.GetLength()+1);
		strcpy(glptr->CounterList[i].UserName, Name);

		glptr->CounterList[i].CounterName = (char *)malloc(
			    strlen(CacheGroup[GroupIndex].Name)+1);
		strcpy( glptr->CounterList[i].CounterName,
			CacheGroup[GroupIndex].Name );

		glptr->CounterList[i].CounterHelp = (char *)malloc(
			    strlen(CacheGroup[GroupIndex].Help)+1);
		strcpy( glptr->CounterList[i].CounterHelp,
			CacheGroup[GroupIndex].Help );

		glptr->CounterList[i].UserHelp = (char *)malloc(
			    strlen(CacheGroup[GroupIndex].Help)+1);
		strcpy( glptr->CounterList[i].UserHelp,
			CacheGroup[GroupIndex].Help );
		break;

	    case LockClass:
		GroupIndex = Num_Lock_Counters * glptr->CounterList[i].groupidx +
			     glptr->CounterList[i].ctrid;
		Name = CString(GroupNames[glptr->CounterList[i].groupid].Name) +
		       CString(" - ") +
		       CString(LockGroup[GroupIndex].Name);
		glptr->CounterList[i].PCounterName = (char *)malloc(Name.GetLength()+1);
		strcpy(glptr->CounterList[i].PCounterName, Name);

		glptr->CounterList[i].UserName = (char *)malloc(Name.GetLength()+1);
		strcpy(glptr->CounterList[i].UserName, Name);

		glptr->CounterList[i].CounterName = (char *)malloc(
			    strlen(LockGroup[GroupIndex].Name)+1);
		strcpy( glptr->CounterList[i].CounterName,
			LockGroup[GroupIndex].Name );

		glptr->CounterList[i].CounterHelp = (char *)malloc(
			    strlen(LockGroup[GroupIndex].Help)+1);
		strcpy( glptr->CounterList[i].CounterHelp,
			LockGroup[GroupIndex].Help );

		glptr->CounterList[i].UserHelp = (char *)malloc(
			    strlen(LockGroup[GroupIndex].Help)+1);
		strcpy( glptr->CounterList[i].UserHelp,
			LockGroup[GroupIndex].Help );
		break;

	    case ThreadClass:
		GroupIndex = Num_Thread_Counters * glptr->CounterList[i].groupidx +
			     glptr->CounterList[i].ctrid;
		Name = CString(GroupNames[glptr->CounterList[i].groupid].Name) +
		       CString(" - ") +
		       CString(ThreadGroup[GroupIndex].Name);
		glptr->CounterList[i].PCounterName = (char *)malloc(Name.GetLength()+1);
		strcpy(glptr->CounterList[i].PCounterName, Name);

		glptr->CounterList[i].UserName = (char *)malloc(Name.GetLength()+1);
		strcpy(glptr->CounterList[i].UserName, Name);

		glptr->CounterList[i].CounterName = (char *)malloc(
			    strlen(ThreadGroup[GroupIndex].Name)+1);
		strcpy( glptr->CounterList[i].CounterName,
			ThreadGroup[GroupIndex].Name );

		glptr->CounterList[i].CounterHelp = (char *)malloc(
			    strlen(ThreadGroup[GroupIndex].Help)+1);
		strcpy( glptr->CounterList[i].CounterHelp,
			ThreadGroup[GroupIndex].Help );

		glptr->CounterList[i].UserHelp = (char *)malloc(
			    strlen(ThreadGroup[GroupIndex].Help)+1);
		strcpy( glptr->CounterList[i].UserHelp,
			ThreadGroup[GroupIndex].Help );
		break;

	    case SamplerClass:
		GroupIndex = Num_Sampler_Counters * glptr->CounterList[i].groupidx +
			     glptr->CounterList[i].ctrid;
		Name = CString(GroupNames[glptr->CounterList[i].groupid].Name) +
		       CString(" - ") +
		       CString(SamplerGroup[GroupIndex].Name);
		glptr->CounterList[i].PCounterName = (char *)malloc(Name.GetLength()+1);
		strcpy(glptr->CounterList[i].PCounterName, Name);

		glptr->CounterList[i].UserName = (char *)malloc(Name.GetLength()+1);
		strcpy(glptr->CounterList[i].UserName, Name);

		glptr->CounterList[i].CounterName = (char *)malloc(
			    strlen(SamplerGroup[GroupIndex].Name)+1);
		strcpy( glptr->CounterList[i].CounterName,
			SamplerGroup[GroupIndex].Name );

		glptr->CounterList[i].CounterHelp = (char *)malloc(
			    strlen(SamplerGroup[GroupIndex].Help)+1);
		strcpy( glptr->CounterList[i].CounterHelp,
			SamplerGroup[GroupIndex].Help );

		glptr->CounterList[i].UserHelp = (char *)malloc(
			    strlen(SamplerGroup[GroupIndex].Help)+1);
		strcpy( glptr->CounterList[i].UserHelp,
			SamplerGroup[GroupIndex].Help );
		break;
	}
    }
}

/*
**  Name: RunProcess()
**
**  Description:
**	Runs the process given by the CmdLine parameter and returns status.
**
**  Errors:
**	0   - Success.
**	n   - Whatever the process exit code was.
*/
static DWORD
RunProcess(CString &CmdLine)
{
	STARTUPINFO		siStInfo;
	PROCESS_INFORMATION	piProcInfo;
	DWORD			status = 0;

	memset (&siStInfo, 0, sizeof (siStInfo)) ;

	siStInfo.cb          = sizeof (siStInfo);
	siStInfo.lpReserved  = NULL;
	siStInfo.lpReserved2 = NULL;
	siStInfo.cbReserved2 = 0;
	siStInfo.lpDesktop   = NULL;
	siStInfo.dwFlags     = STARTF_USESHOWWINDOW;
	siStInfo.wShowWindow = SW_HIDE;

	CreateProcess(NULL, CmdLine.GetBuffer(0), NULL, NULL, TRUE, 0,
				  NULL, NULL, &siStInfo, &piProcInfo);
	WaitForSingleObject(piProcInfo.hProcess, INFINITE);
	GetExitCodeProcess(piProcInfo.hProcess, &status);

	return status;
}

/*
**  Name: ReadInitFile()
**
**  Description:
**	Reads in the oipfctrs.ini and oipfpers.ini files.
**
**  Errors:
**	0   - Success.
**	1   - Error reading registry.
**	2   - Error opening oipfctrs.ini file.
*/

int
ReadInitFile()
{
    int	    i, j, Index, CounterIndex, NumPersonal=0;
    int	    CacheIndex=-1, LockIndex=-1, ThreadIndex=-1, SamplerIndex=-1;
    FILE    *fptr, *fptr2;
    CString filesloc = CString(getenv("II_SYSTEM")) + CString("\\ingres\\files\\");
    CString loc = filesloc + CString("oipfctrs.ini");
    CString CounterName;
    CString CmdLine;
    char    buffer[2048], buffer2[2048], *bptr;
    HKEY    hKeyDriverPerf;
    DWORD   size, type, dwFirstCounter=-1;
    char    *PersCtrs = NULL, *cptr, *cptr2, *cptr3, idx[33];
    char    *GroupName, *CtrName;
    BOOL    Found;
    DWORD   status;
    BOOL    didload = FALSE;

    /*
    ** First, get our starting counter id.
    */
    while (RegOpenKeyEx (
	HKEY_LOCAL_MACHINE,
	PERFCTRS_KEY,
	0L,
	KEY_READ,
	&hKeyDriverPerf) != ERROR_SUCCESS)
    {
	if (didload)	/* Don't try this more than once */
	    return(1);

	// We probably don't even have the registry entries yet, so load them
	CmdLine = CString("regedit /s ") + filesloc + CString("oipfctrs.reg");
	status = RunProcess(CmdLine);
	didload = TRUE;
    }
    size = sizeof (DWORD);
    RegQueryValueEx(
            hKeyDriverPerf,
            "First Counter",
            0L,
            &type,
            (LPBYTE)&dwFirstCounter,
            &size);

	if (dwFirstCounter == -1)
	{
		/*
		** Add the Ingres counters to the registry.
		*/
		loc = filesloc + CString("oipfpers.ini");
		if (GetFileAttributes(loc) == -1)
		{
			loc = filesloc + CString("oipfctrs.ini");
		}
		CmdLine = CString("lodctr ") + loc;
		status = RunProcess(CmdLine);
		if (status)
			return(1);

		RegOpenKeyEx (
			HKEY_LOCAL_MACHINE,
			PERFCTRS_KEY,
			0L,
			KEY_READ,
			&hKeyDriverPerf);
		size = sizeof (DWORD);
		RegQueryValueEx(
				hKeyDriverPerf,
				"First Counter",
				0L,
				&type,
				(LPBYTE)&dwFirstCounter,
				&size);
	}

    /*
    ** Retrieve the Personal counter string, if it exists.
    */
    type = REG_MULTI_SZ;
    if (RegQueryValueEx(
            hKeyDriverPerf,
            "Personal Counters",
            0L,
            &type,
            NULL,
            &size) == ERROR_SUCCESS)
    {
	PersCtrs = (char *)malloc(size);
	RegQueryValueEx(
		hKeyDriverPerf,
		"Personal Counters",
		0L,
		&type,
		(LPBYTE)PersCtrs,
		&size);
    }

    RegCloseKey (hKeyDriverPerf);

    if (dwFirstCounter == -1)
    {
	while (!MainWnd);
	::SendMessage(MainWnd, WM_CLOSE, 0, 0);
	return(2);
    }

    CounterID = dwFirstCounter - 2;
    Index = CounterIndex = 0;
    if ((fptr = fopen(loc, "r")) == NULL)
    {
	while (!MainWnd);
	::SendMessage(MainWnd, WM_CLOSE, 0, 0);
	return(3);
    }

    while (fgets(buffer, sizeof(buffer), fptr))
    {
	if (!strstr(buffer, "_OBJ_"))
	    continue;

	/*
	** This is the group name.
	*/
	CounterID += 2;
	buffer[strlen(buffer)-1] = '\0';
	bptr = strstr(buffer, "=");
	bptr++;
	GroupNames[Index].Name = (char *)malloc(strlen(bptr)+1);
	strcpy(GroupNames[Index].Name, bptr);
	GroupNames[Index].CounterID = CounterID;

	if (strstr(buffer, "_CACHE_"))
	{
	    CacheIndex++;
	    GroupNames[Index].classid = CacheClass;
	}
	else if (strstr(buffer, "_LOCK_"))
	{
	    LockIndex++;
	    GroupNames[Index].classid = LockClass;
	}
	else if (strstr(buffer, "_THREAD_"))
	{
	    ThreadIndex++;
	    GroupNames[Index].classid = ThreadClass;
	}
	else if (strstr(buffer, "_SAMP_"))
	{
	    SamplerIndex++;
	    GroupNames[Index].classid = SamplerClass;
	}

	/*
	** This is the help for the group.
	*/
	fgets(buffer, sizeof(buffer), fptr);
	buffer[strlen(buffer)-1] = '\0';
	bptr = strstr(buffer, "=");
	bptr++;
	GroupNames[Index].Help = (char *)malloc(strlen(bptr)+1);
	strcpy(GroupNames[Index].Help, bptr);

	/*
	** Now, get all the counters for the group. Each group is separated
	** by a blank line.
	*/
	i = 0;
	while (fgets(buffer, sizeof(buffer), fptr) && strcmp(buffer, "\n"))
	{
	    /*
	    ** First, intitialize the personal counter initializer
	    ** for this counter.
	    */
	    PersonalCtr_Init[CounterIndex].CounterHelp =
		PersonalCtr_Init[CounterIndex].CounterName =
		PersonalCtr_Init[CounterIndex].PCounterName =
		PersonalCtr_Init[CounterIndex].UserHelp =
		PersonalCtr_Init[CounterIndex].UserName = NULL;
	    PersonalCtr_Init[CounterIndex].selected =
		PersonalCtr_Init[CounterIndex].UserSelected = FALSE;
	    PersonalCtr_Init[CounterIndex].classid = GroupNames[Index].classid;
	    PersonalCtr_Init[CounterIndex].groupid = Index;
	    PersonalCtr_Init[CounterIndex].ctrid = i; 

	    buffer[strlen(buffer)-1] = '\0';
	    bptr = strstr(buffer, "_");
	    strcpy(buffer2, bptr);
	    bptr = strstr(buffer2, "_009_NAME");
	    *bptr = '\0';
	    PersonalCtr_Init[CounterIndex].DefineName =
		    (char *)malloc(strlen(buffer2)+1);
	    strcpy(PersonalCtr_Init[CounterIndex].DefineName, buffer2);

	    bptr = strstr(buffer, "=");
	    bptr++;
	    CounterID += 2;
	    switch (GroupNames[Index].classid)
	    {
		case CacheClass:
		    PersonalCtr_Init[CounterIndex].groupidx = CacheIndex;
		    CacheGroup[Num_Cache_Counters*CacheIndex + i].Name =
			    (char *)malloc(strlen(bptr)+1);
		    strcpy( CacheGroup[Num_Cache_Counters*CacheIndex + i].Name,
			    bptr );
		    fgets(buffer, sizeof(buffer), fptr);
		    buffer[strlen(buffer)-1] = '\0';
		    bptr = strstr(buffer, "=");
		    bptr++;
		    CacheGroup[Num_Cache_Counters*CacheIndex + i].Help =
			    (char *)malloc(strlen(bptr)+1);
		    strcpy( CacheGroup[Num_Cache_Counters*CacheIndex + i].Help,
			    bptr );
		    CacheGroup[Num_Cache_Counters*CacheIndex + i].CounterID =
			    CounterID;
		    CacheGroup[Num_Cache_Counters*CacheIndex + i].UserSelected =
			    FALSE;
		    break;

		case LockClass:
		    PersonalCtr_Init[CounterIndex].groupidx = LockIndex;
		    LockGroup[Num_Lock_Counters*LockIndex + i].Name =
			    (char *)malloc(strlen(bptr)+1);
		    strcpy(LockGroup[Num_Lock_Counters*LockIndex + i].Name, bptr);
		    fgets(buffer, sizeof(buffer), fptr);
		    buffer[strlen(buffer)-1] = '\0';
		    bptr = strstr(buffer, "=");
		    bptr++;
		    LockGroup[Num_Lock_Counters*LockIndex + i].Help =
			    (char *)malloc(strlen(bptr)+1);
		    strcpy(LockGroup[Num_Lock_Counters*LockIndex + i].Help, bptr);
		    LockGroup[Num_Lock_Counters*LockIndex + i].CounterID =
			    CounterID;
		    LockGroup[Num_Lock_Counters*LockIndex + i].UserSelected =
			    FALSE;
		    break;

		case ThreadClass:
		    PersonalCtr_Init[CounterIndex].groupidx = ThreadIndex;
		    ThreadGroup[Num_Thread_Counters*ThreadIndex + i].Name =
			    (char *)malloc(strlen(bptr)+1);
		    strcpy( ThreadGroup[Num_Thread_Counters*ThreadIndex + i].Name,
			    bptr );
		    fgets(buffer, sizeof(buffer), fptr);
		    buffer[strlen(buffer)-1] = '\0';
		    bptr = strstr(buffer, "=");
		    bptr++;
		    ThreadGroup[Num_Thread_Counters*ThreadIndex + i].Help =
			    (char *)malloc(strlen(bptr)+1);
		    strcpy( ThreadGroup[Num_Thread_Counters*ThreadIndex + i].Help,
			    bptr );
		    ThreadGroup[Num_Thread_Counters*ThreadIndex + i].CounterID =
			    CounterID;
		    ThreadGroup[Num_Thread_Counters*ThreadIndex + i].UserSelected =
			    FALSE;
		    break;

		case SamplerClass:
		    PersonalCtr_Init[CounterIndex].groupidx = SamplerIndex;
		    SamplerGroup[Num_Sampler_Counters*SamplerIndex + i].Name =
			    (char *)malloc(strlen(bptr)+1);
		    strcpy( SamplerGroup[Num_Sampler_Counters*SamplerIndex + i].Name,
			    bptr );
		    fgets(buffer, sizeof(buffer), fptr);
		    buffer[strlen(buffer)-1] = '\0';
		    bptr = strstr(buffer, "=");
		    bptr++;
		    SamplerGroup[Num_Sampler_Counters*SamplerIndex + i].Help =
			    (char *)malloc(strlen(bptr)+1);
		    strcpy( SamplerGroup[Num_Sampler_Counters*SamplerIndex + i].Help,
			    bptr );
		    SamplerGroup[Num_Sampler_Counters*SamplerIndex + i].CounterID =
			    CounterID;
		    SamplerGroup[Num_Sampler_Counters*SamplerIndex + i].UserSelected =
			    FALSE;
		    break;
	    }

	    i++;
	    CounterIndex++;
	}

	Index++;
    }

    fclose(fptr);

    PersonalCounterID = CounterID;

    /*
    ** If there were any personal groups created before, read them in.
    */
    loc = filesloc + CString("oipfpers.ini");
    if (GetFileAttributes(loc) != -1)
    {
	struct grouplist    *node, *gptr;
	int		    id;

	fptr = fopen(loc, "r");
	while (fgets(buffer, sizeof(buffer), fptr))
	{
	    if (!strstr(buffer, "PFCTRS_PERS"))
		continue;
	    if (!strstr(buffer, "_OBJ_"))
		continue;

	    /*
	    ** First, add the group to the master list.
	    */
	    buffer[strlen(buffer)-1] = '\0';
	    bptr = strstr(buffer, "=");
	    bptr++;

	    node = (struct grouplist *)malloc(sizeof(struct grouplist));
	    node->next = NULL;
	    node->NumCtrsSelected = 0;
	    node->PersCtrs = NULL;
	    InitPersonalCounters(node);

	    node->GroupName = (char *)malloc(strlen(bptr)+1);
	    strcpy(node->GroupName, bptr);

	    fgets(buffer, sizeof(buffer), fptr);
	    buffer[strlen(buffer)-1] = '\0';
	    bptr = strstr(buffer, "=");
	    bptr++;
	    node->GroupDescription = (char *)malloc(strlen(bptr)+1);
	    strcpy(node->GroupDescription, bptr);

	    if (!GroupList)
		GroupList = node;
	    else
	    {
		gptr = GroupList;
		while (gptr->next)
		    gptr = gptr->next;
		gptr->next = node;
	    }

	    /*
	    ** Now, turn on the specified counters for the group.
	    */
	    while (fgets(buffer, sizeof(buffer), fptr) && strcmp(buffer, "\n"))
	    {
		/*
		** First, get the define name.
		*/
		buffer[strlen(buffer)-1] = '\0';
		bptr = strstr(buffer, "=");
		bptr++;
		CounterName = CString(bptr);

		bptr = strstr(buffer, "_009_NAME");
		*bptr = '\0';
		loc = filesloc + CString("oipfpers.h");
		fptr2 = fopen(loc, "r");
		while (fgets(buffer2, sizeof(buffer2), fptr2))
		{
		    if (strstr(buffer2, buffer))
			break;
		}
		fclose(fptr2);

		buffer2[strlen(buffer2)-1] = '\0';
		bptr = &buffer2[strlen(buffer2)];
		while (*bptr != ' ')
		    bptr--;

		/* Adjust the index for the current personal group. */
		id = (atoi(++bptr) - Num_Groups*2 - NUM_PERSONAL_COUNTERS*2 - 
		      (NumPersonal*NUM_PERSONAL_COUNTERS*2))/2 -
		      NumPersonal - 1;
		if (id < NUM_PERSONAL_COUNTERS)
		{
		    free(node->CounterList[id].UserName);
		    node->CounterList[id].UserName =
			    (char *)malloc(CounterName.GetLength()+1);
		    strcpy(node->CounterList[id].UserName, CounterName);

		    /*
		    ** Get the help for the counter.
		    */
		    fgets(buffer, sizeof(buffer), fptr);
		    buffer[strlen(buffer)-1] = '\0';
		    bptr = strstr(buffer, "=");
		    bptr++;
		    free(node->CounterList[id].UserHelp);
		    node->CounterList[id].UserHelp =
			    (char *)malloc(strlen(buffer)+1);
		    strcpy(node->CounterList[id].UserHelp, bptr);

		    node->CounterList[id].selected = TRUE;
		}
		else
		{
		    struct PersCtrList	*PCtrNode;

		    /*
		    ** This is a personal counter.
		    */
		    PCtrNode = (struct PersCtrList *)malloc(
				    sizeof(struct PersCtrList));
		    PCtrNode->next = NULL;

		    bptr = strstr(buffer, "_");
		    bptr++;
		    strcpy(buffer2, bptr);
		    bptr = strstr(buffer2, "_");
		    PCtrNode->PersCtr.DefineName = (char *)malloc(strlen(bptr)+1);
		    strcpy(PCtrNode->PersCtr.DefineName, bptr);
		    PCtrNode->PersCtr.UserSelected = FALSE;
		    PCtrNode->PersCtr.classid = PersonalClass;
		    PCtrNode->PersCtr.groupid = -1;
		    PCtrNode->PersCtr.ctrid = -1;
		    PCtrNode->PersCtr.CounterID = -1;

		    PCtrNode->PersCtr.PCounterName =
			    (char *)malloc(CounterName.GetLength()+1);
		    strcpy(PCtrNode->PersCtr.PCounterName, CounterName);
		    PCtrNode->PersCtr.CounterName =
			    (char *)malloc(CounterName.GetLength()+1);
		    strcpy(PCtrNode->PersCtr.CounterName, CounterName);
		    PCtrNode->PersCtr.UserName =
			    (char *)malloc(CounterName.GetLength()+1);
		    strcpy(PCtrNode->PersCtr.UserName, CounterName);

		    /*
		    ** Retrieve the personal counter information from the
		    ** registry. The format is:
		    ** "GroupName;GroupID;CounterName;DefaultScale;Database;Query;"
		    */
		    cptr = PersCtrs;
		    Found = FALSE;
		    while (*cptr != '\0' && !Found)
		    {
			cptr2 = cptr;
			j = 0;
			while (*cptr2 != ';')
			{
			    j++;
			    cptr2++;
			}

			GroupName = (char *)malloc(j+1);
			cptr2 = cptr;
			j = 0;
			while (*cptr2 != ';')
			{
			    GroupName[j] = *cptr2;
			    j++;
			    cptr2++;
			}
			GroupName[j] = '\0';
			cptr2++;
			cptr = cptr2;

			if (!strcmp(GroupName, node->GroupName))
			{
			    /* Ignore the GroupID. */
			    cptr2 = strchr(cptr, ';');
			    cptr2++;

			    j = 0;
			    while (*cptr2 != ';')
			    {
				j++;
				cptr2++;
			    }

			    CtrName = (char *)malloc(j+1);
			    cptr2 = strchr(cptr, ';');
			    cptr2++;
			    j = 0;
			    while (*cptr2 != ';')
			    {
				CtrName[j] = *cptr2;
				j++;
				cptr2++;
			    }
			    CtrName[j] = '\0';

			    if (!strcmp(CtrName,
				PCtrNode->PersCtr.CounterName))
			    {
				Found = TRUE;

				/* Get the default scale. */
				cptr2++;
				j = 0;
				while (*cptr2 != ';')
				{
				    idx[j] = *cptr2;
				    j++;
				    cptr2++;
				}
				idx[j] = '\0';
				PCtrNode->scale = atoi(idx);

				/* Get the database name. */
				j = 0;
				cptr2++;
				cptr3 = cptr2;
				while (*cptr3 != ';')
				{
				    j++;
				    cptr3++;
				}
				PCtrNode->dbname = (char *)malloc(j+1);
				j = 0;
				while (*cptr2 != ';')
				{
				    PCtrNode->dbname[j] = *cptr2;
				    j++;
				    cptr2++;
				}
				PCtrNode->dbname[j] = '\0';

				/* Get the query. */
				j = 0;
				cptr2++;
				cptr3 = cptr2;
				while (*cptr3 != ';')
				{
				    j++;
				    cptr3++;
				}
				PCtrNode->qry = (char *)malloc(j+1);
				j = 0;
				while (*cptr2 != ';')
				{
				    PCtrNode->qry[j] = *cptr2;
				    j++;
				    cptr2++;
				}
				PCtrNode->qry[j] = '\0';
			    }
			}

			cptr2 = strchr(cptr, '\0');
			cptr = cptr2+1;
		    }

		    /*
		    ** Get the help for the counter.
		    */
		    fgets(buffer, sizeof(buffer), fptr);
		    buffer[strlen(buffer)-1] = '\0';
		    bptr = strstr(buffer, "=");
		    bptr++;
		    PCtrNode->PersCtr.CounterHelp =
			    (char *)malloc(strlen(buffer)+1);
		    strcpy(PCtrNode->PersCtr.CounterHelp, bptr);
		    PCtrNode->PersCtr.UserHelp =
			    (char *)malloc(strlen(buffer)+1);
		    strcpy(PCtrNode->PersCtr.UserHelp, bptr);

		    PCtrNode->PersCtr.selected = TRUE;

		    if (!node->PersCtrs)
			node->PersCtrs = PCtrNode;
		    else
		    {
			struct PersCtrList	*PCPtr = node->PersCtrs;

			while (PCPtr->next)
			    PCPtr = PCPtr->next;
			PCPtr->next = PCtrNode;
		    }
		}

		node->NumCtrsSelected++;
	    }

	    NumPersonal++;
	}

	fclose(fptr);
    }

    InitFileRead = TRUE;

    /*
    ** Initially update the counter ids of the personal stuff.
    */
    UpdateCounterIDs();

    Sleep(2000);
    while (!MainWnd);
    ::SendMessage(MainWnd, WM_CLOSE, 0, 0);
    return 0;
}

void
CPersonalGroup::OnItemchangedPersonalGroupList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    GroupIndex = m_PersonalGroupList.GetSelectionMark();
	
    *pResult = 0;
}

void
UpdateCounterIDs()
{
    int			i, Index=PersonalCounterID;
    struct grouplist	*gptr;
    struct PersCtrList	*PCPtr;

    if (GroupList)
    {
	gptr = GroupList;

	while (gptr)
	{
	    Index += 2;
	    gptr->CounterID = Index;
	    for (i=0; i<NUM_PERSONAL_COUNTERS; i++)
	    {
		Index += 2;
		gptr->CounterList[i].CounterID = Index;
	    }

	    if (gptr->PersCtrs)
	    {
		PCPtr = gptr->PersCtrs;

		while (PCPtr)
		{
		    Index += 2;
		    PCPtr->PersCtr.CounterID = Index;
		    PCPtr = PCPtr->next;
		}
	    }

	    gptr = gptr->next;
	}
    }
}
