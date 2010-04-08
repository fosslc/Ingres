
/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

/**
** Name:	irmagic.h - ILRF magic number definitions
*/

/*
** FAIL_MAGIC and CLOSE_MAGIC must be different from CLIENT_MAGIC.  Making
** them distinct unlikely values is handy for debugging - it flags that an
** IRBLK has been through a failed session open, or a session close.  We
** use CLIENT_MAGIC as a magic number on return address blocks, also.
** SWAP_MAGIC is a value which resides in the magic number while files
** are temporarily closed.
*/
#define CLIENT_MAGIC 666
#define FAIL_MAGIC (0x1000 ^ CLIENT_MAGIC)
#define CLOSE_MAGIC (0x2000 ^ CLIENT_MAGIC)
#define SWAP_MAGIC (0x3000 ^ CLIENT_MAGIC)

#ifndef ILRFDTRACE
#define CHECKMAGIC(x,n) if (x->magic != CLIENT_MAGIC) return (ILE_CLIENT)
#else
#define CHECKMAGIC(x,n) sess_trace(x,n); if (x->magic != CLIENT_MAGIC) return (ILE_CLIENT)
#endif
#define CHECKCURRENT(x) if (x->curframe == NULL) return (ILE_CFRAME)
