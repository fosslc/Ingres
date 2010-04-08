# include <stdio.h>
/* Declare the SQLCA structure and the SQLDA typedef */
EXEC SQL INCLUDE SQLCA;
EXEC SQL INCLUDE SQLDA;

EXEC SQL DECLARE stmt STATEMENT;			/* Dynamic SQL 
statement */
EXEC SQL DECLARE csr CURSOR FOR stmt;		/* Cursor for dynamic SQL statement 
*/

/*
** Default number of result columns for a dynamic SELECT. If a SELECT
** statement returns more than DEF_ELEMS, then a new SQLDA will be 
allocated.
*/
# define	DEF_ELEMS	5

/* Size of a DATE string variable */
# define	DATE_SIZE	25

/* The SQL code for the NOT FOUND condition */
# define	SQL_NOTFOUND	100

/* Buffer lengths */
# define	DBNAME_MAX	50		/* Max database name */
# define	INPUT_SIZE	256		/* Max input line length */
# define	STMT_MAX	1000		/* Max SQL statement length */

/* Global SQL variables */

IISQLDA		*sqlda = (IISQLDA *)0;	/* Pointer to the 
SQL dynamic area */

/* Result storage buffer for dynamic SELECT statements 
*/
struct {
	int		res_length;	/* Size of mem_data */
	char		*res_data;	/* Pointer to allocated result buffer */
} res_buf = {0, NULL};

void	Run_Monitor();		/* Run SQL Monitor */
void	Init_Sqlda();		/* Initialize SQLDA */
int	Execute_Select();	/* Execute dynamic SELECT */
void	Print_Header();		/* Print SELECT column headers */
void	Print_Row();		/* Print SELECT row values */
void	Print_Error();		/* Print a user error */
char 	*Read_Stmt();		/* Read statement from terminal */
char 	*Alloc_Mem();		/* Allocate memory */
char 	*calloc();		/* C allocation routine */

/*
** Procedure:	main
** Purpose:	Main body of SQL Monitor application. Prompt for database
**		name and connect to the database. Run the monitor and
**		disconnect from the database. Before disconnecting roll 
**		back any pending updates.
** Parameters:
**			None
*/

main()
{
	EXEC SQL BEGIN DECLARE SECTION;
		char	dbname[DBNAME_MAX +1];		/* Database name */
	EXEC SQL END DECLARE SECTION;

	printf("\n\n%s\n", "****************************************************");
	printf("%s\n", "NOTE: To quit execution at any time, press CONTROL-C"); 
	printf("%s\n\n", "****************************************************");

	/* Prompt for database name - could be command line parameter */
	printf("Enter Database Name: (vnode::dbname) ");
	if (fgets(dbname, DBNAME_MAX, stdin) == NULL)
		exit(1);

	printf( "Enter SQL select statement and a semicolon\n" );

	/* Treat connection errors as fatal */
	EXEC SQL WHENEVER SQLERROR STOP;
	EXEC SQL CONNECT :dbname;

	Run_Monitor();
	

	EXEC SQL WHENEVER SQLERROR CONTINUE;

	printf("SQL: Exiting monitor program.\n");

	EXEC SQL ROLLBACK;
	EXEC SQL DISCONNECT;
} /* main */

/*
** Procedure:	Run_Monitor
** Purpose:	Run the SQL monitor. Initialize the first SQLDA with the
**		default size (DEF_ELEMS 'sqlvar' elements). Loop while
**		prompting the user for input, and processing the statement.
**		If it is not a SELECT statement then execute it, otherwise
**		open a cursor a process a dynamic SELECT statement.
** Parameters:
**		None
*/

