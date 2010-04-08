/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: Dependencies.cpp - Windows Cluster Resource Dependencies property page 
** 
** Description:
** 	The resource dependencies property page displays available resources and 
** 	adds resources to the dependencies of the Ingres Cluster Service resource.
** 
**  History:
**	29-apr-2004 (wonst02)
**	    Created.
** 	03-aug-2004 (wonst02)
** 		Display existing dependencies of the Ingres cluster service if it exists.
**	30-nov-2004 (wonst02) Bug 113345
**	    Remove HELP from the property page.
*/

// Dependencies.cpp : implementation file
//

#include "stdafx.h"
#include "wincluster.h"
#include ".\dependencies.h"


// WcDependencies dialog

IMPLEMENT_DYNAMIC(WcDependencies, CPropertyPage)
WcDependencies::WcDependencies()
	: CPropertyPage(WcDependencies::IDD)
{
    m_psp.dwFlags &= ~(PSP_HASHELP);
}

WcDependencies::~WcDependencies()
{
}

void WcDependencies::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IMAGE, m_image);
	DDX_Control(pDX, IDC_RESOURCE_LIST, m_ResourceListCtrl);
	DDX_Control(pDX, IDC_DEPENDENCY_LIST, m_DependencyListCtrl);
	DDX_Control(pDX, IDC_ADD_BUTTON, m_AddButtonCtrl);
	DDX_Control(pDX, IDC_REMOVE_BUTTON, m_RemoveButtonCtrl);
}


BEGIN_MESSAGE_MAP(WcDependencies, CPropertyPage)
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnBnClickedAddButton)
	ON_BN_CLICKED(IDC_REMOVE_BUTTON, OnBnClickedRemoveButton)
	ON_NOTIFY(NM_DBLCLK, IDC_RESOURCE_LIST, OnNMDblclkResourceList)
	ON_NOTIFY(NM_DBLCLK, IDC_DEPENDENCY_LIST, OnNMDblclkDependencyList)
END_MESSAGE_MAP()


// WcDependencies message handlers

BOOL WcDependencies::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	SetList();

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL WcDependencies::OnSetActive()
{
	property->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

	return CPropertyPage::OnSetActive();
}

//
//	This is a utility function used to move items from the resource list 
//	to the dependency list and visa versa.
//
BOOL MoveItem(int nSelect, CListCtrl &FromListCtrl, CListCtrl &ToListCtrl)
{
	BOOL bResult = FALSE;
	if(nSelect>=0)
	{
		LV_ITEM lvToItem;
		CString szText;

		// Get the item (Name) clicked on:
		szText = FromListCtrl.GetItemText(nSelect, 0);
		// Add the same item (Name) to the other list:
		int i = ToListCtrl.GetItemCount();
		lvToItem.mask=LVIF_TEXT;
		lvToItem.iItem=i;
		lvToItem.iSubItem=0;
		lvToItem.pszText=szText.GetBuffer();
		int index = ToListCtrl.InsertItem(&lvToItem);
		// Get the sub-item (Resource type) of the item clicked on:
		szText = FromListCtrl.GetItemText(nSelect, 1);
		// Add the same sub-item (Resource type) to the other list:
		lvToItem.mask=LVIF_TEXT;
		lvToItem.iItem=index;
		lvToItem.iSubItem=1;
		lvToItem.pszText=szText.GetBuffer();
		ToListCtrl.SetItem(&lvToItem);
		// Delete the item from the list:
		FromListCtrl.DeleteItem(nSelect);
		bResult = TRUE;
	}

	return bResult;
}

//
//	This is a utility function used to find an item in the resource list 
//	or the dependency list.
//
int FindDependency(CString szResource, CListCtrl &ResListCtrl)
{
	for (int nItem = 0; nItem < ResListCtrl.GetItemCount(); nItem++)
	{
		CString szText;

		// Get the item (Name):
		szText = ResListCtrl.GetItemText(nItem, 0);
		if (szText.CompareNoCase(szResource) == 0)
			return nItem;
		// Get the sub-item (Resource type) of the item:
//?		szText = ResListCtrl.GetItemText(nSelect, 1);
	}

	return -1;
}

// Get and display the list of Windows Cluster resources.
void WcDependencies::SetList(void)
{
	LV_ITEM lvitem;
	int index;
	CRect rect;
	CString strName, strType;
	strName.LoadString(IDS_RESOURCENAME);
	strType.LoadString(IDS_RESOURCETYPE);
	
	m_ResourceListCtrl.GetClientRect(&rect);
	m_ResourceListCtrl.InsertColumn(0, strName, LVCFMT_LEFT,
		rect.Width() * 1/2, 0);
	m_ResourceListCtrl.InsertColumn(1, strType, LVCFMT_LEFT,
		rect.Width() * 1/2, 1);
	
	for(int i=0; i<thePreSetup.m_ClusterResources.GetSize(); i++)
	{
		WcClusterResource *resource=(WcClusterResource *) thePreSetup.m_ClusterResources.GetAt(i);
		if(resource &&
			resource->m_name != thePreSetup.m_ResourceName)
		{
			lvitem.mask=LVIF_TEXT;
			lvitem.iItem=i;
			lvitem.iSubItem=0;
			lvitem.pszText=resource->m_name.GetBuffer();
			index = m_ResourceListCtrl.InsertItem(&lvitem);
			
			lvitem.mask=LVIF_TEXT;
			lvitem.iItem=index;
			lvitem.iSubItem=1;
			lvitem.pszText=resource->m_type.GetBuffer();
			m_ResourceListCtrl.SetItem(&lvitem);
		}
	}

	// Also insert the columns of the dependency list.
	m_DependencyListCtrl.GetClientRect(&rect);
	m_DependencyListCtrl.InsertColumn(0, strName, LVCFMT_LEFT,
		rect.Width() * 1/2, 0);
	m_DependencyListCtrl.InsertColumn(1, strType, LVCFMT_LEFT,
		rect.Width() * 1/2, 1);

	GetDependencies();
}

