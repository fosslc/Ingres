/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	erfe.msg -	Front-End Common Message File.
**
** Description:
**	Front End Common Message File.
**
**	These messages are commonly used throughout the frontends,
**	and the constants defined should be used whenever possible
**	in order to ease the work of the translators.
**
** History:
**	Revision 6.0  87/07/26  peter
**	Initial revision.
**	10-aug-92 (blaise)
**		Added FE_EditSource (for editing local procedure source in
**		Vision)
**	24-aug-92 (blaise)
**		Added FE_LocalVars and FE_LocalProcs (for editing local
**		procedures and local variables in Vision)
**	7-dec-92 (blaise)
**		Added F_FE0077_Import (for importing a frame in Vision)
**	20-Aug-97 (rajus01)
**		Added F_FE0119_Login, F_FE011A_Attributes for extending vnode
**		database to specify security mechanisms. 
**      30-Mar-2010 (hanal04) Bug 123491
**              Added F_FE0031_AddLoc for use in CBF log file configuration.
**/
 
#define	_FE_CLASS	161
 
/*
**	The following are the common menu items used in the
**	INGRES frontends, which should be used for consistency.
*/

F_FE0001_All			"All"
#define FE_All			F_FE0001_All

F_FE0002_Append			"Append"
#define FE_Append		F_FE0002_Append

F_FE0003_Application		"Application"
#define FE_Application		F_FE0003_Application

F_FE0004_Archive		"Archive"
#define FE_Archive		F_FE0004_Archive

F_FE0005_Blank			"Blank"
#define FE_Blank		F_FE0005_Blank

F_FE0008_Bottom			"Bottom"
#define FE_Bottom		F_FE0008_Bottom

F_FE0009_Call			"Call"
#define FE_Call			F_FE0009_Call

F_FE000A_Catalog		"Catalog"
#define FE_Catalog		F_FE000A_Catalog

F_FE000C_Column			"Column"
#define FE_Column		F_FE000C_Column

F_FE0010_Compile		"Compile"
#define FE_Compile		F_FE0010_Compile

F_FE0012_Create			"Create"
#define FE_Create		F_FE0012_Create

F_FE0014_Database		"Database"
#define FE_Database		F_FE0014_Database

F_FE0018_Define			"Define"
#define FE_Define		F_FE0018_Define

F_FE0019_Delete			"Delete"
#define FE_Delete		F_FE0019_Delete

F_FE001A_Destroy		"Destroy"
#define FE_Destroy		F_FE001A_Destroy

F_FE001B_DeleteLine		"DeleteLine"
#define FE_DelLine		F_FE001B_DeleteLine

F_FE001C_Edit			"Edit"
#define FE_Edit			F_FE001C_Edit

F_FE001E_End			"End"
#define FE_End			F_FE001E_End

F_FE001F_Examine		"Examine"
#define FE_Examine		F_FE001F_Examine

F_FE0022_Field			"Field"
#define FE_Field		F_FE0022_Field

F_FE0023_File			"File"
#define FE_File			F_FE0023_File

F_FE0024_Find			"Find"
#define FE_Find			F_FE0024_Find

F_FE0025_Forget			"Cancel"
#define FE_Forget		F_FE0025_Forget

F_FE0026_Frame			"Frame"
#define FE_Frame		F_FE0026_Frame

F_FE0027_GetTableDef		"GetTableDef"
#define FE_GetTableDef		F_FE0027_GetTableDef

F_FE0028_Go			"Go"
#define FE_Go			F_FE0028_Go

F_FE0029_Graph			"Graph"
#define FE_Graph		F_FE0029_Graph

F_FE002B_Help			"Help"
#define FE_Help			F_FE002B_Help

F_FE002D_Image			"Image"
#define FE_Image		F_FE002D_Image

F_FE002E_Insert			"Insert"
#define FE_Insert		F_FE002E_Insert

F_FE002F_JoinDef		"JoinDef"
#define FE_JoinDef		F_FE002F_JoinDef

F_FE0030_InsertLine		"InsertLine"
#define FE_InsLine		F_FE0030_InsertLine

