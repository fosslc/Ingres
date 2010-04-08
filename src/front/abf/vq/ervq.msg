/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ervq.msg -	Message file for class VQ
**
** Description:
**	This file defines the messages for class VQ.
**
** History:
**	 3-aug-1987 (bobm)
**		Created.
**	23-oct-1990 (mgw)
**		Corrected some misspellings.
**	24-jul-92 (blaise)
**		Added F_VQ013F_Before_Lookup, F_VQ0140_After_Lookup,
**		F_VQ0141_On_Timeout, F_VQ0142_On_Dbevent,
**		F_VQ0143_Table_Field_Menuitems, F_VQ0144_Menuline_Menuitems;
**		all new escape code types.
**	6-aug-92 (blaise)
**		Added F_VQ0145 to F_VQ0158; E_VQ00E4_No_Proc_Name; all local
**		procedure messages.
**	31-aug-92 (blaise)
**		Added S_VQ00D7_No_Procs_in_Table, E_VQ00E5_Update_Source_Warning
**		E_VQ00E6_Source_Exists; all local procedure messages.
**	5-nov-92 (blaise)
**		Added messages for optimistic locking support: F_VQ0159 to
**		F_VQ0162 and E_VQ00E7 to E_VQ00EE.
**	14-dec-92 (blaise)
**		Added messages for import menuitem: F_VQ0163 to F_VQ0167,
**		and E_VQ00EF
**	7-jan-93 (blaise)
**		Added E_VQ00F0_Unsupported_Type - warning that long bit &
**		long varchar are unsupported.
**	15-jan-93 (blaise)
**		Added S_VQ00F1 to S_VQ00F3 and F_VQ0168 - headings for local
**		procedure help files.
**      23-feb-93 (connie) Bug #43375
**		Added S_VQ00D7_Max_Local_Var to stop user from creating more
**              local variables per frame
**	20-apr-93 (blaise)
**		Added E_VQ00F4_Bad_Return_Type, F_VQ0169_none (messages
**		relating to local procedure return types)
**	22-apr-93 (blaise)
**		Added E_VQ00F5_Bad_Data_Type.
**	24-may-93 (blaise)
**		Renamed S_VQ00D7_No_Procs_in_Table to S_VQ00D8_... due to
**		error number clash with 64 codeline.
**	27-may-93 (blaise)
**		Added messages for duplicating a Vision form: F_VQ0170 to 173;
**		E_VQ00F6, S_VQ00F7.
**	13-aug-93 (connie) Bug #41602
**		Corrected text in warning message F_VQ0122
**		-- "Application Frame Diagram" ==> "Application Flow Diagram"
**	15-sep-93 (connie)
**		Added E_VQ00F8_No_Valid_Column.
**	16-feb-94 (blaise)
**		Added S_VQ00F9_Changed_Locktype, S_VQ00FA_Unset_Optimistic:
**		messages used by the reconciler when dealing with optimistc
**		locking behaviour.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**              Ingres message file cleanup.
**/

# define	_VQ_CLASS	169



/*=====================  FAST MESSAGES ====================================*/



/*------
FRAME TYPES.. These are loaded into an in memory structure
upon first access to the frame flow diagram. These
are displayed as the type of frame within the <>'s in
the frame flow diagram, and are also used to display
a pick list of the types of frames during modification
of a menu frame to another type.
-------*/

F_VQ0001_menu		"Menu"
F_VQ0002_append 	"Append"
F_VQ0003_update 	"Update"
F_VQ0004_browse 	"Browse"
F_VQ0005_user 		"User"
F_VQ0006_qbf 		"QBF"
F_VQ0007_report 	"Report"
F_VQ0008_graph 		"Graph"
F_VQ0009_oslproc 	"4gl Proc"
F_VQ000A_hlproc 	"3gl Proc"
F_VQ000B_dbproc 	"DB Proc"

F_VQ000C_menu_desc	"Frame to display choices"
F_VQ000D_append_desc	"Frame to insert new data"
F_VQ000E_update_desc	"Frame to update data"
F_VQ000F_browse_desc	"Frame to display data"
F_VQ0010_user_desc	"Frame of 4GL commands "
F_VQ0011_qbf_desc	"Frame to run QBF"
F_VQ0012_report_desc	"Frame to run a report"
F_VQ0013_graph_desc	"Frame to display a graph"
F_VQ0014_oslproc_desc	"Procedure of 4GL commands"
F_VQ0015_hlproc_desc	"Procedure of 3GL commands"
F_VQ0016_dbproc_desc	"Procedure run in the server"


/*------
TABLE TYPES.. In the visual query the type of the table is displayed
next to the depiction of the table, this is that text.
-------*/

F_VQ0017_app_tab	"<Append>"
F_VQ0018_upd_tab	"<Update>"
F_VQ0019_dsp_tab 	"<Browse>"
F_VQ001A_lup_tab 	"<Lookup>"

F_VQ001B_table_text	"Table"		/* the word table itself
					   which is displayed with the
					   above words. */

/*------
FRAME INDICATORS.. in the application flow diagram these are used
to indicate the frame status.
-------*/

F_VQ001C_chg_ind	"Changed"
F_VQ001D_err_ind	"Error"
F_VQ001E_new_ind	"New"
F_VQ001F_cust_ind	"Custom"

/*------
Frame flow diagram expansion characters (only the first character
is significant). This are used in the display of the frame flow
diagram to indicates points where the diagram can be expanded.
-------*/

F_VQ0020_expr_char 	">"
F_VQ0021_expl_char 	"<"
F_VQ0022_expd_char 	"V"

/*------
When the frame flow diagram scrolls off the top of the screen this
printf type message displays the name of the frame which is the parent of the
current top of the screen.
-------*/

F_VQ0023_from_text	"..from: " 

/*------
VQ specific menuitems.
-------*/

F_VQ0025_JoinCol	"SetJoinColumn"
F_VQ0027_Compress	"ZoomOut"
F_VQ0028_AddTable	"AddTable"
F_VQ0029_DelTable	"DelTable"
F_VQ002A_AddJoin	"AddJoin"
F_VQ002B_DelJoin	"DelJoin"
F_VQ002C_TableDef	"TableDef"
F_VQ002D_NextTable	"NextTable"
F_VQ002E_Remove		"Remove"
F_VQ002F_Duplicate	"Duplicate"

F_VQ0037_FixError	"FixError"
F_VQ0039_ListingFile	"ListingFile"
F_VQ003A_MarkFixed	"MarkFixed"
F_VQ003B_MarkUnfixed	"MarkUnfixed"

/* title for the application report */

F_VQ013C_AppRepTitle	"%0c - Display Application Report"


/*
**	Titles for vqqparts form
*/
F_VQ003C_rests_text	"Query Restrictions"
F_VQ003D_assgn_text	"Query Assignment"
F_VQ003E_assgns_text	"Query Assignments"
F_VQ003F_defaults_text	"Default Values"
F_VQ0040_detault_text	"Default Value"

F_VQ0038_ChangesOK	"Accept the changes"

F_VQ0041_CreateExpl	"Create a brand new frame"
F_VQ0042_DestroyExpl	"Destroy the current frame"
F_VQ0043_EditExpl	"Edit definitions of the current frame"
F_VQ0044_RemoveExpl	"Remove the current frame"
F_VQ0045_InsertExpl	"Insert an existing frame"
F_VQ0046_CompileExpl	"Compile the current frame"
F_VQ0047_TestExpl	"Run starting at the current frame"
F_VQ0048_DetailsExpl	"Show details about the current frame"
F_VQ0049_CatalogExpl	"View the frame catalog for this application"
F_VQ004A_AFD_TopExpl	"Go to the top of the Application Flow Diagram"
F_VQ004B_AFD_FindExpl	"Find a frame by name"
F_VQ004C_AFD_HelpExpl	"Display help on the Application Flow Diagram"
F_VQ004D_AFD_EndExpl	"End Application Edit, go back to the Catalog"
F_VQ004E_AFD_QuitExpl	"Quit Application Edit, and exit the program"
F_VQ004F_AFD_UtilExpl	"Access Utility submenu"

/* explanations for the new VQ frame prompt display loop */
F_VQ0050_OKExpl		"Create the new frame, and edit its Visual Query"
F_VQ0051_CancelExpl	"Cancel the new frame creation"
F_VQ0052_DuplicateExpl	"Duplicate the Visual Query of an existing frame"
F_VQ0053_LookupExpl	"Present a table selection list"
F_VQ0054_HelpExpl	"Display help on creating a Frame"

/* explanations for the new ABF frame prompt display loop */
F_VQ0055_OKExpl		"Create the new frame"

/* explanations for the editing of a menuitem display loop */
F_VQ0056_OKExpl		"Set the new menuitem"
F_VQ0057_CancelExpl	"Cancel menuitem edit"
F_VQ0058_HelpExpl	"Display help on editing menuitem text"

/* explanations for Visual Query display loop */
F_VQ0059_AddTableExpl	"Add a Lookup table to the Visual Query"
F_VQ005A_DelTableExpl	"Delete a Lookup table from the Visual Query"
F_VQ005B_EditExpl	"Edit definitions for the current frame"
F_VQ005C_AddJoinExpl	"Add a join to the Visual Query"
F_VQ005D_JoinColExpl	"Set the current column as one of the join columns"
F_VQ005E_CancelExpl	"Cancel the AddJoin operation"
F_VQ005F_HelpExpl	"Display help about adding a join to the Visual Query"
F_VQ0060_DelJoinExpl	"Delete a join from the Visual Query"
F_VQ0061_NextTableExpl	"Move to the next table of the Visual Query"
F_VQ0062_TableDefExpl	"Display the table definition for the current table"
F_VQ0063_UtilitiesExpl	"Access to INGRES Utilities"
F_VQ0064_CompressExpl	"Display a compressed view of the Visual Query"
F_VQ0065_HelpExpl	"Display help for the Visual Query"
F_VQ0066_CancelExpl	"Cancel the Visual Query, no save of changes"
F_VQ0067_EndExpl	"Save changes (if any) and exit the Visual Query"
F_VQ0068_QuitExpl	"Quit Visual Query, and exit the program"
F_VQ0069_IngMenu	"INGRES/Menu"
F_VQ006A_IngMenuExpl	"Access INGRES/Menu Utility"
F_VQ006B_EndUtilExpl	"End Utility Submenu, back to Visual Query"
F_VQ006C_HelpUtilExpl	"Display Help on the INGRES Utilities"

/* explanations for Visual Query (compressed view) display loop */
F_VQ006D_HelpExpl	"Display help about the Visual Query compressed view"
F_VQ006E_EndExpl    	"End compressed view, go back to the Visual Query"

/* explanations for prompt for a new lookup table name */
F_VQ006F_OKExpl		"Add the named Table to the Visual Query"
F_VQ0070_CancelExpl	"Cancel the Lookup table insertion"
F_VQ0071_LookupExpl	"Display a selection list of possible tables"
F_VQ0072_HelpExpl	"Display help on entering the Lookup table"

/* explanations for compile error */
F_VQ0074_CompileOneExpl	"Compile the current frame"
F_VQ0077_FixErrorExpl	"Fix the current error"
F_VQ0079_ListingFileExpl	"Examine listing file"
F_VQ007A_MarkFixedExpl	"Mark error fixed"
F_VQ007B_MarkNotFixedExpl	"Mark error unfixed"

/* Summary line for compilation error display */
F_VQ0024_ErrSumm	"%0c '%1c' : %2d Error(s) %3d Warning(s)"
F_VQ0026_Summ		"%0c '%1c'"
F_VQ0030_Frame		"Frame"
F_VQ0031_Procedure	"Procedure"

/* application report help titles */
F_VQ007C_AppRepHelpExpl	"Display help on the Application Report"
F_VQ007D_AppRepEndExpl	"End the Application Report display"
F_VQ007E_AppRepTitle	"Application Report"


/* End from visual query simple display function */
F_VQ0087_EndExpl	"Save changes (if any) and exit the Visual Query"

