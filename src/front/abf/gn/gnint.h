
/*
**Copyright (c) 1987, 2004 Ingres Corporation
** All rights reserved.
*/

/**
** Name:	gnint.h - GN internal names
**
** Description
**	defines names for internal use in GN library which do not
**	have IIGN, and are hence easier to read.  Nobody outside GN
**	should access these symbols.
*/

#define List		IIGN1List
#define find_space	IIGN2find_space
#define make_name	IIGN3make_name
#define hash_func	IIGN4hash_func
#define comp_func	IIGN5comp_func
#define Clen		IIGN6Clen
#define Hlen		IIGN7Hlen
#define legalize	IIGN8legalize
#define Igncase		IIGN9Igncase

FUNC_EXTERN SPACE *IIGN2find_space();
