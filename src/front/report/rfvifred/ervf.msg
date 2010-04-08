/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ervf.msg -	ViFrEd Message Definitions File.
**
** Description:
**	This file defines the messages for class VF.
**
** History:
**	19-aug-1987 (dave)
**		Created.
**	10/25/87 (dkh) - Error message cleanup.
**	11/12/87 (dkh) - Fixed typos.
**	16-nov-87 (bruceb)
**		Added messages for scrolling fields.
**	11/28/87 (dkh) - Added testing support for calling QBF.
**	04-dec-87 (bruceb)
**		Added scrolling field messages.
**	12/05/87 (dkh) - Added message F_VF0044_Col_sep.
**	12/11/87 (dkh) - Added message F_VF0045_blank.
**	02/24/88 (dkh) - Fixed jup bug 1856.
**	07-apr-88 (sylviap)
**		Added error message for GO/COMPILE, E_VF011A_NOT_IMPLEMEN.
**	02-may-88 (sylviap)
**		Added period to error E_VF0011.
**	05/23/88 (dkh) - Added new ids for scrollable fields.
**			 Ids added were F_VF0046_scroll_title,
**			 F_VF0047_scroll_sztitle, E_VF011B_scroll_sync,
**			 E_VF011C_bad_scroll_val and E_VF011D_bad_scroll_len.
**	05/23/88 (dkh) - Integrated Tom's changes as well to keep both
**			 paths working.
**	05/25/88 (dkh) - Integrated more message changes by Tom so both
**			 paths will continue to work.
**	05/25/88 (dkh) - Added new ids E_VF011E_ColScrollFmt and
**			 E_VF011F_ColScrollSize.
**	05/26/88 (dkh) - Added id E_VF0120_scroll_badattr
**	05/27/88 (dkh) - More integration of Tom's messages so both paths'
**			 messages will show up in the mnx files.
**	07/19/88 (tom) - added extended error messages.
**	20-july-88 (sylviap)
**		Added FE_MAXNAME to error message E_VF001D (#2957)
**	08/03/88 (tom) - fixed some spelling errors.
**	08/26/88 (tom) - made the 002C an error not Slow.. and 
**			 extended error messages 002C 002D.
**	08/29/88 (tom) - extended messages, and commenting.
**	10/19/88 (tom) - extended 0106 and 0109 error messages.
**	11/30/88 (tom) - put in 11A error message for CMS not implemented
**			 function
**	08-dec-88 (sylviap)
**		Took out error message, 11A (calls QBF and COMPILE) for DG.
**	16-dec-88 (bruceb)
**		Added message for INVISIBLE field attribute.
**	05-apr-89 (bruceb)
**		Copied a modified E_VF011D to E_VF0121 and removed use of
**		the 11D message; scrollsz field of vifattr frame is now an
**		integer field, not a char field.
**	07-apr-89 (bruceb)
**		Added E_VF0122 and E_VF0123 for VQLOCK'd fields.
**	05/17/89 (dkh) - Fixed bug 3757.
**	20-jun-89 (bruceb)
**		Added new msgs for derived fields.
**	08/07/89 (dkh) - Added support for 80/132 runtime switching.
**	08/09/89 (dkh) - Added support for DuplcateField feature.
**	06-sep-89 (bruceb)
**		Added message for too-large display formats.
**	07-sep-89 (bruceb)
**		Added messages for trim attribute feature.
**	10/06/89 (dkh) - Fixed bug 2115.
**	02-nov-89 (bruceb)
**		Added messages for new ABF-to-VIFRED interface.
**	22-nov-89 (cmr)
**		Added message for Create Aggregate.
**	01-dec-89 (bruceb)
**		Added help file titles for confirmation popups.
**	05-dec-89 (cmr)
**		Changed S_VF00B5... and S_VF00B9... which are for RBF 
**		only.
**	14-dec-89 (bruceb)
**		Added strings for role id.
**	12/23/89 (dkh) - Added support for owner.table.
**	14-feb-90 (bruceb)
**		Changed ellipses to match new style guide.
**	05-apr-90 (bruceb)
**		E_VF0121 no longer used.  Now use E_VF0164.  (Added new param.)
**	26-apr-90 (pete)
**		Changed VQ to Visual Query in two messages.
**	5/22/90 (martym)
**		Added F_VF0093_BrkOpt.
**	08-jun-90 (bruceb)
**		Added messages for create table field's GetTableDef.
**	05-sep-90 (sylviap)
**		Added explanations to the menuitems. #32699
**	08/02/91 (dkh) - Augmented error message E_VF012C to include
**			 Query Only.  Part of fix for bug 38378.
**	08/10/92 (dkh) - Added messages for 6.5.
**	09/25/92 (dkh) - Added messages for owner.table support.
**	12/12/92 (dkh) - Added message to support input masks.
**	12/18/92 (dkh) - Modified message E_VF0168 to reflect reality.
**	05-jan-93 (sylviap)
**		Added E_VF0177_no_jd_name to support creating/editing a joindef
**		from vifred. Also added F_VF00C0_EditExpl.
**	02/25/93 (dkh) - Added message E_VF0178_ALL_COLS_UNSUP to let user
**			 know that vifred could not find any supported
**			 datatype columns.
**	26-jan-94 (connie) Bug #57718
**		Added E_VF0006_no_joindef_name, E_VF0009_Can_t_retrieve_QBF_Jo
**			 
*/

# define	_VF_CLASS	139

/*
**	attr.qc
*/

S_VF0001_Can_not_access_form	
"Can not access form \"%s\", check configuration"
/*%
** Explanation:
**	VIFRED could not find a form it needed for it's internal
**	operations, that is one of it's own forms.
**
** System Status:
**	VIFRED will abort. This messages is only displayed 
**	in a syserr call.
**
** User Action:
**	Fix the configuration, insure that the formindex file
**	is present.
*/


F_VF0001_Box_Field	"Box Field"

F_VF0002_Keep_Previous_Value	"Keep Previous Value"

F_VF0003_Mandatory_Field	"Mandatory Field"

F_VF0004_Reverse_Video	"Reverse Video"

F_VF0005_Blinking	"Blinking"

F_VF0006_Underline	"Underline"

F_VF0007_Brightness_Change	"Brightness Change"

F_VF0008_Query_Only	"Query Only"

F_VF0009_Force_Lower_Case	"Force Lower Case"

F_VF000A_Force_Upper_Case	"Force Upper Case"

F_VF000B_No_Auto_Tab	"No Auto Tab"

F_VF000C_No_Echo	"No Echo"

F_VF000D_Display_Only	"Display Only"

F_VF000E_END_OF_ATTRIBUTES	"END OF ATTRIBUTES"

F_VF000F_Default_Value_for_Fie	"Default Value for Field:"

E_VF0002_Cant_find_field
"Cannot find internal structures for attributes.\n
VIFRED was not able to locate the necessary internal
data structures to allow you to edit the attributes
of a field.  Therefore, VIFRED will cancel the editing
attributes operation and return control back to the
LAYOUT menu."

/*%
** Explanation:
**	VIFRED could not find the internal structures for a
**	field to allow a user to edit the attributes of a field.
**	(internal error??)
**
** System Status:
**	The editing attributes operation will be skipped by
**	VIFRED and control will return to the layout menu.
**
** User Action:
**	User can continue with other editing operations or
**	try saving the form in the database and then re-editing
**	the form again.
*/

F_VF0010_Color	"Color:"

E_VF0003_Could_not_decode_data
"Could not determine field datatype information.\n
The datatype for the field is not known to VIFRED.
Please enter a new datatype for the field when the
Attributes form is displayed."

/*%
** Explanation:
**	The datatype for a field has become corrupted.  VIFRED
**	will thus not be able to display the datatype for the
**	field in the Attributes form.
**
** System Status:
**	VIFRED will continue and display the rest of the
**	attributes for the field.
**
** User Action:
**	User should enter a datatype for the field when the
**	Attributes form is displayed.
*/

E_VF0004_Cant_set_both
"Can't set both Display Only and Query Only.\n
It does not make sense to set both the Display Only and Query Only
attributes on the same field.  Please unset one of them."

/*%
** Explanation:
**	User has set both the Display Only and Query Only attributes
**	for a field.  This is not allowed.  User must unset one of them.
**
** System Status:
**	VIFRED will stop processing the attributes for the current
**	field and wait for user to unset one of the attributes.
**
** User Action:
**	User must unset either the Display Only or Query Only
**	attribute.
*/

E_VF0005_Cant_set_low_up
"Cannot set both Lower Case and Upper Case Attributes.\n
It does not make sense to set both of the aforementioned
attributes.  Please unset one of them."

/*%
** Explanation:
**	User has set both the Lower Case and Upper Case attributes
**	for a field.  This is not allowed.  User must unset one of them.
**
** System Status:
**	VIFRED will stop processing the attributes for the current
**	field and wait for user to unset one of the attributes.
**
** User Action:
**	User must unset either the Lower Case or Upper Case
**	attribute.
*/

E_VF0007_not_valid_datatype
"\"%0c\" is not a valid datatype.\n
You have entered an invalid datatype.  Please correct it.
To obtain list of valid datatypes, use the Help menu choice."

/*%
** Explanation:
**	User has entered an unknown datatype.
**
** System Status:
**	VIFRED will stop processing the attributes for the current
**	field and wait for user to correct the datatype.
**
** User Action:
**	User must enter a correct datatype.
*/

E_VF0008_datatype_not_compat
"Datatype not compatible with field format.\n
The datatype \"%0c\" is not compatible with field format \"%1c\".
Please change the datatype."

/*%
** Explanation:
**	User has entered a datatype that is not compatible with
**	the field format.  For example, a datatype of INTEGER
**	is not compatible with a "c10" field format.
**
** System Status:
**	VIFRED will stop processing the attributes for the current
**	field and wait for user to change the datatype.
**
** User Action:
**	User must change the datatype for the field.
*/


E_VF000A_internal_name_exists
"Duplicate name \"%0c\" already exists for another field.\n
Each field of a form must have a unique name within the form.  For table field
columns, names must be unique within the table field.  Please change the
internal name."

/*%
** Explanation:
**	User has entered a field name that is not unique among
**	regular fields in a form or columns in a table field.
**
** System Status:
**	VIFRED will stop processing the attributes for the current
**	field and wait for user to change the internal field name.
**
** User Action:
**	User must change the internal field name for the field.
*/

E_VF000B_Cant_box_this_field
"Field is too close to the edge to be boxed.\n
Putting a box around the field would cause it to extend beyond
the form boundaries. You must move the field away from the edge of the 
form before setting the box attribute."

/*%
** Explanation:
**	User had set the box attribute but there is not enough
**	room to do so.
**
** System Status:
**	VIFRED will unset the box attribute and continue processing.
**
** User Action:
**	None.
*/

E_VF000C_There_are_no_other_at	
"There are no other attributes to edit.\n
There is only one field (or column) defined. 
Moving to the Next or Previous is not applicable."

/*%
** Explanation:
**	User tried to go to the previous or next field or column 
**	when editing the attributes of a field or table field column.
**
** System Status:
**	Resume processing attribute edit.
**
** User Action:
**	None	
*/

S_VF000D_Field_Attributes	"Field Attributes"

E_VF000E_Error_in_getting_defa
"Error in determining default datatype.\n
VIFRED could not obtain a default datatype based on the field's
format.  Please enter a datatype when the Attributes form is
displayed."

/*%
** Explanation:
**	An internal error prevented VIFRED from creating a
**	default datatype based on the field's format.
**
** System Status:
**	VIFRED will continue processing with no datatype
**	displayed in the Attributes form.
**
** User Action:
**	Enter a datatype for the field when the Attributes
**	form is displayed.
*/

E_VF000F_Error_in_converting_d
"Error in converting datatype information.\n
The datatype for the field is unknown to VIFRED.  Please
enter a datatype when the Attributes form is displayed."

/*%
** Explanation:
**	An internal error prevented VIFRED from determining
**	the datatype of the field.
**
** System Status:
**	VIFRED will continue processing with no datatype
**	displayed in the Attributes form.
**
** User Action:
**	Enter a datatype for the field when the Attributes
**	form is displayed.
*/

F_VF0011_field	"field"

E_VF0010_First_character_of_fi
"'%1c' is an invalid %0c name.\n
First character of a %0c name must be alphabetic.
An internal %0c name consists of alphanumeric characters
with a leading alphabetic character.  The underscore '_'
character is considered an alphabetic character.  Please
correct the %0c name."

/*%
** Explanation:
**	User has entered non-alphanumerics or non-alphabetic
**	first character for the internal field name.
**
** System Status:
**	VIFRED will stop processing the attributes for the current
**	field and wait for user to correct the internal field name.
**
** User Action:
**	User must correct the internal field name for the field.
*/

E_VF0011_Character_not_ok
"'%2c' is an invalid %1c name.\n
Character `%0c' not allowed in a %1c name.
An internal %1c name consists of alphanumeric characters
with a leading alphabetic character.  The underscore '_'
character is considered an alphabetic character.  Please
correct the %1c name."

/*%
** Explanation:
**	User has entered non-alphanumerics or non-alphabetic
**	in an internal field name.
**
** System Status:
**	VIFRED will stop processing the attributes for the current
**	field and wait for user to correct the internal field name.
**
** User Action:
**	User must correct the internal field name for the field.
*/

/*
**	vfcat.qc
*/
E_VF0012_Empty_Find
"There are no forms in the catalog.\n
The 'Find' command is not relevant if there are no forms
in the catalog because there would be nothing to find. You
can create a new form by invoking the 'Create' menu option."
/*%
** Explanation:
**	The user has attempted to 'find' a form when
**	there are no forms in the catalog.
**	
** System Status:
**	Form catalog operations continue.
**
** User Action:
**	Create a form before attempting find.
*/

F_VF0012_VIFRED_Forms_Catalo	"VIFRED - Forms Catalog"

S_VF0013_Retrieving_forms	"Retrieving form '%0c' . . ."

S_VF0014_Creating_default_form	"Creating default form from table '%0c' . . ."

F_VF0013_BlankForm	"BlankForm"

F_VF0014_TableDefault	"TableDefault"

S_VF0015_Table	"  Table: "

F_VF0015_SimpleFields	"SimpleFields"

S_VF0016_Cr_Tabl	"Create Form from Table Submenu"

F_VF0016_JoindefDefault	"JoindefDefault"

S_VF0017_Create_Form_Submenu	"Create Form Submenu"

S_VF0018_Do_you_wish_to_destro	"Do you wish to destroy %0c `%1c'? "

S_VF0019_Destroying	"Destroying %0c `%1c' . . ."

E_VF001A_Deadlock_has_occurred
"Destroy not done due to deadlock condition\n
The form you have requested to destroy has not been destroyed because
of a deadlock condition. This is caused by other programs accessing
the system catalogs when VIFRED was attempting to update them.
You may be successful if you wait and try the operation later."
/*%
** Explanation:
**	Deadlock condition during form destroy.
**
** System Status:
**	Form is not destroyed due to deadlock.
**
** User Action:
**	Try again later.
*/

E_VF001B_Error_occurred_in_del
"Error occurred while destroying form.\n
Form NOT deleted from base catalogs.  Please try again."

/*
** Explanation:
**	Error (internal?) occurred while destroying a form.
**	
**
** System Status:
**	The form was not deleted (maybe corrupted?)
**
** User Action:
**	Try again later
*/

S_VF001C_Enter_new_name
"Enter new name: "

E_VF001D_not_legal_name
"`%0c' is not a legal %1c name.\n
A form name consists of no more than %2d alphanumeric characters with a
leading alphabetic character.  The underscore '_' character
is considered an alphabetic."

/*%
** Explanation:
**	The user has entered a form name that is illegal during
**	a rename request.
**
** System Status:
**	Resume catalog editing.
**
** User Action:
**	Enter a good name during renaming.
*/

E_VF001E_Form_already_exists	"Form `%0c' already exists.\n
Please use a different form name."

/*%
** Explanation:
**
** System Status:
**
** User Action:
*/

S_VF001F_QBFName_exists
"QBFName '%0c' already exists.  Do you still want to Rename?"

S_VF0020_Renaming	"Renaming %0c '%1c' to '%2c' . . ."

E_VF0021_Deadlock_has_occurred
"Rename not done due to deadlock condition.\n
The form you have requested to rename has not been renamed because
of a deadlock condition. This is caused by other programs accessing
the system catalogs when VIFRED was attempting to update them.
You may be successful if you wait and try the operation later."
/*%
** Explanation:
**	Deadlock condition during rename.
**
** System Status:
**	Form name is not changed due to deadlock.
**
** User Action:
**	Try again later.
*/

E_VF0022_Error_occurred_in_cha
"Error occurred during form rename.\n
Form name was not changed. Please try again later."
/*%
** Explanation:
**	Error (internal?) during rename.
**
** System Status:
**	Form name is not changed due to error.
**
** User Action:
**	Try again later.
*/

E_VF0023_Renaming
"Error renaming %0c `%1c'.\n
The Rename operation has not successfully completed."

/*%
** Explanation:
**	Rename operation had problems in the oo rename function.
**
** System Status:
**	System catalogs are in a compromised state. Although
**	the form has been renamed in the base catalogs.. it
**	has not in the OO catalogs.
**	
** User Action:
**	???
**
*/

F_VF0017_VIFRED_Detail_on_a	"VIFRED - MoreInfo about a Form"

S_VF0024_File_in_which_to_comp	"File in which to compile form: "

S_VF0025_Compiling_Form	"Compiling Form . . ."

S_VF0026_File_in_which_to_prin	"File in which to print form: "

S_VF0027_Utilities_Submenu	"Utilities Submenu"

S_VF0028_Table_to_use_with_for	"Table to use with form \"%0c\": "

E_VF0029_Table_does_not	
"Table \"%0c\" does not exist.\n
The table you have requested to be used with this form for 
an invocation of QBF does not exist. You must enter the
name of an existing table."
/*%
** Explanation:
**	The user has requested to invoke Qbf with a form
**	and a table, only the table doesn't exist.
**
** System Status:
**	The previous menu operation is resumed.
**	
** User Action:
**	Enter a valid table name.
**
*/

S_VF002A_Form_Catalogs	"Forms Catalog"

E_VF002B_ExecutionError	
"Error when executing %0c.\n
An error status of `%1x' was returned when the 
program %0c was executed. Please check your configuration
to insure that the program is available."

/*%
** Explanation:
**	The program could not be executed, or returned an 
**	error status when executed.
**
** System Status:
**	Operation resumes.
**
** User Action:
**	Look into configuration and insure that the 
**	executable is available.
*/

/*
**	cursor.qc
*/
E_VF002C_Warning_Duplicates	
"Warning - Duplicate internal name(s) found.\n
You may continue to edit the form but before saving the form it 
will be necessary to change some of the field or column names
so that each one is unique. The form can not be saved with
duplicate names."

/*%
** Explanation:
**	The user has probably created a tabledefault form from more
**	than one table.. and the tables had a column name which is 
**	the same. This causes a duplicate internal name for some
**	of the fields.
**
** System Status:
**	VIFRED will allow this condition to exist, but will
**	not allow saving the form until the condition is remedied.
**
** User Action:
**	The user must edit the field attributes and change
**	the name of the duplicates so that all names are
**	unique.
*/


E_VF002D_Cannot_save_form_beca
"The form can not be saved due to duplicate names.\n
The form contains several fields or columns which have the
same name. You must edit the field attributes and enter
unique internal names for all fields and columns before 
the form may be saved."
/*%
** Explanation:
**	The user has probably created a tabledefault form from more
**	than one table.. and the tables had a column name which is 
**	the same. This causes a duplicate internal name for some
**	of the fields.
**
** System Status:
**	VIFRED will allow this condition to exist, but will
**	not allow saving the form until the condition is remedied.
**
** User Action:
**	The user must edit the field attributes and change
**	the name of the duplicates so that all names are
**	unique.
*/


S_VF002E_Changes_not_saved
"Changes have not been saved.  Would you like to End anyway?"

S_VF002F_Report_Layout_Frame	"Report Layout Frame"

S_VF0030_Form_Layout_Frame	"Form Layout Frame"

S_VF0031_Changes_not_saved
"Changes have not been saved.  Would you like to Quit anyway?"

/*
**	vfdlock.c
*/
S_VF0032_Deadlock_occurred_whi
"Deadlock occurred while writing form, retrying . . ."

/*
**	debug.c
*/
S_VF0033_vfTestAlloc_cant_co
"vfTestAlloc: can't convert test file to LOCATION\n"	/* DIAG */

S_VF0034_vfTestAlloc_cant_op
"vfTestAlloc: can't open %s, test file\n"	/* DIAG */

/*
**	delete.c
*/
E_VF0035_Can_not_delete_right	
"Cannot delete right margin marker.\n
The right margin marker is not a feature of the form which
can be deleted. It is only present to show you where the
right edge of the form is."

/*%
** Explanation:
**	The user has attempted to delete the right form margin.
**
** System Status:
**	Resume cursor mode edit.
**
** User Action:
**	None
*/

E_VF0036_Cant_delete_the_last	
"Cannot delete the end of form marker.\n
The end of form marker is not a feature of the form which
can be deleted. It is only present to show you where the
bottom of the form is."

/*%
** Explanation:
**	The user has attempted to delete the end of form marker.
**
** System Status:
**	Resume cursor mode edit.
**
** User Action:
**	None
*/

E_VF0037_Cant_delete_the_last	
"Cannot delete the last detail component.\n
You must have at least one detail component in a report. Deleting
the last detail component on the report would result in nothing
being reported."

/*%
** Explanation:
**	The user has attempted to delete last detail component
**	on a report spec.
**
** System Status:
**	Resume cursor mode edit.
**
** User Action:
**	None
*/

E_VF0038_Cant_delete_a_RBF_Se	
"Cannot delete a RBF Section Line.\n
These lines are not features which may be deleted. They are
only present to show you where the section divisions are."

/*%
** Explanation:
**	The user has attempted to delete an RBF section line.
**
** System Status:
**	Resume cursor mode edit.
**
** User Action:
**	None
*/

E_VF0039_Cant_delete_a_line	
"Cannot delete a line with a feature on it.\n
You must delete all features from a line before
the line may be deleted."

/*%
** Explanation:
**	The user has attempted to delete a line with features on it.
**
** System Status:
**	Resume cursor mode edit.
**
** User Action:
**	None
*/

/* 003A seems to be the same condition as that which causes
   0036.. except this is triggered if, somehow the special endFrame
   feature is not on the last line.... */

E_VF003A_Cant_delete_the_last	
"Cannot delete the end of form marker.\n
The end of form line is not a feature of the form which
can be deleted. It is only present to show you where the
bottom of the form is."

/*%
** Explanation:
**	The user has attempted to delete the end of form marker.
**
** System Status:
**	Resume cursor mode edit.
**
** User Action:
**	None
*/

E_VF003B_Cant_delete_all_the	
"Cannot delete all the lines in a form.\n
A form must have at least one line."

/*%
** Explanation:
**	The user has attempted to delete the last line of 
**	a form (such to create a 0 length form).
**
** System Status:
**	Resume cursor mode edit.
**
** User Action:
**	None
*/

F_VF0018_End	"End"

E_VF003C_Cant_insert_line_ins	
"Cannot insert line here.\n
Lines can't be inserted inside of a form component. Move to 
a line which either has only the starting line of components,
or has no components on it at all."
/*%
** Explanation:
**	The user has attempted to insert a line
**	into the middle of a component.
**
** System Status:
**	Resume cursor mode edit.
**
** User Action:
**	None
*/

/*
**	edit.c
*/

E_VF003D_Can_only_select_the_E
"Can only select the EDIT command when on a component.\n
You must first position the cursor on a component and
then you may select to edit it."

/*%
** Explanation:
**	The user has attempted to edit a component of the form
**	but is not positioned on any component.
**
** System Status:
**	Resume cursor mode edit.
**
** User Action:
**	None
*/

S_VF003E_Editing_Detail	"Editing Detail"

S_VF003F_Editing_Field	"Editing Field"

F_VF0019_Edit_title	"Edit title (press <MENU KEY> when done)"

F_VF001A_Edit_format	"Edit format (press <MENU KEY> when done)"

F_VF001B_Edit_trim	"Edit trim (press <MENU KEY> when done)"

E_VF0040_Cant_edit_a_RBF_sect	
"Cannot edit a RBF section line.\n
These lines are only displayed so that you may
see the division between sections."

/*%
** Explanation:
**	The user has attempted to edit an RBF section line.
**
** System Status:
**	Resume cursor mode edit.
**
** User Action:
**	None
*/

E_VF0041_Cant_edit_End_of_For	
"Cannot edit End-of-Form line.\n
This line is not an editable feature of the form. It is 
only displayed so that you may see where the end of the
form is."

/*%
** Explanation:
**	The user has attempted to edit the End-of-Form line.
**
** System Status:
**	Resume cursor mode edit.
**
** User Action:
**	None
*/

S_VF0042_Unknown_position_stru	"Unknown position structure in editTrim(%d)"

F_VF001C_Editing_Trim	"Editing Trim"

F_VF001D_Editing_Title	"Editing Title"

F_VF001E_Editing_Data_Window	"Editing Data Window"

E_VF0044_not_legal_fmt	
"'%0c' is not a legal format string\n
You must enter a valid display format string for the field. 
The major display format types are: c=character; i=integer;
f,e,g,n=floating point. For example 'i3' would 
define an integer field which is 3 characters wide
(select Help and then SubTopics to see more display format examples)."
/*%
** Explanation:
**	User has entered a bad display format string.
**
** System Status:
**	The bad string is discarded.
**
** User Action:
**	Reenter the format string.
*/

E_VF0045_Failed_format_compati	
"Internal error: Failed format check\n
Check of display format failed when determining display format
data type compatibility check."
/*%
** Explanation:
**	A routine: fmt_vfvalid returns error even though
**	the display format was already checked.
**
** System Status:
**	The user must reenter a string.
**
** User Action:
**	Reenter the format string.
*/


E_VF0046_New_fmt_not_compat	
"The display format is not compatible with datatype\n
The display format must agree in type with the field's datatype."
/*%
** Explanation:
**	RBF error.. not VIFRED.
**	The display format and the datatype do not agree.
**
** System Status:
**	The user must reenter a format.
**
** User Action:
**	Reenter the format string.
*/

E_VF0047_Error_checking_for_co	
"Internal error:  checking for coercibility.\n
Please reenter the display format string"
/*%
** Explanation:
**	A routine: afe_cancoerce returns error even though
**	the display format was already checked.
**
** System Status:
**	The user must reenter a string.
**
** User Action:
**	Reenter the format string.
*/

E_VF0048_no_fmtstruct	
"Internal error:  building display format structure.\n
Please reenter the display format string."
/*%
** Explanation:
**	A routine: afe_cancoerce returns error even though
**	the display format was already checked.
**
** System Status:
**	The user must reenter a string.
**
** User Action:
**	Reenter the format string.
*/


E_VF0049_Failed_format_compati	
"Internal error:  failed display format check.\n
Please reenter the display format string."
/*%
** Explanation:
**	A routine: fmt_vfvalid returns error even though
**	the display format was already checked.
**
** System Status:
**	The user must reenter a string.
**
** User Action:
**	Reenter the format string.
*/

E_VF004A_Fmt_to_data
"Internal error:  determining data type from display format.\n
Please reenter the display format string."
/*%
** Explanation:
**	A routine: fmt_ftot  returns error even though
**	the display format was already checked, and a valid
**	display format should always map successfully to 
**	a data type.
**
** System Status:
**	The user must reenter a string.
**
** User Action:
**	Reenter the format string.
*/


F_VF0020_End	"End"

/*
**	vfedbox.qsc 
*/

S_VF0124_Mv_to_new_corner	
"Move cursor to new corner position (press <MENU KEY> when done.)"


/*
**	enter.c
*/
F_VF0047_Pos_opp_corner
"Move cursor to position the opposite corner (press <MENU KEY> when done)"
/* This is done during box creation */

F_VF0021_Enter_trim	"Enter trim (press <MENU KEY> when done.)"

S_VF004D_Create_Field_Submenu	"Create Field Submenu"

E_VF004E_no_compat
"Display format and datatype are not compatible.\n
The field display format and the data type of the field are not
compatible with each other. For example an 'i3' integer display format
would not be compatible with a varchar data type. You must edit 
the display format and/or the datatype so that they are compatible."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/


E_VF0051_empty
"Missing empty internal name found for this %0c.\n
An internal %1c name consists of alphanumeric characters
with a leading alphabetic character.  The underscore '_'
character is considered an alphabetic character.  Please
enter a %1c name."
/*
** Explanation:
**	The internal name of the current field is 0 length.
**
** System Status:
**	Resume field creation submenu.
**
** User Action:
**	Select attributes option and enter a 
**	valid internal field name.
*/


E_VF0052_name	"Duplicate name \"%0c\" for this field already exists.\n
Each field of a form must have a unique name within the form.  The default name
assigned to this field already belongs to another field on the form.  You must
select the Attributes menu option and enter a unique internal name for the
field."

/*%
** Explanation:
**	The internal name of the current field already exists.
**	(happens when the title is used to create the internal
**	field name)
**
** System Status:
**	Resume field creation submenu.
**
** User Action:
**	Select attributes option and enter a 
**	valid internal field name.
*/

E_VF0053_enter_a_fmt
"You must enter a display format for the new field.\n
The major display format types are: c=character; i=integer;
f,e,g,n=floating point. For example 'i3' would 
define an integer field which is 3 characters wide
(select Help and then SubTopics to see more display format examples)."
/*%
** Explanation:
**	The user has attempted to end creation of a field without
**	entering a display format for the field.
**
** System Status:
**	Resume field creation submenu.
**
** User Action:
**	Select DisplayFormat menu option, and enter a
**	display format string.
*/

E_VF0054_enter_a_title
"You must enter a title for the new field.\n
All fields must have titles, field creation can't be
completed without entering one."
/*%
** Explanation:
**	The user has attempted to end creation of a field without
**	entering a title.
**
** System Status:
**	Resume field creation submenu.
**
** User Action:
**	Select Title menu option, and enter a title for
**	the field.
*/

E_VF0055_title_fmt
"No title or display format for the new field.\n
You must enter a title and display format for all fields,
field creation can't be completed without them."
/*%
** Explanation:
**	The user has attempted to end creation of a field without
**	entering a title or display format.
**
** System Status:
**	Resume field creation submenu.
**
** User Action:
**	Select Title and DisplayFormat menu options, 
**	and enter a title and display format for
**	the field.
*/

/* opinion: 56 below should not be an error.. we should just allow the 
   re-editing of the title if the user wants to (tom) */

E_VF0056_A_title_has_already_b
"A title has already been entered for the new field.\n
To change the title you have entered you must complete creation
of the field and then invoke the Edit option on this field"
/*%
** Explanation:
**	The user has attempted to edit the title twice during
**	field creation.
**
** System Status:
**	Resume field creation submenu.
**
** User Action:
**	Continue with field definition.
*/

E_VF0057_Cant_put_title_here
"Cannot enter a title here.\n
The cursor is within the data entry area of the field. You must
enter a title at a position which will not overlap the data
entry area of the field."
/*%
** Explanation:
**	The user has initiated entry of a title while 
**	the cursor is positioned within the area designated
**	by the format string.
**
** System Status:
**	Resume field creation submenu.
**
** User Action:
**	Move the cursor to a position which is clear of the 
**	data entry area of the field and enter a title there.
*/

F_VF0022_Enter_title	"Enter title (press <MENU KEY> when done)"

E_VF0059_Title_Overlaps_fmt	
"The Title overlaps the data entry area.\n
The field title is not allowed to overlap the data entry area of the
field."
/*%
** Explanation:
**	The user entered a field title which overlaps the data
**	entry area of the field.
**
** System Status:
**	Resume field creation submenu.
**
** User Action:
**	Move the cursor to a position which is clear of the 
**	data entry area of the field and enter a title there.
*/

E_VF005A_Cant_put_fmt
"Cannot enter the display format here.\n
The display format you have entered would cause the 
data entry area of the field and the field title to overlap."
/*%
** Explanation:
**	The user attempted entry of a display format which 
**	would cause the field title and the data entry area of
**	the field to overlap.
**
** System Status:
**	Resume field creation submenu.
**
** User Action:
**	Move the cursor to a position which is clear of the 
**	title of the field and enter a display format there.
*/

F_VF0023_Enter_format	"Enter format (press <MENU KEY> when done)"

F_VF0024_Editing_format	"Editing format"

S_VF005C_Inconsistency_while_c	"Inconsistency while creating new field."

S_VF005D_Could_not_build_forma	"Could not build format structure."

/*
**	fasttodb.qc
*/

E_VF0060_Unable_to_open_a_temp
"Unable to open a temporary file for writing out form.  Check
if you can create files in your current working directory (or
II_TEMPORARY, if set)."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_VF0064_Unable_to_relo
"ERROR: Unable to relocate pointers for compiled form."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_VF0065_Unable_to_conv
"ERROR: Unable to convert filename to LOCATION"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_VF0066_Unable_to_open
"ERROR: Unable to open intermediate file."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

S_VF0067_read_err
"VIFRED: ERROR: Unable to read intermediate file"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

/*
**	fdtosmlfd.c
*/
S_VF0068_FDToSmlfd__passed_non	"FDToSmlfd: passed non field position.\n"

/*
**	feature.c
*/
S_VF0069__rnewFeature__unknown	"\rnewFeature: unknown type of position (%d.)\n"

S_VF006A__r_nmoveTab__passed_n	"\r\nmoveTab: passed non table field.\n\r"

/*
**	fmt.c
*/
E_VF006B_Invalid_format_specif
"Invalid display format specification.\n
The major display format types are: c=character, i=integer,
f,e,g,n=floating point type. For example 'i3' would 
define an integer field which is 3 characters wide
(select Help and then SubTopics to see more display format examples)."
/*% 
** Explanation:
**	The user has attempted to end creation of a field without
**	entering a display format for the field.
**
** System Status:
**	Resume field creation submenu.
**
** User Action:
**	Select DisplayFormat menu option, and enter a
**	display format string.
*/



/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_VF006C_Could_not_create_defa
"Could not create default format."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

/*
**	frame.c
*/
S_VF006D_VIFRED__vfFmtdisp__Fa
"VIFRED: vfFmtdisp: Failed to get display layout."

/*
**	getargs.c
*/
S_VF006E_Missing_database_name	"Missing database name.\n"

S_VF006F_Database	"Database"

S_VF0070_Missing_form_name_n	"Missing form name.\n"

S_VF0071_Form_name	"Form name"

S_VF0072_Missing_joindef_name_	"Missing JoinDef name.\n"

S_VF0073_Joindef_name	"Joindef name"

S_VF0074_Missing_table_name_n	"Missing table name.\n"

S_VF0075_Table_name	"Table name"

S_VF0076_Bad_flag___s__in_comm	"Bad flag '%s' in command line.\n"

S_VF0077_Too_many_arguments_in	"Too many arguments in command line.\n"

/*
**	getfrmnam.qc
*/
S_VF0078_VIFRED_Saving_a_Form	"VIFRED - Saving a Form"

/*
**	getrel.c
*/
E_VF0079_Unable_to_create_form
"Unable to create forms with more than %0d fields."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

S_VF007A_Unable_to_build_form	"Unable to build form from table(s)."

/*
**	iivifred.c
*/
/*
**	lines.c
*/
S_VF007B_buildLines__null_fiel	"buildLines: null field in frame."

S_VF007C_buildLines__null_non_	"buildLines: null non seq field in frame."

S_VF007D_buildLines__null_trim	"buildLines: null trim in frame."

F_VF0025_End_of_Form	"End-of-Form"

F_VF0026_Title	"Title"

F_VF0027_Column_Headings	"Column-Headings"

F_VF0028_Detail_Lines	"Detail-Lines"

F_VF0029_End_of_Detail	"End-of-Detail"

S_VF007E_INSFLD__can_t_create_	"INSFLD: can't create default format\n"

S_VF007F__rexisting___d___d___	"\rexisting (%d, %d) (%d, %d)\n"

S_VF0080__rnew___d___d____d___	"\rnew (%d, %d) (%d, %d)\n"

S_VF0081__rerror_in_existing_f	"\rerror in existing frame\n"

/*
**	node.c
*/
S_VF0082_vflnNext__Can_t_get_a	"vflnNext: Cannot get adjacency %d for %d"

/*
**	pos.c
*/
S_VF0083_ndonPos__can_t_find_n	"ndonPos: can't find node for %d"

S_VF0084_unLink__can_t_find_po	"unLink: can't find pos %d in line %d\n"

/*
**	qbfmap.qc
*/
E_VF0085_Could_not_get_detaile
"Could not get detailed data on qbfname `%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_VF0086_Too_many_entries_foun
"Too many entries found for qbfname `%0c'"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

S_VF0087_Table_Name	"Table Name:"

S_VF0088_Joindef_Name	"Joindef Name:"

S_VF0089_Form_Name	"Form Name:"

S_VF008A_Can_not_access_form	"Can not access form \"vfqbfcat\"."

S_VF008B_Retrieving_catalogs	"Retrieving catalogs . . ."

S_VF008C_VIFRED_QBFNames_Cat	"VIFRED - QBFNames Catalog for form \"%s\"."

S_VF008D_Name_to_use_with_QBF	"Name to use with QBF : "

S_VF008E_Create_QBFName_Submen	"Create QBFName Submenu"

S_VF008F_You_do_not_own	"You do not own %0c `%1c'"

S_VF0090_Do_you_wish_to_destor	"Do you wish to destroy %0c `%1c'? "

S_VF0091_Destroying	"Destroying %0c `%1c'"

S_VF0092_Deadlock_has_occurred
"Deadlock has occurred too many times.  Entry not deleted"

S_VF0093_You_are_not_the_owner	"You are not the owner of QBFName \"%0c\""

S_VF0094_Editing_a_qbfname	"VIFRED - Editing a QBFName"

S_VF0095_Detail_on_a_qbfname	"VIFRED - MoreInfo about a QBFName"

S_VF0096_QBFName_Catalogs	"QBFName Catalogs"

S_VF0099_already_exist	"%0c '%1c' already exists."

S_VF009C_Table_to_use	"Table to use : "

S_VF009D_Join_Definition	"Join Definition (JoinDef) name to use : "

E_VF009E_Error_occurred_create
"Error occurred created QBFNAME object"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

S_VF009F_Saving_a_qbfname	"VIFRED - Saving a QBFName"

/*
**	rbf.qc
*/
E_VF00A0_Can_only_give_a_headi
"Can only give a heading to a column heading or detail line entry"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_VF00A1_Can_only_give_a_headi
"Can only give a heading to an existing component.  The cursor must be positioned on a field, aggregate, or existing heading before a heading can be created."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_VF00A2_Can_only_give_a_headi
"Can only give a heading to a field, aggregate or existing heading.  The cursor must be positioned on one of these components before a heading can be created."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

F_VF002A_Enter_column_heading
"Enter column heading (press <MENU KEY> when done)"

S_VF00A3_delColumn_passed	"\rdelColumn: passed non-column (%d)\r\n"

S_VF00A4_delColumn_Cant	"\rdelColumn: Cannot find position %d in column %d\r\n"

E_VF00A5_Cant_move_column
"Cannot move column here.\n
Placing the column at the cursor position would cause one 
of the components of the column (field or heading) to extend 
beyond the right report border.  If you want to extend the 
right boundary you must 'Move/Expand' the right form border."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_VF00A6_Cant_move_column
"Cannot move column here.\n
Placing the column at the cursor position would cause one
of the components of the column (field or heading) to extend
beyond the top or bottom report border.  If you want to extend 
the bottom of the report boundary you must 'Move/Expand' the 
bottom report border."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

S_VF00A7_Associated_Field	"Associated Field %s not found\n"

S_VF00A8_Associated_Trim	"Associated Trim %s not found\n"

E_VF00A9_Cursor_must_be_in_det
"Cursor must be in detail section of layout to create a column"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

S_VF00AA_Enter_column_name	"Enter column name: "

S_VF00AB_Column_does_not	"Column '%0c' does not exist in table or view"

S_VF00AC_Col_already	"Column '%0c' already is in the report"

S_VF00AD_Bad_format_found_for	"Bad format found for column \"%0c\""

E_VF00AE_Column_will_not_fit
"Column will not fit this close to the right margin"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_VF00AF_Could_not_get_format
"Could not get format string for column \"%0c\""

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

/*
**	select.c
*/
E_VF00B0_Can_only_select_MOVE
"Can only select MOVE when on a component"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_VF00B1_Cant_move_a_RBF_Sect
"Cannot move a RBF Section Line"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

S_VF00B2_Unknown_position_in	"\rUnknown position in MOVE (%d)"

S_VF00B3_MOVE_Trim_Command	"MOVE (Trim) Command"

S_VF0133_MOVE_Box_Command	"MOVE (Box/Line) Command"

S_VF0131_MOVE_Frm_Command	"MOVE (Form) Command"

S_VF00B4_MOVE_Component_Comm	"MOVE (Component) Command"

S_VF00B5_MOVE_Heading_or_det	"MOVE Command"

S_VF00B6_MOVE_Field_Command	"MOVE (Field) Command"

S_VF00B7_MOVE_Detail_Command	"MOVE (Detail) Command"

S_VF00B8_MOVE_Detail_Command	"MOVE (Detail) Command"

S_VF00B9_Move_Column_Command	"Move (Column/Aggregate) Command"

S_VF00BA_MOVE_Detail_Command	"MOVE (Detail) Command"

S_VF00BB_MOVE_Title_or_fmt	"MOVE (Title or Format) Command"

S_VF00BC_placecmd_inconsisten	"placecmd: inconsistent selObj %d\n"

E_VF00BD_Cant_move_the_field
"Cannot place the field here\n
Placing the field at the cursor position would cause
it to extend beyond the right form border. You must 
'Move/Expand' the right form border first to place the field here."
/*%
** Explanation:
**	The user has attempted to place a field in a position which
**	would cause it to extend beyond the form's right border.
**
** System Status:
**	The Move submenu is resumed.
**
** User Action:
**	Try another location to place the field, or
**	move the form border first to allow placement
**	of the field at this location.
*/

E_VF00BE_Cant_move_the_field
"Cannot place the field here\n
Placing the field at the cursor position would cause
it to extend out of the detail section."
/*%
** Explanation:
**	The user has attempted to place a field in a position which
**	would cause it to extend beyond the detail section.
**
** System Status:
**	The Move submenu is resumed.
**
** User Action:
**	Try another location to place the field.
*/

E_VF00BF_Cant_move_the_table
"Cannot place the table field here\n
Placing the table field at the cursor position would cause
it to extend beyond the right form border. You must 
'Move/Expand' the right form border first to place the table
field here."
/*%
** Explanation:
**	The user has attempted to place a table field in a position which
**	would cause it to extend beyond the form's right border.
**
** System Status:
**	The Move submenu is resumed.
**
** User Action:
**	Try another location to place the table field, or
**	move the form border first to allow placement
**	of the table field at this location.
*/

E_VF00C0_Cant_move_the_trim
"Cannot place the trim here\n
Placing the trim at the cursor position would cause
it to extend beyond the right form border. You must 
'Move/Expand' the right form border first to place the trim here."
/*%
** Explanation:
**	The user has attempted to place a trim in a position which
**	would cause it to extend beyond the form's right border.
**
** System Status:
**	The Move submenu is resumed.
**
** User Action:
**	Try another location to place the trim, or
**	move the form border first to allow placement
**	of the trim at this location.
*/

E_VF00C1_Cant_move_the_title
"Cannot place the title here\n
Placing the title at the cursor position would cause
it to extend beyond the right form border. You must 
'Move/Expand' the right form border first to place the title here."
/*%
** Explanation:
**	The user has attempted to place a title in a position which
**	would cause it to extend beyond the form's right border.
**
** System Status:
**	The Move submenu is resumed.
**
** User Action:
**	Try another location to place the title, or
**	move the form border first to allow placement
**	of the title at this location.
*/

E_VF00C2_Cant_move_the_fmt
"Cannot place the data entry window here\n
Placing the data entry window at the cursor position would cause
it to extend beyond the right form border. You must 
'Move/Expand' the right form border first to place the data entry window here."
/*%
** Explanation:
**	The user has attempted to place a data entry window in a position which
**	would cause it to extend beyond the form's right border.
**
** System Status:
**	The Move submenu is resumed.
**
** User Action:
**	Try another location to place the data entry window, or
**	move the form border first to allow placement
**	of the data entry window at this location.
*/

E_VF00C3_Cant_box_this_field
"Cannot box this field\n
Boxing this field  would cause it to extend beyond the right form border. 
You must either 'Move/Expand' the right form border, or move
this field before it can be boxed."
/*%
** Explanation:
**	The user has attempted to box a field, but boxing it
**	would cause it to extend beyond the form's right border.
**
** System Status:
**	The operation is completed, but the box attribute is 
**	taken off the field.
**
** User Action:
**	Try moving the field to another location, or
**	move the form border first to allow the box field
**	attribute.
*/

E_VF012e_Cant_move_the_box
"Cannot place the box here\n
Placing the box at the cursor position would cause
it to extend beyond the right form border. You must 
'Move/Expand' the right form border first to place the box here."
/*%
** Explanation:
**	The user has attempted to place a box in a position which
**	would cause it to extend beyond the form's right border.
**
** System Status:
**	The Move submenu is resumed.
**
** User Action:
**	Try another location to place the box, or
**	move the form border first to allow placement
**	of the box at this location.
*/


S_VF00C4_Move_Form_Boundaries	"Move (Form Boundaries) Command"

/*
**	sequence.c
*/
E_VF00C5_Cursor_not_on_a_compo
"Cursor not on a component\n
To change the field processing order of a particular field first
place the cursor on a field which allows data entry."
/*%
** Explanation:
**	The user has attempted to change the entry order.. but
**	was not on any component.
**
** System Status:
**	Continue with cursor movement to select a field 
**	for order number editing.
**
** User Action:
**	Position on a data entry field before selecting to 
**	edit the order number.
*/

E_VF00C6_Change_option_not_val
"Edit is not valid for display only fields\n
The Edit option allows you to change the order of field processing 
when the form is run. Display only fields are not entered into,
and therefore changing their field processing order is not applicable."
/*
** Explanation:
**	The user has attempted to change the entry order of 
**	a display only field.
**
** System Status:
**	User is allowed to choose another field.
**	Choose to change the entry order of another field,
**	or end the Field reordering.
**
** User Action:
**	Position on a data entry field before selecting to 
**	edit the order number.
*/

E_VF00C7_Change_option_not_val
"Edit is not valid for display only table fields\n
The Edit option allows you to change the order of field processing
when the form is run. Display only table fields are not entered into,
and therefore changing their field processing order is not applicable."
/*
** Explanation:
**	The user has attempted to change the entry order of 
**	a display only table field.
**
** System Status:
**	User is allowed to choose another field.
**
** User Action:
**	Choose to change the entry order of another field,
**	or end the Field reordering.
*/

E_VF00C8_Change_option_not_val
"Edit is not valid for trim or box/lines\n
The Edit option allows you to change the order of field processing 
when the form is run. Box/lines and Trim are not entered into,
and therefore changing their field processing order is not applicable."

/*%
** Explanation:
**	The user has attempted to change the entry order of 
**	a box feature or trim.
**
** System Status:
**	User is allowed to choose another field.
**
** User Action:
**	Choose to change the entry order of another field,
**	or end the Field reordering.
*/

E_VF00C9_No_sequence_number_en	
"No sequence number entered\n
To change the entry order of this field you must
enter a sequence number. Numbers '1' through
'%1d' are valid field processing order numbers for this form." 
/*%
** Explanation:
**	The user has entered spaces for a sequence number. 
**
** System Status:
**	Continue editing sequence number.
**
** User Action:
**	Enter a valid sequence number.
*/

E_VF00CA_not_a_valid_num	
"'%0c' is not a valid sequence number\n
You must enter a number indicating the order in which this field
is to be processed when the form is run. Numbers '1' through
'%1d' are valid field processing order numbers for this form." 
/*%
** Explanation:
**	The user has entered an invalid sequence number. 
**
** System Status:
**	Continue editing sequence number.
**
** User Action:
**	Enter a valid sequence number.
*/

E_VF00CB_A_sequence_number_mus	
"A sequence number must be greater than zero\n
You must enter a number indicating the order in which this field
is to be processed when the form is run. Numbers '1' through
'%0d' are valid field processing order numbers for this form." 
/*%
** Explanation:
**	The user has entered an invalid sequence number. 
**
** System Status:
**	Continue editing sequence number.
**
** User Action:
**	Enter a valid sequence number.
*/

E_VF00CC_is_too_large
"'%0d' is too large for a sequence number\n
You must enter a number indicating the order in which this field
is to be processed when the form is run. Numbers '1' through
'%1d' are valid field processing order numbers for this form." 
/*%
** Explanation:
**	The user has entered an invalid sequence number. 
**
** System Status:
**	Continue editing sequence number.
**
** User Action:
**	Enter a valid sequence number.
*/


F_VF002B_Change_sequence_num
"Change sequence number (press <MENU KEY> when done)"

E_VF00CD_There_are_no_fields
"Not enough fields to change the field processing order\n
There are one or less fields on the form which allow data entry, therefore
changing the field processing order is not applicable."
/*
** Explanation:
**	The user has selected to change the field processing order
**	but there are only 1 or less fields on the form which allow
**	data entry.
**
** System Status:
**	Resume edit mode.
**
** User Action:
**	Continue editing form.
*/

E_VF00CE_Non_unique_sequence
"Non-unique field order number(s) found\n
Two fields have been given the same field processing order number.
You will be positioned on one of the fields which has a duplicate
number, please make sure that all fields have unique field
processing numbers."

/*
** Explanation:
**	The user has attempted to exit the 'Order' menu option 
**	after having entered duplicate numbers for the field
** 	processing order of 2 fields.
**
** System Status:
**	The exit is not effected and the cursor is positioned
**	on the first field which has a duplicate field order number.
**
** User Action:
**	Edit the sequence numbers so that all fields are unique.
*/

S_VF00CF_Field_Order_Menu	"Field Order Menu"

/*
**	tfcreate.qc
*/

E_VF011E_ColScrollFmt	"Column format is not compatible with scrollability.\n
The table field column `%0c' defined in row `%1d' has been set to be
scrollable, but has a display format `%2c' that either specifies that the value
be displayed justified or in more than one row on the screen.  This is not
compatible with being scrollable.  Please change the format or edit the
attributes of the column so that it is not scrollable."

/*
**  Explanation:
**	A scrollable table field column had a format that specified that it be
**	displayed either justified or in more than one row on the screen.  This
**	is not allowed for scrollable columns (or fields.)
**
**	Note that a field or column can be made scrollable only if the format
**	specifies no more than one row and does not specify justification, and
**	the display length (based on the format) is less than the data length.
**	Additionally, the datatype of the field or column must be a string type
**	either `varchar,' `char,' `text' or `c' and the attributes Display Only
**	and No Echo MUST NOT be set.
**
**  System Status:
**	VIFRED will not let user create table field until he has either
**	changed the format and edit the attributes of the table field
**	to remove the scrollability flag.
**
**  User Action:
**	User needs to clear the error message and then either fix the
**	format and remove scrollability by editing the attributes of
**	the column.
*/

E_VF011F_ColScrollSize "Column display length not compatible with scrollability.\n
The table field column `%0c' defined in row `%1d' has been set to be
scrollable, but has a display length as specified in the format `%2c' that is
more than the scrollable data length.  Please change the format, or edit the
attributes and change the scrollable data length or change the column so that
it is not scrollable."

/*
**  Explanation:
**	A scrollable table field column had a format that specified a display
**	length that was longer than the scrollable data length.  Since it only
**	makes sense to scroll a column (or field) if the data length is greater
**	than the display length, this will not be allowed.
**
**	Note that a field or column can be made scrollable only if the format
**	specifies no more than one row and does not specify justification, and
**	the display length (based on the format) is less than the data length.
**	Additionally, the datatype of the field or column must be a string type
**	either `varchar,' `char,' `text' or `c' and the attributes Display Only
**	and No Echo MUST NOT be set.
**
**  System Status:
**	VIFRED will not let user create table field until he has either
**	changed the format and edit the attributes of the table field
**	to remove the scrollability flag.
**
**  User Action:
**	User needs to clear the error message and then either fix the
**	format and remove scrollability by editing the attributes of
**	the column.
*/

E_VF0120_scroll_badattr	"Scrollability conflicts with other attribute.\n
The No Echo attribute can not be set for a scrollable field or column.
Either unset scrollability or the No Echo attribute."

/*
**  Explanation:
**	A field or column was specified with the No Echo attribute
**	and was also set to be scrollable.  This attribute is not compatible
**	with being scrollable and can not be allowed.  Note that a field or
**	column can be made scrollable only if the format specifies no more than
**	one row and does not specify justification, and the display length
**	(based on the format) is less than the data length.  Additionally, the
**	datatype of the field or column must be either `varchar,' `char,' `text'
**	or `c' and the attribute No Echo MUST NOT be set.
**
**  System Status:
**	VIFRED will not allow user to exit the attribute form or to
**	edit other attributes until the condition is resolved.
**
**  User Action:
**	User needs to either unset noecho or scrollability.
*/

S_VF00D0_NoForm	"Cannot access form \"tfcreate\"."

F_VF002C_NEW	"NEW%d"

/*%
** Explanation:
**
** System Status:
**
** User Action:
*/

E_VF00D1_BadColDefault	
"Could not get default format for a column."

S_VF00D2_Table	"Table: "

S_VF00D3_Working	"Working . . ."

E_VF00D4_NoTable	
"Table '%0c' does not exist.\n
You must enter the name of an existing table.  Just press
[RETURN] if you wish to cancel this operation."
/*%
** Explanation:
**	The user has entered the name of a table which doesn't exist.
**
** System Status:
**	Resume prompting for a name.
**
** User Action:
**	Enter the name of an existing table.
*/

E_VF00D5_InvalidColFmt	
"Column '%0c' (Row %1d):  '%2c' is not a legal format string.\n
You must enter a valid display format string for the column.  The major display
format types are:  c=character; i=integer; f,e,g,n=floating point.  For
example 'i3' would define an integer field which is 3 characters wide
(select Help and then SubTopics to see more display format examples)."
/*%
** Explanation:
**	User has specified a bad display format string for a column of a table
**	field.
**
** System Status:
**	The bad string is discarded.
**
** User Action:
**	Reenter the format string.
*/

E_VF00D6_TooManyCols
"Too many columns to be added to table field.\n
The selected table definition would have added more columns than are supported
to a table field.  No more columns will be added.  (Table fields can have no
more than %0d columns in them.)"
/*
** Explanation:
**	During table field definition there were found to be
**	too many columns in the table field.
**	
** System Status:
**
** User Action:
*/

E_VF00D7_BadFmtType
"Column '%0c' (in row %1d):  Incompatible format and datatype.\n
The display format and the datatype specified for the column '%0c' are not
compatible.  For example a 'c' (character) display format would be incompatible
with an integer datatype."

/*
** Explanation:
**	The user has entered an incompatible combination for 
**	display format and data type.
**
** System Status:
**	Back to table field definition.
**
** User Action:
**	Must correct incompatibility before ending table field
**	definition.
*/

E_VF00D8_DupColName	"Column name \"%0c\" is duplicated.\n
The internal name \"%0c\" has been used by two different columns of the table
field.  The internal names for columns of a table field must be unique."
/*%
** Explanation:
**	A table field has two (or more) columns with the same internal name.
**
** System Status:
**	Resume table field definition.
**
** User Action:
**	The duplication must be corrected before 
*	successful completion of table field definition.
*/

E_VF00D9_NoColsSpecified	"Cannot create a table field with no columns.\n
Table field columns can be created by entering their title name, and display
format into the table above, or by choosing the 'GetTableDef' menu option to
extract column definitions from a database table."

/*%
** Explanation:
**	Attempt to end table field definition, but no columns have been defined.
**
** System Status:
**	Resume to table field definition.
**
** User Action:
**	Lack of columns must be corrected before 
*	successful completion of table field definition.
*/

/*??? this shouldn't happen?? */
E_VF00DA_BadFmtConvert
"Could not convert format to datatype."

/*%
** Explanation:
**
** System Status:
**
** User Action:
*/

E_VF00DB_TooBigTblFld	
"The table field will not fit.\n
The table field you were defining will not fit at the desired location
because it would extend beyond the form border. The position of the
table field is being adjusted so that it will fit on the form."
/*%
** Explanation:
**	The user has defined a table field that will not fit
**	at it's present location because it would extend beyond
**	the form border.
**
** System Status:
**	This warning is given and the origin of the table field
**	is moved so that it will just fit with it's right edge
**	right next to the right margin.
**
** User Action:
*/

E_VF00DC_nxtfattrErr	"Internal error.\n
The function 'nxtfattr()' called with bad type argument."

/*%
** Explanation:
**	Internal error, this should never happen.
**
** System Status:
**	The next/previous attribute command during attribute
**	edit will not take place.
**
** User Action:
**	Continue attribute editing.
*/

F_VF002D_Display_Top	"Display Top?(y/n):"

F_VF002E_special_bottom	"Display special bottom?(y/n):"

F_VF002F_Invis	"Invis?:"

E_VF00E0_TblFldMenu	"The \"%0c\" option is not applicable.\n
The \"%0c\" option %1c the column
information table field.  You are not currently editing the column information
so the \"%0c\" option is not applicable."

/*
** Note:  The following will each be inserted into
**	  the preceding message as the %1 parameter.
*/
S_VF00E1_InsertAction	"inserts a new column definition into"
S_VF00E2_DeleteAction	"deletes a column definition from"
S_VF00E3_EditAction	"allows editing of the attributes of a column
definition from"
S_VF00E4_MoveAction	"moves a column definition within"

/*%
** Explanation:
**	The specified menu item was selected, the column information was not
**	being edited.  (That is, you were not positioned on the table field
**	containing the column information.)
**
** System Status:
**	The selected action will not be performed; table field definition form
**	is resumed.
**
** User Action:
**	Continue editing fields of the table field definition frame.
*/

F_VF0030_EditAttr	"EditAttr"

E_VF00E5_TooManyCols	"Too many columns specified for table field.\n
The selected table definition would have added more columns than are supported
to a table field.  No more columns will be added.  (Table fields can have no
more than %0d columns in them.)"

S_VF00E7_EmptyColMove	"Cannot move an empty column."	/* should not happen */

S_VF00E9_BadMoveCol	"Destination must be on a column."

S_VF00EA_TblFldMoveCol	"Table Field Move Column Submenu"

S_VF00EB_EndRequired
"Must use End command to commit attribute changes"

S_VF00EC_NoCols	"No columns in table field."

S_VF00ED_CreateEditTblFld	"Creating/Editing Table Fields"

S_VF00EE_ChangingTblFld	"Changing Table Field . . ."

S_VF00EF_CreatingTblFld	"Creating Table Field . . ."

F_VF0031_TblFld	"table field"

S_VF00F0_TooWideTblFld	"The table field is too wide."

/*
**	undo.c
*/

/*
**	valerr.qc
*/
S_VF00F8_Can_not_access_form__	"Cannot access form \"valerr\"."

S_VF00F9_Correcting_Validation	"Correcting Validation Strings"

/*
**	vfchknam.c
*/
S_VF00FA_Names_must_begin_with	"Names must begin with an alphabetic."

S_VF00FB_The_name_contains	"The name '%0c' contains a disallowed character."

/*
**	vfsymtbl.c
*/
F_VF0032_column	"column"

E_VF00FC_Warning_Duplicate
"Warning - Duplicate %0c internal name \"%1c\" found."
/*%
** Explanation:
**
** System Status:
**
** User Action:
**
*/

E_VF00FD_Inconsistent_symbol
"Inconsistent symbol table for internal name \"%0c\"."
/*%
** Explanation:
**
** System Status:
**
** User Action:
**
*/

S_VF00FE_buildst	"buildst: bad object type found"	/* DIAG */

/*
**	vifred.qc
*/
S_VF00FF_Form	"  Form : "

S_VF0100_Retrieving_forms	"Retrieving form '%0c' . . ."

S_VF0101_editcom_blank_frame	"blankcom: blank frame returns NULL\n" /* DIAG*/

E_VF0009_Can_t_retrieve_QBF_Jo  "Can't retrieve QBF JoinDef '%0c' from catalog"

E_VF0102_Relation_does_not
"Table '%0c' does not exist.\n  VIFRED could not find table '%0c' in
database '%1c'.  Please check the spelling and be sure you are using
the correct database."
/*
** Explanation:
**	User has specified the name of a table that does not exist
**	in the database for the user.  Either user has mis-spelled
**	the name or is using the wrong database.
**
** System Status:
**	VIFRED will ignore the request and wait for more input.
**
** User Action:
**	Re-issue request with a new table name.
*/

E_VF0103_Table_name_checking
"Table name checking consistency error.\n
A VIFRED internal error has occurred while checking the names 
of table(s) to use in building a default form.
Request will be canceled.  Please try again."

/*%
** Explanation:
**	Error occurred while trying to break up list of table names.
**
** System Status:
**	VIFRED will ignore the request and wait for more input.
**
** User Action:
**	Re-issue request.
*/

S_VF0104_Create_Submenu	"Create Submenu"

F_VF0033_Heading	"Heading"

F_VF0048_Box	"Box/Line"

S_VF0105_Create_Component_Subm	"Create Component Submenu"

/*
**	widefrm.c
*/

/*
**	writefrm.qc
*/
E_VF0106_QBFNAME_exists	
"QBFName `%0c' already exists\n
The QBFName `%0c' is already associated with another form.
You will be asked if you still wish save the form. If you answer
yes, then the form will be saved and the QBFName will be associated
with this form. If you answer no the form will not be saved and the 
QBFName will still be associated with the old form."
/*%
** Explanation:
**	QBFName exists already, but this form would like to 
**	overwrite it.
**
** System Status:
**	The user will be asked if he still wishes to save the form
**	knowing this.
**
** User Action:
**	Answer yes or no to the question which follows. #VF0107
*/


S_VF0107_Do_you_still_want_to	"Do you still want to save? "

S_VF0108_Default_QBFName_entry	"Default QBFName entry for form `%s'"

E_VF0109_Error_occurred_while	
"Error occurred while creating QBFName object\n
A database error occurred while creating a QBFName object. The form
will not be saved."
/*%
** Explanation:
**	Error occurred while trying to save the QBFName for the
**	form being saved.
**
** System Status:
**	Neither the form nor the qbfname will not be saved.
**
** User Action:
**	Try again later.
*/


/*
**	getargs.c
*/
S_VF010C_Usage	"Correct syntax: vifred dbname [-p] [-uuser] [-e] [[-t|-j|-f] name]\n"

/*
**	tfcreate.qc
*/
F_VF0034_Display_Title	"Display Title?(y/n):"

/*
**	menudef.c
*/
F_VF0035_Order	"Order"

F_VF001f_DisplayStyle "FormAttr"

F_VF0036_Title	"Title"

F_VF0037_DispFmt	"DisplayFormat"

F_VF0038_Attr	"Attributes"

F_VF0039_Left	"Left"

F_VF003A_Center	"Center"

F_VF003B_Right	"Right"

F_VF003C_Shift	"Shift"

F_VF003D_Format	"Format"

F_VF003E_Data	"DisplayFormat"

F_VF003F_ColOpt	"ColumnOptions"


F_VF0040_RepOpt	"ReportOptions"

F_VF0041_Expand	"Expand"

F_VF0042_DefOrd	"DefaultOrder"

/*
**	getfrmnam.qc
*/
E_VF010D_Error_occurred_while
"Error occurred while creating object for form."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_VF010E_Could_not_create_obje
"Could not create object for form."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

/*
**	enter.c
*/

/* opinion: like 56, 10F should not be an error.. but rather the
   operation the user is attempting should be allowed. */

E_VF010F_A_display_format_has	
"A display format has already been entered\n
To change the display format you have entered you must complete creation
of the field, and then invoke the Edit option on this field"
/*%
** Explanation:
**	The user has attempted to edit the display format twice during
**	field creation.
**
** System Status:
**	Resume field creation submenu.
**
** User Action:
**	Continue with field definition.
*/

/*
**	vfcat.qc
*/
S_VF0110_Destroy_error
"An error was encountered Destroying %0c `%1c'."

E_VF0111_COMPILE_FAILED
"Could not compile form \"%0c\" into file \"%1c\".\n
Either the form does not exists or VIFRED was unable to
create the output file for the compilation process."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

/*
**	vfinittbl.qc
*/
E_VF0112_ING_ERR
"Error `%0d' encountered while getting table information.\n
While VIFRED was trying to build a default form from a
database table definition for table \'%1c\", it received the
above error number."

E_VF0113_NO_TABLE
"Table \"%0c\" does not exist or is not owned by you or the DBA.\n
VIFRED could not find a database definition for the above table
to build a default form."


/*
**  qbfmap.qc
*/
E_VF0114_destroy_err
"Error occurred while destroying %0c `%1c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

/*
** vffrmatr.qsc
*/


/* From the vffrmatr form menu (selecting this menu choice allows the user
to orient a popup form visually on the screen */
F_VF0049_VisuallyAdjust		"VisuallyAdjust"

/* the following 4 items represent the titles of fields which 
can be dynamically blanked out during form operation */ 
F_VF004a_Postitle		"Position (fixed/floating):"
F_VF004b_Bordtitle		"Border (y/n):"

/* the Rowtitle and Coltitle are used as titles on the vffrmatr form, where
the code will append a ':' on the end..  These items are also expanded
into the error message StartRCOvflo and StartRowColErr */
  
F_VF004c_Rowtitle		"StartRow"
F_VF004d_Coltitle		"StartColumn"

/* Title string for frmatr form.. this string, along with the name
of the form is centered in the "title" field of the form.  */
F_VF004e_Frmatr_title		"Form Attributes for '%s'"


/* The following 4 items represent the text strings which are the values
a user can enter into the "style" and "postype" fields on vffrmatr form
Translator NOTE:   these strings must agree with the field validations
for these fields. Note... in different languages if these strings are of
different lengths then the length of the fields must be changed on the form */
F_VF004f_fullscreen		"fullscreen"
F_VF0050_popup			"popup"
F_VF0051_fixed			"fixed"
F_VF0052_floating		"floating"

/* this is the help subject for vffrmatr.frm */
S_VF0130_Form_Attr	"Form Attributes"

/* this is the help subject for visualAdjust submenu */
S_VF0134_Vis_Adj	"Visual Adjust"


F_VF0056_WhersCurs	"Location"

F_VF0046_Resize		"Resize"

S_VF012d_CurPosStr	"Cursor location on screen is row %d, column %d."

/* This is the help subject text for the vfboxatr form's help file */
S_VF0132_Box_Attributes	"Box/Line Attributes"

E_VF0135_Back_to_fullscreen
"The form can no longer be a Popup\n
The action just taken has caused the form to be expanded. The
expansion makes it impossible for this form to still have the
Popup attribute because it would exceed the screen boundaries.
The form is being given the Fullscreen attribute."

/*
** attr.qc
*/

F_VF0053_scroll_title
"Scrollable?(y/n):"

/*
**  Explanation:
**	String to display as title for field "scrollable" in form "vifattr".
*/

F_VF0054_scroll_sztitle
"Scroll Size:"

/*
**  Explanation:
**	String to display as title for field "scrollsz" in form "vifattr".
*/

E_VF011B_scroll_sync
"Warning!  Removing scrollability for field or column `%0c'.\n
Field or column was previously
defined to be scrollable but currently does not qualify to be scrollable.
This is probably due to a change in the format specification.  The rules
for scrollability are: 1) that the datatype for a field must be one of the
character ones - varchar, char, text or c; 2) attribute No echo must NOT
be set; 3) field format must be a
single line format and 4) there must be no format justification in the
field's format string specification."

