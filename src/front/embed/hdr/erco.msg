SQLSTATE_MAPPING_REQUIRED

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erco.msg -	Message file for class CO
**
** Description:
**	This file defines the messages for class CO.  The result is
**	erco.h which is used to compile the iicopy.c program, and
**	s184.mnx which is used at run time by IIlocerr().
**
**	To generate erco.h and s184.mnx, issue the following VMS commands:
**
**	ercomp :== $disk$develop3:[jupiter.vtools.r.bin]ercompile.exe
**	ercomp -t erco.msg
**	copy s184.mnx disk$develop3:[jupiter.front.frontcl.rplus.files.english]
**	copy s184.mnx disk$develop2:[jupiter.ingres.files.english]
**
** History:
**	 4-sep-1987 (barbara)
**		Created.
**	20-sep-87 (puree)
**		Modified to fit the iicopy.c on VMS.
**	03-dec-90 (teresal) 
**		Added new error message for bug fix 32562.
**	08-oct-90 (teresal)
**		Modified error message E_CO0015 for bug fix 20335.
**	13-dec-90 (teresal)
**		Added new error message for bug fix 34405.
**	26-dec-90 (teresal)
**		Added new error message for bug fix 6936.
**	02-apr-91 (barbara)
**		Added error message E_CO0040_END_OF_DATA for COPY FROM PROGRAM.
**	31-oct-92 (larrym)
**		Added SQLSTATES to errors.  
**	15-mar-93 (mgw)
**		Added E_CO0041_FILE_WRITE_MSG_ERR for Large Object COPY.
**	08-apr-1993 (mgw)
**		Added E_CO0042_BLOB_READ_ERR, E_CO0043_BLOB_SEG_READ_ERR,
**		and a number of Slow messages for tailoring these and the
**		E_CO0041_FILE_WRITE_MSG_ERR msgs. I changed the parameter
**		list for E_CO0041_FILE_WRITE_MSG_ERR, a no-no under normal
**		circumstances, but OK since this error msg hasn't been
**		released to users yet.
**	19-oct-96 (mcgem01)
**		All references to MISC_ERRORS changed to MISC_ERRORS
**		for the Jasmine/Ingres code sharing project.
**      21-oct-98 (wanya01)
**              X-integrate change 436382
**              Added E_CO0048_ALLDUPS_OR_DISKFULL to indicate that 0 rows
**              were copied because either all rows were duplicates or there
**              was a disk full problem. This change fixes bug 90968.
**	20-jan-2004 (somsa01)
**		Added E_CO0049_PROG_WRITE_MSG_ERR, E_CO004A_PROG_READ_MSG_ERR.
**	8-Dec-2009 (kschendel) SIR 122974
**	    Add CO004B, CO0004 messages.
**/
# define	_CO_CLASS	184

