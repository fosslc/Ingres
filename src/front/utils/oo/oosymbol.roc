/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
**
**	Change history:
**
**	12-Dec-1991 (wojtek)
**	    Renamed _init to _iiOOinit  to avoid symbol clash with C
**	    library (references also renamed elsewhere).
**	3-aug-1992 (fraser)
**		Changed _flush, etc. to ii_flush because of conflict with
**		Microsoft C library.
**	27-jan-93 (blaise)
**		The previous change changed all tabs in this file to spaces;
**		changed back to tabs again.
**	8-feb-93 (fraser)
**		Changed "flushAll" to "IIflushAll" because function name
**		was changed.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile. Added LIBOBJECT hint which creates a rule
**	    that copies specified object file to II_SYSTEM/ingres/lib.
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
LIBOBJECT = oosymbol.roc
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<oosymbol.h>

GLOBALCONSTDEF char	_ []		= ERx("");
GLOBALCONSTDEF char	_altr_count []	= ERx("alter_count");
GLOBALCONSTDEF char	_altr_date []	= ERx("alter_date");
GLOBALCONSTDEF char	_alloc []	= ERx("alloc");
GLOBALCONSTDEF char	_at []		= ERx("at");
GLOBALCONSTDEF char	_atEnd []	= ERx("atEnd");
GLOBALCONSTDEF char	_atPut []	= ERx("atPut");
GLOBALCONSTDEF char	_authorized[]	= ERx("authorized");
GLOBALCONSTDEF char	_catForm []	= ERx("catForm");
GLOBALCONSTDEF char	_catInit []	= ERx("catInit");
GLOBALCONSTDEF char	_cattable []	= ERx("cattable");
GLOBALCONSTDEF char	_confirmName[]	= ERx("confirmName");
GLOBALCONSTDEF char	_class []	= ERx("class");
GLOBALCONSTDEF char	_copy []	= ERx("copy");
GLOBALCONSTDEF char	_copyId []	= ERx("copyId");
GLOBALCONSTDEF char	_create_date []	= ERx("create_date");
GLOBALCONSTDEF char	_currentName []	= ERx("currentName");
GLOBALCONSTDEF char	_decode []	= ERx("decode");
GLOBALCONSTDEF char	_destroy []	= ERx("destroy");
GLOBALCONSTDEF char	_do []		= ERx("do");
GLOBALCONSTDEF char	_edit []	= ERx("edit");
GLOBALCONSTDEF char	_encode []	= ERx("encode");
GLOBALCONSTDEF char	_env []		= ERx("env");
GLOBALCONSTDEF char	_estring []	= ERx("estring");
GLOBALCONSTDEF char	_fetch []	= ERx("fetch");
GLOBALCONSTDEF char	_fetch0 []	= ERx("fetch0");
GLOBALCONSTDEF char	_fetchAll []	= ERx("fetchAll");
GLOBALCONSTDEF char	_fetchDb []	= ERx("fetchDb");
GLOBALCONSTDEF char	_first []	= ERx("first");
GLOBALCONSTDEF char	ii_flush []	= ERx("flush");
GLOBALCONSTDEF char	ii_flush0 []	= ERx("flush0");
GLOBALCONSTDEF char	ii_flushAll []	= ERx("IIflushAll");
GLOBALCONSTDEF char	_id []		= ERx("id");
GLOBALCONSTDEF char	_ii_longremarks []	= ERx("ii_longremarks");
GLOBALCONSTDEF char	_ii_objects []	= ERx("ii_objects");
GLOBALCONSTDEF char	_iicatalog []	= ERx("iicatalog");
GLOBALCONSTDEF char	_iidetail []	= ERx("iidetail");
GLOBALCONSTDEF char	_iisave []	= ERx("iisave");
GLOBALCONSTDEF char	_iiOOinit []	= ERx("init");
GLOBALCONSTDEF char	_init0 []	= ERx("init0");
GLOBALCONSTDEF char	_initDb []	= ERx("initDb");
GLOBALCONSTDEF char	_insertrow []	= ERx("insertrow");
GLOBALCONSTDEF char	_isEmpty []	= ERx("isEmpty");
GLOBALCONSTDEF char	_isNil []	= ERx("isNil");
GLOBALCONSTDEF char	_is_current []	= ERx("is_current");
GLOBALCONSTDEF char	_last_altered_by []	= ERx("last_altered_by");
GLOBALCONSTDEF char	_long_remark []	= ERx("long_remark");
GLOBALCONSTDEF char	_markChange []	= ERx("markChange");
GLOBALCONSTDEF char	_mlook []	= ERx("mlook");
GLOBALCONSTDEF char	_name []	= ERx("name");
GLOBALCONSTDEF char	_nameOk []	= ERx("nameOk");
GLOBALCONSTDEF char	_new []	= ERx("new");
GLOBALCONSTDEF char	_new0 []	= ERx("new0");
GLOBALCONSTDEF char	_newDb []	= ERx("newDb");
GLOBALCONSTDEF char	_next []	= ERx("next");
GLOBALCONSTDEF char	_noMethod []	= ERx("noMethod");
GLOBALCONSTDEF char	_object []	= ERx("object");
GLOBALCONSTDEF char	_owner []	= ERx("owner");
GLOBALCONSTDEF char	_ownercode []	= ERx("ownercode");
GLOBALCONSTDEF char	_perform []	= ERx("perform");
GLOBALCONSTDEF char	_print []	= ERx("print");
GLOBALCONSTDEF char	_readfile []	= ERx("readfile");
GLOBALCONSTDEF char	_rename []	= ERx("rename");
GLOBALCONSTDEF char	_sequence []	= ERx("sequence");
GLOBALCONSTDEF char	_setPermit []	= ERx("setPermit");
GLOBALCONSTDEF char	_setRefNull []	= ERx("setRefNull");
GLOBALCONSTDEF char	_short_remark []	= ERx("short_remark");
GLOBALCONSTDEF char	_subClass []	= ERx("subClass");
GLOBALCONSTDEF char	_subResp []	= ERx("subResp");
GLOBALCONSTDEF char	_time []	= ERx("time");
GLOBALCONSTDEF char	_title []	= ERx("title");
GLOBALCONSTDEF char	_vig []		= ERx("vig");
GLOBALCONSTDEF char	_withName []	= ERx("withName");
GLOBALCONSTDEF char	_writefile []	= ERx("writefile");