/*
**  Explanation:
**	VIFRED has discovered a field or column that is set to be scrollable
**	no longer qualifies to be scrollable.  VIFRED will unset scrollability.
**	This is only a warning to the user.
**
**  System Status:
**	System will wait for user to clear the warning message and then
**	proceed with processing.
**
**  User Action:
**	User just clears the warning message to continue.
*/

E_VF011C_bad_scroll_val
"Bad value entered for `scrollability'.\n  Please enter a `y' or 'n'
to turn on or off scrollability, respectively.  It is an error to
specify other values."

/*
**  Explanation:
**	User enter something other than a 'y' or 'n' into field "scrollable".
**
**  System Status:
**	System will allow user to correct value and then continue.
**
**  User Action:
**	User clears out this error message and then enters either a 'y'
**	or 'n' into field "scrollable".
*/


E_VF011D_bad_scroll_len	""

/*
**  Explanation:
**	No longer used--see E_VF0121.
*/


F_VF0043_scroll_datawidth	"Scrolling Data Width:"

/*
** This is a field title on the edit attribute (vifattr) form.
** It will only be seen if VIFRED is invoked with the -q flag.
*/

F_VF0044_Col_sep	"Display Column Separator?(y/n):"

/*
**	getfrmnam.qc
*/

F_VF0045_blank	"blank"
/*
**  Default name for a form when user created a form by selecting
**  the "BlankForm" option.
*/