E_CO0004_NULL_INTO_NOTNULL:SS22000_DATA_EXCEPTION
"COPY: Row %0d, domain '%1c', column value is NULL, but no
WITH NULL substitute was defined in the COPY INTO list."
/*
** Explanation:
**	NULL requires an explicit substitute in the COPY INTO list,
**	but none was defined.
**
** System Status:
**	The row is not copied.  The copy continues if the ON_ERROR=CONTINUE
**	option was set.
**
** User Action:
**	Revise the COPY list to include a WITH NULL(value) clause.
*/
E_CO0005_FILE_OPEN_ERR:SS50000_MISC_ERRORS
"COPY: can't open file '%0c'."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0006_FILE_CREATE_ERR:SS50000_MISC_ERRORS
"COPY: can't create file '%0c'."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0008_DOMAIN_SIZE:SS22000_DATA_EXCEPTION
"COPY: domain '%0c', size too small for conversion.
There were %1c rows successfully copied from '%2c' into '%3c'."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0009_BAD_INPUT:SS22000_DATA_EXCEPTION
"COPY: bad input string for domain '%0c'. Input was '%1c'.
There were %2c rows successfully copied from '%3c' into '%4c.'"
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO000A_EOF_ERR:SS50000_MISC_ERRORS
"COPY: unexpected end of file found while filling domain '%0c'
for row %1c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO000B_LEN_SPEC_ERR:SS22000_DATA_EXCEPTION
"COPY: bad varchar length specifier found while filling domain '%0c' 
for row %1c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO000F_UNTERMINATED:SS22000_DATA_EXCEPTION
"COPY: an unterminated 'c0' or 'text(0)' field occurred while filling domain
'%0c'."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0011_WIDTH_EXCEEDED:SS22000_DATA_EXCEPTION
"COPY: The maximum width of the output file cannot exceed 2008 bytes per row."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0014_BLANKED_CHARS:SS01000_WARNING
"COPY: Warning: %0c domains had control characters which were converted
to blanks."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0015_TRUNCATED_CHARS:SS01004_STRING_RIGHT_TRUNC
"COPY: Warning: %0c character domains were truncated when copied
from the file to character type columns in the table."
/*
** Explanation:
**	This error occurs on COPY FROM when a file contains longer 
**	character data than what can fit into a character type column in 
**	the table.  COPY will truncate the characters to the right and
**	issue this warning.  Note: a character domain refers to the
**	area of a file that contains the character data to be copied 
**	into a column.
**
** System Status:
**	Copy continues.
**
** User Action:
**	
*/
E_CO0017_NOTEXT:SS50000_MISC_ERRORS
"COPY: The filetype must not be 'text' when using bulk copy."
/*
** Explanation:
**	This error occurs for a bulk copy (i.e., no column
**	list was given) and the filetype specified is TEXT.  Bulk
**	copy should only be done with a binary or variable filetype.
**	A TEXT filetype will contain carriage returns and thus,
**	a bulk copy written to a TEXT file will not be able to be
**	copied back in again.  Note: filetypes are only applicable on
**	VMS.
**
** System Status:
**	Error is generated - Copy is aborted.
**
** User Action:
**	Change the filetype or don't use a bulk copy.
*/
E_CO0018_BAD_FILETYPE:SS50000_MISC_ERRORS
"COPY: '%0c' is an invalid filetype.
\nFiletype must be 'text', 'binary',or 'variable'."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0019_NOBINARY:SS50000_MISC_ERRORS
"COPY: The filetype must not be 'binary' when any of the columns are 'text(0)'."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO001A_GENERAL_ERR:SS50000_MISC_ERRORS
"COPY: Error encountered while processing row %0c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO001B_INVALID_VALUE:SS22000_DATA_EXCEPTION
"COPY: Invalid character value (numeric value %0d) found while
processing row %1c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO001D_ROWLEN_EXCEEDED:SS50000_MISC_ERRORS
"COPY: Computed row length exceeds allowable row length."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO001E_TYPE_CONVERT:SS22000_DATA_EXCEPTION
"COPY: Column '%0c', cannot convert type '%1c' to type '%2c'."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0022_COPY_INIT_ERR:SS50000_MISC_ERRORS
"COPY: Internal error initializing COPY."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0023_COPY_INTRNL_ERR:SS50000_MISC_ERRORS
"COPY: Ingres internal error processing row %0c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0024_FILE_READ_ERR:SS50000_MISC_ERRORS
"COPY: Unexpected END OF FILE while processing row %0c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0025_MEM_ALLOC_ERR:SS50003_EXHAUSTED_RESOURCE
"COPY: Error allocating memory while processing row %0c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0028_WARNINGS:SS01000_WARNING
"COPY: Warning: Copy completed with %0c warnings.  %1c rows successfully
copied."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0029_COPY_TERMINATED:SS50000_MISC_ERRORS
"COPY: Copy terminated abnormally.  %0c rows successfully copied."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO002A_COPY_ABORTED:SS50000_MISC_ERRORS
"COPY: Copy has been aborted."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO002B_BADROWS:SS50000_MISC_ERRORS
"COPY: %0c rows could not be processed."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
I_CO002C_LOGGED:SS50000_MISC_ERRORS
"COPY: %0c rows were written to the Copy Log File '%1c'."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO002F_LOG_WRITE_ERR:SS50000_MISC_ERRORS
"COPY: Error writing to Copy Log File '%0c' while processing row %1c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0030_LOG_CLOSE_ERR:SS50000_MISC_ERRORS
"COPY: Error closing Copy Log File '%0c'."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0031_FILE_CLOSE_ERR:SS50000_MISC_ERRORS
"COPY: Error closing Copy File '%0c'."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0032_BAD_DATA:SS22000_DATA_EXCEPTION
"COPY: Bad data value for column '%0c' encountered while processing row %1c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0036_LOG_OPEN_ERR:SS50000_MISC_ERRORS
"COPY: Error opening Copy Log File '%0c'."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0037_MEM_INIT_ERR:SS50003_EXHAUSTED_RESOURCE
"COPY: Error allocating memory while initializing Copy."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0038_MEM_ERR:SS50003_EXHAUSTED_RESOURCE
"COPY: Error allocating memory while processing row %0c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO0039_TUP_FORMAT_ERR:SS22000_DATA_EXCEPTION
"COPY: Error processing row %0c.  Cannot convert column '%1c' to tuple format."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO003A_ROW_FORMAT_ERR:SS22000_DATA_EXCEPTION
"COPY: Error processing row %0c.  Cannot convert column '%1c' to row format."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO003B_FILE_WRITE_ERR:SS50000_MISC_ERRORS
"COPY: Error writing to Copy File while processing row %0c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO003C_PROG_READ_ERR:SS5000L_PROTOCOL_ERROR
"COPY: Error reading row from Ingres Application while processing row %0c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO003D_PROG_WRITE_ERR:SS5000L_PROTOCOL_ERROR
"COPY: Error writing row to Ingres Application while processing row %0c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_CO003E_NOT_NULLABLE:SS22000_DATA_EXCEPTION
"COPY: Error copying null value into not nullable column '%0c'.
File contains the value specified in the WITH NULL(value) option."
/*
** Explanation:
**	The user is attemping to put null data into a column which is 
**      not nullable.  This situation occurs when data is being copied 
**	from a file into a table (COPY FROM).  If the user has specified
**	the "with null('value')" option for a not nullable column and the
**	data in the file matches 'value', this error message is displayed. 
**
** System Status:
**	Error is generated, copy is aborted unless 'on-error' or
**	'error_count' were specified.
**
** User Action:
**	Modify table/file to accept/omit the null value.
**		
*/
E_CO003F_DUPLICATES_FOUND:SS02000_NO_DATA
"COPY: Warning: %0c rows not copied because duplicate key detected."
/*
** Explanation:
**	This warning occurs on COPY FROM when a file containing duplicate
**	key data is copied into a table with a unique key; the server will
**	not add rows with duplicate key data to the table.  Note
**	that these rows will not be written to the COPY log file if
**	logging is specified; therefore, this message warns the user
**	that these rows have been dropped.
**
** System Status:
**	Copy continues.
**
** User Action:
**
*/

