/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : parse.h : header file
**    Project  : Data Loader 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parse the command lines & Control File
**
** History:
**
** 02-Jan-2004 (uk$so01)
**    Created.
*/

#if !defined (PARSE_CMDxINPUT_HEADER)
#define PARSE_CMDxINPUT_HEADER


/*
** Reurn
**     0: failure
**     1: OK
** strCommandLine: the input command line string
** pDataStructure: some members of this structure that will be updated
*/
int ParseCommandLine(char* strCommandLine, COMMANDLINE* pDataStructure);

/*
** Reurn
**     0: failure
**     1: OK
** pCmd: the input command line structure (the member 'szControlFile' must be set)
** pDataStructure: some members of this structure that will be updated
*/
int ParseControlFile(DTSTRUCT* pDataStructure, COMMANDLINE* pCmd);
/*
** return an array of int:
** 0: LOAD DATA. 1: INFILE. 2: BADFILE. 3: DISCARDFILE. 4: DISCARDMAX. 5: INTO TABLE
** 6: APPEND. 7: FIELDS TERMINATED BY. 8: COLUMN DESCRIPTION
*/
int* GetControlFileMandatoryOptionSet();


#endif /* PARSE_CMDxINPUT_HEADER */

