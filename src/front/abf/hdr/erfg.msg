/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	erfg.msg -	Message file for class FG
**
** Description:
**	This file defines the messages for class FG.
**
** History:
**	 3-aug-1989 (bobm)
**		Created.
**	2/jan-1990 (pete)	Tried to make it clear in all messages that
**				the error occurred during 4gl code generation.
**				Also, added notes to internal errors, that
**				user should try to narrow down what caused the
**				internal error, before calling Tech Supt.
**	23-oct-1990 (mgw)
**		Corrected a misspelling in a message.
**	24-jul-92 (blaise)
**		Added F_FG003E_Before_Lookup, F_FG003F_After_Lookup,
**		F_FG0041_On_Timeout, F_FG0042_On_Dbevent,
**		F_FG0043_Table_Field_Menuitems and F_FG0044_Menuline_Menuitems,
**		all new Vision escape code types.
**	13-aug-92 (blaise)
**		Added F_FG0045 and F_FG0046: local procedure messages.
**	6-dec-92 (blaise)
**		Added F_FG0047_OptLkHidFldComnt - comment for optmistic locking
**		fields.
**	09-sep-93 (cmr)
**		Made F_FG0031_HidnKeyFldComment more generic since hidden flds
**		are generated for all sorts of reasons (seq flds, optimistic 
**		locking, lookups, etc) and not just for primary keys and joins.
**/

# define	_FG_CLASS	173

/*
**	fgifutl.c
*/
E_FG0001_Unknown_VQ_Var	"Error occurred while generating 4gl code\n
Found bad Visual Query variable in ##IF stmt\n
An error occurred trying to generate 4gl code for this frame.
The ##IF statement on line %0d of template file '%1c' contains a
reference to '%2c',
which is either an unknown Visual Query variable, or is a Visual Query
variable that does not evaluate to TRUE or FALSE (only boolean Visual
Query variables may be used in ##IF statements). This is an error in your
template file; correct the template file '%1c' before trying to generate
4gl again."

E_FG0002_IfStmt_Nesting  "Error occurred while generating 4gl code\n
An error occurred trying to generate 4gl code for this frame.
##IF/##IFDEF/##ENDIF blocks may only be nested up to %0d levels deep.
Line %1d of template file '%2c' contains a ##IF or ##IFDEF statement which,
if executed, would have exceeded this limit. This is an error in your template
file; correct the template file '%2c' before trying to generate 4gl again."

E_FG0003_Mismatch_ElseEndif	"Error occurred while generating 4gl code\n
Template file error: mismatched ##ELSE or ##ENDIF.
An error occurred trying to generate 4gl code for this frame.
On line %0d of template file '%1c', a ##ELSE or ##ENDIF statement was
encountered that does not match any previous ##IF or ##IFDEF statement.
This is an error in your template file; correct the template file '%1c'
before trying to generate 4gl again."

E_FG0004_Too_Many_Elses	"Error occurred while generating 4gl code\n
Template file error: too many ##ELSE stmts found.
Line %0d of template file '%1c' contains a duplicate ##ELSE statement. A
##ELSE statement has already been seen for the previous ##IF/##IFDEF statement,
so this duplicate ##ELSE is an error in the template file '%1c'. Correct the
template file before trying to generate 4gl again."

/*
**	framegen.c
*/
E_FG0005_No_TFdir	"Error occurred while generating 4gl code\n
Can't find location for template files.
An error occurred trying to generate 4gl code for this frame: neither
of the environment variables II_CONFIG or II_TFDIR are set. One of these
must be set to the directory where the template files live. Before continuing
you must first set one of these environment variables to point to where your
template files are. If you are not sure how to do that, then contact your Site
Technical Contact for assistance."

E_FG0006_Open_Error	"Error occurred while generating 4gl code\n
Unable to open output file for generated code.
An error occurred trying to generate 4gl code for this frame: the output
file '%0c' could not be opened. Check that you have sufficient disk space
and quota on this directory, and that you have authority to create files there."

E_FG0007_IncludeStmt_Nesting	"Error occurred while generating 4gl code\n
##INCLUDE statements nested too deep.
An error occurred generating 4gl code for this frame. The statement
'##INCLUDE %0c' was encountered, and cannot be processed, because that
would exceed the limit of %1d levels deep in ##INCLUDE file processing. Often
an infinite loop in your ##INCLUDE file processing will cause this error (for
example if file1 does '##INCLUDE file2', and file2 does '##INCLUDE file1',
then an infinite loop will occur). Check your template files for an
infinite loop in ##INCLUDE statements, or ##INCLUDE statements nested more
than %1d levels deep."

E_FG0008_TooMany_Endifs	"Error occurred while generating 4gl code\n
Excessive number of ##ENDIF statements found.
An error occurred generating 4gl code for this frame. On line %0d of template
file '%1c', a ##ENDIF statement was found that does not match up with any
preceding ##IF or ##IFDEF statement. This is an error in the template file
and should be corrected before proceeding."

E_FG0009_Unclosed_IfStmt	"Error occurred while generating 4gl code\n
##ENDIF missing for ##IF or ##IFDEF in template file.
An error occurred generating 4gl code for this frame. Template file
'%0c' has an error: it contains at least one ##IF or ##IFDEF statement that
is not closed by a corresponding ##ENDIF statement in the same file. This is an
error in the template file and should be corrected before proceeding. If
you don't know how to correct an error in a template file, then contact
your Site Technical Contact for assistance."