/*
**	vifred.qsc
*/

E_VF0118_TEMP_NO_WRITE
"Local or II_TEMPORARY directory NOT writable.\n  The local or
II_TEMPORARY directory is NOT writable.  VIFRED will thus not be
able to save forms into the database."

/*%
** Explanation:
**	Temporary directory (II_TEMPORARY or local) is not writable.
**	VIFRED thus can not create files for the COPY statement
**	when user wants to save a form.
**
** System Status:
**	VIFRED will prompt user to continue.
**
** User Action:
**	User can continue if he replies 'y' to prompt.
*/

S_VF0119_WISH_TO_CONT
"Do you wish to continue? "

F_VF0055_BlankLine	"NewLine"

/*
** Catalog Frame Explanations ("vfcat.qsc".)
*/
F_VF0057_CreateExpl	"Create a new form."
F_VF0058_BlnkFormExpl	"Start with a blank form."
F_VF0059_TblDefExpl	"Start with a default form for a database table."
F_VF005A_SimpleFldExpl	"Place simple fields in the form."
F_VF005B_TblFldExpl	"Place a single table field in the form."
F_VF005C_JoinDefExpl	"Start with a default form for a join definition."
F_VF005D_DestroyExpl	"Destroy the selected form."
F_VF005E_EditExpl	"Browse or change the selected form."
F_VF005F_RenameExpl	"Rename the selected form."
F_VF0060_UtilExpl	"Compile a form, Print a form, or Manage QBFNames."
F_VF0061_UtilRTIExpl	"Compile a form, Print a form, Manage QBFNames, or Copy out a form."
F_VF0062_CompileExpl	"Compile the selected form to include in a program."
F_VF0063_PrintExpl	"Write a printable representation of the selected form into a file."
F_VF0064_QBFNamesExpl	"Manage names by which QBF identifies an existing form."
F_VF0065_CopyFormExpl	"Copy the selected form into a file."
F_VF0066_QBFrunExpl	"Run QBF on a table using the selected form."
F_VF0067_QuitVF	"Exit VIFRED."