F_FE0031_AddLoc			"AddLocation"
#define FE_AddLoc		F_FE0031_AddLoc

F_FE0034_Line			"Line"
#define FE_Line			F_FE0034_Line

F_FE0037_LongRemark		"LongRemark"
#define FE_LongRemark		F_FE0037_LongRemark

F_FE0038_MoreInfo		"MoreInfo"
#define FE_MoreInfo		F_FE0038_MoreInfo

F_FE0039_Move			"Move"
#define FE_Move			F_FE0039_Move

F_FE003B_Next			"Next"
#define FE_Next			F_FE003B_Next

F_FE003C_NextPage		"NextPage"
#define FE_NextPage		F_FE003C_NextPage

F_FE003D_Place			"Place"
#define FE_Place		F_FE003D_Place

F_FE003E_Plot			"Plot"
#define FE_Plot			F_FE003E_Plot

F_FE003F_Previous		"Previous"
#define FE_Previous		F_FE003F_Previous

F_FE0040_PrevPage		"PrevPage"
#define FE_PrevPage		F_FE0040_PrevPage

F_FE0042_Print			"Print"
#define FE_Print		F_FE0042_Print

F_FE0043_Procedure		"Procedure"
#define FE_Procedure		F_FE0043_Procedure

F_FE0044_QBFNames		"QBFNames"
#define FE_QBFNames		F_FE0044_QBFNames

F_FE0045_Query			"Query"
#define FE_Query		F_FE0045_Query

F_FE0046_Quit			"Quit"
#define FE_Quit			F_FE0046_Quit

F_FE0047_Read			"ReadFile"
#define FE_ReadFile		F_FE0047_Read

F_FE0048_Rename			"Rename"
#define FE_Rename		F_FE0048_Rename

F_FE0049_Report			"Report"
#define FE_Report		F_FE0049_Report

F_FE004A_Resume			"Resume"
#define FE_Resume		F_FE004A_Resume

F_FE004B_Retrieve		"Retrieve"
#define FE_Retrieve		F_FE004B_Retrieve

F_FE004E_Save			"Save"
#define FE_Save			F_FE004E_Save

F_FE0053_Table			"Table"
#define FE_Table		F_FE0053_Table

F_FE0055_TableField		"TableField"
#define FE_TblField		F_FE0055_TableField

F_FE0057_Top			"Top"
#define FE_Top			F_FE0057_Top

F_FE0058_WriteFile		"WriteFile"
#define FE_WriteFile		F_FE0058_WriteFile

F_FE0059_Trim			"Trim"
#define FE_Trim			F_FE0059_Trim

F_FE005A_Update			"Update"
#define FE_Update		F_FE005A_Update

F_FE005D_Utilities		"Utilities"
#define FE_Utilities		F_FE005D_Utilities

/*
** out of alphabetical order, but I didn't feel like resequencing
*/
F_FE005E_Undo			"Undo"
#define FE_Undo			F_FE005E_Undo

F_FE005F_EditInfo		"EditInfo"
#define FE_EditInfo		F_FE005F_EditInfo

F_FE0060_Write			"Write"
#define FE_Write		F_FE0060_Write

F_FE0061_Select			"Select"
#define FE_Select		F_FE0061_Select

/* The following are four system-dependent names for the same menu item */
F_FE0062_Shell			"Shell"
#define FE_Shell		F_FE0062_Shell

F_FE0063_Spawn			"Spawn"
#define FE_Spawn		F_FE0063_Spawn

F_FE0064_CMSSubset		 "CMS/Subset"
#define FE_CMSSubset		F_FE0064_CMSSubset

F_FE0065_CLI		 	 "CLI"
#define FE_CLI			F_FE0065_CLI

/* (tom) some additional standard menuitems */

F_FE0066_OK		 	"OK"
#define FE_OK			F_FE0066_OK

#define FE_Cancel		F_FE0025_Forget

F_FE0067_Details		"Details"
#define FE_Details		F_FE0067_Details

F_FE0068_Lookup		 	"ListChoices"
#define FE_Lookup		F_FE0068_Lookup

F_FE0069_Cut		 	"Remove"
#define FE_Cut			F_FE0069_Cut

