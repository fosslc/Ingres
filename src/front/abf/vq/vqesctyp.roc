/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <fe.h>
# include       <ooclass.h>
# include       <metafrm.h>
# include	"vqescinf.h"
# include	"ervq.h"


/**
** Name:	vqesctyp.roc - define escape types
**
** Description:
**	Statically define the attributes for the escape code types.
**	The ordering of this table corresponds to the definition
**	of escape code types in metafrm.qsh. The first member
**	is a bit mask which indicates which frame types 
**	use the indexed escape types. The second 2 members
**	give the message number of the user readable text 
**	for the escape type, and the header line (description).  
**	Note: there is an additional test in the code which uses 
**	this table because the Query-next, Append-start/end and
**	Delete-start/end escape type are dependent on whether or not 
**	various flags are set.
**
** History:
**	8/10/89 (Mike S)	Initial version
**	9/23/89 (tom)		Eliminating Modify/Display frame types
**	2/91 (Mike S)		Add user meuitem escape type
**	24-jul-92 (blaise)
**		Added new escape types: before-lookup, after-lookup,
**		on-timeout, on-devent, menuline-menuitems and
**		table-field-menuitems (which replaces user-menuitems).
**	22-aug-92 (blaise)
**		Added new (dummy) local procedure escape code type.
**	21-may-93 (blaise)
**		Changed an erroneous "<" to "<<" to make before-lookup &
**		after-lookup valid escape types for an update frame
**		(bug #51592)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/* GLOBALDEF's */
GLOBALDEF const ESCTYPE vqescs[] = 
{
	/* Escape types are 1-based */
	{0, 0, 0},

        {~0, F_VQ00D8_Form_Start, F_VQ00EC_Esc_Edit_Header},

        {~0, F_VQ00D9_Form_End, F_VQ00EC_Esc_Edit_Header},

        {(1<<MF_BROWSE) | (1<<MF_UPDATE),
            F_VQ00DA_Query_Start, F_VQ00EC_Esc_Edit_Header},

        {(1<<MF_BROWSE) | (1<<MF_UPDATE), 
	    F_VQ00DC_Query_New_Data, F_VQ00EC_Esc_Edit_Header},

        {(1<<MF_BROWSE) | (1<<MF_UPDATE),
            F_VQ00DD_Query_End, F_VQ00EC_Esc_Edit_Header},

        {(1<<MF_APPEND) | (1<<MF_UPDATE), 
	    F_VQ00DE_Append_Start, F_VQ00EC_Esc_Edit_Header},

        {(1<<MF_APPEND) | (1<<MF_UPDATE), 
            F_VQ00DF_Append_End, F_VQ00EC_Esc_Edit_Header},

        {(1<<MF_UPDATE),
            F_VQ00E0_Update_Start, F_VQ00EC_Esc_Edit_Header},

        {(1<<MF_UPDATE),
            F_VQ00E1_Update_End, F_VQ00EC_Esc_Edit_Header},

        {(1<<MF_UPDATE),
            F_VQ00E2_Delete_Start, F_VQ00EC_Esc_Edit_Header},

        {(1<<MF_UPDATE),
            F_VQ00E3_Delete_End, F_VQ00EC_Esc_Edit_Header},

        {~0, F_VQ00E4_Menu_Start, F_VQ00EE_Menu_Esc_Edit_Header},
	
        {~0, F_VQ00E5_Menu_End, F_VQ00EE_Menu_Esc_Edit_Header},

        {~0, F_VQ00E6_Field_Enter, F_VQ00ED_Field_Esc_Edit_Header},

        {(1<<MF_APPEND) | (1<<MF_MENU) | (1<<MF_UPDATE), 
	    F_VQ00E7_Field_Change, F_VQ00ED_Field_Esc_Edit_Header},

        {(1<<MF_APPEND) | (1<<MF_MENU) | (1<<MF_UPDATE),
	    F_VQ00E8_Field_Exit, F_VQ00ED_Field_Esc_Edit_Header},

	{(1<<MF_MENU),
	    F_VQ0143_Table_Field_Menuitems, F_VQ00EC_Esc_Edit_Header},
	
	{~0, F_VQ0144_Menuline_Menuitems, F_VQ00EC_Esc_Edit_Header},

	{(1<<MF_APPEND) | (1<<MF_BROWSE) | (1<<MF_UPDATE),
		F_VQ013F_Before_Lookup, F_VQ00EC_Esc_Edit_Header},

	{(1<<MF_APPEND) | (1<<MF_BROWSE) | (1<<MF_UPDATE),
		F_VQ0140_After_Lookup, F_VQ00EC_Esc_Edit_Header},

	{~0, F_VQ0141_On_Timeout, F_VQ00EC_Esc_Edit_Header},

	{~0, F_VQ0142_On_Dbevent, F_VQ00EC_Esc_Edit_Header},

	{0, F_VQ0152_Local_Procedures, F_VQ0153_Loc_Proc_Edit_Header}
};

GLOBALDEF const i4  vqnumesc = sizeof(vqescs) / sizeof(ESCTYPE);
