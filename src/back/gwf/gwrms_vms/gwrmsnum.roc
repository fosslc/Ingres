/*
** Copyright (c) 1990, Ingres Corporation
** All Rights Reserved
*/

#include	<compat.h>

/**
**
**  Name: GWRMSNUM.ROC - Readonly table for converting numeric string types
**
**  Description:
**	This file defines a table which allows easy conversion to/from all the
**	VAX numeric string types.
**
**	For the first table:
**	The array index will be the value of the last byte of the numeric
**	string (zoned, overpunch, or unsigned), and the array value is the
**	digit it should be transformed to followed by the sign that should be
**	used.  Note, by using blank for the sign generated from digits 0-9,
**	this all works for leading sign numeric too, since the blank is stuck
**	in front of the actual sign byte {'+', '-', or ' ') and is then
**	ignored in the conversion.
**
**	For the second table:
**	The array index is the last byte of a packed decimal number and the
**	value is the last byte of the trailing numeric number (zoned, overpunch,
**	or unsigned).
**
{@func_list@}...
**
**
**  History:
**	06-apr-90 (jrb)
**	    Created.
**	19-jun-90 (edwin)
**	    Add note in re why this works for leading sign numeric too.  Change
**	    value returned for invalid byte from hex zero to 'X'.
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
[@function_definition@]...
*/

/*
**  Note the bytes that match the first and last elements of this array
**  (0x30 and 0x7d) are used in #define's in gwrmsdtmap.h -- change those
**  #define's (of RMS_8NUMSTR_MIN and RMS_9NUMSTR_MAX) too if you change
**  the size of this array.  If you change the character for invalid byte
**  from 'X' to something else, change the #define of RMS_7NUMSTR_BAD in
**  gwrmsdtmap.h.
*/

GLOBALCONSTDEF	char	Rms_numstr_map[][2] =
    {
/* 0x30 */
	{'0', ' '}, {'1', ' '}, {'2', ' '}, {'3', ' '}, {'4', ' '},
    	{'5', ' '}, {'6', ' '}, {'7', ' '}, {'8', ' '}, {'9', ' '},
	{'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'X', 'X'},
	{'X', 'X'}, {'X', 'X'}, {'1', ' '}, {'2', ' '}, {'3', ' '},
	{'4', ' '}, {'5', ' '}, {'6', ' '}, {'7', ' '}, {'8', ' '},
	{'9', ' '}, {'1', '-'}, {'2', '-'}, {'3', '-'}, {'4', '-'},
	{'5', '-'}, {'6', '-'}, {'7', '-'}, {'8', '-'}, {'9', '-'},
/* 0x53 */
	{'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'X', 'X'},
	{'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'X', 'X'},
	{'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'X', 'X'},
	{'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'X', 'X'},
	{'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'X', 'X'},
	{'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'X', 'X'}, {'0', '-'},
/* 0x71 */
	{'1', '-'}, {'2', '-'}, {'3', '-'}, {'4', '-'}, {'5', '-'},
	{'6', '-'}, {'7', '-'}, {'8', '-'}, {'9', '-'}, {'X', 'X'},
	{'0', ' '}, {'X', 'X'}, {'0', '-'}
    };

GLOBALCONSTDEF	char	Rms_deczon_map[] =
    {
/* high nybble:
**      0  1  2  3  4  5  6  7  8  9    A     B     C     D     E     F    */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x30, 0x70, 0x30, 0x70, 0x30, 0x30,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x31, 0x71, 0x31, 0x71, 0x31, 0x31,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x32, 0x72, 0x32, 0x72, 0x32, 0x32,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x33, 0x73, 0x33, 0x73, 0x33, 0x33,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x34, 0x74, 0x34, 0x74, 0x34, 0x34,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x35, 0x75, 0x35, 0x75, 0x35, 0x35,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x36, 0x76, 0x36, 0x76, 0x36, 0x36,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x37, 0x77, 0x37, 0x77, 0x37, 0x37,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x38, 0x78, 0x38, 0x78, 0x38, 0x38,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x39, 0x79, 0x39, 0x79, 0x39, 0x39
    };

GLOBALCONSTDEF	char	Rms_decovr_map[] =
    {
/* high nybble:
**      0  1  2  3  4  5  6  7  8  9    A     B     C     D     E     F    */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7b, 0x7d, 0x7b, 0x7d, 0x7b, 0x7b,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x41, 0x4a, 0x41, 0x4a, 0x41, 0x41,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x42, 0x4b, 0x42, 0x4b, 0x42, 0x42,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x43, 0x4c, 0x43, 0x4c, 0x43, 0x43,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x44, 0x4d, 0x44, 0x4d, 0x44, 0x44,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x45, 0x4e, 0x45, 0x4e, 0x45, 0x45,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x46, 0x4f, 0x46, 0x4f, 0x46, 0x46,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x47, 0x50, 0x47, 0x50, 0x47, 0x47,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x48, 0x51, 0x48, 0x51, 0x48, 0x48,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x49, 0x52, 0x49, 0x52, 0x49, 0x49
    };