E_CO0040_END_OF_DATA:SS50000_MISC_ERRORS
"COPY: Unexpected end of user data while filling domain %0c for row %1c."
/*
** Explanation:
**	In a COPY FROM PROGRAM the length of the data provided by the
**	user handler is not sufficient to process the row according to the
**	formats given on the COPY statement.  There is either insufficient
**	data in user buffer handed to copy or the data formats on the COPY
**	statement do not reflect the data in the user's buffer.
**
** System Status:
**	Copy is aborted.
**
** User Action:
**	Change copy format or copy handler.  
*/
E_CO0041_FILE_WRITE_MSG_ERR:SS50000_MISC_ERRORS
"COPY: Error writing to Copy File while processing row %0c, col %1c.
%2c"
/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_CO0042_BLOB_READ_ERR:SS50000_MISC_ERRORS
"COPY: Error processing row %0c, column %1c:
Unexpected EOF reading %2c"
/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_CO0043_BLOB_SEG_READ_ERR:SS50000_MISC_ERRORS
"COPY: Error processing row %0c, column %1c:
Unexpected EOF reading %2c %3c."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_CO0044_FMT_BLOB_ORDER:SS50000_MISC_ERRORS
"COPY: Bad column order for formatted copy with large objects.
You have specified in your COPY format that column '%0c' should
come before column '%1c'. Since there is a large object column
in this table, the columns will have to be formatted in their proper
order. Please reorder the columns in your format. You can do a
'help table' on this table to see the proper order."
/*
** Explanation:
**	The columns currently only come across the wire in the order
**	in which they were created. In tables with no large objects we
**	can just buffer them up and format them in any order the user
**	requests, but in tables with large objects are involved we can't
**	store a whole large object for re-ordering so we disallow this.
**
** System Status:
**	Copy is aborted.
**
** User Action:
**	Reorder columns according to the order they were created. Columns
**	may be omitted or repeated, but must be listed in ascending order.
*/

