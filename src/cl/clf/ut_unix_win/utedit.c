/*
**	Copyright (c) 1985, 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<ds.h>
# include	<pc.h>
# include	<lo.h>
# include	<nm.h>
# include	<st.h>
# include	<PCerr.h>
# include	<ut.h>
# include	"UTi.h"

/**
** Name:	utedit.c -	Edit file.
**
**	Function:
**		UTedit
**
**	Arguments:
**		char		*editor;	* name of editor to spawn *
**		LOCATION	*filename;	* name of file to edit    *
**
**	Result:
**		edit the file 'filename' with 'editor'.  If 'editor' is
**			NULL or can't be found the default editor is spawned.
**
**	Side Effects:
**		Obviously 'filename' can be changed.
**
**	History:
**
**		Revision 3.1  85/06/05  20:12:59  wong
**		Changed to ignore exit status of editor.
**		
**		Revision 3.0  85/05/08  12:21:01  wong
**		Deleted extraneous function declarations.
**		
**		03/83 -- (gb)
**			written
**
**		12/84 (jhw) -- 3.0 Integration.
**		06/01/87 (daveb& lin) PCwait now take a pid by value.
**			  It is fixed so the old bug requiring a PCwait
**			  sometimes after a failed PCspawn is no longer
**			  required.
**	30-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Look at VISUAL and EDITOR before ING_EDIT, because ING_EDIT
**		will be set at system level default, and you want the user's
**		EDITOR to win.
**	8-may-90 (blaise)
**	    NMgtAt is a void and therefore cannot be compared with OK. 
**	    Changed successive calls to NMgtAt, which check for VISUAL, EDITOR
**	    and ING_EDIT, from a long if to an iterative loop, which no
**	    longer attempts to compare the function value with OK.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	19-jun-95 (emmag)
**	    The default editor on NT should be notepad.exe
**	26-jun_95 (sarjo01)
**	    For NT, just do system() to launch edit session.
**	15-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	24-nov-1997 (canor01)
**	    Launch subprocess with PC_WAIT, not PC_NO_WAIT.  Let the code
**	    in PC deal with waiting for the process to end.
**  12-may-98 (kitch01)
**		Bug 88211. On Windows95 system() does not wait for the called process
**		to exit before control is returned.Modified to call pcdowindowcmdline()
**		to run the chosen editor and wait for completion.
**      03-aug-1999 (rigka01) - bug# 96875
**	    As a result of change 438134 for bug/sir #93752, when the ING_EDIT
**	    editor is spawned successfully and exits with a return code,
**	    that return code is now returned in UTstatus. This incorrectly
**	    causes this routine to attempt to start the default editor.
**	    This routine has been changed to remove this attempt in that
**	    case.
**	15-Jun-2004 (schka24)
**	    Safe env variable handling.
**	17-Jun-2004 (schka24)
**	    I missed the input param, also untrustworthy.
*/

# ifdef NT_GENERIC
static	char	def_editor[]	=	"notepad.exe";
# else
static	char	def_editor[]	=	"/bin/ed";
# endif

STATUS
UTedit(editor, filename)
char		*editor;
LOCATION	*filename;
{
    char	user_edit[MAX_LOC];	/* store the name of the editor */
    char	*u_edit;        	/* set by NMgtAt on ING_EDIT */
    char	*fname;			/* fname for system call */
    PID		pid = 0;        	/* pid of editor process */
    bool	using_def;	/* TRUE when user doesn't specify an editor */
    char	edit_vars[3][16];	/* store names of environment variables
						VISUAL, EDITOR, ING_EDIT */
    int		i;


    UTstatus = OK;
    
    /*
    ** we're requiring the caller to name the file to edit,
    **	-- it doesn't have to exist.
    */

    if ((filename == (LOCATION *)NULL) ||
            (LOtos(filename, &fname), fname == (char *)NULL))
        UTstatus = UT_ED_CALL;
    else
    {
        /*
        **	if user specified an editor
        **		use it
        **	else if VISUAL || EDITOR || ING_EDIT is defined
        **		use it
        **	else
        **		use the default editor
        */

        if (!(using_def = ((editor == (char *)NULL) || (*editor == EOS))))
            STlcopy(editor, user_edit, sizeof(user_edit)-1);
        else
	{
		STcopy("VISUAL", edit_vars[0]);
		STcopy("EDITOR", edit_vars[1]);
		STcopy("ING_EDIT", edit_vars[2]);
		for (i=0; i<=2; i++)
		{
			NMgtAt(edit_vars[i], &u_edit);
                        if (u_edit != NULL && *u_edit != EOS)
			{
                		STlcopy(u_edit, user_edit, sizeof(user_edit)-1);
				break;
			}
			else
			if (i == 2)
			{
                		STcopy(def_editor, user_edit);
			}
		}
	}

# ifndef NT_GENERIC

        if (UTstatus == OK)
        {
#           define	MAXARG	2		/* no. of args to PCspawn */
            char	*argv[MAXARG+1];	/* arguments to PCspawn */
            /*
            ** set up arguments to PCspawn
            */

            argv[0]        = user_edit;
            argv[1]        = fname;
            argv[MAXARG]   = (char *)NULL;

            /*
            ** if couldn't spawn user specified editor,
            ** spawn the default
            */

	    UTstatus = PCspawn((i4) MAXARG, argv, PC_WAIT,
                        (LOCATION *)NULL,
                        (LOCATION *)NULL, &pid);
	    /* Note that PCspawn could return FAIL.  The value 
	    ** of FAIL is 1.   A successfully spawned child
	    ** may also return a ret code of 1.  For ret codes
	    ** returned by a successfuly spawned child, we don't 
	    ** want to spawn the default editor.  But for any
	    ** failure to spawn the child, we do want to spawn
	    ** the default editor.  Therefore, we must differentiate
	    ** between these two meanings of UTstatus=1.  Thus
	    ** pid value is checked as well as UTstatus.  If pid == -1,
	    ** we know that there was an attempt to spawn the child
	    ** process but it was unsuccessful.  If pid > 0, 
	    ** the process was spawned and only UTstatus > PC_CM_CALL
	    ** indicates an error that is not the exit code from
	    ** the spawned process.	 
	    **
	    ** The value of pid remains at 0 if PCspawn did not
	    ** attempt to spawn the process (due to earlier recognition
	    ** of an error condition). 
	    */ 
	    if (((pid == -1) || (pid == 0) ||
		 ((pid > 0) && (UTstatus >= PC_CM_CALL)))
                    && !using_def)
            {
                argv[0]	  = def_editor;

                UTstatus  = PCspawn((i4) MAXARG, argv, PC_WAIT,
                            (LOCATION *) NULL, 
                            (LOCATION *) NULL, 
                            &pid);
            }
	    else
	     if ((pid > 0) && (UTstatus < PC_CM_CALL))
		UTstatus = OK;

        }

# else  /* NT_GENERIC */
        if (UTstatus == OK)
        {
	    char	cmdline[256];
		CL_ERR_DESC	clerror;

	    sprintf(cmdline, "%s %s", user_edit, fname);
		UTstatus = PCdowindowcmdline((LOCATION *)NULL, cmdline, PC_WAIT, FALSE, NORMAL, NULL, &clerror);
        }
# endif
    }

    return (UTstatus == PC_WT_BAD ? OK : UTstatus);

}