E_FG000A_Directory_ChkError	"Error occurred while generating 4gl code\n
Unable to check for valid directory.
An error occurred generating 4gl code for this frame. The environment
variable '%0c' is set to '%1c'. An error occurred while checking if
'%1c' is a valid directory. Check that '%0c' is set correctly, and if so,
check that you are authorized to access files in the directory it points to." 

E_FG005D_NoFilesDir	"Error occurred while generating 4gl code\n
The directory location: '%0c'
does not exist, or you are not allowed to access it.\n
This is the directory that corresponds to the II_LANGUAGE (or 'english')
subdirectory of the Ingres 'files' directory (this location is where
the template and help files needed to generate 4gl code are located).\n
This indicates there is
a problem with your Ingres installation; please check that you have an
Ingres 'files' directory, and a subdirectory within that, named the same
as your II_LANGUAGE setting (or 'english'), and that you are allowed
to access files in that directory. You must fix this installation
problem before proceeding. Contact your Site Technical Contact
for assistance."

E_FG005C_BadFilesDir	"Error occurred while generating 4gl code\n
Unable to locate the directory corresponding to the II_LANGUAGE (or 'english')
subdirectory of the Ingres 'files' directory (this location is where
the template and help files needed to generate 4gl code are located -- the
internal error generated was %0d). This indicates there is
a problem with your Ingres installation; please check that you have an
Ingres 'files' directory, and a subdirectory within that, named the same
as your II_LANGUAGE setting (or 'english'). You must fix this installation
problem before proceeding. Contact your Site Technical Contact
for assistance."

E_FG000B_Invalid_Directory	"Error occurred while generating 4gl code\n
Invalid directory.
An error occurred generating 4gl code for this frame. The environment
variable '%0c' is set to '%1c', which is not a valid directory. This
environment variable must be set to a valid directory name. Please fix
this error before proceeding."

E_FG000C_Bad_Filename 	"Error occurred while generating 4gl code\n
Invalid name for a template file.
An error occurred generating 4gl code for this frame. The template file name
'%0c' is not valid. Check for a '##INCLUDE %0c' statement in one of
your template files. If you cannot find this statement, or do not know
how to fix it, then call your Site Technical Contact for assistance."

E_FG000D_Cant_Open_File	"Error occurred while generating 4gl code\n
Unable to open template file.
An error occurred generating 4gl code for this frame. The template file '%0c'
could not be opened. Check that this file exists and can be read by you.
This file must be created, or the ##INCLUDE statement that
references it must be removed before proceeding.
If you are not sure how to check this template file, then contact your Site
Technical Contact for assistance."

E_FG000E_File_Not_Found	"Error occurred while generating 4gl code\n
Template file not found.
An error occurred generating 4gl code for this frame. The template file '%0c'
does not exist. This is an error in your template files. Often this
error is caused by a ##INCLUDE statement that references a nonexistent
file.
This file must be created, or the ##INCLUDE statement that
references it must be removed before 4gl code can be generated."

E_FG000F_Bad_TL_Statement	"Error occurred while generating 4gl code\n
Found invalid Template Language statement.
An error occurred generating 4gl code for this frame. Line %1d of template
file '%2c' contains the statement '## %0c', which is not a valid statement.
This is an error in the template file and must be corrected before proceeding."

E_FG0010_Missing_Args	"Error occurred while generating 4gl code\n
Found bad ##GENERATE template language statement.
An error occurred generating 4gl code for this frame. Line %0d of template
file '%1c' contains a '## GENERATE' statement with zero arguments. This
is a syntax error in the template file, and should be fixed before you
continue."

E_FG0011_Bad_Generate_Stmt	"Error occurred while generating 4gl code\n
Bad ##GENERATE template language statement.
An error occurred generating 4gl code for this frame. Line %1d of template
file '%2c' contains the statement '## GENERATE %0c', and '%0c' is not a valid
type of ##GENERATE statement. This is a syntax error in the template file and
must be fixed before you continue."

E_FG0012_Token_TooLong	"Error occurred while generating 4gl code\n
Found too long a token in template file.
An error occurred generating 4gl code for this frame. The token '%0c'
was found in a template file, and this exceeds the maximum allowable
token size (max size = %1d). This is an error in the template file, and
must be corrected before proceeding."

E_FG004B_AV_SegViol	"Internal Error occurred while generating 4gl code\n
An internal error (known as an Access Violation, Bus Error, or
Segmentation Violation on many systems)
occurred while generating 4GL for this frame. Please
narrow down the conditions required to make this error occur,
by changing sections of your Visual Query and then re-Compiling this
frame until the error goes away. Then report this problem to Ingres Technical
Support." 

E_FG0057_RenameError	"Error occurred while generating 4gl code\n
An error occurred trying to rename a file in the source code directory
for this application. After writing the 4gl code for this frame
into a temporary file in the application source directory, an error
occurred trying to rename that file to the correct name.
The 4gl code for this frame was not created.
Check that you have authority to change
the name of files in the application source code directory." 

E_FG0058_Bad_Env_Var	"Error occurred after generating 4gl code\n
An error occurred after 4gl code generation, while attempting to
translate and then execute the environment variable '%0c'. As a result,
any actions that this environment variable should have triggered
did not take place. However, the 4gl code for this frame was successfully
created in the application source directory before the error occurred.
If you are not sure how to fix this problem, then tell your Site
Technical Contact that an error occurred while translating
and executing environment variable '%0c'."

