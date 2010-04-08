/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ermc.msg -	Message file for class MC
**
** Description:
**	This file defines the messages for class MC.
**
**/

# define	_MC_CLASS	180

E_MC0001_NOEFILE	"%0c: No error file specified with -e option."

/*%
** Explanation:
**	-e option was used without a filename attached.
**
** System Status:
**	program exits.
**
** User Action:
**	Use option correctly.
*/

E_MC0002_USAGE		"Usage: %0c [-e<errfile>] <character set> <file>"

/*%
** Explanation:
**	Bad command line syntax.
**
** System Status:
**	program exits.
**
** User Action:
**	Specify proper command line arguments.
*/

E_MC0003_FILEOPEN	"%0c: Can't open file `%1c' for %2c"

/*
** F_MC0002 and F_MC0004 are used for %2c parameter
*/

/*%
** Explanation:
**	A file could not be opened, either the input file or the error
**	output file.
**
** System Status:
**	program exits.
**
** User Action:
**	Address the file problem (file may not exist, permission problems,
**	etc.) and rerun command.
*/

E_MC0004_READFAIL	"%0c: Read failure on input."

/*%
** Explanation:
**	Read from the input file failed.
**
** System Status:
**	program exits.
**
** User Action:
**	Address the file i/o error and rerun the command.
*/

E_MC0005_SYNTAXERR	"%0c: Errors encountered in file `%1c'."

/*%
** Explanation:
**	Errors were encountered in the file.  More explicit messages
**	have already been produced for the individual errors.
**
** System Status:
**	program exits.
**
** User Action:
**	Fix the errors, and rerun command.
*/

E_MC0006_CASEBOUNDS	"%0c, line %1d: Translation to lower case results in
codes outside the range 0 - 255."

/*%
** Explanation:
**	A character given as an upper case character possesses a lower case
**	translation, either explicitly specified, or defaulted by adding
**	'a' - 'A', which is outside the range 0-255.
**
** System Status:
**	Error counted.  Processing continues.
**
** User Action:
**	Translate case properly.
*/

E_MC0007_BADHEX		"%0c, line %1d: Bad hexadecimal code.  `%2c' found
where a hexadecimal code was expected."

/*%
** Explanation:
**	Improper specification of a hexadecimal number.
**
** System Status:
**	Error counted.  Processing continues.
**
** User Action:
**	Specify a legal value.
*/

E_MC0008_DUPCODE	"%0c, line %1d: Hexadecimal code %2x specified
multiple times."

/*%
** Explanation:
**	A given hexadecimal code was already specified in an earlier
**	line of the file.
**
** System Status:
**	Error counted.  Processing continues.
**
** User Action:
**	Only specify a given character once.
*/

E_MC0009_BADRANGE	"%0c, line %1d: Improper use of `-' in hexadecimal
range specification."

/*%
** Explanation:
**	A hexadecimal range contained a '-' either at the front or the
**	end of the range.  "-" must appear between two hexadecimal numbers.
**
** System Status:
**	Error counted.  Processing continues.
**
** User Action:
**	Specify range properly.
*/

E_MC000A_BADORDER	"%0c, line %1d: Hexadecimal codes ordered incorrectly."

/*%
** Explanation:
**	Second number of hexadecimal range is smaller than first.  Admittedly,
**	we could just reverse these, but the syntax of this file is esoteric
**	enough that expecting people to enter ranges in order is not much of
**	an imposition.  This may catch a typo.
**
** System Status:
**	Error counted.  Processing continues.
**
** User Action:
**	Give ranges as <low code>-<high code>
*/

E_MC000B_ILLCHAR	"%0c, line %1d: Hexadecimal code outside range 0-255."

/*%
** Explanation:
**	A hexadecimal character code was not in the range 0-255.
**
** System Status:
**	Error counted.  Processing continues.
**
** User Action:
**	Specify only codes in the legal range.
*/

E_MC000C_ALPHABIT	"%0c, line %1d: Incompatible case attributes specified.
Only one of u, l and n may be used."

/*%
** Explanation:
**	Attribute bits given contained conflicting case attributes.  A
**	character can only be one of upper, lower, or non-cased.
**
** System Status:
**	Error counted.  Processing continues.
**
** User Action:
**	Specify non-conflicting attributes.
*/

E_MC000D_PRINTBIT	"%0c, line %1d: Incompatible printability
attributes specified.  The p and c attributes may not both be used."

/*%
** Explanation:
**	Attribute bits given contained both the "p" and "c" characters.
**	A character cannot be classified as both "control" and "printable".
**
** System Status:
**	Error counted.  Processing continues.
**
** User Action:
**	Specify non-conflicting attributes.
*/

E_MC000E_ILLATTR	"%0c, line %1d: Illegal attribute code - `%2c'."

/*%
** Explanation:
**	The attribute field of the line contained an illegal character.
**
** System Status:
**	Error counted.  Processing continues.
**
** User Action:
**	Specify legal attribute characters.
*/

E_MC000F_XLBAD		"Upper case hexadecimal character code %2x
translated to non lower case code %3x."

/*%
** Explanation:
**	A character classified as upper case was given a case translation
**	to a character not classified as lower case.
**
** System Status:
**	Error counted.  Processing continues.
**
** User Action:
**	Translate upper case characters ONLY to lower case.  Remember that
**	if an explicit translation is not given, one is obtained by adding
**	'a' - 'A' to the value.  If an alphabetic character has no case
**	translation, it must be classified as non-cased alpha.
*/

E_MC0010_MANYTO1	"Hexadecimal code %2x is the case translation of more
than one upper case character."

/*%
** Explanation:
**	Multiple upper case characters were translated to the same lower
**	case character.  This makes it impossible to specify the proper
**	upper case translation of that lower case character.
**
** System Status:
**	Error counted.  Processing continues.
**
** User Action:
**	Make sure that all upper case characters have distinct lower case
**	translations.
*/

E_MC0011_NOUPPER	"Lower case hexadecimal code %2x has no upper case
translation."

/*%
** Explanation:
**	A lower case character exists which is not the lower case translation
**	of any upper case character.
**
** System Status:
**	Error counted.  Processing continues.
**
** User Action:
**	Translate some upper case character to this one, or make the character
**	non-cased alpha.
*/

E_MC0012_TOOMANY	"%0c: Too many errors."

/*%
** Explanation:
** 	Too many errors have been encountered in processing the file.
**
** System Status:
**	program exits.
**
** User Action:
**	Fix the errors and rerun the program.
*/

E_MC0013_CMWRTERR	"%0c: Cannot write attributes - %1c."

/*%
** Explanation:
**	Attempt to install the attributes failed.  Parameter should be
**	a more specific error.
**
** System Status:
**	program exits.
**
** User Action:
**	Address the specific error message.
*/

E_MC0014_ABORT		"Character set not installed."

/*%
** Explanation:
**	Final message on all error exits.
**
** System Status:
**	program exits.
**
** User Action:
**	Address the earlier error messages.
*/

F_MC0001_CMDNAME	"chset_install"

F_MC0002_ERROUT		"error output"
F_MC0003_STDIN		"<stdin>"
F_MC0004_INPUT		"input"

F_MC0005_GFAIL		"unknown failure"
