/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    ersm.msg -	symbol objects message file.
**
** Description:
**
** History:
**	13-sep-1989 (Joe)
**	    First written
**	6-may-1991 (jab)
**	    Added a sentence that makes errors 000A and 000D 
**          a little less frightening, since they come up during normal
**          'importing' of an application.
** 13-0ct-92 (elein)
**     Added 'report' as an object type
** 26-oct-92 (pete)
**	Added F_SY000C_SCRIPTSRC for use in symscrip.c.
** 05-oct-95 (lan) Bug 70520
**	Fix grammar error in message E_SY0007_BADNAMESTART.
** 26-jan-1999 (donc) Bug 95016
**      Make the E_SY000C message more informative if in the context
**	of a External Class Library.
** 04-Aug-2000 (yeema01) Bug 102264
**      Changed the SY message class to SM to avoid conflict with Rosetta.
**/

#define	_SM_CLASS	197

E_SM0000_BADARRTYPE	"The type for an array's rows is invalid\n
The type for an array's row must be class.  An array is trying to be
created whose row's type is not a class.  This is invalid."

/*%
** Explanation:
**	Arrays in Sapphire can only contain rows whose elements are
**	reference variable.  This means the type of the row must be
**	a class.  An attempt has been made to build an array whose
**	row's type is not a class and this is illegal.
**
** System Status
**	The system behavior depends on the context this is done in.
**	If 4GL code, a compile time error will be generated.  In
**	the user interface, a specific error will be generated.
**
** User Action:
**	Declare the array so that its row type is a class.
*/

E_SM0001_TYPETOOLONG	"Type unknown or typename too long\n
The name given for a datatype is not a known class name, and is too
long to be the name of a simple type."

/*%
** Explanation:
**	The string the user gave for a datatype was too long to
**	be a valid scalar datatype, and the name was not found
**	to be a class name.  This means the type is invalid.
**
** System Status
**	The system behavior depends on the context this is done in.
**	If 4GL code, a compile time error will be generated.  In
**	the user interface, a specific error will be generated.
**
** User Action:
**	Use a valid type name.
*/

E_SM0002_OBJDEFSTR   
"Illegal setting for DefaultValue for a reference variable.\n
The DefaultValue for the reference variable '%0c' was set to a string.
A reference variable can not have a string for a DefaultValue.  It can
only have the System default or NULL.  Please change the setting of the
DefaultValue."

/*%
** Parameters:
**	0		The name of the variable.
**
** Explanation:
**	The setting of DefaultValue for a variable is illegal for a reference
**	variable.  It was set to a string value, but a reference variable can
**	not have a string default value.  It can only have a System default
**	or a default value of NULL.
**
** System Status:
**	The variable can not be created since it has an illegal setting.
**
** User Action:
**	Change the default value to one of the allowed ones.
*/

S_SM0003_NONAME		"<noname>"

/*%
** Explanation:
**	Some of the errors that include the name of a variable will use
**	this if the variable was not given a name.
*/

E_SM0004_NULLDEF	"NULL default value given to non-NULLable variable.\n
The non-NULLable variable '%0c' was given the default value NULL.  This
is illegal since the variable can not contain the NULL value.  The type of
the variable is '%1c'."

/*%
** Parameters:
**	0		The name of the variable.
**
**	1		The type of the variable.
**
** Explanation:
**	A variable whose type is non-NULLable was given a NULL value as
**	its default value.  This is illegal since the variable can not
**	contain the NULL value.
**
** System Status:
**	The variable can not be created since it has a bad setting.
**
** User Action:
**	Change the DefaultValue to a something other than NULL.
*/

E_SM0005_DEFSTRTOOLONG	"The string for DefaultValue is too long.\n
The variable '%0c' was given a string for its default value that is too
long.  The string has '%1d' characters but only '%2d' characters are
allowed."

/*%
** Parameters:
**	0		The name of the variable.
**
**	1		The number of characters in the string.
**
**	2		The number of characters allowed in the string.
**
** Explanation:
**	The string given for the default value for a variable was too
**	long.
**
** System Status:
**	The variable will not be created since it has an error.
**
** User Action:
**	Shorten the default value.
*/