/*
**	fghelp.c
*/
E_FG004D_TooFewArgs "Warning: can't create help file for frame\n
Warning: while generating 4gl code, a '## GENERATE HELP' statement on
line '%0d' of file '%1c' was discovered, which
does not contain the name of a help file. The word 'HELP' should be followed
by the name of a file that exists in the english subdirectory of the
directory referenced by the II_CONFIG environment variable. Normal
processing of this statement causes the referenced help file to be moved
into your application's source directory. This is an error in your
template file.\n
Code generation will continue, but a help file will be missing from your
application source directory."

E_FG004E_TooManyArgs "Warning: ##GENERATE HELP stmt has too many arguments\n
Warning: while generating 4gl code, a '## GENERATE HELP' statement was found on
line '%0d' of file '%1c' which has too many arguments following the
word 'HELP'. Exactly one
argument should follow the word 'HELP'. The word 'HELP' should be followed
by the name of a file that exists in the english subdirectory of the
directory referenced by the II_CONFIG environment variable.
Correct this error in your template file.\n
Code generation will continue and an attempt will be made to copy the
help file referenced on this ##GENERATE HELP statement..."

E_FG004F_BadHelpFileName "Warning: cannot access help file for frame\n
Warning, a help file error occurred while generating 4gl code.
The '## GENERATE HELP' statement on line '%0d' of file
'%1c' contains a reference to the help file: '%2c'.
That file either does not exist (in the english subdirectory of the
directory referenced by the II_CONFIG environment variable), or it does
exist, but you do not have permission to access it. Code generation will
continue, but, since it cannot be copied, a help file will be missing
from your application source directory."

E_FG0050_GenericCopyErr "Warning: error copying help file\n
Warning, a help file error occurred while generating 4gl code.
An error occurred trying to create a help file in your
application source directory while processing the '## GENERATE HELP'
statement on line '%0d' of file '%1c'. The 4gl code generator was not able
to copy the help file '%2c' (in the english subdirectory of the directory
referenced by the II_CONFIG environment variable)
to your source directory.
Check that disk space is available and that you are authorized to
create files in the application source directory.
Code generation will
continue, but, since it cannot be copied, a help file will be missing
from your application source directory."

/*
**	fgquery.c
*/
E_FG0013_Query_InsfArgs "Error occurred while generating 4gl code\n
Insufficient arguments specified on line %0d in file '%1c'.\n
This is an error in your template file. It must be corrected before code
generation can continue.
If you don't know how to fix this problem, then report it
to your Site Technical Contact."

E_FG0014_Query_TooManyArgs "Error occurred while generating 4gl code\n
Too many arguments specified on line %0d in file '%1c'.\n
This is an error in your template file. It must be corrected before code
generation can continue.
If you don't know how to fix this problem, then report it
to your Site Technical Contact."

E_FG0015_Query_InvType "Error occurred while generating 4gl code\n
Illegal query type '%2's specified on line %0d in file '%1c'.\n
This is an error in your template file. It must be corrected before code
generation can continue.
If you don't know how to fix this problem, then report it
to your Site Technical Contact."

E_FG0016_Query_InvOption "Error occurred while generating 4gl code\n
Illegal query option '%2c' specified on line %0d in file '%1c'.\n
This is an error in your template file. It must be corrected before code
generation can continue.
If you don't know how to fix this problem, then report it
to your Site Technical Contact."

E_FG0017_Query_NoTable 	"Error occurred while generating 4gl code\n
Nonexistent table %2c specified on line %0d in file '%1c'.\n
Please report this internal problem to your Site Technical Contact."

E_FG0018_Query_FrameType "Error occurred while generating 4gl code\n
Query '%2c' inconsistent with this frame type. Line %0d in file '%1c'.\n
The template file '%1c' contains a '## GENERATE QUERY' statement that is not
consistent with this frame type.
This is an error in your template file. It must be corrected before 4gl code
generation can continue.
If you don't know how to fix this problem, then report it
to your Site Technical Contact."

E_FG0019_Query_NoKey "Error occurred while generating 4gl code\n
No unique key specified for table '%2c' on line %0d in file '%1c'.\n
Please narrow down the conditions required to make this error occur,
by changing sections of your Visual Query and then re-Compiling this
frame until the error goes away. Then report this problem to Ingres Technical
Support."

E_FG001C_Query_NoJoin "Error occurred while generating 4gl code\n
No master field joins to column %3c in table '%2c' on line %0d in file '%1c'.\n
Please narrow down the conditions required to make this error occur,
by changing sections of your Visual Query and then re-Compiling this
frame until the error goes away. Then report this problem to Ingres Technical
Support."

E_FG0020_Query_BadSect "Error occurred while generating 4gl code\n
Invalid query section '%2c' was specified on line %0d in file '%1c'.\n
Please report this internal problem to your technical representative."

E_FG0021_Query_NoPriTbl "Error occurred while generating 4gl code\n
No database table found for section '%2c' on line %0d in file '%1c'.\n
An error occurred while generating 4gl for this frame.
The template file '%1c' requested that code be generated for a '%2c' section
of the visual query, and no such section exists. This could either be an error
in template file '%1c', or a bug in INGRES.
This error must be corrected before code generation can continue.
Please report this problem to your Site Technical Contact."

