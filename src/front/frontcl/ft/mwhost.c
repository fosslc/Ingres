/*
**	MWhost.c
**	"@(#)mwhost.c	1.52"
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include <compat.h>
# include <cv.h>		 
# include <lo.h>
# include <me.h>
# include <nm.h>
# include <si.h>
# include <st.h>
# include <te.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <adf.h>
# include <fe.h>
# include <fmt.h>
# include <ft.h>
# include <frame.h>
# include <kst.h>
# include <tdkeys.h>
# include <mapping.h>
# include "mwproto.h"
# include "mwmws.h"
# include "mwhost.h"
# include "mwform.h"
# include "mwintrnl.h"

# ifdef DATAVIEW
/*
** Name:	MWhost.c - Host Frontend Routines to Support MacWorkStation.
**
** Usage:
**	INGRES FE system and MWhost on Macintosh.
**
** Description:
**	File contains routines to handle MacWorkStation frontends.
**
**	Routines defined here are:
**	  High Level Interface:
**	  - IIMWiInit		Initialize the MWS modules.
**	  - IIMWcClose		Shut down the MWS modules.
**	  - IIMWomwOverrideMW	Permanently disable MWS modules.
**	  - IIMWsmwSuspendMW	Suspend the MWS modules.
**	  - IIMWrmwResumeMW	Resume the MWS modules.
**	  - IIMWkfnKeyFileName	Do keymap file name accroding to keyboard.
**	  - IIMWkmiKeyMapInit	Send the key map tbls to MWS.
**	  - IIMWemEnableMws	Enable/disable MWS modules.
**	  - IIMWimIsMws		Find out if MWS active.
**	  - IIMWscmSetCurMode	Set mode of operation.
**	  - IIMWduiDoUsrInp	Let user interact with the form.
**	  - IIMWdlkDoLastKey	Tell MWS to process the last key.
**	  - IIMWsmeSetMapEntry	Set an entry in a mapping array.
**	  - IIMWrmeRstMapEntry	Tentatively set an entry in a mapping array.
**	  - IIMWcmtClearMapTbl	Clear an FRS keymap table.
**
**	  Utility Routines:
**	  - IIMWofOpenFile	Open a file.
**	  - IIMWpelPrintErrLog	Print msg. in the error log.
**	  - IIMWplPrintLog	Print an entry in the log file.
**
**	  MWS Protocol Routines:
**	  - IIMWdvvDViewVersion	Process DataView info.
**	  - IIMWmvMwsVersion	MWS version response from P006 message.
**	  - IIMWsvSysVersion	System version response from P007 message.
**	  - IIMWqhvQueryHostVar	Query the state of a host application variable.
**	  - IIMWshvSetHostVar	Modify a host application variable.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	05/22/90 (dkh) - Changed NULLPTR and NULLFILE to NULL for VMS.
**	07/10/90 (dkh) - Integrated more MACWS changes.
**	08/23/90 (nasser) - Send X011 if failure during initialization.
**	09/16/90 (nasser) - Added IIMWscmSetCurMode().
**	09/16/90 (nasser) - Added IIMWdlkDoLastKey() and IIMWdvv().
**		Added parameters to IIMWscm() for key intercept and
**		key find.
**	10/08/90 (nasser) - Moved IIMWof() from mwio.c.
**	10/08/90 (nasser) - Changed use from mwio calls to TE calls.
**	10/10/90 (nasser) - Changed call to IIMWdrDoRun() to
**		the new interface.
**	10/10/90 (nasser) - Added IIMWkfnKeyFileName().
**	10/18/90 (dkh) - Added include of fe.h so DATAVIEW is defined
**			 for VMS.  This is necessary since UNIX does
**			 not have the correct CL changes yet.
**	11/28/90 (nasser) - Added mws_suspended variable to keep
**		track of when MWS is in suspended state.
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	21-apr-97 (cohmi01)
**	    Rename theVersionInfo to IIMWVersionInfo, make global. (bug 81574)
**	    Add KS_SCRTO, similar to KS_GOTO but can scroll off of screen.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      03-May-2001 (fanra01)
**          Sir 104700
**          Add IIJS capability flag to function parameters.  Call TErestore
**          accordingly.
*/

GLOBALREF	i4	IIMWmws;
GLOBALREF       MWSinfo IIMWVersionInfo;

static i4		 override = FALSE;
static i4		 mws_initted = FALSE;
static i4		 mws_suspended = FALSE;
static char		*log_env = "MW_LOG_FILE";
static char		*err_env = "MW_ERR_FILE";
static FILE		*log_fp = NULL;
static FILE		*err_fp = NULL;
static i4		 err_ctrl = mwERR_GENERIC | mwERR_SYSTEM
				| mwERR_LOG_FILE | mwERR_OUT_SYNC
				| mwERR_BMSG_CLASS | mwERR_BAD_MSG;
