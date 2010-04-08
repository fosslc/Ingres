/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : compress.h
//    data compression for .cfg files
//
//  15-Jan-2001 (noifr01)
//    (SIR 107121) removed obsolete #ifdefs
********************************************************************/
#ifndef _COMPRESS_H
#define _COMPRESS_H

// Function prototypes
int DBAReadCompressedData(FILEIDENT fident, void * bufresu, size_t cb);
int DBACompress4Save(FILEIDENT fident, void * buf, UINT cb);
BOOL CompressFlush (FILEIDENT fident);
void CompressFree(void);

#endif      // _COMPRESS_H