E_FG0022_Query_NoDynMem "Error occurred while generating 4gl code\n
Insufficient dynamic memory available while trying to generate 4GL code for
line %0d in file '%1c'.
This usually indicates
that your process or system ran out of memory, or tried to exceed a
memory limit quota. Take whatever steps you can to free up more memory
for your use and then try this again. Contact your Site Technical
Contact for assistance."

E_FG004C_Query_NoHiddenName "Error occurred while generating 4gl code\n
No hidden name for column '%2c' on line %0d in file '%1c'.
This non-displayed column requires a hidden field name, but none was generated.
This is an internal error, which must be corrected before code generation
can continue.
Please narrow down the conditions required to make this error occur,
by changing sections of your Visual Query and then re-Compiling this
frame until the error goes away. Then report this problem to Ingres Technical
Support."

E_FG0023_Write_Error "Error occurred while generating 4gl code\n
An error occurred while writing generated 4gl code to this frame's source file.
The source file will be deleted. Make sure you have enough disk space
available to create this file."

E_FG0024_Missing_DetailTbl "Error occurred while generating 4gl code\n
Missing detail table name.
An internal error occurred while generating 4gl for this frame. Internal
information about your Visual Query indicates that this is a Master/Detail
frame, but the name of the Detail table could not be found. Please determine
exactly what circumstances are required to make this error occur and report
it as a bug in INGRES."

E_FG0025_Missing_MasterTbl "Error occurred while generating 4gl code\n
Missing master table name.
An internal error occurred while generating 4gl for this frame. Internal
information about your Visual Query does not indicate which table
is the primary master table. Please determine exactly what circumstances are
required to make this error occur and report it as a bug in INGRES."

E_FG0026_Null_Metaframe "Error occurred while generating 4gl code\n
Bad visual query information.
An internal error occurred while generating 4gl for this frame. The
description of the Visual Query, passed by the Visual Query module
to the 4gl code generation module, was empty. Please determine exactly
what circumstances are required to make this error occur and report
it as a bug in INGRES."

E_FG0027_Null_ABF_Ptr "Error occurred while generating 4gl code\n
Bad ABF information referenced by VQ.
An internal error occurred while generating 4gl for this frame. The
Visual Query does not contain a valid reference to an ABF frame.
Please determine exactly what circumstances are required to make this
error occur and report it as a bug in INGRES."

E_FG0028_Bad_Mode "Error occurred while generating 4gl code\n
Bad internal information found in Visual Query.
An internal error occurred while generating 4gl for this frame. The
Visual Query contained a metaframe.mode entry that does not
correspond to a frame mode that is currently supported.  Please 
determine exactly what circumstances are required to make this error 
occur and report it as a bug in INGRES."

E_FG0029_BadFrmTyp_For4glGen "Error occurred while generating 4gl code\n
Bad internal information found in Visual Query.
An internal error occurred while generating 4gl for this frame. The
Visual Query contains a metaframe.apobj.class entry of %0d, which is not one of
the types that code can be generated for.
Please determine exactly
what circumstances are required to make this error occur and report it
as a bug in INGRES."

E_FG0031_NoDisplayedCols "Error occurred while generating 4gl code\n
No columns were marked displayed for table '%2c', on line %0d in file '%1c'\n
Please determine exactly
what circumstances are required to make this error occur and report it
as a bug in INGRES."

/*
**	fgdefram.c
*/
E_FG001A_Deffrm_NoDynMem "Error occurred while generating 4gl code\n
Insufficient dynamic memory is available.
Please report this internal problem to your technical representative."

E_FG001B_BadMfMode "Error occurred while generating 4gl code\n
Bad internal information found in Visual Query.
An internal error occurred while generating a default form for this frame. The
Visual Query contains the invalid metaframe.mode entry, '%0d'.  Please 
determine exactly what circumstances are required to make this error occur 
and report it as a bug in INGRES."

/*
**	fgnulkey.c
*/
E_FG0059_MissingArgs "Error occurred while generating 4gl code\n
The '## GENERATE SET_NULL_KEY_FLAGS' statement on line '%0d' of file '%1c'
does not contain an argument following the word 'SET_NULL_KEY_FLAGS'.
Either the word 'MASTER' or 'DETAIL' must follow the word 'SET_NULL_KEY_FLAGS'.
You must correct this error in your template file before 4gl code can
be generated for this frame."

E_FG005A_TooManyArgs "Warning occurred while generating 4gl code\n
Warning: too many arguments on a ## statement.
Error: the '## GENERATE SET_NULL_KEY_FLAGS' statement on line '%0d'
of template file '%1c' has the word '%3c' following the argument
'%2c'.  '%3c' is not a valid argument to the
'## GENERATE SET_NULL_KEY_FLAGS' statement.
Code generation will continue and the extra argument(s) will be ignored . . ."

E_FG005B_UnknownArg "Error occurred while generating 4gl code\n
The '## GENERATE SET_NULL_KEY_FLAGS' statement on line '%0d' of file '%1c'
has the word '%2c' following the argument
'SET_NULL_KEY_FLAGS'.  '%2c' is not a valid argument to the
'## GENERATE SET_DEFAULT_VALUES' statement.
You must correct this error in your template file before 4gl code can
be generated for this frame."