static i4		 log_ctrl = mwLOG_GENERIC | mwLOG_EXTRA_MSG
				| mwLOG_MAC_INFO | mwLOG_MWS_INFO
				| mwLOG_PUT_VAL | mwMSG_ALL_RCVD
				| mwMSG_ALL_SENT;
static  struct mapping  *kmap = NULL;
static  struct mapping  *l_kmap = NULL;
static  struct frsop    *frsmap = NULL;
static  struct frsop    *l_frsmap = NULL;
static  i2              *froper = NULL;
static  i2              *l_froper = NULL;
static  i1              *fropflag = NULL;
static  i1              *l_fropflag = NULL;
static	i4		 kmap_size = 0;
static	i4		 frsop_size = 0;
static	i4		 froper_size = 0;
static	bool		 km_active = FALSE;
static	char		 rme_map = '\0';
static	i4		 rme_index = -1;
static	KEYSTRUCT	 keystr = {0};
static	MWsysInfo	 theSysInfo = {0};
static	bool		 mws_does = FALSE;
static	bool		 compat_result = TRUE;

/*{
** Name:  IIMWiInit -- Initialize MacWorkStation
**
** Description:
**	Initialize MacWorkStation modules; find out if using
**	MWS; establish connection.
**
** Inputs:
**	MW	terminal capability for MWS; TRUE if MWS active
**	JS	terminal capability for JS;  TRUE if JS active
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser) - Initial definition.
**	23-aug-90 (nasser) - Send X011 if failure during initialization.
**      03-May-2001 (fanra01)
**          Add JS capability flag to function parameter list.
*/
STATUS
IIMWiInit( i4 MW, i4 JS )	/* Terminal capability for MWS */
{
	bool	x010_sent = FALSE;
	bool	status;
        i4      temode;

        /*
        ** if neither of the capabiliies flags are set continue.
        */
	if (override || !(MW || JS) )
		return(OK);

	/* open the error log */
	_VOID_ IIMWofOpenFile(err_env, "a", &err_fp, (i4) 0);

	/* open the log file */
	_VOID_ IIMWofOpenFile(log_env, "w", &log_fp, (i4) mwERR_LOG_FILE);

	/* Initialize I/O */
        temode = (MW) ? TE_CMDMODE : (JS) ? TE_IO_MODE : TE_CMDMODE;

        if (TErestore( temode ) != OK)
        {
            if (log_fp != NULL)
                _VOID_ SIclose(log_fp);
            if (err_fp != NULL)
                _VOID_ SIclose(err_fp);
            return(FAIL);
        }

	/* Initiate protocol with MWS */
	status = IIMWbfsBegFormSystem();
	if (status == OK)
		x010_sent = TRUE;
	if ((status == FAIL) ||
		(IIMWgsiGetSysInfo() == FAIL) ||
		! compat_result)
	{
		if (x010_sent)
			_VOID_ IIMWefsEndFormSystem();
		_VOID_ TErestore(TE_FORMS);
		if (log_fp != NULL)
			_VOID_ SIclose(log_fp);
		if (err_fp != NULL)
			_VOID_ SIclose(err_fp);
		return(FAIL);
	}

	mws_initted = TRUE;
	IIMWmws = TRUE;

	return(OK);
}

/*{
** Name:  IIMWcClose -- End this session with MacWorkStation.
**
** Description:
**	 Display the message, free memory, and close connection
**
** Inputs:
**	msg	Message to be displayed.
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWcClose(msg)
char	*msg;
{
	STATUS	stat = OK;

	if ( ! IIMWmws)
		return(OK);

	_VOID_ IIMWfmFlashMsg(msg, FALSE);
	stat = IIMWefsEndFormSystem();
	IIMWcfCloseForms();
	_VOID_ TErestore(TE_FORMS);
	if (log_fp != NULL)
		_VOID_ SIclose(log_fp);
	if (err_fp != NULL)
		_VOID_ SIclose(err_fp);
	IIMWmws = FALSE;
	mws_initted = FALSE;
	return(stat);
}

/*{
** Name:  IIMWomwOverrideMW -- Permanently disable MWS modules.
**
** Description:
**	If this functio is called before a call to IIMWiInit()
**	is made, MWS will not be initialized.
**	This function must be called before IIMWiInit().
**
** Inputs:
**	None.
**
** Outputs:
** 	Returns:
**		None.
**
** Side Effects:
**
** History:
**	30-jul-90 (nasser)
**		Initial definition.
*/
STATUS
IIMWomwOverrideMW()
{
	if (mws_initted)
		return(FAIL);
	override = TRUE;
	return(OK);
}