/* CopyForm output file name prompt */
S_VF012A_OutputFile	"File to which to copy form: "

/*
**	vffrmatr.c
*/

E_VF0125_StartRowColErr	"StartRow and StartColumn must be numbers >= 1.\n
The StartRow or StartColumn must be numeric values.  The minimum values of 1,1
correspond to the upper left corner of the screen."

/*%
** Explanation:
**	The user has either specified a value for the start
**	row or column of a popup form as 0 or negative or
**	has entered something that is not a number.
**
** System Status:
**	The field is cleared and operation resumes with the user 
**	positioned on the field in error.
**
** User Action:
**	Enter a valid numeric value greater than 1 and less than
**	a value which would cause the popup form to extend beyond
*	the screen boundary.
*/


E_VF0126_StartRCOvFlo
"The %0c specified is too large\n
The %0c specified would cause the PopUp form to extend
beyond the screen area.  You must specify a smaller value
for %0c. The upper-left corner of the screen is 1,1."

/*%
** Explanation:
**	The user has either specified a value for StartRow
**	or StartColumn which would cause the form to 
**	extend beyond the screen boundary.
**
** System Status:
**	The field is cleared and operation resumes with the user 
**	positioned on the field in error.
**
** User Action:
**	Enter a valid numeric value greater than 1 and less than
**	a value which would cause the popup form to extend beyond
*	the screen boundary.
*/


