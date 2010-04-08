/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <pc.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <duf.h>

#include    <er.h>
#include    <duerr.h>

#include    <duucatdef.h>

#include    <cs.h>
#include    <lk.h>
#include    <st.h>
#include    <dudbms.h>
#include    <dusdb.h>
#include    <duenv.h>
#include    <duustrings.h>


/**
**
**  Name: DUSUTIL.C -	C routines used by the database utility, sysmod.
**
**  Description:
**        This file contains all the C routines specific to the database
**	utility, sysmod.
**
**          dus_get_args()	 - parse the command line initializing
**				   parameter(mode) vector.
**	    dus_init()		 - initialize variables used by sysmod.
**	    dus_validate_sysmod() - validate the use of the sysmod command.
**
**
**  History:
**      11-nov-86 (ericj)
**          Initial creation.
**      15-may-1989 (teg)
**          Local hdr include now lib hdr include.
**	08-may-90 (pete)
**	    Add support for passing front-end dictionary client names to sysmod.
**	12-jul-1990 (pete)
**	    In dus_init(): remove call to dui_fecats_def() and remove the line:
**		    Dus_allcat_defs[DU_FE_CATS]         = &Dui_21fecat_defs[0];
**	01-dec-1992 (robf)
**	    Update privilege handling, no SUPERUSER or -s flag anymore.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	8-aug-93 (ed)
**	    unnest dbms.h
**	13-nov-93 (rblumer))
**	    changed dus_get_args function to use new du_xusrname field.
**      31-jan-94 (mikem)
**          Added include of <cs.h> before <lk.h>.  <lk.h> now references
**          a CS_SID type.
**      28-may-98 (stial01)
**          Support VPS system catalogs.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      20-may-99 (chash01)
**        12-dec-97 (schang) roll in 6.4 rmsgw change
**          Add logic to handle '/rms' on dbname to signal sysmod'ing of an 
**	    RMS Gateway database.  Note: needed to add #include <duustrings.h>
**	    above to allow for internationalized string "DU_0RMS_GATEWAY" in
**	    borrowed use (from CREATEDB) of RMS-gateway-specific errorcode.
**      07-nov-2001 (stial01)
**          Changed invalid page size message.
**      05-Jan-2004 (nansa02)
**          To fix bug(113662) Added a break statement,to break out of the loop
**          once all the client names are read. 
**/

GLOBALREF char  iiduNoFeClients[]; /* special client name*/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
*/

/*
** Definition of all global variables owned by this file.
*/

GLOBALREF DUU_CATDEF         *Dus_allcat_defs[];
/*
[@global_variable_definition@]...
*/
/*
[@static_variable_or_function_definitions@]
*/


/*{
** Name: dus_get_args()	-   parse the command line initializing modes
**			    for the database utility, sysmod.
**
** Description:
**        This routine parses the command line to set the mode flags
**	for the sysmod database utility.
**
** Inputs:
**      argc                            Command line argument count.
**	argv				Command line argument vector.
**	dus_modes			Ptr to a DUS_MODE_VECT which
**					will describe the various modes
**					which sysmod can operate under.
**	dus_dbenv			Ptr to a DU_ENV struct which describes
**					DBMS environment for a database utility.
**	dus_errcb			Ptr to error-handling control block.
**
** Outputs:
**      *dus_modes
**	    .dus_wait_flag		True, is the "+w" flag was used on the
**					command line.
**	    .dus_dbdb_flag		True, if the "dbdb" is being sysmoded.
**	    .dus_all_catalogs		False, if one or more system catalog
**					names was given on command line.
**	    .dus_???_cat		Here ??? stands for some system catalog
**					name without the prepended "ii".  If the
**					DUS_PARAMETER_CAT bit is set, this
**					catalog was listed as a sysmod parameter.
**          .dus_gw_flag                True, if a gateway database is to be
**                                      sysmoded.
**	    .dus_progname		The name of the program being
**					executed.
**	*dus_dbenv
**	    .du_dbname			Name of the database to be sysmoded.
**	    .du_usrinit			Processes' user name.
**	    .du_xusrname		Name of the effective user.
**	*dus_errcb			If an error occurs, this will be set
**					by a call to du_error().
**
**	Returns:
**	    E_DU_OK			Completed successfully.
**	    E_DU3121_UNKNOWN_FLAG_SY	An unknown flag was passed on the
**					command line.
**	    E_DU3010_BAD_DBNAME		The database name parameter was
**					syntactically incorrect.
**	    E_DU3014_NO_DBNAME		No database name was given on the
**					command line.
**	    E_DU3312_DBDB_ONLY_CAT_SY	A "dbdb" system catalog has been
**					given as a parameter with a database
**					other than the "dbdb".
**	    E_DU5130_GW_INVALID_NAME	The database name parameter was a
**					RMS-gateway designation: "gdbname"/rms
**					but "gdbname" was syntactically
**                                      incorrect.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-nov-86 (ericj)
**          Initial creation.
**	08-may-90 (pete)
**	    Add support for passing front-end dictionary client names to sysmod.
**      11-oct-1990 (pete)
**          Reintroduce the -f flag for client names, to be consistent with
**	    Createdb.
**	08-oct-91 (teresa)
**	    put in support for -f nofeclients (fix bug 40229)
**	21-dec-92 (robf))
**	    -s option is now a noop. Per LRC decision, issue a warning.
**	13-nov-93 (rblumer))
**	    changed to use new du_xusrname field.
**      28-may-98 (stial01)
**          dus_get_args() Support VPS system catalogs.
**      20-may-99 (chash01) code merge
**        12-dec-97 (schang) roll in 6.4 rmsgw change
**          Add logic to handle '/rms' on dbname to signal sysmod'ing of an 
**	    RMS Gateway database.  Note: needed to add #include <duustrings.h>
**	    above to allow for internationalized string "DU_0RMS_GATEWAY" in
**	    borrowed use (from CREATEDB) of RMS-gateway-specific errorcode.
*/

