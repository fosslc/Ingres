/*
**
**  Name: ChooseDir.cpp
**
**  Description:
**	This file contains all functions necessary to implement the directory
**	choosing dialog box.
**
**  History:
**	23-apr-1999 (somsa01)
**	    Created from Enterprise Installer version.
**  28-Sep-2000 (noifr01)
**      (bug 99242) cleanup for dbcs compliance
*/

#include "stdafx.h"
#include "iilink.h"
#include "ChooseDir.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static char *BS="\\";

/*
** CChooseDir dialog
*/
CChooseDir::CChooseDir(CString &s, CWnd* pParent)
: CDialog(CChooseDir::IDD, pParent)
{
    m_title = s;
    m_imageList.Create (IDB_CHOOSEDIR, 17, 4, (COLORREF)RGB(0,0,255));
}

void CChooseDir::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CChooseDir)
    DDX_Control(pDX, IDC_TREE1, m_dirs);
    DDX_Control(pDX, IDC_EDIT1, m_edit);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CChooseDir, CDialog)
    //{{AFX_MSG_MAP(CChooseDir)
    ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE1, OnItemexpandingDirs)
    ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, OnSelchangedDirs)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CChooseDir message handlers
*/
BOOL
CChooseDir::OnInitDialog() 
{
    char	szBuffer[1024], szBuffer2[1024], *cptr, *cptr2;
    CString	s;
    HTREEITEM	hItem, hItem2 = NULL;

    CDialog::OnInitDialog();

    s.LoadString(IDS_CHOOSEDIR_TXT);
    SetDlgItemText(IDC_CHOOSEDIR_TXT, s);
    s.LoadString(IDS_OK);
    SetDlgItemText(IDOK, s);
    s.LoadString(IDS_CANCEL);
    SetDlgItemText(IDCANCEL, s);
    m_dirs.SetImageList(&m_imageList, TVSIL_NORMAL);
    GetWindowText(s); 
    s+=m_title; 
    SetWindowText(s);
    FillDrives();
    if (m_path)
	m_edit.SetWindowText(m_path);
    strcpy (szBuffer, m_path.Left(2));
    hItem = FindInTree(m_dirs.GetRootItem(), szBuffer);
    if (hItem)
	m_dirs.Expand(hItem, TVE_EXPAND);
    else
	m_dirs.Expand(m_dirs.GetRootItem(), TVE_EXPAND);

    /*
    ** If we found the drive, drill down the path given.
    */
    if (hItem)
    {
	strcpy(szBuffer, m_path);
	hItem2 = hItem;
	while ((cptr = _tcsstr(szBuffer, "\\")) != NULL) /* '\\' can be a DBCS trailing byte */
	{
	    cptr++;
	    strcpy(szBuffer2, cptr);
	    if ((cptr2 = _tcsstr(szBuffer2, "\\")) != NULL)  /* '\\' can be a DBCS trailing byte */
		*cptr2 = '\0';
	    hItem = m_dirs.GetChildItem(hItem);
	    hItem = FindInTree(hItem, szBuffer2);
	    if (hItem)
	    {
		m_dirs.Expand(hItem, TVE_EXPAND);
		hItem2 = hItem;
	    }
	    else
		break;
		
	    strcpy(szBuffer, cptr);
	}
    }
    m_dirs.SelectItem(hItem2);

    return TRUE;
}

void
CChooseDir::GetCurPath(CString & path, HTREEITEM hitem)
{
    CStringList	sl;

    path.Empty();	
    for (;hitem; hitem = m_dirs.GetParentItem(hitem))
	sl.AddHead(m_dirs.GetItemText(hitem));
	
    for (POSITION p=sl.GetHeadPosition(); p;)
    {
	CString &s = sl.GetNext(p);
	if (path.IsEmpty())
	    path = s;
	else
	    path = path+BS+s;
    }
}

void
CChooseDir::FillDir(HTREEITEM hParent)
{
    CWaitCursor		wait;
    CString		path;
    WIN32_FIND_DATA	FileData;
    HANDLE		hSearch;

    GetCurPath(path,hParent);
    path += "\\*.*";
    hSearch = FindFirstFile(path, &FileData); 
    if (hSearch != INVALID_HANDLE_VALUE) 
    {
	do
	{
	    if (FileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
	    {
		if (*FileData.cFileName!='.')
		{
		    TV_ITEM	item;

		    item.hItem = m_dirs.InsertItem(FileData.cFileName,hParent);
		    item.mask = TVIF_CHILDREN | TVIF_HANDLE| TVIF_IMAGE|
				TVIF_SELECTEDIMAGE; 
		    item.iImage = item.iSelectedImage = 0;
		    item.cChildren = HasSubDirs(item.hItem) ? 1 : 0;
		    m_dirs.SetItem(&item);
		}
	    }
	} while (FindNextFile(hSearch, &FileData));
    } 
    FindClose(hSearch);
}

void
CChooseDir::OnItemexpandingDirs(NMHDR* pNMHDR, LRESULT* pResult) 
{
    TV_ITEM	 tvi; 
    NM_TREEVIEW	*pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    HTREEITEM	hItem;
	
    if (pNMTreeView->action == TVE_EXPAND)
    {
	if (!m_dirs.GetChildItem(pNMTreeView->itemNew.hItem))
	    FillDir(pNMTreeView->itemNew.hItem);
    }
	
    hItem = m_dirs.GetParentItem(pNMTreeView->itemNew.hItem);

    if (hItem)  /* If it is not a root folder, show open or closed folder */
    {
	tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
	tvi.hItem = (pNMTreeView->itemNew).hItem;
	    
	if (pNMTreeView->action == TVE_COLLAPSE) 
	{ 
	    tvi.iImage = 0;    /* Closed Folder */
	    tvi.iSelectedImage = 0;
	    m_dirs.SetItem(&tvi);
	}
 	
	if (pNMTreeView->action==TVE_EXPAND)
	{
	    tvi.iImage = 1;   /* Open Folder */
	    tvi.iSelectedImage = 1;
	    m_dirs.SetItem(&tvi);
	}
    }      
        
    *pResult = 0;
}

void
CChooseDir::OnOK() 
{
    m_edit.GetWindowText(m_path);
    if (GetFileAttributes(m_path) != -1)
	CDialog::OnOK();
    else
    {
	CString	Message, Message2;

	Message.LoadString(IDS_NOSRC_DIR);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_ICONEXCLAMATION | MB_OK);
    }
}