/* help titles for confirmation popups */
F_VQ0088_DelfrTitle	"Destroying a frame"
F_VQ0089_RemfrTitle	"Removing a frame"
F_VQ008A_DelltTitle	"Deleting a Lookup Table"
F_VQ008B_DeljoTitle	"Deleting a Join"
F_VQ008C_QuitvqTitle	"Quitting the Visual Query edit"


/* short error text to describe the errors which occur in the
   visual query edit.. these are returned to the error processing code,
   these should be no longer than 80 characters */
F_VQ008D_No_Err	 "No errors in the Visual Query"
F_VQ008E_No_Mcol "Visual Query: No master table columns marked for display"
F_VQ008F_No_Dcol "Visual Query: No detail table columns marked for display"
F_VQ0090_No_Join "Visual Query: A table with no joins"
F_VQ0091_No_Mdjo "Visual Query: No join between the master and detail tables"
F_VQ0092_Bad_Joi "Visual Query: No join columns displayed or having an 
assignment expression"

/*------
VQ validation error message arguments
-------*/
F_VQ0093_Master		"Master"
F_VQ0094_Detail		"Detail"

/*------
VQ specific list pick prompts and help titles.
-------*/

/*---- frame flow ----*/

F_VQ0095_insdir_prmpt	
"Frame insert direction"

F_VQ0096_inslr_opt
"Right - Insert frame to the right\n\
Left  - Insert frame to the left"

F_VQ0097_insdn_opt
"\nDown  - Insert frame below"

F_VQ0098_newvq_prmpt
"New Visual Query"

F_VQ0099_insnew_prmpt
"Frame Insertion"

F_VQ009A_paste_prmpt
"Frame Insert"

F_VQ009B_frmmod_prmpt
"Modify Frame"

F_VQ009C_frmvis_opt
"Form  - Modify the frame's form\n\
Query - Modify the frame's query"

F_VQ009D_frmtyp_opt
"Form - Modify the frame's form\n\
Type - Modify the frame's type"

F_VQ009E_newtyp_prmpt
"Frame type to be inserted"

F_VQ009F_newtyp_tit
"Frame Type Selection"


F_VQ00A0_pstdir_prmpt	
"Frame insert direction"

F_VQ00A1_pstlr_opt
"Right - Insert frame to the right\n\
Left  - Insert frame to the left"

F_VQ00A2_pstdn_opt
"\nDown  - Insert frame below"

F_VQ00A3_vqed_prmpt
"Visual Query Edit Options"

F_VQ00A4_vqglob_prmpt
"Global Components"

F_VQ00A5_vqglob_opt
"Variables\n\
Constants\n\
Record Types"

F_VQ00A6_vqloc_glob_opt
"Local Variables\n\
Global Variables\n\
Global Constants\n\
Record Types"

F_VQ00A7_vqloc_glob_prmpt
"Local/Global Components"

F_VQ00A8_vqloc_glob_menu
"Variables"

F_VQ00A9_vqloc_glob_expl
"Edit Local Variables or Global Components"


/*--------------- the following are ****ALL**** of the edit options used by 
		  both the application flow diagram, and visual query */

F_VQ00AA_eop_visqry 	"Visual Query"
F_VQ00AB_eop_genfrm 	"Data Entry Form"
F_VQ00AC_eop_mnutxt	"Menuitem Text"
F_VQ00AD_eop_frparm 	"Frame Parameters"
F_VQ00AE_eop_locvar 	"Local Variables"
F_VQ00AF_eop_globob 	"Global Components"
F_VQ00B0_eop_esccod 	"Escape Code"
F_VQ00B1_eop_srcfil 	"Source Code File"
F_VQ00B2_eop_errors 	"Errors"
F_VQ00B3_eop_frbeha 	"Frame Behavior"
F_VQ00B4_eop_qbfdef 	"QBF Definition"
F_VQ00B5_eop_rbfdef 	"Report Definition"
F_VQ00B6_eop_grfdef 	"Graph Definition"
F_VQ0151_eop_locprc	"Local Procedures"

/*---------------- */

F_VQ00B7_ffed_tit 	"Frame Edit Options"
F_VQ00B8_mnuedt_tit 	"Menuitem Edit"

F_VQ00B9_add_join	"Adding a Join"
F_VQ00BA_compre_view	"Visual Query Compressed View"
F_VQ00BB_inslup_tit	"Inserting a Lookup Table"

/*---- visual query ----*/

F_VQ00BC_vq_prmpt	"Visual Query"


/*------
Miscellaneous strings which are used on the VQ display.
-------*/
/*
** One-character representation of "variable" use of database column.
** must not confict with "Yes" or "no".
*/
F_VQ013D_variable                       "v"

/* some field titles for specific
   table types */
F_VQ00C1_ins_text 			"Insert (y/n):"
F_VQ00C2_del_text 			"Delete (y/n):"
F_VQ00C3_qua_text 			"Qualify Lookup (y/n):"


/* "Join" must not contain  double byte
   characters since it is drawn
   vertically on the screen */

F_VQ00C4_join_text			"Join"

/* the dividing line between the
   master and detail on VQ frame */
F_VQ00C5_detail_tf 			"Detail Table Field"

/* indication on VQ frame that the frame is
   "masters only in a table field" */
F_VQ0139_master_tf 			"Master Data In Table Field"

/* various strings used as labels
   for visual query tables.. some of
   these are created as fields so they
   can overlap the border of the
   table field. */
F_VQ00C6_aexp_text 			"Assignment/Default"
F_VQ00C7_asgn_text 			"Assignment"
F_VQ00C8_sort_text 			"Sort"
F_VQ00C9_rest_text 			"Query Restriction"
F_VQ00CA_luptit_text 			"Lookup List Popup"
F_VQ00CB_lord_text 			"Order in Popup"
F_VQ00CC_ltit_text 			"Column Title"

/* acceptable characters in the
   sort direction field..
   e.g. descending or ascending */
F_VQ00CD_d_text  			"d"
F_VQ00CE_a_text  			"a"

/* acceptable characters for answer to the prompt
   which allows "Quit" and "All" as responses */
F_VQ00CF_quit_text  			"q"
F_VQ00D0_all_text  			"a"


/* the next two are used
   indicate the title of the
   inclusion column text.. the first
   one is displayed on one line and the
   next is displayed on a line above like;
      Display
     /
*/
F_VQ00D1_incptr_text 			"/"
F_VQ00D2_inccol_text 			"Display on Form"


/* alphabet array so we can generate the labels for the joins in
   the visual query, this should be changed in different languages */
F_VQ00D3_alphatab			"abcdefghijklmnopqrstuvwxyz"

/* validation strings for some of the
   fields of the visual query */

F_VQ00D4_dir_val 			" in [\"a\", \"d\", \" \"]"
F_VQ00D5_dir_vmsg
"The sort direction must be either \"a\" for ascending, or \"d\" for descending."

F_VQ00D6_yn_val  			" in [\"y\", \"n\"]"
F_VQ00D7_yn_vmsg
" This field must be either \"y\" (for yes) or \"n\" (for no)."

/* Escape types */
F_VQ00D8_Form_Start                     "Form-Start"
F_VQ00D9_Form_End                       "Form-End"
F_VQ00DA_Query_Start                    "Query-Start"
F_VQ00DB_Query_Next                     "Query-Next"
F_VQ00DC_Query_New_Data			"Query-New-Data"
F_VQ00DD_Query_End                      "Query-End"
F_VQ00DE_Append_Start                   "Append-Start"
F_VQ00DF_Append_End                     "Append-End"
F_VQ00E0_Update_Start                   "Update-Start"
F_VQ00E1_Update_End                     "Update-End"
F_VQ00E2_Delete_Start                   "Delete-Start"
F_VQ00E3_Delete_End                     "Delete-End"
F_VQ00E4_Menu_Start                     "Menu-Start"
F_VQ00E5_Menu_End                       "Menu-End"
F_VQ00E6_Field_Enter                    "Before-Field-Entry"
F_VQ00E7_Field_Change                   "After-Field-Change"
F_VQ00E8_Field_Exit                     "After-Field-Exit"
F_VQ013E_User_Menuitem                  "User-Menuitem"
F_VQ013F_Before_Lookup			"Before-Lookup"
F_VQ0140_After_Lookup			"After-Lookup"
F_VQ0141_On_Timeout			"On-Timeout"
F_VQ0142_On_Dbevent			"On-Dbevent"
F_VQ0143_Table_Field_Menuitems		"Table-Field-Menuitems"
F_VQ0144_Menuline_Menuitems		"Menuline-Menuitems"
F_VQ0152_Local_Procedures		""

F_VQ00E9_Select_Esc_Type 		
"Select the Escape\n\
Type to Edit"

F_VQ00EA_Select_Esc_Menuitem 		
"Select the Menuitem\n\
to Edit Escape code for"

F_VQ00EB_Select_Esc_Fld
"Select the Field to\n\
Edit Escape code for"

F_VQ00EC_Esc_Edit_Header
"\"%0c\" Escape Code"

F_VQ00ED_Field_Esc_Edit_Header
"\"%0c\" Escape Code for Field \"%1c\""

F_VQ00EE_Menu_Esc_Edit_Header
"\"%0c\" Escape Code for Menuitem \"%1c\""

F_VQ0153_Loc_Proc_Edit_Header
"%0cLocal Procedure \"%1c\""

F_VQ00EF_Select_Paste_Frame
"Select the Frame to be inserted"

F_VQ00F0_Select_P_Frame
"Frame Insert"

F_VQ00F1_Select_Copy_Frame
"Select the frame to be duplicated"

F_VQ00F2_Select_C_Frame
"Frame Duplication"


F_VQ00F3_EndEscEdit	"Ending escape code edit with unsaved changes"

F_VQ00F4_Frame_Name
"Frame Name"

F_VQ00F5_Frame_Type
"Type"

F_VQ00F6_Frame_Desc
"Description"

F_VQ00F7_FF_ident
"Vision - Application Flow Diagram"

F_VQ00F8_FF_id2
"Editing: "

F_VQ00F9_VQ_ident
"Vision - Visual Query Editor"

F_VQ00FA_VQ_id2
"Frame: "


F_VQ00FB_Unavailable	"Unavailable"

/*------
String used to prefix Tables Utility screen titles. For Tables ListPick.
------*/

F_VQ00FD_JoinLines      "JoinLines"
F_VQ00FE_SplitLine      "SplitLine"
F_VQ00FF_Line           "Line"
F_VQ0100_File           "File"
F_VQ0101_LineExpl	"Insert, Delete, Split, or Join lines"
F_VQ0102_FileExpl	"Read or Write a file"
F_VQ0103_JoinExpl	"Join two lines together"
F_VQ0104_SplitExpl	"Split a line into two lines"


/* the word "sequenced" has special meaning in the info field of a 
   master append table of an append frame.. where it means that the
   field is to be automatically maintained with a sequenced value. */
F_VQ0105_Seq_Text	"Sequenced"


F_VQ0106_Parameters	"Parameters"
F_VQ0107_param_prmpt	"Parameter Selection"
F_VQ0108_localv_prmpt	"Editing Local Variables"
F_VQ0109_CreateLV_expl  "Create a new Local Variable"
F_VQ010A_DeleteLV_expl	"Destroy the selected Local Variable"
F_VQ010B_EditLV_expl	"Browse or edit the selected Local Variable"
F_VQ010C_RenameLV_expl	"Rename the selected Local Variable"
F_VQ010D_n_a		"N/A"
F_VQ010E_localv_create	"Creating Local Variables"
F_VQ010F_Globals	"Globals"
F_VQ0110_Globals_expl	"Edit Global Variable definitions"
F_VQ0155_LocalVars	"LocalVars"
F_VQ0156_Locals_expl	"Edit Local Variable definitions"

F_VQ0145_localp_prmpt	"Editing Local Procedures"
F_VQ0146_CreateLP_expl  "Create a new Local Procedure"
F_VQ0147_DeleteLP_expl	"Destroy the selected Local Procedure"
F_VQ0148_EditLP_expl	"Browse or edit the selected Local Procedure"
F_VQ0149_RenameLP_expl	"Rename the selected Local Procedure"
F_VQ014A_localp_create	"Creating Local Procedures"
F_VQ0154_EditSource_expl"Edit the source code for the selected Local Procedure"
F_VQ0157_LocalProcs	"LocalProcs"
F_VQ0158_Procs_expl	"Edit Local Procedure definitions and source code"

