/*
**	Copyright (c) 1985, 2003 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ol.h -	Compatibility Library OL Interface Definition File.
**
** Description:
**	Include file for datatypes and constants in the Operation Language
**	module of the compatibility library.
**
** History:
**	xx/xx (xxx) -- 
**		First written.
**	12/84 (jhw) -- 
**		Brought over from VMS for initial 3.0 integration
**	11-dec-92 (davel)
**		Added OL_LANGINFO struct, and IIolLangInfo[] global declaration,
**		which replaces global IIolLang[].
**	11-jan-93 (davel)
**		Added OL_MAX_RETLEN define.
**	12-feb-93 (davel)
**		several 6.5 changes: 
**		1. changed OL_PV's OL_value to PTR.
**		2. added structure OL_VSTR.
**		3. added OL_DEC as a legal parameter type.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	7/93 (Mike S) Add OL_PTR, OL_ptr, and OL_STRUCT_PTR
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	07-feb-2003 (abbjo03)
**	    Remove PL/1 as a supported language.
*/

# ifndef OLHDEF
# define OLHDEF

typedef struct ol_struct *OL_STRUCT_PTR;

typedef struct {
	i4	OL_type;
	PTR	OL_value;
	i4	OL_size;
} OL_PV;

typedef struct {
	i2	slen;
	char	string[1];
} OL_VSTR;

typedef struct {
	char	*name;			/* host langauge name; e.g. "Fortran" */
	i4	support;		/* bitmask indicating support level: */
# define	OL_I_DEFINED		1
# define	OL_I_HAS_DECIMAL	2

} OL_LANGINFO;

/*
** Note: This typedef for OL_RET also appears in ~ingres/files/oslhdr.h.
**	 Any changes to this typedef, therefore, must also appear there.
*/
typedef union
{
	OL_STRUCT_PTR OL_ptr;
	i4	OL_int;
	f8	OL_float;
	char	*OL_string;
} OL_RET;

/* OL_MAX_RETLEN is the size of buffer that should be set in the OL_RET
** OL_string member before calling OLpcall, if return value is a string.
*/
# define	OL_MAX_RETLEN	4096+sizeof(i2)+1

/*
** The possible values for OL_type.
*/
# define	OL_NOTYPE	0
# define	OL_I4	1
# define	OL_F8	2
# define	OL_DEC	3
# define	OL_STR	6
# define	OL_PTR	7

/*
** The languages
*/
# define	OLC		0
# define	OLFORTRAN	1
# define	OLPASCAL	2
# define	OLBASIC		3
# define	OLCOBOL		4
# define	OLADA		6
# define	OLOSL		7
# define	OLSQL		8

# define	_OLMAX		8

/*
** Name:	IIolLangInfo[] -	Host-Language Map.
*/
GLOBALREF OL_LANGINFO	IIolLangInfo[_OLMAX+1];

/*{
** Name:	OLanguage() -	Return Name for Host-Language Type.
**
** Description:
**	Returns the name of the host-language type.
**
** Input:
**	ol_type	{i4}  The language type one of OLC, etc.
**
** Returns:
**	{char *}  A reference to the language name, or NULL on error.
**
** History:
**	02/89 (jhw) -- Written.
**	11-dec-92 (davel)
**		modifed to use new IIolLangInfo global.
*/
#define OLanguage(ol_type) (((ol_type) < 0 || (ol_type) > _OLMAX) \
				? NULL : IIolLangInfo[ol_type].name)

#define IIOLanguage OLanguage

/*{
** Name:	OLang_info() -	Return Support level for Host-Language Type.
**
** Description:
**	Returns word with bits set to indicate degree of supportability.
**
** Input:
**	ol_type	{i4}  The language type one of OLC, etc.
**
** Returns:
**	{i4}	A word indicating the degree of supportability; zero or more
**		of the following bits:
**			OL_I_DEFINED
**			OL_I_HAS_DECIMAL
**
** History:
**	11-dec-92 (davel)
**		Written.
*/
#define OLang_info(ol_type) (((ol_type) < 0 || (ol_type) > _OLMAX) \
				? 0 : IIolLangInfo[ol_type].support)

#define IIOLlang_info OLang_info
# endif /* OLHDEF */
