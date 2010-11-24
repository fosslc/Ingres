/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<te.h>
# include	<cmcl.h>
# include	<st.h>
# include	<me.h>
# include	<ex.h>
# include	<si.h>
# include	<termios.h>

/*
**
**	SIhistgetrec
**
**	Similar to SIgetrec. Get next input record for terminal monitor. 
**	Simulates a line editor which maintains command history using arrow keys. 
**
**	Get the next record from stream, previously opened for reading.
**	SIgetrec reads n-1 characters, or up to a full record, whichever
**	comes first, into buf.  The last char is followed by a newline.
**	On unix, a record on a file is a string of bytes terminated by a newline.  
**	On systems with formatted files, a record may be defined differently.
**
**	This file maintains command history in a doubly linked list. 
**
**	This file defines following functions:
**
**	SIhistgetrec() similar to SIgetrec(), but has line editing capability
**		using arrow keys and also recollects history of lines typed.
**	SIgetprevhist() gets the previous line in the history.
**	SIgetnxthist() gets the next line in the history.
**	SIaddhistnode()	adds a line to the doubly linked list containing history.
**	SIclearhistory() clears the doubly-linked list memory.
**	SIeraseline() erases the current line being edited.
**	SIeraselineend() erases the current line from cursor till end.
**
**	History
**	31-mar-2004 (kodse01)
**		SIR 112068
**		Written.
**	29-apr-2004 (kodse01)
**		Fixed handling of special keys at buffer boundary condition.
**		Added support for control-u key to erase complete line.
**		Added support for control-k key to erase line from cursor
**		till end.
**	27-Jun-2007 (kiria01) b110040
**		Executing \i caused the SIclearhistory() but did not
**		clear the hist pointer thereby meaning that the next command
**		added to the list would get the old block reallocated via
**		the stale hist pointer. This resulted in the block pointing
**		to itself and a cpu loop waiting to happen.
**	20-Apr-2010 (hanje04)
**	    SIR 123622
**	    Add control-c (interupt) and contol-d (quit) support.
**	    Replace references to int with i4.
**	15-nov-2010 (stephenb)
**	    Include si/h and correctly define funcs for prototyping.
*/

# define	MAX_BUF_LENGTH	512 /* maximum buffer length */

# ifndef	EBCDIC
# define	ESC		'\033'
# define	BS		'\010'
# define	DEL		'\177'
# define	NAK		'\025'
# define	VT		'\013'
# define        ETX             '\003'
# define        EOT             '\004'
# else
# define	ESC		'\047'
# define	BS		'\026'
# define	DEL		'\007'
# define	NAK		'\075'
# define	VT		'\013'
# define        ETX             '\003'
# define        EOT             '\004'
# endif

/* Structure defining the doubly linked list node to maintain history */
typedef struct historynode
{ 
	char his_str[MAX_BUF_LENGTH]; 
	struct historynode *fptr; 
	struct historynode *bptr;
} HISTORYNODE;

/* Doubly linked list master pointer */
HISTORYNODE *hist = NULL;  /* points to the current history line */

int curline = 1; /* 1 if in the current line, 0 if in history line */

/* gets the previous line in the history */
static void 
SIgetprevhist(buf)
char *buf;
{ 
	if (hist != NULL) 
	{ 
		if (curline) 
			curline = 0; 
		else if (hist->bptr != NULL) 
			hist = hist->bptr; 
		MEfill(MAX_BUF_LENGTH, 0, buf); 
		STcopy(hist->his_str, buf); 
	}
}

/* gets the next line in the history */
static void 
SIgetnxthist(buf)
char *buf;
{
	if (hist != NULL)
	{
		MEfill(MAX_BUF_LENGTH, 0, buf); 
		if (hist->fptr != NULL) 
		{ 
			hist = hist->fptr; 
			STcopy(hist->his_str, buf); 
		} 
		else 
			curline = 1; 
	}
}