/*
**	It's important that constants come last in F_VQ0111_frame_var_opt,
**	so that the choices in F_VQ0033_frame_var_nc_opt appear in the same
**	rows of the Listpick Table Field.
*/
F_VQ0111_frame_var_opt
"Form Fields\n\
Local Variables\n\
Global Variables\n\
Global Constants"

F_VQ0033_frame_var_nc_opt
"Form Fields\n\
Local Variables\n\
Global Variables"

F_VQ0112_Select_Parameter
"Select item to use\n\
for Parameter passing"

F_VQ0113_ExplMove     "Move the current table field row"
F_VQ0114_ExplPlace    "Place the Moved row above the current row"
F_VQ0115_Move_Column_Submenu  "Move Row Submenu"

/* some strings for the frame behavior screens */

F_VQ0116_QualProc	"Qualification Processing"
F_VQ0117_NextMast	"Next Master Menuitem"
F_VQ0118_Concur		"Locks Held on Displayed Data"
F_VQ0119_Urule		"Update Integrity Rule"
F_VQ011A_Drule		"Delete Integrity Rule"

F_VQ011B_Concurrency	";No\n"
F_VQ011C_Consistency	";Yes\n"
F_VQ011D_Cascade	";Cascade\n"
F_VQ011E_Restrict	";Restrict\n"
F_VQ0138_NoRule		";DBMS\n"
F_VQ011F_Enabled	";Enabled\n"
F_VQ0120_Disabled	";Disabled\n"

F_VQ0121_CustFrEdHelp	"Custom Frame Edit Confirmation"

F_VQ0122_CustFrEdTitle	"\
WARNING: The changes you have just initiated will\n\
not show up in the application until the 4gl code\n\
for frame '%1c' is regenerated.\n\
\n\
Frame '%1c' has been customized by having\n\
its 4GL source file edited. Since '%1c' has been\n\
customized, 4gl code for it cannot be regenerated\n\
without losing those customizations.\n\
\n\
You can force a regeneration of the 4gl code for\n\
'%1c' by placing your cursor on that frame in\n\
the Application Flow Diagram and pressing 'Compile'.\n\
\n\
Do you wish to continue the '%0c' operation?\n"

F_VQ0123_CustFrEdYes "Continue with the operation"
F_VQ0124_CustFrEdNo "Cancel the operation"


F_VQ0125_AFD_Line	"[%s] -> %s: %s"

/* Error in VQ indicator */
F_VQ0126_ErrorInVq	"Error in Visual Query"

/* vqrecapp.qsc */
F_VQ0127_Reconcile_lp_prompt	"What do you want to do ?"
F_VQ0128_Reconcile_lp		"Reconcile Visual Queries\nReport discrepancies"
F_VQ0129_Reconcile_lp_banner	"Visual Query Reconciliation"

F_VQ012A_End_Rec_Check		"Write reconciliation report to a file?"
F_VQ012B_End_Rec_Yes		"Write report to a file, then End"
F_VQ012C_End_Rec_No		"End without writing report to a file"
F_VQ012D_End_Rec_Banner		"Ending reconciliation without writing report"

/* vqlocals.qsc */
F_VQ0075_CreateLocal		"%0c - Create a Local Variable"
F_VQ0076_EditLocal		"%0c - Edit a Local Variable"
F_VQ0078_EditLocalVars		"%0c - Edit Local Variables"

/* vqprocs.qsc */
F_VQ014C_CreateProc		"%0c - Create a Local Procedure"
F_VQ014D_EditProc		"%0c - Edit a Local Procedure"
F_VQ014E_EditLocalProcs		"%0c - Edit Local Procedures"

F_VQ013A_field			"field"
F_VQ013B_column			"column"
/*=====================  SLOW MESSAGES ====================================*/

S_VQ0001_Insert_down 		"Do you wish to insert a frame downward?(y/n):"

S_VQ0002_Select_Esc_Type 	"Select Escape Type"


S_VQ0005_Select_Esc_Menuitem 	"Select Escape Menuitem"

S_VQ0006_Select_Esc_Fld 	"Select Escape Field"

S_VQ0007_EscEditHelpBanner	"Edit Escape Code"

S_VQ000B_Destroy_Frm_Msg	"Destroying Frame '%0c' . . ."
S_VQ000C_Cut_Frm_Msg		"Removing Frame '%0c' . . ."

S_VQ000D_EscEditFileHelpBanner	"Edit Escape Code File"

S_VQ000E_EscEditLineHelpBanner	"Edit Escape Code Line"

S_VQ0010_Find_prmpt		"Name of frame to find: "

/*-----------------------------*/

S_VQ0011_QualProc_prmpt
"Qualification Processing:\n\
 \n\
When this frame starts up, do you want\n\
the user to be able to enter a query\n\
qualification or not?\n "

S_VQ0012_QualProc_0
"Enable; User qualification is allowed\n"

S_VQ0013_QualProc_1
"Disable; User qualification not allowed\n"

/*-----------------------------*/

S_VQ0014_NextMast_prmpt
"Next Master Menuitem:\n\
 \n\
Do you want a \"Next\" menuitem to be\n\
present on the menuline of this frame?\n\
The \"Next\" menuitem allows the user\n\
to retrieve multiple master rows.\n "

S_VQ0015_NextMast_0
"Enable; Include a \"Next\" menuitem\n"

S_VQ0016_NextMast_1
"Disable; No \"Next\" menuitem is desired\n"

/*-----------------------------*/

S_VQ0017_Concur_prmpt
"Hold Locks on Displayed Data:\n\
 \n\
Do you want this update frame to\n\
hold locks on displayed data? If you\n\
answer \"Yes\" then other users will\n\
not be able to update the data\n\
while this frame is displayed.\n"

S_VQ0018_Concur_0
"No; Do not hold locks on data\n"

S_VQ0019_Concur_1
"Yes; Hold locks on displayed data\n"

/*-----------------------------*/

S_VQ001A_Urule_prmpt
"Update Integrity Rule:\n\
 \n\
Do you want changes to the master join\n\
column(s) to be \"Cascaded\" to the detail\n\
rows, or would you like to \"Restrict\"\n\
(prohibit) updates to the join column(s) when\n\
detail rows exist?\n "

S_VQ001B_Urule_0
"Cascade the join column changes to the detail\n"

S_VQ001C_Urule_1
"Restrict updates if the join column changes\n"

S_VQ00D5_Urule_None
"DBMS will handle detail join column updates\n"

/*-----------------------------*/

S_VQ001D_Drule_prmpt
"Delete Integrity Rule:\n\
 \n\
If the master row is deleted do you wish the\n\
deletion to be \"Cascaded\" to the detail rows,\n\
or do you want to \"Restrict\" (prohibit)\n\
deletions of the master row when detail rows\n\
exist.\n "

S_VQ001E_Drule_0
"Cascade the deletion to the detail rows\n"

S_VQ001F_Drule_1
"Restrict deletions if there are detail rows\n"

S_VQ00D6_Drule_None
"DBMS will handle detail row deletions\n"

/*-----------------------------*/

S_VQ0020_EditErrors		"Would you like to edit the errors?"
S_VQ0021_ApplErrs		
"Application '%0c' : %1d Error(s) %2d Warning(s)"
S_VQ0025_OSLFixedError		"FIXED\n"

S_VQ0026_EditCompileErrs	"Edit Compilation Errors"
S_VQ0027_EditVQInfo		"Edit Visual Query Information"
S_VQ0028_DisplayListFile	"Display Listing File"


/* application report strings */
/* ..the leading spaces are desirable for formatting */

S_VQ0029_AppInfo		"---- Application Information ----"
S_VQ002A_AppName		"   Application Name: %s"
S_VQ002B_AppOwner		"  Application Owner: %s"
S_VQ002C_AppCreate		"      Creation Date: %s"
S_VQ002D_AppMod			"   Last Modify Date: %s"
S_VQ002E_AppRemark		"       Short Remark: '%s'"
S_VQ002F_AppVersion		"            Version: %d"
S_VQ0030_AppSrcDir		"   Source Directory: %s"
S_VQ0031_AppQryLang		"     Query Language: %s"
S_VQ0032_AppStart		"      Default Start: %s (%s)"
S_VQ0033_AppExe			" Default Executable: %s"
S_VQ0034_AppLinkOpt		"  Link Options File: %s"
S_VQ0035_AppBatch		"              Batch: %s"

S_VQ0036_AppFlow		"---- Application Flow Diagram ----"
S_VQ0037_AppStartup		"-- TOP --"

S_VQ0038_FrameDetails		"---- Frame Details ----"
S_VQ0039_FrmName		"         Frame Name: %s"
S_VQ003A_FrmType		"               Type: %s"
S_VQ003B_FrmOwner		"              Owner: %s"
S_VQ003C_FrmCreate		"      Creation Date: %s"
S_VQ003D_FrmMod			"   Last Modify Date: %s"
S_VQ003E_FrmRemark		"       Short Remark: '%s'"

S_VQ003F_NoFrames	"No frames have been created in this application."
S_VQ0040_SaveVQ 		"Saving changes to the Visual Query . . ."



/*=====================  ERROR MESSAGES ===================================*/


E_VQ0041_Insert_at_top
"Cannot '%0c' a frame at the top\n\
To '%0c' a frame below it is necessary to move down to the
row you want to insert the frame on and invoke the '%0c' operation
again. You will then be asked if you wish to insert to the right or
left, or possibly down. It is not possible to '%0c' to the right or
left of the topmost frame, and it already has frames below it."

/*%
** Explanation:
**	The user has just invoked the <insert frame> function
**	while positioned at the top of the frame flow diagram.
**
** System Status:
**	The insertion is aborted.
**
** User Action:
**	Move down in the frame flow diagram to do the insert.
*/

E_VQ0042_Max_mitems
"Too many menuitems\n\
The current frame already contains the maximum number of menuitems so it
is not possible to '%1c' a new frame either to the right or to the
left. The maximum number of menuitems that a frame can contain is '%0d'.
If you desire a frame can be inserted downward from the current frame."

/*%
** Explanation:
**	The user has attempted to insert a frame, but the parent
**	frame already has a maximum number of menuitems.. it
**	is possible to insert downward however.
**
** System Status:
**	Since the insertion can take place downward, we will prompt
**	if that is what the user wants to do.. if so we do it
**	if not we abort the insertion.
**
** User Action:
**	Either choose to insert down or not.
*/

E_VQ0043_Max_mitems
"Too many menuitems, can't '%2c'\n\
The current frame already contains the maximum number of menuitems so it
is not possible to '%2c' a new frame either to the right or to the
left. The maximum number of menuitems that a frame can contain is '%0d'.
In addition a frame cannot be inserted downward because that would
exceed the maximum frame depth which is '%1d'. You must choose another
place to insert a frame."

/*%
** Explanation:
**
** System Status:
**	The insertion sequence is aborted.
**
** User Action:
**	Move to another location to insert a frame.
*/

E_VQ0044_Max_mitems
"Too many menuitems, can't '%1c'\n\
The current frame already contains the maximum number of menuitems so it
is not possible to '%1c' a new frame either to the right or to the
left. The maximum number of menuitems that a frame can contain is '%0d'.
In addition a frame cannot be inserted downward because the current
frame already has other frames attached. You must choose another place
to insert a frame."

/*%
** Explanation:
**
** System Status:
**	The insertion sequence is aborted.
**
** User Action:
**	Move to another location to insert a frame.
*/


E_VQ0045_No_menuitem
"No menuitem entered\n\
You must enter the menuitem you want to be used to invoke the new frame
you are inserting. This menuitem will appear on the '%0c' frame when
it is run by the user, and will cause your new frame to be called.
If you wish to cancel the insertion invoke the '%1c' menuitem."

