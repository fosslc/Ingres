/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    oserr.h -	OSL Parser Error Number Definitions File.
**
** Description:
**	Contains the definitions for the OSL parser error numbers.
**
** History:
**	Revision 6.0  87/06  wong
**	Re-worked for 6.0.
**	10-feb-89 (marian)
**		Added OSXACTWRN.
**	18-aug-92 (davel)
**		Added OSUNTERMDELIM (for unterminated delim ID).
*/

#define	    OSNODB	E_OS0000 /* Fatal:  No database name on command line */
#define	    OSNOINP	E_OS0001 /* Fatal:  Can't open input file (filename) */
#define	    OSNOOUT	E_OS0002 /* Fatal:  Can't open output file (filename)*/
#define	    OSNOFRMNAME	E_OS0003 /* Fatal:  No form name given for frame */
#define	    OSELIST	E_OS0004 /* Warning: Can't open list file (filename) */
#define	    OSERRSUM	E_OS0005 /* Summary of undefined variables referenced
						(number of variables) */

/* Fatal Bug-checks */
#define	    OSBUG	E_OS0010 /* An internal bug exists (name of routine) */
#define	    OSNOMEM	E_OS0011 /* Ran out of memory (name of routine) */
#define	    OSNULLCONST	E_OS0012 /* Null value passed (type expected) */
#define	    OSNULLVAL	E_OS0013 /* Null value passed as operand
						(type of operator) */
#define	    OSBADEXPR	E_OS0014 /* Badly formed expression */

/* Form Declaration Errors */
#define	    OSNOFORM	E_OS0020 /* Fatal:  No form found (form name) */
#define	    OSE2FLD	E_OS0021 /* Fatal:  Two fields with same name
					(field, form) */
#define	    OSE2TFLD	E_OS0022 /* Fatal:  Table field name conflicts with
					some other field (table field, form) */
#define	    OSE2COL	E_OS0023 /* Fatal:  Two columns have the same name
					(column, table field) */
#define	    OSEHIDDEN	E_OS0024 /* A hidden field has the same name
						as a field (field, form) */
#define	    OSEHIDCOL	E_OS0025 /* A hidden col. has the same name as a column
					(column, table field) */
#define	    OSHIDTYPE	E_OS0026 /* Bad type for a hidden field (field name) */
#define	    OSHIDSIZE	E_OS0027 /* Bad size for a hidden field (field name) */
#define	    OSECONSTANT	E_OS0028 /* A constant has the same name
						as a field (field, form) */
/* Lexical Errors */
#define	    OSUNXEOF	E_OS0030 /* Fatal:  Unexpected EOF on input */
#define	    OSCOMUNTERM	E_OS0031 /* Unterminated comment */
#define	    OSUNTERMSTR	E_OS0032 /* Unterminated string constant */
#define	    OSUNTERMDELIM E_OS003A_UntermDelimID /* Unterminated delim ID */
#define	    OSLNGSTR	E_OS0033 /* String too long (string) */

#define	    OSBADERR	E_OS0034 /* Bad number in ERROR (number) */
#define	    OSBADLINE	E_OS0035 /* Bad number in LINE (number) */
#define	    OSBADNUM	E_OS0036 /* Bad number (number) */

/* SQL Lexical Errors */
#define	    OSDBLQUOTE	E_OS0037 /* Warning:  No double-quotes for SQL */
#define	    OSLNGSQUOTE	E_OS0038 /* Long single quote constant (string) */
#define	    OSSNGLQUOTE	E_OS0039 /* Single-quote in OSL/QUEL */
#define	    OSLNGHEX	E_OS0040 /* Hexadecimal string too long */
#define	    OSBADHEX	E_OS0041 /* Bad character in hexadecimal string */
#define	    OSBADHEXSTR	E_OS0042 /* Badly specified hexadecimal string */