F_FE006A_Paste		 	"Insert"
#define FE_Paste		F_FE006A_Paste

F_FE006B_Test		 	"Test"
#define FE_Test			F_FE006B_Test

F_FE006D_LineEdit		"LineEdit"
#define FE_LineEdit		F_FE006D_LineEdit

F_FE006E_JoinLines		"JoinLines"
#define FE_JoinLines		F_FE006E_JoinLines

F_FE006F_SplitLine		"SplitLine"
#define FE_SplitLine		F_FE006F_SplitLine

/*
** and a few more... (blaise)
*/
F_FE0074_EditSource		"EditSource"
#define FE_EditSource		F_FE0074_EditSource

F_FE0075_LocalVars		"LocalVars"
#define FE_LocalVars		F_FE0075_LocalVars

F_FE0076_LocalProcs		"LocalProcs"
#define FE_LocalProcs		F_FE0076_LocalProcs

F_FE0077_Import			"Import"
#define FE_Import		F_FE0077_Import

/*
**	The following are commonly used terms in the frontends
**	that should be used consistently.
*/

F_FE0070_application		"application"
#define FE_application		F_FE0070_application

F_FE0071_form			"form"
#define FE_form			F_FE0071_form

F_FE0072_report			"report"
#define FE_report		F_FE0072_report

F_FE0073_graph			"graph"
#define FE_graph		F_FE0073_graph

/* Common Messages */

F_FE0080_PressEnter		"[PRESS RETURN]"
#define	FE_HitEnter		F_FE0080_PressEnter

/* Explanations */

F_FE0100_ExplFrameHelp
"Display help for this frame, key definitions, or current field."
#define	Explfrmhelp	ERget(F_FE0100_ExplFrameHelp)

F_FE0101_ExplKeyHelp	"Display help for this command or key definitions."
#define Explsubhelp	ERget(F_FE0101_ExplKeyHelp)

F_FE0102_ExplEnd	"Return to the previous frame."
#define Explend		ERget(F_FE0102_ExplEnd)

F_FE0103_ExplCancel	"Cancel this command."
#define Explcancel	ERget(F_FE0104_ExplCancel)

F_FE0104_ExplFind	"Locate a specified string in the table field."
#define Explfind	ERget(F_FE0104_ExplFind)

F_FE0105_ExplTop	"Scroll to the top of the table field."
#define Expltop		ERget(F_FE0105_ExplTop)

F_FE0106_ExplBottom	"Scroll to the bottom of the table field."
#define Explbottom	ERget(F_FE0106_ExplBottom)

F_FE0107_ExplMoreInfo	"Display more information on this object."

F_FE0108_ExplSelect	"Select the value displayed in the current row."

F_FE0109_ExplForget	"Cancel the operation and return to the previous frame."

F_FE010A_ExplShell	"Escape to the operating system."

/*
**	The simplest message
*/
F_FE010B_Empty		""
#define	FE_Empty F_FE010B_Empty

/* More explanations */
F_FE010C_FileExpl		"Read or Write a file."

F_FE010D_ReadExpl       	"Read text from a file."

F_FE010E_WriteExpl      	"Write the currently displayed text to a file."

F_FE010F_LineEditExpl		"Insert, Delete, Split, or Join lines."

F_FE0110_InsertExpl     	"Insert a blank line."

F_FE0111_DeleteExpl     	"Delete the current line."

F_FE0112_JoinExpl		"Join two lines together."

F_FE0113_SplitExpl		"Split a line into two lines."

F_FE0114_BlankExpl      	"Delete the text."

F_FE0115_EditExpl       	"Edit text with the system editor."

F_FE0116_LookupExpl "Display a list of choices for the current field."
#define FE_LookupExpl		F_FE0116_LookupExpl

F_FE0117_SaveExpl		"Save changes to the database."
#define FE_SaveExpl		F_FE0117_SaveExpl

F_FE0118_LngRemExpl		"Edit/view the long remark for this object."

F_FE0119_Login		 	"Login"
#define FE_Login		F_FE0119_Login

F_FE011A_Attributes	 	"Attributes"
#define FE_Attributes		F_FE011A_Attributes