E_SM0006_EMPTYNAME	"A variable was not given a name.\n
A variable that is being created does not have a name.  The variable
must be given a name."

/*%
** Explanation:
**	A variable is being created, but the variable has not been
**	given a name.
**
** System Status:
**	The variable will not be created.
**
** User Action:
**	Give the variable a name.
*/

E_SM0007_BADNAMESTART	"Bad first character for a name.\n
A variable was given the name '%0c'.  The starting character of this
name is illegal.  Check your INGRES/SQL Reference Manual for INGRES naming
conventions."

/*%
** Parameters:
**	0		The variable name that is bad.
**
** Explanation:
**	The name given to a variable is illegal.  The name does not
**	begin with a character that can start a name.
**
** System Status:
**	The variable can not be created.
**
** User Action:
**	Correct the name.
*/

E_SM0008_NAMETOOLONG	"Variable name is too long.\n
A variable was given the name '%0c'.  This name is too long.  It has
%1d characters while a name can only contain up to %2d characters."

/*%
** Parameters:
**	0		The variable name that is bad.
**
**	1		The number of characters in the name.
**
**	2		The maximum number of characters allowed
**			in a name.
**
** Explanation:
**	The name given to a variable is illegal.  The name contains
**	too many characters.
**
** System Status:
**	The variable can not be created.
**
** User Action:
**	Shorten the name.
*/

E_SM0009_BADNAME	"Illegal name given to a variable.\n
A variable was given the name '%0c'.  This name is illegal.  Check the
INGRES documentation for the rules governing names in INGRES."

/*%
** Parameters:
**	0		The variable name that is bad.
**
** Explanation:
**	The name given to a variable is illegal.  It contains a
**	character that is not allowed in name.
**
** System Status:
**	The variable can not be created.
**
** User Action:
**	Correct the name.
*/

E_SM000A_BADTYPE	"Bad type given to a variable.\n
The type '%1c' given to the variable '%0c' is not known to the system
or is incorrectly specified. (If importing an application, a type name
might have been used before being declared, which is dealt with later
in the import process, and is a minor error.) A subsequent error
may give more information.  If not, check the INGRES documentation for
the legal type names."

/*%
** Parameter:
**	0		The name of the variable.
**
**	1		The name of the type that is bad.
**
** Explanation:
**	The type given to a variable is not known to the system.
**
** System Status:
**	The variable can not be created.
**
** User Action:
**	Enter a correct type name.
*/

E_SM000B_BADDEFVALUE	"Bad default value string for a variable.\n
The string '%1c' given as the default value for the variable '%0c'
is bad.  It can not be converted into a legal value for the type of the
variable which is '%2c'."

/*%
** Parameters:
**	0		The name of the variable.
**	
**	1		The string to be used as the default value.
**
**	2		The type of the variable.
**
** Explanation:
**	The string given as the default value for a variable is not
**	legal for the type of the variable.
**
** System Status:
**	The variable can not be created.
**
** User Action:
**	Correct the value of the string for the default value.
*/

E_SM000C_NAMENOTTYPE	"Name given as type is not a TYPE.\n
The name '%0c' was used in a declaration as a type name.  However the
component with the name '%0c' is a '%1c', so it can't be used in a type
declaration.  If the name given corresponds to an element of an
External Class Library, you should prepend the name with the External Class
Library name followed by the '!' operator"

/*%
** Parameters:
**	0		The name that was supposed to be a type.
**
**	1		The kind of component that was found.
**			this will be one of:
**					    F_SM0001_VARIABLE
**					    F_SM0002_CONSTANT
**				            F_SM0003_PROCEDURE
**				            F_SM0004_FRAME
**					    F_SM0005_ADFOP
**					    F_SM0006_LABEL
**					    F_SM0007_CLASS
**					    F_SM0008_SPECFUNC
**					    F_SM0009_SYMBOL
**					    F_SM000A_FIELD
**					    F_SM000B_REPORT
**
** Explanation:
**	A name that was used in a declaration (e.g. var = type_name) was
**	looked up in the symbol table and found, but the component that
**	was found was not a type.  Thus this name can't be used as a type
**	name.
**
** System Status:
**	The variable can not be created.
**
** User Action:
**	Give a correct type name or if the type is present but obscured
**	by a definition with the same name, fully qualify the name.
*/