/* Syntax Errors */
#define	    OSSYNTAX	E_OS0100 /* A syntax error (line, symbol) */
#define	    OSESYNTAX	E_OS0101 /* A syntax error w/ expected tokens */
#define	    OSENDIF	E_OS0102 /* A syntax error w/ ENDIF; possible ELSE IF */
#define	    OSEHEADER	E_OS0103 /* Op header spec messed up */
#define	    OSEEMPTY	E_OS0104 /* No operations in file */
#define	    OSESEMIC	E_OS0105 /* Missing ; */
#define	    OSEINVOP	E_OS0106 /* Invalid context for operation */
#define	    OSEERROP	E_OS0107 /* Invalid context for operation in query loop */
#define	    OSEMISBDY	E_OS0108 /* Missing body for query loop */

/* Type or Definition Errors */
#define	    OSASNTYPES	E_OS0110 /* Type clash in assignment stmt */
#define	    OSNULLCHK	E_OS0111 /* Warning:  Assignment from nullable to non */
#define	    OSTYPECHK	E_OS0112 /* Type checking */
#define	    OSUMINUS	E_OS0113 /* Improper type with unary minus */
#define	    OSLOGTYPCHK	E_OS0114 /* Non-boolean operand for logical
						operator (name of op) */
#define	    OSRHSUNDEF	E_OS0115 /* Undefined variable (variable name) */
#define	    OSNOTBLFLD	E_OS0116 /* No such table field in form
						(table name, form name) */
#define	    OSNOCOL	E_OS0117 /* Not a column (form, table, column) */
#define	    OSVARUNDEF	E_OS0118 /* Undefined variable on lhs of
						assignment stmt (var name) */
#define	    OSVARPSTR	E_OS0119 /* Non-string in prompted */
#define	    OSASNOVAR	E_OS011A /* Unassignable object on lhs of assignment
					statement (name) */
#define     OSBADREF	E_OS011B /* Warning:  Superfluous ':' in lhs
					(6.0 only) */
#define	    OSFUNCOPS	E_OS011C /* Illegal function operands (function name) */
#define	    OSFNARG	E_OS011E /* Illegal function operand */

#define	    OSNOTSTRX	E_OS0120 /* Number where string variable or
					expression required */
#define	    OSNOTINT	E_OS0121 /* String or float where int required */
#define	    OSNOTREF	E_OS0122 /* String or Int required */
#define	    OSNOTSTR	E_OS0123 /* String variable expected (context) */

#define	    OSNORETVAL	E_OS012A /* Frame/Procedure with type "none"
						used in an expr (name) */
#define	    OSEXPRETVAL	E_OS012B /* Return value ignored for frame/procedure
						(name) */
#define	    OSNULLPROC	E_OS012C /* Return value ignored for function (name) */
#define	    OSNORETYPE	E_OS012D /* No return value expected for frame/proc */
#define	    OSBADRET	E_OS012E /* Type clash for return value */
#define	    OSRETTYPE	E_OS012F /* Warning: Ret. value expected for frm/proc */

#define	    OSNULLEXPR	E_OS0130 /* Illegal null-valued expression */
#define	    OSNULLCMP	E_OS0131 /* Illegal null comparison */

/* Query Errors */
#define	    OSOLDQRY	E_OS0150 /* Warning:  Obsolete master-detail syntax */
#define	    OSBADQRYALL	E_OS0151 /* Query .all with no fields on form (name.) */
#define	    OSDYNQRY	E_OS0152 /* Attempt to specify retrieve statement
						dynamically */
#define	    OSQRYTFLD	E_OS0153 /* Table field appears in target list
						of retrieve into current form
						(table field) */
#define	    OSQRYTFALL	E_OS0154 /* Use of .all applying to table field
						in retrieve target list */
#define	    OSNOFLD	E_OS0155 /* Non-existent field referenced in
						target list of retrieve
						(field name, form name) */
#define	    OSNOTFCOL	E_OS0156 /* Non-existent table field column in
						target list of retrieve
						(column, table field, form) */
#define	    OSNOTCUR	E_OS0157 /* Form named is not current form (name) */
#define	    OSEXTRQRY	E_OS0158 /* Too many queries in parameter list */
#define	    OSNOQRYOBJ	E_OS0159 /* Non-existent query object */