DU_STATUS
dus_get_args(argc, argv, dus_modes, dus_dbenv, dus_errcb)
i4		    argc;
char		    *argv[];
DUS_MODE_VECT	    *dus_modes;
DU_ENV		    *dus_dbenv;
DU_ERROR	    *dus_errcb;
{
    char	    *p;
    char	    *tmp;
    i4		    i;
    DUU_CATDEF	    *catdef_p;
    i4              num_clients = 0;    /* nmbr dict client names on cmd line */
    char            *tail_str;
    bool            client_flag = FALSE; /* true if -f flag given */
    bool	    nofeclients = FALSE; /* true if -f nofeclients is given */

    VOID	    du_get_usrname();
    i4         page_size;
    STATUS          status;

    /* Get the process's user name */
    du_get_usrname(dus_dbenv->du_usrinit);
    if (du_chk2_usrname(dus_dbenv->du_usrinit) != OK)
	return(du_error(dus_errcb, E_DU3000_BAD_USRNAME, 2,
	       0, dus_dbenv->du_usrinit));
    else
	/* Initialize the effective user name to that of the process's
	** user name.
	*/
	STcopy(dus_dbenv->du_usrinit, dus_dbenv->du_xusrname);

    /* The first argument is the program being executed */
    STcopy(argv[0], dus_modes->dus_progname);

    while (--argc)
    {
	p   = *(++argv);
	switch (p[0])
	{
	  case '-':
	    switch (p[1])
	    {
	      case 'p':
		/*
		** This option used to specify -page_size=N
		*/
		if (STncasecmp(&p[1], "page_size=", 10 ) == 0)
		{
		    status = CVal(&p[11], &page_size);
		    if (status == OK &&
			    (page_size == 2048 || page_size == 4096 
			    || page_size == 8192 || page_size == 16384
			    || page_size == 32768 || page_size == 65536))
		    {
			dus_modes->dus_page_size = page_size;
			break;
		    }
		}
		return du_error(dus_errcb, E_DU3303_BAD_PAGESIZE, 2, 0, page_size);

	      case 's':
		/*
		** This option used to mean SUPERUSER. This is now 
		** determined automatically, so is a no-op. Per LRC
		** decision accept the option but issue a warning.
		*/
	        _VOID_ du_error(dus_errcb, W_DU1831_SUPUSR_OBSOLETE, 0);
		break;

	      case 'w':
		/* This is a no-op.  By default, we set this flag to FALSE */
		/* '+w' will set it to TRUE */
		break;

              case 'f':
                client_flag = TRUE;
                tmp = STskipblank(&p[2], (i4) DU_MAX_CLIENT_NAME);
                if ( (tmp != (char *) NULL) && (*tmp != EOS))
                {
                    /* A name was attached to -f flag; tack it onto what we
                    ** already have (if anything). NOTE: multiple -f flags
                    ** are allowed, but not documented (LRC wanted it that
                    ** way!).  However, it remains illegal to use the nofeclient
		    ** name in conjunction with other flags.
                    */
		    if (STcompare(tmp,iiduNoFeClients) ==0)
			nofeclients = TRUE;
		    else
			num_clients++ ;
                    STpolycat (3, dus_modes->dus_client_name,
                                 (num_clients == 0) ? ERx("") : ERx(" "),
                                 tmp,
                                 dus_modes->dus_client_name);
                }

                /* multiple client names can appear following the -f
                ** flag (e.g. -fingres visibuild windows_4gl).
                ** Scan for more client names, or another flag,
                ** or end of arguments.
                */

                /* argc & argv should be incremented together, so don't do
                ** it as part of the conditional on argc (e.g. "if (--argc)").
                */
                for (--argc, p = *(++argv); argc ; --argc, p = *(++argv))
                {
                    if (p[0] == '-' || p[0] == '+')
                    {
                        /* uh oh, another flag. we're done with client list */
                        break;
					}
                    else
                    {

			if (STcompare(p, iiduNoFeClients) ==0)
			{
			    if (num_clients)
			    {
				/* opps, its illegal to have the nofeclients  
				** flag used with other flags.  Generate an 
				** error. */
				return(du_error(dus_errcb, 
					    E_DU3515_CLIENT_NM_CONFLICT, 0));
			    }
			    else
				nofeclients=TRUE;
			}
			else
			{
			    if (nofeclients)
				/* opps - we used nofeclients with other flags*/
				return(du_error(dus_errcb, 
					    E_DU3515_CLIENT_NM_CONFLICT, 0));
			}

			/* argv is a client name */
                        STpolycat (3, dus_modes->dus_client_name,
                                (num_clients == 0) ? ERx("") : ERx(" "),
                                 p,
                                 dus_modes->dus_client_name);
			num_clients++ ;
                    }
                }
		/* if the -f flag is specified with anything other than
		** the client name nofeclients, then the all-clients flag
		** needs to be set to false. */
		if (num_clients &&  !nofeclients)
		    dus_modes->dus_all_catalogs = FALSE;

                /* We're pointing at either last argument in list, which is
                ** a client name, or at a flag that follows -f flag. Either way
                ** we've gone one too far. Back up one argument to let outer
                ** loop handle it.
                */
                argc++;
                argv--;

                break;

	      default:
		return(du_error(dus_errcb, E_DU3300_UNKNOWN_FLAG_SY, 2,
				0, p));
		break;

	    }	/* end of switch (p[1]) */
	    break;

	  case '+':
	    if (p[1] == 'w')
		dus_modes->dus_wait_flag    = TRUE;
	    else
		return(du_error(dus_errcb, E_DU3300_UNKNOWN_FLAG_SY, 2,
				0, p));
	    break;

	  default:
	    /* Must be a database name parameter or a system catalog
	    ** parameter
	    */
	    if (dus_dbenv->du_dbname[0] == '\0')
	    {
		/* It must be a dbname */
		/*
		**   Determine if it's an RMS-gateway dbname ...
		**   Handle RMS/Gateway name = "vnode::dbname/rms"
		*/
		tail_str = STindex(p, ":", STlength(p));
		if (tail_str)
		{
		    /*
		    ** Assume a "vnode::" preceding "dbname/rms".
		    ** skip over (assumed) second one
		    ** and re-define dbname after "vnode::"
		    */
		    ++tail_str;
		    p = ++tail_str;
		}
		tail_str = STindex(p, "/", STlength(p));
		if (tail_str) (VOID) CVlower(tail_str);
		else tail_str = DU_INGTAIL;

		if (!STcompare( tail_str, DU_RMSTAIL))
		{
		    /* 
		    ** YES -- an RMS-Gateway designation:
		    ** Extract "gdbname" from p = "gdbname"/rms,
		    ** and validate it ...
		    */
		    STlcopy( p, 
			     dus_dbenv->du_gdbname, 
			     (STlength(p)-STlength(tail_str))
			    );
		    if (du_chk1_dbname(dus_dbenv->du_gdbname) != OK)
		    {
			/* Stole this from CREATEDB:duc_ginit_dbname() */
			return(du_error(dus_errcb, E_DU5130_GW_INVALID_NAME,
				4, 0, DU_0RMS, 0,
				dus_dbenv->du_gdbname));
		    }
		    dus_modes->dus_gw_flag = DU_GW_RMS_FLAG;
		    STcopy(dus_dbenv->du_gdbname, dus_dbenv->du_dbname);
		}
                else
		{
		    /* Not an RMS-gateway database, resume checking ... */
		    if (du_chk1_dbname(p) != OK)
		        return(du_error(dus_errcb, E_DU3010_BAD_DBNAME, 2, 0, p));
		    else
		        STcopy(p, dus_dbenv->du_dbname);
	        	
		    /* Is it the "dbdb"? */
		    if (!STcompare(dus_dbenv->du_dbname, DU_DBDBNAME))
		        dus_modes->dus_dbdb_flag = TRUE;
                }
	    }
	    else
	    {
		/* It must be a system catalog name */
		dus_modes->dus_all_catalogs = FALSE;

		for (i = 0; i < DU_MAXCAT_DEFS; i++)
		{
		    if (Dus_allcat_defs[i] == NULL)
			/* There's a set of catalog definitions missing. */
			continue;

		    for (catdef_p = Dus_allcat_defs[i];
			 catdef_p->du_relname; 
			 catdef_p++
			)
		    {
			if (!STcompare(catdef_p->du_relname, p))
			{
			    catdef_p->du_requested	= TRUE;
			    break;
			}
		    }

		    if (catdef_p->du_requested)
			break;
		}	/* end of while loop. */


		if (!catdef_p->du_requested)
		{
		    /* Sysmod doesn't know a catalog by this name.
		    ** See if it's the name of a front-end catalog.
		    */
		    if (STncasecmp(p, ERx("ii_"), 3 ) == 0)
		    {
			/*assume front-end catalog. give warning and continue */

                	du_error(dus_errcb, W_DU1032_FECAT_IGNORE, 2, 0, p);
		    }
		    else
		    {
			/* Error: unknown back-end or standard catalog */
		        return(du_error(dus_errcb,
				E_DU3304_BAD_SYSTEM_CATALOG_SY,
				2, 0, p));
		    }
		}
	    }	/* end of else we have a system catalog */
		
	    break;  /* break from the default case */

	}   /* end of switch(p[0]) */
    }	/* end of while loop */

    /* Check if -f flag was given with no clients */
    if (client_flag && (num_clients == 0))
        return(du_error(dus_errcb, E_DU3514_EMPTY_CLIENT_LIST,
                2, (i4 *) NULL, ERx("sysmod")));

    /* Check to see that a database name parameter was given */
    if (dus_dbenv->du_dbname[0] == EOS)
	return(du_error(dus_errcb, E_DU3014_NO_DBNAME, 0));

    /* Check to see if "dbdb" system catalogs were given as parameters
    ** for a database other than the "dbdb".
    */
    if (!dus_modes->dus_dbdb_flag)
    {
	for (i = 0; Dub_31iidbdbcat_defs[i].du_relname != NULL; i++)
	    if (Dub_31iidbdbcat_defs[i].du_requested)
		return(du_error(dus_errcb, E_DU3312_DBDB_ONLY_CAT_SY, 4,
				0, Dub_31iidbdbcat_defs[i].du_relname,
				0, DU_DBDBNAME));

    }

    return(E_DU_OK);
}