E_SM000D_ADF_TYPE	"Type '%0c' is unknown.\n
The type specification '%0c' was used in a variable declaration.  This
specification is unknown to the system. (If importing an application,
a type name might have been used before being declared, which is
dealt with later in the import process, and is a minor error.)"

/*%
** Parameters:
**	0		The type specification that is bad.
**
** Explanation:
**	A type specification given in a variable declaration uses a
**	type name that is unknown.
**
** System Status:
**	The variable will not be declared.
**
** User Action:
**	Correct the variable declaration by giving a known type.
*/

E_SM000E_ADF_LEN	"Bad length given in type '%0c'.\n
The type specification '%0c' was used in a variable declaration.  The length
given in this specification is incorrect for the type.  Consult the Ingres
documentation to find the valid values for the length of this type."

/*%
** Parameters:
**	0		The type specification that is bad.
**
** Explanation:
**	A type specification given in a variable declaration has an
**	incorrect length value for the type being given.
**
** System Status:
**	The variable will not be declared.
**
** User Action:
**	Correct the variable declaration by giving a legal length value.
*/

E_SM000F_REDECL		"Redeclaration of '%0c'.\n
The name '%0c' is being redeclared.  There is already a
%1c in this scope named '%0c'."

/*%
** Parameters:
**	0		The name that is being redeclared.
**
**	1		One of the E_SMXXX strings below.
**
** Explanation:
**	A same name is being used in two different declarations.
**
** System Status:
**	The second usage will be ignored.
**
** User Action:
**	Change the name of one of the objects so that the name is
**	unique.
*/

E_SM0010_KEYWORD	"Reserved word used as name.\n
A variable (or other kind of object) was given the name '%0c' which is
a reserved word.  You can not use a reserved word to name an object."

/*%
** Parameters:
**	0		The variable name that is bad.
**
** Explanation:
**	The name given to a variable is illegal.  The name is the
**	same as a reserved word.
**
** System Status:
**	The variable can not be created.
**
** User Action:
**	Correct the name.
*/

E_SM0011_RESTYPE	"Can't get type information for a symbol.\n
The type information for the %1c '%0c' is incorrect.  A subsequent
error may give more information about what is wrong with the type information.
Normally this occurs when a user class has been deleted.  If this is the
case, the user class must be recreated."

/*%
** Parameters:
**	0		The name of the symbol that had a problem.
**
**	1		The kind of symbol this is.  This is one
**			of the F_SY values below.
**
** Explanation:
**	Some type information that is needed for a symbol was not
**	found or contains some problems.  This can sometimes happen
**	when a user class is deleted.
**
** System Status:
**	The system will continue, but may encounter problems later.
**
** User Action:
**	If the problem is a user class that has been destroyed,
**	then you must create the user class again to get things
**	working
*/ 

E_SM0012_CHILDRES	"Errors found in children of '%0c'.\n
The information in the children of the %1c '%0c' is incorrect.
A subsequent error will give more information."

/*%
** Parameters:
**	0		The name of the symbol that had a problem.
**
**	1		The kind of symbol this is.  This is one
**			of the F_SY values below.
**
** Explanation:
**	There was a problem with information about the children of
**	a symbol.
**
** System Status:
**	The system will continue, but may encounter problems later.
**
** User Action:
**	See the specific errors from the children to determine what
**	to do.
*/ 

E_SM0013_GLOBPKG	"Can not find application for a global object.\n
There was an error trying to find the scope of application '%0c' which
contains the global object '%1c'.  A previous error should explain where
this global object is used.  You should recompile the object that uses
this global object."
/*%
** Parameters
**	0		This the name of the application that is
**			supposed to contain the global object.
**
**	1		This is the name of the global object that is
**			being searched for.
**
** Explanation:
**	The system could not find an application that is supposed to
**	contain a global object used by another object.  For example,
**	the system might be looking for a global variable that is used
**	in a frame.
**
** System Status:
**	The system will continue, but may encounter problems later.
**
** User Action:
**	Try recompiling the object that uses this global object.  If
**	that doesn't help, contact your Ingres Representative.
*/