/* Misc. Semantic Errors */
#define	    OSDUPLABEL	E_OS0170 /* Label name not unique (name, formname) */
#define	    OSNOLABEL	E_OS0171 /* No label of specified name (name) */
#define	    OSBADLABEL	E_OS0172 /* 'Endloop label' referring to inactive
						while loop (label name) */
#define	    OSINPROC	E_OS0173 /* Statement not allowed within OSL
						procedure (name of stmt) */
#define	    OSENDLOOP	E_OS0175 /* Endloop outside of query scope */
#define	    OSNESTUNLD	E_OS0176 /* Illegal nesting of unloadtables */
#define	    OSTABINIT	E_OS0177 /* Warning:  No hidden cols for table init.*/
#define	    OSILLCHAR	E_OS0178 /* Illegal character in string constant */
#define	    OSEPRED	E_OS0179 /* Unexpected predicate not in DML statement */
#define	    OSHIDPRED	E_OS017A /* Hidden field in predicate */
#define	    OSHIDCOLPRED  E_OS0229 /* Hidden column in predicate */
#define     OSBADOWNER  E_OS017B /* Bad table owner identifier */

/* QUEL Errors */
#define	    OSRNGFLD	E_OS0200 /* Range variable with same name as field.
						(range, table) */
#define     OSQUALID	E_OS0201 /* Non-attribute ID in qualification 
					predicate element */

/* SQL Errors */
#define	    OSSELLIST	E_OS0210 /* Select target element without name mapping */
#define	    OSINTO	E_OS0211 /* INTO not allowed in SELECTs */
#define	    OSINSALL	E_OS0212 /* INSERT: .all in VALUEs used without '*' */
#define	    OSINSSTAR	E_OS0213 /* INSERT: '*' used without .all in VALUEs */
#define	    OSINSLISTS	E_OS0214 /* INSERT: unbalanced column and value lists */
#define	    OSNODEFAULT	E_OS0215 /* Invalid WITH DEFAULT clause */

/* DDB Errors */
#define	    OSCRTLINK	E_OS0220 /* define/create [temp|perm] not
					   followed by link  (following id) */
#define	    OSDRPLINK	E_OS0221 /* A bad word (word) was found in
					   a destroy/drop link statement.  The
					   word (expected) was expected. */
#define	    OSADDNODE	E_OS0222 /* add node not supported */
#define	    OSREMNODE	E_OS0223 /* remove node not supported */

/* Conversion Warning Messages */
#define     OSXACTWRN	E_OS0224 /* warn about 5.0 sql transaction semantic
					changes for 6.0 */

#define     OSNULLDECL    E_OS0228	/* class declared NOT NULL */

#define	    OSOBJUNDEF	E_OS0250 /* Undefined variable (general) */
#define	    OSBADDOT	E_OS0251 /* Bad use of DOT operator. */
#define	    OSNOTARRAY	E_OS0252 /* Bad use of subscript. */
#define	    OSNOTMEM	E_OS0253 /* nonexistent class member. */
#define	    OSNOTAB	E_OS0254 /* nonexistent dbms table. */
#define	    OSBADHIDCOL	E_OS0255 /* hidden column declared as complex type */
#define	    OSCANTARRAY	E_OS0256 /* simple type declared as array */
#define     OSNOINDEX   E_OS0257 /* array access without index */
#define     OSNOTTFLD   E_OS0258 /* tablefield op attempted on array */
#define     OSRECURSIVE E_OS0259 /* recursively defined class. */
#define     OSNOROW 	E_OS0260 /* no rownumber given on array reference. */
#define     OSBADGLOB 	E_OS0261 /* global has non-existent datatype. */
#define     OSBADATTR 	E_OS0262 /* attribute has non-existent datatype. */
#define	    OSLDTBLIDX	E_OS0264 /* row number specified for LOADTABLE */
#define	    OSLDTBLEMP	E_OS0265 /* no column assignments for LOADTABLE */

#include	<eros.h>
