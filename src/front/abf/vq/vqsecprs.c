/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<abclass.h>
# include	<metafrm.h>
# include	<erfg.h>
# include	<eros.h>
# include	"ervq.h"
# include	"vqsecnam.h"
/**
** Name:	vqsecprs.c -  Parse section name
**
** Description:
**	This file defines:
**
**	IIVQpsnParseSectionName		Parse Error file section name
**
** History:
**	11/13/89 (Mike S) Initial version
**	24-jul-92 (blaise)
**		Added new escape code types: before-lookup, after-lookup,
**		on-timeout, on-dbevent, menuline-menuitems and
**		table-field-menuitems (which replaces user-menuitems)
**	13-aug-92 (blaise)
**		Added local procedures section name.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/* # define's */
# define HASH_SIZE 63		/* Size of hash table */

/* Type for translating error section names to numeric types */
typedef struct
{
	ER_MSGID	msgid;	/* Error message ID for section name */
	i4		major;	/* major type number */
	i4		minor;	/* minor type number */
}	SECTION_NAMES;

/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN	STATUS IIUGhiHtabInit();
FUNC_EXTERN	STATUS IIUGhfHtabFind();
FUNC_EXTERN	STATUS IIUGheHtabEnter();

/* static's */
static	bool	init_done = FALSE;
static	PTR	hashctx;		/* Hash table context */	

static const SECTION_NAMES section_names[] =
{
	{F_FG0014_Hidden, 	EST_HIDDEN, 	0},
	{F_FG0015_Call,		EST_CALL,	0},
	{F_FG0016_Insert,	EST_INSERT,	0},
	{F_FG0017_Select,	EST_SELECT,	0},
	{F_FG001A_Form_Start,	EST_ESCAPE,	ESC_FORM_START},
	{F_FG001B_Form_End,	EST_ESCAPE,	ESC_FORM_END},
	{F_FG001C_Query_Start,	EST_ESCAPE,	ESC_QRY_START},
	{F_FG002A_Query_New_Data,EST_ESCAPE,	ESC_QRY_NEW_DATA},
	{F_FG001E_Query_End,	EST_ESCAPE,	ESC_QRY_END},
	{F_FG001F_Append_Start,	EST_ESCAPE,	ESC_APP_START},
	{F_FG0020_Append_End,	EST_ESCAPE,	ESC_APP_END},
	{F_FG0021_Update_Start,	EST_ESCAPE,	ESC_UPD_START},
	{F_FG0022_Update_End,	EST_ESCAPE,	ESC_UPD_END},
	{F_FG0023_Delete_Start,	EST_ESCAPE,	ESC_DEL_START},
	{F_FG0024_Delete_End,	EST_ESCAPE,	ESC_DEL_END},
	{F_FG0025_Menu_Start,	EST_ESCAPE,	ESC_MENU_START},
	{F_FG0026_Menu_End,	EST_ESCAPE,	ESC_MENU_END},
	{F_FG0027_Field_Enter,	EST_ESCAPE,	ESC_FLD_ENTRY},
	{F_FG0028_Field_Change,	EST_ESCAPE,	ESC_FLD_CHANGE},
	{F_FG0029_Field_Exit,	EST_ESCAPE,	ESC_FLD_EXIT},
	{F_FG0043_Table_Field_Menuitems,EST_ESCAPE,	ESC_TF_MNUITM},
	{F_FG0044_Menuline_Menuitems,EST_ESCAPE,	ESC_ML_MNUITM},
	{F_FG003E_Before_Lookup,EST_ESCAPE,	ESC_BEF_LOOKUP},
	{F_FG003F_After_Lookup,	EST_ESCAPE,	ESC_AFT_LOOKUP},
	{F_FG0041_On_Timeout,	EST_ESCAPE,	ESC_ON_TIMEOUT},
	{F_FG0042_On_Dbevent,	EST_ESCAPE,	ESC_ON_DBEVENT},
	{F_FG0045_LocalProcs,  EST_LOCPROC,	0},
	{F_FG0030_Default,	EST_DEFAULT,	0},
	{F_OS0019_NoSection,	EST_NONE,	0},
	{F_FG002B_Source_Code,	EST_SOURCE,	0},
	{0,			0,		0}
};

/*{
** Name:	IIVQpsnParseSectionName         Parse Error file section name
**
** Description:
**	Parse the section name from a 4GL listing file.  Divide the section 
**	name into a major part, and an optional minor part and name.
**
** Inputs:
**	section		char *		section name
**
** Outputs:
**	major_type	i4 *		e.g. Escape Code, Hidden Field
**	minor_type	i4 *		e.g. Escape code type, master section
**	name		char *		e.g. Escape code field name
**
**	Returns:
**		none
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	11/13/89 (Mike S)	Initial version
*/
VOID 
IIVQpsnParseSectionName(section, major_type, minor_type, name)
char	*section;
i4  	*major_type;
i4	*minor_type;
char	*name;
{
	char seccopy[80];
	char *divider;
	SECTION_NAMES *snptr;
	PTR	*data;

	if (!init_done)
	{
		/* Initialize hash table */
		_VOID_ IIUGhiHtabInit(HASH_SIZE, NULL, NULL, NULL, &hashctx);

		/* Fill hash table */
		for (snptr = section_names; snptr->msgid != 0; snptr++)
			_VOID_ IIUGheHtabEnter(hashctx, ERget(snptr->msgid), 
					       (PTR)snptr);
		
		init_done = TRUE;
	}

	/* Get major part of name */
	STcopy(section, seccopy);
	divider = STindex(seccopy, ERx("\\"), 0);
	if (divider != NULL) *divider = EOS;

	/* Try to recognize major part of name */
	if (IIUGhfHtabFind(hashctx, seccopy, &data) != OK)
	{
		*major_type = EST_UNKNOWN;
		return;
	}
	
	snptr = (SECTION_NAMES *)data;	
	*major_type = snptr->major;
	*minor_type = snptr->minor;
	if (divider != NULL) 
		STcopy(++divider, name);
	else
		*name = EOS;

	/* If it's a query, translate query section name */
	if (snptr->major == EST_INSERT || snptr->major == EST_SELECT)
	{
		if (STcompare(name, ERget(F_FG0018_Master)) == 0)
		{
			*minor_type = EST_QMASTER;
		}
		else if (STcompare(name, ERget(F_FG0019_Detail)) == 0)
		{
			*minor_type = EST_QDETAIL;
		}
		else
		{
			*major_type = EST_UNKNOWN;
		}
	}
}