E_SM0014_GLOBFIND	"Can not find a global object.\n
There was an error trying to find the global object '%1c' which is
contained in the application '%0c'.  A previous error should explain where
this global object is used.  You should recompile the object that uses
this global object."
/*%
** Parameters
**	0		This the name of the application that is
**			supposed to contain the global object.
**
**	1		This is the name of the global object that is
**			being searched for.
**
** Explanation:
**	The system could not find a global object that is used by
**      another object.  For example, the system might be looking
**      for a global variable that is used in a frame.
**
** System Status:
**	The system will continue, but may encounter problems later.
**
** User Action:
**	Try recompiling the object that uses this global object.  If
**	that doesn't help, contact your Ingres Representative.
*/

E_SM0015_NOPARENT	"Bad type information in symbol.\n
The %1c '%0c' contains bad type information.  This is an internal error
that represents a problem in Windows 4GL.  Please contact your Ingres
Representative."
/*%
** Parameters:
**	0		The name of the symbol that has a problem.
**
**	1		The kind of symbol this is.  This will be
**			one of the F_SY strings below.
** Explanation:
**	There is a problem in Windows 4GL.
**
** System Status:
**	The system will continue, but may encounter problems later.
**
** User Action:
**	Contact your Ingres Representative.
*/

E_SM0016_NOTARRAY	"Bad type information in symbol.\n
The %1c '%0c' contains bad type information. This is an internal error
that represents a problem in Windows 4GL.  Please contact your Ingres
Representative.  The type is supposed to be an array or object, but it is not."
/*%
** Parameters:
**	0		The name of the symbol that has a problem.
**
**	1		The kind of symbol this is.  This will be
**			one of the F_SY strings below.
** Explanation:
**	There is a problem in Windows 4GL.
**
** System Status:
**	The system will continue, but may encounter problems later.
**
** User Action:
**	Contact your Ingres Representative.
*/

E_SM0017_GETSET		"Bad type information in symbol.\n
The %1c '%0c' contains bad type information.  This is an internal error
that represents a problem in Windows 4GL.  Please contact your Ingres
Representative.  There is not %3c accessor method for the class %2c"
/*%
** Parameters:
**	0		The name of the symbol that has a problem.
**
**	1		The kind of symbol this is.  This will be
**			one of the F_SY strings below.
**
**	2		The name of the class that has the problem.
**
**	3		This will be set or get.
**
** Explanation:
**	There is a problem in Windows 4GL.
**
** System Status:
**	The system will continue, but may encounter problems later.
**
** User Action:
**	Contact your Ingres Representative.
*/

E_SM0018_BADATTR	"Attribute is unknown.\n
The %1c '%0c' is associated with an attribute that no longer exists.
The attribute '%0c' is supposed to be defined is the class '%2c'.
However, there is no attribute named '%0c' in the class '%2c'.  A
previous error should explain which object this %1c is contained in.
This error condition can come about if an attribute that is used by
the application is deleted.  You either need to get rid of all places that
use this attribute, or you need to recreate the attribute."
/*%
** Parameters:
**	0		The name of the attribute that can't be found.
**
**	1		The kind of symbol this is.  This will be
**			one of the F_SY strings below.
**
**	2		The name of the class that this attribute is
**			supposed to be in.
**	
** Explanation:
**	A variable or field is associated with an attribute of a class
**	that no longer exists.  This can happen if an attribute is removed
**	from a user class, but not all users of the attribute are changed.
**
** System Status:
**	The system will continue.
**
** User Action:
**	Either recreate the attribute or change this symbol so it no longer
**	uses the attribute.
*/


/*
** The following are the kinds of components known in the system.
** The names used here should match the names used in the document.
*/
F_SM0001_VARIABLE	"variable"
F_SM0002_CONSTANT	"constant"
F_SM0003_PROCEDURE	"procedure"
F_SM0004_FRAME		"frame"
F_SM0005_ADFOP		"built-in function"
F_SM0006_LABEL		"loop label"
F_SM0007_CLASS		"class"
F_SM0008_SPECFUNC	"special function"
F_SM0009_SYMBOL		"symbol"
F_SM000A_FIELD		"FieldObject"
F_SM000B_REPORT		"Report"
F_SM000C_SCRIPTSRC	"ScriptSource"
F_SM000D_LIBRARY	"External Class Library"

