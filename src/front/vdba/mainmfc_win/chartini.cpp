/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : chartini.cpp, Implementation File
**
**  Project  : Ingres II/Vdba.
**
**  Author   : UK Sotheavut
**
**  Purpose  : Initialize the pie chart data
**
**  History:
**	17-feb-2000 (somsa01)
**	    For HP, we use the 'bdf' command. Also, added logic to account
**	    for those platforms where the 'df' or 'bdf' command's output
**	    does not wrap.
** 23-May-2000 (uk$so01)
**    bug 99242 (DBCS) modify the functions:
**    UNIX_GetSpaceForLocation(), UNIX_MatchPartition(), UNIX_GetFreeSpace()
**    UNIX_SpaceUsedByLocation(),
**	09-May-2001 (hanje04)
**	    In Chart_Create, added LPTSTR cast to 2nd argument of call to 
**	    UNIX_SpaceUsedByLocation() to resolve compile errors about 
**	    incorrect type on UNIX.
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "chartini.h"
#include "statfrm.h"
extern "C" 
{
#include "monitor.h"
#include "domdata.h"
#include "error.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


COLORREF tab_colour [] =         
{
	RGB (192,  0,  0),  // Red
	RGB (  0,192,  0),  // Green
	RGB (  0,  0,192),  // Bleu
	RGB (255,255,  0),  // Yellow
	RGB (255,  0,255),  // Pink
	RGB (  0,255,255),  // Blue light
//	RGB (255,255,255),  // Blanc
//	RGB (128,  0,  0),  // Brown
	RGB (  0,128,  0),  // Green dark
	RGB (128,  0,128),  // violet
	RGB (  0,128,128),  // Blue
	RGB (128,128,  0),  // Brun (Marron)
	RGB (128,128,128),  // Gris Sombre
	RGB (255,  0,  0),  // Red
	RGB (  0,255,  0),  // Green
	RGB (192,  0,255),  // Violet
	RGB (  0,192,192),  // Blue
	RGB (  0,  0,192),  // Blue
	RGB (  0,  0,128),  // Blue
	RGB (192,192,  0),  // orange
	RGB (255,  0,128),  // Pink dark
	RGB (192,220,192),  // Green 
	RGB ( 64,  0,128),  // Violet 
	RGB (128, 64,  0),  // Brown 
	RGB (128,128, 64),  // Green 
	RGB (  0,  0,  0)   // Black
};

#define MAX_DRIVE_LEN      4
//
// 16-07-1997 UKS ports to UNIX
//
#if defined MAINWIN
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define DATA_PATH _T("%s/ingres/data/default")
#define WORK_PATH _T("%s/ingres/work/default")
#define JNL_PATH  _T("%s/ingres/jnl/default")
#define CHK_PATH  _T("%s/ingres/ckp/default")
#define DMP_PATH  _T("%s/ingres/dmp/default")

#else

#define DATA_PATH _T("%s\\ingres\\data\\default\\*.*")
#define WORK_PATH _T("%s\\ingres\\work\\default\\*.*")
#define JNL_PATH  _T("%s\\ingres\\jnl\\default\\*.*")
#define CHK_PATH  _T("%s\\ingres\\ckp\\default\\*.*")
#define DMP_PATH  _T("%s\\ingres\\dmp\\default\\*.*")

#endif // MAINWIN


class CuDirectoryData: public CObject
{
public:
	CuDirectoryData(){m_strName = _T(""); m_dSize = 0.0;};
	CuDirectoryData(LPCTSTR lpszName, double dSize){m_strName = lpszName; m_dSize = dSize;}

	virtual ~CuDirectoryData(){}

	CString m_strName;
	double	m_dSize;
};

static BOOL GetSpaceForLocation( 
	const char* CurrSearchDir, 
	double& dTotalDir,
	BOOL bWithSubDirectory, 
	BOOL bOneLevelOnly,
	CTypedPtrList<CObList, CuDirectoryData*>*  pListDir = NULL);


//
// Verify is the given string 'strDir' is
// an existing database name.
static BOOL IsDatabase (const CString& strDir, int nHnode)
{
	UCHAR bufObj    [MAXOBJECTNAME];
	UCHAR buffilter [MAXOBJECTNAME];
	UCHAR extradata [MAXOBJECTNAME];
	int ires;

	ires = DOMGetFirstObject (nHnode, OT_DATABASE, 0, NULL, TRUE, NULL, bufObj, buffilter, extradata);
	while (ires == RES_SUCCESS) 
	{
		if (strDir.CompareNoCase ((LPCTSTR)bufObj) == 0)
			return TRUE;
		ires  = DOMGetNextObject (bufObj, buffilter, extradata);
	}
	return FALSE;
}


//
// strLoc: Astring of the form loc1 + loc2 + ... + locn
// Find if lpszLoc is one of the loci for i in [1; n]
//
static BOOL MatchShortLocationName (CString& strLoc, LPCTSTR lpszLoc, int hNode)
{
	// 
	// Call FNN function that supposed to checke if lpszLoc is part of the string
	// strLoc.
	int res = LocationInLocGroup (hNode, (LPTSTR)lpszLoc, (LPTSTR)(LPCTSTR)strLoc);
	return (res == RES_SUCCESS)? TRUE: FALSE;
}


// GetSpaceForLocation 
// Parameters :
//       CurrentSearchDirectory  : A valid directory.
//       dTotalDir               : Sum of files size.
//
//       bWithSubDirectory       : TRUE  get the file size in all sub directory or one level if
//                                  bOneLevelOnly is TRUE .
//                                 FALSE Get the file size only in CurrentSearchDirectory.
//
//       bOneLevelOnly     : TRUE get the file size only in one level of sub directory 
//                           FALSE get the file size in all sub directory
//
//static BOOL GetSpaceForLocation( const char *CurrSearchDir , double& dTotalDir ,
//                          BOOL bWithSubDirectory    , BOOL bOneLevelOnly)
static BOOL GetSpaceForLocation( 
    const char *CurrSearchDir , 
    double& dTotalDir ,
    BOOL bWithSubDirectory, 
    BOOL bOneLevelOnly,
    CTypedPtrList<CObList, CuDirectoryData*>*  pListDir)
{
   WIN32_FIND_DATA FileData;
   HANDLE hSearch;
   double dTotalSubDir;
   BOOL fFinished = FALSE;
   BOOL bResult;

   if (x_strlen(CurrSearchDir) > _MAX_PATH)
      return FALSE;

   hSearch = FindFirstFile(CurrSearchDir,&FileData);
   if(hSearch == INVALID_HANDLE_VALUE)  {
      //MessageBox(GetFocus(),"No File Found",
      //           "Error in FindFirstFile",MB_OK|MB_ICONINFORMATION );
      return FALSE;
   }

   dTotalDir = 0;

   while (!fFinished) {
      dTotalSubDir = 0;
      if ( (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            bWithSubDirectory == TRUE &&
            x_strcmp(FileData.cFileName,_T("."))!=0 &&
            x_strcmp(FileData.cFileName,_T(".."))!=0) {
         char CurrSubDir [_MAX_PATH];
         char * pdest = NULL;

         dTotalSubDir = 0;

         pdest = x_strrchr(CurrSearchDir,'\\');
         if ( pdest == NULL || 
              (x_strlen(FileData.cFileName) + x_strlen(CurrSearchDir) + 1) > _MAX_PATH) {
             // Close the search handle. 
            if (!FindClose(hSearch))   {
               //MessageBox(GetFocus(),"Couldn't close search handle.",
               //"Error on Handle",MB_OK|MB_ICONINFORMATION );
            }
            return FALSE;
         }
         fstrncpy ((unsigned char *)CurrSubDir,(unsigned char *)CurrSearchDir,(pdest-CurrSearchDir)+2);
         x_strcat(CurrSubDir,FileData.cFileName);
         if (pListDir)
         {
            WIN32_FIND_DATA aFileData;
            memset (&aFileData, 0, sizeof (aFileData));
            //
            // ADD FileData.cFileName as Database
            CuDirectoryData* pDir = new CuDirectoryData();
            pDir->m_strName = FileData.cFileName;
            x_strcat(CurrSubDir,_T("\\*.*"));
            HANDLE haSearch = FindFirstFile(CurrSubDir, &aFileData);
            if(haSearch == INVALID_HANDLE_VALUE) 
               return FALSE;

            BOOL bEnd = FALSE;
            double dSpaceDir = 0.0;
            while (!bEnd)
            {
                //
                // Total file size. 
                dSpaceDir += (aFileData.nFileSizeHigh*MAXDWORD) + aFileData.nFileSizeLow;
                if (!FindNextFile(haSearch, &aFileData)) 
                {
                    if (GetLastError() == ERROR_NO_MORE_FILES)   
                        bEnd = TRUE; 
                    else
                        return FALSE;
                } 
            }
            //
            // Close the search handle. 
            if (!FindClose(haSearch)) 
                return FALSE;
            //
            // ADD dTotalDir as Size consumed by the FileData.cFileName
            pDir->m_dSize = dSpaceDir;
            pListDir->AddTail (pDir);
            /*
            if (pDir->m_dSize > 0.0)
                pListDir->AddTail (pDir);
            else
                delete pDir;
            */
            dTotalDir += dSpaceDir;
         }
         else
         {
            x_strcat(CurrSubDir,_T("\\*.*"));
            if (bOneLevelOnly)
               bResult = GetSpaceForLocation( CurrSubDir , dTotalSubDir , FALSE, TRUE );
            else
               bResult = GetSpaceForLocation( CurrSubDir , dTotalSubDir , TRUE , FALSE);
            
            if (!bResult)  {
                // Close the search handle. 
               if (!FindClose(hSearch))   {
                  //MessageBox(GetFocus(),"Couldn't close search handle.",
                  //"Error on Handle",MB_OK|MB_ICONINFORMATION );
               }
               return FALSE;
            }
         }
      }
      // total file size. 
      //if (!pListDir)
         dTotalDir = dTotalSubDir + dTotalDir + (FileData.nFileSizeHigh*MAXDWORD) + FileData.nFileSizeLow;
      if (!FindNextFile(hSearch, &FileData)) 
         if (GetLastError() == ERROR_NO_MORE_FILES)  { 
             fFinished = TRUE; 
         } 
         else   {
            // Close the search handle. 
            if (!FindClose(hSearch))   {
               //MessageBox(GetFocus(),"Couldn't close search handle.",
               //"Error on Handle",MB_OK|MB_ICONINFORMATION );
            }
            //GetLastError();
            //MessageBox(GetFocus(),"Couldn't find next file.",
            //           "Error in FindNextFile",MB_OK|MB_ICONINFORMATION );
            return FALSE;
         } 
   
   }
   
   // Close the search handle. 
   if (!FindClose(hSearch)) 
   {
      //MessageBox(GetFocus(),"Couldn't close search handle.",
      //           "Error on Handle",MB_OK|MB_ICONINFORMATION );
      return FALSE;
   }
   return TRUE;
}



static void GetIndice ( char *Letter, int &CurrIndice )
{
	CurrIndice = toupper ( *Letter ) - 65;
	//return CurrIndice;
}




#if defined (MAINWIN)
static BOOL UNIX_GetSpaceForLocation (LPCTSTR CurrSearchDir, double& dTotalDir, CTypedPtrList<CObList, CuDirectoryData*>* pListDir)
{
	int   len, nCount, p[2];
	TCHAR  buff[620];
	TCHAR  chr [2];

	if (!CurrSearchDir)
		return FALSE;
	len = x_strlen (CurrSearchDir);
	dTotalDir = 0.0;
	memset (buff, _T('\0'), sizeof (buff));
	pipe (p);
	buff [0] = _T('\0');
	if (fork() == 0)
	{
		close (p[0]);
		dup2  (p[1], STDOUT_FILENO);
		close (p[1]);
		execlp ("du", "du", "-k", CurrSearchDir, (char*)0);
		exit(2);
	}
	close (p[1]);
	wait (NULL);
	nCount = 0;
	while (read (p[0], chr, 1) > 0)
	{
		chr[1] = _T('\0');
		if (chr[0] == _T('\n'))
		{
			TCHAR directory [520];
			long total;

			buff [nCount] = _T('\0');
			_tcslwr (buff);
			sscanf (buff, "%ld %s", &total, directory);
			if (pListDir && directory[0] && (int)x_strlen (directory) > len)
			{
				//
				// The directory contains the last string as Database Name
				TCHAR* subdir;
				subdir = &directory [len+1];
				CuDirectoryData* pDir = new CuDirectoryData();
				pDir->m_strName = subdir;
				pDir->m_dSize = total;
				pListDir->AddTail (pDir);
			}
			if (directory[0] && (int)x_strlen (directory) == len)
				dTotalDir = total;

			nCount = 0;
			memset (buff, _T('\0'), sizeof (buff));
		}
		else
		{
			buff [nCount] = chr[0];
			nCount++;
		}
	}
	close (p[0]);
	return TRUE;
}
#endif // MAINWIN


//
// 15-07-1997: UKS Ports to UNIX
#if defined (MAINWIN)
//
// directory: given a directory, ex: /disk1/oi20a/ingres/data/default
// partition: given a partition, ex: /disk1
// The function gives the first match starting from the end of the string. So it matchs /disk1 and not a '/'
//
int static UNIX_MatchPartition (LPCTSTR directory, LPCTSTR partition)
{
	TCHAR buff [640];
	int  i, len;
	if (!(directory || partition))
		return 0;
	memset (buff, _T('\0'), sizeof (buff));
	_tcscpy (buff, directory);
	len = _tcslen (buff);
	if (_tcscmp (buff, partition) == 0)
		return 1;
	//
	// Obtain a TCHAR pointer to the last "logical character" of a string 
	TCHAR* pLast = _tcsdec(buff, buff +  _tcslen(buff));
	while (pLast && *pLast != _T('\0') && pLast != buff)
	{
		if (*pLast == _T('/'))
		{
			*pLast = _T('\0');
			if (_tcscmp (pLast, partition) == 0)
				return 1;
		}
		pLast = _tcsdec(buff, pLast);
	}
	//
	// check for single '/':
	i = 0;
	i += _tclen(buff + 1);
	buff [i] = _T('\0');
	if (_tcscmp (buff, partition) == 0)
		return 1;
	return 0;
}
#endif


#if defined (MAINWIN)
static long UNIX_GetFreeSpace (TCHAR* MountedDirectoty)
{
	int   nCount, p[2], nLine = 0;
	TCHAR  buff[1024];
	TCHAR  chr [2];
	bool  ContFlag;

	if (!MountedDirectoty)
		return 0L;

	pipe (p);
	buff [0] = _T('\0');
	if (fork() == 0)
	{
		close (p[0]);
		dup2  (p[1], STDOUT_FILENO);
		close (p[1]);

#if defined(hp8_us5) || defined(hpb_us5)
		execlp ("bdf", "bdf", MountedDirectoty, (char*)0);
#else
		execlp ("df", "df", "-k", MountedDirectoty, (char*)0);
#endif /* hp8_us5 hpb_us5 */

		exit(2);
	}

	close (p[1]);
	wait (NULL);
	nCount = 0;
	ContFlag = FALSE;
	while (read (p[0], chr, 1) > 0)
	{
		TCHAR filesys [128];
		chr[1] = _T('\0');
		if (chr[0] == _T('\n'))
		{
			char percent [128];
			char mounton [128];
			long total=-1, used, avail;

			buff [nCount] = _T('\0');
			_tcslwr (buff);
			if (nLine > 0)
			{
				if (ContFlag)
				{
					/*
					** The line has wrapped.
					*/
					sscanf (buff, "%ld %ld %ld %s %s", &total, &used,
						&avail, percent, mounton);
					ContFlag = FALSE;
				}
				else
				{
					sscanf (buff, "%s %ld %ld %ld %s %s", filesys,
						&total, &used, &avail, percent, mounton);
				}

				if (total == -1)
				{
					/*
					** We need to wrap this line, as the next line
					** contains the stuff we want.
					*/
					ContFlag = TRUE;
					nCount = 0;
					buff [0] = _T('\0');
				}
				else
				{
					close (p[0]);
					return avail;
				}
			}

			nLine++;
			nCount = 0;
			buff [0] = _T('\0');
		}
		else
		{
			buff [nCount] = chr[0];
			nCount++;
		}
	}

	close (p[0]);
	return 0L;
}
#endif



#if defined (MAINWIN)
//
// Given a base directory: ex: /disk1/oi20/ingres/data/default,
// then every sub-derectory is supposed to be a database.
// To make sure, we should test if it is really a database !!!.
static int UNIX_SpaceUsedByLocation (int hNodeHandle, TCHAR* baseDirectory, long& spaceUsed)
{
	int res = RES_ERR;
	LOCATIONDATAMIN  LocationData;
	int   len, nCount, p[2], nLine = 0;
	TCHAR  buff[520];
	TCHAR  chr [2];
	spaceUsed = 0L;
	if (!baseDirectory)
		return 0;
	len = x_strlen (baseDirectory);
	memset (&LocationData, 0, sizeof(LocationData));
	memset (buff, '\0', sizeof(buff));

	pipe (p);
	buff [0] = '\0';
	if (fork() == 0)
	{
		close (p[0]);
		dup2  (p[1], STDOUT_FILENO);
		close (p[1]);
		execlp ("du", "du", "-k", baseDirectory, (char*)0);
		exit(2);
	}
	close (p[1]);
	wait (NULL);
	nCount = 0;

	while (read (p[0], chr, 1) > 0)
	{
		chr[1] = _T('\0');
		if (chr[0] == _T('\n'))
		{
			TCHAR  directory [256];
			TCHAR* subdir;
			long  total;

			buff [nCount] = _T('\0');
			_tcslwr (buff);
			sscanf (buff, "%ld %s", &total, directory);
			//subdir = &directory [len+1];
			TCHAR* ptchSubDir = &directory[len];
			subdir = _tcsinc (ptchSubDir);
			CString strDB = subdir;
			if (subdir[0] > 0 && IsDatabase (strDB, hNodeHandle))
			{
				spaceUsed += total;
			}
			nCount = 0;
			memset (buff, _T('\0'), sizeof(buff));
		}
		else
		{
			buff [nCount] = chr[0];
			nCount++;
		}
	}
	close (p[0]);
	return 1;
}
#endif



// *****************
// Create the object:
// This object is equivalent to the entire Disk 
// (Capacity of Disk is the same as the Total capacity of the Pie Chart)
// It may not contains more than 4 sectors:
//      1) Location pData->LocationName
//      2) Other locations if exists
//      3) Other files
//      4) Free space.
//
// Member of Structure  LOCATIONDATAMIN:
//    - LocationName,       the name of location.
//    - LocationArea,       th area of location. Ex C:\Base1\Dx
//    - LocationUsages [5], LocationUsages [i] = ATTACHED_YES ==> LocationUsages [i] is valid. 
//          0 =  DATA_PATH (database)
//          1 =  WORK_PATH (work)
//          2 =  JNL_PATH  (journaling)
//          3 =  CHK_PATH  (checkpoint)
//          4 =  DMP_PATH  (dump)
//
#define IPM_MAXUSAGE 5
BOOL Pie_Create(CaPieInfoData* pPieInfo, int nHnode, void* pLocationDatamin, BOOL bDetail)
{
	LPLOCATIONDATAMIN pData = (LPLOCATIONDATAMIN)pLocationDatamin;
	double dCapacity = 0.0;
	double dPercent  = 0.0;
	double dTotal    = 0.0;
	LPTSTR lpszPath;
	#if defined (MAINWIN)
	TCHAR tchszFixPath [IPM_MAXUSAGE][32] =
	{
		_T("/ingres/data/default"),
		_T("/ingres/work/default"),
		_T("/ingres/jnl/default"),
		_T("/ingres/ckp/default"),
		_T("/ingres/dmp/default")
	};
	#else
	TCHAR tchszFixPath [IPM_MAXUSAGE][32] =
	{
		_T("\\ingres\\data\\default\\*.*"),
		_T("\\ingres\\work\\default\\*.*"),
		_T("\\ingres\\jnl\\default\\*.*" ),
		_T("\\ingres\\ckp\\default\\*.*" ),
		_T("\\ingres\\dmp\\default\\*.*" )
	};
	#endif

	COLORREF         colorOtherLocations = NULL;
	CaBarData*      pDisk = NULL;
	CaOccupantData*  pOcc  = NULL;
	CString          strOther;
	CString          strFree;
	CString          strLocationArea;
	CuDirectoryData* pDir  = NULL;
	CuDirectoryData* pDirC = NULL;

	if (strOther.LoadString (IDS_DISKSPACE_OTHER) == 0)
		strOther = _T("Other");
	if (strFree.LoadString (IDS_DISKSPACE_FREE) == 0)
		strFree = _T("Free");

	pPieInfo->Cleanup();
	if (GetPathLoc ((char*)pData->LocationArea, (char**)&lpszPath))
		strLocationArea = lpszPath;
	else
		strLocationArea = (LPCTSTR)pData->LocationArea;
	CString  strDiskUnit= strLocationArea.Left (2);

	dCapacity = 0.0; // The Disk space used by pData->LocationName 
	CTypedPtrList<CObList, CuDirectoryData*>  listDir;

	TCHAR tchSepPath;
	#if defined (MAINWIN)
		tchSepPath = _T('/');
	#else
		tchSepPath = _T('\\');
	#endif

	for (int i=0; i<IPM_MAXUSAGE; i++)
	{
		CString strPath = strLocationArea;
		CString strTemp = strLocationArea;
		TCHAR* pLocArea = (TCHAR*)(LPCTSTR)strTemp;
		//
		// When modifing the buffer directly of strTemp, strTemp is no longer valid as CString
		// DO NOT REFER to strTemp any more !
		TCHAR* pch = _tcsdec(pLocArea, pLocArea + _tcslen(pLocArea));
		if (pch && *pch == tchSepPath)
		{
			*pch = _T('\0');
			strPath = pLocArea;
		}

		if (pData->LocationUsages [i] == ATTACHED_YES)
		{
			double dSpace;
			if (GetPathLoc ((char*)pData->LocationArea, (char**)&lpszPath))
			{
				strPath  = lpszPath;
				strPath += tchszFixPath [i]; 
			}
			else
				strPath += tchszFixPath [i]; 
			#if defined (MAINWIN) // MAINWIN
			if (UNIX_GetSpaceForLocation (strPath, dSpace, &listDir))
			{
				dCapacity += dSpace; // Under unix: du -k gives a 1024 unit
			}
			#else                 // NOT MAINWIN
			if (GetSpaceForLocation ((const char*)strPath, dSpace, TRUE, TRUE, &listDir))
			{
				dCapacity += dSpace;
			}
			#endif
		}
	}

	pPieInfo->SetSmallUnit(_T("Kb"));
	#if defined (MAINWIN)
	pPieInfo->SetCapacity (dCapacity / 1024.0, _T("Mb"));
	#else
	pPieInfo->SetCapacity (dCapacity / (1024.0*1024.0), _T("Mb"));
	#endif
	if (!listDir.IsEmpty())
	{
		POSITION posSave, posDel, px  = NULL;
		POSITION pos = listDir.GetHeadPosition();
		while (pos != NULL)
		{
			posSave = pos;
			pDir = listDir.GetNext(pos);
			px   = pos;
			while (px != NULL)
			{
				posDel = px;
				pDirC = listDir.GetNext(px);
				if (pDirC->m_strName == pDir->m_strName)
				{
					pDir->m_dSize += pDirC->m_dSize;
					listDir.RemoveAt (posDel);
					delete pDirC;
					pos = posSave;
					break;
				}
			}
		}
	}
	//
	// listDir: List of Directories assumed to be database name.
	//          We must verify if they are real databases, if so we display them.
	CaLegendData* pLegendData = NULL;
	int    iIndex = 0;
	double dDatabaseSpace = 0.0;
	BOOL   bContinueMessage = TRUE; // In case of existence of multiple directories
	                                // that are not databases, we keep displaying message.
	while (!listDir.IsEmpty())
	{
		pDir = listDir.RemoveHead();
		//
		// Verify if a directory is a real database name
		if (IsDatabase (pDir->m_strName, nHnode))
		{
			CTypedPtrList<CObList, CaLegendData*>& lg = pPieInfo->GetListLegend();
			iIndex = lg.GetCount();
			if (dCapacity > 0.0 && pDir->m_dSize > 0.0)
			{
				if (iIndex+2 < sizeof (tab_colour)/sizeof(COLORREF))
					pPieInfo->AddPie (pDir->m_strName, pDir->m_dSize*100.0/dCapacity, tab_colour[iIndex+2]);
				else
					pPieInfo->AddPie (pDir->m_strName, pDir->m_dSize*100.0/dCapacity);
				dDatabaseSpace+= pDir->m_dSize;
			}
			else
			{
				if (iIndex+2 < sizeof (tab_colour)/sizeof(COLORREF))
					pPieInfo->AddLegend (pDir->m_strName, tab_colour[iIndex+2]);
				else
					pPieInfo->AddLegend (pDir->m_strName);
			}
		}
		else
		if (bContinueMessage)
		{
			CString strMsg;
			AfxFormatString2 (strMsg, IDS_DISKSPACE_INVALID_DATABASE, (LPCTSTR)pData->LocationName, pDir->m_strName);
			if (BfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO) == IDNO)
				bContinueMessage = FALSE;
		}
		delete pDir;
	}
	if (dCapacity - dDatabaseSpace > 0.0001)
	{
		pPieInfo->AddPie (strOther, (dCapacity-dDatabaseSpace)*100.0/dCapacity, tab_colour[0]);
	}
	LOCATIONDATAMIN locmin;
	CString strLocWithDouble = (LPCTSTR)pData->LocationName;
	memset (&locmin, 0, sizeof (locmin));
	int ires = GetFirstMonInfo (nHnode, 0, NULL, OT_LOCATION_NODOUBLE, &locmin, NULL);
	while (ires == RES_SUCCESS) 
	{
		CString strFullName = (LPCTSTR)locmin.LocationName;
		if (strFullName.Find ((LPCTSTR)pData->LocationName) != -1)
		{
			strLocWithDouble = strFullName;
			break;
		}
		ires = GetNextMonInfo (&locmin);
	}
	//
	// "Databases on Location %s", pData->LocationName
	CString strTitle;
	AfxFormatString1 (strTitle, IDS_TAB_LOCATION_PAGE1_TITLE, (LPCTSTR)strLocWithDouble);
	pPieInfo->SetTitle (strTitle, (LPCTSTR)strLocWithDouble);
	pPieInfo->m_bError = FALSE;
	return bDetail;
}

// *****************
// Create the object:
// This object is equivalent to the entire Disk 
// (Capacity of Disk is the same as the Total capacity of the Pie Chart)
// It must not contains more than 4 sectors:
//      1) Location pData->LocationName
//      2) Other locations if exists
//      3) Other files
//      4) Free space.
//
// Member of Structure  LOCATIONDATAMIN:
//    - LocationName,       the name of location.
//    - LocationArea,       th area of location. Ex C:\Base1\Dx
//    - LocationUsages [5], LocationUsages [i] = ATTACHED_YES ==> LocationUsages [i] is valid. 
//          0 =  DATA_PATH (database)
//          1 =  WORK_PATH (work)
//          2 =  JNL_PATH  (journaling)
//          3 =  CHK_PATH  (checkpoint)
//          4 =  DMP_PATH  (dump)
//
BOOL Pie_Create(CaPieInfoData* pPieInfo, int nHnode, void* pLocationDatamin)
{
	LPLOCATIONDATAMIN pData = (LPLOCATIONDATAMIN)pLocationDatamin;
	double   dCapacity = 0.0;
	double   dPercent  = 0.0;
	double   dTotal    = 0.0;
	LPTSTR   lpszPath  = NULL;
	POSITION pos       = NULL;
	COLORREF colorOther     = tab_colour [0];
	COLORREF colorFree      = tab_colour [1];
	COLORREF colorLocation  = tab_colour [2];   // Current Location
	COLORREF colorLocations = tab_colour [5];   // Other locations
	
	CaBarData*     pDisk = NULL;
	CaOccupantData* pOcc  = NULL;
	CaBarPieItem*   pLoc  = NULL;
	CString strOther;
	CString strFree;
	if (strOther.LoadString (IDS_DISKSPACE_OTHER) == 0)
		strOther = _T("Other");
	if (strFree.LoadString (IDS_DISKSPACE_FREE) == 0)
		strFree = _T("Free");
	
	pPieInfo->Cleanup();
	CString  strLocationArea;
	if (GetPathLoc ((char*)pData->LocationArea, (char**)&lpszPath))
		strLocationArea = lpszPath;
	else
		strLocationArea = (LPCTSTR)pData->LocationArea;
	CString  strDiskUnit= strLocationArea.Left (2);
	
	CaBarInfoData diskinfo;
	if (!Chart_Create(&diskinfo, nHnode))
	{
		pPieInfo->SetError(FALSE);
		return FALSE;
	}
	//
	// Calculate the partition under UNIX operating system.
	#if defined (MAINWIN)
	strDiskUnit = _T("");
	pos = diskinfo.m_listUnit.GetTailPosition();
	while (pos != NULL)
	{
		pDisk = diskinfo.m_listUnit.GetPrev (pos);
		if (UNIX_MatchPartition ((LPCTSTR)strLocationArea, (LPCTSTR)pDisk->m_strDiskUnit))
		{
			strDiskUnit = pDisk->m_strDiskUnit;
			break; 
		}
	}
	#endif

	CaBarPieItem* pFind = NULL;
	CTypedPtrList<CObList, CaBarPieItem*>& listbar = diskinfo.GetListBar();
	pos = listbar.GetHeadPosition();

	while (pos != NULL)
	{
		pLoc = listbar.GetNext (pos);
		if (MatchShortLocationName (pLoc->m_strName, (LPCTSTR)pData->LocationName, nHnode))
			pFind = pLoc;
	}
	if (pFind)
		pPieInfo->AddLegend (pFind->m_strName, colorLocation);
	else
	{
		pPieInfo->SetError(TRUE);
		return FALSE; // TODO: Multiple locations cannot be managed !!!! 
	}

	//
	// Set the total capacity of the Pie Chart
	pos = diskinfo.m_listUnit.GetHeadPosition();
	while (pos != NULL)
	{
		pDisk = (CaBarData*)diskinfo.m_listUnit.GetNext (pos);
		if (pDisk->m_strBarUnit.CompareNoCase (strDiskUnit) == 0)
		{
			break;
		}
	}
	//
	// The disk must exist.
	ASSERT (pDisk);
	pPieInfo->SetCapacity (pDisk->m_nCapacity, _T("Mb"));
	pPieInfo->SetSmallUnit(_T("Kb"));

	//
	// Disk C: = 200Mb
	CString strTitle;
#if defined (MAINWIN)
	strTitle.Format (_T("Partition %s = %0.2f%s"), (LPCTSTR)pDisk->m_strBarUnit, pDisk->m_nCapacity, (LPCTSTR)pPieInfo->GetUnit());
#else
	CString strCapacity;
	strCapacity.Format (_T("%0.2f%s"), pDisk->m_nCapacity, (LPCTSTR)pPieInfo->GetUnit());
	AfxFormatString2 (strTitle, IDS_TAB_LOCATION_PAGE2_TITLE, pDisk->m_strBarUnit, strCapacity);
#endif
	pPieInfo->SetTitle (strTitle, (LPCTSTR)pData->LocationName);

	CaOccupantData* pOther = NULL;
	CaBarPieItem* pFree  = NULL;
	dCapacity = 0.0;
	//
	// Add current location (first sector)

	pos = pDisk->m_listOccupant.GetHeadPosition();
	while (pos != NULL)
	{
		pOcc = (CaOccupantData*)pDisk->m_listOccupant.GetNext (pos);


		//if (pOcc->m_strName.Find ((LPCTSTR)pData->LocationName) != -1)
		if (MatchShortLocationName (pOcc->m_strName, (LPCTSTR)pData->LocationName, nHnode))
		{
			//
			// Add the pie called 'pData->LocationName' (sector)
			pPieInfo->AddPie ((LPCTSTR)pOcc->m_strName, pOcc->m_fPercentage, colorLocation);
			dTotal += pOcc->m_fPercentage;
		}
		else
		if (pOcc->m_strName.CompareNoCase (strOther) == 0)
		{
			dCapacity = pOcc->m_fPercentage;
			pOther    = pOcc;
		}
		else
		{
			//
			// Calculate the total percentage of other locations.
			dPercent += pOcc->m_fPercentage;
		}
	}
	//
	// Add other locations. (second sector)
	CString strOtherLocation;
	if (strOtherLocation.LoadString (IDS_DISKSPACE_OTHERLOCATIONS) == 0)
		strOtherLocation = _T("Other Locations");
	if (dPercent > 0.0 && colorLocations != NULL)
	{
		pPieInfo->AddPie ((LPCTSTR)strOtherLocation, dPercent, colorLocations);
		dTotal += dPercent;
	}
	else
	{
		//
		// Due to FNN, this pie chart must have 4 legends !!
		// 1. The current location.
		// 2. The other locations (even if there is no other locations)
		// 3. Ther other (other files than that the location)
		// 4. The Free space (even there is no free space)
		pPieInfo->AddLegend ((LPCTSTR)strOtherLocation, colorLocations);
	}
	//
	// Add others. (third sector)
	if (dCapacity > 0.0 && pOther)
	{
		pPieInfo->AddPie ((LPCTSTR)strOther, dCapacity, colorOther);
		dTotal += dCapacity;
	}
	else
	{
		pPieInfo->AddLegend (strOther, colorOther);
	}
	//
	// Add the Free (fourth sector)
	if (100.00 - dTotal > 0.0)
	{
		pos = diskinfo.m_listBar.GetTailPosition();
		CaBarPieItem* pLoc = NULL;
		while (pos != NULL)
		{
			pLoc = (CaBarPieItem*)diskinfo.m_listBar.GetPrev(pos);
			if (pLoc->m_strName.CompareNoCase (strFree) == 0)
			{
				pFree = pLoc;
				break;
			}
		}
		if (pFree)
			pPieInfo->AddPie ((LPCTSTR)strFree, 100.00 - dTotal, colorFree);
	}
	else
	{
		pPieInfo->AddLegend (strFree, colorFree);
	}
	pPieInfo->SetError(FALSE);
	return TRUE;
}



#if defined (MAINWIN)
//
// Return TRUE  : no error.
// Return FALSE : List of drives not available or no information size for 
// a drive.
//  
BOOL Chart_Create(CaBarInfoData* pDiskInfo, int nNodeHandle)
{
	int   res = RES_ERR;
	int   nColorIndex = 2;
	long  nSaveSlash = 0L;
	TCHAR  szCurrDir [1024];
	TCHAR  partSlash [2];
	TCHAR* CurrentPath;
	TCHAR* upath[5]= {DATA_PATH, WORK_PATH, JNL_PATH, CHK_PATH, DMP_PATH};
	LOCATIONDATAMIN LocationData;
	CaBarPieItem* pLocData = NULL;
	CStringArray arrayPath;
	bool  ContFlag;
	arrayPath.SetSize (32);
	ASSERT (pDiskInfo);
	if (!pDiskInfo)
		return FALSE;
	//
	// Clean up old data
	memset (&LocationData, 0, sizeof(LocationData));
	pDiskInfo->Cleanup();
	pDiskInfo->m_strSmallUnit = _T("Kb");
	// Get All locations
	res  = GetFirstMonInfo (nNodeHandle, 0, NULL, OT_LOCATION_NODOUBLE, &LocationData, NULL);
	while (res == RES_SUCCESS) 
	{
		if (nColorIndex < (int)(sizeof (tab_colour)/sizeof(COLORREF)))	
		{
			//
			// Add the locations.
			pDiskInfo->AddBar ((LPCTSTR)LocationData.LocationName, tab_colour[nColorIndex]);
			nColorIndex++;
		}
		else
		{
			//
			// Add the locations with Random Colour.
			pDiskInfo->AddBar ((LPCTSTR)LocationData.LocationName);
		}
		res = GetNextMonInfo (&LocationData);
	}
	CString strOther;
	CString strFree;
	if (strOther.LoadString (IDS_DISKSPACE_OTHER) == 0)
		strOther = _T("Other");
	if (strFree.LoadString (IDS_DISKSPACE_FREE) == 0)
		strFree = _T("Free");
	pDiskInfo->AddBar ((LPCTSTR)strOther, tab_colour[0]);
	pDiskInfo->AddBar ((LPCTSTR)strFree , tab_colour[1]);

	//
	// Add the disk units.
	// AddBarUnit (partition). Ex: /usr: 400.00	 // 400 KB
	int   nCount, p[2], nLine = 0;
	char  buff[1024];
	char  chr [2];

	pipe (p);
	buff [0] = '\0';
	if (fork() == 0)
	{
		close (p[0]);
		dup2  (p[1], STDOUT_FILENO);
		close (p[1]);

#if defined(hp8_us5) || defined(hpb_us5)
		execlp ("bdf", "bdf", (char*)0);
#else
		execlp ("df", "df", "-k", (char*)0);
#endif	/* hp8_us5 hpb_us5 */

		exit(2);
	}
	close (p[1]);
	wait (NULL);
	nCount = 0;
	ContFlag = FALSE;
	memset (partSlash, _T('\0'), sizeof(partSlash));
	while (read (p[0], chr, 1) > 0)
	{
		TCHAR filesys [128];
		chr[1] = _T('\0');
		if (chr[0] == _T('\n'))
		{
			char percent [128];
			char mounton [128];
			long total=-1, used, avail;

			buff [nCount] = _T('\0');
			_tcslwr (buff);
			if (nLine > 0)
			{
				int existed = 0;

				if (ContFlag)
				{
					/*
					** The line has wrapped.
					*/
					ContFlag = FALSE;
					sscanf (buff, "%ld %ld %ld %s %s", &total, &used,
						&avail, percent, mounton);
				}
				else
				{
					sscanf (buff, "%s %ld %ld %ld %s %s", filesys,
						&total, &used, &avail, percent, mounton);
				}

				if (total == -1)
				{
					/*
					** We need to wrap this line, as the next line
					** contains the stuff we want.
					*/
					ContFlag = TRUE;
					nCount = 0;
					buff [0] = _T('\0');
				}
				else
				{
					POSITION pos = pDiskInfo->m_listUnit.GetHeadPosition();

					while (pos != NULL)
					{
						CaBarData* pDiskData =
							pDiskInfo->m_listUnit.GetNext (pos);

						if (pDiskData->m_strDiskUnit == filesys ||
						x_strcmp (partSlash, filesys) == 0)
						{
							/*
							** Ignore the substitute partition
							** aready mounted !!!
							*/
							existed = 1;
							break;
						}
					}

					if (existed == 0)
					{
						/*
						** important: The partition '/' is always
						** added at the head of the list
						*/
						if (x_strcmp (mounton, "/") != 0)
						{
							if (total > 0 && used != -1)
							{
								pDiskInfo->AddBarUnit ((LPCTSTR)mounton,
										(double)total/1024.0);
							}
						}
						else
						{
							x_strcpy (partSlash, mounton);
							nSaveSlash = total;
						}
					}
				}
			}

			nLine++;
			nCount = 0;
			buff [0] = _T('\0');
		}
		else
		{
			buff [nCount] = chr[0];
			nCount++;
		}
	}
	close (p[0]);
	if (partSlash[0])
		pDiskInfo->m_listUnit.AddHead (new CaBarData ((LPCTSTR)partSlash, (double)nSaveSlash/1024.0)); 
	//
	// Add the Occupants.
	// Get for all location the space used
	// ie. If multiple locations use the same area then the resulting space
	// is used by that multiple locations.

	POSITION pos = pDiskInfo->m_listLocation.GetHeadPosition();
	while (pos != NULL)
	{
		pLocData = (CaBarPieItem*)pDiskInfo->m_listLocation.GetNext (pos);
		if (pLocData->m_strName.CompareNoCase (strFree) == 0 || pLocData->m_strName.CompareNoCase (strOther)==0)
			continue;
		//
		// Get the location area and add the path to the array.
		//
		arrayPath.RemoveAll();
		res  = GetFirstMonInfo (nNodeHandle, 0, NULL, OT_LOCATION_NODOUBLE, &LocationData, NULL);
		while (res == RES_SUCCESS) 
		{
			if (pLocData->m_strName.CompareNoCase ((LPCTSTR)LocationData.LocationName) == 0)   
			{
				for (int iuse=0; iuse<5; iuse++) 
				{
					if (LocationData.LocationUsages[iuse] == ATTACHED_YES)	 
					{
						int  fExit = 0;

						szCurrDir [0] = '\0';
						if (GetPathLoc((char *)LocationData.LocationArea, &CurrentPath) == TRUE)
							sprintf (szCurrDir, upath[iuse], CurrentPath);
						else
							sprintf (szCurrDir, upath[iuse], LocationData.LocationArea);
						int nM = arrayPath.GetUpperBound();
						for (int k = 0; k < nM; k++)
						{
							CString s = arrayPath.GetAt (k);	
							if (s == szCurrDir)
							{
								fExit = 1;
								break;
							}
						}
						if (fExit == 0)
							arrayPath.Add (szCurrDir);
					}
				}
			}
			res = GetNextMonInfo (&LocationData);
		}
		//
		// Get the space used by this location (single or all path)
		// 
		int  nPath = arrayPath.GetUpperBound();
		long spaceUsed = 0, sum = 0;
		for (int j=0; j<=nPath; j++)
		{
			if (UNIX_SpaceUsedByLocation (nNodeHandle, (LPTSTR)(LPCTSTR)arrayPath.GetAt(j), sum))
				spaceUsed += sum;
		}
		if (spaceUsed > 0L)
		{
			//
			// Find the partition of disk in wihich the current location is belong to.
			double dSpaceUsed = (double)spaceUsed / 1024.0;
			CString strMainDir = szCurrDir;
			POSITION px = pDiskInfo->m_listUnit.GetTailPosition();
			while (px != NULL)
			{
				CaBarData* pDiskData = pDiskInfo->m_listUnit.GetPrev (px);
				if (UNIX_MatchPartition ((LPCTSTR)strMainDir, (LPCTSTR)pDiskData->m_strDiskUnit))
				{
					// 
					// Add Occupant (location)
					pDiskInfo->AddOccupant (pDiskData->m_strDiskUnit, pLocData->m_strName, dSpaceUsed * 100.0 / pDiskData->m_nCapacity);
					break;
				}
			}
		}
	}
	//
	// Add Occupant (Every thing that is not a location)
	// in each partition.
	POSITION px = pDiskInfo->m_listUnit.GetHeadPosition();
	while (px != NULL)
	{
		long Free = 0L;
		CaBarData* pDiskData = pDiskInfo->m_listUnit.GetNext (px);
		POSITION py = pDiskData->m_listOccupant.GetHeadPosition();
		double dSpaceOfLocation = 0.0;
		while (py != NULL)
		{
			CaOccupantData* pOccupantData = pDiskData->m_listOccupant.GetNext (py);
			dSpaceOfLocation += pOccupantData->m_fPercentage;
		}
		Free  = UNIX_GetFreeSpace ((TCHAR*)(LPCTSTR)pDiskData->m_strDiskUnit);
		double dOther = (pDiskData->m_nCapacity - (double)Free/1024.0 - dSpaceOfLocation)*100.0/pDiskData->m_nCapacity;
		pDiskInfo->AddOccupant (pDiskData->m_strDiskUnit, strOther, dOther);
	}
	return TRUE;
}

#else

//
// Return TRUE  : no error.
// Return FALSE : List of drives not available or no information size for 
// a drive.
//  
BOOL Chart_Create(CaBarInfoData* pDiskInfo, int nNodeHandle)
{
   ASSERT (pDiskInfo);
   if (!pDiskInfo)
       return FALSE;
   DWORD dwRet,dwBuffLen,i,dwMaxTab,l,dwNumberLocColour;
   DWORD SectorsPerCluster;       // address of sectors per cluster 
   DWORD BytesPerSector;          // address of bytes per sector 
   DWORD NumberOfFreeClusters;    // address of number of free clusters  
   DWORD TotalNumberOfClusters;    // address of total number of clusters  
   char  szAllDrive[26*MAX_DRIVE_LEN],TabAllDrive[26][MAX_DRIVE_LEN], szCurrDir[_MAX_PATH],szDrive[10];
   char  *CurrentPath, *p;
   double dResult,dSpaceLoc,dPercentLoc,dTemp,dMaxCap,dTotalSpace,dTotalFree;
   double TabFreeAllDrive[26],TabTotalPercentUseloc[26],dTotalOther;
   double dMegaOctet = 1048576; // 1 Mega Bytes = 1024 * 1024
   int Max,Indice,ires;
   POSITION pos;
   CaBarPieItem* pData = NULL;
   CStringArray ArrayPath;
   LOCATIONDATAMIN  LocationData;
   BOOL bTemp = FALSE;

   dwNumberLocColour = 2; // Colours 0 ,1 are reserved for locations 
                          // "Other"and "Free"

   //
   // Clean up old data
   pDiskInfo->Cleanup();
   pDiskInfo->m_strSmallUnit = _T("Kb");
   // Get All locations
   ires  = GetFirstMonInfo( nNodeHandle,0,NULL,OT_LOCATION_NODOUBLE,&LocationData,NULL );
   while (ires==RES_SUCCESS) {
      if (dwNumberLocColour < sizeof (tab_colour)/sizeof(COLORREF))  {
      //
      // Add the locations.
         pDiskInfo->AddBar ((char * )LocationData.LocationName,tab_colour[dwNumberLocColour]);
         dwNumberLocColour++;
      }
      else
      //
      // Add the locations with Random Colour.
         pDiskInfo->AddBar ((char * )LocationData.LocationName);
      ires=GetNextMonInfo(&LocationData);
   }
   CString strOther;
   CString strFree;
   if (strOther.LoadString (IDS_DISKSPACE_OTHER) == 0)
       strOther = _T("Other");
   if (strFree.LoadString (IDS_DISKSPACE_FREE) == 0)
       strFree = _T("Free");
   pDiskInfo->AddBar ((LPCTSTR)strOther,tab_colour[0]);
   pDiskInfo->AddBar ((LPCTSTR)strFree ,tab_colour[1]);

   //
   // Add the disk units.
   // AddBarUnit ("C:", 400.00);     // 400 MB
   memset(TabTotalPercentUseloc,'\0',sizeof(TabTotalPercentUseloc));
   memset(TabFreeAllDrive,'\0',sizeof(TabFreeAllDrive));
      
   // read all drives on this system  
   dwBuffLen = sizeof(szAllDrive)-1;
   dwRet = GetLogicalDriveStrings(dwBuffLen,szAllDrive);
   if (dwRet >dwBuffLen || dwRet == 0)
      return FALSE; 

   p=szAllDrive;
   dwMaxTab=0;
   while (*(p)!= EOS)
   {
      if (STlength(p)>= MAX_DRIVE_LEN )
      {
          myerror(ERR_LIST);
          return FALSE;
      }

      x_strcpy(TabAllDrive[dwMaxTab],p);
      dwMaxTab++;

      while(*(p) != EOS) {
        CMnext(p);
      }
      CMnext(p);
   }
   // Get the Total space and free space for each Drive on this System
   for (l = 0 ;l <= dwMaxTab ;l++ )   {
      if (GetDriveType( TabAllDrive[l] ) == DRIVE_FIXED)  {   // address of root path 
          if ( GetDiskFreeSpace(
                  TabAllDrive[l]     ,       // address of root path 
                  &SectorsPerCluster,       // address of sectors per cluster 
                  &BytesPerSector   ,       // address of bytes per sector 
                  &NumberOfFreeClusters,    // address of number of free clusters  
                  &TotalNumberOfClusters   // address of total number of clusters  
             ) ) {
               dTotalSpace = (double)TotalNumberOfClusters*(BytesPerSector*
                                            SectorsPerCluster);
               dTotalFree = NumberOfFreeClusters*(BytesPerSector*
                                          SectorsPerCluster);
               dResult = dTotalSpace/dMegaOctet;
               GetIndice ( &TabAllDrive[l][0], Indice );
               TabFreeAllDrive[Indice] = (dTotalFree*(double)100)/dTotalSpace ;
               //
               // Add the disk units.

               {
                    CString str =   TabAllDrive[l];
                    pDiskInfo->AddBarUnit ((LPCTSTR)str.Left (2), dResult);
               }
          }
          else
            return FALSE; // error information not availlable
      }
   }
    
   //
   // Add the Occupants.
   // Get for all location the space used

   for ( pos = pDiskInfo->m_listBar.GetHeadPosition(); pos != NULL; pDiskInfo->m_listBar.GetNext(pos))   {
      dSpaceLoc = 0.0;
      dMaxCap = 0;
      memset(szCurrDir,'\0',sizeof(szCurrDir));
      ArrayPath.RemoveAll();
      pData = (CaBarPieItem*)pDiskInfo->m_listBar.GetAt( pos );
      if ( pData->m_strName.CompareNoCase (strFree ) == 0 ||
           pData->m_strName.CompareNoCase (strOther) == 0)
         continue;

      // Get Area for this location and add the path in ArrayPath.
      ires  = GetFirstMonInfo( nNodeHandle,0,NULL,OT_LOCATION_NODOUBLE,&LocationData,NULL );
      while (ires==RES_SUCCESS) {
         if ( pData->m_strName.CompareNoCase ( (char *)LocationData.LocationName ) == 0  )   {
            int iloc;
            for (iloc=0;iloc<5;iloc++) {
              if (LocationData.LocationUsages[iloc] == ATTACHED_YES)   {
                char *pc[5]={DATA_PATH,WORK_PATH,JNL_PATH,CHK_PATH,DMP_PATH};
                if (GetPathLoc((char *)LocationData.LocationArea,&CurrentPath) == TRUE)
                   wsprintf (szCurrDir,pc[iloc],CurrentPath);
                else
                   wsprintf (szCurrDir,pc[iloc],LocationData.LocationArea);
                ArrayPath.Add (szCurrDir);
              }
            }
         }
         ires=GetNextMonInfo(&LocationData);
      }


      // Get the space for this location on all or one path.  
      for (Max = ArrayPath.GetSize(),i = 0,dTemp = 0;(int)i < Max;i++,dTemp = 0)  {
         bTemp = GetSpaceForLocation((LPCTSTR)ArrayPath.GetAt(i) , dTemp , TRUE , TRUE);
         if ( bTemp )
            dSpaceLoc += dTemp ; 
      }
      if (dSpaceLoc == 0.0)
         continue;
      // Get the Capacity for this drive
      fstrncpy((unsigned char *)szDrive,(unsigned char *)szCurrDir,3);
      dMaxCap = pDiskInfo->GetBarCapacity(szDrive);
      if ( dMaxCap > 0.0 )  {
         dPercentLoc = ((dSpaceLoc/dMegaOctet)*(double)100)/dMaxCap;
         //
         // Add the Occupants.
         pDiskInfo->AddOccupant (szDrive,pData->m_strName, dPercentLoc);
         GetIndice ( &szDrive[0], Indice );
         TabTotalPercentUseloc[Indice] += dPercentLoc;
      }
   }   

   // Calculate "Other" for Each drive
   // TabTotalPercentUseloc   =  percentage Cummul used by all loccations for this drive 
   // TabFreeAllDrive         =  Total free space for a drive 
   for (l = 0 ;l < 26 ;l++ )   {
      dTotalOther = 100 - TabTotalPercentUseloc[l] - TabFreeAllDrive[l];
      if (dTotalOther < 100) {
         char szandrive[3];
         Indice  = __toascii(l+65);
         wsprintf ( szandrive,"%c:", Indice);
         //
         // Add the Occupant "other".
         pDiskInfo->AddOccupant ( szandrive, strOther, dTotalOther);
      }
   }
   
   return TRUE;
}
#endif // MAINWIN



