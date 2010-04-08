/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: SCSTOK.H - This file describes the data structures and defines
**		    for the sequencer's tokenizer.
**
** Description:
**
**
** History:
**    31-july-1996 (reijo01)
**          Created for the sequencer's tokenizer.
**    21-jul-1997 (reijo01)
**        Add support for odbGetObjsVers() C-API calls.
*/

# define  SCS_MAX_TOKEN_LENGTH 32      /* Max token size */


/*
** Parser token types and ID's 
*/

/* Token types */
# define  PUNCTUATION		1
# define  SYMBOL		2
# define  NAME			3
# define  KEYWORD		4
# define  QUOTED_STRING		5
# define  NUMERIC_LITERAL	6

/* Token ID's */
# define  NOT_A_KEYWORD		0x0000

/* Keywords which identify statements */
# define  COMMIT_STATEMENT	0x0001
# define  COPY_STATEMENT	0x0002
# define  CREATE_STATEMENT	0x0003
# define  DEFINE_STATEMENT	0x0004
# define  DELETE_STATEMENT	0x0005
# define  DESCRIBE_STATEMENT	0x0006
# define  DIRECT_STATEMENT	0x0007
# define  DROP_STATEMENT	0x0008
# define  EXECUTE_STATEMENT	0x0009
# define  GRANT_STATEMENT	0x000a
# define  INSERT_STATEMENT	0x000b
# define  OPEN_STATEMENT	0x000c
# define  PREPARE_STATEMENT	0x000d
# define  REGISTER_STATEMENT	0x000e
# define  REMOVE_STATEMENT	0x000f
# define  REVOKE_STATEMENT	0x0010
# define  ROLLBACK_STATEMENT	0x0011
# define  SAVEPOINT_STATEMENT	0x0012
# define  SELECT_STATEMENT	0x0013
# define  SELECT_FUNCTION	0x0014
# define  SET_STATEMENT		0x0015
# define  UPDATE_STATEMENT	0x0016

/* Datatypes */
# define  CHAR_TYPE		0x0018
# define  DATE_TYPE		0x0019
# define  DECIMAL_TYPE		0x001a
# define  FLOAT_TYPE		0x001b
# define  INTEGER_TYPE		0x001c
# define  REAL_TYPE		0x001d
# define  SMALLINT_TYPE		0x001e
# define  VARCHAR_TYPE		0x001f

/* Other keywords */
# define  ALL			0x0020
# define  AS			0x0021
# define  ASC			0x0022
# define  AUTOCOMMIT		0x0023
# define  AVG			0x0024
# define  BY			0x0025
# define  BYREF			0x0026
# define  CHECK			0x0027
# define  COUNT			0x0028
# define  CURRENT		0x0029
# define  CURRENT_DATE		0x002a
# define  CURRENT_TIMESTAMP	0x002b
# define  CURSOR		0x002c
# define  DBA			0x002d
# define  DEFAULT		0x002e
# define  DSC			0x002f
# define  DISTINCT		0x0030
# define  FOR			0x0031
# define  FOREIGN		0x0032
# define  FROM			0x0033
# define  GROUP			0x0034
# define  HAVING		0x0035
# define  IMMEDIATE		0x0036
# define  IMPORT		0x0037
# define  INDEX			0x0038
# define  INTO			0x0039
# define  IS			0x003a
# define  JOINOP		0x003b
# define  JOURNALING		0x003c
# define  LOCKMODE		0x003d
# define  MAX			0x003e
# define  MIN			0x003f
# define  NOJOURNALING		0x0040
# define  NOPRINTQRY		0x0041
# define  NOQEP			0x0042
# define  NOT			0x0043
# define  NOTRACE		0x0044
# define  NULL_SYMBOL		0x0045
# define  OFF			0x0046
# define  ON			0x0047
# define  OPTION		0x0048
# define  ORDER			0x0049
# define  KW_PRIMARY		0x004a
# define  PRINTQRY		0x004b
# define  PRIVILEGES		0x004c
# define  PROCEDURE		0x004d
# define  QEP			0x004e
# define  READ_ONLY		0x004f
# define  RESULT_STRUCTURE	0x0050
# define  SCHEMA		0x0051
# define  SUM			0x0052
# define  TABLE			0x0053
# define  TO			0x0054
# define  TRACE			0x0055
# define  UNION			0x0056
# define  UNIQUE		0x0057
# define  USER			0x0058
# define  USING			0x0059
# define  VALUES		0x005a
# define  VIEW			0x005b
# define  WHERE			0x005c
# define  WITH			0x005d
# define  WORK			0x005e

/* Punctuation */
# define  DOUBLE_QUOTE		0x0060
# define  DOLLAR_SIGN		0x0061
# define  SINGLE_QUOTE		0x0062
# define  L_PAREN		0x0063
# define  R_PAREN		0x0064
# define  ASTERISK		0x0065
# define  PLUS_SIGN		0x0066
# define  COMMA			0x0067
# define  MINUS_SIGN		0x0068
# define  PERIOD		0x0069
# define  EQUAL_SIGN		0x006a
# define  QUESTION_MARK		0x006b
# define  TILDE			0x006c
# define  BEGIN_COMMENT		0x006d
# define  END_COMMENT		0x006e

/* Composite identifiers */
# define  CREATE_INDEX		0x0080
# define  CREATE_SCHEMA		0x0081
# define  CREATE_TABLE		0x0082
# define  CREATE_VIEW		0x0083
# define  DROP_INDEX		0x0084
# define  DROP_TABLE		0x0085
# define  DROP_VIEW		0x0086
# define  EXECUTE_PROCEDURE	0x0087
# define  REGISTER_PROCEDURE	0x0088
# define  REMOVE_PROCEDURE	0x0089

# define  DOLLAR_VAR		0x0090
# define  TILDE_Q		0x0091
# define  TILDE_V		0x0092

/* C-API identifiers */
# define  ODBADDMMDATA_STATEMENT 0x0093
# define  ODBGETMMDATA_STATEMENT 0x0094
# define  ODBGETVALUES_STATEMENT 0x0095
# define  ODBREPMMDATA_STATEMENT 0x0096
# define  ODBSETVALUES_STATEMENT 0x0097
# define  ODBGETOBJSVERS_STATEMENT 0x0098