/*
**	fghidfld.c
*/
E_FG002A_WrongNmbrArgs "Warning: wrong number of arguments on ##statement.\n
Warning: the '## GENERATE HIDDEN_FIELDS' statement on line '%0d' of file '%1c'
contains arguments that will be ignored (the '## GENERATE HIDDEN_FIELDS'
statement does not allow arguments). Correct this error in your
template file. Code generation will continue and the extra arguments
will be ignored..."

E_FG001D_BadInternalType "Error occurred while generating 4gl code\n
Bad internal Data Type information found in VQ.
An internal error occurred while creating 4gl code for a
'##GENERATE HIDDEN_FIELDS' statement for column '%0c' in table '%1c'.
An internal structure element, metaframe.mftab.mfcol.type, was determined
to be invalid by routine afe_tyoutput(), which returned '%2d'.
Please determine exactly what circumstances are required to make this
error occur and report it as a bug in INGRES."

E_FG001E_BadInternalType2 "Error occurred while generating 4gl code\n
Bad internal Data Type information found in VQ.
An internal error occurred while creating 4gl code for a
'##GENERATE HIDDEN_FIELDS' statement for the user defined variable (hidden
field) '%0c'.  An internal structure element, metaframe.mfvar.dtype,
for this user variable was determined to be invalid by routine
afe_tyoutput(), which returned '%1d'.  Please determine exactly what
circumstances are required to make this error occur and report it as a bug
in INGRES."

/*
**	These comments are written to the generated 4gl file in fghidfld.c.
**	They describe the various types of hidden field declarations.
**	(##GENERATE HIDDEN_FIELDS).
*/
F_FG0011_HidnFldComment "/* Local Variables created by developer of frame */"
F_FG0046_LocProcComment "/* Local Procedures created by developer of frame */"
F_FG0031_HidnKeyFldComment "/* hidden versions of table columns */"
F_FG0036_NullKeyComment	"/* flag fields needed for NULLable key column(s) */"
F_FG0037_FlagFieldName	"iiNullKeyFlag"
F_FG0038_SetNulKeyComnt	"/* set NULLable-key flag fields */"
F_FG003B_VQhidfldComnt	"/* Local Variables specified in Visual Query */"
F_FG0047_OptLkHidFldComnt "/* hidden versions of optimistic locking columns */"

/*
**	fghdrdoc.c
*/
E_FG001F_WrongNmbrArgs "Warning: wrong number of arguments on ##statement\n
Warning: while generating 4gl code, a '## GENERATE HEADER_DOC' statement
was found on line '%0d' of file '%1c' which contains arguments that will
be ignored (the '## GENERATE HEADER_DOC'
statement does not allow additional arguments). Correct this error in your
template file. Code generation will continue and the extra arguments will
be ignored..."

/*
**	fgusrmnu.c
*/

/* following is written to generated code as a comment whenever the
** 'all' escape is replicated in the exit activate block for a field
** or column. This happens when a field has a change escape, but
** no exit escape, and an 'all' escape exists.
*/
F_FG0032_CopyExitAll "/* field/column 'all' exit escape: */"

E_FG002B_WrongNmbrArgs "Warning: incorrect arguments on ##statement\n
Warning: while generating 4gl code, a '## GENERATE USER_MENUITEMS' statement
was found on line '%0d' of file '%1c'
which contains arguments that will be ignored (the '## GENERATE USER_MENUITEMS'
statement allows one optional argument). Correct this error in your template
file. Code generation will continue and the extra argument(s) will be
ignored..."

E_FG0061_WrongNmbrArgs "Warning: incorrect arguments on ##statement\n
Warning: while generating 4gl code, a '## GENERATE CHECK_IIRETVAL' statement
was found on line '%0d' of file '%1c' which contains argument(s) that 
will be ignored (the '## GENERATE CHECK_IIRETVAL'
statement allows no arguments). Correct this error in your template
file. Code generation will continue and the extra argument(s) will be
ignored..."

/*
** The following is written to the generated 4gl as a comment about an
** "IF" test following a generated CALLFRAME statement.
*/
F_FG0039_ReturnToTop	"Return to top (this is not the start frame)"

/*
** The following is written to the generated 4gl as a comment about
** the value assigned to IIretval after a generated CALLFRAME statement.
*/
F_FG003A_RestoreDefVal	"restore default value"

/*
** fgloadmi.c
*/
E_FG0062_WrongNmbrArgs "Warning: incorrect arguments on ##statement\n
Warning: while generating 4gl code, a '## GENERATE LOAD_MENUITEMS' statement
was found on line '%0d' of file '%1c' which contains argument(s) that 
will be ignored (the '## GENERATE LOAD_MENUITEMS'
statement allows no arguments). Correct this error in your template
file. Code generation will continue and the extra argument(s) will be
ignored..."

/*
**	fgusresc.c
*/
E_FG002C_WrongNmbrArgs "Warning: wrong number arguments on ##statement\n
Warning: while generating 4gl code, a '## GENERATE USER_ESCAPE' statement
was found on line '%0d' of file '%1c' which contains multiple arguments
following the word 'USER_ESCAPE'. The extra arguments will be ignored
(the '## GENERATE USER_ESCAPE' statement should have only 1 argument after
the word 'USER_ESCAPE'). Correct this error in your template file.
Code generation will continue with extra argument(s) being ignored..."