void
Run_Monitor()
{
    	EXEC SQL BEGIN DECLARE SECTION;
	    char	stmt_buf[STMT_MAX +1];	/* SQL statement input buffer */
	EXEC SQL END DECLARE SECTION;

	int		stmt_num;		/* SQL statement number */
	int		rows;			/* Rows affected */

	/* Allocate a new SQLDA */
	Init_Sqlda(DEF_ELEMS);

	/* Now we are set for input */
	for (stmt_num = 1;; stmt_num++)
	{
	    /*
	    ** Prompt and read the next statement. If Read_Stmt
	    ** returns NULL then end-of-file was detected.
	    */
	    if (Read_Stmt(stmt_num, stmt_buf, STMT_MAX) == NULL)
			break;

	    /* Errors are non-fatal from here on out */
	    EXEC SQL WHENEVER SQLERROR GOTO Stmt_Err;
	    
	    if (strcmp(stmt_buf, "quit") == 0) 
		exit(1);

	    /*
	    ** Prepare and describe the statement. If we cannot fully describe
	    ** the statement (our SQLDA is too small) then allocate a new one
	    ** and redescribe the statement.
	    */
	    EXEC SQL PREPARE stmt FROM :stmt_buf;
	    EXEC SQL DESCRIBE stmt INTO :sqlda;
	    if (sqlda->sqld > sqlda->sqln)
	    {
		    Init_Sqlda(sqlda->sqld);
		    EXEC SQL DESCRIBE stmt INTO :sqlda;
	    }

	    /* If 'sqld' = 0 then this is not a SELECT */
	    if (sqlda->sqld == 0)
	    {
			EXEC SQL EXECUTE stmt;
			rows = sqlca.sqlerrd[2];
	    }
	    else	/* SELECT */
	    {
			rows = Execute_Select();
	    }
	    printf("[%d row(s)]\n", rows);
	    continue;				/* Skip error handler */

    Stmt_Err:
	    EXEC SQL WHENEVER SQLERROR CONTINUE;
	    /* Print error messages here and continue */
	    Print_Error();
	} /* for each statement */
} /* Run_Monitor */

/*
** Procedure:	Init_Sqlda
** Purpose:	Initialize SQLDA. Free any old SQLDA's and allocate a new
**		one. Set the number of 'sqlvar' elements.
** Parameters:
**		num_elems    - Number of elements.
*/

void
Init_Sqlda(num_elems)
int	num_elems;
{
	/* Free the old SQLDA */
	if (sqlda)
	    free((char *)sqlda);

	/* Allocate a new SQLDA */
	sqlda = (IISQLDA *)
		Alloc_Mem(IISQDA_HEAD_SIZE + (num_elems * IISQDA_VAR_SIZE),
			  "new SQLDA");
	sqlda->sqln = num_elems;		/* Set the size */
} /* Init_Sqlda */

/*
** Procedure:	Execute_Select
** Purpose:	Run a dynamic SELECT statement. The SQLDA has already been
**		described, so print the column header (names), open a cursor,
**		and retrieve and print the results. Accumulate the number or
**		rows processed.
** Parameters:
**		None
** Returns:
**		Number of rows processed.
*/

int
Execute_Select()
{
	int	rows;			/* Counter for rows fetched */

	/*
	** Print the result column names, allocate the result variables,
	** and setup the types.
	*/
	Print_Header();
	EXEC SQL WHENEVER SQLERROR GOTO Close_Csr;

	/* Open the dynamic cursor */
	EXEC SQL OPEN csr;

	/* Fetch and print each row */
	rows = 0;
	while (sqlca.sqlcode == 0)
	{
	    EXEC SQL FETCH csr USING DESCRIPTOR :sqlda;
	    if (sqlca.sqlcode == 0)
	    {
		rows++;				/* Count the rows */
		Print_Row();
	    }
	} /* While there are more rows */

Close_Csr:
	/* If we got here because of an error then print the error message */
	if (sqlca.sqlcode < 0)
	    Print_Error();
	EXEC SQL WHENEVER SQLERROR CONTINUE;
	EXEC SQL CLOSE csr;

	return rows;
} /* Execute_Select */

/*
** Procedure:	Print_Header
** Purpose:	A statement has just been described so set up the SQLDA for
**		result processing. Print all the column names and allocate
**		a result buffer for retrieving data. The result buffer is
**		one buffer (whose size is determined by adding up the results
**		column sizes). The 'sqldata' and 'sqlind' fields are pointed
**		at offsets into this buffer.
** Parameters:
**			None
*/