void WcDependencies::GetDependencies(void)
{
	HRESOURCE hRes = OpenClusterResource(thePreSetup.m_hCluster, CT2CW(thePreSetup.m_ResourceName.GetBuffer()));
	if (hRes == NULL)
		return;

    DWORD dwResult = ERROR_SUCCESS;         // Captures return values
    DWORD dwIndex  = 0;                     // Enumeration index
    DWORD dwType   = CLUSTER_RESOURCE_ENUM_DEPENDS; // Type of object to enumerate 
    DWORD cbNameAlloc =                     // Allocated size of the name buffer
		MAXIMUM_NAMELENGTH * sizeof( WCHAR );
    DWORD cchName = 0;        // Size of the resulting name as a count of wchars

    LPWSTR lpszName =                       // Buffer to hold enumerated names
		(LPWSTR) LocalAlloc( LPTR, cbNameAlloc );
    if (lpszName == NULL)
    {
        dwResult = GetLastError();
        goto endf;
    }

	// Open enumeration handle.
    HRESENUM hResEnum = ClusterResourceOpenEnum( hRes, dwType );
    if (hResEnum == NULL)
    {
        dwResult = GetLastError();
        goto endf;
    }

    while(dwResult == ERROR_SUCCESS)
    {
        cchName = cbNameAlloc / sizeof( WCHAR );

        dwResult = ClusterResourceEnum(
			hResEnum,
			dwIndex,
			&dwType,
			lpszName,
			&cchName);

    	// Reallocate the name buffer if not big enough.
		if (dwResult == ERROR_MORE_DATA)
        {
            cchName++;   // Leave room for terminating NULL.
            cbNameAlloc = cchName / sizeof( WCHAR );
            LocalFree( lpszName );
            lpszName = (LPWSTR) LocalAlloc( LPTR, 
                                            cbNameAlloc );
            if ( lpszName == NULL )
            {
                dwResult = GetLastError();
                goto endf;
            }

            dwResult = ClusterResourceEnum( 
				hResEnum, 
				dwIndex,
				&dwType,
				lpszName,
				&cchName );
        }

        if( dwResult == ERROR_SUCCESS ) 
        {
			int nItem = FindDependency(lpszName, m_ResourceListCtrl);
			if (nItem >= 0)
				MoveItem(nItem, /*from*/m_ResourceListCtrl, /*to*/m_DependencyListCtrl);
        }
        else if( dwResult == ERROR_NO_MORE_ITEMS )
        {
			break;
        }
        else // enum failed for another reason
        {
            dwResult = GetLastError();
            goto endf;
        }
      
        dwIndex++;
    } //  End while.

endf:
	CloseClusterResource(hRes);
}

void WcDependencies::OnBnClickedAddButton()
{
	int nSelected=-1;

	while ((nSelected=m_ResourceListCtrl.GetNextItem( -1, LVIS_SELECTED )) > -1)
	{
		MoveItem(nSelected, /*from*/m_ResourceListCtrl, /*to*/m_DependencyListCtrl);
	}
}

void WcDependencies::OnBnClickedRemoveButton()
{
	int nSelected=-1;

	while ((nSelected=m_DependencyListCtrl.GetNextItem( -1, LVIS_SELECTED )) > -1)
	{
		MoveItem(nSelected, /*from*/m_DependencyListCtrl, /*to*/m_ResourceListCtrl);
	}
}

void WcDependencies::OnNMDblclkResourceList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	int nSelect=pNMListView->iItem;
	if (nSelect >= 0)
		MoveItem(nSelect, m_ResourceListCtrl, m_DependencyListCtrl);
	*pResult = 0;
}

void WcDependencies::OnNMDblclkDependencyList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	int nSelect=pNMListView->iItem;
	if(nSelect>=0)
		MoveItem(nSelect, m_DependencyListCtrl, m_ResourceListCtrl);

	*pResult = 0;
}

LRESULT WcDependencies::OnWizardNext()
{
	int nItem=-1;
	TCHAR szName[MAXIMUM_NAMELENGTH], szType[MAXIMUM_NAMELENGTH];
	LPTSTR lpszName = szName, lpszType = szType;
	DWORD cchName = MAXIMUM_NAMELENGTH, cchType = MAXIMUM_NAMELENGTH;
	thePreSetup.EmptyDependencies();

	// Add all dependencies from the dialog control to the dependency list.
	while ((nItem=m_DependencyListCtrl.GetNextItem( nItem, LVNI_ALL )) > -1)
	{
		m_DependencyListCtrl.GetItemText(nItem, 0, lpszName, cchName);
		m_DependencyListCtrl.GetItemText(nItem, 1, lpszType, cchType);
		thePreSetup.AddDependency(lpszName, lpszType);
	}

	return CPropertyPage::OnWizardNext();
}
