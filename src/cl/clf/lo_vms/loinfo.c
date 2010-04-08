/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

# include 	<compat.h>
# include	<gl.h>
# include	<rms.h>
# include	<chpdef.h>
# include	<armdef.h>
# include	<iledef.h>
# include	<ssdef.h>
# include	<psldef.h>
# include	<starlet.h>
# include       <lo.h>
# include 	<er.h>
# include 	<st.h>
# include 	<me.h>
# include       "lolocal.h"
# include	"loparse.h"
# include	<tm.h>
# include	<tmtz.h>

/* External and Internal function references */

FUNC_EXTERN	long int TMconv();
static _init_rms();
static STATUS _do_perms();

/*
** Name: LOinfo()	- get information about a LOCATION
**
** Description:
**
**	Given a pointer to a LOCATION and a pointer to an empty
**	LOINFORMATION struct, LOinfo() fills in the struct with information
**	about the LOCATION.  A bitmask passed to LOinfo() indicates which
**	pieces of information are being requested.  This is so LOinfo()
**	can avoid unneccessary work.  LOinfo() clears any of the bits in
**	the request word if the information was unavailable.  If the
**	LOCATION object doesn't exist, all these bits will be clear.
**
**	The recognized information-request bit flags are:
**		LO_I_TYPE	- what is the location's type?
**		LO_I_PERMS	- give process permissions on the location.
**		LO_I_SIZE	- give size of location.
**		LO_I_LAST	- give time of last modification of location.
**		LO_I_ALL	- give all available information.
**
**	Note that LOinfo only gets information about the location itself,
**	NOT about the LOCATION struct representing the location.  That's
**	why LOisfull, LOdetail, and LOwhat are not subsumed by this
**	interface.
**
**	This interface makes LOexists, LOisdir, LOsize,
**	and LOlast obsolete, and they will be phased out in the usual way.
**	(meaning: they will hang on for years.)
**
** Inputs:
**	loc		The location to check.
**	flags		A word with bits set that indicate which pieces of
**			 information are being requested.  This can save a
**			 lot of work on some machines...
**
** Outputs:
**	locinfo		The LOINFORMATION struct to fill in.  If any
**			 information was unavailable, then the appropriate
**			 struct member(s) are set to zero (all integers),
**			 or zero-ed out SYSTIME struct (li_last).
**			 Information availability is also indicated by the
**			 'flags' word...
**	flags		The same word as input, but bits are set only for
**			 information that was successfully obtained.
**			 That is, if LO_I_TYPE is set and
**			 .li_type == LO_IS_DIR, then we know the location is
**			 a directory.  If LO_I_TYPE is not set, then we
**			 couldn't find out whether or not the location
**			 is a directory.
**
** Returns:
**	STATUS - OK if the LOinfo call succeeded and the LOCATION
**		exists.  FAIL if the call failed or the LOCATION doesn't exist.
**
** Exceptions:
**	none.
**
** Side Effects:
**	none.
**
** Example Use:
**	LOCATION loc;
**	LOINFORMATION loinf;
**	i4 flagword;
**
**	/% find out if 'loc' is a writable directory. %/
**	flagword = (LO_I_TYPE | LO_I_PERMS);
**	if (LOinfo(&loc, &flagword, &loinf) == OK)
**	{
**		/% LOinfo call succeeded, so 'loc' exists. %/
**		if ( (flagword & LO_I_TYPE) == 0
**		  || loinf.li_type != LO_IS_DIR )
**			/% error, we're not sure that 'loc' is a directory. %/
**		if ( (flagword & LO_I_PERMS) == 0
**		  || (loinf.li_perms & LO_P_WRITE) != 0 )
**			/% error, 'loc' might not be writable by us. %/
**
**		/% 'loc' is an existing directory, and we can write to it. %/
**	}
**
** History:
**	2/17/89 - First written (billc).
**	4/19/89 - Use LOparse and LOclean_fab (Mike S)
**		  Make itemlist automatic
**	6/2/89 -  Use expanded string version of name for sys$open. (Mike S)
**	10/89  -  Clean up (Mike S)
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	09/01/93 (dkh) - Fixed bug 49893.  The time for the location will
**			 now be in GMT time with timezone adjustments.
**			 This change is needed since TMconv no longer
**			 returns GMT time.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes and
**	   external function references
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	29-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	31-jan-2008 (toumi01/abbjo03) BUG 115913
**        Introduce for LOinfo a new query type of LO_I_XTYPE that can,
**        without breaking the ancient behavior of LO_I_TYPE, allow for
**        query of more exact file attributes (but these aren't used on
**        VMS). Default to LO_IS_UNKNOWN so as to avoid fall-through logic.
**	09-oct-2008 (stegr01/joea)
**	    Replace our own item list struct by ILE3.
*/