/* adds a line to the doubly linked list containing history */
static i4 
SIaddhistnode(buf)
char *buf;
{ 
	HISTORYNODE *tnode; 
	if ((buf != NULL) && (STlength(buf) != 0)) 
	{ 
		if ((tnode = (HISTORYNODE *) MEreqmem (0, sizeof(HISTORYNODE), 
			TRUE, NULL)) == NULL) 
			return -1; 
		tnode->bptr = tnode->fptr = NULL; 
		STcopy(buf, tnode->his_str); 
		if (hist != NULL) 
		{ 
			while (hist->fptr != NULL) 
				hist = hist->fptr; 
			hist->fptr = tnode; 
			tnode->bptr = hist; 
			hist = hist->fptr; 
		} 
		else 
			hist = tnode; 
	} 
	return 0;
}

/* clears the doubly-linked list memory */
void 
SIclearhistory(void)
{ 
	HISTORYNODE *temp; 
	if (hist != NULL) 
	{ 
		while (hist->fptr != NULL) 
			hist = hist->fptr; 
		while (hist->bptr != NULL) 
		{ 
			temp = hist; 
			hist = hist->bptr; 
			MEfree((PTR)temp); 
		} 
		MEfree((PTR)hist); 
		/* Having released last buffer - don't use it again */
		hist = NULL;
	}
}

/* erases the current line being edited */
static void 
SIeraseline(next, last)
i4 next;
i4 last;
{
	i4 i;
	for (i=0; i < last-next; i++)
		SIprintf(" ");
	for (i=0; i < last; i++)
		printf("\b \b");
}

/* erases the current line from cursor till end */
static void 
SIeraselineend(next, last)
i4 next;
i4 last;
{
	i4 i;
	for (i=0; i < last-next; i++)
		SIprintf(" ");
	for (i=0; i < last-next; i++)
		printf("\b");
}

/*{
**  NAME: SIHISTGETREC -- terminal monitors version of SIgetrec which contains 
**		command history feature.
**  Description:
**	This function sets the terminal settings to capture arrow keys. The 
**	up-arrow and down-arrow are used to navigate through the history. The 
**	left-arrow, right-arrow and backspace (^H) are used to edit the line.
**	
**	Known limitations:
**		Uses ANSI sequence of characters for arrow keys. Editing of line is
**	limited to a single line on the screen.
**	
**	History
**	31-mar-2004 (kodse01)
**		Written.
**	29-apr-2004 (kodse01)
**		Fixed handling of special keys at buffer boundary condition.
**		Added support for control-u key to erase complete line.
**		Added support for control-k key to erase line from cursor till end.
*/

