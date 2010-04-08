/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmldbf.h: header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation of data of dBASE file 
**
** History:
**
** 15-Feb-2001 (uk$so01)
**    Created
** 21-Jan-2002 (uk$so01)
**    (bug 106844). Additional fix: Replace ifstream and some of CFile.by FILE*.
**/

#if !defined (DMLDBF_HEADER)
#define DMLDBF_HEADER
#include "formdata.h"



//
// DBF_QueryRecord
//
// Summary: Query the DBF records.
// Parameters:
//    hwndAnimate : Animate dialog, if this parameter is not NULL, the function can interact 
//                  with animation dialog for showing the progression ...
//    pData       : import assistant data (page1, ..., page3)
//    pFile       : dbf file to read.
//    strOutRecord: not affected if nQueryMode = 0.
//                  if nQueryMode = 2, the formated record to the varchar(0)
//                  i.e each field is prefixed by a 5-characters length.
//    nQueryMode  : 0, strOutRecord is not affected and scan the file to update pData.
//                  1, strOutRecord is not affected and scan the file to skip the header
//                     until the firt record.
//                  2, read the record one by one, you must call with nQueryMode = 1 first.
// Throw exception as integer (int)
// 1: unsupport version of DBF file
// 2: cannot read file.
// 3: The number of columns in dbf file is not equal to the number of columns in the table.
// ************************************************************************************************
void DBF_QueryRecord(HWND hwndAnimate, CaIIAInfo* pData, FILE* pFile, CString& strOutRecord, int nQueryMode = 0);

#endif //DMLDBF_HEADER