typedef struct NAM _NAM;
typedef struct FAB _FAB;

/*
**	We will check up to 512 bytes of ACL information for the file.
**	This is enough for 99.99 % of all VMS systems.
*/
#define	ACLSIZE	512

STATUS
LOinfo(loc, flags, locinfo)
	LOCATION *loc;
	i4 *flags;
	LOINFORMATION *locinfo;
{
	STATUS  rmsstatus;
	STATUS  dummy;
	LOpstruct pstruct;
	register _FAB *fab = &pstruct.pfab;
	register _NAM *nam = &pstruct.pnam;

	struct XABFHC xab_fhc;
	struct XABPRO xab_pro;
	struct XABDAT xab_dat;
	i4	lflags;
	i4	ltype;
	char	*tmpname;
	char    lname[MAX_LOC];
	char	acl_info[ACLSIZE];

	PTR	tz_cb;

	LOtos(loc,  &tmpname);
	STcopy(tmpname, lname);

	/* initialize the LOINFORMATION struct */
	MEfill((u_i2) sizeof(*locinfo), '\0', (PTR) locinfo);

	lflags = *flags;
	*flags = 0;

	/* parse to determine if directory exists */
	rmsstatus = LOparse(lname, &pstruct, TRUE);
	if ((rmsstatus & 0x01) == 0)
		goto error_cleanup;

	if ((nam->nam$b_name != 0) ||
	    ((loc->desc & FILENAME) == FILENAME))
	{
		/* file was given, so check it */

		rmsstatus = sys$search(fab);

		if ((rmsstatus & 0x01) == 0)
			goto error_cleanup;
	}

	/*
	** Location exists! Now get other info.
	*/

	/* what is this thing? */
	if (nam->nam$b_name == 0)
		ltype = LO_IS_DIR;
	else
		ltype = LO_IS_FILE;

	if ((lflags & LO_I_TYPE) || (lflags & LO_I_XTYPE))
	{
		/*
		** We should be handling many different kinds of
		** objects here, but currently we only do directories
		** and files.  This is definitely a punt.
		*/

		*flags |= LO_I_TYPE;
		locinfo->li_type = ltype;
	}
	else
		locinfo->li_type = LO_IS_UNKNOWN;

	if (ltype == LO_IS_DIR)
	{
	     	/*  Set up directory name in name.dir format.
		**  Start with the expanded string name from the parse. This
		**  has two advantages:
		**  1.	Logical name translation has already been done: we
		**	know we're in "[node::]device:[dir.[dir...]]file.typ;v"
		**	format.
		**  2.  If we were given a search path, we've got the name of
		**	the first directory in the path, which is the sensible
		**	thing to check for permissions.  It's what VMS checks
		**	if you try to create a file in the search path.
 		**
		*/
        	*nam->nam$l_name = EOS;
		LOdir_to_file(pstruct.pesa, lname);
	}

	/* Deallocate I/O channel, etc. */
	LOclean_fab(fab);

	/* The rest of the info requires XABs to be setup.  Leave now if we
	** don't need this.
	*/
	if ( (lflags & (LO_I_SIZE | LO_I_PERMS | LO_I_LAST)) == 0)
		return OK;

	/*
	** OK, chain together as many XABs as we need to get the info.
	*/
	_init_rms(&pstruct, lname);
	fab->fab$l_xab = NULL;

	if ((lflags & LO_I_SIZE) && ltype == LO_IS_FILE)
	{
		/* Initialize the RMS xab. */
		MEfill(sizeof(xab_fhc), '\0', (PTR)&xab_fhc);
		xab_fhc.xab$b_cod = XAB$C_FHC;
		xab_fhc.xab$b_bln = XAB$C_FHCLEN;

		/* chain it on the list. */
		xab_fhc.xab$l_nxt = fab->fab$l_xab;
		fab->fab$l_xab = &xab_fhc;
	}

	if (lflags & LO_I_PERMS)
	{
		/* Initialize the RMS xab. */
		MEfill(sizeof(xab_pro), '\0', (PTR)&xab_pro);
		xab_pro.xab$b_cod = XAB$C_PRO;
		xab_pro.xab$b_bln = XAB$C_PROLEN;
		xab_pro.xab$l_aclbuf = acl_info;
		xab_pro.xab$w_aclsiz = sizeof(acl_info);

		/* chain it on the list. */
		xab_pro.xab$l_nxt = fab->fab$l_xab;
		fab->fab$l_xab = &xab_pro;
	}

	if (lflags & LO_I_LAST)
	{
		/*      Initialize the RMS xab          */
		MEfill(sizeof(xab_dat), '\0', (PTR)&xab_dat);
		xab_dat.xab$b_cod = XAB$C_DAT;
		xab_dat.xab$b_bln = XAB$C_DATLEN;

		/* chain it on the list. */
		xab_dat.xab$l_nxt = fab->fab$l_xab;
		fab->fab$l_xab = &xab_dat;
	}

        /* See if we can open it. */
	if ((sys$open(fab) & 0x01) == 0)
	{
		/*
		** This may seem odd.  However, we believe the location
		** exists, so we'll return without getting any other info.
		*/
		return OK;
	}

	if ((lflags & LO_I_SIZE) && ltype == LO_IS_FILE)
	{
		/* get number of bytes in use */
		locinfo->li_size = (xab_fhc.xab$l_ebk - 1) * 512;
		locinfo->li_size += xab_fhc.xab$w_ffb;

		*flags |= LO_I_SIZE;
	}

	if (lflags & LO_I_PERMS)
	{
		STATUS _do_perms();

		if (_do_perms(&xab_pro, &(locinfo->li_perms)) == OK)
			*flags |= LO_I_PERMS;
	}

	if (lflags & LO_I_LAST)
	{
		i4      vmstime[2];

		/* get revision time*/
		MEcopy((PTR)&xab_dat.xab$q_rdt, sizeof(vmstime), (PTR)vmstime);

		/* return time_return location last modified */
		locinfo->li_last.TM_secs = TMconv(vmstime);
		locinfo->li_last.TM_msecs = 0;

		/*
		**  Convert to GMT since TMconv no longer
		**  returns GMT time.
		*/
		if (TMtz_init(&tz_cb) == OK)
		{
			locinfo->li_last.TM_secs -= TMtz_search(tz_cb,
				TM_TIMETYPE_LOCAL, locinfo->li_last.TM_secs);
		}

		*flags |= LO_I_LAST;
	}

	fab->fab$l_xab = NULL;
	fab->fab$l_fop = NULL;
	dummy = sys$close(fab);

	return ( OK );

error_cleanup:
	LOclean_fab(fab);
	return FAIL;
}

