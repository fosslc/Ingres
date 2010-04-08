// SaveLoad.cpp : implementation of serialization mechanisms for mfc documents
// integrated with our "old" save/load C management
//
/*
**
**
** History:
**
** 19-Mar-2009 (drivi01)
**    In effort to port to Visual Studio 2008, clean up warnings.
**
*/

#include "stdafx.h"

#include "saveload.h"
#include "resmfc.h"
#include "mainfrm.h"
#include "mainmfc.h"

#include "cmixfile.h"

extern "C" {
#include "dbafile.h"      // ARCHIVEBUFSIZE
};

static BOOL OpenNamedDocument(CString reqDocName)
{
	CWinApp* pWinApp = AfxGetApp();
  ASSERT (pWinApp);
  POSITION pos = pWinApp->GetFirstDocTemplatePosition();
  while (pos) {
    CDocTemplate *pTemplate = pWinApp->GetNextDocTemplate(pos);
    CString docName;
    pTemplate->GetDocString(docName, CDocTemplate::docName);
    if (docName == reqDocName) {
      // prepare detached document
      CDocument *pDoc = pTemplate->CreateNewDocument();
      ASSERT(pDoc);
      if (pDoc) {
        // load document data from archive file
        CMixFile *pMixFile = GetCMixFile();
        CArchive ar(pMixFile, CArchive::load, ARCHIVEBUFSIZE);
        try {
          DWORD dw = (DWORD)pMixFile->GetPosition();
          ASSERT (dw);
          pDoc->Serialize(ar);
          ar.Close();
        }
        // a file exception occured
        catch (CFileException* pFileException)
        {
          pFileException->Delete();
          ar.Close();
          return FALSE;
        }
        // an archive exception occured
        catch (CArchiveException* pArchiveException)
        {
          VDBA_ArchiveExceptionMessage(pArchiveException);
          pArchiveException->Delete();
          return FALSE;
        }

        // Associate to a new frame
        CFrameWnd *pFrame = pTemplate->CreateNewFrame(pDoc, NULL);
        ASSERT (pFrame);
        if (pFrame) {
          pTemplate->InitialUpdateFrame(pFrame, pDoc);
          return TRUE;
        }
      }
    }
  }

  return FALSE;
}

BOOL OpenNewMonitorWindow()
{
  return OpenNamedDocument("Ipm");
}

BOOL OpenNewDbeventWindow()
{
  return OpenNamedDocument("DbEvent");
}

BOOL OpenNewSqlactWindow()
{
  return OpenNamedDocument("SqlAct");
}

BOOL OpenNewNodeselWindow()
{
  return OpenNamedDocument("Virtual Node");
}

static BOOL SaveMfcDocument(HWND hWnd)
{
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hWnd);
  ASSERT (pWnd);
  if (!pWnd)
    return FALSE;
  CFrameWnd *pFrame = (CFrameWnd *)pWnd;
  CDocument *pDoc = pFrame->GetActiveDocument();
  ASSERT (pDoc);
  if (!pDoc)
    return FALSE;

  CMixFile *pMixFile = GetCMixFile();
  CArchive ar(pMixFile, CArchive::store, ARCHIVEBUFSIZE);
  try {
    DWORD dw = (DWORD)pMixFile->GetPosition();
    ASSERT (dw);
    pDoc->Serialize(ar);
    ar.Close();
  }
  // a file exception occured
  catch (CFileException* pFileException)
  {
    pFileException->Delete();
    ar.Close();
    return FALSE;
  }
  // an archive exception occured
  catch (CArchiveException* pArchiveException)
  {
    VDBA_ArchiveExceptionMessage(pArchiveException);
    pArchiveException->Delete();
    return FALSE;
  }

  return TRUE;
}

BOOL SaveNewMonitorWindow(HWND hWnd)
{
  return SaveMfcDocument(hWnd);
}

BOOL SaveNewDbeventWindow(HWND hWnd)
{
  return SaveMfcDocument(hWnd);
}

BOOL SaveNewSqlactWindow(HWND hWnd)
{
  return SaveMfcDocument(hWnd);
}
BOOL SaveNewNodeselWindow(HWND hWnd)
{
  return SaveMfcDocument(hWnd);
}

BOOL SaveExNonMfcSpecialData(HWND hWnd)
{
  return SaveMfcDocument(hWnd);
}
