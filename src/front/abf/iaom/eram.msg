/*
**	Copyright (c) 1987, 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	eram.msg -	IAOM Message Definitions File.
**
** Description:
**	This file defines the messages for class AM.
**
** History:
**	Revision 6.4
**	11/08/90 (emerson)
**		Added E_AM0017 to iamdec.
**
**	Revision 6.3  90/03  wong
**	Added system-defined warning messages and duplicate object errors for
**	'iiamCk*()' methods.  JupBug #7026.  (6.4 added 89/07)
**	90/04  wong  Added class names.
**
**	Revision 6.0  87/08/03  bobm
**	Initial revision.
**
**	14-dec-90 (blaise)
**	    Reworded E_AM0021, E_AM0022 and E_AM0023, to make it clear that
**	    frames, procedures and global variables & constants share the same
**	    name space, and that E_AM0023 is referring to constants as well as
**	    variables. Bug #34889.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**/

# define	_AM_CLASS	45
/*
**	iamalgn.c
*/
/*
**	fatal error - not supposed to happen
*/
S_AM0001_afc_cxinfo	"IAOM: unexpected afc_cxinfo failure" /* DIAG */
/*
**	iamdec.qc
*/
/*
**	syntax error strings.  These should not happen, and would indicate
**	bad data in a system catalog if they do.  The slow strings are
**	the arguments possible with the error string.
*/
S_AM0002_token_too_long	"token too long"
S_AM0003_unexpected_end	"unexpected end"
S_AM0004_bad_array_decoding	"bad array decoding"
S_AM0005_exp_obj_end	"expected end of object"
S_AM0006_exp_int	"expected integer"
S_AM0007_exp_float	"expected float"
S_AM0008_expected_array	"expected array"
S_AM0009_array_id	"non-negative array id"
S_AM000A_exp_size	"expected array size"
S_AM000B_neg_length	"negative array length"
S_AM000C_exp_obj_id	"expected object id"
S_AM000D_unexp_obj_id	"unexpected object id"
S_AM000E_exp_start	"expected object start"
S_AM000F_unexpected_characters	"unexpected characters at end of string"
E_AM0010_encoding_syntax	"ii_encodings table syntax error: %0c"
/*
**	fatal error - not supposed to happen
*/
S_AM0011_decode_err	"oo_decode, program error %0d" /* DIAG */

E_AM0017_badvers	"Frame was compiled by a later release.\n
This frame (or procedure) was compiled using a later release of 4GL
and cannot be run using this release of 4GL."

/*%
** Explanation:
**	4GL found that the frame (or procedure) was compiled using a later
**	release of 4GL that produces intermediate language that cannot be
**	interpreted by this release of 4GL.
**
** System Status:
**	4GL will not attempt to run the frame.
**
** User Action:
**	Compile the frame using the current (old) release of 4GL
**	(if possible: you may get compile-time errors), or run the 
**	frame using the later release of 4GL.
*/

/*
**	iamdtbl.c
*/
/*
**	fatal error - not supposed to happen
*/
S_AM0012_unexp_encode	"Encoding procedure called while decoding" /* DIAG */
/*
**	iametbl.c
*/
/*
**	fatal error - not supposed to happen
*/
S_AM0013_bad_decode_call	"decode procedure called while encoding" /* DIAG */
/*
**	iamfadd.qc
*/
E_AM0014_No_more_ids	"No more IDs may be generated -- reached limit\n"
/*
**	iamfchk.qc
*/
E_AM0015_objects_corrupt	"ii_objects table corrupted.  %0d occurrences of id %1d.\n" /* DIAG */

E_AM0016_badobj		"Unknown object type in catalogs.\n
An unknown object (type '%0d') was found in the catalogs for this application.\ 
This is most likely because this application has been modified by a later
release to use new features not recognized by this release.  All unknown objects
will be ignored in this release."

/*%
** Explanation:
**	ABF found an unknown object type in its catalog for this application.
**	This is most likely a new object representing new features added by a
**	later revision of ABF.
**
** System Status:
**	ABF will ignore these new objects, but you should be aware that the
**	application may not compile or execute correctly without them.
**
** User Action:
**	You should probably not attempt to modify the application.
*/

/* Application Component Create Frame Warning Prompts */

F_AM0001_ADFNameConflict
"Warning:  This name conflicts with a system-defined function name.\n\nDo you
wish to override the system-defined function (yes/no)?"