/*%
** Explanation:
**	The user is being presented with the popup form to fill in
**	the menuitem and short remark.. and has selected the
**	create or insert option but the menuitem field has been 
**	left blank.
**
** System Status:
**	The user is placed back on the form on the menuitem field.
**
** User Action:
**	Fill in the field, or cancel the insert.
*/

E_VQ0046_Remove_at_top
"Can't '%0c' the top frame\n\
You cannot '%0c' the top frame of the application. If you wish you may
change the starting frame of the application by accessing the 'Defaults'
option from the 'Frame Catalog'."

/*%
** Explanation:
**	The user is trying to remove the top frame of the application.
**	This is not allowed.
**
** System Status:
**	The remove operation is canceled.
**
** User Action:
**	Continue editing the frame flow diagram.
*/

E_VQ0047_No_master_table
"No master table entered\n\
You have not entered the name of a master table. The query definition of
a frame must at least have one table defined. You must enter a table name
or select the '%0c' menuitem to cancel the frame insertion. If you
would like a list of the tables in the database to select from invoke
the '%1c' menuitem."

/*%
** Explanation:
**	During frame insertion..
**	The user has invoked the create menuitem.. but has not entered
**	anything into the field for the master table.
**
** System Status:
**	The user is placed back on the field for the master table.
**
** User Action:
**	Enter the name of a master table, or cancel.
*/

E_VQ0048_No_Table
"Table '%0c' does not exist\n\
If you would like a list of tables in the current database to select
from you may invoke the '%1c' menuitem. You may cancel the frame type
modification by invoking the '%2c' menuitem."


/*%
** Explanation:
**	From various places..
**	The user has entered the name of a table which does not exist.
**
** System Status:
**	Put the user back into the appropriate field so they can reenter
**	the table name.
**
** User Action:
**	Enter the name of a valid table, or cancel the operation.
*/

E_VQ0049_Max_Joins
"Exceeding maximum number of joins\n\
Visual Querys can only contain '%0d' joins, and there already that many
joins in this Visual Query."

/*%
** Explanation:
**	The user has attempted to enter more than the maximum number of
**	joins in a visual query.
**
** System Status:
**	The join insert sequence is disallowed.
**
** User Action:
**	Continue editing the visual query.
*/

E_VQ004A_2nd_LupJoin
"Only 1 join per 'Lookup' table is allowed\n\
Lookup tables only support one of their columns to be joined to
the primary Master or Detail table. The current table, '%0c', already has
a join column specified. To cancel the join insertion you may
invoke the '%1c' menuitem."

/*%
** Explanation:
**	The user has attempted to specify a 2nd join on a lookup table.
**
** System Status:
**	The join insert is disallowed.
**
** User Action:
**	Continue with specifying a join.
*/

E_VQ004B_Already_Joined
"Column already has a join\n\
The column you are attempting to join already has a join associated
with it. You cannot join this column again.  To cancel the join
insertion you may invoke the '%0c' menuitem."

/*%
** Explanation:
**	The user has attempted to insert a join to a column which al
**	already has a subordinate join attached to it.
**
** System Status:
**	The join insert is disallowed.
**
** User Action:
**	Continue with specifying a join.
*/

E_VQ004C_Same_Table
"Joining to the same table\n\
You are trying to join a column of table '%0c' to another
column of table '%0c'. Joins are only used between two different tables
of the Visual Query, never to the same table. (Note that if
the database table is represented twice in the visual query, such
as, for example, if the 'employee' table were specified as both the
Master and Detail table, then the self join would be allowable.)
To cancel the join insertion you may invoke the '%1c' menuitem."

/*%
** Explanation:
**	The user has attempted to insert a join such that a
**	table would be joined to itself.
**
** System Status:
**	The joining of a table to itself is possible, but the
**	the table must be represented twice in the visual query.
**	For example if the employee table is both the master and
**	the detail table, then the self joins can be made.
**
** User Action:
**	Continue editing the visual query.
*/

E_VQ004D_Dif_Section
"Joining to a different section\n\
You are trying to join a column of table '%0c' to table '%1c' which
is in a different section. Lookup tables can only be joined to the
primary table in the section (either the Master section or Detail section).
To cancel the join insertion you may invoke the '%2c' menuitem."

/*%
** Explanation:
**
** System Status:
**	The join insert is disallowed.
**
** User Action:
**	Continue with specifying a join.
*/


E_VQ004E_Dbl_Lupjoin
"Two Lookup tables joined to the same column\n\
You are attempting to join the Lookup table '%0c' to a column
which is already joined to another lookup table.
A column can only be joined to a single Lookup table.
To cancel the join insertion, invoke the '%1c' menuitem."

/*%
** Explanation:
**
** System Status:
**	The join insert is disallowed.
**
** User Action:
**	Continue with specifying a join.
*/

E_VQ004F_Both_Lups
"Joining two Lookup tables together\n\
You are trying to join the Lookup table '%0c' to the Lookup table '%1c'.
Lookup tables must be joined to either the primary Master table,
or to the primary Detail, but cannot be joined to other Lookup tables.
To cancel the join insertion you may invoke the '%2c' menuitem."

/*%
** Explanation:
**
** System Status:
**	The join insert is disallowed.
**
** User Action:
**	Continue with specifying a join.
*/

E_VQ0050_Bad_Inc
"Only 'y', 'v', or 'n' are valid\n\
You have attempted to enter a value other than 'y', 'v', or 'n' into the
field inclusion column. A 'y' means that you want
this table column to become a simple field or table field column in the 
generated form for this frame. A 'v' means that you want the table column 
to become a local variable or hidden column in the generated code for this
frame.  An 'n' means that you do not want this table column
to be included in the generated form or code. Only these values are
allowed in this column of the table field."

/*%
** Explanation:
**	Bad character entered into the field inclusion column.
**
** System Status:
**	Put the old value back into the field.
**
** User Action:
**	Continue editing the visual query.
*/

E_VQ0051_Bad_Star
"Column cannot be included on the form\n\
The table column you have indicated cannot be included on the
generated form because this column is joined to one of the
primary query tables."

/*%
** Explanation:
**	The user has attempted to include (by starring) a
**	subordinate join column.
**
** System Status:
**	Put the old value back into the field.
**
** User Action:
**	Continue editing the visual query.
*/

E_VQ0052_Bad_NoStar
"Column must be included on the form\n\
The column you have indicated must be included on the
generated form because it has a column in a lookup table
joined to it."

/*%
** Explanation:
**	The user has attempted to not include (by unstarring) a
**	primary join column.
**
** System Status:
**	Put the old value back into the field.
**
** User Action:
**	Continue editing the visual query.
*/

E_VQ0053_Order_First
"You must enter the display order first\n\
The column you have just entered text into is to contain the
title of the Lookup popup which will display the possible lookup
values. You must first enter a number into the field to the left 
to indicate the order of the columns in the lookup"

/*%
** Explanation:
**	The user has attempted to not include (by unstarring) a
**	primary join column.
**
** System Status:
**	Put the old value back into the field.
**
** User Action:
**	Continue editing the visual query.
*/

E_VQ0054_Max_Tables
"Too many tables in Visual Query\n\
There can only be '%0d' tables in a Visual Query, and there are
already that many tables. You must delete one of the existing
tables before inserting any new tables to this Visual Query."

/*%
** Explanation:
**	The user is attempting to add a table, but there are already
**	the maximum number of tables in the Visual Query.
**
** System Status:
**	Disallow the operation.
**
** User Action:
**	Continue editing the visual query.
*/


E_VQ0055_Del_Join_2
"Column has 2 joins attached\n\
The current column you are positioned on has two join lines attached to
it, therefore, it is not clear which join you wish to delete. You must
move to the other end of the join you wish to delete before invoking
the 'DelJoin' function."

/*%
** Explanation:
**	The user is positioned on a column with 2 joins attached.
**
** System Status:
**	Disallow the destroy operation.
**
** User Action:
**	Continue editing the visual query.
*/


E_VQ0056_No_Lookup
"No '%0c' operation on the current field\n\
The '%0c' operation is not supported on the current field. You must
be on the fields which contain the names of the Master or Detail
table names before the '%0c' operation is active. When invoked on the
table name fields, this operation will bring up a list of tables
for you to choose from. If you want to cancel the new frame creation
then invoke the '%1c' menuitem."

/*%
** Explanation:
**
** System Status:
**	Disallow the operation.
**
** User Action:
**	Continue editing the visual query.
*/


E_VQ0057_Del_Join_Err
"Not positioned on a join column\n\
To delete a join you must be positioned on the column of a table which
has a join line attached to it. If there are two join lines proceeding
from the column, then you must move to the other end of the join you
wish to delete."

/*%
** Explanation:
**	The user is positioned on either a simple field, a column with
**	no join attached, or a column with 2 joins attached.
**
** System Status:
**	Disallow the operation.
**
** User Action:
**	Continue editing the visual query.
*/

E_VQ0058_Del_Not_Lup
"The current table is not a Lookup table\n\
Only Lookup tables may be deleted from the Visual Query, and
'%0c' is not a lookup table. The primary Master and (optional) detail
tables cannot be deleted."

/*%
** Explanation:
**	The user is positioned on either the master, or detail table
**	of the visual query, and has requested to delete it.
**
** System Status:
**	Disallow the operation.
**
** User Action:
**	Continue editing the visual query.
*/

E_VQ0059_No_Table_Join
"Only one table in Visual Query\n\
Because there is only one table in the Visual Query, there is no possibility
of creating a Join at this time. If you wished to join a table to itself,
then the table must be represented to the Visual Query twice, either  as
a detail table, or as a Lookup table. To add Lookup tables invoke the
'%0c' menuitem and select to insert a 'Lookup Table'."

/*%
** Explanation:
**	The user is attempting to insert a join, but there is only one
**	table in the Visual Query.
**
** System Status:
**	Disallow the operation.
**
** User Action:
**	Continue editing the visual query.
*/

E_VQ005A_No_Table_Name
"No table name entered\n\
You have not entered the name of a Lookup table. You must enter a table name
or select the '%0c' menuitem to cancel the Lookup table insertion. If you
would like a list of the tables in the database to select from invoke
the '%1c' menuitem."

/*%
** Explanation:
**	The user is attempting to insert a lookup table, but has
**	not entered an name into the name field.
**
** System Status:
**	The user is placed back on the field.
**
** User Action:
**	Enter the name of a table, or cancel.
*/


E_VQ005B_No_MD_Join
"No Master/Detail join found\n\
No join can be found between the Master and Detail tables
where columns in both tables have exactly the same name, data type,
nullability and length, indicating a probable join.
You will have to specify a join between the Master and Detail
tables by invoking the '%0c' menuitem.  Until this is done,
the frame you are creating cannot be used to generate code."

/*%
** Explanation:
**	The user is creating a new master/detail frame, and no join
**	was found between the master and detail tables.
**
** System Status:
**	This is just a warning.
**
** User Action:
**	Proceed to editing of the Visual Query.
*/



E_VQ005C_No_Lup_Join
"No Lookup table join found\n\
No join can be found between the new Lookup table you are
inserting, and the '%1c' table where the columns in both
tables have exactly the same name, type and length,
indicating a probable join.
You will have to specify a join between the tables by invoking
the '%0c' menuitem. Until this is done, the frame you are creating
cannot be used to generate code."

/*%
** Explanation:
**	The user is inserting a new lookup table into the frame, and no join
**	was found between the master and detail tables.
**
** System Status:
**	This is just a warning.
**
** User Action:
**	Continue editing of the Visual Query.
*/

E_VQ005D_TooManyEscapes
"There is no room for more escape code\n\
Only %0d pieces of escape code are allowed in a frame, and there
are already that many.  You can delete any unneeded pieces of
escape code to make room for this one"
/*%
** Explanation:
**	The maximum number of pieces of escape code already exists.
**
** System Status:
**	Return to previous menu.
**
** User Action:
**	Delete any unneeded escape code to make room for the new piece.
*/

E_VQ005F_No_Fields_On_Form
"Form '%0c' contains no fields."