/*{
** Name:  IIMWsmwSuspendMW -- Suspend the MWS modules.
**
** Description:
**	This function suspends the MWS modules.  It disables
**	all the MWS functions on the host.  In addition, it
**	sends a suspend command to the Mac.  In response to
**	this command, the Mac goes into tty mode.
**
** Inputs:
**	None.
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	23-dec-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWsmwSuspendMW()
{
	if ( ! IIMWmws)
		return(OK);
	
	if ((IIMWsfsSuspendFormSystem() == FAIL) ||
		(TErestore(TE_FORMS) == FAIL))
	{
		return(FAIL);
	}
	
	IIMWemEnableMws(FALSE);
	mws_suspended = TRUE;
	return(OK);
}

/*{
** Name:  IIMWrmwResumeMW -- Resume the MWS modules.
**
** Description:
**	This function resumes the MWS modules.  It enables
**	all the MWS functions on the host.  In addition, it
**	sends a resume command to the Mac.  In response to
**	this command, the Mac goes back into forms mode.
**
** Inputs:
**	None.
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	23-dec-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWrmwResumeMW()
{
	if ( ! mws_initted)
		return(OK);
	if (IIMWmws)
		return(FAIL);
	
	if ((TErestore(TE_CMDMODE) == FAIL) ||
		(IIMWrfsResumeFormSystem() == FAIL))
	{
		return(FAIL);
	}
	
	mws_suspended = FALSE;
	IIMWemEnableMws(TRUE);
	return(OK);
}

/*{
** Name:  IIMWkfnKeyFileName -- Do keymap file name accroding to keyboard.
**
** Description:
**	This function modifies the filename according to the
**	keyboard type.
**	It assumes that for MWS the filename starts with mws00.
**
** Inputs:
**	name	Name of the file gotten from termcap.
**
** Outputs:
**	name	Modified name.
** 	Returns:
**		STATUS
**
** Side Effects:
**
** History:
**	10-oct-90 (nasser) -- Initial definition.
*/
STATUS
IIMWkfnKeyFileName(name)
char	*name;
{
	char	buf[8];

	if ( ! IIMWmws)
		return(OK);
	if (STbcompare(name, 5, "mws00", 5, TRUE) == 0)
	{
		_VOID_ STprintf(buf, "%02d", theSysInfo.keyBoardType);
		MEcopy(buf, (u_i2) 2, &name[3]);
	}
	return(OK);
}

/*{
** Name:  IIMWkmiKeyMapInit -- Send the key map tbls to MWS.
**
** Description:
**	Send the three key map tables to MWS.  Go through
**	each table and send the significant entries.
**
** Inputs:
**	map1		pointer to control and function key map
**	map1_sz		size of map1
**	map2		pointer to frs key map
**	frsmap_sz	size of map2
**	frop		pointer to map of key operations
**	frop_sz		size of frop
**	flg		pointer to flags for key operations
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	16-dec-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWkmiKeyMapInit(map1, map1_sz, map2, frsmap_sz, frop, frop_sz, flg)
struct mapping	*map1;
i4		 map1_sz;
struct frsop	*map2;
i4		 frsmap_sz;
i2		*frop;
i4		 frop_sz;
i1		*flg;
{
	i4	 i;

	if ( ! IIMWmws)
		return(OK);

	/* Initialize the local variables for the various maps */
	kmap = map1;
	kmap_size = map1_sz;
	l_kmap = (struct mapping *)
			MEreqmem(0, (u_i4) map1_sz * sizeof(struct mapping),
				 FALSE, (STATUS *) NULL);
	if (l_kmap == NULL)
		return(FAIL);
	frsmap = map2;
	frsop_size = frsmap_sz;
	l_frsmap = (struct frsop *)
			MEreqmem(0, (u_i4) frsmap_sz * sizeof(struct frsop),
				 FALSE, (STATUS *) NULL);
	if (l_frsmap == NULL)
		return(FAIL);
	froper = frop;
	froper_size = frop_sz;
	fropflag = flg;
	l_froper = (i2 *) MEreqmem(0, (u_i4) frop_sz * sizeof(i2),
				   FALSE, (STATUS *) NULL);
	if (l_froper == NULL)
		return(FAIL);
	l_fropflag = MEreqmem(0, (u_i4) frop_sz * sizeof(i1),
			      FALSE, (STATUS *) NULL);
	if (l_fropflag == NULL)
		return(FAIL);

	/* Send the maps to MWS and initialize local maps */
	for (i = 0; i < map1_sz; i++)
	{
		l_kmap[i].cmdval = kmap[i].cmdval;
		l_kmap[i].cmdmode = kmap[i].cmdmode;
		if (kmap[i].cmdval != ftNOTDEF)
			_VOID_ IIMWfksFKSet(mwKMAP, i, kmap[i].cmdval,
				kmap[i].cmdmode);
	}

	for (i = 0; i < frsmap_sz; i++)
	{
		l_frsmap[i].intrval = frsmap[i].intrval;
		if (frsmap[i].intrval != fdopERR)
			_VOID_ IIMWfksFKSet(mwFRSMAP, i, frsmap[i].intrval, -1);
	}

	for (i = 0; i < frop_sz; i++)
	{
		l_froper[i] = froper[i];
		l_fropflag[i] = fropflag[i];
		if (froper[i] != fdopERR)
			_VOID_ IIMWfksFKSet(mwKEYOP, i, froper[i], fropflag[i]);
	}

	km_active = TRUE;

	return(OK);
}

