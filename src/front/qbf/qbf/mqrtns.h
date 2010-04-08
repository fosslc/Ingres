/*
**static char	Sccsid[] = "@(#)mqrtns.h	30.1	11/14/84";
*/

/*
**	MQRTNS.h  - This file contains various routine 
**		    declarations used in MQBF.
**
**	Copyright (c) 2004 Ingres Corporation
*/
MQQDEF	*mqistruct();
QDESC	*mqsetparam();
FRAME	*mq_makefrm();
i4	mqgetwidth();
i4	mqfindatt();
i4	mqfindjoin();
i4	mqfindtbl();
i4	mqretinit();
i4	mqgetmaster();
i4	mqgetrecord();
i4	mqgetdtl();
i4	mqgetscroll();
i4	mqdelrest();
char	*mqsaveStr();
i4	mq_mflds();
i4	mqtblinqdef();