/*%
** Explanation:
**	The user is editing escape code for a form with no fields.
**
** System Status:
**	This is just a warning.
**
** User Action:
**	Add fields to the form with the form editor.
*/

E_VQ0061_FormReadErr
"Error reading form %0c\n\
An error occurred reading form %0c from the database.  The status code
is %1d.  This is an unexpected internal error.  Please notify your 
Site Technical Contact."
/*%
** Explanation:
**      The form for this frame could not be retrieved from the 
**	database.
**
** System Status:
**      This probably indicates that the form is corrupt in the
**	database.
**
** User Action:
**      Save any recent changes and exit.
*/

/*
** vqcmperr.qsc errors
*/
E_VQ0064_InvalidListingFile
"The listing file is invalid.\n\
The current copy of the listing file for frame `%0c` is invalid.
Another compilation has overwritten the listing file for this 
frame.  This makes it impossible to display the current error."
/*%
** Explanation:
**      A subsequent compilation has overwritten the listing file.
**
** System Status:
**      Not an error.
**   
** User Action:
**      The user can fix errors in other frames.  To fix errors in the
**	current frame, exit and re-enter the Compilation Error frame.
*/   

E_VQ0065_CannotUpdateListFile
"The listing file for frame '%0c' cannot be updated.\n\
Error '%1d' occurred attempting to update the list file for this frame.
The information about which errors in this frame have been fixed
has been lost.  Recompiling this frame will regenerate this information."
/*%
** Explanation:
**      An error occurred attempting to update the listing file for
**	the frame.
**
** System Status:
**      Not a system error.  The information about which errors have
**	been fixed is lost.
**   
** User Action:
**      The user can recompile the frame.  This will create a new listing
**	file with up-to-date information.
*/   

E_VQ0066_NoSuchMenuitem
"There is no menuitem '%1c' for frame '%0c'\n\
The menuitem has been deleted or renamed since the frame was last
compiled.  Recompiling this frame will eliminate this error."
/*%
** Explanation:
**      The menuitem specified in the listing file does not exist.
**
** System Status:
**      Not an error.  The menuitem has been deleted or renamed.
**   
** User Action:
**      The user can recompile the frame.  This will create a new listing
**	file with up-to-date information.
*/   

E_VQ0067_ErrorOutsideSection
"This error occurs before the first named section.\n\
There is an error in the template file.  If the template file 
has been customized at your site, please fix the error.  Otherwise
notify your Site Technical Contact."
/*%
** Explanation:
**      An error has occurred before the first named section.  The error
**	must be in the template file.
**
** System Status:
**      Not an error.  The template file must be corrected.
**   
** User Action:
**      If the template file has been changed at the site, it must be corrected.
**	If the error is in the template file supplied with the system, the
**	Site Technical Contact must be notified.
*/   

E_VQ0068_UnknownErrorLocation
"Error location '%0c' is unknown.\n\
This is not a valid error location.  If the template file has been 
customized at your site, please be sure it is not defining this invalid
error location.  Otherwise notify your Site Technical Contact."
/*%
** Explanation:
**      An unknown error location has been declared.  There actual location
**	of the error cannot be determined.
**
** System Status:
**      Not a system error.  The actual location of the error must be
**	determined manually.
**   
** User Action:
**      If the template file has been changed at the site, it must be corrected.
**	If the error is in the template file supplied with the system, the
**	Site Technical Contact should be notified.  If the user can determine
**	the actual location of the error, he can fix it and proceed.
*/   

E_VQ0069_Cannot_Make_LP_Form
"The listpick form could not be created.\n\
This is an unexpected internal error.  Please notify your
Site Technical Contact. The error status is %0d"

/*%
** Explanation:
**	The listpick form for choosing a field could not be created.
**
** System Status:
**	We return to the escape type menu.
**
** User Action:
**	Save the current changes and exit the program.
*/


E_VQ006B_Yes_No_Field
"Only '%0c' or '%1c' are valid\n\
Only '%0c' (meaning 'yes') or '%1c' (meaning 'no') are valid
in this field. Any other values in this field are not valid."

 /*%
 ** Explanation:
 **	The user is editing one of the y/n fields on the visual query
 **	and has placed an invalid value into the field.
 **	The field is blanked out and the user is placed back on it.
 **
 ** System Status:
 ** User Action:
 **	Continue editing of the Visual Query.
 */

E_VQ00E1_ErrorsNoMore
"%1c '%0c' no longer has errors. Another user has fixed the errors and
recompiled it successfully."

 /*%
 ** Explanation:
 **	Another user, working concurrently, has removed the errors for this
 **	frame and recompiled it successfully.
 **
 ** System Status:
 **	Not an error; an informational message.
 **
 ** User Action:
 **	Edit the errors for a different frame.
 */

E_VQ00E2_HasBeenRecompiled
"%1c '%0c' has been recompiled by another user.  We will load the new
error information."

 /*%
 ** Explanation:
 **	Another user, working concurrently, has recompiled this frame.
 **
 ** System Status:
 **	The new error information will be loaded.
 **
 ** User Action:
 **	No action needed.

 */
E_VQ006C_Dup_menuitem
"Duplicate menuitem\n\
The menuitem text that you entered, '%0c' is identical to another
menuitem on frame '%2c'. Each menuitem of a particular frame
must be unique. Please edit the menuitem text so that it is unique.
Note that the uniqueness is tested without regard to case.
If you wish to cancel the insertion invoke the '%1c' menuitem."

/*%
** Explanation:
**	The user has entered a duplicate menuitem name.
**
** System Status:
**	The user is placed back on the form on the menuitem field.
**
** User Action:
**	Edit the menuitem so that it is unique.
*/


E_VQ006D_Destroy_RefFound
"Cannot destroy frame '%0c'\n\
Because the frame '%0c' is referenced from another frame in the
application it cannot be destroyed. If you wish, the frame can
be removed from its current location.
Answer 'y' for yes to the next question to remove the frame
from this point in the application flow diagram. If this is
done, the frame can be re-inserted into the application at a
different point later in development."

/*%
** Explanation:
**	The user has requested to destroy a frame from the
**	frame flow diagram, but there are other references to
**	the frame.
**
** System Status:
**	Ask the user if they wish to 'Remove' the frame from
**	the diagram.
**
** User Action:
**	The user can choose to remove the frame from the diagram
**	by answering yes to the next question, if the answer
**	is no then nothing will be done.
*/

E_VQ006E_Bad_Sel_Form
"The frame selection form could not be created.\n\
This is an unexpected internal error.  Please notify your
Site Technical Contact."

/*%
** Explanation:
**	The insert frame form for choosing a frame to be inserted into
**	the application flow diagram could not be created.
**
** System Status:
**	The insert is aborted.
**
** User Action:
**	Save the current changes and exit the program.
*/

E_VQ006F_Paste_Err1
"Cannot insert frame '%0c'.\n\
Frame '%0c' cannot be inserted into the application flow diagram because
it already appears in the diagram above the position it is to be
inserted into. Note that although a particular frame may be inserted into
the diagram in more than one place, it is not possible to place a frame
in the diagram such that it could be activated twice. You will be placed
back on the list of frames and will have another opportunity to
choose a frame to be inserted."

/*%
** Explanation:
**	The frame to be inserted already occurs in the call stack.
**
** System Status:
**	The insert is aborted. The user is placed back on the
**	list selection.
**
** User Action:
**	Choose another frame to be inserted.. or forget the
*/

E_VQ0070_Paste_Err2
"Cannot insert frame '%0c'.\n\
Frame '%0c' cannot be inserted into the application flow diagram because
one of the frames that it calls (possibly nested several frames deep)
namely '%1c' already appears in the diagram above the position that
'%0c' is to be inserted. Note that although a particular frame may be
inserted into the diagram in more than one place,
it is not possible to place a frame
in the diagram such that it could be activated twice. You will be placed
back on the list of frames and will have another opportunity to
choose a frame to be inserted."

/*%
** Explanation:
**	The a child of the frame to be inserted already
**	occurs in the call stack.
**
** System Status:
**	The inserted is aborted. The user is placed back on the
**	list selection.
**
** User Action:
**	Choose another frame to be inserted.. or forget the
*/

E_VQ0071_Seq_2
"Already a sequenced column\n\
The column '%0c' in table '%1c' is already indicated as the sequenced
column. There can only be one sequenced column for 
a particular table. If you want '%2c' to be the sequenced column you
must first remove the '%3c' indication from column '%0c'."

/*%
** Explanation:
**
** System Status:
**	The operation is disallowed.
**
** User Action:
**	Remove the sequenced indicator from the other column.
*/

E_VQ0072_Seq_3
"Sequenced columns must be integers\n\
The column '%0c' in table '%1c' is not of type integer of length 4.
Only integers of length 4 can be indicated as sequenced columns with
the '%2c' indicator."

/*%
** Explanation:
**
** System Status:
**	The operation is disallowed.
**
** User Action:
**	Must specify another column for the sequenced indicator,
**	or alter the table definition, and reconcile this frame
**	to the new table definition.
*/

E_VQ0073_TinyScreen
"Screen is too small\n\
The screen (or window) you are currently running in is less than
23 lines high, or less than 80 columns wide. The screen must be
at least this big to be able to run the Application Flow Diagram."

/*%
** Explanation:
**
** System Status:
**	The operation is disallowed.
**
** User Action:
**	The user must have a bigger (terminal emulator) window to 
**	run the application flow diagram in.
*/

E_VQ0074_NoInsertAllowed
"No frame insertion allowed\n\
The frame '%0c' is not a frame type which can have frames inserted below it.
Only Menu, Append, Browse, and Update frames can have child frames attached
to them."

/*%
** Explanation:
**
** System Status:
**	The operation is disallowed.
**
** User Action:
**	The user must have a bigger (terminal emulator) window to 
**	run the application flow diagram in.
*/


E_VQ0075_No_Paste_Frames
"No frames to insert\n\
There are no frames in this application which can be inserted."

/*%
** Explanation:
**	There are only 0 (a real error) or 1 frame in the application.
**	Since it is impossible to insert a frame as a child of itself
**	the case of one is an error.
**
** System Status:
**	Disallow the requested frame insertion.
**
** User Action:
**	Continue application flow diagram edit.
*/

E_VQ0076_No_Copy_Frames
"No frames to duplicate\n\
There are no frames in this application which could be duplicated.
You must first create some frames of type APPEND, UPDATE or BROWSE
before invoking the duplicate command."

/*%
** Explanation:
**	The user has invoked the "duplicate" menuitem.. but
**	there are 0 frames in the application which have visual
**	queries associated with them.
**
** System Status:
**	Disallow the requested frame insertion.
**
** User Action:
**	Continue application flow diagram edit.
*/


E_VQ0077_Mixed_Datatype_Join	
"WARNING: Different data types used in join.\n\
You have joined two columns that have different data types.  A join
between columns '%0c.%1c' and '%2c.%3c' may cause anomalies when accessing
data from tables within your database.  Please refer to the
Using INGRES Through Forms and Menus manual for more information on the problems
that you may encounter when joining two columns that are not of the same
data type."

/*%
** Explanation:
**	The join that was being attempted is between columns which
**	have different data types.
**
** System Status:
**	The join is allowed, but the user is warned about the difference
**	so that they may reconsider.
**
** User Action:
**	Continue editing the VQ.
*/

E_VQ0078_Mixed_Nullable_Join	
"WARNING: Nullable and non-nullable data type used in join\n\
You have joined a nullable column with a non-nullable column.  A join
between columns '%0c.%1c' and '%2c.%3c' may cause anomalies when accessing
data from tables within your database.  Please refer to the
manual for more information on the problems
that you may encounter when joining two columns with different
nullability characteristics."

/*%
** Explanation:
**	The join that was being attempted is between columns which
**	differ in their nullability.
**
** System Status:
**	The join is allowed, but the user is warned about the difference
**	so that they may reconsider.
**
** User Action:
**	Continue editing the VQ.
*/