E_VF0127_BorderOvFlo
"A border on the form would cause overflow\n
A border has been requested for the PopUp form, but surrounding the
form with a border would cause it to extend beyond the screen
area."

/*%
** Explanation:
**	The user has requested a border around the form but
**	use of a border would cause the form to exceed the screen
**	boundary.
**
** System Status:
**	The border is turned off and operation resumes in
**	the next field.
**
** User Action:
**	Reposition, or resize the form so that addition of a border
**	will not overflow the screen.
*/



E_VF0128_CantMakePopup
"The form is too large to be made into a PopUp\n
The form can not be converted into a PopUp because doing so
would cause it to extend beyond the screen boundaries. You must
move or delete features, and reduce the size of the form before
attempting to turn it into a PopUp."

/*%
** Explanation:
**	The user has requested to convert a large form into a PopUp.
**	Doing so would cause the form to exceed the screen
**	boundary.
**
** System Status:
**	Execution resumes on the Style field.
**
** User Action:
**	Return to the edit screen and move features of the form
**	so that a smaller than screen sized form can be created.
*/


E_VF0129_NoVisAdjFullScr
"Cannot visually adjust a Fullscreen style form\n
Visual adjustment of forms refers to positioning and 
sizing of a PopUp form. Only PopUp style forms may
be visually adjusted."