/*{
** Name: dus_init() -	initialize variables and structs used by sysmod.
**
** Description:
**        This routine initializes the variables and structs used by 
**	sysmod.
**
** Inputs:
**      dus_modes                       Ptr to a DUS_MODE_VECT struct.
**	dus_dbenv			Ptr to a DU_ENV struct.
**	dus_errcb			Ptr to the error-handling control block.
**
** Outputs:
**      *dus_modes			The initialized DUS_MODE_VECT.
**	*dus_dbenv			The initialized DU_ENV.
**	*dus_errcb			The initialized error-handling control
**					block.
**	Returns:
**	    E_DU_OK			Completed successfully.
**	Exceptions:
**	    none
**
** Side Effects:
**	      Initializes the global variable dus_dbenv to point to the
**	    DUS_MODE_VECT address that was passed in.
**
** History:
**      13-Oct-86 (ericj)
**          Initial creation.
**	15-May-89 (teg)
**	    added calls for distributed catalogs
**	8-may-90 (pete)
**	    Initialize dictionary client info.
**      28-may-98 (stial01)
**          dus_init() Support VPS system catalogs.
**      20-may-99 (chash01) code merge
**          12-dec-97 (schang)
**            RMS-gateway support:  initialize "dus_gw_flag".
**	19-Apr-2007 (bonro01)
**	    Detect internal error in dus_init() and exit with
**	    a bad return code.

[@history_template@]...
*/
DU_STATUS
dus_init(dus_modes, dus_dbenv, dus_errcb)
DUS_MODE_VECT		*dus_modes;
DU_ENV			*dus_dbenv;
DU_ERROR		*dus_errcb;
{
    /* Initialize the system utility environment */
    du_envinit(dus_dbenv);

    /* Initialize the system utility error-handling struct */
    du_reset_err(dus_errcb);
    /* {@fix_me@} */
    /* Ds_errcb	= du_errcb; */

    /* initialize the sysmod modes vector */
    dus_modes->dus_dbdb_flag	    = FALSE;
    dus_modes->dus_wait_flag	    = FALSE;
    dus_modes->dus_all_catalogs	    = TRUE;	/* The default is to sysmod
						** all system catalogs.
						*/
    /* chash01: Gateway support */				
    dus_modes->dus_gw_flag	    = DU_GW_NONE;
    dus_modes->dus_page_size = 0;
    dus_modes->dus_client_name[0]   = EOS;

    Dus_allcat_defs[DU_CORE_CATS]	= &Dub_01corecat_defs[0];
    Dus_allcat_defs[DU_DBMS_CATS]	= &Dub_11dbmscat_defs[0];
    Dus_allcat_defs[DU_IIDBDB_CATS]	= &Dub_31iidbdbcat_defs[0];
    Dus_allcat_defs[DU_DDB_CATS]	= &Dub_41ddbcat_defs[0];
    Dus_allcat_defs[DU_SCI_CATS]	= &Dui_51scicat_defs[0];

    dub_corecats_def();
    if( dub_dbmscats_def() != E_DU_OK )
	return(E_DU_FATAL);
    dub_ddbcats_def();
    dub_iidbdbcats_def();

    return(E_DU_OK);
}