/*
**	mscopes.c
*/

E_SM0019_SCOPES_DECODE_BAD_V	"Bad version # %d found while decoding a symbol scope.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a scope/symbol.
**	This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	mscopesy.c
*/

E_SM001A_SCOPESYMTAB_DECODE_BV	"Bad version # %d found while decoding a symbol table.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol
**	table.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msymadfo.c
*/

E_SM001B_SYMADFOP_DECODE_BV	"Expression operator at %x (named %s) has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding an expression
**	operator.  This is a fatal error.
**
** System Status:
**	Winodws4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msymcomp.c
*/

E_SM001C_SYMCOMPFIELD_DCODE_BV	"The symbol for a subfield bound to a computed attribute (at %x and named %s) has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol
**	for a subfield bound to a computed attribute.  This is a fatal
**	error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msymcons.c
*/

E_SM001D_SYMCONSTANT_DECODE_BV	"The symbol for a symbolic constant (at %x and
named %s) has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol for
**	a symbolic constant.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msymexec.c
*/

E_SM001E_SMMEXECUTABLE_DCDE_BV	"The symbol for an executable object (at %x and named %s) has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol for
**	an executable object.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msymfiel.c
*/

E_SM001F_SYMFIELD_DECODE_BV	"The symbol for a field (at %x and named %s) has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol for
**	a field on a form.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msymfram.c
*/

E_SM0020_SYMFRAME_DECODE_BV	"The symbol for a frame (at %x and named %s) has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol for
**	a frame.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msymfrmv.c
*/

E_SM0021_SYMFRMVAR_DECODE_BV	"The symbol for the current frame/procedure variable (at %x and named %s) has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol for
**	the special built-in current frame and current procedure variables.
**	This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msymlabe.c
*/

E_SM0022_SYMLABEL_DECODE_BV	"The symbol for a label (at %x and named %s) has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol for
**	a label.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msymobj.c
*/

E_SM0023_SYMBOLOBJECT_DCODE_BV	"The symbol for an object (at %x and named %s) has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol for
**	an object.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msymproc.c
*/

E_SM0024_SYMPROCEDURE_DCODE_BV	"The symbol for a procedure (at %x and named %s) has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol for
**	a procedure.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msymsfun.c
*/

E_SM0025_SYMSPECFUNC_DECODE_BV	"The symbol for a function (at %x and named %s) has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol for
**	a built-in function.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msymtype.c
*/

E_SM0026_SYMTYPE_DECODE_BV	"The symbol for a TYPE (at %x and named %s) has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol for
**	a TYPE (CLASS or ARRAY).  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msymvar.c
*/

E_SM0027_SYMVARIABLE_DECODE_BV	"The symbol for a variable (at %x and named %s) has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol for
**	a variable.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	msyusegl.c
*/

E_SM0028_SYMUSEGLOB_DECODE_BV	"The symbol for a scope tied to a global variable (at %x and named %s) has version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a symbol for
**	a scope that uses a global variable.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
**	mvardesc.c
*/

E_SM0029_VARDESCRIPTOR_DEC_BV	"The variable descriptor at %x has unsupported version # %d.\n"

/*%
** Explanation:
**	Windows4gl found an unsupported version while decoding a variable
**	descriptor.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check that the image they are trying to run is compatible
**	with the version of windows4gl they are using.
*/

/*
** msyssym.c
*/

E_SM002A_BADSYSTEMAPP
"Cannot load the IISYSTEM application\n
The IISYSTEM application cannot be loaded.  The predefined components
contained in this application will not be available.  Please report this error
to your site technical contact.  Further information about why IISYSTEM
could not be loaded may follow."

/*%
** Explanation:
**	OpenROAD is unable to find out the type information for the class
**	of a property or method.
**
** User Action:
**	User should check that the OLE library containing the type information
**	is installed on the system.
*/

/*
** woof!ntole!oleimpl.c
*/
E_SM002B_UNKNOWN_EXTCLASS
"Unable to resolve the type information of a property or method belonging
to the class '%0c.'  The property/method in question is of type '%1c,'
which OpenROAD is unable to resolve.  The problem might be solved
by installing the OLE library that implements the class '%1c.'"