static
_init_rms(pstruct, name)
LOpstruct *pstruct;	/* Parse structure */
char 	  *name; 	/* file name */
{
	register _FAB *fab = &pstruct->pfab;
	register _NAM *nam = &pstruct->pnam;

	/* Initialize the RMS fab. */
	MEfill(sizeof(*fab), '\0', (PTR)fab);
	fab->fab$b_bid = FAB$C_BID;
	fab->fab$b_bln = FAB$C_BLN;
	fab->fab$l_fna = name;
	fab->fab$b_fns = STlength(name);
	fab->fab$l_nam = nam;

	/* Initialize the RMS nam. */
	MEfill(sizeof(*nam), '\0', (PTR)nam);
	nam->nam$b_bid = NAM$C_BID;
	nam->nam$b_bln = NAM$C_BLN;
	nam->nam$b_ess = MAX_LOC;
	nam->nam$l_esa = pstruct->pesa;
}


/*
**	Below are codes for items in the item list, and an explanation of
**	what each specifies.  All are inputs to SYS$CHKPRO; the only output
**	we recieve is whether access is permitted.
*/
#define ACCESS 		0	/* The access requested (e.g. WRITE) */
#define ACMODE		1	/* Our access mode (always USER) */
#define MODE		2	/* The file owner's access mode (again, USER) */
#define MODES		3	/* The modes at which to check access (still USER) */
#define	OWNER 		4	/* The file's UIC */
#define	PROT		5	/* The file's protection code */
#define	ACL		6	/* The file's ACL */
#define END		7	/* The end of the itemlist */
#define	NO_ITEMS        8