/*{
** Name:  IIMWemEnableMws -- Enable/disable MWS modules.
**
** Description:
**	This function provides the means to temporarily
**	enable/disable the MWS modules.
**
** Inputs:
**	enable		Enable MWS if TRUE; Disable MWS if FALSE.
**
** Outputs:
** 	Returns:
**		None.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	07-dec-89 (nasser)
**		Initial definition.
*/
VOID
IIMWemEnableMws(enable)
i4	enable;
{
	if ( ! mws_initted || mws_suspended)
		return;

	if (enable)
	{
		if ( ! IIMWmws)
			IIMWmws = TRUE;
	}
	else
	{
		if (IIMWmws)
			IIMWmws = FALSE;
	}
}

/*{
** Name:  IIMWimIsMws -- Are we using MWS?
**
** Description:
**	This function returns true if MacWorkStation is active.
**
** Inputs:
**	None.
**
** Outputs:
** 	Returns:
**		TRUE	if MWS is active.
**		FALSE	otherwise.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	17-oct-89 (nasser)
**		Initial definition.
*/
i4
IIMWimIsMws()
{
	return(IIMWmws);
}

/*{
** Name: IIMWscmSetCurMode -- Set mode of operation.
**
** Description:
**	IIMWscmSetCurMode() allows the host application to define the
**	mode, given by curMode, of operation. The mode defines
**	whether the user can modify the textual value of the
**	current item on the form, or if the user can only make a
**	menu selection. 
**	
** Inputs:
**	runMode		run mode
**	curMode		current item mode
**	kyint		key intercept active or not
**	keyfind		key find active or not
**	keyfinddo	doing key find or not
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	16-sep-90 (nasser) - Initial definition & implementation.
*/
STATUS
IIMWscmSetCurMode(runMode, curMode, kyint, keyfind, keyfinddo)
i4	runMode;
i4	curMode;
bool	kyint;
bool	keyfind;
bool	keyfinddo;
{
    i4		flags = 0;

    if ( ! IIMWmws)
	return(OK);

    if (kyint)
	flags |= MWKEYINT;
    if (keyfind)
	flags |= MWKEYFIND;
    if (keyfinddo)
	flags |= MWKEYFINDDO;

    return(IIMWsrmSetRunMode(runMode, curMode, flags));
}

/*{
** Name:  IIMWduiDoUsrInput -- Let the user interact with the form.
**
** Description:
**	Give control to MacWorkStation so that the user can interact
**	with the form.
**
** Inputs:
**	None.
**
** Outputs:
**	ks	Pointer to structure representing key pressed.
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser) - Initial definition.
*/
STATUS
IIMWduiDoUsrInput(ks)
KEYSTRUCT	**ks;
{
    i4		key;
    i4		flags;
    char	buf[mwMAX_MSG_LEN + 1];

    if ( ! IIMWmws)
	return(OK);

    /* wait for event */
    if (IIMWdrDoRun(&key, &flags, buf, &keystr) == FAIL)
    	return(FAIL);
    if (flags & MWFLD_CHNGD)
	IIMWpvPutVal(TRUE, buf);
    if (flags & MWMWS_DOES)
	mws_does = TRUE;
    else
	mws_does = FALSE;

    keystr.ks_fk = 0;
    keystr.ks_ch[0] = NULLCHAR;

    if (key < 0)
    {
	/*
	** special KS_XX type keys.
	** adjust the parameters passed back by MWS.
	*/
	keystr.ks_fk = key;
	if (key == KS_GOTO || key == KS_SCRTO)
	{
	    keystr.ks_p1 -= FLDINX;
	    if (key == KS_GOTO)
	    {
		/* GOTO is relative to 0-based scr pos */
	    	keystr.ks_p2--;
	    	keystr.ks_p3--;
	    }
	}
	else if (key == KS_MFAST)
	{
	    keystr.ks_p1--;
	}
	else if (key == KS_HOTSPOT)
	{
	    keystr.ks_p1 -= TRIMINX;
	}
    }
    else if (key < KEYOFFSET)
    {
	keystr.ks_fk = 0;
	keystr.ks_ch[0] = (u_char) key;
	keystr.ks_ch[1] = NULLCHAR;
    }
    else if (key < ftMUOFFSET)
    {
	keystr.ks_fk = key;
    }
    *ks = &keystr;
    return(OK);
}