E_FG002D_MissingArgs "Error occurred while generating 4gl code\n
Missing argument on a ##statement.
Error: the '## GENERATE USER_ESCAPE' statement on line '%0d' of file '%1c'
does not have an argument following the word 'USER_ESCAPE' to
tell what type of user_escape code should be generated. This is an
error in your template file. It must be corrected before code generation
can continue."

E_FG002E_UnknownArg "Error occurred while generating 4gl code\n
Unknown argument on a ## statement.
Error: the '## GENERATE USER_ESCAPE' statement on line '%0d' of template file
'%1c' has the word '%2c' following the word 'USER_ESCAPE'.  '%2c' is not a
valid argument to the '## GENERATE USER_ESCAPE' statement.  This is an
error in your template file. It must be corrected before code generation
can continue."

E_FG0030_OutOfMemory	"Error occurred while generating 4gl code\n
Not enough memory available.
The code generator was unable to allocate enough dynamic memory
to generate the 4gl code for this frame. This usually indicates
that your process or system ran out of memory, or tried to exceed a
memory limit quota. Take whatever steps you can to free up more memory
for your use and then try this again. Contact your Site Technical
Contact for assistance."

/*
**	NOTE: 11/89 this message is no longer used. fgusresc.c now does this.
**	This appears in a message statement following a Lookup Validation Query
**	in the generated code.
*/
F_FG000C_NotValidValue "MESSAGE FILE ERROR: F_FG000C NO LONGER USED"

/*
**	fgutils.c
**	'$frame_type'  and "generate header_doc" use these fast messages.
**
**	*** DEBUG: SEE IF VQ VERSIONS OF THESE EXIST.
*/
F_FG0001_MenuFrame	"Menu"
F_FG0002_BrowseFrame	"Browse"
F_FG0003_AppendFrame	"Append"
F_FG0004_UpdateFrame	"Update"

/*
**	fghdrdoc.c
**	These are format strings used by the CL routine: SIfprintf.
**	They control the format of output from the "## generate header_doc"
**	statement.
*/
F_FG0007_FrameName	"%s         Frame: %s\n"
F_FG0008_FormName       "%s          Form: %s\n"
F_FG0009_SourceFile	"%s   Source File: %s\n"
F_FG000A_FrameType	"%s    Frame Type: %s\n"
F_FG000D_Date		"%sDate Generated: %s\n"

/*
**	fgdefmnu.c
*/
E_FG002F_Defmnu_NoDynMem "Insufficient dynamic memory is available.\n
Please report this internal problem to your technical representative."

F_FG000B_Menu		"Menu"

/*
**	fgchkchg.c
*/
E_FG0032_MissingArgs "Error occurred while generating 4gl code\n
Missing argument on a ## statement.
Error: the '## GENERATE CHECK_CHANGE' statement on line '%0d' of file '%1c'
does not have an argument following the word 'check_change' to
tell what type of change-checking code to generate. This is an
error in your template file. It must be corrected before code generation
can continue."

E_FG0033_UnknownArg "Error occurred while generating 4gl code\n
Unknown argument on a ## statement.
Error: the '## GENERATE CHECK_CHANGE' statement on line '%0d' of template file
'%1c' has the word '%2c' following the word 'check_change'.  '%2c' is not a
valid argument to the '## GENERATE CHECK_CHANGE' statement.  This is an
error in your template file. It must be corrected before code generation
can continue."

/*
**	These comments are written to the generated 4gl
**	file in fgchkchg.c. They describe the series of statements in an
**	Update frame that check for a change to join fields.
**	(## generate check_change)
*/
F_FG000E_CheckChngComnt "/* check for change to join field(s) */"
F_FG000F_JoinFldChanged "/* a join field was changed */"

/*
**	fgundchg.c
*/
E_FG0034_MissingArgs "Error occurred while generating 4gl code\n
Missing argument on a ## statement.
Error: the '## GENERATE COPY_HIDDEN_TO_VISIBLE' statement on line '%0d'
of file '%1c' does not have an argument following the word
'copy_hidden_to_visible'. That argument is required to tell what type
of information to copy. This is an error in your template file.
It must be corrected before code generation can continue."

E_FG0035_UnknownArg "Error occurred while generating 4gl code\n
Unknown argument on a ## statement.
Error: the '## GENERATE COPY_HIDDEN_TO_VISIBLE' statement on line '%0d'
of template file '%1c' has the word '%2c' following the word
'COPY_HIDDEN_TO_VISIBLE'.  '%2c' is not a valid argument to the
'## GENERATE COPY_HIDDEN_TO_VISIBLE' statement.  This is an
error in your template file. It must be corrected before code generation
can continue."

/*
**	This comment is written to the generated 4gl file in fgundchg.c.
**	It describes the series of statements in an Update frame that
**	undo a change to join field(s).  (## generate undo_change)
*/
F_FG0010_UndoChngComment "/* undo changes made to join field(s) */"

/*
**	fgfixupf.c
*/
E_FG0036_NoFormToFixup 	"Error(s) loading form.\n
Errors occurred attempting to load form '%0c'.  Contact your Site Technical
Contact for assistance."

E_FG0037_FixupMemErr	"Not enough memory available to modify '%0c'\n
Insufficient dynamic memory was available to modify the form.
This usually indicates that your process or system ran out of 
memory, or tried to exceed a memory limit quota. Take whatever 
steps you can to free up more memory for your use and then try 
this again. Contact your Site Technical Contact for assistance."