E_VQ0079_Mixed_Length_Join	
"WARNING:  Data types of different lengths used in join\n\
You have joined two columns that have different lengths.  A join
between columns '%0c.%1c' and '%2c.%3c' may cause anomalies when accessing
data from tables within your database.  Please refer to the
manual for more information on the problems
that you may encounter when joining two columns with different
lengths."

/*%
** Explanation:
**	The join that was being attempted is between columns which
**	differ in length.
**
** System Status:
**	The join is allowed, but the user is warned about the difference
**	so that they may reconsider.
**
** User Action:
**	Continue editing the VQ.
*/

E_VQ007A_Join_Type
"Incompatible data types used in join\n\
You are attempting to join two columns that have incompatible data types.
A join between columns '%0c.%1c' and '%2c.%3c' is not allowed.
Please refer to the manual for more information on specifying joins."

/*%
** Explanation:
**	The join that was being attempted is between columns which
**	have incompatible data types.
**
** System Status:
**	The join is not allowed.
**
** User Action:
**	Continue editing the VQ.
*/

E_VQ007B_No_Cols
"WARNING: No '%0c' columns displayed\n\
The '%0c' table of this Visual Query has no columns which are displayed.
This is a condition which will prevent generating a form or code for this frame.
You must edit this Visual Query and specify some columns to be displayed
on the form before it can be used to generate code."

/*%
** Explanation:
**	The user has exited the visual query edit and left all of the
**	columns of the master (or detail) table set to undisplayed.
**	This warning may also be given if the user attempts
**	to edit either the form or the code from the VQ.
**
** System Status:
**	This is only a warning.. the frame will be saved but cannot
**	be used for code generation until this condition is fixed.
**
** User Action:
**	Must re-edit and fix before code gen can take place.
*/


E_VQ007C_No_Join
"WARNING: No join to table '%0c'\n\
The '%0c' table of this Visual Query is not joined to any other table.
All tables in Visual Querys must be joined to some other table.
This is a condition which will prevent generating a form or code for this frame.
You must edit this Visual Query and specify a join to table '%0c' before
this frame can be used to generate a form or code."

/*%
** Explanation:
**	The user has exited the visual query edit and left (at least) one
**	of the tables with no join.
**	This warning may also be given if the user attempts
**	to edit either the form or the code from the VQ.
**
** System Status:
**	This is only a warning.. the frame will be saved but cannot
**	be used for code generation until this condition is fixed.
**
** User Action:
**	Must re-edit and fix before code gen can take place.
*/


E_VQ007D_No_MD_Join
"WARNING: No Master/Detail join\n\
There is no join between the Master and Detail table of this Visual Query. 
This is a condition which will prevent generating code for this frame.
You must edit this Visual Query and specify a join between the Master
and Detail tables before this frame can be used to generate code."

/*%
** Explanation:
**	The user has exited the visual query edit but the master
**	and detail tables are not joined.
**	This warning may also be given if the user attempts
**	to edit either the form or the code from the VQ.
**
** System Status:
**	This is only a warning.. the frame will be saved but cannot
**	be used for code generation until this condition is fixed.
**
** User Action:
**	Must re-edit and fix before code gen can take place.
*/



E_VQ007E_No_Edit
"WARNING: Cannot edit '%0c'\n\
Due to the warning messages that were just displayed it is not
yet possible to edit the '%0c' as you have requested. You must
first fix the problems that were mentioned."

/*%
** Explanation:
**	The user has exited the visual query edit but the master
**	and detail tables are not joined.
**
** System Status:
**	This is only a warning.. the frame will be saved but cannot
**	be used for code generation until this condition is fixed.
**
** User Action:
**	Must re-edit and fix before code gen can take place.
*/

E_VQ007F_Bad_MD_Join
"WARNING: Problem with Master/Detail Join\n\
No master/detail join column(s) are either displayed, or contain
a non-null assignment expression. At least one of the master/detail
join columns must either be displayed, or contain an assignment
expression for the Visual Query to be used to generate code. This
condition must be fixed before code can be generated for this frame."

/*%
** Explanation:
**	The user has exited the visual query edit and left 
**	all of the join columns between the master and detail
**	table undisplayed, or with null assignment expressions.
**
** System Status:
**	This is only a warning.. the frame will be saved but cannot
**	be used for code generation until this condition is fixed.
**
** User Action:
**	Must re-edit and fix before code gen can take place.
*/



E_VQ0080_NoApplErrors		
"There are currently no compilation errors in application '%0c'."
/*%
** Explanation:
**	The application has no compilation errors.
**
** System Status:
**	We return to the calling frame.
**
** User Action:
**	This isn't an error; no user action is required.  The 
** 	application can now be tested or built into an image.
*/

E_VQ0081_NoApplErrors		
"All current compilation errors for application '%0c' have been fixed."
/*%
** Explanation:
**	All compilation errors have been fixed.
**
** System Status:
**	We return to the calling frame.
**
** User Action:
**	This isn't an error; no user action is required.  The 
** 	application can now be tested or built into an image.
*/

E_VQ0083_NoListFile
"Frame '%0c' has no listing file\n\
The listing file for this frame has been deleted.  This makes it 
impossible to fix errors, to examine the listing file, or
to mark errors fixed or unfixed.  Please recompile this frame to
produce a new listing file."
/*%
** Explanation:
**	The listing file has been deleted since the compilation.  Since
**	it contained the information about where errors occurred, they can
**	no longer be found.
**
** System Status:
**	Not a system error.
**
** User Action:
**	Recompile the frame to generate a new listing file.
*/

E_VQ0084_NoSuchSection
"Frame '%0c' has no tables in section '%1c'\n\
The section of the Visual Query which caused the compilation error
currently contains no tables.  The Visual Query has changed since 
the frame was last compiled.  Please recompile the frame."
/*%
** Explanation:
**	The Visual Query has changed since the frame was last compiled.
**	The section of the Visual Query which was causing the error no 
**	longer exists.
**
** System Status:
**	Not an error.
**
** User Action:
**	Recompile the frame without attempting to fix this error.
*/

E_VQ0086_GoodCompile 	"%1c '%0c' compiled successfully."
/*%
** Explanation:
**	The frame compiled successfully.
**
** System Status:
**	Not an error.
**
** User Action:
**	The user can examine and fix errors in other frames, if there are any.
*/

E_VQ0087_CannotFixVQError
"Cannot fix error in Visual Query\n\
Frame '%0c' has an error in its Visual Query.  You must edit the Visual Query
to remove the error before this frame can be compiled."
/*%
** Explanation:
**	There is an error in the frame's Visual Query.
**
** System Status:
**	Not a system error.
**
** User Action:
**	The user must go to the Visual Query-editing frame to fix the error,
**	before this frame can be compiled.
*/

E_VQ0088_No_Params_Allowed
"Parameters may not be passed to %0c frames.\n\
%0c frames do not support parameters passed to them from another
frame.  You may only pass parameters to the following frame types:
Append, Update, Browse, User, Report.  You may also pass parameters to
either 4gl, 3gl, or Database procedures."

/*%
** Explanation:
**      The frame in question does not accept parameters.
**
** System Status:
**      Same as if the parameter option were never selected.
**
** User Action:
**	Re-evaluate what they are try to do in the context of what
**	the product currently supports.
*/

E_VQ0089_Unknown_Frame_Type
"Parameters may not be passed to current frame.\n\
The frame you are currently positioned on does
not support parameters passed to it from another
frame.  You may only pass parameters to the following frame types:
Append, Update, Browse, User, Report.  You may also pass parameters to
either 4gl, 3gl, or Database procedures."

/*%
** Explanation:
**      An unknown frame type was encountered.  Because of this,
**	parameter passing to this frame will not be allowed.
**
** System Status:
**      Same as if the parameter option were never selected.
**
** User Action:
**	Re-evaluate what they are try to do in the context of what
**	the product currently supports.
*/

E_VQ008A_No_Source_Param
"Missing expression or field name for parent frame.\n\
When passing parameters between frames you must supply a field
name or expression that will be passed from the parent frame to
a field in the frame that is being called.  The frame upon which you
are currently positioned is the frame to which the field or
expression will be passed."

/*%
** Explanation:
**      The calling frame must pass something to the called frame if
**	a field is supplied to receive something in the called frame.
**
** System Status:
**      The parameter information will not be saved.
**
** User Action:
**	The user must correct the information by either adding the missing
**	item or by removing the entire line.
*/

E_VQ008B_No_Destin_Param
"Field name required to receive passed parameter.\n\
When passing parameters between frames you must supply a field
name from the current frame into which the information from the
parent frame will be placed."

/*%
** Explanation:
**      The called frame must have a place to store the information that
**	is being passed to it.
**
** System Status:
**      The parameter information will not be saved.
**
** User Action:
**	The user must correct the information by either adding the missing
**	item or by removing the entire line.
*/

E_VQ008C_No_Locals_Allowed
"Local variables not allowed.\n\
Local variables may not be defined for the current frame.  You may
only define local variables for the following frame types:
Append, Update, Browse, Menu."

E_VQ014B_No_Procedures_Allowed
"Local procedures not allowed.\n\
Local procedures may not be defined for the current frame.  You may
only define local procedures for the following frame types:
Append, Update, Browse, Menu."

/*%
** Explanation:
**      The frame in question does not allow local variables.
**
** System Status:
**      Same as if the local variable option were never selected.
**
** User Action:
**	Re-evaluate what they are try to do in the context of what
**	the product currently supports.
*/

E_VQ008D_duplicate_local
"Local variable or procedure already exists.\n\
A Local variable or local procedure of the name '%0c' already exists
for the current frame.  You may not create a new variable of the same
name.  If you wish to change the definition of a local variable choose
the 'Edit' option from the 'Edit Local Variables' form."

E_VQ0150_duplicate_proc
"Local variable or procedure already exists.\n\
A Local variable or local procedure of the name '%0c' already exists
for the current frame.  You may not create a new procedure of the same
name.  If you wish to change the definition of a local procedure choose
the 'Edit' option from the 'Edit Local Procedures' form."

/*%
** Explanation:
**      The frame in question already has a local variable or local
**	procedure with the name the user is trying to create.
**
** System Status:
**      The creation of the local variable/procedure will not be allowed.
**
** User Action:
**	The user should change the name of the local variable/procedure
**	or cancel the create option and then edit the existing local
**	variable/procedure.
*/

S_VQ008E_None_in_table  "There are no Local Variables in the table."
S_VQ00D8_No_Procs_in_Table	"There are no Local Procedures in the table."
S_VQ008F_Delete_Prompt "Do you wish to destroy Local Variable '%0c'? "
S_VQ0090_Enter_new_name_Prompt  "Enter new name: "
S_VQ0091_No_Local_Vars        "Frame '%0c' has no Local Variables defined."
S_VQ0092_No_Global_Vars       "There are no Global Variables defined."
S_VQ0093_No_Global_Cons       "There are no Global Constants defined."
S_VQ0094_Select_Parameter     "Parameter Selection"
S_VQ00D7_Max_Local_Var  "A maximum of %0d Local Variables is reached.\n\
You cannot create more than %1d local variables per frame."

E_VQ0095_Cannot_Move_Empty_Row  
"Cannot move an empty row.\n\
The cursor must be on a row containing a parameter to use the \"Move\"
command."

/*%
** System Status:
**      "Move" command is ignored.
**
** User Action:
**      Put cursor on row containing a valid column name.
*/

E_VQ0096_No_Form_For_Frame	
"No form is associated with this frame.\n\
Frame '%0c' is a type of frame that does not have an associated form."

/*%
** Explanation:
**	Cannot provide form help for a frame type that has no form.  These
**	types of frames are: 3GL, 4GL, DB Proc, etc.
**
** System Status:
**	No ListChoice help will be provided.
**
** User Action:
**	Use ListChoice for help on local or global variables or enter
**	the parameter by hand.
*/

E_VQ0097_No_Locals_Defined
"No local variables defined.\n\
Frame '%0c' is a type of frame for which local variables cannot be
defined."

/*%
** Explanation:
**      The frame in question does not allow local variables.
**
** System Status:
**      No ListChoice help will be providec.
**
** User Action:
**	Use ListChoice for help on global variables, global constants, or enter
**	the parameter by hand.
*/

