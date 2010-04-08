/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	erab.msg -	ABF Message Definitions File.
**
** Description:
**	Contains the definitions for the messages and error messages of ABF.
**
** History:
**	Revision 6.0  87/08/04  bobm
**	Initial revision.
**	Revision 6.2  89/09/20  kevinm  (JupBug #7289)
**	Added two messages for the OtherLanguages menuitem,
**	F_AB002D_OtherLang_expl and F_AB002E_Other.
**
**	Revision 6.3  89/11  wong
**	Added illegal file-name message, E_AB0138.
**
**	90/03  wong  Added unsupported language message, E_AB0139.
**	23-oct-1990 (mgw)
**		Corrected some misspellings. All are in comments, not messages,
**		but these comments get picked up for the messages.txt file via
**		erxtract and are thus in a deliverable.
**	14-aug-91 (blaise)
**		Added E_AB0154_NoObjFile.
**	22-aug-91 (blaise)
**		E_AB0154_NoObjFile now only takes one argument.
**	26-feb-1992 (mgw)
**		Added E_AB0155_OSCompErr.
**	28-feb-1992 (mgw) Bug #41111
**		Made S_AB0146_ReturnChange and S_AB0148_FrameChange obsolete
**		and replaced them with S_AB0156_NewReturnChange and
**		S_AB0157_NewFrameChange respectively to have these messages
**		refer to frames or procedures as appropriate instead of just
**		frames.
**	16-Nov-1992 (donc) Bug #44162
**		Added E_AB036_BadSymbolName 
**	13-oct-92 (davel)
**		Added fast messages for support of the new Global Constants 
**		File utility screen - F_AB00E0 through F_AB00E6.
**	26-oct-92 (davel)
**		Added E_AB0129 and E_AB012A for global constant backward
**		compatibilty warnings.
**	20-nov-92 (mgw)
**		Added more fast messages for Global Constants support.
**		F_AB00E7 - F_AB00EA will be used in an IIUFccConfirmChoice
**		call.
**	12-jan-93 (essi)
**		Added several messages for the support of global constants
**		file.
**	15-jan-93 (davel)
**		Added E_AB012B_TypeChange (datatype of global constant cannot
**		be changed during "edit a constant" operation). Also made
**		small changes to other global constant messages.
**	11-feb-93 (blaise)
**		Integrated DonC's change from 63p code line: added
**		E_AB036B_Bad_SymbolName
**	18-Feb-1993 (connie) Bug #48982
**		Added S_AB0158_RunHostLangProc,F_AB00BC_Title,F_AB00BD_Yestext,
**		F_AB00BE_Notext, F_AB00BF_CHelp
**	23-feb-1993 (connie) Bug #40884
**		Added E_AB0281_PathName to check for pathname in source file
**	22-feb-93 (davel)
**		Added E_AB036A_Bad_PassDecimal message.
**	26-jul-93 (blaise)
**		Added F_AB00EB to F_AB00EF - comments written to global
**		constants file.
**	16-mar-94 (lan)
**		Fixed bug #58066 - Replaced 'Technical Support Representative'
**		with 'Site Technical Contact'.
*/

#define	_AB_CLASS	40

/*
**	Exception Handler Error Messages ("abexchand.c")
*/
E_AB0001_Exceptional_error	
"Exceptional error found.  Writing out application . . ."
E_AB0002_Error_writing	
"Error while writing application. Changes lost!  Exiting . . ."
E_AB0003_Exception	"Exception: %0x "

/*
** abcatalog.qsc
*/

/*
** Catalog and Edit Frame Menu Items and Titles
**	("editapp.qc" and "abcatlog.qc".)
*/
F_AB0001_Cat_title		"%0c - Applications Catalog"
F_AB0002_AppCreate_title	"%0c - Create an Application"
F_AB0003_AppDetail_title	"%0c - MoreInfo about an Application"

/*
**	Menu items for Catalog and Edit Application Frames.
*/
F_AB0004_IngMenu	"INGRES/Menu"
F_AB0005_UtilitiesTitle	"Application Utilities SubMenu."
F_AB0006_Defaults	"Defaults"

/*
** 	Catalog Menu explanation strings
*/
F_AB0007_Create_expl	"Create a new application."
F_AB0008_Destroy_expl	"Destroy the selected application."
F_AB0009_Edit_expl	"View or edit the selected application."
F_AB000A_Rename_expl	"Rename the selected application."
F_AB000B_Go_expl	"Test the application."
F_AB000C_Util_expl	"Access the Image and INGRES/Menu utilities."
F_AB000D_Quit_expl	"Exit ABF."

F_AB000E_Image_expl	"Create the application's image file."
F_AB000F_IngMenu_expl	"Call the INGRES/Menu utility."

F_AB0010_Defaults_expl	"View or edit the application defaults."

/*
**	abimgfrm.qc
*/
F_AB0011_Image_expl "Create the application's image file with the given name."
F_AB0012_BatchImage	"BatchImage"
#define AB_BatchImg F_AB0012_BatchImage
F_AB0013_Bimg_expl	"Create image file for application in background."

/*
** editapp.qc
*/
F_AB0014_AppEdit_title	"%0c - Edit an Application"

F_AB0015_CreateObj_expl	"Create a new frame or procedure."
F_AB0016_DestroyObj_expl "Destroy the selected frame or procedure."
F_AB0017_EditObj_expl	"View or edit the selected frame or procedure."
F_AB0018_RenameObj_expl	"Rename the selected frame or procedure."
F_AB02FA_Form		"form"
F_AB02FB_Errors		"Errors"
F_AB02FC_ErrorsExpl	"Display and edit compilation errors"
F_AB02FD_ChooseBuf	"Current %0c '%1c'\nDefault starting %2c '%3c'"
F_AB02FE_Procedure	"procedure"
F_AB02FF_Frame		"frame"
F_AB0300_Parts_expl	"Edit the application global components."

S_AB0005_StartAt	"Start execution at '%0c' ?"
S_AB0006_ChooseStart	"Choose starting frame or procedure"
/*
** Application Component Create Titles.
*/
F_AB0019_AFPCrt_title	"%0c - Create a Frame or Procedure"
F_AB001A_AFrmCrt_title	"%0c - Create a Frame"
F_AB001B_AProcCrt_title	"%0c - Create a Procedure"
F_AB031C_AGloCrt_title  "%0c - Create a Global Variable"
F_AB031D_ARecCrt_title  "%0c - Create a Record Definition"
F_AB031E_AConstCrt_title "%0c - Create an Application Constant"
F_AB031F_ARattCrt_title	"%0c - Create a Record Attribute"

F_AB001C_Frame_expl	"Select an application frame component for creation."
F_AB001D_Proc_expl	"Select an application procedure component for creation."

F_AB001E_Frame_desc	"An application frame."
F_AB001F_Procedure_desc	"An application procedure."

F_AB0020_AUFCrt_desc	"A user-specified (4GL) frame."
F_AB0021_ARFCrt_desc	"A report frame."
F_AB0022_AQFCrt_desc	"A QBF (query) frame."
F_AB0023_AGFCrt_desc	"A graph frame."
F_AB0024_A4PCrt_desc	"A 4GL procedure."
F_AB0025_ADBCrt_desc	"A Database procedure."
F_AB0026_ACPCrt_desc	"A C procedure."
F_AB0027_AFPCrt_desc	"A FORTRAN procedure."
F_AB0028_APPCrt_desc	"A Pascal procedure."
F_AB0029_AcPCrt_desc	"A COBOL procedure."
F_AB002A_ABPCrt_desc	"A BASIC procedure."
F_AB002B_AAPCrt_desc	"An Ada procedure."
F_AB002C_A1PCrt_desc	"A PL/1 procedure."

F_AB002D_OtherLang_expl	"Other languages that you may have on your system."
F_AB002E_Other	"OtherLanguages"

F_AB0030_AUFCrt_title	"%0c - Create a USER Frame"
F_AB0031_ARFCrt_title	"%0c - Create a REPORT Frame"
F_AB0032_AQFCrt_title	"%0c - Create a QBF Frame"
F_AB0033_AGFCrt_title	"%0c - Create a GRAPH Frame"
F_AB0034_A4PCrt_title	"%0c - Create a 4GL Procedure"
F_AB0035_ADBCrt_title	"%0c - Create a Database Procedure"
F_AB0036_ACPCrt_title	"%0c - Create a C Procedure"
F_AB0037_AFPCrt_title	"%0c - Create a FORTRAN Procedure"
F_AB0038_APPCrt_title	"%0c - Create a Pascal Procedure"
F_AB0039_AcPCrt_title	"%0c - Create a COBOL Procedure"
F_AB003A_ABPCrt_title	"%0c - Create a BASIC Procedure"
F_AB003B_AAPCrt_title	"%0c - Create an Ada Procedure"
F_AB003C_A1PCrt_title	"%0c - Create a PL/1 Procedure"

/*
** Application Component MoreInfo Titles.
*/
F_AB0040_AUFDet_title	"%s - MoreInfo about a USER Frame"
F_AB0041_ARFDet_title	"%s - MoreInfo about a REPORT Frame"
F_AB0042_AQFDet_title	"%s - MoreInfo about a QBF Frame"
F_AB0043_AGFDet_title	"%s - MoreInfo about a GRAPH Frame"
F_AB0044_A4PDet_title	"%s - MoreInfo about a 4GL Procedure"
F_AB0045_ADBDet_title	"%s - MoreInfo about a Database Procedure"
F_AB0046_ACPDet_title	"%s - MoreInfo about a C Procedure"
F_AB0047_AFPDet_title	"%s - MoreInfo about a FORTRAN Procedure"
F_AB0048_APPDet_title	"%s - MoreInfo about a Pascal Procedure"
F_AB0049_AcPDet_title	"%s - MoreInfo about a COBOL Procedure"
F_AB004A_ABPDet_title	"%s - MoreInfo about a BASIC Procedure"
F_AB004B_AAPDet_title	"%s - MoreInfo about an Ada Procedure"
F_AB004C_A1PDet_title	"%s - MoreInfo about a PL/1 Procedure"
F_AB004D_AGLDet_title	"%0c - MoreInfo about a Global Variable"
F_AB004E_AReDet_title	"%0c - MoreInfo about a Record Definition"
F_AB004F_ARADet_title	"%0c - MoreInfo about a Record Attribute"
F_AB0370_ACODet_title	"%0c - MoreInfo about an Application Constant"

F_AB00C0_VBFDet_title	"%s - MoreInfo about a BROWSE Frame Definition."
F_AB00C1_VMFDet_title	"%s - MoreInfo about a MENU Frame Definition."
F_AB00C2_VAFDet_title	"%s - MoreInfo about an APPEND Frame Definition."
F_AB00C3_VUFDet_title	"%s - MoreInfo about an UPDATE Frame Definition."

/*
**	fatal error - not supposed to happen
*/
S_AB000B_adc_cvinto	"unexpected failure, adc_cvinto"	/* DIAG */
/*
**	abfobjs.c
*/
E_AB000F_Out_of_Memory	"Ran out of memory reading application from catalogs"
/*
**	abftab.c
*/
/*
**	fatal error - not supposed to happen
*/
S_AB001F_IIGNssStartSpace	"bad return from IIGNssStartSpace" /* DIAG */

E_AB0300_InvLanguage
"'%0c' is an invalid language\n\
The II_APPLICATION_LANGUAGE environment logical specified a language not 
supported by INGRES.  The default language '%1c' will be used instead."

/*%
** Explanation:
**	The language specified with the II_APPLICATION_LANGUAGE environment
**	logical is not one supported by INGRES.
**
** System Action:
**	Continue the build with the default (II_LANGUAGE) language instead.
**
** User Action:
**	If the default language is not what was required, exit ABF, set
**	II_APPLICATION_LANGUAGE correctly, and rerun the build.
*/
E_AB0301_NoConstants
"String constant values are not defined for language '%0c'.\n\
The II_APPLICATION_LANGUAGE environment logical specified a language for
which string constant values have not been defined. The default language
'%1c' will be used instead."

/*%
** Explanation:
**      The language specified with the II_APPLICATION_LANGUAGE environment
**      logical does not have values defined for the string constants.
**
** System Action:
**      Continue the build with the default (II_LANGUAGE) language instead.
**
** User Action:
**      If the default language is not what was required, define string
**	constants for the II_APPLICATION_LANGUAGE language and rerun the build.
*/

E_AB0302_NoConstantValues
"Constant '%0c' has no values.\n\
This constant does not have a value assigned to it an any language.  This
makes further processing impossible.  Please assign it a value at least in the
default language."

/*%
** Explanation:
**	This constant has no value in any language.  This is an internal error.
**
** System Action:
**	Exit.
**
**  User Action:
**	Rerun ABF and define values for the constant.
*/

E_AB0303_NewLanguage
"String constant values are not defined for language '%0c'.\n\
The default language contains no values for the string constants.  Instead,
the build will proceed using the language '%1c'."

/*%
** Explanation:
**      The default language does not have values defined for the string 
**	constants.
**
** System Action:
**      Continue the build with a language which does.
**
** User Action:
**      If this language is not what was required, define string
**      constants for the default language and rerun the build.
*/

/*
** Messages supporting the global constants file Create and Edit operations.
*/

E_AB0304_BadGlobFileName	
"Improperly named file. \n\
This menuitem is used to create/edit the file you specified, 
but the named file is not supported as a valid file name. Enter a proper
name."
E_AB0305_ConsFileExists		
"The named file already exists. Please enter a different name."
E_AB0306_LocIsDir               
"Given name is a directory. Please specify a file name."
E_AB0307_CantOpenForWrite	
"Cannot open named file for write.  Check permission and space."
E_AB0308_CantWrite		"Failed to write. Check space."
E_AB0309_BadAppName		"Improperly named application."
E_AB0310_BadObjectCat		"Singleton select on ii_objects failed. \
This may be caused by ii_objects corruption."
E_AB0311_AbfObjJoinFail		"Joined select on ii_objects and ii_abfobjects\
 failed."
E_AB0312_FileDoesnotExist	"Named file does not exist."

/*
**	Menu items and explanations for Component Definition frame(s)
*/
F_AB0050_NewEdit	"NewEdit"
#define AB_NewEdit	F_AB0050_NewEdit
F_AB0051_FormEdit	"FormEdit"
#define AB_FormEdit	F_AB0051_FormEdit

F_AB0052_NewEdit_expl	"Select or create a component for editing."
F_AB0053_Go_expl	"Test this frame."
F_AB0054_GoProc_expl	"Test this procedure."
F_AB0055_Edit4GL_expl	"Edit the 4GL source-code for this frame."
F_AB0056_EditRep_expl	"Edit the report definition for this frame."
F_AB0057_EditQBF_expl	"Run RBF to edit the report for this frame."
F_AB0058_Edit_expl	"Edit the source code for this procedure."
F_AB0059_EditGr_expl	"Run VIGRAPH to define the graph for this frame."
F_AB005A_Compile4GL_expl "Compile the 4GL source-code for this frame."
F_AB005B_CompileRep_expl "Compile the definition of the report in the database."
F_AB005C_Compile_expl	"Compile the source code for this procedure."
F_AB005D_EditForm_expl	"Edit the form for this frame."
F_AB005E_Empty_expl	""	/* DIAG */
F_AB005F_Print_expl	"Print the 4GL source code for this frame."

F_AB0301_Components	"Globals"
#define AB_Parts	F_AB0301_Components
F_AB0302_Comp_title	"%0c - Create or Edit Global Components"

F_AB0303_Globals	"Variables"
#define AB_Globals	F_AB0303_Globals
F_AB0304_Glob_expl	"Create or Edit Global Variables."
F_AB0305_Global		"Variable"

F_AB0314_GloEdit_title	"%0c - Edit Global Variables"
F_AB0315_CreateGlo_expl	"Create a new Global Variable."
F_AB0316_DestroyGlo_expl "Destroy the selected Global Variable."
F_AB0317_EditGlo_expl	"View or edit the selected Global Variable."

F_AB0333_Records	"RecordTypes"
#define AB_Records	F_AB0333_Records
F_AB0334_Rec_expl	"Create or Edit Record Type Definitions."
F_AB0335_Record		"RecordType"

F_AB0336_RecEdit_title	"%0c - Edit Application Record Type Definitions"
F_AB0337_CreateRec_expl	"Create a new Record Type Definition."
F_AB0338_DestroyRec_expl "Destroy the selected Record Definition."
F_AB0339_EditRec_expl	"View or edit the selected Record Definition."

/* Record Attributes */
F_AB0343_Record_atts	"Record Attributes"
#define AB_RAtts	F_AB0343_Record_atts
F_AB0344_RAtt_expl	"Insert or Delete record attributes."
F_AB0345_Record_att	"Record Attribute"

F_AB0347_InsertRAtt_expl	"Insert a new Record Attribute."
F_AB0348_DeleteRAtt_expl	"Delete the selected Record Attribute."
F_AB0349_EditRAtt_expl		"View or edit the selected Record Attribute."

/* Application Constants */

F_AB0355_Constants	"Constants"
#define AB_Consts	F_AB0355_Constants
F_AB0356_Const_expl	"Create or Edit Application Constants."
F_AB0357_Constant	"Constant"
F_AB0358_ConstEdit_title	"%0c - Edit Application Constants"
F_AB0359_CreateConst_expl	"Create a new Application Constant."
F_AB0360_DestroyConst_expl	"Destroy the selected Application Constant."
F_AB0361_EditConst_expl		"View or edit the selected Application Constant."

/*
** Procedure Definition Form Titles.
*/
F_AB0060_A4PDef_title	"%0c - Edit a 4GL Procedure Definition"
F_AB0061_ADBDef_title	"%0c - Edit a Database Procedure Definition"
F_AB0062_ACPDef_title	"%0c - Edit a C Procedure Definition"
F_AB0063_AFPDef_title	"%0c - Edit a FORTRAN Procedure Definition"
F_AB0064_APPDef_title	"%0c - Edit a Pascal Procedure Definition"
F_AB0065_AcPDef_title	"%0c - Edit a COBOL Procedure Definition"
F_AB0066_ABPDef_title	"%0c - Edit a BASIC Procedure Definition"
F_AB0067_AAPDef_title	"%0c - Edit an Ada Procedure Definition"
F_AB0068_A1PDef_title	"%0c - Edit a PL/1 Procedure Definition"
F_AB0069_AGLDef_title	"%0c - Edit a Global Variable Definition"
F_AB006A_AReDef_title	"%0c - Edit a Record Type Definition"
F_AB006B_ARADef_title	"%0c - Edit a Record Attribute Specification"
F_AB006C_ACODef_title	"%0c - Edit a Constant Definition"

F_AB0380_ObjPrint_expl	"Print information about this object."

E_AB0012_NotLibOper	"You cannot %0c the source-code for a library
procedure.\n
ABF does not maintain the source-code for procedures defined as part of a
library.  ABF maintained procedures must have a source-code file name specified
consisting of a name and a valid extension."

/*%
** Explanation:
**	The Edit and Compile menuitems cannot be applied to the source-code for
**	a library procedure.  Library procedures are maintained outside of the
**	ABF environment through other means.  Library procedures do not have a
**	source-code file specified, but are specified to ABF.  This allows
**	these procedures to be called from the ABF RunTime environment.
**
** System Action:
**	None.
**
** User Action:
**	If the source-code for a procedure should be maintained by ABF, the
**	procedure should be specified as not being a library procedure and the
**	full name of the source-code file must be specified in the procedure
**	definition.  The source-code file name consist of "name.ext" where
**	the extension is a supported procedure type.
*/

/*
** NewEdit SubMenu Explanations
*/
F_AB0070_Frame_expl	"Edit an existing frame."
F_AB0071_Procedure_expl	"Edit an existing procedure."
F_AB0072_NewEdit_help	"Specify or Create Object for Edit SubMenu."

F_AB0073_Frame_name	"Frame Name"
F_AB0074_Procedure_name	"Procedure Name"

/*
** Shell MenuItems and Explanations (for 'IIAButilities()'.)
*/
F_AB0075_Shell		"Shell"
F_AB0076_Spawn		"Spawn"

F_AB0077_Shell_expl	"Call a command shell."
F_AB0078_Spawn_expl	"Spawn a sub-process."

/*
** ABF Special Type Names.
*/
F_AB007E_none	"none"		/* DB_NODT */
F_AB007F_string	"string"	/* DB_DYC_TYPE */

/*
** ABF Error Listing Title.
*/
F_AB0080_ErrListTitle	"%s - %s Error Listing for \"%s\""
F_AB0081_WarnListTitle	"%s - %s Warning Listing for \"%s\""

/*
** Menuitems for application report on the utilities menu.
*/
F_AB0082_AppReport	"AppReport"
F_AB0083_AppReport_expl	"Display Application Report"
F_AB0084_AppReport_msg	"Preparing Application Report . . ."

/*
** Menuitems for application reconciliation on the utilities menu.
*/
F_AB0085_Reconcile	"Reconcile"
F_AB0086_Reconcile_expl	"Reconcile Application with Database"
/*
**	gninit.c
*/
/*
**	fatal error - not supposed to happen
*/
S_AB0026_Init_Spaces	"Cannot initialize numbered name space." /* DIAG */
/*
**	abfdep.qc
*/
S_AB0027_abdepupd_fopen	"abdepupd:  Cannot open %0c." /* DIAG */
S_AB0028_abobjget_NULL	"abdepupd:  abobjget returned NULL." /* DIAG */
S_AB0029_abdepupd_no_blank	"abdepupd:getnext:  Cannot find blank in %0c." /* DIAG */
S_AB002A_abdepupd_bad_integer	"abdepupd:getnext:  Bad integer in %0c." /* DIAG */
S_AB002B_abdepprocs_open	"abdepprocs:  Could not open %0c." /* DIAG */
S_AB002C_abdepadd_NULL_pool	"abdepadd:  FElpget returns NULL." /* DIAG */
S_AB002D_FElpcreate_NULL	"FElpcreate returned NULL." /* DIAG */
/*
**	Messages displayed while operation is being done
*/
S_AB0031_Building_Image	"Building Image"
S_AB0034_Exiting	"Exiting application . . ."
/*
** prompts
** arguments to query language prompt are QUEL or SQL.
*/
S_AB0037_Frame_prompt	"Frame to call"
/*
**	errors
*/
E_AB003C_invocation	"ABF not invoked from the operating system. Call to INGRES/MENU disallowed."
E_AB003D_no_file	"You must specify a source file."

E_AB003E_NoRepFile	"Cannot Compile RBF specified report.\n
No report specification file was specified for the named report.  If you wish to
specify a report using a report specification, you must specify a file name."

/*
**	Message strings used in handling exceptions
**	Retain embedded newline characters.
*/
F_AB008C_UserTerminate	" -- Terminating running image at your request."
F_AB008D_Terminating	"\n\nTerminating running image."
F_AB008E_InRunningImage	" in running image."
/*
**	abstatus.c
*/
S_AB0040_Compiling	"Compiling '%0c' . . ."
S_AB0041_BuildingImage	"Building runnable image of %0c . . ."
S_AB0042_CompilingForm	"Adding compiled version of %s to the application"
S_AB0043_Working	"Working . . .\r\n"

/*
** abcatget.c
*/
E_AB0047_CatGet		"Attempted retrieval of application %0c from the ABF
catalogs failed.  ABF will exit at this point because it is impossible
to tell whether the application really exists or not, making creation of
an application with this name an unsafe operation."

E_AB0048_Part_app	"A partial application with name %0c was found.
It will be deleted before continuing"

E_AB0049_NoDir	"No source-code directory has been specified.\n
ABF requires that a source-code be specified before it can continue.  Please
specify a source code directory before proceeding."

/*%
** Explanation:
**	ABF must have access to the source-code for an application's frames and
**	procedures before it can perform certain operations (such as "Image".)
**	This source-code directory must be specified in the labeled field of
**	the Application Definition Form.  An empty specification DOES NOT
**	mean the current directory.
**
** System Status:
**	ABF will not execute any application dependent operations (such as
**	"Go") until a valid source-code directory has been specified.
**
** User Action:
**	Enter a valid source-code directory path in the labeled field of the
**	Application Definition Form.
*/

E_AB004A_BadDir	"The source code directory was not found.\n
The directory, '%0c', could not be accessed.  Please check that the directory
exists and that you have access rights to read it before proceeding.  You can 
enter a new source-code directory by choosing MoreInfo and then Defaults from
the application catalog."

/*%
** Explanation:
**	ABF must have access to the source-code for an application's frames and
**	procedures before it can perform certain operations (such as "Go".)
**	The source-code directory path specified for the application
**	could not be accessed by ABF.  This could be because it does not 
**	exist, or because you do not have access rights to read it.
**
** System Status:
**	ABF will not execute any application dependent operations (such as
**	"Image") until a valid source-code directory has been specified.
**
** User Action:
**	Enter a valid source-code directory path in the labeled field of the
**	Application Definition Form.
*/

E_AB004B_NoWrtDir	"The source-code directory is not writable.\n
The directory, '%0c', is not writable by you.  Please check the permissions on
the directory before proceeding.  You can enter a new source-code directory 
by choosing MoreInfo and then Defaults from the application catalog."

/*%
** Explanation:
**	You must have write access to the source-code directory for an ABF
**	application before you create or edit any of the frames or procedures.
**	The source-code directory specified in the labeled field of the ABF
**	Application Defaults frame does not have any write permissions granted
**	to you.
**
** System Status:
**	ABF will not execute any application create or edit operations (such
**	as "Create") until a writable source-code directory has been
**	specified.
**
** User Action:
**	Check that the source-code directory specified in the labeled field of
**	the Application Definition Form is correct and that you have access
**	rights to write to it.
*/

E_AB004C_FileLength	"Source filename too long.\n
When compiling the source file `%0c', ABF would have to create an object file
named `%1c', which is too long and cannot be allowed.  Please shorten the
source file name `%0c' to avoid compilation errors."

/*
** defapp
*/
E_AB004D_BadDML	"Application with mixed SQL/QUEL frames and procedures.\n
The system database catalogs for this application specify both QUEL and SQL
frames or procedures for this application.  This is not supported and must
be corrected before the application can be used.  Please contact your
Site Technical Contact."

/*
** Name:	IIAB_ERRORS -	ABF Error Messages.
*/

E_AB0100	"Missing or inconsistent ABF symbol table.\n
The ABF symbol table, `%1c', is missing, unreadable, or is otherwise
inconsistent.  Please see your INGRES system manager."

#define	ABBADSYM	E_AB0100	/* bad symbol table file */

/*%
** Explanation:
**	To be able to run applications dynamically, ABF requires a special
**	symbol table file to be present, "abf.stb", found in the "ingres!files"
**	directory (which may be specified by the II_CONFIG environment logical.)
**	This file should be readable, and it will be checked for consistency
**	with the version of ABF that is being run.  If neither of these is true,
**	then this is most likely because the installation is incorrect.
**
** System Status:
**	FATAL.  ABF will exit.
**
** User Action:
**	Please contact your site system manger and check that the INGRES
**	installation is consistent.
*/

E_AB0101	"The INGRES name, ING_ABFDIR, is not defined.\n
The environment logical, ING_ABFDIR, should be set to the path of a directory
where ABF can find or create the object code directory for the database.  Please
see your INGRES system manager."

#define	ABBASEPATH	E_AB0101	/* ING_ABFDIR not defined */

/*%
** Explanation:
**	ABF (and ABFIMAGE) place object code files for an application image,
**	on a per database and per application basis, in a directory that is
**	a sub-tree of a base object code directory.  This base object code
**	directory should be specified by the environment logical, ING_ABFDIR,
**	and should be a directory where ABF can either find or create the object
**	code directory sub-trees for applications.  Either component of the
**	directory sub-tree for an application (that is, "database/application")
**	will be created if it does not exist either when the application is
**	created, or if the application already exists.
**
**	ABF (and ABFIMAGE) require a base object code directory be specified
**	before it can run.  This base object code directory should be specified
**	through the environment logical, ING_ABFDIR, which should be set to the
**	pathname of the base object code directory.  This is usually set on a
**	system-wide basis.
**
** System Status:
**	FATAL.  ABF will exit.
**
** User Action:
**	Please contact your site system manager and inform them that the
**	environment logical, ING_ABFDIR, has not been set.  (It should have
**	been set on a system-wide basis.)  Also, contact your site system
**	manager if it is necessary to find out the pathname of the directory
**	to which you must set this environment logical and how to set it.
*/

E_AB0102	"Non-existent ING_ABFDIR directory.\n
The base object code directory specified by the environment logical, ING_ABFDIR,
does not exist or is not a directory.  The pathname specified by ING_ABFDIR is
`%0c'.  Please see your INGRES systems manager."

#define	ABBASEDIR	E_AB0102	/* ING_ABFDIR does not exist */

/*%
** Explanation:
**	ABF (and ABFIMAGE) require that the base object code directory exist
**	before it can run.  This base object code directory was specified
**	through the environment logical, ING_ABFDIR, which was set to the
**	pathname of this directory.  This path either does not exist or does
**	not specify a directory.
**
**	(For more information on what is a base object code directory, see
**	the explanation for error E_AB0101.)
**
** System Status:
**	FATAL.  ABF will exit.
**
** User Action:
**	Please contact your site system manager and inform them that the
**	pathname specified by the environment logical, ING_ABFDIR, is incorrect.
**	Or, if you set this environment logical yourself, make sure that it
**	correctly specifies the pathname for the base object code directory.
*/

E_AB0103	"Unwritable ING_ABFDIR directory.\n
ABF cannot create directories in the base object code directory specified by
the environment logical, ING_ABFDIR.  The directory specified by ING_ABFDIR
is `%0c'.  Please see your INGRES systems manager and check that you have access
rights and enough disk space to create directories in this directory."

#define	ABBASEWRT	E_AB0103	/* ING_ABFDIR not writable */

/*%
** Explanation:
**	ABF (or ABFIMAGE) will need to create the entire object code directory
**	sub-tree for any applications in the database since the database
**	component of the sub-trees does not exist.  However, directories cannot
**	be created in the base object code directory.  This is most likely
**	because you do not have either access rights or enough disk space
**	available to create directories in this directory.
**
**	(For more information on object code directory sub-trees and the base
**	object code directory, see the explanation for error E_AB0101.)
**
** System Status:
**	FATAL.  ABF will exit.
**
** User Action:
**	Please contact your site system manager and check that you have access
**	rights and enough disk space to create directories in the base
**	object code directory.
*/

E_AB0104	"Non-existent application object code directory.\n
The object code directory for this application, `%1c', in the directory `%0c',
does not exist even though the application has already been created.  ABF will
create this directory and will compile all the source files when the application
is run or an image is created.\n\n
Compiling all the source files may take some time.  If the object code
directory for the application should exist but does not, check that the
environment logical ING_ABFDIR specifies the correct pathname of the
directory in which the object code directory is to be found."

#define	NOOBJDIR	E_AB0104	/* 2 (dir) (file)the objdir deleted */

/*%
** Explanation:
**	When an application already exists, then ABF (and ABFIMAGE) expect the
**	directory sub-tree for the object code of the application to exist and
**	will create it if it does not.  Then, since none of the object code
**	files for the application exists, ABF will compile all the source files
**	that belong to the application when it is run or imaged, which may take
**	some time.  This would occur as it should when an application was copied
**	from one system or database to another.
**
**	However, if the object code directory for the application should exist,
**	then it is likely that the environment logical, ING_ABFDIR, did not
**	specify the correct pathname for the base object code directory that
**	should contain the object code directory sub-tree for the application.
**	This is a user error.
**
**	(For more information on object code directory sub-trees and the base
**	object code directory, see the explanation for error E_AB0101.)
**
** System Status:
**	ABF will create the directory sub-tree and continue execution.
**
** User Action:
**	No action required, unless the base object code directory path was not
**	correctly specified through the environment logical ING_ABFDIR.  Then,
**	exit ABF, set ING_ABFDIR correctly, and restart ABF.
*/

E_AB0105	"Unwritable object code directory.\n
ABF cannot create files in the object code directory, `%0c'.  Please see your
INGRES systems manager and check that you have access rights and enough
disk space to create files in this directory."

#define	ABOBJWRT	E_AB0105	/* object code directory not writable */

/*%
** Explanation:
**	ABF (or ABFIMAGE) cannot create files in the object code directory for
**	the application.  This is most likely because you do not have either
**	access rights or enough disk space available to create files in this
**	directory.  This directory must be writable so that ABF can compile the
**	source files that belong to the application (4GL, EQL/C, compiled forms,
**	etc.) into object code files placed there.
**
**	(For more information on object code directory sub-trees and the base
**	object code directory, see the explanation for error E_AB0101.)
**
** System Status:
**	FATAL at present.  ABF will exit.
**
** User Action:
**	Please contact your site system manager and make sure that you both have
**	access rights to the object code directory and enough disk space is
**	available to you to create files there.
*/

E_AB0106	"Create object code directory error.\n
ABF could not create the directory `%1c' in the directory `%0c'.  Check that
you have enough disk space and access rights to create directories in the
directory `%0c'."

#define	DIRCREATE	E_AB0106	/* 2 (dir)(file) Cannot create */

/*%
** Explanation:
**	ABF (or ABFIMAGE) could not create an object code directory sub-tree
**	component for the application in the database object code directory (or
**	possibly could not create the database directory component in the base
**	object code directory.)  This is most likely because you no longer have
**	enough disk space available to you to create directories in the
**	object code directory sub-tree.  Less likely, but still possible, is
**	that you do not have access rights to create directories in the database
**	object code directory.
**
** System Status:
**	ABF (or ABFIMAGE) will stop whatever action was being performed,
**	which most likely will stop the program.
**
** User Action:
**	Please contact your site system manager and make sure that you have
**	enough disk space available to create directories in the base object
**	code directory.
*/

E_AB0107_LibExists	"`%0c' exists.  The application cannot be created there."

#define E_LibExists	E_AB0107_LibExists

E_AB0108_NoLibRm	"Cannot remove object-code directory, `%0c'.\n
"
#define E_NoLibRm	E_AB0108_NoLibRm

E_AB0110	"The frame `%2c' of the application `%1c' could not be found in the ABF catalogs."
#define	ABNOFRM		E_AB0110	/* 3 routine, app, frm */

E_AB0112	"The frame `%2c' appears in the ABF catalogs twice.  The frame is part of the application `%1c'.  The ABF catalogs are inconsistent and should be fixed."
#define	DUPFRM		E_AB0112	/* 3 routine, app, frm */
E_AB0113	"The frame `%1c' contains an impossible value.  This is a bug and should be reported.  The value in the usage field is `%2c'.  The error occurred in routine `%0c'."
#define	BADUSAGE	E_AB0113	/* 3 routine, frame name, usage value */

E_AB0114	"An operation for the frame `%2c' is missing from the ABF catalogs."
#define	ABNOOP		E_AB0114	/* 3 routine, app, frm */
E_AB0115	"ABF could not complete a retrieve on the ABF system catalogs.  The error occurred in routine `%0c'."
#define	BADREAD		E_AB0115	/* 1 routine */
E_AB0116	"ABF could not complete an append on the ABF system catalogs.  The error occurred in routine `%0c'."
#define	BADAPPEND	E_AB0116	/* 2 routine frm */
E_AB0117	"ABF could not complete a replace on the ABF system catalogs.  The error occurred in routine `%0c'."
#define	BADREPLACE	E_AB0117	/* 2 routine frm */

E_AB0118	"There was an INGRES error while deleting the frame `%0c'."
# define	FRMDEL		E_AB0118

E_AB0119	"There was an INGRES error while deleting the rows for the application `%0c'."
#define	APPDEL		E_AB0119

E_AB0120_CompErrs	"Compilation errors for `%0c'.\n
The compilation of the source file was not completed because errors were
detected in the specification.  Please edit the frame definitions and
correct the errors."

#define E_CompErrs E_AB0120_CompErrs

/*%
** Explanation:
**	One of the programs used to compile a source file returned an error
**	status, most likely because it detected a syntactic or semantic error
**	in the source code, or because the source file does not exist.  Other
**	possible causes might be because you do not have access rights or
**	enough disk space to create files in either the temporary (working)
**	directory or the object code directory for the application.
**
**	Very rarely, these errors might be caused by an internal programming
**	problem in the 4GL code generator, resulting in an error from that
**	program or the C compiler.
**
** System Status:
**	The compilation will be stopped and any "Image" or "Go" operation
**	(ABF) or image build (ABFIMAGE) will not be completed.
**
** User Action:
**	Edit the file and correct any syntactic or semantic errors.  Also, make
**	sure that the you have access rights to both the temporary (working)
**	directory and the object code directory for the application, and that
**	enough disk space is available to you for both directories.  In
**	the unlikely case that errors were detected by the 4GL code generator
**	or the following C compilation, report this problem to your Site
**	Technical Contact.
*/

E_AB0121	"Internal ABF file creation error.\n
ABF could not open or write to the internal file `%0c' in the object code
directory, `%1c', for the application.  Please check that you have access
rights to this directory and the file if it exists.  Also, check that you
have enough disk space to create files in this directory."

# define	NOEXTRACT	E_AB0121

/*%
** Explanation:
**	ABF (or ABFIMAGE) could not open the file used to define the run-time
**	structures that define the application.  This could be for any number
**	of reasons, but most likely is because you do not have access rights to
**	the internal file (if it already exists) or because you have exceeded
**	the disk space available to you in the object code directory for the
**	application.
**
** System Status:
**	The program will not stop, but the "Image" or "Go" operation (ABF)
**	or image build (ABFIMAGE) will be stopped without completing.
**
** User Action:
**	Make sure that the you have access rights to both the internal file, if
**	it exists, and to the object code directory for the application, and
**	also that enough disk space is available to you in that directory.
**	Then, retry the operation before reporting this as a problem to your
**	Site Technical Contact.
*/

E_AB0122	"Internal ABF file compilation error.\n
ABF could not compile the internal file `%0c' in the object code directory,
`%1c', for the application.  Please check that you have enough disk space
to create files in this directory.  Then retry the operation.  If this error
occurs consistently, it may indicate an internal programming problem and should
be reported to your Site Technical Contact."

#define	COMEXTRACT	E_AB0122

/*%
** Explanation:
**	ABF (or ABFIMAGE) could not compile the internal file that defines the
**	run-time structures that define an application.  This could be for
**	any number of reasons, but most likely is because you did not have
**	enough disk space in the object code directory for the application
**	for ABF to either completely write the file or to compile the file into
**	an object code file.
**
**	If this error occurs and disk space or other file or process limitations
**	are known not to be a problem, then this may be evidence of an internal
**	program error in the creation of this file.
**
** System Status:
**	The program will not stop, but the "Image" or "Go" operation (ABF)
**	or image build (ABFIMAGE) will be stopped without completing.
**
** User Action:
**	Make sure that the you have enough disk space available in the
**	object code directory for the application and retry the operation
**	before reporting this as a problem to your Site Technical Contact.
*/

E_AB0123	"Could not dynamically link the application `%0c'.\n
The application could not be dynamically linked by ABF.  Check that the
libraries and object files for the application exist and that you have
access rights to read them."

#define	ABNOLINK	E_AB0123

/*%
** Explanation:
**	ABF could not link the application to be run dynamically.  This is most
**	likely an operating system error, possibly because the link (or load)
**	command could not find or read a required library or object file.  Or,
**	another possible reason is that you have exceeded the disk space that is
**	available to you in the temporary (or working) directory or you do not
**	have access rights to that directory.
**
** System Status:
**	ABF will not run the application.
**
** User Action:
**	Make sure that all the libraries and object files exist and are readable
**	by you.  Also, make sure that the you both have access rights to the
**	temporary directory and that enough disk space is available to you
**	there.  Then, retry the operation before reporting this as a problem to
**	your Site Technical Contact.
*/

E_AB0124	"Application build compilation errors.\n
Compilation errors were found for at least one of the source files for the
application.  These problems must be corrected before the application can be run
or before an application image can be built."

#define	COMIMAGE	E_AB0124

E_AB0125	"ABF ran out of memory while creating an internal table.  The error occurred in routine `%0c'."
#define	NOTAB		E_AB0125
E_AB0126	"Some tables needed by ABF could not be created and thus the application could not be run."
#define	TABIMAGE	E_AB0126

E_AB0127_NoRun	"ABF could not run the application because of errors."

E_AB0128_NoConstsGen	"Could not generate constants images.\n
ABF could not generate the constants image files in the work directory for the
database (under ING_ABFDIR).  Please check that you have access rights to this
directory and the language-specific directories under it, and that have enough
disk space to create files in this directory tree."

#define E_NoConstsGen	E_AB0128_NoConstsGen

/*%
** Explanation:
**	ABF could not generate the pre-6.5 language-dependent constants image 
**	files that are used by the application image when run in a  backward
**	compatibility mode (i.e. logical II_APPLICATION_LANGUAGE is defined).  
**	This is because an error occurred when one of these language-dependent 
**	constants files was created or written; these errors can occur only 
**	when you do not have access-rights to the ABF database work directory 
**	or language directories below it, or you have exceeded the disk space 
**	available to you there.
**	
** System Status:
**	ABF will continue with the application build, but some of the language-
**	dependent constants image files necessary to execute the application
**	for different languages will not exist.
**
** User Action:
**	The best action is to begin to use the new method for specifying 
**	alternate sets of Global Constants, using user-maintained Global
**	Constant Files, as described in the INGRES/ABF Reference Manual.
**	If you need to continue to use the old method of ABF generated 
**	language-dependent constants image files, then make sure that you 
**	have access rights to the ABF work directory for the database and 
**	to the language specific directories below it, and that you have
**	sufficient disk space available there to create and write the
**	constants image files.  Remember that this method will not be 
**	supported in the next major release of INGRES/ABF or INGRES/VISION.
*/

E_AB0129_InvAppLang 	"'%0c' is an invalid language.\n
The II_APPLICATION_LANGUAGE environment logical specified a language not 
supported by INGRES. The II_APPLICATION_LANGUAGE environment logical
will be ignored, and the default constant values will be used."

/*%
** Explanation:
**	The language specified with the II_APPLICATION_LANGUAGE environment
**	logical is not one supported by INGRES. This environment logical
**	is only supported for backward compatibility with pre-6.5 releases
**	of INGRES/ABF and INGRES/VISION.  Support will be discontinued in
**	a future release.
**
** System Action:
**	Continue the build, ignoring the II_APPLICATION_LANGUAGE setting,
**	and using the default constant values instead.
**
** User Action:
**	If you must use the backward compatibility mode, exit back to the 
**	operating system, reset II_APPLICATION_LANGUAGE correctly, and rerun 
**	the build.  Since support for this option will be discontinued
**	in a future release, the best user action is to begin using the new
**	method for specifying alternate sets of global constants using
**	user-maintained Global Constant Files; please consult the INGRES/ABF
**	Reference Manual for more information.
*/

E_AB012A_AppLangUsed 	"Warning: II_APPLICATION_LANGUAGE will not be supported in the next major release of INGRES/ABF or INGRES/VISION.\n
The build will continue using the set of global constant values corresponding
to the language '%0c', which is the current setting of II_APPLICATION_LANGUAGE."

/*%
** Explanation:
**	The II_APPLICATION_LANGUAGE environment logical is set to a valid
**	natural language, which indicates that global constants corresponding 
**	to this language will be used during the build instead of the default 
**	set of constant values.  This was the supported method for specifying 
**	alternate sets of global constant values prior to release 6.5.  
**	Currently, support for this environment logical exists for backward
**	compatibility with pre-6.5 releases of INGRES/ABF and INGRES/VISION
**	only, and will be discontinued in a future release.
**
** System Action:
**	Continue the build, using the set of global constants corresponding to 
**	the II_APPLICATION_LANGUAGE setting if they are defined.
**
** User Action:
**	For now, no action is required.  In future releases of INGRES/ABF
**	and INGRES/VISION, you will need to use the new method for specifying 
**	alternate sets of global constants using user-maintained Global 
**	Constant Files; please consult the INGRES/ABF Reference Manual for 
**	more information.
*/

E_AB012B_TypeChange 	"Cannot change the type of a Global Constant.\n
The data type of the Global Constant may not be changed once it has been 
created; only the length may be modified."

/*%
** Explanation:
**	During an "Edit a Global Constant" operation, the type of the constant
**	(char, varchar, integer, smallint, float, or decimal) may not be
**	changed.  Only the length may be altered during the edit operation.
**
** System Action:
**	Allow editing to resume, after restoring previous data type value.
**
** User Action:
**	Continue editing the Global Constant definition.  If the data type
**	must be changed, you must destroy the constant and re-create it.
*/

E_AB012E_NoFormForm		"Missing or unreadable form `%0c'.\n
ABF can not open the form `%0c'.  You must create this form before
application '%1c' can be run or an image of it can be built."
#define E_NoFormForm E_AB012E_NoFormForm

E_AB012F_NoForm		"Missing or unreadable form `%0c'.\n
ABF can not open the form `%0c'.  You must create this form before
frame '%1c' can be compiled."
#define E_NoFrameForm E_AB012F_NoForm

E_AB0130_NoSrcFile	"Missing or unreadable source file `%1c'.\n
ABF can not open the source file `%1c' in the directory `%0c'.  Please make
sure that the file exists and that you have access rights to read it."

#define E_NoSrcFile E_AB0130_NoSrcFile

E_AB0230_NoCompForm	"Missing or unreadable compiled form file `%1c'.\n
ABF can not read the compiled form file `%1c'.  Please make
sure that the file exists and that you have access rights to read it."

E_AB0131_BadFrame	"The name `%0c' is not a valid name for a frame.\n
Frame names must begin with either an underscore `_' or a letter, and can
consist of letters, digits, and the characters _, $, @, and #, and can be no
more than %1d characters in length."


E_AB0132_NoForm	"You must specify a form before calling VIFRED."

E_AB0133_BadProcedure	"The name `%0c' is not a valid name for a procedure.\n
Procedure names must begin with either an underscore `_' or a letter, and can
consist of letters, digits, and the characters _, $, @, and #, and can be no
more than %1d characters in length."

E_AB0134_UnknownExt	"Unknown source-code file extension.\n
The extension for the file `%0c' is not recognized by ABF.  Please refer to
the INGRES ABF/4GL Reference Manual for valid file extensions."

E_AB0135_IllegalExt	"Illegal file extension for source-code language.\n
The extension for the file `%0c' is not a legal one for the source-code
language specified for this frame or procedure.  Please refer to the INGRES
ABF/4GL Reference Manual for valid file extensions."

E_AB0136_UnsupportedExt	"Unsupported file extension.\n
This system does not support the extension for the file `%0c'.
%1c"

E_AB0137_UnknownObject	"Unknown %0c name:  `%1c'\n
No application component with the name `%1c' can be found for this application."

/*%
** Explanation:
**	The name you specified for the new (different) application component
**	that you want to edit is not the name of any of the components of this
**	application.
**
** System Status:
**	ABF will return to the previous application component edit frame.
**
** User Action:
**	Specify the correct name when prompted after again selecting the
**	NewEdit menuitem and the component type menuitem.  If you are
**	unsure of the correct name, return to the Edit Application frame
**	and examine the names displayed in the table field.
*/

E_AB0138_IllegalFileName	"Illegal file name: `%0c'\n
The specified name is not recognized as a legal file name on this system
for the source-code directory `%1c'.
%2c"

/*%
** Explanation:
**	The given name was not recognized as a legal file name on this system
**	for the application source-code directory although it may be a legal
**	path name.  ABF expects only a file name (or a relative path name) in
**	this context, not a full path name.  File names generally consist of a
**	name and an extension, while path names specify the full directory path
**	for the file.
**
** System Status:
**	ABF will not continue until a legal file name is specified.
**
** User Action:
**	Specify a legal file name.
*/

E_AB0139_UnsupportedLang	"Unsupported host-language.\n
This system does not support the host-language specified for this procedure,
`%0c'.  The application cannot be compiled or executed on this system."

/*%
** Explanation:
**	The host-language specified for a procedure is not supported on the
**	system where the application is being executed or compiled.  This
**	application was defined on a system that did support the host-language,
**	but was then accessed (through INGRES/Net) or copied (using COPYAPP) on
**	a system that does not support it.  This is not supported.
**
** User Action:
**	Destroy the procedure and recreate it for a host-language supported on
**	both systems.  Then, rewrite the source-code for the procedure in this
**	common host-language.
*/

E_AB0150_NotOwner       "You are not the owner of %0c '%1c'."

/*%
** Explanation:
**      You are attempting to rename an abf application which you do not own.
**
** System Action:
**      Returns to the ABF applications catalog menu
*/

S_AB0151_Enter_new_name_prompt  "Enter new name: "

/*%
** Explanation:
**      Prompt given when renaming an ABF application.
*/

E_AB0152_Not_a_legal_name       "'%0c' is not a legal %1c name."

/*%
** Explanation:
**      You are attempting to rename an ABF application to an illegal name.
**
** System Action:
**      Returns to the ABF applications catalog menu
*/

S_AB0153_Rename_status          "Renaming %0c '%1c' to '%2c' . . ."

/*%
** Renaming application abc to xyz
*/

/* Interpreter specific, not yet defined */
# define	NOHOSTPROC	E_AB0182	/* Can't call host lang procs */
# define	RTTMEM		E_AB0183	/* Couldn't allocate rtt */
						/*
						** (err from abrtadef)
						*/
# define	RTTFILE		E_AB0184	/* Couldn't create timg file */
						/*
						** (err from IIOItcTstfilCreate)
						*/
/*
** NOTE:
**	using bias of 200 to convert old fast messages to slow.
*/
/*
**	abret.c.
*/
E_AB0264_BadConstValue	"Illegal constant value for type: `%0c'.\n
The value specified for this constant is not legal for a constant of type `%1c':
`%0c' is not a legal `%1c'."

/*%
** Explanation:
**	The value specified for the constant is not legal for the type of the
**	constant.  For example, "xyz" is not a legal integer, float, or decimal
**	number.  The value must be legal for the type.
*/

E_AB0265_BadConstType	"Illegal value for constant data type: `%0c'.\n
The data type must be a legal INGRES type for an application constant.  Select
ListChoices to get a list of legal application constant types."

/*%
** User Action:
**	Please refer to the INGRES ABF Reference Manual for legal application
**	constant types.  Or, select ListChoices from the menu to get a list of
**	legal INGRES types.
*/

S_AB0266_IgnCharLen	"Length for '%0c' type will be ignored.\n
Lengths for character constants are determined by the constant value and should
not be specified as part of the data type."

/*%
** Explanation:
**	The length of a character constant is determined by its value since
**	that can change for different languages.  For example, "Hello" in
**	English and "Bonjour" in French.  You should not specify a length
**	in the type specification for a constant.
**
** System Status:
**	ABF will ignore the length in the type specification and replace it
**	with a non-length specific type specification.
**
** User Action:
**	None.
*/

E_AB0263_RecRecursion	"Illegal record attribute type.\n
Record type '%0c' contains record type '%1c' either directly or indirectly.  It
is illegal to define an attribute of type '%0c' as a member of type '%1c' since
it would lead to a record that contains itself."
/*%
** Explanation:
**      Recursive definitions of record types are not allowed.  That is, a
**      record attribute cannot contain the record being defined.  Legal record
**	attribute types are any legal INGRES type or any other record type
**	defined for this application.
**
** System Status:
**      ABF will resume to the type field where you must enter a legal type
**      for the record attribute.
**
** User Action:
**      Please refer to the INGRES Reference Manual for legal INGRES types.  Or,**      select ListChoices from the menu to get a list of legal INGRES types.
*/

/*
** NOTE:
**	using bias of 200 to convert old fast messages to slow.
*/
/*
**	abret.c.
*/
E_AB0267_RecAttr	"Illegal record attribute type.\n
The data type specified for a record attribute cannot be the record type,
itself; it must be a legal INGRES type or a record type defined for the
application other than the record type being defined.  Select ListChoices to
get a list of legal INGRES types."

/*%
** Explanation:
**	Recursive definitions of record types are not allowed.  That is, a
**	record attribute cannot be of the same type as the record being defined.
**	Legal record attribute types are any legal INGRES type or any record
**	type defined for this application that does not contain the current
**	record either directly or indirectly.
**
** System Status:
**	ABF will resume to the type field where you must enter a legal type
**	for the record attribute.
**
** User Action:
**	Please refer to the INGRES Reference Manual for legal INGRES types.  Or,
**	select ListChoices from the menu to get a list of legal INGRES types.
*/

E_AB0268_Ret_type	"Illegal value for return type.\n
The data type specified must be a legal INGRES type or `none'.  Select
ListChoices to get a list of legal INGRES types."

/*%
** User Action:
**	Please refer to the INGRES Reference Manual for legal INGRES types.  Or,
**	select ListChoices from the menu to get a list of legal INGRES types.
*/

E_AB0269_Data_type	"Illegal value for data type: `%0c'.\n
The data type must be a legal INGRES type or any other record type defined for
the application (other than the one being defined).  Select ListChoices to get
a list of legal INGRES types."

/*%
** Explanation:
**	Record attributes can only be types that are legal INGRES types or
**	are record types already defined for the application (other than the
**	record type being defined).  The type entered for a record attribute
**	was not recognized as specifying one of these legal types.
**
** System Status:
**	ABF will resume to the type field where you must enter a legal type
**	for the record attribute.
**
** User Action:
**	Please refer to the INGRES Reference Manual for legal INGRES types.  Or,
**	select ListChoices from the menu to get a list of legal INGRES types.
*/

E_AB026C_Not_int_type	"Illegal value for data type: `%0c'.\n
The data type must be an INGRES integer type or `none'.  Select ListChoices to
get a list of legal database procedure return types."

/*%
** Explanation:
**	Database procedures can only return a single-valued integer as a result,
**	or none.  The data type specified as the return type for the database
**	procedure is not an integer.
**
** System Status:
**	ABF will resume to the type field where you must enter a legal type
**	for the return type of the database procedure, or "none" if the it
**	does not return a value.
**
** User Action:
**	Please refer to the INGRES Reference Manual for legal INGRES types.
**	Or, select ListChoices from the menu to get a list of legal database
**	procedure return types.
*/

E_AB036A_Bad_PassDecimal "Illegal type for passing decimal parameters: `%0c'.\n
Decimal parameters can only be passed as float or decimal; please correct
this specification."

E_AB036B_Bad_SymbolName "Illegal Symbol name: `%0c'.\n
The symbol name must not contain blanks.  This name is used by the linker
to resolve references to this Host-Language Procedure within your application."

E_AB036C_Hproc_type	"Illegal value for data type: `%0c'.\n
The data type must be a legal INGRES type for host-language procedures
or `none'.  Please refer to the INGRES ABF/4GL Reference Manual for legal
host-language procedure return types."

E_AB026A_OSL_extension	"File extension must specify 4GL source (.osl or .osq.)"

E_AB026B_ObjConflict	"Source file name conflict with other object names.\n
The file name `%0c' is identical exclusive of its extension with other source
files that belong to the application.  This cannot be allowed since the object
code files for these source file names will conflict by having the same file
name and extension."

E_AB0281_PathName 	"Source file name cannot be a pathname.\n
A fully specified filename is not allowed here. All the source code files are
in the source directory `%0c'."

/*
** abfrun.qsc
*/

E_AB026D_rtt	"Could not build internal data file `%0c'.\n
ABF could not build an internal data file that it uses to pass information to
the interpreter.  Please check that you have access rights to the temporary
(working) directory and the file if it exists.  Also, check that you have
enough disk space to create files in this directory."

/*%
** Explanation:
**	ABF could not build a temporary file used to pass runtime information
**	to the interpreter.  This is most likely because you have exceeded
**	the disk space that is available to you in the temporary (or working)
**	directory or you do not have access rights to that directory.
**
**	Very unlikely, but possible, this error will indicate that ABF could
**	not build the file because of an internal error.
**
** System Status:
**	ABF will not proceed with "Go" (test) operation.
**
** User Action:
**	Make sure that the you both have access rights to the temporary
**	directory and that enough disk space is available to you there.
**	Then, retry the operation before reporting this as a problem to
**	your Site Technical Contact.
*/

S_AB0158_RunHostLangProc	"Warning: Running host-language procedure.\n
Running a host-language procedure may cause unexpected results since using
parameters, if there are any, in the called procedure requires assigned
addresses which are not available until the calling routine is invoked."

F_AB00BC_Title		"Do you really want to continue?"
F_AB00BD_Yestext	"Continue"
F_AB00BE_Notext		"Quit"
F_AB00BF_CHelp		"Running host-language procedure confirmation"

/*
** ablink.c
*/

S_AB026E_linking	"Linking host language procedures . . ."

F_AB0091_NewTop		"Creating a New Starting Frame"
F_AB0092_CrVision	"Creating a Starting Frame for a New Application"

S_AB0272_TopDestroy	"Warning: destroying top frame.\n
You are about to destroy the starting frame of an INGRES/Vision
application.  If you do this, a new starting frame will be required
to enter the Application Flow diagram, and that frame will initially have
no menuitems."

S_AB0273_ChildDestroy	"Warning: called by frame `%0c'.\n
You are about to destroy a frame or procedure called by choosing a menuitem
from an INGRES/Vision frame.  If you do this, the corresponding menuitem
will be deleted from the calling frame."


F_AB0094_USER		"%0c - Edit a USER Frame Definition."
F_AB0095_BROWSE 	"%0c - Edit a BROWSE Frame Definition."
F_AB0096_MENU		"%0c - Edit a MENU Frame Definition."
F_AB0097_APPEND		"%0c - Edit an APPEND Frame Definition."
F_AB0098_UPDATE		"%0c - Edit an UPDATE Frame Definition."

F_AB009B_ATMFCrt_desc	"A top-level menu frame."
F_AB009C_ATMFCrt_title	"%0c - Create a Top-Level MENU Frame"

S_AB0274_Changing	"Changing frame to new type . . ."

S_AB0275_Regenerate	"Do you wish to regenerate 4GL code for '%0c'?"
S_AB0276_Form		"Rewriting previous form for '%0c' . . ."
S_AB0277_Code		"Generating 4GL code for '%0c' . . ."

S_AB0278_Failed		"Compilation failures.\n
Some frames or procedures in this application failed to compile.  You can either
fix the errors or continue with the test execution.  However, until the errors
are fixed, you will not be able to call any of the frames or procedures that
failed to compile."

S_AB0021_SevereFailed		"Severe compilation failures.\n
Some frames or procedures in this application failed to compile, including some
which are necessary for the test execution.  The test execution cannot proceed."

S_AB0279_Proceed	"Proceed with test execution?"

S_AB027A_Checking	"Checking dates . . .\n\n"

E_AB027B_BadGen		"Cannot generate the form or 4GL.\n
The definition for frame '%0c' contains errors that make generation
impossible.  You can fix the errors by editing the frame's Visual Query."

E_AB027C_BadComp	"Previous compilation failure.\n
Frame '%0c' previously failed to compile, and the source code does not
appear to have been modified since that date (%1c)."

S_AB027D_BCPrompt	"Do you wish to continue with compilation?"

S_AB027E_Regenerate	"Do you wish to regenerate form for menu frame '%0c'?"

E_AB027F_BadLanguage	"'%0c' is not a valid language!"

E_AB0280_BadConstCopy	"Cannot copy constant values to language '%0c'.\n'%1c'"

F_AB009D_Nullable	"Nullable"
F_AB009E_Array		"Array"

E_AB026F_Rename	"Could not remove old object code directory.\n
The old object code directory, '%0c', could not be removed,
although a new one reflecting the application's new name was created.
Normal operations may take place for this application, but problems will
be experienced should you try to create a new application with the
name '%1c', or rename an existing one to '%1c'."

/*%
** Explanation:
**	When handling a rename operation, we were able to create a new
**	object code directory, but not able to remove the old one.
**
** System Status:
**	Rename operation is treated as successful, but future new applications
**	with the old name will experience difficulties because the object
**	code directory will already exist.
**
** User Action:
**	Remove the directory
*/

F_AB00A0_Refresh	"Refresh"
F_AB00A1_OkExpl	"Edit, even though there is another user"
F_AB00A2_CanExpl	"Cancel edit operation"
F_AB00A3_ELockHelp	"Edit lock warning"
F_AB00A4_RefExpl	"Refresh list from database"
F_AB00A5_RCHelp	"Application flow refresh confirmation."

F_AB00A6_RTitle	"\nApplication flow may have been changed by another user.\nDo
you wish to refresh this information?\n"
F_AB00A7_Yexp	"Refresh."
F_AB00A8_Nexp	"Do not refresh."
F_AB00A9_LokTitle	"%0c - Lock administration."
F_AB00AA_Lock	"lock"
F_AB00AB_LDHelp	"Delete lock confirmation."
F_AB00AC_LockMenu	"LockStatus"
F_AB00AD_LockExpl	"Show what users are editing applications"
F_AB00AE_LDetHelp	"Lock details"
F_AB00AF_CleanMenu	"Cleanup"
F_AB00B0_CleanExpl	"Execute cleanup operation on database"
F_AB00B1_LockDestHelp	"Destroy / remove lock warning"

F_AB00B2_CatMsg	"Retrieving Application catalogs . . ."

/*
** this is a duplicate of the OO message so we can put this out when
** we are doing lengthy retrieves BEFORE th OO message is printed.
*/

F_AB00B3_LockInfo	"Currently being edited by another user.  %s
\nChoose OK to continue editing, or Cancel otherwise."

F_AB00B4_ObjectLocked	"If you\ncontinue with your operation, you should be aware\nthat changes you make will overwrite any made by\nthe other user.\n"

F_AB00B5_AppLocked	"If you\ncontinue with your operation, you should be aware\nthat removal or renaming of frames may affect\nother users.\n"

F_AB00B6_VappLocked	"If you\ncontinue with your operation, you should be aware\nthat modification of the application flow diagram\nmay affect other users.\n"

E_AB00A0_LockSet	"Can't update lock status.\n
Cannot enter updated lock information into the database.  You may continue with
your session, but other users will not know that you are editing anything.
Please contact your DBA"

/*%
** Explanation:
**	DB query to update locks table failed.  There will likely have
**	been a lower level message describing the DB failure.
**
** System Status:
**	abf will continue.
**
** User action:
**	Continue, but be aware that status concerning concurrency is
**	not being properly maintained.
*/

S_AB00A1_LockRead	"Reading lock information . . ."
S_AB00A2_LockDelete	"Deleting lock information . . ."

E_AB00A3_LockFetch	"Can't obtain lock information.\n
Cannot get information about lock status from the database.  You may
continue with your session, but there may be other users working on
the same application.  Please contact your DBA."

/*%
** Explanation:
**	DB query to obtain lock status failed.  There will likely have
**	been a lower level message describing the DB failure.
**
** System Status:
**	abf will continue.
**
** User action:
**	Continue, but be aware that status concerning concurrent users is
**	unknown.  If user knows that noone else is doing anything with the
**	application, this message may be safely ignored.
*/

E_AB00A4_DestLock	"%0c is being edited.\n
Currently being edited by another user (%1c, since %2c).  You may not destroy
it, as that user's updates would then fail.  There may be more than one other
user.  Edit, or examine LockStatus to obtain the entire list.  If lock
information is erroneous, and corresponds to a session belonging to you,
you may delete the lock.  Otherwise, the DBA may release the lock."

/*%
** Explanation:
**	User is attempting to delete something currently being edited by
**	another user.
**
** System Status:
**	abf will not allow it.
**
** User action:
**	Coordinate with the other user.  If lock is erroneous, the DBA may
**	remove the lock by using "LockStatus" under "Utilities".
*/

E_AB00A5_NoLocks	"No locks are displayed."

/*%
** Explanation:
**	User is attempting to show details or delete locks from an
**	empty display.
**
** System status:
**	Nothing done.
**
** User action:
**	nothing.  We show an empty tablefield so that the user can
**	use the "refresh" item to monitor the status, but as long as
**	there are no rows in the display, there is nothing to delete.
*/

E_AB00A6_OwnLock	"You may not delete your own session.\n
The row you are positioned on is the one corresponding to your own edit
session.  You may not delete locks corresponding to that session."

/*%
** Explanation:
**	User is attempting to delete lock corresponding to their own
**	edit session on a particular application.
**
** System status:
**	Nothing done.
**
** User action:
**	nothing.  Operation is prevented.
*/

F_AB00B7_CTitle	"\nThis operation involves changes to many system tables,\nand
may take a long time to complete.\n\nDo you wish to perform cleanup?\n"
F_AB00B8_Yexp	"Cleanup."
F_AB00B9_Nexp	"Do not cleanup."
F_AB00BA_Cmessage	"Performing cleanup . . ."
F_AB00BB_CHelp	"Cleanup confirmation."

E_AB00A7_CErrs	"Failed queries.\n
One or more of the cleanup queries failed.  %0d rows were removed
by queries that did not fail."

/*%
** Explanation:
**	User has attempted to clean up FE catalogs, and some of the queries
**	failed.  There should have been DB error messages preceding this.
**
** System status:
**	Something may still need to be cleaned up.
**
** User action:
**	Address the DB failures.
*/

S_AB00A8_CNone	"Nothing to clean up.\n
Cleanup queries were run, but no rows were deleted from system tables."

S_AB00A9_CWork	"Cleanup finished - %0d rows removed."

S_AB00AA_Form	"Updating previously generated form for '%0c' . . ."
S_AB00AB_ReForm	"Creating form for '%0c' . . ."

/*	Global Constants File screen
**
*/

F_AB00E0_ConsFile_title		"%0c - Specify a Global Constants File"
F_AB00E1_ConsFile_menu		"ConstantsFile"
F_AB00E2_ConsFile_expl		"Specify a Global Constants File"
F_AB00E3_Create_ConsFile_expl	"Create a new Global Constants File"
F_AB00E4_Edit_ConsFile_expl	"Edit the specified Global Constants File"
F_AB00E5_End_ConsFile_expl	"End; the specified Global Constants File will be used in future \"Go\" and \"Image\" operations"
F_AB00E6_Create_ConsFile_msg	"Creating Constants File . . ."
F_AB00E7_Cons_Note		"You've given Constant %0c\n\
but the datatype you've specified is '%1c'. This value\n\
won't fit in that datatype. Do you want to expand the\n\
datatype to accommodate the value?\n"
F_AB00E8_Cons_Yes		"Expand the datatype to accommodate the value"
F_AB00E9_Cons_No		"Truncate the value to fit the datatype"
F_AB00EA_Cons_Hlp		"Constant Datatype Expansion Confirmation"
F_AB002F_ConstFileHlp		"ABF Global Constant File"

/* Strings written to global constant files */
F_AB00EB_ConsFile_Str1		"# PM readable Global Constant File\n"
F_AB00EC_ConsFile_Str2		"# Application name:"
F_AB00ED_ConsFile_Str3		"# Number of Global Constants:"
F_AB00EE_ConsFile_Begin		"#BEGIN"
F_AB00EF_ConsFile_End		"#END"

/*
**	compilation/link display
*/
F_AB00FF_BuildTitle		"%0c - Build Application Image"
F_AB0100_BuildAppl		"Building application '%0c'"
F_AB0101_BlankLine		"\n"

S_AB0010_ListFileSep		"\n----------\n"
S_AB0011_Link_Banner		"Building an application"
S_AB0013_CompFailed		"Compilation failed.\n\n"
S_AB0014_CompWorked		"Compilation succeeded.\n\n"
S_AB002F_CompWorkedWarnings	"Compilation succeeded, with warnings.\n\n"
S_AB0015_NoComp			"Compilation was not performed.\n\n"
S_AB0016_Processing		"Processing %0c '%1c' . . ."
S_AB0017_CompExtFile		"Compiling extract file . . ."
S_AB0018_ImageWorked		"The image was built successfully."
S_AB0019_ImageFailed		"The image build failed."
S_AB001A_TstBldWorked		"The test can proceed."
S_AB001B_TstBldFailed		"The test cannot proceed."
S_AB001C_EndtoExit		"Choose '%0c' to end the build session."
S_AB0030_GoToTest		"Choose '%0c' to proceed with the test."
S_AB001D_BadCompile		
"The user has chosen not to recompile.\n\n"
S_AB0022_LibraryProc		"Library procedures do not require compilation."
S_AB0023_NewerObjFile
"Not recompiling because the object file is newer than the source file."
S_AB0024_NoFID
"Forcing compilation to create internal object code."
S_AB0025_NewFID
"Not recompiling because the object code is newer than the source file."
S_AB002E_NewForm
"Recompiling because the form is newer than the object code."
S_AB0140_ForceReport
"Forcing compilation to create report."
S_AB0141_DoReport
"Recompiling because the report file is newer than the database report."
S_AB0142_OldIL
"Recompiling because the object code is older than the source file."
S_AB0143_NoObject
"Forcing compilation to create object file."
S_AB0144_OldObject
"Recompiling because the object file is older than the source file."
S_AB0145_OldObject
"Recompiling because the object file is older than the database object code."
S_AB0146_ReturnChange
"Recompiling because a called frame's return type has changed."
S_AB0147_GlobChange
"Recompiling because a global definition has changed."
S_AB0148_FrameChange
"Recompiling because the frame's definition has changed."
S_AB0149_OldObject
"Recompiling because the object file is older than the form."
S_AB0156_NewReturnChange
"Recompiling because a called %0c's return type has changed."
S_AB0157_NewFrameChange
"Recompiling because the %0c's definition has changed."

S_AB02F4_CompFailure	
"Severe compilation errors.  The status returned was '%0d'."
S_AB02F5_CustomVQchanges
"Changes to this frame's Visual Query would ordinarily cause its 4GL code
to be regenerated, but this frame's 4GL has been customized, and will not be
overwritten.  To regenerate the 4GL code, you must do a '%0c' operation."

E_AB001E_CompFailure
"Compilation failed\n
The compilation of '%0c' failed with status '%1d'.  This could mean either
that the compiler could not be run or that it ran and returned an error."

E_AB0020_LinkFailure
"Link failed\n
The link failed with status '%0d'.  This could mean either
that the linker could not be run or that it ran and returned an error."

F_AB02F0_CustRegenHelp	"Custom Frame Edit Confirmation"
 
F_AB02F1_CustRegenTitle  "\
Frame '%1c' has been customized by having\n\
its 4GL source file edited. Since '%1c' has been\n\
customized, 4GL code for it cannot be regenerated\n\
without losing those customizations.\n\
\n\
Do you wish to keep the customizations instead of\n\
regenerating the 4GL code?\n"
  
F_AB02F2_CustRegenYes	"Regenerate the 4GL code"
F_AB02F3_CustRegenNo	"Keep the customizations"

E_AB02F6_WrongDML	"Warning:  QUEL application.\n
You cannot edit QUEL based applications through the Application Flow Diagram
interface of Vision; you will be placed directly on the ABF Edit an
Application frame."

/*%
** Explanation:
**	You are using Vision on an application whose default query language
**	is QUEL.  This is not supported since Vision generated frame execute
**	exclusively with SQL transaction semantics.
**
** System status:
**	Vision will allow you to edit the application only through ABF and
**	will place you directly on the ABF Edit an Application frame if you
**	are authorized to use ABF.
**
** User action:
**	Edit as an ABF application.
*/

E_AB02F7_LockOwner	"Cannot delete.\n
In order to delete locks for a session, you must either be DBA,
or be deleting locks for another of your own sessions."

/*%
** Explanation:
**	User is attempting to delete a lock that does not belong to them,
**	and they are not DBA.
**
** System status:
**	Nothing done.
**
** User action:
**	Enter as DBA or other user to delete lock, or get DBA or other
**	user to do it.
*/

S_AB02F8_NoLocks	"All locks have been deleted.\n
Your operation will now proceed."

F_AB02F4_MRegenTitle	
"The form for menu frame `%1c' has been modified,\n\
but that frame is now out of date.  If the form\n\
is regenerated, possibly reflecting new menu\n\
items, those changes will be lost.\n\
\n\
Do you wish to keep the current form instead of\n\
generating a new form?\n"

F_AB02F5_MRegenHelp	"Menu Frame Form Regeneration Confirmation"
F_AB02F6_MRegenYes	"Regenerate the form"
F_AB02F7_MRegenNo	"Keep the existing form"

/*
** abpaswrd.qsc
*/
F_AB02A0_EnterPassword	"Enter password for role '%0c'"
F_AB02A1_VerifyPassword	"Verify password for role '%0c'"
F_AB02A2_PwdOKexpl	"Accept password just typed"

F_AB02A3_PrevPwdHelp	"Reusing previous password"
F_AB02A4_PrevPwdTitle	"Reuse previous password for role '%1c' ?"
F_AB02A5_PrevPwdYes	"Reuse previous password"
F_AB02A6_PrevPwdNo	"Enter new password"

S_AB02A0_PwdHelpBanner	"Entering role password"
E_AB02A1_PwdBadVerify	
"Password verification failed\n
You have typed different strings when first entering the password
and when verifying it.  Please re-enter the password."

/*%
** Explanation:
**	User typed different strings when entering and verifying the password.
**	and they are not DBA.
**
** System status:
**	The user is asked to re-enter the password.
**
** User action:
**	Re-enter and verify the password.
*/

/*
** defaults.qsc
*/
F_AB00D0_YesMakeComp	"Make all frames use compiled forms"
F_AB00D1_LeaveAlone	"Leave existing frames alone"
F_AB00D2_MakeComp	"\
Frames created in the future will default to\n\
using compiled forms.  Should all existing\n\
frames which now use forms from the database be\n\
changed to use compiled forms?\n"

F_AB00D3_YesMakeDB	"Make all frames use database forms"
F_AB00D4_MakeDB	"\
Frames created in the future will default to\n\
fetching forms from the database when running as\n\
an image.  Should all existing frames which now\n\
use compiled forms be changed to use forms from\n\
the database?\n"

F_AB00D5_CompChangeBanner	"Change of Default for Compiled Forms"

S_AB02B0_ChangeCompForm		
"Changing existing frames (this may take some time) . . ."

F_AB00D6_YesChangeMenus	"Change all menu frames to default style"
F_AB00D7_LeaveAlone	"Leave existing menu frames alone"
F_AB00D8_ChangeStyle	"\
Menu frames created in the future will default to\n\
using the menu style just chosen. Should all\n\
existing menu frames which use different styles\n\
be changed to use this style?\n"

F_AB00D9_StyleChangeBanner	"Change of Default Menu Style"

S_AB02B1_ChangeMenuStyle
"Changing existing menu frames (this may take some time) . . ."

S_AB0080_DemoPasswordPrompt "\
You are not authorized to use INGRES/Vision.  If you feel this is not\n\
correct, please see your INGRES System Administrator for assistance.\n\
\n\
To run a demonstration version of INGRES/Vision, enter the appropriate\n\
password below and press RETURN.  If you do not know the demonstration\n\
password and wish to see a demonstration of INGRES/Vision, please contact\n\
your Ingres Sales Representative for assistance.  In the U.S., you may\n\
call 1-800-4-INGRES; outside the U.S., you may contact your regional\n\
Ingres sales office.\n------\n"

S_AB0081_DemoWarn "\
Welcome to the demonstration version of INGRES/Vision.\n\
\n\
For first time users, we recommend that you try the Vision tutorial\n\
first.  A Vision tutorial database can be created by simply typing\n\
'visiontutor' at the operating system prompt.  Then you may follow the\n\
step-by-step instructions provided in the Vision Tutorial document.\n\
\n\
As this is a demonstration version of INGRES/Vision, please refer all\n\
questions, including technical questions about the operation of the\n\
product, to your Ingres Sales Representative.  In the U.S., you may\n\
call 1-800-4-INGRES; outside of the U.S., you may contact your regional\n\
Ingres sales office.\n\
\n\
Press RETURN to run the demonstration version of INGRES/Vision."

E_AB0082_NoVision "\
Vision frames in the application\n\
The application you are attempting to image contains frames generated
with the optionally available Vision product, but this installation
is not authorized to use Vision. Contact your Ingres Sales Representative
for information about purchasing Vision for this installation."

/*%
** Explanation:
**
** System status:
**	The application cannot be imaged.
**
** User action:
**	Delete the vision frames before attempting to image.
*/

E_AB0083_DemoError "\
The password you entered is not valid.\n\
\n\
Contact your Ingres Sales Representative for information about the\n\
INGRES/Vision demonstration."

/*%
** Explanation:
**
** System status:
**	Vision will not run.
**
** User action:
**	Contact sales about a demonstration password.
*/

E_AB0154_NoObjFile	"Missing object file `%0c'.\n
The object file `%0c' does not exist. You will need to perform the necessary
compilation and make sure the resulting object file is in this directory
before an image can be built."

/*%
** Explanation:
**	The user is trying to run imageapp with the "nocompile" option, but
**	a required object file does not exist.
**
** System status:
**	The application will not be imaged. Imageapp will exit.
**
** User action:
**	Compile the appropriate source file (C or fortran) into the directory
**	$ING_ABFDIR/dbname/appname. If the source file does not exist, it
**	should be created using oslsql, compform, esqlc, eqc, esqlf or eqf as
**	appropriate.
*/

E_AB0155_OSCompErr	"Compile failed with Operating System error message: 
\n`%0c'"

/*%
** Explanation:
**	The compile failed and generated the Operating System level error
**	message shown.
**
** System status:
**	The compile failed.
**
** User action:
**	Check that commands in utcom.def are OK and compiler is being
**	referenced correctly. On VMS, check to see that the Job level
**	logical table isn't full or near full. Make sure permissions
**	are OK in the object directory and the source directory.
*/
E_AB00CA_BadConstantFile
"Either the file '%0c' does not exist, or it is improperly formatted. If it
exists, make sure it conforms to the original syntax. The original syntax 
can be seen after creating the global constants file."
/*%
** Explanation:
**	The user specified the '-constants_file'  flag, but the file
**	specified could not be open/read.
**
** System Status:
**	The application will exit after issuing this error message.
**
** User Action:
**	Make sure the file exists. If it is edited by user, make sure
**	it conforms to the original syntax.
*/
E_AB00CB_NameToLoc
"Conversion of file name string to proper location failed. Check file name."
/*%
** Explanation:
**	The user specified the '-constants_file' flag, but the file
**	specified could not be converted to proper Ingres location.
**
** System Status:
**	The application will exit after issuing this error message.
**
** User Action:
**	Make sure the file has correct syntax and uses acceptable characters
**	for file name.
*/
E_AB00CC_BadConstant
"Failed to read the constant value for global constant '%0c'."
/*%
** Explanation:
**	One or more global constants in constants_file may be improperly
**	formatted. 
**
** System Status:
**	The application will exit after issuing this error message.
**
** User Action:
**	Make sure the constants have correct syntax.
*/
E_AB00CD_AllocFail
"Failed to allocate memory for global constant '%0c'."
/*%
** Explanation:
**	Failed to allocate memory for global constant.
**
** System Status:
**	The application will exit after issuing this error message.
**
** User Action:
**	Make sure the process has enough memory.
*/
E_AB00CE_IntConvFailed
"Failed to convert global constant '%0c' to Integer."
/*%
** Explanation:
**	Failed to convert global constant to Integer."
**
** System Status:
**	The application will exit after issuing this error message.
**
** User Action:
**	Make sure the global constant has valid format.
*/
E_AB00CF_FloatConvFailed
"Failed to convert global constant '%0c' to Float."
/*%
** Explanation:
**	Failed to convert global constant to Float."
**
** System Status:
**	The application will exit after issuing this error message.
**
** User Action:
**	Make sure the global constant has valid format.
*/
E_AB00DA_PackConvFailed
"Failed to convert global constant '%0c' to Packed Decimal."
/*%
** Explanation:
**	Failed to convert global constant to Packed Decimal."
**
** System Status:
**	The application will exit after issuing this error message.
**
** User Action:
**	Make sure the global constant has valid format.
*/