void
Print_Header()
{
	int		i;			/* Index into 'sqlvar' */
	IISQLVAR	*sqv;			/* Pointer to 'sqlvar */
	int		base_type;		/* Base type w/o nullability */
	int		res_cur_size;		/* Result size required */
	int		round;			/* Alignment */

	/*
	** For each column print its title (and number), and accumulate
	** the size of the result data area.
	*/
	for (res_cur_size = 0, i = 0; i < sqlda->sqld; i++)
	{
	    /* Print each column name and its number */
	    sqv = &sqlda->sqlvar[i];
	    printf("[%d] %.*s ",
		   i+1, sqv->sqlname.sqlnamel, sqv->sqlname.sqlnamec);

	    /* Find the base-type of the result (non-nullable) */
	    if ((base_type = sqv->sqltype) < 0)
			base_type = -base_type;

	    /* Collapse different types into INT, FLOAT or CHAR */
	    switch (base_type)
	    {
	      case IISQ_INT_TYPE:
			/* Always retrieve into a long integer */
			res_cur_size += sizeof(long);
			sqv->sqllen = sizeof(long);
			break;

	      case IISQ_MNY_TYPE:
			/* Always retrieve into a double floating point */
			if (sqv->sqltype < 0)
			    sqv->sqltype = -IISQ_FLT_TYPE;
			else
			    sqv->sqltype = IISQ_FLT_TYPE;
			res_cur_size += sizeof(double);
			sqv->sqllen = sizeof(double);
			break;

	      case IISQ_FLT_TYPE:
			/* Always retrieve into a double floating point */
			res_cur_size += sizeof(double);
			sqv->sqllen = sizeof(double);
			break;

	      case IISQ_DTE_TYPE:
			sqv->sqllen = DATE_SIZE;
			/* Fall through to handle like CHAR */

	      case IISQ_CHA_TYPE:
	      case IISQ_VCH_TYPE:
			/*
			** Assume no binary data is returned from the CHAR type.
			** Also allocate one extra byte for the null terminator.
			*/
			res_cur_size += sqv->sqllen + 1;
			/* Always round off to aligned data boundary */
			round = res_cur_size % 4;
			if (round)
			    res_cur_size += 4 - round;
			if (sqv->sqltype < 0)
			    sqv->sqltype = -IISQ_CHA_TYPE;
			else
			    sqv->sqltype = IISQ_CHA_TYPE;
			break;
	    } /* switch on base type */

	    /* Save away space for the null indicator */
	    if (sqv->sqltype < 0)
			res_cur_size += sizeof(int);
	} /* for each column */

	printf("\n\n");

	/* 
	** At this point we've printed all column headers and converted
	** all types to one of INT, CHAR or FLOAT. Now we allocate a
	** single result buffer, and point all the result column data
	** areas into it.
	**
	** If we have an old result data area that is not large enough then
	** free it and allocate a new one. Otherwise we can reuse the last
	** one.
	*/

	if (res_buf.res_length > 0 && res_buf.res_length < res_cur_size)
	{
	    free(res_buf.res_data);
	    res_buf.res_length = 0;
	}
	if (res_buf.res_length == 0)
	{
	    res_buf.res_data = Alloc_Mem(res_cur_size,
					 "result data storage area");
	    res_buf.res_length = res_cur_size;
	}

	/*
	** Now for each column now assign the result address (sqldata) and
	** indicator address (sqlind) from the result data area.
	*/
	for (res_cur_size = 0, i = 0; i < sqlda->sqld; i++)
	{
	    sqv = &sqlda->sqlvar[i];

	    /* Find the base-type of the result (non-nullable) */
	    if ((base_type = sqv->sqltype) < 0)
			base_type = -base_type;

	    /* Current data points at current offset */
	    sqv->sqldata = (char *)&res_buf.res_data[res_cur_size];
	    res_cur_size += sqv->sqllen;

	    if (base_type == IISQ_CHA_TYPE)
	    {
	    	res_cur_size++;			/* Add one for null */
		round = res_cur_size % 4;	/* Round to aligned boundary */
		if (round) 
		res_cur_size += 4 - round;
	    }

	    /* Point at result indicator variable */
	    if (sqv->sqltype < 0)
	    {
		sqv->sqlind = (short *)&res_buf.res_data[res_cur_size];
		res_cur_size += sizeof(int);
	    }
	    else
	    {
		sqv->sqlind = (short *)0;
	    } /* if type is nullable */
	} /* for each column */
} /* Print_Header */

/*
** Procedure:	Print_Row
** Purpose:	For each element inside the SQLDA, print the value. Print
**		its column number too in order to identify it with a column
**		name printed earlier. If the value is NULL print "N/A".
** Parameters:
**		None
*/

void
Print_Row()
{
	int		i;			/* Index into 'sqlvar' */
	IISQLVAR	*sqv;			/* Pointer to 'sqlvar */
	int		base_type;		/* Base type w/o nullability */

	/*
	** For each column, print the column number and the data.
	** NULL columns print as "N/A".
	*/
	for (i = 0; i < sqlda->sqld; i++)
	{
	    /* Print each column value with its number */
	    sqv = &sqlda->sqlvar[i];
	    printf("[%d] ", i+1);

	    if (sqv->sqlind && *sqv->sqlind < 0)
	    {
		printf("N/A ");
	    }
	    else /* Either not nullable, or nullable but not null */
	    {
		/* Find the base-type of the result (non-nullable) */
		if ((base_type = sqv->sqltype) < 0)
		    base_type = -base_type;

		switch (base_type)
		{
		  case IISQ_INT_TYPE:
		    /* All integers were retrieved into long integers */
		    printf("%d ", *(long *)sqv->sqldata);
		    break;

		  case IISQ_FLT_TYPE:
		    /* All floats were retrieved into doubles */
		    printf("%g ", *(double *)sqv->sqldata);
		    break;

		  case IISQ_CHA_TYPE:
		    /* All characters were null terminated */
		    printf("%s ", (char *)sqv->sqldata );
		    break;
		} /* switch on base type */
	    } /*if not null */
	} /* foreach column */
	printf("\n");
} /* Print_Row */