/*{
** Name:  IIMWdlkDoLastKey -- Tell MWS to process the last key.
**
** Description:
**	In key intercept or key find modes, MWS passes the key
**	back to the host. MWS also sends info whether MWS or host
**	would handle the key if the key is not to be used by
**	higher levels than FT. This function returns info about
**	handles the key. In addition, if MWS is to handle the key,
**	it tells MWS so. If the ignore parameter is set, then MWS
**	will ignore the key.
**
** Inputs:
**	ignore		the key is to be ignored.
**
** Outputs:
**	do_on_host	FT should process the key.
** 	Returns:
**		STATUS
** 	Exceptions:
**
** Side Effects:
**
** History:
**	16-sep-90 (nasser) - Initial definition.
*/
STATUS
IIMWdlkDoLastKey(ignore, do_on_host)
bool	 ignore;
bool	*do_on_host;
{
    *do_on_host = TRUE;
    if (! IIMWmws)
	return(OK);
    if (! mws_does && ! ignore)
	return(OK);
	
    *do_on_host = FALSE;
    return(IIMWplkProcLastKey(ignore));
}

/*{
** Name:  IIMWsmeSetMapEntry -- Set an entry in one of the mapping arrays.
**
** Description:
**	INGRES uses three mapping arrays, keymap, froperation
**	and frsmap to map keyboard inputs into opearations.
**	In order to determine which keys are valid, and what
**	operation they are bound to, the Macintosh needs to
**	have these arrays as well.  IIMWsmeSetMapEntry is
**	used to send the value of an entry in one of these
**	arrays to the Mac.
**	
**
** Inputs:
**	map_type	Indicator of which map
**	index		Index of entry to change
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWsmeSetMapEntry(map_type, index)
char	map_type;
i4	index;
{
	i2	val;
	i1	flg;

	if ( ! IIMWmws)
		return(OK);
	if ( ! km_active)
		return(OK);

	if (rme_index != -1)
	{
		if ((rme_map != map_type) || (rme_index != index))
		{
			/* The check above should prevent runaway recursion */
			if (IIMWsmeSetMapEntry(rme_map, rme_index) == FAIL)
				return(FAIL);
		}
		rme_index = -1;
	}

	switch (map_type)
	{
	case mwKMAP:
		if ((kmap[index].cmdval == l_kmap[index].cmdval) &&
		    (kmap[index].cmdmode == l_kmap[index].cmdmode))
		{
			flg = -1;
		}
		else
		{
			val = kmap[index].cmdval;
			flg = kmap[index].cmdmode;
			l_kmap[index].cmdval = kmap[index].cmdval;
			l_kmap[index].cmdmode = kmap[index].cmdmode;
		}
		break;

	case mwFRSMAP:
		if (frsmap[index].intrval == l_frsmap[index].intrval)
		{
			flg = -1;
		}
		else
		{
			val = frsmap[index].intrval;
			flg = fdcmINBR;
			l_frsmap[index].intrval = frsmap[index].intrval;
		}
		break;

	case mwKEYOP:
		if ((froper[index] == l_froper[index]) &&
		    (fropflag[index] == l_fropflag[index]))
		{
			flg = -1;
		}
		else
		{
			val = froper[index];
			flg = fropflag[index];
			l_froper[index] = froper[index];
			l_fropflag[index] = fropflag[index];
		}
		break;
	}

	if (flg == -1)
		return(OK);
	return(IIMWfksFKSet(map_type, index, val, flg));
}

/*{
** Name:  IIMWrmeRstMapEntry -- Tentatively set an entry in a mapping array.
**
** Description:
**	When setting an entry in the keymap array, FT first resets
**	the entry that has the same command value as the new
**	entry if there is such an entry.  The two entries may
**	actually be the same.  To reduce traffic to the Mac, the
**	entry that is being reset is not sent until it has been
**	established that it is different from the new entry.
**	IIMWrmeRstMapEntry records the entry that is being
**	reset.
**
** Inputs:
**	map_type	Indicator of which map
**	index		Index of entry to change
**
** Outputs:
** 	Returns:
**		None.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	09-dec-89 (nasser)
**		Initial definition.
*/
VOID
IIMWrmeRstMapEntry(map_type, index)
char	map_type;
i4	index;
{
	if ( ! IIMWmws)
		return;
	if ( ! km_active)
		return;

	rme_map = map_type;
	rme_index = index;
}