F_AM0002_SysProcNameConflict
"Warning:  This name conflicts with a system-defined procedure name.\n\nDo you
wish to override the system-defined procedure (yes/no)?"

F_AM0003_SysFrmNameConflict
"Warning:  This name conflicts with a system-defined frame name.\n\nDo you wish
to override the system-defined frame (yes/no)?"

E_AM0020_ExistingAppl	"Duplicate application name `%0c'\n
The application `%0c' already exists in this database.  Application names must
be unique within a database.  Please enter another name for this application."

/*%
** Explanation:
**	All applications share the same name space within a database
**	and so must be unique.
**
** System Action:
**	ABF will not create or rename the application and will remain on the
**	current frame.
**
** User Action:
**	Choose another name for the application and enter it in the name field.
**	Or, select Cancel to cancel the operation.
*/

E_AM0021_ExistingFrm	"Duplicate frame name `%0c'.\n
The name `%0c' is already in use in the application.  A frame name must be
unique, and may not be the same as an existing procedure, global variable or
global constant name, within an application.  Please enter another name
for this frame."

/*%
** Explanation:
**	All frame, procedure, global variable, and constant components of an
**	application share the same name space and so must be unique within
**	that application.
**
** System Action:
**	ABF will not create or rename the frame definition and will remain on
**	the current frame.
**
** User Action:
**	Choose another name for the frame and enter it in the name field.  Or
**	select Cancel to cancel the operation.
*/

E_AM0022_ExistingProc	"Duplicate procedure name `%0c'.\n
The name `%0c' is already in use in the application.  A procedure name must be
unique, and may not be the same as an existing frame, global variable or global
constant name, within an application.  Please enter another name for this
procedure."

/*%
** Explanation:
**	All procedure components of an application share the same name space
**	with other procedures and with frames, global variables, and constants
**	and so must be unique within that application.
**
** System Action:
**	ABF will not create or rename the procedure and will remain on the
**	create frame.
**
** User Action:
**	Choose another name for the procedure and enter it in the name field.
**	Or select Cancel to cancel the operation.
*/

E_AM0023_ExistingGlobal	"Duplicate global name `%0c'.\n
The name `%0c' is already in use in the application.  A global variable or
constant may not have the same name as an existing frame, procedure, global
variable or global constant, within an application.  Please enter another
name."

/*%
** Explanation:
**	All global variable or constant components of an application share
**	the same name space with other global variables and constants and
**	with frames and procedures as well, and so must be unique within
**	that application.
**
** System Action:
**	ABF will not create or rename the global variable or constant and will
**	remain on the current frame.
**
** User Action:
**	Choose another name for the global variable or constant and enter it in
**	the name field.  Or select Cancel to cancel the operation.
*/

E_AM0024_ExistingRecord	"Duplicate record name `%0c'.\n
The name `%0c' already belongs to another record in the application.  Record
names must be unique within an application.  Please enter another name for the
record."

/*%
** Explanation:
**	All class components of an application share the same name space
**	and so must be unique within that application.
**
** System Action:
**	ABF will not create or rename the class and will remain on the current
**	frame.
**
** User Action:
**	Choose another name for the class and enter it in the name field.  Or
**	select Cancel to cancel the operation.
*/

E_AM0025_BadNamePrefix	"Illegal `ii' prefix for name `%0c'.\n
The prefix `ii' is reserved for internal names.  Please enter another name."

/*%
** Explanation:
**	All names beginning with the letters `ii' are reserved for internal use.
**
** System Action:
**	ABF will not create or rename the component and will remain on the
**	current frame.
**
** User Action:
**	Choose another name and enter it in the name field.  Or select Cancel to
**	cancel the operation.
*/