/*{
** Name: dus_validate_sysmod() -  validate the sysmod command.
**
** Description:
**        This routine validates the user's authority to issue
**	the sysmod command with the given parameters.
**
** Inputs:
**      du_dbenv                        Database environment description.
**	    .du_usrstat			The Ingres status of the user issueing
**					the sysmod command.
**	    .du_usrinit			The name of the user issueing the
**					sysmod command.
**	    .du_dba			The dba of the database that is to
**					be sysmoded.
**	du_modes			Description of the parameters used
**					on the sysmod command line.
**	    .du_dbdb_flag		Is the "dbdb" being sysmoded?
**	    .du_all_catalogs		Are all existing catalogs to be sysmoded
**					or only catalogs those specified
**					as parameters as listed below.
**	    .du_xxx_cat			Here xxx stands for some system
**					catalog name without the prepended "ii".
**					If the DUS_PARAMETER_CAT bit is set,
**					the user has requested to sysmod this
**					catalog.  If the DUS_EXISTS_CAT bit
**					is set, this catalog exists in this
**					database.
**	du_errcb			DUF error-handling control block.
**
** Outputs:
**      *du_errcb                       If an error occurs, this block will
**					be set by a call to du_error().
**	Returns:
**	    E_DU_OK			Completed successfully.
**	    E_DU3310_NOT_DBA_SY		The user cannot sysmod a database for
**					which they are not the DBA.
**	    E_DU3314_NOSUCH_CAT_SY	If the user listed a specific catalog
**					on the command line, but the catalog
**					doesn't exist in the database.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-nov-86 (ericj)
**          Initial creation.
**	05-oct-92 (robf)
**	    Added check for OPERATOR privilege
[@history_template@]...
*/

