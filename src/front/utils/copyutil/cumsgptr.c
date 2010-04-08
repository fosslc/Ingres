/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<cu.h>
# include	<er.h>
# include	"ercu.h"

VOID
cu_msgptr(class, name, goodcopy)
OOID	class;
char	*name;
bool	goodcopy;
{
    char 	buf[256];
    char	*IICUrtoRectypeToObjname();
    char	*classname;
    ER_MSGID	msg;

    if ((classname = IICUrtoRectypeToObjname(class)) == NULL)
	return;
    if (goodcopy)
	msg = S_CU0006_GOOD_COPY;
    else
	msg = S_CU0007_BAD_COPY;
    IIUGmsg(ERget(msg), FALSE, 2, (PTR) classname, (PTR) name);
}