E_AM0026_BadClassName	"Illegal record type name `%0c'.\n
The name `%0c' is the name of an INGRES data type and cannot be used to name
a record definition for an application.  Please enter another name."

/*%
** Explanation:
**	Class names cannot override INGRES type names.
**
** System Action:
**	ABF will not create or rename the class and will remain on the current
**	frame.
**
** User Action:
**	Choose another name and enter it in the name field.  Or select Cancel to
**	cancel the operation.
*/

E_AM0027_Form_name "Cannot generate unique form name for frame '%0c'."

/*%
** Explanation:
**	Probably failed DB access.
**
** System Action:
**	ABF will not create the frame.
**
** User Action:
**	If you got some kind of a DB error also, rectify that situation,
**	and try again.
*/

E_AM0028_ExistingRatt "Duplicate record attribute name `%0c'.\n
The name `%0c' already belongs to another attribute in the record.  Attribute
names must be unique within a record definition.  Please enter another name
for the attribute."

/*%
** Explanation:
**	All attributes of a record definition share the same name space
**	and so must be unique within that record definition.
**
** System Action:
**	ABF will not create or rename the record attribute and will remain on
**	the current frame.
**
** User Action:
**	Choose another name for the attribute and enter it in the name field.
**	Or select Cancel to cancel the operation.
*/

E_AM0029_MetaPopErr
"Error reading frame '%0c'\n\
An error occurred reading information about frame '%0c' from the database.
Previous error messages might explain the problem in more detail.
This is an unexpected internal error.  Please notify your Site
Technical Contact."
/*%
** Explanation:
**      When reading info for a particular frame from the database
**	there was an error.
**
** System Status:
**	Whatever operation caused the frame information to be read
**	will be aborted.
**
** User Action:
**	Basically the frame is unusable until the problem is resolved.
*/

E_AM0030_GlobUsesRec "A Global variable is of type '%0c'.\n
Global variable '%1c' is of type '%0c'.  If you wish to delete or rename
the record type, please delete or change the type of the global variable."

/*%
** Explanation:
**      The record type is in use; a global variable is of that type.
**
** System Action:
**      ABF will not delete or rename the record type.
**
** User Action:
**      Delete or change the type of the global variable, or don't delete
**	or rename the record.
*/

E_AM0031_RecUsesRec "Another record contains record '%0c'.\n
Record type '%1c' contains type '%0c'.  If you wish to delete or rename
the record type, please edit record type '%1c' to remove the reference to '%0c'."

/*%
** Explanation:
**      The record type is in use; another record contains it.
**
** System Action:
**      ABF will not delete or rename the record type.
**
** User Action:
**      Remove the reference to the record type, or don't delete
**	or rename the record.
*/

E_AM0032_MultRecurseTrav "An internal error has occurred.\n
Multiple recursive traversals are active in the Dependency Manager.  Please
report this error to your Site Technical Contact."	

/*%
** Explanation:
**      The Dependency Manager was asked to begin a recursive traversals while
**	another recursive traversal was active.  This is an internal error.
**
** System Action:
**      ABF will exit.
**
** User Action:
**      Report the circumstances causing the error to your Site Technical 
**	Contact.
*/

E_AM0033_OLDOSL_UpgradeFailure
"Frame '%0c' could not be upgraded\n\
This frame is an OLDOSL frame.  OLDOSL is no longer supported in the current
release of INGRES.  The attempt to upgrade this frame to a User Frame failed.  
This frame cannot be compiled or run until it is converted.  This is
an internal error. Please report it to your Site Technical Contact."

/*
** Explanation:
**      An error occurred trying to convert the OLDOSL frame to a User Frame.
**
** System Action:
**      It will remain an OLDOSL frame.  It must be upgraded before it can be
**	compiled or run.
**
** User Action:
**      Report the circumstances causing the error to your Site Technical 
**	Contact.
*/

/*
**
** Class names.
*/
F_AM0010_Application	"Application"
F_AM0011_ApplComp	"Application Component"
F_AM0012_HLProc		"Procedure"
F_AM0013_4GLProc	"4GL Procedure"
F_AM0014_DBProc		"DB Procedure"
F_AM0015_4GLFrame	"User Frame"
F_AM0016_ReportFrame	"Report Frame"
F_AM0017_QBFFrame	"QBF Frame"
F_AM0018_GraphFrame	"Graph Frame"
F_AM0019_MenuFrame	"Menu Frame"
F_AM001A_AppendFrame	"Append Frame"
F_AM001B_UpdateFrame	"Update Frame"
F_AM001C_BrowseFrame	"Browse Frame"
F_AM001D_GlobalVar	"Global Variable"
F_AM001E_Constant	"Constant"
F_AM001F_ClassDef	"Record Type Definition"
F_AM0020_ClassAttr	"Record Attribute"
F_AM0021_HelpFrame	"Help Frame"
F_AM0022_FormChange	"Recompiling because form '%0c' has changed."
F_AM0023_TableChange	"Recompiling because table '%0c' has changed."