/*%
** Explanation:
**	The user has selected the menu choice which allows visual
**	adjustment of a form, but the form's style is Fullscreen.
**	Visual adjustment is relevant only to PopUp forms.
**
** System Status:
**	Execution resumes on the Style field.
**
** User Action:
**	If desired the form may be converted to PopUp style and
**	then visually adjusted.
*/


E_VF012b_CantMvFrmThere
"The form can not be moved there\n
Moving the form to the present cursor location would cause the 
form to extend beyond the screen boundaries. PopUp forms may not
be moved so that they extend beyond the screen boundary."

/*%
** Explanation:
**	The user has requested to move a PopUp so that 
**	it would exceed the screen dimensions.
**
** System Status:
**	The placement is disallowed.. and the move sequence continues. 
**
** User Action:
**	Select a position for the move which will not exceed the 
**	screen dimensions.
*/


S_VF011A_Not_implemen
"This feature is not implemented in the current release."

/*
** Explanation:
**	Some ports of vifred do not have GO and COMPILE implemented.
**	Currently this includes CMS.
**
** System Status:
**	Vifred will prompt user to continue.
**
** User Action:
**	Press return to continue.
*/

F_VF0068_Invisible	"Invisible"
F_VF006E_Layout		"Layout"
F_VF006F_Aggregate	"Aggregate"

/*
** Explanation:
**	One of the per-field attributes that may be specified in VIFRED
**	on the 'EditAttributes' frame.
**
** System Status:
**	None.
**
** User Action:
**	None.
*/

S_VF0136_EDITING_BOX_LINE	"Editing a Box/Line"