/*{
** Name: IIMWcmtClearMapTbl -- Clear an FRS keymap table.
**
** Description:
**	This routine clears an FRS keymap table. It clears the
**	local copy of the table as well as sends a message to
**	MWS to clear its table.
**	At present, the only table that can be cleared is type
**	mwFRSMAP which is the frsmap table.
**	
** Inputs:
**	map_type	char (which table)
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	07-jul-90 (nasser)
**		Initial definition & implementation.
*/
STATUS
IIMWcmtClearMapTbl(map_type)
char	map_type;
{
	i4	i;

	if ( ! IIMWmws)
		return(OK);
	if (map_type != mwFRSMAP)
		return(FAIL);

	for (i = 0; i < frsop_size; i++)
	{
		l_frsmap[i].intrval = fdopERR;
	}

	return(IIMWfkcFKClear(map_type));
}

/*{
** Name:  IIMWofOpenFile -- Open a file.
**
** Description:
**	Open a file based on the environment variable passed in.
**	The value of the environment variable is read, and if it
**	is not null, it is taken to be the full pathname of the
**	file.
**	Files opened are always of type SI_TXT and the length is
**	(mwMAX_MSG_LEN + 1).
**
** Inputs:
**	file_env	Name of the environment variable
**	mode		Mode to open file in
**	log_ctrl	What, if any, to print in the error log
**
** Outputs:
**	fp	Stream pointer
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	19-dec-89 (nasser) -- Initial definition.
**	08-oct-90 (nasser) -- Moved from mwio.c to here.
*/
STATUS
IIMWofOpenFile(file_env, mode, fp, log_ctrl)
char	*file_env;
char	*mode;
FILE   **fp;
i4	 log_ctrl;
{
	char		*filename;
	LOCATION	 fileloc;
	STATUS		 stat = OK;

	NMgtAt(file_env, &filename);
	if ((filename != NULL) && (*filename != EOS))
	{
		if ((LOfroms(PATH & FILENAME, filename, &fileloc) != OK) ||
			(SIfopen(&fileloc, mode, SI_TXT,
				(i4) (mwMAX_MSG_LEN + 1), fp) != OK))
		{
			*fp = NULL;
			if (log_ctrl != 0)
				IIMWpelPrintErrLog(log_ctrl, filename);
			stat = FAIL;
		}
	}
	else
	{
		*fp = NULL;
	}

	return(stat);
}

/*{
** Name:  IIMWpelPrintErrLog -- Print msg. in the error log.
**
** Description:
**	Print the message "msg" to the error log.
**	The parameter type determines if the msg should
**	be printed.
**
** Inputs:
**	type	Type of message
**	msg	Message to be written to the log file
**
** Outputs:
** 	Returns:
**		None.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	16-dec-89 (nasser)
**		Initial definition.
*/
VOID
IIMWpelPrintErrLog(type, msg)
i4	 type;
char	*msg;
{
	char	buf[mwMAX_MSG_LEN + 1];

	switch (type)
	{

	case mwERR_GENERIC :
		_VOID_ STprintf(buf, "GErr: %s\n", msg);
		break;

	case mwERR_SYSTEM :
		_VOID_ STprintf(buf, "SErr: %s\n", msg);
		break;

	case mwERR_ALIAS :
		_VOID_ STprintf(buf, "AlEr: Alias error in %s\n", msg);
		break;

	case mwERR_LOG_FILE :
		_VOID_ STprintf(buf, "LogF: Can't open log %s\n", msg);
		break;

	case mwERR_OUT_SYNC :
		_VOID_ STprintf(buf, "OSyn: %s\n", msg);
		break;

	case mwERR_BMSG_CLASS :
		_VOID_ STprintf(buf, "BMCl: %s\n", msg);
		break;

	default :
		break;
	}

	if ((err_fp != NULL) && (type & err_ctrl))
	{
		_VOID_ SIputrec(buf, err_fp);
		_VOID_ SIflush(err_fp);
	}
	if ((log_fp != NULL) && (type & err_ctrl))
	{
		_VOID_ SIputrec(buf, log_fp);
		_VOID_ SIflush(log_fp);
	}
}

