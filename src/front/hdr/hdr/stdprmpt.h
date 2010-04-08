/*
**	Copyright (c) 2004 Ingres Corporation
*/

# define CONF_DELETE	1
# define CONF_END	2
# define CONF_QUIT	3
# define CONF_DESTROY	4
# define CONF_REMOVE	5

# define CONF_GENERIC	99

# define CONFCH_YES	1
# define CONFCH_NO	2
# define CONFCH_CANCEL	3

/*
** BOXPROMPT structure:
**
** appearance of generated prompt:
**
**	+-------------------------------------------+
**	| TEXT........................              |
**	| ....................................      |
**	| SHORT_TEXT ____________________________   |
**	+-------------------------------------------+
**
**	Ok  Cancel  ListChoices  Help
*/
 
/*
** text - prompt text.
**
**	Newlines may be embedded in the text string to control line breaks.
**	Text wider than the screen will be broken on word boundaries.  If
**	the maximum width required for a text line and the prompt area is
**	smaller than the screen width the display box will be shrunk to
**	fit.  Leading/trailing whitespace is NOT trimmed - this may
**	be used to control text placement.  Extra newlines may be used
**	to force blank lines.  In particular, a newline on the end of
**	the string will force a blank line between the last line of the
**	text and the prompt area.  DO NOT USE TABS.
**
** short_text - if non-NULL, displayed to the left of the prompt area,
**	separated by one space - caller must provide the ":" or "?"
**	or whatever punctuation is desired.
**
** positioning - x,y >= 0 specify screen coordinate of upper
**	left hand box corner.  0 BASED addressing.
**
**	special values may be specified for specific screen
**	positioning (these values are < 0):
**		GRAV_LEFT, GRAV_RIGHT, GRAV_TOP,
**		GRAV_BOTTOM, GRAV_CENTER, GRAV_FLOAT
**	x = GRAV_TOP/BOTTOM or y = GRAV_LEFT/RIGHT, or x,y =
**	undefined negative values yield undefined results.
**
** buffer - buffer for user input.
**
** len - length of input allowed.  buffer must point to len+1 chars of
**	storage to allow for EOS.
**
** displen - display length.  If <= 0 or >len will be set = len.  If > 0 and
**	< len, prompt area will be set to this length and scrolled.
**
** initbuf - initial value to be placed into prompt area - may be NULL.
**	(initbuf = buffer is legal)
**
** htitle, hfile - Help title and Help file name.  htitle = NULL to specify
**	no help.
**
** lfn - Listchoices handler.  If NULL, no ListChoices menuitems is presented.
**
** 	Call sequence:
**
**	(*lfn)(buffer,len)
**	char *buffer;
**	i4 len;
**
**	This routine will probably call some variant of listpick.  buffer
**	is passed in with the current contents of the prompt area.  Returns
**	TRUE to indicate that contents of prompt area is to be replaced with
**	what was passed back in buffer, FALSE otherwise.  The len argument
**	is the len item of the structure, not the current string length.
**
** vfn - Verfication routine.
**
**	Calling sequence identical to lfn.  If non-NULL, this routine is
**	called to verify the validity of the user's input.  It should
**	either produce an error message explaining what is wrong with the
**	input, and return FALSE, or return TRUE.
**
**	(*vfn)() may also be used to "massage" the input in some way,
**	such as lower-casing it or removing blanks before returning TRUE.
*/

typedef struct
{
	char *text;		/* prompt text */
	char *short_text;	/* short prompt for same line as prompt area */
	i4 x,y;		/* positioning */
	char *buffer;		/* user buffer */
	i4 len;		/* length of buffer */
	i4 displen;		/* length of display field for prompt */
	char *initbuf;		/* initial value */
	char *htitle;		/* help screen title */
	char *hfile;		/* help file name */
	bool (*lfn)();		/* Listchoices handler */
	bool (*vfn)();		/* verification function */
} BOXPROMPT;

/* Prototypes defined in stdprmpt */

FUNC_EXTERN bool IIUFbpBoxPrompt(BOXPROMPT *);