E_CO0045_BAD_NULL_BLOB:SS50000_MISC_ERRORS
"COPY: Error processing row %0c, column %1c:
Read an unexpected value where a Large Object NULL indicator was expected.
NULL indicator should have been 0 or 1."
/*
** Explanation:
**	Expected to read a Large Object NULL indicator in the COPY file but
**	read some other value
**
** System Status:
**	Copy is aborted.
**
** User Action:
**	Fix the NULL indicator for this Large Object in the COPY file.
**	It will probably be difficult to identify which byte is the culprit.
**	The byte after the '0' segment length indicator should be 0 if the
**	value is NOT NULL and 1 if it IS NULL.
*/

E_CO0046_NOZERO_NULL_BLOB:SS50000_MISC_ERRORS
"COPY: Error processing row %0c, column %1c:
Read an unexpected value where a Large Object NULL indicator was expected.
NULL indicator should have been 0 since a Large Object value was read."
/*
** Explanation:
**	Expected to read a NOT NULL Large Object NULL indicator in the COPY
**	file but read some other value
**
** System Status:
**	Copy is aborted.
**
** User Action:
**	Fix the NULL indicator for this Large Object in the COPY file.
**	It will probably be difficult to identify which byte is the culprit.
**	The byte after the '0' segment length indicator should be 0 if since
**	a Large Object value was read.
*/

E_CO0047_FILE_READ_MSG:SS50000_MISC_ERRORS
"COPY: Error reading from Copy File while processing row %0c, col %1c.
%2c"
/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_CO0048_ALLDUPS_OR_DISKFULL:SS50000_MISC_ERRORS
"COPY: Copy terminated abnormally.  0 rows successfully copied because
either all rows were duplicates or there was a disk full problem."
/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_CO0049_PROG_WRITE_MSG_ERR:SS50000_MISC_ERRORS
"COPY: Error writing row to Ingres Application while processing row %0c, col %1c.
%2c"
/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_CO004A_PROG_READ_MSG_ERR:SS50000_MISC_ERRORS
"COPY: Error reading from Ingres Application while processing row %0c, col %1c.
%2c"
/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_CO004B_CSV_NONWHITE:SS22000_DATA_EXCEPTION
"COPY: Row %0d: Unexpected character '%1c' seen while looking for delimiter after
reading CSV-format quoted string, column '%2c'."
/*
** Explanation:
**	A double-quoted CSV-format value was seen in the input file.
**	The delimiter (comma or newline) is expected to be immediately
**	after the double-quoted string, or separated from it by spaces
**	or tabs only;  but some other non-blank, non-delimiter was seen.
**
** System Status:
**	The copy statement is terminated.
**
** User Action:
**	Look for an un-doubled quote in the input string that might be
**	terminating the CSV value too soon.
*/


