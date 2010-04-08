/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : cmixfile.cpp 
**
**    Project  : CA-OpenIngres/VDBA - Monitoring.
** 
**    Functions for Save/Load environment 
** 
** History since 14-Apr-2004:
**
** 14-Apr-2004: (uk$so01)
**    BUG #112150 /ISSUE 13350768 
**    VDBA, Load/Save fails to function correctly with .NET environment.
** 19-Mar-2009 (drivi01)
**    In effort to port to Visual Studio 2008, clean up warnings.
*/

#include "stdafx.h"
#include "cmixfile.h"

extern "C" {
#include "dba.h"
#include "domdata.h"
#include "error.h"
#include "dbafile.h"
#include "esltools.h"
#include "compress.h"
};

extern CMixFile CFile4LoadSave; // static because of problems with object 
                                // pointers when exchanged between C an C++
static BOOL bFileInUse;
static BOOL bModeWrite;
CMixFile * GetCMixFile()
{
  if (!bFileInUse)
    return (CMixFile * )0;
  else
    return &CFile4LoadSave;
}

extern "C" {


int DBAFileOpen4Save(LPUCHAR lpfilename, FILEIDENT * fident)
{
  if (bFileInUse)
    return RES_ERR;

  BOOL bRes = CFile4LoadSave.Open((LPCTSTR)lpfilename,
                      CFile::modeCreate|CFile::modeWrite|CFile::typeBinary);
  if (!bRes)
    return RES_ERR;
  
  InitLLFile4Save();
  bFileInUse = TRUE;
  bModeWrite = TRUE;

  return RES_SUCCESS;
}


int DBAAppend4Save(FILEIDENT fident, void *buf, UINT cb)
{
  return LLDBAAppend4Save(CFile4LoadSave.m_pStream, buf, cb);
}

int DBAAppend4SaveNC(FILEIDENT fident, void *buf, UINT cb)
{
  return LLDBAAppend4SaveNC(CFile4LoadSave.m_pStream, buf, cb);
}

int DBAAppendChars4Save(FILEIDENT fident, char c, UINT cb)
{
  return LLDBAAppendChars4Save(CFile4LoadSave.m_pStream, c, cb);
}

int DBAFileOpen4Read(LPUCHAR lpfilename, FILEIDENT * fident)
{
  if (bFileInUse)
    return RES_ERR;

  BOOL bRes = CFile4LoadSave.Open((LPCTSTR)lpfilename,CFile::modeRead|CFile::typeBinary);
  if (!bRes)
    return RES_ERR;
  
  InitLLFile4Read();
  bFileInUse = TRUE;
  bModeWrite = FALSE;

  return RES_SUCCESS;
}

int DBAReadData(FILEIDENT fident, void *bufresu, UINT cb)
{
  return LLDBAReadData(CFile4LoadSave.m_pStream, bufresu, cb, NULL);
}

int DBACloseFile(FILEIDENT fident)
{
  int ires,ires1;
  if (!bFileInUse) {
    myerror(ERR_FILE);
    return RES_ERR;
  }
  ires=RES_SUCCESS;
  try
  {
     if (bModeWrite) {
       char * pc=(char *)ESL_AllocMem(ENDOFFILEEMPTYCHARS);
       if (!pc)
          ires=RES_ERR;
       else {
          if (LLDBAAppend4Save(CFile4LoadSave.m_pStream, pc, ENDOFFILEEMPTYCHARS)!=RES_SUCCESS)
            ires=RES_ERR;
          ESL_FreeMem(pc);
       }
     }
     ires1 = LLDBATerminateFile(CFile4LoadSave.m_pStream);
     if (ires1!=RES_SUCCESS)
       ires=ires1;
     CFile4LoadSave.Close();
  }
  catch( CFileException* pFExcept )
  {
    pFExcept->Delete();
    ires=RES_ERR;
  }

  bFileInUse = FALSE;

  return ires;
}

}; // extern "C"


UINT CMixFile :: Read(void* lpBuf, UINT nCount)
{
  UINT ActualCb;
  int result = LLDBAReadData(CFile4LoadSave.m_pStream, lpBuf, nCount, &ActualCb);
  if (result !=RES_SUCCESS && result != RES_ENDOFDATA)    // Modified EMB 19/03
#ifdef MAINWIN
  		 AfxThrowFileException(CFileException::generic, -1, m_strFileName);
#else
  		 AfxThrowFileException(CFileException::genericException, _doserrno, m_strFileName);
#endif
	  return ActualCb;  // otherwise exception would have been thrown
};

void CMixFile :: Write(const void* lpBuf, UINT nCount)
{
    if (LLDBAAppend4Save(CFile4LoadSave.m_pStream, (void *)lpBuf, nCount)!=RES_SUCCESS)
#ifdef MAINWIN
		  AfxThrowFileException(CFileException::generic, -1, m_strFileName);
#else
		  AfxThrowFileException(CFileException::genericException, _doserrno, m_strFileName);
#endif
};

ULONGLONG CMixFile :: Seek(LONGLONG lOff, UINT nFrom)
{
  ULONGLONG lres;
  if (!LLDBAisCmprsActive())
    return CStdioFile::Seek(lOff,nFrom);

  if (nFrom!=current) {
    myerror(ERR_FILE);
    return 0L;
  }
  lres = (ULONGLONG)LLDBASeekRel(CFile4LoadSave.m_pStream,(long)lOff);
  if (lres<0)
#ifdef MAINWIN
		AfxThrowFileException(CFileException::badSeek, -1,	m_strFileName);
#else
		AfxThrowFileException(CFileException::badSeek, _doserrno,	m_strFileName);
#endif
  return lres;
}