E_VF0121_bad_scroll_len	"Invalid scroll size entered.\n
Either you have left the `scroll size' field blank
or the size is less than the format size.  The format size
is `%0d'.  Please enter a correct size."

/*
**  Explanation:
**	User has entered a bad scroll size due to one of
**	the reasons stated above.
**
**  System Status:
**	System will allow user to correct value and then continue.
**
**  User Action:
**	User needs to enter a correct scroll size after clearing out
**	this error message.
*/
E_VF0122_disp_fmt_restricted
"Display format restricted for this field.\n
Use of the new display format would require changing the field
datatype.  This field is critical to the proper operation of a Visual
Query frame that uses this form.  Changing the datatype is not allowed."

/*
**  Explanation:
**	User has specified a format for the field that would normally
**	force a change in the field's datatype.  This is not allowed
**	for a restricted field.
**
**  System Status:
**	System will allow user to re-specify the format.
**
**  User Action:
**	User needs to enter an acceptable display format.
*/

E_VF0123_field_restricted
"Cannot delete a restricted field.\n
This field is critical to the proper operation of a Visual
Query frame that uses this form.  You may not delete this field."

/*
**  Explanation:
**	User has attempted to delete a field restricted by VQ.
**	This is not allowed.
**
**  System Status:
**	Action denied.
**
**  User Action:
**	No recourse--can't delete this field.
*/

E_VF012C_Cant_set_derived_and
"Cannot set Derived with other Attributes.\n
A field cannot be made into a Derived field and still have
the Mandatory, Force Lower, Force Upper, Query Only
and Display Only attributes.  These attributes will be cleared."

/*
**  Explanation:
**	User has attempted to set certain other attributes of a
**	field along with a derivation formula.  This is not allowed
**	and the other attributes are being turned off.
**
**  System Status:
**	The other attributes are turned off.
**
**  User Action:
**	Either change the field to be non-derived, or leave it alone.
*/

/* Titles for Help files. */
S_VF0137_Field_Validations	"Field Validations"
S_VF013A_Field_Derivations	"Field Derivations"

S_VF0138_Correcting_Derivation	"Correcting Derivation Formulas"

/*
**  Explanation:
**	Titles for help files.
**
**  System Status:
**	Displayed at top of help files.
**
**  User Action:
**	None.
*/

/* Psuedo-titles on attribute (vifattr/vifvalder) forms. */
F_VF0069_Der_Form	"Derivation Formula:"
F_VF006A_Val_Chk	"Validation Check to Perform:"

F_VF006B_MoreEdit	"MoreEdit"	/* Menu item for attributes frame. */

F_VF006C_ValAllowed	"(Validation Allowed)"
F_VF006D_ValNotAllowed	"(Validation Not Allowed)"
/*
** Explanation:
**	Explanatory text placed next to the 'derived' field on the attributes
**	form.  When 'derived' contains 'n' (no), the text will display
**	(Validation Allowed).  When 'derived' is 'y' (yes, it's a derived
**	field), the text will display (Validation Not Allowed).
**
** System Status:
**	None.
**
** User Action:
**	None.
*/

F_VF0070_Val_Str	"Validation String"	/* Trim on valerr form. */
F_VF0071_Der_Form	"Derivation Formula"	/* Trim on valerr form. */

E_VF0139_Change_option_not_val
"Edit is not valid for derived fields.\n
The Edit option allows you to change the order of field processing 
when the form is run.  Derived fields are not entered into,
and therefore changing their field processing order is not applicable."
/*
** Explanation:
**	The user has attempted to change the entry order of 
**	a derived field.
**
** System Status:
**	User is allowed to choose another field.
**	Choose to change the entry order of another field,
**	or end the Field reordering.
**
** User Action:
**	Position on a data entry field before selecting to 
**	edit the order number.
*/

F_VF0072_Narrow		"narrow"	/* Narrow screen size */
F_VF0073_Wide		"wide"		/* Wide screen size */
F_VF0074_Current	"current"	/* Default screen size */

F_VF0075_DuplicateFld	"DuplicateField"	/* DuplicateField menu item */
F_VF0076_Listforms	"ListForms"		/* ListForms menu item */

S_VF013B_DupHelp	"Duplicating a Field"

F_VF0077_NewFldName	"Name for new field: "	/* Prompt to ask for new name */

S_VF013C_NoFldInForm	"Form `%s' contains no fields."

F_VF0078_FldsIn		"Fields in"

S_VF013D_Dupfromdb	"Duplicating a Field From a Form in the Database"

F_VF0079_Crfld		"Creating new field . . ."

F_VF007A_FormsIn	"Forms in"

F_VF007B_DB		"Database"

F_VF007C_RetFlds	"Retrieving list of fields from the database . . ."

F_VF007D_CurForm	"Current Form"

F_VF007E_RetForms	"Retrieving list of forms from the database . . ."

F_VF007F_NoCurFld
"No fields in current form, retrieving list of forms from the database . . ."

E_VF013E_TooManyRows
"Too many rows returned for simple field `%0c' in form `%1c'.
The database catalog appears to be inconsistent as more than
one set of field definitions was returned for the above simple
field.  Please re-edit the form containing the problem field."