/*
** Slow messages for insertion into E_CO0041_FILE_WRITE_MSG_ERR,
** E_CO0049_PROG_WRITE_MSG_ERR
*/
S_CO0051_NO_ZERO_IND:SS50000_MISC_ERRORS
"Failed to write 0 length indicator."
S_CO0052_NO_NULL_IND:SS50000_MISC_ERRORS
"Failed to write null indicator."
S_CO0053_CANT_BE_NULL:SS50000_MISC_ERRORS
"Received a NULL value in a non-NULLable Large Object."
S_CO0054_NO_NOT_NULL:SS50000_MISC_ERRORS
"Failed to write NOT NULL indicator."
S_CO0055_PARTIAL_NON_BLOB:SS50000_MISC_ERRORS
"Failed to write partial non-Large Object column."
S_CO0056_NO_NON_BLOB_COL:SS50000_MISC_ERRORS
"Failed to write non-Large Object column."
S_CO0057_SEG_TOO_LONG:SS50000_MISC_ERRORS
"Segment too long, must be less than 32767."
S_CO0058_NO_SEG_LEN:SS50000_MISC_ERRORS
"Failed to write segment length."
S_CO0059_NO_SEG:SS50000_MISC_ERRORS
"Failed to write segment."
S_CO005A_NO_NO_MORE_DATA:SS50000_MISC_ERRORS
"Failed to write 'no more data' specifier."
S_CO0062_WRT_NULL_VAL:SS50000_MISC_ERRORS
"Failed to write NULL value"
S_CO0063_NO_WITH_NULL:SS50000_MISC_ERRORS
"NULL value encountered, you must specify the WITH NULL clause for this column"

/* Slow messages for insertion into E_CO0042_BLOB_READ_ERR */
S_CO005B_FIRST_SEG_LEN:SS50000_MISC_ERRORS
"length for first segment."
S_CO005C_FIRST_NULL_IND:SS50000_MISC_ERRORS
"NULL indicator after first segment."
S_CO005D_REG_COL_VAL:SS50000_MISC_ERRORS
"value for this column."
S_CO005E_NULL_IND:SS50000_MISC_ERRORS
"NULL indicator."

/* Slow messages for insertion into E_CO0043_BLOB_SEG_READ_ERR */
S_CO0060_SEG_VAL:SS50000_MISC_ERRORS
"value for segment "
S_CO0061_SEG_LEN:SS50000_MISC_ERRORS
"length for segment "

/*
** Slow messages for insertion into E_CO0047_FILE_READ_MSG,
** E_CO004A_PROG_READ_MSG_ERR
*/
S_CO005F_FMT_SEG_LEN:SS50000_MISC_ERRORS
"Error reading segment length of blob value."
S_CO0064_SEC_SEG_LEN:SS50000_MISC_ERRORS
"Error reading second segment length of nullable blob value."
S_CO0065_EXPECT_NMD:SS50000_MISC_ERRORS
"Expected 'no more data' segment length after null blob value."
S_CO0066_SEEK_VAL:SS50000_MISC_ERRORS
"Error SIfseek()'ing after attempted read of null blob value."
S_CO0067_SEEK_LEN:SS50000_MISC_ERRORS
"Error SIfseek()'ing after attempted read of null blob segment length."
S_CO0068_FST_SEGMENT:SS50000_MISC_ERRORS
"Error reading first segment of nullable blob value."
S_CO0069_DELIM:SS50000_MISC_ERRORS
"Error reading delimiter after blob value."
S_CO006A_BAD_DELIM:SS50000_MISC_ERRORS
"Delimiter after blob value does not match formatted delimiter."

NO_SQLSTATE_MAPPING
