/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    erpf.msg -	PrintForm Message Definitions File.
**
** Description:
**	Contains the message definitions for the PrintForm program.
**
** History:
**	Revision 6.0  10/09/87 (dave)
**	Created.
**	05/28/88 (dkh) - Added new ids to support printing Venus attributes.
**	06/22/88 (dkh) - Improved some messages.
**	27-jun-88 (bruceb)
**		Re-arranged text in S_PF0001, E_PF0002, S_PF0004 messages
**		for extra clarity against DG's CEO.
**	02-jan-89 (bruceb)
**		Added invisible.
**	15-aug-89 (bruceb)
**		Changed to account for derived fields/columns.
**	12-dec-89 (bruceb)	Fix for bug 8495.
**		Added 'query only' message text.
**	06/18/93 (dkh) - Fixed bug 51409.  The input mask attribute will
**			 now be listed by printfrom.
**/

# define	_PF_CLASS	87
/*
**	dumpform.qc
*/
S_PF0001_Writing_form	"Writing form '%0c' to '%1c' file . . ."
E_PF0002_Cant_open_file	"Can't open '%0c' file for writing."
F_PF0001_Form_name	"\n\n\n\n\nForm name: %s\n"
F_PF0002_Form_owner	"Form owner: %s\n"
F_PF0003_Number_of_columns	"Number of columns on screen: %d\n"
F_PF0004_Number_of_lines	"Number of lines on screen: %d\n"
F_PF0005_Number_of_fields	"Number of fields: %d\n"
F_PF0006_Number_of_trim_string	"Number of trim strings: %d\n"
F_PF0007_Date_first_created	"Date first created: %s\n"
F_PF0008_Date_last_modified	"Date last modified: %s\n\n\n"
S_PF0003_Writing_out_detailed	"Appending detailed field information . . ."
F_PF0009_FIELD_DESCRIPTIONS	"FIELD DESCRIPTIONS\n\n\n"
F_PF000A_Field_name	"Field name: %s\n"
F_PF000B_Type_of_field_table	"Type of field: table field\n"
F_PF000C_Number_of_rows_in_tab	"Number of rows in table field: %d\n"
F_PF000D_No_row_separators	" No row separators,"
F_PF000E_Highlighting_current	" Highlighting current row enabled,"
F_PF000F_Special_table_field	"Special table field attributes:"
F_PF0010_None	"None\n"
F_PF0011_Field_title	"Field title: %s\n"
F_PF0012_Nullable	"Nullable "
F_PF0013_Non_nullable	"Non-nullable "
F_PF0014_Data_type	"Data type: %s %s\n"
F_PF0015_Display_format	"Display format: %s\n"
F_PF0016_Display_only	" Display only,"
F_PF0017_Box_field	" Box field,"
F_PF0018_Keep_previous_value	" Keep previous value,"
F_PF0019_Mandatory_field	" Mandatory field,"
F_PF001A_Reverse_video	" Reverse video,"
F_PF001B_Blinking	" Blinking,"
F_PF001C_Underline	" Underline,"
F_PF001D_Brightness_change	" Brightness change,"
F_PF001E_Force_lower_case	" Force lower case,"
F_PF001F_Force_upper_case	" Force upper case,"
F_PF0020_No_auto_tab	" No auto tab,"
F_PF0021_No_echo	" No echo,"
F_PF0022_Right_to_left_input	" Right to left input,"
F_PF0023_Special_display_attri	"Special display attributes:"
F_PF0024_None	" None\n"
F_PF0025_Type_of_field	"Type of field: "
F_PF0026_regular	"regular\n"
F_PF0027_column_in_table_field	"column in table field\n"
F_PF0028_Default_value	"Default value: %s\n"
F_PF0029_Validation_check	"Validation check: %.60s\n"
F_PF002A_Validation_check	"Validation check: %.60s\n          %.60s\n"
F_PF002B_Validation_check	"Validation check: %.60s\n          %.60s\n          %.60s\n"
F_PF002C_Validation_error_mess	"Validation error message: %.50s\n\n\n"
S_PF0004_Form_written	"Form '%0c' written to '%1c' file."
/*
**	printform.qc
*/
E_PF0005_Could_not_open_databa	"Could not open database \"%0c\"\n"

/*
**	dumpform.qsc
*/
F_PF002D_full	"Form Display Style is: FullScreen.\n"
F_PF002E_fixed
"Form Display Style is: Fixed position popup at row `%d' and column `%d'.\n"
F_PF002F_float
"Form Display style is: Floating popup.\n"
F_PF0030_notitle	" don't display column titles,"
F_PF0031_scrollable	" scrollable,"
F_PF0032_text_trim
"Text trim at row `%d' and column `%d' is:\n\t`%s'.\n"
F_PF0033_box
"Box trim at row `%d' and column `%d' is `%d' rows high and `%d' columns wide.\n"
F_PF0034_boxed	"Form has borders enabled.\n"
F_PF0035_nobox	"Form has borders disabled.\n"
F_PF0036_color	"Foreground display color number is: `%d'\n"
F_PF0037_trimdesc	"\n\n\nTRIM DESCRIPTIONS\n\n\n"
F_PF0038_Column_name	"Column name: %s\n"
F_PF0039_Column_title	"Column title: %s\n"
F_PF003A_TblField_name	"Table field name: %s\n"

/* Use F_PF003B */
F_PF003C_Invisible	" Invisible,"

F_PF003B_Derived	" Derived,"
F_PF003D_Derivation_formula	"Derivation formula: %.60s\n\n\n"
F_PF003E_Derivation_formula	"Derivation formula: %.60s\n          %.60s\n\n\n"
F_PF003F_Derivation_formula	"Derivation formula: %.60s\n          %.60s\n          %.60s\n\n\n"
F_PF0040_Derivation_formula	"Derivation formula: %.60s\n          %.60s\n          %.60s\n          %.60s\n\n\n"
F_PF0041_Validation_check	"Validation check: %.60s\n          %.60s\n          %.60s\n          %.60s\n"
F_PF0042_Validation_error_mess	"Validation error message: %.50s\n          %.50s\n\n\n"

/*
**	dumpform.qc
*/
F_PF0043_Query_only	" Query only,"

F_PF0044_EMASK	"Input Masking,"
/*%
**  Explanation:
**	Text of the input masking attribute to be outputted by
**	printform.
**
**  System Status:
**	N/A.
**
**  User Action:
**	None.
*/
