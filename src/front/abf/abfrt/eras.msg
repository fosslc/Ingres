/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    eras.msg -	ABF Run-Time Main Line
**
** Description:
**	This file defines the messages for class AS.
**
** History:
**	12-aug-1987 (peter)
**		Created.
**	5-aug-91 (blaise)
**		Added E_AS0009_BadArgument and E_AS0010_R_FlagSyntax.
**/

# define	_AS_CLASS	42



E_AS0001_TooManyArgs
"The command line contained too many (%0d) arguments."

/*%
** Explanation:
**
** System Status:
**	The application will continue.
**
** User Action:
*/

E_AS0002_ExtraFrame
"The extra frame or procedure name `%0c' was ignored."

/*%
** Explanation:
**	More than one frame or procedure was specified on the command-line
**	to the application.  The extra ones will be ignored.
**
** System Status:
**	The application will continue.
**
** User Action:
*/

E_AS0003_FrameRequired
"Illegal call of application.  You must specify the first frame or procedure to call on the command-line when calling an application."

/*%
** Explanation:
**	If the application was not defined with a default first frame (or
**	procedure,) the command line must specify the first frame or procedure
**	to call.
**
** System Status:
**	ABF will exit after issuing the error message.
**
** User Action:
**	Re-issue the command, including the name of the first frame or
**	procedure in the application to be called.  Please refer to the
**	"INGRES ABF/4GL Reference Manual"
*/

E_AS0004_No_ING_ABFDIR
"The INGRES name, ING_ABFDIR, is not defined.\n
The environment logical, ING_ABFDIR, must be set to the path of the directory
where ABF can find the application constants for this application. Please
see your INGRES system manager."

/*%
** Explanation:
**	This application needs to find the files which contain the constant
**	strings for the current language.  The environment logical ING_ABFDIR
**	should point to the head of the directory tree which contains
**	these files.
**
** System Status:
**	The application will exit after issuing the error message.
**
** User Action:
**	Define ING_ABFDIR and rerun the application.
*/

E_AS0005_Bad_language
"'%0c' is not a supported language.\n\
The language specified by the II_LANGUAGE or II_APPLICATION_LANGUAGE
environment logical is not a language supported by INGRES.  Please check
the values of these logicals and rerun the application."

/*%
** Explanation:
**	The application has been asked to run with an unsupported language.
**	This language was set in the environment logical II_LANGUAGE or
**	II_APPLICATION_LANGUAGE.
**
** System Status:
**	The application will exit after issuing the error message.
**
** User Action:
**	Correct the values of the II_LANGUAGE and II_APPLICATION_LANGUAGE
**	environment logicals and rerun the application.
*/

E_AS0006_Cant_Open_Constants
"Cannot open application constants file.\n\
The file which contains the application constants cannot be opened.  This
file should exist in a subdirectory of the ING_ABFDIR directory.  Please
see the Ingres Forms-based Application Development Tools User's Guide for more information about this file."

/*%
** Explanation:
**	The file containing the application constants cannot be opened.
**	This file should exist in a subdirectory of the ING_ABFDIR directory.
**	The Ingres Forms-based Application Development Tools User's Guide
**      contains details on locating and recreating this file.
**
** System Status:
**	The application will exit after issuing the error message.
**
** User Action:
**	Put the file in the proper directory and rerun the application.
*/

E_AS0007_Cant_Read_Constants
"Cannot read application constants file.\n\
The file '%0c',  which contains the application constants, cannot be read.  
Be sure that it exists, and that its file permissions allow you access to it."

/*%
** Explanation:
**	The file containing the application constants cannot be read.
**	Check that it exists and is readable.
**
** System Status:
**	The application will exit after issuing the error message.
**
** User Action:
**	Get the file or change its permissions and rerun the application.
*/

E_AS0008_Wrong_File_Version
"The wrong version of the application constants file was found.\n\
The file '%0c',  which contains the application constants, is not the
expected version.  Please replace it with the proper version, which was created
when the application image was created."

/*%
** Explanation:
**	The file containing the application constants is not the proper 
**	version.  The application image and constants files are created at the
**	same time, and must match.
**
** System Status:
**	The application will exit after issuing the error message.
**
** User Action:
**	Find the proper version of the constants file and rerun the
**	application.
*/


E_AS0009_BadArgument
"%0c: Bad argument."

E_AS0010_R_FlagSyntax
"Correct Syntax:\nimagename [ -Rroleid ] [ flags ]"
/*%
** Explanation:
**	The user tried to specify a role and password when starting up an
**	imaged application, by using the illegal syntax "-R<role_id>/<passwd>".
**
** System Status:
**	The application will exit after issuing this error message.
**
** User Action:
**	Specify the role, using "-R<role_id>", then wait to be prompted for a
**	password.
*/
E_AS000A_BadConstantFile
"File <%0c> is not properly formatted. The correct format can be seen
after creating the global constants file from the ABF's utility screen."
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
E_AS000B_NameToLoc
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
E_AS000C_BadConstant
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
E_AS000D_AllocFail
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
E_AS000E_IntConvFailed
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
E_AS000F_FloatConvFailed
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
E_AS0011_PackConvFailed
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
