/*
**  Copyright (c) 2001 Ingres Corporation
*/

/*
**  Name: msiupd.c
**
**  Description:
**	Update MSI database.
**	
**  History:
**	05-sep-2001 (penga03)
**	    Created.
**	08-nov-2001 (somsa01)
**	    Updated brandings to say "Advantage Ingres".
**	18-feb-2002 (penga03)
**	    Updated brandings to say "Advantage Ingres SDK" for Ingres SDK.
**	26-mar-2002 (penga03)
**	    Set the brandings to empty in those pages with images.
**	27-mar-2002 (penga03)
**	    Used strstr instead of STindex to substring "Advantage 
**	    Ingres" in argv[1].
**	03-may-2002 (penga03)
**	    Added some change to make msiupd have the ability to update 
**	    a Ingres merge module database using installation identifier 
**	    (II_INSTALLATION).
**	    Note that for those Ingres merge modules that are embedded 
**	    into one product, the installation identifier used to update 
**	    those merge modules should be same.
**	02-july-2001 (penga03)
**	    if MsiDatabaseOpenView or MsiViewExecute fails, do not go 
**	    further.
**	17-jan-2003 (penga03)
**	    Using MsiCloseHandle to close the handles that are opened by 
**	    MsiDatabaseOpenView or MsiViewFetch.
**	02-mar-2004 (penga03)
**	    Replaced all SI and ST functions with corresponding C 
**	    functions.
**	26-jul-2004 (penga03)
**	    Removed all references to "Advantage". 
**	07-dec-2004 (penga03)
**	    Changed the formulation to generate GUIDs since the old one does
**	    not work for id from A1 to A9.
**	15-dec-2004 (penga03)
**	    Backed out the change to generate ProductCode. If the installation
**	    id is between A0-A9, use the new formulation.
*/

#include <windows.h>
#include <winbase.h>
#include <msiquery.h>
#include <compat.h>

BOOL ViewExecute(MSIHANDLE hDatabase, char *szQuery);
BOOL UpdateComponentIds(MSIHANDLE hDatabase, int idx);

