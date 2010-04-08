/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

/**
** Name:	iamtok.h - token interface definitions
**
** Description:
**	structure to hold parsed token from encoded string.  dat union holds
**	converted data based on type item.
*/
typedef struct
{
	i4 type;
	union
	{
		f8 f8val;
		i4 i4val;
		i2 i2val;
		char *str;
		DB_TEXT_STRING	*hexstr;
	} dat;
} TOKEN;

/*
** token types
*/
#define TOK_f8 1	/* f8 value in f8val */
#define TOK_i4 2	/* i4 value in i4val */
#define TOK_i2 3	/* i2 value in i2val */
#define TOK_str 4	/* string pointer in str */
#define TOK_obj 5	/* object index in i4val */
#define TOK_start 6	/* starting brace - no data */
#define TOK_end 7	/* ending brace - no data */
#define TOK_eos 8	/* end of string - no data */
#define TOK_id 9	/* id value in i4val */
#define TOK_char 10	/* illegal char, ascii code in i2val */
#define TOK_xs 11	/* hexadecimal string, length & string in xs */
#define TOK_vers 12	/* object manager version stamp, number in i4val */