E_VQ0098_No_Form_Defined	
"No form has been specified for frame.\n\
A form has not been specified for use with frame '%0c'.  By editing the
details of the frame description, you may add a form to be used with
frame '%0c'."

/*%
** Explanation:
**	No form name has been supplied for use with the frame in question.
**
** System Status:
**	No ListChoice help will be provided.
**
** User Action:
**	Modify the frame definition if a form is desired.  Otherwise, 
**	ListChoice help may be obtainable on global variables or constants.
*/

E_VQ0099_LockStack		
"Internal error in lock administration.\n\
An internal error has been detected which means that edit locks are not being
properly maintained.  Concurrent users may not be aware of your edit."

/*%
** Explanation:
**	Internal diagnostic - not supposed to happen.
**
** System Status:
**	We will continue anyway.
**
** User Action:
**	Report a bug.
*/

/*
** vqrecapp.qsc
*/
F_VQ012E_UseMarkedComps "Use what is currently marked"
F_VQ012F_MarkFrames 	"Mark frames for reconciliation"

/*
**	Menuitems
*/
F_VQ0130_ExamineOld		"OldVQ"
F_VQ0131_ExamineNew		"NewVQ"

/*
** Menuitem Explanations
*/
F_VQ0132_Reconcile_OK		"Accept the reconciliation changes"
F_VQ0133_Reconcile_Cancel	"Reject the reconciliation changes"
F_VQ0134_Examine_NewVQ		"Examine the new Visual Query"
F_VQ0135_Examine_OldVQ		"Examine the old Visual Query"

/* Local Variable/Local Procedure Destroy */
F_VQ0136_Confirm_LV_Destroy	"Confirm Request to Destroy"
F_VQ0137_Local_Variable		"Local Variable"
F_VQ014F_Local_Procedure	"Local Procedure"

S_VQ009A_Reconcile_Banner	"Reconcile Visual Query"
S_VQ009B_Reconciling		"Reconciling '%0c' . . ."
S_VQ009C_Reconcile_Done		"The reconciliations are complete."

E_VQ009D_Nothing_marked	
"Nothing is marked for use\n\
If you wish to exit without doing anything, invoke 'Cancel'."
/*%
** Explanation:
**      No application components are marked for use.
**
** System Status:
**      The user will remain in the current frame.
**
** User Action:
**	The user can either mark some frames for use, or Cancel.
*/

E_VQ009E_No_VQ_Frames
"No frames contain visual queries\n\
No frames in application '%0c' contain visual queries.  Thus there is
nothing to reconcile."
/*%
** Explanation:
**      There are no Append, Browse, or Update frames in the application.
**	Thus there are no Visual Queries to reconcile with the database.
**
** System Status:
**      The user will be returned to the previous menu.
**
** User Action:
**	Reconciliation makes no sense, but the user can choose any other
**	valid function.
*/

E_VQ009F_Bad_mark
"Only 'y' and 'n' are valid\n\
You have attempted to enter a value other than 'y' or 'n' into the
use column. A 'y' character would mean 'yes' that you want to use this
one, and a 'n' means that 'no' you do not want to use it.  Only these
values are allowed in this column of the table field."

/*%
** Explanation:
**	Bad character entered into the use column.
**
** System Status:
**	Don't leave the column until a valid character is entered.
**
** User Action:
**	Continue editing the visual query.
*/


E_VQ00A0_No_Frames
"No frames remain for reconciliation\n\
You have chosen not to reconcile any of the frames in application '%0c'."
/*%
** Explanation:
**      All of the frames originally chosen for reconciliation were locked, 
**	and the user chose not to reconcile them.
**
** System Status:
**      The user will be returned to the previous menu.
**
** User Action:
**	Wait until the frames are no longer locked, and then reconcile them.
*/


/*
** vqrecfrm.qsc
*/

E_VQ00A2_Primary_Master_Gone
"The primary master table has been deleted\n\
Table '%1c' has been deleted.  This is the primary table in the master
section for frame '%0c'.  It is not possible to reconcile this change.
You must either delete the frame or recreate the table."
/*%
** Explanation:
**      The primary master table for this frame has been deleted.
**	This change cannot be reconciled.
**
** System Status:
**      The next frame will be processed.
**
** User Action:
**	Proceed with the reconciliation.
*/

E_VQ00A3_Primary_Detail_Gone
"The primary detail table has been deleted\n\
Table '%1c' has been deleted.  This is the primary table in the detail
section for frame '%0c'.  It is not possible to reconcile this change.
You must either delete the frame or recreate the table."
/*%
** Explanation:
**      The primary detail table for this frame has been deleted.
**	This change cannot be reconciled.
**
** System Status:
**      The next frame will be processed.
**
** User Action:
**	Proceed with the reconciliation.
*/

E_VQ00A4_No_Primary_Master
"This frame has no primary master table\n\
Frame '%0c' has no primary master table.  This is a severe error,
which makes the frame's visual query useless.  Please contact your Site 
Technical Contact for assistance."
/*%
** Explanation:
**      The frame has no primary master table.  This shouldn't happen,
**	but if it does, the visual query is useless.
**
** System Status:
**      We won't try to reconcile this frame.
**
** User Action:
**      Proceed with the reconciliation.
*/

E_VQ00A5_No_Primary_Detail
"This frame has no primary detail table\n\
Frame '%0c' has no primary detail table.  This is a severe error,
which makes the frame's visual query useless.  Please contact your Site 
Technical Contact for assistance."
/*%
** Explanation:
**      The frame has no primary detail table.  This shouldn't happen,
**	but if it does, the visual query is useless.
**
** System Status:
**      We won't try to reconcile this frame.
**
** User Action:
**      Proceed with the reconciliation.
*/

/*
** The lines which make up the reconciliation report follow. They should be
** self-explanatory.  Embedded newlines in them separate lines in the report
*/
S_VQ00A6_Reconcile_Report	
			"Reconciliation report for Application '%0c'"
S_VQ00A7_Reconcile_Begun
			"Reconciliation session begun at %0c"
S_VQ00A8_Reconcile_Ended
			"Reconciliation session ended at %0c"
S_VQ00A9_No_Master	
"The primary master table '%0c' has been deleted.\nFrame '%1c' cannot 
be reconciled."
S_VQ00AA_No_Detail	
"The primary detail table '%0c' has been deleted.\nFrame '%1c' cannot 
be reconciled."
S_VQ00AB_Table_Deleted	"Table '%0c' has been deleted."
S_VQ00AC_VQ_Invalid	"The visual query for frame '%0c' is invalid:"
S_VQ00AD_No_Changes	"No changes were needed for frame '%0c'."
S_VQ00AE_Columns_Deleted	"Column(s) have been deleted:"
S_VQ00AF_Columns_Added		"Column(s) have been added:"
S_VQ00B0_Alias_Changes		"Column alias names have changed:"
S_VQ00B1_Tbl_Alias_Changes	"In table '%0c':"
S_VQ00B2_Alias_Change		"'%0c' has changed to '%1c'."
S_VQ00B3_User_Field_Conflict
"The user field '%0c' now causes a field name conflict.\nIts
'%1c' escape code will be deleted:"
S_VQ00B4_Severe_Errors		
"Frame '%0c' has severe errors and cannot be reconciled."
/*
** The "datatype has changed" message is formatted as:
**	1. Translation of S_VQ00B5_Datatype_Changed1
**	2. old datatype name
**	3. Translation of S_VQ00B6_Datatype_Changed2
**	4. new datatype name
*/
S_VQ00B5_Datatype_Changed1	"The datatype has changed from"
S_VQ00B6_Datatype_Changed2	"to"
S_VQ00B7_Bad_Seq_Datatype	
"The datatype is no longer valid for a sequenced column.\nThe column
will no longer be sequenced."
S_VQ00B8_Previous_Keys	"The key columns for this table were:"
S_VQ00B9_Current_Keys	"The key columns for this table are now:"
/*
** The "join is no longer valid" message is formatted as:
**	1. Translation of S_VQ00BA_Bad_Join1
**	2. First column name
**	3. Translation of S_VQ00BB_Bad_Join2
**	4. Second column name
**	5. Translation of S_VQ00BC_Bad_Join3
*/
S_VQ00BA_Bad_Join1	"The join between "
S_VQ00BB_Bad_Join2	"and"
S_VQ00BC_Bad_Join3	"is no longer valid."
S_VQ00BD_Table_Changed	"Table '%0c' has changed:"
S_VQ00BE_Column_Changed	"Column '%0c' has changed:"
S_VQ00BF_Escape_Code_Deleted	
"Column '%0c' has %1c escape code which will be deleted:"
S_VQ00C0_Changes_Accepted	"The changes for frame '%0c' were accepted."
S_VQ00C1_Changes_Rejected	"The changes for frame '%0c' were rejected."
S_VQ00C2_Begin_Escape_Code	"------Begin Escape Code------"
S_VQ00C3_End_Escape_Code	"------End Escape Code--------"
S_VQ00C4_Locked			
"Frame '%0c' was locked by another developer.\nIt was not reconciled."

S_VQ00D4_CannotReconcile
"There were errors reading frame '%0c' from the database.\n\
It cannot be reconciled."

/*
** The first argument to this one should be ERget(FE_OK)
*/
S_VQ00C5_Accept_Changes
"Choose '%0c' to accept the changes to frame '%1c'."

/*
** The first argument to this one should be ERget(FE_End)
*/
S_VQ00C6_End_Reconcile
"Choose '%0c' to end the reconciliation session."

/*
** Datatype qualifiers
*/
S_VQ00C7_with_null			"(nullable)"
S_VQ00C8_not_null			"(not nullable)"
S_VQ00C9_not_null_with_default		"(not nullable, with a default)"

/*
** Quote arguments to make them stand out.  The second form combines a table
** and column name, and quotes the whole thing.
*/
S_VQ00CA_Quote_Arg			"'%0c'"
S_VQ00CB_Quote_Table_Column		"'%0c.%1c'"

E_VQ00CC_Bad_Msg_File
"Internal error: bad message file\n\
The message E_VQ00F8_id2 is > 26 long, should be smaller"
/*%
** Explanation:
**	Shouldn't happen if the message file is good.
**
** System Status:
**	Cannot edit the AFD till this is fixed.
**
** User Action:
*/

E_VQ00CD_No_Def_Vals
"Table field columns cannot have default values\n\
The column '%1c' in table '%0c' will become a column in
a table field. It is not possible to set default values for
displayed or hidden columns of table fields."
/*%
** Explanation:
**	An Append frame, and the user has tried to set a default starting
**	value for the column, but this is not supported.
**
** System Status:
**	The value that the user entered is blanked out.
**
** User Action:
**	Continue edit of the visual query.
*/

E_VQ00CE_No_Tab_Key
"WARNING: table '%0c' has no unique key\n\
A unique key is used for primary tables in UPDATE frames, to
build the WHERE clause for UPDATE and DELETE statements.
\n\
When no unique key is available, every column in the table
is used to build the WHERE clause in UPDATE and DELETE
statements. That can hurt performance, particularly for 
tables with many columns.
\n\
You can continue building this frame now, and add a unique
key later, but after doing so, you must run the Vision
\"Reconcile\" menuitem on this frame. That will cause
Vision to see and use the new unique key. 
\n\
Consult your SQL Reference Manual or your Site Technical 
Contact for information on how to create a unique key
or unique index on a table."
/*%
** Explanation:
**	The user is using a table with no unique key in an update frame.
**
** System Status:
**	This is just a warning.
**
** User Action:
**	Continues..
*/