/*
** Procedure:	Print_Error
** Purpose:	SQLCA error detected. Retrieve the error message and print 
it.
** Parameters:
**			None
*/

void
Print_Error()
{
	EXEC SQL BEGIN DECLARE SECTION;
		char	error_buf[150];		/* For error text retrieval */
	EXEC SQL END DECLARE SECTION;

	EXEC SQL INQUIRE_INGRES (:error_buf = ERRORTEXT);
	printf("\nSQL Error:\n    %s\n", error_buf );
} /* Print_Error */

/*
** Procedure:	Read_Stmt
** Purpose:	Reads a statement from standard input. This routine prompts
**			the user for input (using a statement number) and scans input
**			tokens for the statement delimiter (semicolon).
**			- Continues over new-lines.
**			- Uses SQL string literal rules.
** Parameters:
**			stmt_num - Statement number for prompt.
**			stmt_buf - Buffer to fill for input.
**			stmt_max - Max size of statement.
** Returns:	
**			A pointer to the input buffer. If NULL then end-of-file was
**			typed in.
*/

char *
Read_Stmt(stmt_num, stmt_buf, stmt_max)
int		stmt_num;
char	stmt_buf[];
int		stmt_max;
{
	char	input_buf[INPUT_SIZE +1];	/* Terminal input buffer */
	char	*icp;	 /* Scans input buffer */
	char	*ocp;  	/* To output (stmt_buf) */
	int	in_string;				/* For string handling */

	ocp = stmt_buf;
	in_string = 0;
	while (fgets(input_buf, INPUT_SIZE, stdin) != NULL) 
			{
	    for (icp = input_buf; *icp && (ocp - stmt_buf < stmt_max); 
icp++)
	    {
		/* Not in string - check for delimiters and new lines */
		if (!in_string)
		{
		    if (*icp == ';')		/* We're done */
		    {
				*ocp = '\0';
				return stmt_buf;
		    }
		    else if (*icp == '\n')
		    {
				/* New line outside of string is replaced with blank */
				*ocp++ = ' ';
				break;			/* Read next line */
		    }
		    else if (*icp == '\'')	/* Entering string */
		    {
				in_string++;
		    }
		    *ocp++ = *icp;
		}
		else 					/* Inside a string */
		{
		    if (*icp == '\n')
		    {
				break;			/* New-line in string is ignored */
		    }
		    else if (*icp == '\'')
		    {
			if (*(icp+1) == '\'')		/* Escaped quote ? */

			    *ocp++ = *icp++;
			else
			    in_string--;
		    }
		    *ocp++ = *icp;
		} /* if in string */
	    } /* for all characters in buffer */

	    if (ocp - stmt_buf >= stmt_max)
	    {
		/* Statement is too large; ignore it and try again */
		printf("SQL Error: Maximum statement length (%d) exceeded.\n",
		       stmt_max);
		printf("%3d> ", stmt_num);			/* Re-prompt user */
		ocp = stmt_buf;
		in_string = 0;
	    }
	    else /* Break on new line - print continue sign */
	    {
		printf("---> Enter a semicolon and press Enter");
	    }
	} /* while reading from standard input */
	return NULL;
} /* Read_Stmt */

/*
** Procedure:	Alloc_Mem
** Purpose:	General purpose memory allocator. If it cannot allocate
**			enough space, it prints a fatal error and aborts any
**			pending updates.
** Parameters:
**			mem_size     - Size of space requested.
**			error_string - Error message to print if failure.
** Returns:	
**			Pointer to newly allocated space.
*/

char *
Alloc_Mem(mem_size, error_string)
int	mem_size;
char	*error_string;
{
	char	*mem;

	mem = calloc(1, mem_size);
	if (mem)
	    return mem;

	/* Print an error and roll back any updates */
	printf("SQL Fatal Error: Cannot allocate %s (%d bytes).\n",
	       error_string, mem_size);
	printf("Any pending updates are being rolled back.\n");
	EXEC SQL WHENEVER SQLERROR CONTINUE;
	EXEC SQL ROLLBACK;
	EXEC SQL DISCONNECT;
	exit(-1);
} /* Alloc_Mem */
