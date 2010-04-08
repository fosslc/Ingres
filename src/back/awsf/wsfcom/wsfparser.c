/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wsf.h>
#include <wsfparser.h>
#include <erwsf.h>

#include <asct.h>

/*
**
**  Name: wsfparser.c - Parser Engine
**
**  Description:
**		provide functions to parse an ascii text
**	
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      05-Nov-98 (fanra01)
**          Add braces to aid readability.
**      18-Jun-1999 (fanra01)
**          Modified trace messages to be more informative.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Name: ParseOpen() - Open the parser
**
** Description:
**	Initialize parser structures
**
** Inputs:
**	PPARSER_NODE	ParserTab
**	PPARSER_IN		in
**	PPARSER_OUT		out
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
ParseOpen (
	PPARSER_NODE ParserTab, 
	PPARSER_IN in, 
	PPARSER_OUT out) 
{
	GSTATUS err = GSTAT_OK;
	in->current_node = in->first_node;
	in->last_first = 0;
return(err);
}

/*
** Name: Parse() - Parse the text passes through the PARSER_IN structure
**
** Description:
**	
**
** Inputs:
**	PPARSER_NODE	ParserTab
**	PPARSER_IN		in
**	PPARSER_OUT		out
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      05-Nov-98 (fanra01)
**          Add braces to aid readability.
**      04-Mar-1999 (fanra01)
**          Add trace messages.
**      18-Jun-1999 (fanra01)
**          Modified trace messages to be more informative.
*/
GSTATUS 
Parse (
    PPARSER_NODE ParserTab,
    PPARSER_IN in,
    PPARSER_OUT out)
{
    GSTATUS         err = GSTAT_OK;
    PPARSER_NODE    node;
    u_i4           i, end;
    unsigned char   current;

    node = &ParserTab[in->current_node];

    asct_trace( ASCT_PARSER )( ASCT_TRACE_PARAMS,
        "Parser start position=%d buffer=%40s", in->current,
         &in->buffer[in->position] );
    while (!err && in->current < in->length)
    {
        current = in->buffer[in->current];
        if (node->Separator || !IsSeparator(current))
        {
            end = node->end;
            for (i = node->begin; !err && i <= end; i++)
            {
                current = in->buffer[in->current];
                if (ParserTab[i].Case == FALSE)
                {
                    current = ((current >= 'a' && current <= 'z') ?
                        current - ('a' - 'A') : current);
                }
                if (current >= ParserTab[i].min && current <= ParserTab[i].max)
                {
                    if (in->current_node != i)
                    {
                        in->current_node = i;
                        if (in->current_node == in->first_node)
                            in->last_first = in->current;

                        node = &ParserTab[i];
                        if (err == GSTAT_OK && node->execution)
                        {
                            asct_trace( ASCT_PARSER )( ASCT_TRACE_PARAMS,
                                "Parser found node position=%d buffer=%t",
                                in->current, (in->current - in->position),
                                &in->buffer[in->position] );
                            err = node->execution(in, out);
                        }
                    }
                    break;
                }
            }
            if (err == GSTAT_OK && i > end)
            {
                err = DDFStatusAlloc( E_WS0001_PARSER_UNKNOW_NODE);
            }
            if (err != GSTAT_OK)
            {
                asct_trace( ASCT_PARSER )( ASCT_TRACE_PARAMS,
                    "Parser error status=%08x position=%d buffer=%40s",
                    err->number, in->current, &in->buffer[in->position] );
            }
        }
        in->current++;
    }

    if (err == GSTAT_OK &&
        in->position == 0 &&
        in->last_first > in->position &&
        ParserTab[in->first_node].endExecution)
    {
        err = ParserTab[in->first_node].endExecution(in, out);
        asct_trace( ASCT_PARSER )( ASCT_TRACE_PARAMS,
            "Parser end status=%08x position=%d",
            (err ? err->number : 0),
            in->current );
    }
    return(err);
}

/*
** Name: ParseClose() - Close the parsing
**
** Description:
**	
**
** Inputs:
**	PPARSER_NODE	ParserTab
**	PPARSER_IN		in
**	PPARSER_OUT		out
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
ParseClose (
	PPARSER_NODE ParserTab, 
	PPARSER_IN in, 
	PPARSER_OUT out) 
{
	GSTATUS err = GSTAT_OK;
	if (ParserTab[in->current_node].endExecution)
		err = ParserTab[in->current_node].endExecution(in, out);
	else
		err = DDFStatusAlloc( E_WS0002_PARSER_NOEXPECTED_END);

return(err);
}

/*
** Name: ParserPrint() - Print the table definition
**
** Description:
**		Debug function
**
** Inputs:
**	PPARSER_NODE	ParserTab
**	char*			file name to print the table
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
void ParserPrint(PPARSER_NODE tab, char *filename)
{
	FILE *fd;
	if ((fd = fopen(filename, "a")) != NULL)
	{
		u_i4 i;
		u_i4 length = sizeof(tab)/sizeof(PARSER_NODE);

		for (i = 0; i < length; i++)
			fprintf(fd, "%d %c %c %d %d %d %d\n", 
						i, 
						tab[i].min, 
						tab[i].max, 
						tab[i].begin, 
						tab[i].end, 
						tab[i].Case, 
						tab[i].Separator);
		fclose(fd);
	}
}