static char user_mode = PSL$C_USER;
static int user_modes[2] =
{
        PSL$C_USER | (PSL$C_USER<<2) | (PSL$C_USER<<4) | (PSL$C_USER<<6), 0
};


#define NUM_ACCESS_TYPES 	3
static  int vms_access_types[NUM_ACCESS_TYPES] =
	{ ARM$M_READ, ARM$M_WRITE, ARM$M_EXECUTE };
static  int lo_access_types[NUM_ACCESS_TYPES]  =
	{ LO_P_READ,  LO_P_WRITE,  LO_P_EXECUTE  };

static STATUS
_do_perms(xabp, uperms)
struct XABPRO *xabp;		/* Protection XAB */
i4 *uperms;			/* Returned permissions */
{
	int access_type;
	int i, status;
	ILE3 P_AccLst[NO_ITEMS];	/* Itemlist for SYS$CHKPRO */

	/*
	**	Set up the itemlist.
	*/
	P_AccLst[ACCESS].ile3$w_length = sizeof(int);
	P_AccLst[ACCESS].ile3$w_code = CHP$_ACCESS;
	P_AccLst[ACCESS].ile3$ps_bufaddr = (int *)&access_type;
	P_AccLst[ACCESS].ile3$ps_retlen_addr = NULL;

	P_AccLst[ACMODE].ile3$w_length = sizeof(char);
	P_AccLst[ACMODE].ile3$w_code = CHP$_ACMODE;
	P_AccLst[ACMODE].ile3$ps_bufaddr = (int *)&user_mode;
	P_AccLst[ACMODE].ile3$ps_retlen_addr = NULL;

	P_AccLst[MODE].ile3$w_length = sizeof(char);
	P_AccLst[MODE].ile3$w_code = CHP$_MODE;
	P_AccLst[MODE].ile3$ps_bufaddr = (int *)&user_mode;
	P_AccLst[MODE].ile3$ps_retlen_addr = NULL;

	P_AccLst[MODES].ile3$w_length = 2*sizeof(int);
	P_AccLst[MODES].ile3$w_code = CHP$_MODES;
	P_AccLst[MODES].ile3$ps_bufaddr = (int *)user_modes;
	P_AccLst[MODES].ile3$ps_retlen_addr = NULL;

	P_AccLst[OWNER].ile3$w_length = sizeof(int);
	P_AccLst[OWNER].ile3$w_code = CHP$_OWNER;
	P_AccLst[OWNER].ile3$ps_bufaddr = (int *)&xabp->xab$l_uic;
	P_AccLst[OWNER].ile3$ps_retlen_addr = NULL;

	P_AccLst[PROT].ile3$w_length = sizeof(short);
	P_AccLst[PROT].ile3$w_code = CHP$_PROT;
	P_AccLst[PROT].ile3$ps_bufaddr = (int *)&xabp->xab$w_pro;
	P_AccLst[PROT].ile3$ps_retlen_addr = NULL;

	if (xabp->xab$w_acllen > 0)
	{
	 	P_AccLst[ACL].ile3$w_code = CHP$_ACL;
 	 	P_AccLst[ACL].ile3$w_length = xabp->xab$w_acllen;
	 	P_AccLst[ACL].ile3$ps_bufaddr = (int *)xabp->xab$l_aclbuf;
	 	P_AccLst[ACL].ile3$ps_retlen_addr = NULL;

		P_AccLst[END].ile3$w_code = CHP$_END;
 	 	P_AccLst[END].ile3$w_length = 0;
	}
	else
	{
	 	P_AccLst[ACL].ile3$w_code = CHP$_END;
 	 	P_AccLst[ACL].ile3$w_length = 0;
	}

	/*
	**	Test all the accesses (READ, WRITE, and EXECUTE).  The only
	**	VMS status we're expecting are SS$_NORMAL (access OK) and
	**	SS$_NOPRIV (no access).  We'll return failure otherwise.
	*/
	*uperms = 0;
	for ( i = 0; i < NUM_ACCESS_TYPES; i++ )
	{
		access_type = vms_access_types[i];
		status = sys$chkpro(P_AccLst, 0, 0);
		if (status == SS$_NORMAL)
			*uperms |= lo_access_types[i];
		else if (status != SS$_NOPRIV)
			return FAIL;
	}

 	return OK;
}