E_VF013F_TFRetErr
"Could not retrieve the complete definition of the table field
`%0c' in form `%1c' from the database.  Please check that
form `%1c' is still accessible with Vifred."

E_VF0140_NoForms
"There are no forms in the database that you have access to."

S_VF0141_LsForms
"Selecting a Form in the Database to Duplicate a Field From"

E_VF0142_Format_too_big
"The specified display format is too large.\n
The maximum allowable display format is %0d characters wide.
Specify a format that is less than or equal to that maximum."
/*
** Explanation:
**	The maximum allowable display format is (currently) 4096
**	characters wide.
**
** System Status:
**	Reject the specified display format.
**
** User Action:
**	Enter a smaller display format.
*/

S_VF0143_Trim_Attributes	"Trim Attributes"
S_VF0144_Edit_Trim	"Edit Trim"
/*
** Explanation:
**	Subject strings for help on editing trim and trim attributes.
**
** System Status:
**
** User Action:
*/

S_VF0145_Duplicate	"Duplicate"
S_VF0146_Duplicate_expl	"Create a form by copying an existing form"
S_VF0147_Blank	"Blank"
S_VF0148_Blank_expl	"Create a form starting with a blank screen"
S_VF0149_Table	"Table"
S_VF014A_Table_expl	"Create a form based on a Table"
S_VF014B_JoinDef	"JoinDef"
S_VF014C_JoinDef_expl	"Create a form based on a JoinDef"
/*
** Explanation:
**	Choices for method of creation of a new form in VIFRED.
**
** System Status:
**
** User Action:
*/

S_VF014D_Form_Creation	"Form Creation"
/*
** Explanation:
**	Help file title for VIFRED form creation.
**
** System Status:
**
** User Action:
*/

S_VF014E_tablefield	"tablefield"
S_VF014F_simplefields	"simplefields"
/*%
** Explanation:
**	Styles of fields on form when creating a default form based on
**	a database table.  Either each table column will be represented
**	by a simple field, or each column will map to a table field column.
**
** System Status:
**
** User Action:
*/

E_VF0150_no_table_name
"A table name must be entered.\n
A table name needs to be entered here in order to create a default
form.  If you do not wish to create a form based upon a table, use
the 'Cancel' menu item."

E_VF0151_no_form_name
"A form name must be entered.\n
A form name needs to be entered here in order to create a new
form.  If you do not wish to create a form based upon an existing form, use
the 'Cancel' menu item."

E_VF0152_no_joindef_name
"A JoinDef name must be entered.\n
A JoinDef name needs to be entered here in order to create a new
form.  If you do not wish to create a form based upon a JoinDef, use
the 'Cancel' menu item."
/*%
** Explanation:
**	The user has entered 'OK', thus requesting creation of a form
**	without entering the name of a table (form, joindef) upon which
**	to base the new form.
**
** System Status:
**	Do nothing.
**
** User Action:
**	Either enter a name or type Cancel.
*/

S_VF0153_Form_from_Table	"Create Form from Table"
S_VF0154_Form_from_Dup_Form	"Create Form from Existing Form"
S_VF0155_Form_from_JoinDef	"Create Form from JoinDef"
/*%
** Explanation:
**	Help file title for VIFRED form creation:
**		from dbms table.
**		from existing form.
**		from joindef.
**
** System Status:
**
** User Action:
*/

S_VF0156_Existing_Forms	"List of Existing Forms"
S_VF0157_JoinDefs	"List of JoinDefs"
/*%
** Explanation:
**	Help file title for VIFRED form creation:
**		list of form names.
**		list of joindef names.
**
** System Status:
**
** User Action:
*/

F_VF0080_Create_Duplicate_Form	"Creating duplicate form from '%0c'"
/*%
** Explanation:
**	Creating a new form based upon an existing form.  User has
**	specified a form as the basis for creation of a new form.
**
** System Status:
**	Set user up editing a copy of an existing form, allowing the
**	copy to be saved with a new name.
**
** User Action:
**	Edit the form.
*/

F_VF0081_Create_from_JoinDef	"Creating form from JoinDef '%0c'"
/*%
** Explanation:
**	Creating a new form based upon a joindef.  User has
**	specified a joindef as the basis for creation of a new form.
**
** System Status:
**	Set user up editing a form bases upon a joindef.
**
** User Action:
**	Edit the form.
*/

F_VF0082_Selecting_form_names	"Selecting form names . . ."
F_VF0083_Form_Name	"Form Name"
F_VF0084_Selecting_JDef_names	"Selecting JoinDef names . . ."
F_VF0085_JoinDef_Name	"JoinDef Name"
F_VF0086_forms	"forms"
F_VF0087_JoinDefs	"JoinDefs"

F_VF0089_JOINDEF	"Joindef:"
F_VF0090_TABLE		"Table:"
F_VF0091_FORM		"Form:"

/*%
** Explanation:
**	Messages to display while querying the database for a list
**	of accessible forms/joindefs.  Title to use for the form
**	displaying the list of forms/joindefs.  Object 'type' names
**	(plural) to use in an error message if none exist.
**
** System Status:
**
** User Action:
*/


F_VF0092_ExplListForms "Retrieve list of forms from the database."
/*%
**  Explanation:
**	Displayed in Help Keys for the ListForms menu item in the
**	create duplicate field code.
**
**  System Status:
**
**  User Action:
*/

F_VF0093_BrkOpt	"BreakOptions"

/*      Title for "Enter Table Name:" Help Screen */

F_VF0094_Help_Table_Prompt	"Enter a Table Name"
/*%
**  Explanation:
**	Title for the "Enter Table Name:" help screen.
**
**  System Status:
**
**  User Action:
*/

F_VF0095_ExplOKTable "Use this table in the table field definition."
/*%
**  Explanation:
**	Displayed in Help Keys for the OK menu item in the GetTableDef
**	code for creating a table field.
**
**  System Status:
**
**  User Action:
*/

S_VF0158_Destroy_Form	"Confirm Request to Destroy Form"
S_VF0159_Destroy_QBFName	"Confirm Request to Destroy QBFName"
S_VF015A_Save_Form_End	"Save Form Before Ending?"
S_VF015B_Save_Report_End	"Save Report Before Ending?"
S_VF015C_Save_Form_Quit	"Save Form Before Quitting?"
S_VF015D_Save_Report_Quit	"Save Report Before Quitting?"
/*%
** Explanation:
**	Help file subject titles for confirmation popups.
**
** System Status:
**
** User Action:
*/

S_VF015E_Missing_role_name	"Missing role name.\n"
S_VF015F_Role	"Role"
/*%
** Explanation:
**	If the -R (role) flag is specified with neither a role nor a
**	password, then prompt for the role.  If prompting is turned off,
**	then state that the role name is missing (and then exit.)
**
** System Status:
**	Obtain role name, or exit.
**
** User Action:
**	Enter role name.
*/

E_VF0160_Cursor_must_be_in_ftr	
"Cursor must be in a footer section of layout to create an aggregate"
/*%
** Explanation:
**	The user is trying to create an aggregate but the cursor
**	is not in a footer section.  An aggregates can only occur
**	in a footer section (report/page/break).
**
** System Status:
**	RBF returns back to the layout frame after the error msg 
**	is displayed.  The cursor position remains unchanged.
**
** User Action:
**	Move the cursor to a footer section and then create
**	the aggregate.  The user can create a footer section
**	if one does not exist by using the 'Layout' menu option.
*/

E_VF0161_Cant_move_the_agg 
"Cannot place the aggregate here.\n
Placing the aggregate at the cursor position would cause
it to move out of a footer section.  Aggregates can only
occur in the footer sections of a report."
/*%
** Explanation:
**	The user is trying to move an aggregate into a non-footer
**	section.  An aggregates can only occur in a footer section 
**	(report/page/break).
**
** System Status:
**	RBF returns back to the layout frame after the error msg 
**	is displayed.  The cursor position remains unchanged.
**
** User Action:
**	Move the aggregate into a footer section only.
**	The user can create an additional footer section 
**	by using the 'Layout' menu option.
*/

E_VF0162_Cant_move_column
"Cannot move column here.\n
Placing the column at the cursor position would cause one
of the components of the column (field or heading) to 
overwrite one of the special RBF Section Lines."
/*%
** Explanation:
**	The user is trying to move a column (field/aggregate) but
**	this would cause it to overwrite one of the special section
**	lines.
**
** System Status:
**	RBF returns back to the layout frame after the error msg 
**	is displayed.  The cursor position remains unchanged.
**
** User Action:
**	Move the column elsewhere or add lines.
*/

E_VF0163_Cant_delete_the_last	
"Cannot delete the section.\nDeleting this section would result
in an empty report.  You must have at least one detail component 
in a report.  Please move a field into another section."
/*%
** Explanation:
**	The user has attempted to delete last detail component
**	on a report spec.
**
** System Status:
**	Resume cursor mode edit.
**
** User Action:
**	None
*/

E_VF0164_bad_scroll_len	"Invalid scroll size entered.\n
Either you have left the 'scroll size' field blank,
the size is less than the format size or the size is too large.
The format size is '%0d' with a maximum scroll size of '%1d'.
Please enter a correct size."
/*%
**  Explanation:
**	User has entered a bad scroll size due to one of
**	the reasons stated above.
**
**  System Status:
**	System will allow user to correct value and then continue.
**
**  User Action:
**	User needs to enter a correct scroll size after clearing out
**	this error message.
*/
E_VF0165_no_table_name
"A table name must be entered.\n
A table name needs to be entered before selecting the '%0c' menu item.
If you wish to leave this screen without entering a table name,
select the '%1c' menu item."

E_VF0006_no_joindef_name
"A JoinDef name must be entered.\n
A JoinDef name needs to be entered before selecting the '%0c' menu item.
If you wish to leave this screen without entering a JoinDef name,
select the '%1c' menu item."

F_VF0096_cr_trim
"Create trim at the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF0097_cr_field
"Create a field, consisting of a title and a display format, at the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF0098_cr_tbl_field
"Create a table field at the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/


F_VF0099_cr_line
"Create a blank line at the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/


F_VF009A_cr_box
"Create a box or line at the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/


F_VF009B_dup_field
"Duplicate an existing field or table at the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/


F_VF009C_end
"Return to the main catalog frame."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/


F_VF009D_column
"Create a column at the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/


F_VF009E_agg
"Create an aggregate at the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF009F_heading
"Create a column heading at the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00A0_rbf_place
"Move the highlighted report component to the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00A1_rbf_left
"Move the highlighted report component to the left margin."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00A2_rbf_center
"Center the highlighted report component on the current line."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/


F_VF00A3_rbf_right
"Move the highlighted report component to the right margin."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00A4_rbf_shift
"Move component to current position, moving other components if necessary."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00A5_rbf_end
"Return to the main edit layout menu line."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00A6_rbf_display_f
"Edit the column's display format."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00A7_rbf_colopts
"Go to the Column Options frame."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00A8_rbf_brkopts
"Go to the Break Options popup."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00A9_rbf_create
"Create a new report component at the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00AA_rbf_delete
"Delete the report component at the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00AB_rbf_edit
"Modify the report component at the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00AC_rbf_move
"Move the report component at the current cursor position to another position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/


F_VF00AD_rbf_layout
"Go to the Report Layout popup to edit the different report sections."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00AE_rbf_ropts
"Go to the Report Options frame."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00AF_rbf_undo
"Undo the last editing command."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00B0_rbf_save
"Save the current report to the database."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00B1_rbf_quit
"Exit RBF."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/


F_VF00B2_rbf_end2
"Return to the main report catalog frame."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00B3_rbf_column
"Move the column's heading and display format together as a unit."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00B4_margin_place
"Move the margin to the current cursor position."
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00B5_margin_expand
"Move the margin out." 
/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

E_VF0166_no_obj_for_name
"Can only select the NAME command when on a component.\n
You must first position the cursor on a component before
selecting the NAME command."

/*%
**  Explanation:
**	The cursor was not on any component when the NAME
**	command was selected.
**
**  System Status:
**	The request will be ignored.
**
**  User Action:
**	Place cursor on a component before selecting the NAME command.
*/

E_VF0167_not_in_detail
"Cursor not in detail section.\n
The NAME command is only valid for the detail section of a report.
You must first position the cursor on a component in the detail
section before selecting the NAME command."

/*%
**  Explanation:
**	The cursor was not in the detail section of the report.
**	The NAME command is only valid for components in this section.
**
**  System Status:
**	Command will be ignored.
**
**  User Action:
**	Move the cursor to a component in the detail section
**	before selecting the NAME command.
*/

E_VF0168_not_a_field
"Selected component is not a field.\n
The cursor is not on a field component.  The NAME command is only
valid for field components in a report."

/*%
**  Explanation:
**	User did not place cursor on a field component.  The NAME
**	command is only valid for field components.
**
**  System Status:
**	Command will be ignored.
**
**  User Action:
**	Move the cursor to a field component
**	before selecting the NAME command.
*/

S_VF0169_name_is
"Base-Table Column Name: '%s'"

/*%
**  Explanation:
**	Message telling the user what the base table column name
**	for a field in rbf is.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00B6_Name
"Name"

/*%
**  Explanation:
**	The "Name" menuitem in the layout frame for RBF.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00B7_exp_name
"Determine base table column name for a field."

/*%
**  Explanation:
**	Help, keys explanation.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

E_VF016A_cant_del_x_hair
"Can not delete a straight edge.\n
Straight edges may not be deleted with the DELETE command.  Use the 'Rulers'
menu command to modify the display of straight edges."

/*%
**  Explanation:
**	User tried to delete a straight edge.  This is not allowed.
**
**  System Status:
**	Command will be ignored.
**
**  User Action:
**	Continue editing the form/report.
*/

E_VF016B_cant_edit_x_hair
"Can not edit a straight edge.\n
Corss hairs are not editable components.  If you wish to move them, use
the MOVE command.  To turn off the display of straight edges, use the
'Rulers' menu command."

/*%
**  Explanation:
**	User tried to edit a straight edge.  This is not allowed.
**
**  System Status:
**	Command will be ignored.
**
**  User Action:
**	Continue editing the form/report.
*/

F_VF00B8_Align
"Rulers"

/*%
**  Explanation:
**	The Align menuitem for the vifred/rbf layout menus.  Allows
**	users to set the display of the alignment options.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00B9_exp_align
"Control display of ruler options."

/*%
**  Explanation:
**	Help/Keys explanation for the Rulers menuitem.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00BA_group_move
"GroupMove"

/*%
**  Explanation:
**	The GroupMove menuitem for the vifred layout menu.  Allows
**	users to move a group of components together.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

F_VF00BB_exp_group_move
"Move a group of components together."

/*%
**  Explanation:
**	Help/Keys explanation for the GroupMove menuitem.
**
**  System Status:
**	Not applicable.
**
**  User Action:
**	Not applicable.
*/

E_VF016C_NO_COMPS
"No components found for GroupMove.\n  Vifred could not find any
components that were completely within the specified boundaries.
The GroupMove will be cancelled."

/*%
**  Explanation:
**	The user specified a boundary that did not completely enclose
**	any component on the form.
**
**  System Status:
**	Processing continues.
**
**  User Action:
**	Select GroupMove again.
*/

E_VF016D_GM_COLLISION
"Selected destination will cause components to overlap.\n
The new anchor point for the GroupMove command will cause the selected
components to overlap existing components.  Please select another anchor
point or select 'End' to cancel the GroupMove command."

/*%
**  Explanation:
**	The user selected a new anchor point that will result in overlaps
**	between the selected and existing components.
**
**  System Status:
**	Vifred will wait for user to select another anchor point or
**	to cancel with 'End'.
**
**  User Action:
**	Select another anchor point or 'End' to cancel the command.
*/

S_VF016E_GROUPMOVE	"Group Move Command"

S_VF016F_RULER_CTRL	"Control Frame for Rulers and StraightEdges"

F_VF00BC_placeexpl	"Place move destination at cursor position."

E_VF0170_BAD_OWNER_NAME
"Invalid identifier for owner name entered.\n
'%0c' is not valid as an
owner name.  Please check for invalid characters as well as the correct
use of quotes for entering delimited identifiers."

/*%
**  Explanation:
**	The user entered an identifier that is not valid as an
**	owner name.
**
**  System Status:
**	The data will be rejected.
**
**  User Action:
**	User must re-enter a valid identifier.
*/

E_VF0171_BAD_TBL_NAME
"Invalid identifier for table name entered.\n
'%0c' is not valid as a
table name.  Please check for invalid characters as well as the correct
use of quotes for entering delimited identifiers."

/*%
**  Explanation:
**	The user entered an identifier that is not valid as an
**	table name.
**
**  System Status:
**	The data will be rejected.
**
**  User Action:
**	User must re-enter a valid identifier.
*/

F_VF00BD_ENTER_TNAME	"Enter Table Name:"

/*%
**  Explanation:
**	The title string for prompting for a table name for
**	- the GetTableDef menuitem in table field creation
**	- creating a new qbfname linked to a table.
**
**  System Status:
**	n/a
**
**  User Action:
**	n/a
*/

F_VF00BE_ENTER_JNAME	"Enter JoinDef Name:"

/*%
**  Explanation:
**	The title string for prompting for a joindef name for
**	- creating a new qbfname linked to a joindef.
**
**  System Status:
**	n/a
**
**  User Action:
**	n/a
*/

E_VF0172_BAD_DTYPE_FOUND
"Unsupported datatype encountered.\n
Column(s) defined on an unsupported datatype were encountered.  These
column(s) have been ignored.  The currently supported datatypes in the forms
system are: integer, float, varchar, char, text, c, decimal, date and
money."

/*%
**  Explanation:
**	The user referenced a table that included columns with unsupported
**	datatypes (e.g., long varchar).
**
**  System Status:
**	Vifred will ignore columns with unsupported datatypes.
**
**  User Action:
**	None.
*/

S_VF0173_MOVING_STR_EDGE	"Moving a Straight Edge"

S_VF0174_TBL_PROMPT_QN		"Entering a Table Name for a QbfName"

S_VF0175_JDF_PROMPT_QN		"Entering a JoinDef Name for a QbfName"

E_VF0176_BAD_JDEF_NAME
"Invalid identifier for a JoinDef Name entered.\n
'%0c' is an invalid identifier for a JoinDef Name.  Please check
for invalid characters."

/*%
**  Explanation:
**	The user entered an invalid name for a joindef name.  Probably
**	some illegal characters in the input.
**
**  System Status:
**	Vifred will ask for input again.
**
**  User Action:
**	Fix the entered data.
*/

F_VF00BF_Input_mask	"Input Masking"
/*%
**  Explanation:
**	Input mask name for attribute table field in attributes frame.
**
**  System Status:
**	n/a
**
**  User Action:
**	n/a
*/

E_VF0177_no_jd_name
"A JoinDef name must be entered.\n
A JoinDef name needs to be entered here in order to create a new
form.  You may also edit/create a JoinDef by entering in the name 
and selecting 'Edit'.  If you do not wish to create a form based 
upon a JoinDef, use the 'Cancel' menu item."
/*%
** Explanation:
**	The user has entered 'OK', thus requesting creation of a form
**	without entering the name of a joindef upon which
**	to base the new form.
**
** System Status:
**	Do nothing.
**
**  User Action:
**	Enter a JoinDef name and select either 'Edit' (to edit/create)
**	or 'OK' (to create a form based on the joindef.  The user
**	can select 'Cancel' to cancel the operation.
*/

F_VF00C0_EditExpl       "Edit/create a JoinDef on which to create the form."

/*%
** Explanation:
**      Help explanation for the Edit menuitem.
**
** System Status:
**      Not applicable.
**
** User Action:
**      None.
*/

E_VF0178_ALL_COLS_UNSUP
"No column based on supported datatypes found.\n
All the column(s) for the specified table(s) are defined
on unsupported datatypes.  No default form will be created.
Please enter table name(s) that have at least one column
that is defined on supported datatypes.  The currently supported
datatypes in the forms system are: integer, float, varchar, char,
text, c, decimal, date and money."
/*%
**  Explanation:
**	All the columns were defined on unsupported datatypes.
**
**  System Status:
**	Vifred will not create a default form and wait for user
**	to enter new table name(s).
**
**  User Action:
**	Enter new table name(s) that include supported datatypes.
*/