i4 
main(i4 argc, char* argv[])
{

    MSIHANDLE hDatabase;
    char view[1024];
    int Branding_Width, DlgLine_Width, DlgLine_X;
    char Branding_Text[64];

    if((argc < 2) || (GetFileAttributes(argv[1])==-1))
    {
	printf ("usage:\nmsiupd <full path to MSI file>\nmsiupd <full path to MSM file> <%II_INSTALLATION%>");
	return 1;
    }
	
    if(strstr(argv[1], "Ingres SDK") != NULL)
    {
	Branding_Width=40;
	DlgLine_X=45;
	DlgLine_Width=329;
	strcpy(Branding_Text, "Ingres SDK");
    }
    else
    {
	Branding_Width=30;
	DlgLine_X=35;
	DlgLine_Width=339;
	strcpy(Branding_Text, "Ingres");
    }

    MsiOpenDatabase(argv[1], MSIDBOPEN_DIRECT, &hDatabase);
    if (!hDatabase)
	return 1;

    if (argc==2 && strstr(argv[1], ".msi"))
    {
	/*
	** Change the brandings on each page from InstallShield to
	** Advantage Ingres.
	*/
	sprintf(view, "UPDATE Control SET Width = '%d' WHERE Control = 'Branding1'", Branding_Width);
	if(!ViewExecute(hDatabase, view))
	    return 1;
	sprintf(view, "UPDATE Control SET Width = '%d' WHERE Control = 'Branding2'", Branding_Width);
	if(!ViewExecute(hDatabase, view))
	    return 1;

	sprintf(view, "UPDATE Control SET X = '%d' WHERE Control = 'DlgLine'", DlgLine_X );
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET X = '0' WHERE Dialog_ = 'AdminWelcome' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET X = '0' WHERE Dialog_ = 'InstallWelcome' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET X = '0' WHERE Dialog_ = 'PatchWelcome' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET X = '0' WHERE Dialog_ = 'SetupCompleteError' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET X = '0' WHERE Dialog_ = 'SetupInterrupted' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET X = '0' WHERE Dialog_ = 'SetupCompleteSuccess' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET X = '0' WHERE Dialog_ = 'SetupInitialization' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET X = '0' WHERE Dialog_ = 'SetupResume' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET X = '0' WHERE Dialog_ = 'MaintenanceWelcome'AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;

	sprintf(view, "UPDATE Control SET Width = '%d' WHERE Control = 'DlgLine'", DlgLine_Width );
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Width = '374' WHERE Dialog_ = 'AdminWelcome' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Width = '374' WHERE Dialog_ = 'InstallWelcome' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Width = '374' WHERE Dialog_ = 'PatchWelcome' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Width = '374' WHERE Dialog_ = 'SetupCompleteError' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Width = '374' WHERE Dialog_ = 'SetupInterrupted' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Width = '374' WHERE Dialog_ = 'SetupCompleteSuccess' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Width = '374' WHERE Dialog_ = 'SetupInitialization' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Width = '374' WHERE Dialog_ = 'SetupResume' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Width = '374' WHERE Dialog_ = 'MaintenanceWelcome' AND Control = 'DlgLine'");
	if(!ViewExecute(hDatabase, view))
	    return 1;

	sprintf(view, "UPDATE Control SET Text = '{&MSSWhiteSerif8}%s' WHERE Control = 'Branding1'", Branding_Text);
	if(!ViewExecute(hDatabase, view))
		return 1;

	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'AdminWelcome' AND Control = 'Branding1'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'InstallWelcome' AND Control = 'Branding1'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'PatchWelcome' AND Control = 'Branding1'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'SetupCompleteError' AND Control = 'Branding1'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'SetupInterrupted' AND Control = 'Branding1'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'SetupCompleteSuccess' AND Control = 'Branding1'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'SetupInitialization' AND Control = 'Branding1'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'SetupResume' AND Control = 'Branding1'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'MaintenanceWelcome'AND Control = 'Branding1'");
	if(!ViewExecute(hDatabase, view))
	    return 1;

	sprintf(view, "UPDATE Control SET Text = '{&Tahoma8}%s' WHERE Control = 'Branding2'", Branding_Text);
	if(!ViewExecute(hDatabase, view))
		return 1;

	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'AdminWelcome' AND Control = 'Branding2'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'InstallWelcome' AND Control = 'Branding2'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'PatchWelcome' AND Control = 'Branding2'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'SetupCompleteError' AND Control = 'Branding2'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'SetupInterrupted' AND Control = 'Branding2'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'SetupCompleteSuccess' AND Control = 'Branding2'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'SetupInitialization' AND Control = 'Branding2'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'SetupResume' AND Control = 'Branding2'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
	strcpy(view,  "UPDATE Control SET Text = '' WHERE Dialog_ = 'MaintenanceWelcome'AND Control = 'Branding2'");
	if(!ViewExecute(hDatabase, view))
	    return 1;
    }
    else if(argc==3 && strstr(argv[1], ".msm"))
    {
	/* Update Merge Module database with II_INSTALLATION. */
	MSIHANDLE hView, hRecord;
	int idx;
	char szID[3];

	strcpy(szID, _strupr(argv[2]));

	if (strlen(szID)!=2 || !isalpha(szID[0]) || !isalnum(szID[1]))
	{
	    printf ("Invalid installation identifier!");
	    return 1;
	}

	if (!strcmp(szID, "II"))
	{
	    if(MsiDatabaseCommit(hDatabase)!=ERROR_SUCCESS)
		return 1;
	    if(MsiCloseHandle(hDatabase)!=ERROR_SUCCESS)
		return 1;

	    return 0;
	}

	/* Update shortcut folders. */
	if (!MsiDatabaseOpenView
	(hDatabase, "SELECT Directory, DefaultDir FROM Directory", &hView))
	{
	    if (!MsiViewExecute(hView, 0))
	    {
		while (MsiViewFetch(hView, &hRecord)!=ERROR_NO_MORE_ITEMS)
		{
		    char szValue[80];
		    DWORD dwSize=sizeof(szValue);
			
		    MsiRecordGetString(hRecord, 2, szValue, &dwSize);
		    if (strstr(szValue, "Ingres II [ "))
			sprintf(szValue, "Ingres II [ %s ]", szID);
		    else if (strstr(szValue, "Ingres [ "))
			sprintf(szValue, "Ingres [ %s ]", szID);
		    MsiRecordSetString(hRecord, 2, szValue);
		    MsiViewModify(hView, MSIMODIFY_UPDATE, hRecord);
		    MsiCloseHandle(hRecord);
		}
	    }
	    MsiCloseHandle(hView);
	}
	
	/* Update IVM Startup shortcut. */
	if (!MsiDatabaseOpenView(hDatabase, "SELECT Name, Component_ FROM Shortcut", &hView))
	{
	    if(!MsiViewExecute(hView, 0))
	    {
		while (MsiViewFetch(hView, &hRecord)!=ERROR_NO_MORE_ITEMS)
		{
		    char szValue[80], szValue2[80];
		    DWORD dwSize, dwSize2;
			
		    dwSize=sizeof(szValue); 
		    dwSize2=sizeof(szValue2);
		    MsiRecordGetString(hRecord, 1, szValue, &dwSize);
		    MsiRecordGetString(hRecord, 2, szValue2, &dwSize2);
		    if (strstr(szValue, "Ingres Visual Manager [ ") 
		        && strstr(szValue2, "Shortcuts.") 
		        && !strstr(szValue2, "SDKShortcuts."))
			sprintf(szValue, "Ingres Visual Manager [ %s ]", szID);
		    MsiRecordSetString(hRecord, 1, szValue);
		    MsiViewModify(hView, MSIMODIFY_UPDATE, hRecord);
		    MsiCloseHandle(hRecord);
		}
	    }
	    MsiCloseHandle(hView);
	}

	/* Update Component GUIDs. */
	idx = (toupper(szID[0]) - 'A') * 26 + toupper(szID[1]) - 'A';
	if (idx <= 0)
	    idx = (toupper(szID[0]) - 'A') * 26 + toupper(szID[1]) - '0';

	UpdateComponentIds(hDatabase, idx);

    }
    else
    {
	printf ("usage:\nmsiupd <full path to MSI file>\nmsiupd <full path to MSM file> <%II_INSTALLATION%>");
	return 1;
    }

    if(MsiDatabaseCommit(hDatabase)!=ERROR_SUCCESS)
	return 1;
    if(MsiCloseHandle(hDatabase)!=ERROR_SUCCESS)
	return 1;
	
    return 0;
}