DU_STATUS
dus_validate_sysmod(dus_modes, dus_dbenv, dus_errcb)
DUS_MODE_VECT	    *dus_modes;
DU_ENV		    *dus_dbenv;
DU_ERROR	    *dus_errcb;
{
    i4		i;
    DUU_CATDEF	*catdef_p;


    /* Check if the user is authorized to sysmod this database,
    ** needs to be DBA or have OPERATOR/SECURITY privilege.
    **/
    if (STcompare(dus_dbenv->du_usrinit, dus_dbenv->du_dba) &&
	    (dus_dbenv->du_usrstat & (DU_UOPERATOR|DU_USECURITY)) == 0)
    {
	return(du_error(dus_errcb, E_DU3310_NOT_DBA_SY, 4,
			0, dus_dbenv->du_dba,
			0, dus_dbenv->du_usrinit));
    }

    /* Check if the user specified a valid catalog name that doesn't
    ** exist in this database.  Maybe, the user deleted it to conserve disk
    ** space?
    */

    if (!dus_modes->dus_all_catalogs)
    {
	for (i = 0; i < DU_MAXCAT_DEFS; i++)
	{
	    if (Dus_allcat_defs[i] == NULL)
		/* There's a set of catalog definitions missing. */
		continue;

	    for (catdef_p = Dus_allcat_defs[i];
		 catdef_p->du_relname; 
		 catdef_p++
		)
	    {
		if (catdef_p->du_requested && !catdef_p->du_exists)
		    return(du_error(dus_errcb, E_DU3314_NOSUCH_CAT_SY,
				    4, 0, catdef_p->du_relname,
				    0, dus_dbenv->du_dbname));
	    }
	}	/* end of first for loop. */
    }

    return(E_DU_OK);
}