STATUS
SIhistgetrec(buf, n, stream)
char	*buf;		/* read record into buf and null terminate */
i4		n;			/* (max number of characters - 1) to be read */
FILE	*stream;	/* get record from file stream */
{
	struct termios new_tty_termios, org_tty_termios;

	static char streambuf[MAX_BUF_LENGTH] = {0};
	char tempbuf1[MAX_BUF_LENGTH] = {0};
	char tempbuf2[MAX_BUF_LENGTH] = {0};
	char onechar[4] = {0};
	i4 next_ch = 0;	/* Next character from stream */
	i4 ch_indx = 0;	/* Zero-based index for last character in streambuf */
	i4 next_indx = 0;	/* Zero-based index for next character in streambuf */
						/* (middle of line) */
	i4 i = 0;

	if (TEsettmattr() < 0)
		return ENDFILE;

	MEfill(MAX_BUF_LENGTH, 0, streambuf);

	if (n > MAX_BUF_LENGTH)
	{
		TEresettmattr();
		return ENDFILE;
	}

	while ((next_ch != EOF) && (next_ch != '\n') && (next_ch != '\r'))
	{
		next_ch = SIgetc(stream);
		if (next_ch == EOF) break;

		if (next_ch == ESC)
		{
			next_ch = SIgetc(stream);
			if (next_ch == EOF) break;
			if (next_ch == '[')
			{
				next_ch = SIgetc(stream);
				if (next_ch == EOF) break;
				switch (next_ch)
				{
				case 'A': /* Up arrow pressed */
				/* erase the line and display previous line */
					SIeraseline(next_indx, ch_indx);
					SIgetprevhist(streambuf);
					ch_indx = next_indx = STlength(streambuf);
					SIprintf("%s", streambuf);
					break;
				case 'B': /* Down arrow pressed */
				/* if next line available erase the line and display next line*/
					SIeraseline(next_indx, ch_indx);
					SIgetnxthist(streambuf);
					ch_indx = next_indx = STlength(streambuf);
					SIprintf("%s", streambuf);
					break;
				case 'C': /* Right arrow pressed */
				/* if in the middle of line, go forward. else do nothing */
					if (next_indx < ch_indx)
					{
						SIprintf("%c", streambuf[next_indx]);
						next_indx++;
					}
					break;
				case 'D': /* Left arrow pressed */
				/* if start of line, do nothing, else go backward */
					if (next_indx > 0)
					{
						next_indx--;
						SIprintf("\b");
					}
					break;
				default:
				/* ignore other function keys */
					break;
				}
			/* ignore other function keys */
			}
		}
		else if (next_ch == BS || next_ch == DEL) /* Backspace, Delete */
		{
			/*Delete the previous char if available*/
			if (next_indx > 0)
			{
				SIprintf("\b \b");
				SIprintf("%s ", streambuf+next_indx);
				for (i=0; i <= ch_indx - next_indx; i++)
					SIprintf("\b");
				STlcopy(streambuf, tempbuf1, next_indx-1);
				STcopy(streambuf+next_indx, tempbuf2);
				MEfill(MAX_BUF_LENGTH, 0, streambuf);
				STpolycat(2, tempbuf1, tempbuf2, streambuf);
				ch_indx--; next_indx--;
			}
		}
		else if (next_ch == NAK) /* control-u */
		{
			SIeraseline(next_indx, ch_indx);	
			MEfill(MAX_BUF_LENGTH, 0, streambuf);	
			ch_indx = next_indx = 0;
		}
		else if (next_ch == VT) /* control-k */
		{
			SIeraselineend(next_indx, ch_indx);	
			MEfill(MAX_BUF_LENGTH-next_indx, 0, streambuf+next_indx);	
			ch_indx = next_indx;
		}
		else if (next_ch == ETX) /* control-c */
		{
		        TEresettmattr();
			/* raise an interrupt */
			EXsignal(EXINTR, 0);
		}
		else if (next_ch == EOT) /* control-c */
		{
		        TEresettmattr();
			/* raise an interrupt */
			return(ENDFILE);
		}
		else if (ch_indx >= n-1)
		{
			SIungetc(next_ch, stream);
			break;
		}
		else if (next_ch == '\t') /* Tab */
		{
			/* Ignore tab operation */
		}
		else if (next_ch == '\n' || next_ch == '\r')
		{
			streambuf[ch_indx] = next_ch;
			if (next_indx < ch_indx)
			{
				SIprintf("%s", streambuf+next_indx);
			}
			else
				SIprintf("%c", next_ch);
			ch_indx++;
			next_indx++;
			curline = 1;
		}
		else if (CMcntrl(&next_ch))
		{
			/* Ignore other control characters like ^L */
		}
		else  /* All other characters -- printable */
		{
			if (ch_indx == next_indx)
			{
				SIprintf("%c", next_ch);
				streambuf[ch_indx] = next_ch;
			}
			else
			{
				SIprintf("%c", next_ch);
				SIprintf("%s", streambuf+next_indx);
				for (i=0; i < ch_indx - next_indx; i++)
					SIprintf("\b");
				STlcopy(streambuf, tempbuf1, next_indx);
				STcopy(streambuf+next_indx, tempbuf2);
				MEfill(MAX_BUF_LENGTH, 0, streambuf);
				STprintf(onechar, "%c\0", next_ch);
				STpolycat(3, tempbuf1, onechar, tempbuf2, streambuf);
			}
			ch_indx++;
			next_indx++;
			curline = 1;
		}
	}	/*end of while loop */

	if ((ch_indx >= n-1) && next_ch != '\n' && next_ch != '\r')
		SIprintf("\n");
	
	if (next_ch == EOF)
	{
		TEresettmattr();
		return ENDFILE;
	}

	STcopy(streambuf, buf); /* set output parameter */

	/* donot put newline in history */
	if (streambuf[STlength(streambuf) - 1] == '\n')
		streambuf[STlength(streambuf) - 1] = '\0';
	STtrmwhite(streambuf); /* Remove trailing spaces */
	SIaddhistnode(streambuf); /* Add line to the history at the end */

	if (TEresettmattr() < 0)
		return ENDFILE;

	return OK;
}

