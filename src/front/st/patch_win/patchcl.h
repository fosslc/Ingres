/*
**  Copyright (c) 2005 Ingres Corporation. All Rights Reserved.
*/

/*
**  Name: patchcl.h: Header file used by installer executables
**                   and static/shared libaries.
**
**  History:
**		01-Dec-2005 (drivi01)
**			Created.
**		16-jun-2006 (drivi01)
**			Updated PATCH_SIZE to 10 characters due to the _SOL suffix
**			that need to be added for SOL patches.
**		20-may-2009 (smeke01) b121784
**			Added IsSuffixed() helper function.
**
*/

#define II_SIZE	3
#define	PATCH_SIZE	10
#define BUFF_SIZE	1024
#define CKSUM_SIZE   64
#define	SHORT_BUFF	 32
#define SMALLER_BUFF 512


VOID AppendToLog(char *str);
unsigned long checksum(char *filename); 
BOOL IsSuffixed(const char * thestring, const char * thesuffix);