E_FG0038_NotVisionForm "Attempt to modify user-created form '%0c'\n
From '%0c' does not contain restricted field '%1c'.  Thus it is not
a version of the form created by INGRES for this frame. 
The visual query changes cannot be applied to it. You may decide 
either to overwrite it completely or to abort form creation."

S_FG0039_GenNewForm 	"Do you want to overwrite form '%0c'?"
S_FG003A_TableHeaderMsg	"Fields from table '%0c':"
S_FG0040_ShortRemark1	"Frame '%0c' in Application '%1c'"
S_FG0041_ShortRemark2	"Frame '%0c'"
S_FG0042_LongRemark	"Form Created by INGRES for Frame '%0c' in Application '%1c'"

E_FG003B_Back_to_fullscreen "Form '%0c' changed to Fullscreen\n
The modifications to the form have made it bigger than the screen.
This makes it impossible for the form to remain a Popup.
The form is being given the Fullscreen attribute."

E_FG003C_NameConflicts	"Form '%0c' not produced due to conflicts\n
Trying to apply the Visual Query changes to the form produced field
name conflicts.  Please rename the user-created fields which caused
the conflicts and try again."

E_FG003D_TfNameConflict	"Tablefield '%0c' causes a name conflict\n
The Visual Query marks '%0c' as a displayed column, but the form 
contains a user-created tablefield of this name.  Please rename the
tablefield and try again."

E_FG003E_FldNameConflict "Field '%0c' causes a name conflict\n
The Visual Query marks '%0c' as a displayed column, but the form 
contains a user-created field of this name.  Please rename the
user-created field and try again."

E_FG003F_ColNameConflict "Column '%0c' causes a name conflict\n 
The Visual Query marks '%0c' as a displayed column, but the form 
contains a user-created column of this name.  Please rename the
user_created column and try again."

E_FG0060_FormProblems	"Problems with form '%0c'.\n
The previous error messages indicate problems which exist with the
newly created version of form '%0c'.  You can correct these problems by 
editing the form."

/*
** fgcpvtoh.c 
** 2/90 NOTE: this file not needed (yet). Probably won't be needed in
** first release of product. These messages left in because code was
** partially written and these are used in the partially written file.
** The source file was added to the project but with comments that it isn't
** used yet. It isn't linked into the executable.
*/
E_FG0054_MissingArgs "Warning occurred while generating 4gl code\n
Warning: missing argument on a ## statement.
Error: the '## GENERATE COPY_VISIBLE_TO_HIDDEN' statement on line '%0d'
of file '%1c' does not have an argument following the word
'COPY_VISIBLE_TO_HIDDEN'.
Code generation will continue and the argument 'KEY_FIELDS' will
be used . . ."

E_FG0055_TooManyArgs "Warning occurred while generating 4gl code\n
Warning: unknown argument on a ## statement.
Error: the '## GENERATE COPY_VISIBLE_TO_HIDDEN' statement on line '%0d'
of template file '%1c' has the word '%2c' following the word
'COPY_VISIBLE_TO_HIDDEN'.  '%2c' is not a valid argument to the
'## GENERATE COPY_VISIBLE_TO_HIDDEN' statement.
Code generation will continue; the argument 'KEY_FIELDS', rather
than '%2c' will be used . . ."

E_FG0056_UnknownArg "Warning occurred while generating 4gl code\n
Warning: extra arguments ignored on a ## statement.
Error: the '## GENERATE COPY_VISIBLE_TO_HIDDEN' statement on line '%0d'
of template file '%1c' has additional arguments following the word
'KEY_FIELDS'.  This statement should not have arguments
after the word 'KEY_FIELDS'.
Code generation will continue and the additional arguments will be ignored. . ."

/*
** fgflddef.c 
*/
E_FG0043_InvFrameType "Warning: Invalid frame type for ## statement\n
Warning: while generating 4gl code, a '## GENERATE SET_DEFAULT_VALUES'
statement was found on line '%1d' of template file '%0c', which does not
apply for this frame type (it can only be used in
APPEND frames).  This statement will be ignored.  Please correct this error
in your template file.  Code generation will continue and this statement
will be ignored..."

E_FG0051_MissingArgs "Warning occurred while generating 4gl code\n
Warning: missing argument on a ## statement.
Error: the '## GENERATE SET_DEFAULT_VALUES' statement on line '%0d'
of file '%1c' does not have an argument following the word 'SET_DEFAULT_VALUES'.
Code generation will continue and the argument 'SIMPLE_FIELDS' will
be used . . ."

E_FG0052_UnknownArg "Warning occurred while generating 4gl code\n
Warning: unknown argument on a ## statement.
Error: the '## GENERATE SET_DEFAULT_VALUES' statement on line '%0d'
of template file '%1c' has the word '%2c' following the argument
'SET_DEFAULT_VALUES'.  '%2c' is not a valid argument to the
'## GENERATE SET_DEFAULT_VALUES' statement.
Code generation will continue; the argument 'SIMPLE_FIELDS', rather
than '%2c' will be used . . ."

E_FG0053_TooManyArgs "Warning occurred while generating 4gl code\n
Warning: extra arguments ignored on a ## statement.
The '## GENERATE SET_DEFAULT_VALUES' statement on line '%0d'
of template file '%1c' has additional words following the argument
'%2c'.  This statement should not have arguments
after '%2c'.
Code generation will continue and the additional arguments will be ignored. . ."