/*{
** Name:  IIMWplPrintLog -- Print an entry in the log file.
**
** Description:
**	Print the message "msg" to the log file.
**	The parameter type determines if the msg should
**	be printed.
**
** Inputs:
**	type	Type of message
**	msg	Message to be written to the log file
**
** Outputs:
** 	Returns:
**		None.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (nasser)
**		Initial definition.
*/
VOID
IIMWplPrintLog(type, msg)
i4	 type;
char	*msg;
{
	i4	i;

	if ((log_fp == NULL) || !(type & log_ctrl))
		return;

	switch (type)
	{

	case mwLOG_GENERIC :
		SIfprintf(log_fp, "GLog: %s\n", msg);
		break;

	case mwLOG_EXTRA_MSG :
		SIfprintf(log_fp,
			"EMsg: Unexpected message in %s\n", msg);
		break;

	case mwLOG_DIR_MSG :
		SIfprintf(log_fp, "DMsg: %s\n", msg);
		break;

	case mwMSG_READ :
		SIfprintf(log_fp, "READ:");

		for (i = STlength(msg); i > 0; i--, msg++)
		{
			SIfprintf(log_fp, " %x", *msg);
		}
		SIfprintf(log_fp, "\n");
		break;

	case mwMSG_ALL_RCVD :
		SIfprintf(log_fp, "RCVD: %s\n", msg);
		break;

	case mwMSG_ALL_SENT :
		SIfprintf(log_fp, "SENT: %s\n", msg);
		break;

	case mwLOG_MAC_INFO :
	case mwLOG_MWS_INFO :
	case mwLOG_PUT_VAL :
	case mwMSG_OBJ_CRE :
	case mwMSG_OBJ_DES :
	case mwMSG_OBJ_GEN :
	case mwMSG_FLO_CNT :
	case mwMSG_STATE :
	case mwMSG_INS_RCV :
	case mwMSG_INS_SND :
	default :
		break;
	}

	_VOID_ SIflush(log_fp);
}

/*{
** Name:  IIMWdvvDViewVersion -- Process DataView info.
**
** Description:
**	This function takes the P260 message from MWS and extracts
**	the DataView version number. It then checks to see if this
**	version is compatible with the version of the host. It
**	then sends the compatibility info to MWS. The static
**	variable compat_result is set indicating whether this
**	session should proceed or abort.
**
** Inputs:
**	theMsg	Ptr. to message recevied from MWS.
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**
** Side Effects:
**
** History:
**	16-sep-90 (nasser) - Initial definition.
*/
STATUS
IIMWdvvDViewVersion(theMsg)
TPMsg	theMsg;
{
    char	 buf[64];
    i4		 ing_maj;
    i4		 ing_min;
    i4		 rel;
    i4		 rev;
    i4	 dv_ver;
    i4		 cmpt_info;

    /* the string returned is "d.d/dd (dvu.mac/dd)" */
    if ((IIMWmgdMsgGetDec(theMsg, &ing_maj, '.') != OK) ||
    	(IIMWmgdMsgGetDec(theMsg, &ing_min, '/') != OK) ||
    	(IIMWmgdMsgGetDec(theMsg, &rel, ' ') != OK) ||
    	(IIMWmgsMsgGetStr(theMsg, buf, '/') != OK) ||
    	(IIMWmgdMsgGetDec(theMsg, &rev, ')') != OK))
    {
	return(FAIL);
    }

    dv_ver = ing_maj*100000 + ing_min*10000 + rel*100 + rev;

    if (dv_ver <= MWMWS_FATAL)
	cmpt_info = MWNOT_COMPAT;
    else if (dv_ver <= MWMWS_IFFY)
	cmpt_info = MWASK_COMPAT;
    else
	cmpt_info = MWOK_COMPAT;
    return (IIMWdccDoCompatCheck(cmpt_info, MWHOST_VER, &compat_result));
}

/*{
** Name:  IIMWmvMwsVersion -- Extract info about MWS from a message.
**
** Description:
**	P261 - MWS version response from P006
**
** Inputs:
**	theMsg	Pointer to message received from MWS.
**
** Outputs:
** 	Returns:
**		OK/FAIL.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (bspence)
**		Initial definition.
*/
STATUS
IIMWmvMwsVersion(theMsg)
TPMsg theMsg;
{
	MWSinfo *versionInfo = &IIMWVersionInfo;

	if ((IIMWmgdMsgGetDec(theMsg, &versionInfo->version, ';') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &versionInfo->v[0], ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &versionInfo->v[1], ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &versionInfo->v[2], ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &versionInfo->v[3], ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &versionInfo->v[4], ';') == OK) &&
	    (IIMWmgsMsgGetStr(theMsg, versionInfo->text, ';') == OK))
	{
		return(OK);
	}
	else
	{
		return(FAIL);
	}
}

