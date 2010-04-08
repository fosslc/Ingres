/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : CellIntC.h, header file
**
**  Project  : CA-OpenIngres/Visual DBA.
**
**  Author   : Emmanuel Blattes
**
**  Purpose  : C interface for Cellular Control to display page usage
**		of Ingres Table
**
**  History:
**	15-feb-2000 (somsa01)
**	    Removed extra comma from pageType declaration.
**	28-apr-2009 (smeke01) b121545
**	    Enable display of ISAM index levels 1 to 9.
*/

#ifndef CELLINTC_HEADER
#define CELLINTC_HEADER

#ifndef __cplusplus
#error  CellIntC.h cannot be included in non-cplusplus files
#endif

extern "C" {
#include "dba.h"
}

typedef enum {
  PAGE_HEADER = 1,          // Free Header page
  PAGE_MAP,                 // Free Map    page
  PAGE_FREE,                // Free page, has been used and subsequently made free
  PAGE_ROOT,                // Root page
  PAGE_INDEX,               // Index page
  PAGE_INDEX_1,				// Index page level 1
  PAGE_INDEX_2,				// Index page level 2
  PAGE_INDEX_3,				// Index page level 3
  PAGE_INDEX_4,				// Index page level 4
  PAGE_INDEX_5,				// Index page level 5
  PAGE_INDEX_6,				// Index page level 6
  PAGE_INDEX_7,				// Index page level 7
  PAGE_INDEX_8,				// Index page level 8
  PAGE_INDEX_9,				// Index page level 9
  PAGE_SPRIG,               // Sprig page
  PAGE_LEAF,                // Leaf page
  PAGE_OVERFLOW_LEAF,       // Overflow leaf page
  PAGE_OVERFLOW_DATA,       // Overflow data page
  PAGE_ASSOCIATED,          // Associated data page
  PAGE_DATA,                // Data page
  PAGE_UNUSED,              // Unused page, has never been written to
  // hash specific (for Ingres >= 2.5)
  PAGE_EMPTY_DATA_NO_OVF,   // Empty data, with no overflow page
  PAGE_DATA_WITH_OVF,       // Non-Empty data, with overflow pages
  PAGE_EMPTY_DATA_WITH_OVF, // Empty data, wih overflow pages
  PAGE_EMPTY_OVERFLOW,      // Empty overflow page
  // end of hash specific section (for ingres >2.5)
  PAGE_ERROR_UNKNOWN        // UNKNOWN TYPE!
} pageType;

#define FILLFACTORBITS  6

typedef struct ingrespageinfo
{
   long ltotal;
   long linuse;
   long loverflow;
   long lfreed;
   long lneverused;

   long lItemsPerByte;
   long nbElements;

   BOOL bNewHash;
   long lbuckets_no_ovf;
   long lbuck_with_ovf;
   long lemptybuckets_no_ovf;
   long lemptybuckets_with_ovf;
   long loverflow_not_empty;
   long loverflow_empty;
   long llongest_ovf_chain;
   long lbuck_longestovfchain;
   double favg_ovf_chain;

   CByteArray* pByteArray;

} INGRESPAGEINFO, FAR * LPINGRESPAGEINFO;

BOOL GetIngresPageInfo(int nNodeHandle, LPUCHAR dbName, LPUCHAR tableName, LPUCHAR tableOwner, LPINGRESPAGEINFO lpPageInfo);

BYTE ConvertPageCharToByte(char ch);
int GetFillFactor(BYTE by, int itemsPerByte);
int GetOverflowFactor(BYTE by, int itemsPerByte);

#endif  // CELLINTC_HEADER