/*
** fgtitle.c
** Used to format title for generated form.
*/
S_FG0044_MenuFrame 	"Menu Frame"
S_FG0045_AppendFrame 	"Append Frame"
S_FG0046_UpdateFrame 	"Update Frame"
S_FG0048_BrowseFrame 	"Browse Frame"

/*
**	fglookup.c
*/
E_FG004A_LookupTooManyArgs "Warning: too many arguments on a ## statement\n
Warning: during 4gl code generation, a '## GENERATE LOOKUP' statement was
found on line '%0d' of file '%1c' which contains arguments that will be
ignored (the '## GENERATE LOOKUP'
statement does not allow arguments following the word 'LOOKUP').
Correct this error in your template file.
Code generation will continue and the extra arguments will be ignored..."

/*
**	fgdeclar.c
*/
E_FG0063_Duplicate_Var	"Cannot reDEFINE built-in $variable...\n
On line %1d of file %2c, a '##DEFINE %0c' was found, which attempts
to reset the value for the built-in $variable: '%0c'. You can not
reset the value for a built-in $variable."

E_FG0064_Syntax_Define	"Syntax error on DEFINE statement...\n
A syntax error was found in a ##DEFINE statement on line %0d of file %1c.
Correct syntax is: '##DEFINE $variable_name value_expression', where
'value_expression' is optional."

E_FG0065_Bad_Type_Define	"Bad value expression in DEFINE statement...\n
An error occurred while trying to execute the '##DEFINE %0c' statement
on line %1d of file %2c. There is probably an error in the value
expression for the $variable. The previous
error message should have given more detail about the type of error
that occurred."

E_FG0066_Bad_Var_Define	"Illegal $var name in DEFINE stmt...\n
The $variable name: '%0c', specified in the DEFINE statement
on line %1d of file %2c, is not a valid variable name.
Variable names must begin with a '$' and
following that must be alpha-numeric and underscore characters."

E_FG0067_Syntax_IF_WHILE	"Syntax error in %0c statement...\n
During 4gl code generation, a '%0c' statement was found
on line '%1d' of file '%2c', that is missing the terminating word: '%3c'.
This is an error in your template file. It must be corrected before
code generation can continue."

E_FG0068_No_Such_Variable	"Warning: $variable does not exist...\n
Warning: while generating 4gl code, a '##UNDEF %0c' statement was found
on line '%1d' of file '%2c'. There is no user-defined $variable named '%0c'.\n
This is an error in your template file.
Code generation will continue, but this ##UNDEF statement will be
ignored."

E_FG0069_Bad_Expression		"Error evaluating an expression...\n
An error occurred during 4gl code generation while trying to evaluate the
expression on line '%0d' of file '%1c'. The previous
error message should have given more detail about the type of error
that occurred. This error in your template file
must be corrected before code generation can continue."

/*
** following 5 messages are written 
** to the generated 4gl as comments about the
** code generated from the ##GENERATE LOOKUP statement.
*/
F_FG0012_SimpleField "/* cursor in simple field */"
F_FG0013_TableField  "/* cursor in table field */"

F_FG0033_SkipLookup1  "/* Skip look_up() call if table field is empty (get"
F_FG0034_SkipLookup2  "** error if read from or assign to an empty table field)"
F_FG0035_EndOfComment "*/"

/* 
** Strings used to delimit sections of 4GL code follow.
*/
F_FG0014_Hidden				"LocalVars"
F_FG0045_LocalProcs			"LocalProcs"
F_FG0015_Call				"Call"
F_FG0016_Insert				"Insert"
F_FG0017_Select				"Select"
F_FG0018_Master				"Master"
F_FG0019_Detail				"Detail"
F_FG001A_Form_Start                     "Form-Start"
F_FG001B_Form_End                       "Form-End"
F_FG001C_Query_Start                    "Query-Start"
F_FG001D_Query_Next                     "Query-Next"
F_FG002A_Query_New_Data                 "Query-New-Data"
F_FG001E_Query_End                      "Query-End"
F_FG001F_Append_Start                   "Append-Start"
F_FG0020_Append_End                     "Append-End"
F_FG0021_Update_Start                   "Update-Start"
F_FG0022_Update_End                     "Update-End"
F_FG0023_Delete_Start                   "Delete-Start"
F_FG0024_Delete_End                     "Delete-End"
F_FG0025_Menu_Start                     "Menu-Start"
F_FG0026_Menu_End                       "Menu-End"
F_FG0027_Field_Enter                    "Before-Field-Entry"
F_FG0028_Field_Change                   "After-Field-Change"
F_FG0029_Field_Exit                     "After-Field-Exit"
F_FG003C_User_Menuitems                 "User-Menuitems"
F_FG003E_Before_Lookup			"Before-Lookup"
F_FG003F_After_Lookup			"After-Lookup"
F_FG0041_On_Timeout			"On-Timeout"
F_FG0042_On_Dbevent			"On-Dbevent"
F_FG0043_Table_Field_Menuitems		"Table-Field-Menuitems"
F_FG0044_Menuline_Menuitems		"Menuline-Menuitems"
F_FG0030_Default                        "Default"
F_FG002B_Source_Code                    "Source Code"

/* Menu style names */
F_FG003D_SingleLine			"single-line"
F_FG0040_TableField			"tablefield"
