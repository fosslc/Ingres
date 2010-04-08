/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      07-Sep-1998 (fanra01)
**          Corrected incomplete last line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef PARSER_INCLUDED
#define PARSER_INCLUDED

#include <ddfcom.h>

typedef struct __PARSER_IN 
{
	u_i4 first_node;
	u_i4 current_node;
	u_i4 last_first;
	char *buffer;	/* analysed buffer */
	u_i4 position;	/* position of the cursor on the buffer string */
	u_i4 current;	/* current character read */
	u_i4 length;		/* length of the string that should be parsed */	
	u_i4 size;		/* size of the buffer */
} PARSER_IN, *PPARSER_IN;

typedef PTR		PPARSER_OUT;

typedef struct __PARSER_NODE 
{
	unsigned char		min; 
	unsigned char		max;
	u_i4				begin;
	u_i4				end;
	bool				Case;
	bool				Separator;
	GSTATUS ( *execution ) ( PPARSER_IN, PPARSER_OUT);
	GSTATUS ( *endExecution) ( PPARSER_IN, PPARSER_OUT);
} PARSER_NODE, *PPARSER_NODE;

GSTATUS 
ParseOpen (
	PPARSER_NODE	ParserTab, 
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS 
Parse (
	PPARSER_NODE	ParserTab, 
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS 
ParseClose (
	PPARSER_NODE	ParserTab, 
	PPARSER_IN		in, 
	PPARSER_OUT		out);

#endif