BOOL 
ViewExecute(MSIHANDLE hDatabase, char *szQuery)
{
	MSIHANDLE hView;
	
	if(MsiDatabaseOpenView(hDatabase, szQuery, &hView)!=ERROR_SUCCESS)
		return FALSE;
	if(MsiViewExecute(hView, 0)!=ERROR_SUCCESS)
		return FALSE;
	if(MsiCloseHandle(hView)!=ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}

BOOL 
UpdateComponentIds(MSIHANDLE hDatabase, int idx)
{
    MSIHANDLE hView, hRecord;

    if (!(MsiDatabaseOpenView(hDatabase, 
          "SELECT Component, ComponentId FROM Component", 
          &hView)==ERROR_SUCCESS))
    {
	return FALSE;
    }

    if (!(MsiViewExecute(hView, 0)==ERROR_SUCCESS))
	return FALSE;
    
    while (MsiViewFetch(hView, &hRecord)!=ERROR_NO_MORE_ITEMS)
    {
	char szValueBuf[39];
	DWORD cchValueBuf=sizeof(szValueBuf);
	char *token, *tokens[5];
	int num=0;

	MsiRecordGetString(hRecord, 2, szValueBuf, &cchValueBuf);
	
	token=strtok(szValueBuf, "{-}");
	while (token != NULL )
	{
	    tokens[num]=token;
	    token=strtok(NULL, "{-}");
	    num++;
	}
	sprintf(szValueBuf, "{%s-%s-%s-%04X-%012X}", tokens[0], tokens[1], tokens[2], idx, idx*idx);

	MsiRecordSetString(hRecord, 2, szValueBuf);
	
	if (!(MsiViewModify(hView, MSIMODIFY_UPDATE, hRecord)==ERROR_SUCCESS))
	    return FALSE;

	MsiCloseHandle(hRecord);
    }
    
    if (!(MsiCloseHandle(hView)==ERROR_SUCCESS))
	return FALSE;

    return TRUE;
}