void CChooseDir::OnSelchangedDirs(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_TREEVIEW	*pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    CString	path;
	
    GetCurPath(path,pNMTreeView->itemNew.hItem);
    if (path.GetLength()<3)
	path += BS;
    m_edit.SetWindowText(path);
    *pResult = 0;
}

BOOL
CChooseDir::HasSubDirs(HTREEITEM hitem)
{
    BOOL		bret = FALSE;
    CString		path;
    WIN32_FIND_DATA	FileData;
    HANDLE		hSearch;

    GetCurPath(path,hitem);
    path += "\\*.*";
    hSearch = FindFirstFile(path, &FileData); 
    if (hSearch != INVALID_HANDLE_VALUE) 
    {
	do
	{
	    if (FileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
	    {
		if (*FileData.cFileName!='.')
		{
		    bret=TRUE;
		}
	    }
	} while (!bret && FindNextFile(hSearch, &FileData));
    } 

    FindClose(hSearch);
    return bret;
}

CChooseDir::~CChooseDir()
{
    m_imageList.DeleteImageList();
}

void
CChooseDir::FillDrives()
{
    CWaitCursor wait;

    TRY
    {
	DWORD		drives = GetLogicalDrives();
	char		ach[16] = "A:\\";
	char		vol[1024], fs[1024], ach2[3];
	int		i, bit;
	DWORD		dw1, dw2, flags;
	BOOL		ret;
	UINT		type;
	TV_ITEM		item;
	HTREEITEM	hItem;

	if (!drives) 
	{
	    EndDialog(IDCANCEL);
	    return;
	}

	for (i='C'; i<='Z'; i++)
	{
	    bit = i-'A';
	    if ((drives>>bit)&1)
	    {
		ach[0] = i;
				
		ret=GetVolumeInformation(ach, vol, sizeof(vol), &dw1, &dw2,
					 &flags, fs, sizeof(fs));		
		if (ret)
		{
		    type = GetDriveType(ach);
		    if (type == DRIVE_REMOVABLE ||
			type == DRIVE_FIXED ||
			type == DRIVE_REMOTE ||
			type == DRIVE_RAMDISK)
		    {
			ach2[0] = ach[0];
			ach2[1] = ach[1];
			ach2[2] = 0;
			item.hItem = m_dirs.InsertItem((LPCSTR)ach2,TVI_ROOT);
			item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_IMAGE |
				    TVIF_SELECTEDIMAGE; 
			switch (type)
			{
			    case DRIVE_REMOVABLE :
				item.iImage = item.iSelectedImage = 4;
				break;
			
			    case DRIVE_FIXED :
				item.iImage = item.iSelectedImage = 2;
				break;

			    case DRIVE_REMOTE: 
				item.iImage = item.iSelectedImage = 3;
				break;

			    case DRIVE_RAMDISK: 
				item.iImage = item.iSelectedImage = 5;
				break;
			}
			item.cChildren = HasSubDirs(item.hItem) ? 1 : 0;
			m_dirs.SetItem(&item);
		    }
		}
	    }
	}

	hItem = m_dirs.GetNextItem(0,TVGN_ROOT);
	m_dirs.SelectItem(hItem);
    }
	
    CATCH( CException, e )
    {
	return;
    }
    END_CATCH			
}

HTREEITEM
CChooseDir::FindInTree(HTREEITEM hRootItem, char *szFindText)
{
    HTREEITEM	hNewItem;
    CString	strItem;

    if ((!hRootItem) || (!szFindText))
	return(NULL);

    hNewItem = hRootItem;
    while (hNewItem)
    {
	strItem = m_dirs.GetItemText(hNewItem);
	if (strItem.CompareNoCase(szFindText) == 0)
	    return hNewItem;
	
	hNewItem = m_dirs.GetNextSiblingItem(hNewItem);
    }

    return NULL;  /* Not Found - returning NULL */
}