E_VQ00E3_WideRowWarning
"WARNING: table '%0c', data row too wide\n\
Table '%0c' does not have a unique key and its row width (%1d bytes) is more
than half the maximum allowed by Ingres (%2d bytes). Errors related to too
large a row size may occur when you run this frame and select 'Go'. 
\n\n
In Update frames, when you press 'Go', Vision must SELECT two copies of
each column in the table's unique key; one copy is displayed on the screen,
the other is saved for UPDATE and DELETE statements.
\n\n
For tables like this one that do not have a unique key, every column in the
table is used as the unique key. If all columns are displayed, then every
one will appear twice in the list of selected columns. This
may result in a list of columns that is wider than the maximum size
allowed by Ingres.
\n\n
You can continue building this frame now, and create a unique key for the
table later. Remember to run 'Reconcile' on the frame after creating
the unique key."
/*%
** Explanation:
**	The user is using a table with no unique key in an update frame.
**	This message will only show up when the DBMS that Vision is running
**	against is Ingres (not a Gateway or Star), and the rowsize of one
**	of the primary tables in an update frame is greater than one half
**	the maximum record size allowed by Ingres (currently 2008).
**
** System Status:
**	This is just a warning.
**
** User Action:
**	Continues..
*/

E_VQ00CF_No_Asgn
"No assignment to detail table join field\n\
Since the detail table join field of a master/detail append frame
is always assigned the value of the master table join field it
is not possible to assign it a value."

/*%
** Explanation:
**	An Append frame, and the user has tried to set an assignment
**	expression for the detail table join field.
**
** System Status:
**	The value that the user entered is blanked out.
**
** User Action:
**	Continue edit of the visual query.
*/


E_VQ00D0_No_Restrict
"No restrictions on detail table join field\n\
Since the detail table join field of a master/detail frame
is always equal to the value of the master table join field it
is not possible to place a restriction on it. Perhaps you would
get the behavior you desire by placing the restriction on the
master table's join field."

/*%
** Explanation:
**	The user has attempted to place a restriction on the
**	detail table's join field.
**
** System Status:
**	Such as restriction does not make sense. 
**	The restriction is cleared.
**
** User Action:
**	Continue edit of the visual query.
*/


E_VQ00D1_No_Sort
"No sorting on detail table join field\n\
Since the detail table join field of a master/detail frame
is always equal to the value of the master table join field it
is not possible to sort on it. Perhaps you would get the behavior 
you desire by placing the sort specification on the master 
table's join field."

/*%
** Explanation:
**	The user has attempted to place a sort on the
**	detail table's join field.
**
** System Status:
**	Such as sort does not make sense. 
**	The sort is cleared.
**
** User Action:
**	Continue edit of the visual query.
*/



E_VQ00D2_Order_First
"You must specify sort order first\n\
Before the ascending/descending sort specification can be made, you
must first specify the sort order for this column."

/*%
** Explanation:
**	The user has attempted to enter a sort direction without
**	first giving a sort order for the column.
**
** System Status:
**	The sort direction is cleared.
**
** User Action:
**	Continue edit of the visual query.
*/


E_VQ00D3_No_Compile
"Cannot compile %0c: '%1c'\n\
Only 4GL frames and procedures and reports can be compiled."

/*%
** Explanation:
**	The user has attempted to compile a frame type which does not support
**	compilation.
**
** System Status:
**	The command is ignored.
**
** User Action:
**	Choose another action.
*/

E_VQ00E0_Is_Derived	"'%0c' is a derived %1c.\n\
You have selected a derived %1c.  Its value comes from its derivation formula,
and a user cannot land on it.  You cannot assign values to it.  You also
cannot place escape code on it.\n\n\
Existing escape code for derived fields and columns will be purged when the
source code is regenerated.  You can force source code to be regenerated by
compiling the frame from the Application Flow Diagram."

/*%
** Explanation:
**	The user has chosen a derived field or column in an inappropriate 
**	context.
**
** System Status:
**	The choice is ignored.
**
** User Action:
**	Choose another field or column, or edit the form to make the one chosen
**	no longer a derived field.
*/

E_VQ00E4_No_Proc_Name
"No procedure name specified\n\
In order to edit a local procedure's source code you must specify its name."

/*%
** Explanation:
**	The user has selected EditSource from the "create a procedure" popup
**	to edit a local procedure's source code, but no procedure name has
**	been specified.
**
** System Status:
**	Resume on the name field, prompting the user to enter a procedure
**	name.
**
** User Action:
**	Enter a procedure name then select EditSource again.
*/

E_VQ00E5_Update_Source_Warning
"WARNING: Source code needs to be updated\n\
You must update the source code for this procedure to reflect its new name."

/*%
** Explanation:
**	The user has renamed a local procedure on the Edit Local Procedures
**	screen. There is existing source code for this procedure which must
**	be updated to reflect the procedure's new name.
**
** System status:
**	No action, this is just a warning.
**
** User action:
*	Select EditSource and edit the procedure's source code, changing the
**	procedure name.
*/

E_VQ00E6_Source_Exists
"You must destroy the source code for this procedure first.\n\
There is existing source code for this procedure, which must be deleted
before the procedure can be destroyed on this screen."

/*%
** Explanation:
**	The user is trying to destroy a procedure on the Edit Local Procedures
**	screen, but there is source code for this procedure which must be
**	destroyed first.
**
** System status:
**	Do nothing (as opposed to destroying the procedure)
**
** User action:
**	Select EditSource and delete the procedure's source code.
*/


/*
** Settings for "Locks Held on Displayed Data" frame behaviour (vqfrbeha.qsc)
*/
F_VQ0159_No_Lock	"None"
F_VQ015A_Dbms_Lock	"Dbms"
F_VQ015B_Opt_Lock	"Optimistic"

/* ListChoices title & explanations for the above */
F_VQ015C_Locks_Held	"Locks Held on Displayed Data"
F_VQ015D_No_Lock_Exp	"Do not hold locks on data"
F_VQ015E_Dbms_Lock_Exp	"Hold locks on displayed data"
F_VQ015F_Opt_Lock_Exp	"Use optimistic locking"

/* Listchoices title for optimistic locking column types */
F_VQ0160_Integer_Cols	"Integer Columns"
F_VQ0161_Date_Cols	"Date Columns"
F_VQ0162_Char_Cols	"Char and Varchar Columns"

E_VQ00E7_Lock_Behaviour	"Invalid setting for frame behaviour.\n
You have entered an invalid setting for this frame behaviour. Select
ListChoices for a list of valid settings."
/*%
** Explanation:
**	The user entered an invalid setting for the "locks held on displayed
**	data" frame behaviour.
**
** System status:
**	Place the cursor on the invalid entry
**
** User action:
**	Enter a valid setting.
*/

E_VQ00E8_No_Locking_Cols "No columns specified for optimistic locking.\n
You must specify at least one column to be used for optimistic locking.
Select Help for more information."

/*%
** Explanation:
**	The user selected optimistic locking (a setting of "optimistic" for
**	the locks held frame behaviour) but didn't enter the names of any
**	columns to be used for optimistic locking.
**
** System status:
**	Place the cursor on the table field
**
** User action:
**	Enter one or more column names to use for optimistic locking
*/

E_VQ00E9_Nonexist_Col "Column '%0c' does not exist in table '%1c'"

E_VQ00EA_Wrong_Type "Column '%0c' is the wrong datatype."

E_VQ00EB_Sequence_Col "An update counter must be an integer column."

E_VQ00EC_Date_Col "A date/time stamp must be a date column."

E_VQ00ED_Username_Col "A username must be a varchar or char column."

E_VQ00EE_No_Cols "No choices available. There are no %0c columns in table %1c."

S_VQ00F9_Changed_Locktype
"All columns which were used for optimistic locking have been deleted or\n\
have had their datatype changed. The 'Locks Held' frame behaviour will\n\
be changed to 'None'."
/*%
** Explanation:
**	After a frame has been reconciled, the 'locks held' frame behaviour
**	is set to 'optimistic', but there are no longer any columns in the 
**	metaframe which use optimistic locking.
**
** System status:
**	Change the 'locks held' frame behaviour to 'none'.
**
** User action:
**	None required.
*/

S_VQ00FA_Unset_Optimistic
"Column will not be used for optimistic locking."
/*%
** Explanation:
**	Reconciler: a column's datatype has changed; the column was being
**	used for optimistic locking.
**
** System status:
**	Change the metaframe so that this column is no longer used for
**	optimistic locking.
**
** User action:
**	None required.
*/

E_VQ00FB_Unique_Key "Column '%0c' is part of the unique key for this table.\n\
It cannot be used as an optimistic locking column."

E_VQ00FC_Key_Column "Key columns will not be used for optimistic locking."

/* Messages for import menuitem */

F_VQ0163_ImportExpl "Duplicate the visual query of an existing frame in another application."

F_VQ0164_import_prmpt "Importing a Frame"

F_VQ0165_Appl_Name	"Application Name"

F_VQ0166_Select_Imp_App
"Select the application from which you want to\nimport a frame"

F_VQ0167_Select_Imp_Frame "Select the frame to be imported from "

E_VQ00EF_No_Import_Frames
"No frames to import\n\
There are no APPEND, BROWSE or UPDATE frames in this application."

/* BLOBS (non)support: */

E_VQ00F0_Unsupported_Type
"WARNING: Column with unsupported data type.\n\
Column '%0c' in table '%1c' has a datatype not supported under the current
release of Vision. The column will be ignored: its name will not appear on
the Visual Query, and no field will be created for it on the data entry form."
/*
** Explanation:
**	Vision has found a long varchar or long bit column while creating
**	a metaframe structure. In 6.5, long varchar and long bit are not
**	supported in Vision.
** System status:
**	After issuing this warning, Vision will ignore the column, i.e. it
**	will not show up on the visual query or in the generated code, and
**	no field will be created for it on the generated form.
** User Action:
**	Continue with the Vision session.
*/	

/* Help file headings for local procedures: */

S_VQ00F1_lpEditSrcHelpBanner	"Edit Local Procedure Source Code"

S_VQ00F2_lpEditFileHelpBanner	"Edit Source Code File"

S_VQ00F3_lpEditLineHelpBanner	"Edit Source Code Line"

F_VQ0168_EndLocProcEdit	"Ending local procedure code edit with unsaved changes"
/* ---------------------------------------- */

/*special return types for local procedures */
F_VQ0169_none	"none"		/* DB_NODT */
/* ---------------------------------------- */

E_VQ00F4_Bad_Return_Type	"Illegal value for return type: `%0c'.\n
The return type of a local procedure must be a legal INGRES type or `none'.
Select ListChoices to get a list of legal return types."
/*
** Explanation:
**	User has entered an invalid Return Type for a local procedure
** System status:
**	After issuing this error, the cursor will resume at the beginning of
**	the Return Type field.
** User Action:
**	Enter a valid Return Type.
*/	

E_VQ00F5_Bad_Data_Type	"Illegal value for data type: `%0c'.\n
The data type must be a legal INGRES type or any record type defined for the
application.  Select ListChoices to get a list of legal INGRES types."
/*
** Explanation:
**	User has entered an invalid Data Type for a local variable
** System status:
**	After issuing this error, the cursor will resume at the beginning of
**	the Data Type field.
** User Action:
**	Enter a valid Data Type.
*/	

/* ------------------------------------------------------------------- */

/* Messages connected with duplicating a Vision form */

F_VQ0170_DupformTitle   "Duplicating a form"
F_VQ0171_DupformPrmpt	"Duplicate the form of frame %1c?"
F_VQ0172_DupformYes	"Duplicate the form"
F_VQ0173_DupformNo	"Generate a new form"
E_VQ00F6_NoForm
"There is no form for frame `%0c'. Vision will create a default form."
/*
** Explanation:
**	User is duplicating a frame, and wants to duplicate the form, but
**	the frame has no form.
** System status:
**	After issuing this error, Vision will not attempt to create a form at
**	this stage. When the user wants to edit the form for the new frame,
**	a default form will be created.
** User action:
**	None.
*/
S_VQ00F7_DupForm	"Duplicating the form . . . "

/* ------------------------------------------------------------------- */

E_VQ00F8_No_Valid_Column	
"No column based on supported datatypes found.\n
All column(s) in table `%0c' are of unsupported datatypes under the current
release of VISION. To generate a form or code for this frame, a table is
required to contain one or more columns of supported datatypes. Since the
frame becomes invalid without a form, it will be DESTROYED."