/*{
** Name:  IIMWsvSysVersion -- Extract info about Mac system from msg.
**
** Description:
**	P262 - system version response from P007
**
** Inputs:
**	theMsg	Pointer to message received from MWS.
**
** Outputs:
** 	Returns:
**		OK/FAIL
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (bspence)
**		Initial definition.
*/
STATUS
IIMWsvSysVersion(theMsg)
TPMsg theMsg;
{
	i4 t,l,b,r;
	MWsysInfo *sysInfo = &theSysInfo;

	if ((IIMWmgdMsgGetDec(theMsg, &sysInfo->environsVersion, ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &sysInfo->machineType, ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &sysInfo->systemVersion, ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &sysInfo->processor, ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &sysInfo->hasFPU, ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &sysInfo->hasColorQD, ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &sysInfo->keyBoardType, ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &sysInfo->atDrvrVersNum, ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &sysInfo->sysVRefNum, ';') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &sysInfo->mBarHeight, ';') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &t, ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &l, ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &b, ',') == OK) &&
	    (IIMWmgdMsgGetDec(theMsg, &r, ';') == OK))
	{
		sysInfo->screenSize.top = t;
		sysInfo->screenSize.left = l;
		sysInfo->screenSize.bottom = b;
		sysInfo->screenSize.right = r;
		return(OK);
	}
	else
	{
		return(FAIL);
	}
}

/*{
** Name: IIMWqhvQueryHostVar -- query the state of a host application variable.
**
** Description:
**	This routine initiates a protocol that allows an engineer
**	testing an MWS exec module to query the state of the
**	application running on the remote host. The host application
**	responds to this message by sending a HostVariableResp
**	message back to MWS.
**	Variable is an integer that defines the variable the
**	message refers to. The second parameter is a string.
**	The value of both of these parameters is understood by
**	the debugging code on both MWS and the remote host, but
**	it is outside the scope of this message protocol.
**	
** Inputs:
**	theMsg	Ptr to message recevied from MWS.
**
** Outputs:
**	Returns:
**		STATUS
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWqhvQueryHostVar(theMsg)
TPMsg	theMsg;
{
	i4	variable;
	char	buf[256];

	if (IIMWmgdMsgGetDec(theMsg, &variable, NULLCHAR) != OK)
		return(FAIL);

	switch (variable)
	{
	case mwVAR_ERR :
		_VOID_ STprintf(buf, "%x", err_ctrl);
		break;
	
	case mwVAR_LOG :
		_VOID_ STprintf(buf, "%x", log_ctrl);
		break;
	
	default :
		break;
	}

	return(IIMWhvrHostVarResp(variable, buf));
}

/*{
** Name: IIMWshvSetHostVar -- modify a host application variable.
**
** Description:
**	This routine initiates a protocol that allows an engineer
**	testing an MWS exec module to modify the state of the
**	application running on the remote host. The host application
**	responds to this message by sending a HostVariableResp
**	message back to MWS.
**	Variable is an integer that defines the variable the
**	message refers to. The second parameter is a string.
**	The value of both of these parameters is understood by
**	the debugging code on both MWS and the remote host, but
**	it is outside the scope of this message protocol.
**	
** Inputs:
**	theMsg	Ptr to message recevied from MWS.
**
** Outputs:
**	Returns:
**		STATUS
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	28-feb-92 (leighb) DeskTop Change: Use CVahxl instead of STscanf.
*/
STATUS
IIMWshvSetHostVar(theMsg)
TPMsg	theMsg;
{
	i4	variable;
	char	buf[256];
	i4	value;					 
	STATUS	stat = FAIL;

	if ((IIMWmgdMsgGetDec(theMsg, &variable, ';') == FAIL) ||
		(IIMWmgsMsgGetStr(theMsg, buf, NULLCHAR) == FAIL))
	{
		return(FAIL);
	}

	switch (variable)
	{
	case mwVAR_ERR :
		if (CVahxl(buf, &value) == OK)		 
		{
			err_ctrl = value;
			_VOID_ STprintf(buf, "%x", err_ctrl);
			stat = OK;
		}
		break;
	
	case mwVAR_LOG :
		if (CVahxl(buf, &value) == OK)		 
		{
			log_ctrl = value;
			_VOID_ STprintf(buf, "%x", log_ctrl);
			stat = OK;
		}
		break;
	
	default :
		break;
	}

	if (stat == OK)
		stat = IIMWhvrHostVarResp(variable, buf);
	return(stat);
}

/* Temporary function to reduce complaints about unused function args. */
/*VARARGS */
VOID
IIMWuaUnusedArg()
{
}
# endif /* DATAVIEW */
