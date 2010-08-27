/*
**	Copyright (c) 2010 Ingres Corporation
**	All rights reserved.
*/
EXEC SQL include sqlca;

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ui.h>
# include	<uigdata.h>
# include	<dictutil.h>

static VOID	err_check();
static VOID	so_clean_up();

/**
** Name:	main.c -	Main Routine
**
** Description:
**	main		The main program.
**
** History:
**	14-apr-2010 (stephenb)
**	    Copied from apdev15u.sc
**/

FUNC_EXTERN bool        IIDDigIsGateway();


/*{
** Name:	main() -	Upgrdae apdev2 to version 2.
**
** Description:
**	Performs upgrade of APPLICATION_DEVELOPMET_2 from version 1 to version 2.
**	APPLICATION_DEVELOPMENT_2 creates OpenROAD catalogs. Upgrade from
**	v1 to v2 involves changing ii_srcobj_encoded.sequence_no from
**	smallint to integer (i2 to i4). Since ALTER TABLE ALTER COLUMN
**	cannot be run on catalogs, this must be done by copying the
**	contents of the catalog into a temp table, dropping the catalog,
**	re-creating it with the new format, copying the temp table into
**	the new catalog, and destroying the temp table.
**
** Side Effects:
**	Updates system catalogs.
**
** History:
**	14-apr-2010 (stephenb)
**	    Copied from apdev15u.sc
**	27-Jul-2010 (frima01) Bug 124137
**	    Add exit if no argument has been passed to avoid
**	    segmentation violation when accessing arguments.
**      02-Aug-2010 (bonro01) Bug 124137
**          Previous change causes compile error on Windows.
**          Move code after variable definitions.
*/

i4
main(argc, argv)
i4	argc;
char	**argv;
{

    char	*largv[10], *chp = NULL;
    int i;
    char	*colon_ptr;
    char	*vnode_ptr;
    char	vnode[DB_MAXNAME+1];
    bool    maybe_star = TRUE;
   

    EXEC SQL begin declare section;
      char     dbname[2*DB_MAXNAME+1]; /* double size since it can contain vnode */
      i4       dbservice;
      char	   iidbdbname[DB_MAXNAME+1];
    EXEC SQL end   declare section;

    if (argc < 2)
	PCexit(FAIL);
 
        
    for (i=0; i< argc; i++)
        {
           largv[i] = argv[i];
        }
        chp = STindex(argv[1], ERx("/"), 0);
        largv[3] = ERx("-u'$ingres'");
        /*
        ** chash01 (08-jum-99) if "/" exists, then we need to know
        **   what follows it, if not STAR, we set chp back to NULL
        */
	if (chp)
	{
	    if ( STbcompare("STAR", 4, chp+1, 4, TRUE) )
            {
		chp = NULL;
                maybe_star = FALSE;
            }
	}

	STcopy ((char *)argv[1], dbname );
	if (((colon_ptr = STindex(dbname, ":", 0)) != NULL) &&
	STindex(colon_ptr + 1, ":", 0) == (colon_ptr + 1))
	{
		/* Having found there is a vnode copy it */
		STlcopy(dbname, vnode, colon_ptr - dbname);
		vnode[colon_ptr-dbname] = '\0';
		/* The dbname as passed is a quoted string. We need
		** to remove the leading quote from the vnode, if there
		** is one.
		*/
		if ((vnode_ptr = STindex(vnode, "'", 1)) != NULL)
		{
			STcopy(vnode_ptr+1, vnode);
		}
		/* vnode saved off.  Remove it from dbname */
	        STcopy (colon_ptr+2, dbname );

		/* The dbname as passed is a quoted string. We need
		** to remove the trailing quote from the dbname, if there
		** is one.
		*/
		if (*(vnode_ptr = (dbname + strlen(dbname) - 1)) == '\'')
		{
		        *vnode_ptr = '\0';
		}
	}
	else
	   vnode[0] = '\0';

	/* IF we got control from upgradedb, the '/star' is not passed
	   therefore, we must connect to the iidbdb and determine if
	   this database is a DDB.                                     */
	    
	if (chp == NULL && maybe_star && (STcompare (argv[1], ERx("iidbdb")) ))
	{
	  exec sql whenever sqlerror call sqlprint;
	  /* If there is a vnode we are trying to upgrade a remote db,
	  ** So connect to the remote DBs iidbdb to see if this is a 
	  ** star database
	  */
	  if (vnode[0] != '\0')
	     STprintf(iidbdbname, "%s::iidbdb", vnode);
	  else
	     STcopy("iidbdb", iidbdbname);
	  exec sql connect :iidbdbname;
	  err_check();
	  exec sql select dbservice into :dbservice from iidatabase where name
	       = :dbname;
          err_check();
	  exec sql disconnect;
	  if (dbservice & 1)  /* 1 = ddb  2 = cdb */
	     chp = argv[1]; 
	}


	if (IIDDdbcDataBaseConnect(argc,&largv) != OK)
	{
		PCexit(FAIL);
	}
	exec sql commit;
	exec sql set autocommit off;
	exec sql set lockmode session where level = table,
		readlock = exclusive;
	iiuicw1_CatWriteOn();

        if (chp != NULL)
	{ 
            exec sql commit;
	    if (IIDDccCdbConnect() != OK)
	    {
	       err_check();
	    }
	}

	/*
	** Alter table ii_srcobj_encoded to modify the datatype of sequence_no to
	** i4.
	*/
	exec sql CREATE TABLE iitmp_ii_srcobj_encoded
                AS SELECT * from ii_srcobj_encoded;
	err_check();
	exec sql DROP TABLE ii_srcobj_encoded;
	err_check();
	exec sql CREATE TABLE ii_srcobj_encoded
	(
		entity_id          integer not null,
		sub_type           smallint not null,
		sequence_no        integer not null,
		text_string        varchar(1790) not null
	);
	err_check();
	exec sql INSERT INTO ii_srcobj_encoded
	SELECT * from iitmp_ii_srcobj_encoded;
	err_check();

        /* Gateway won't support the modify syntax */
        if(IIDDigIsGateway() == FALSE)
        {
	    exec sql MODIFY ii_srcobj_encoded
		    TO BTREE UNIQUE ON entity_id, sub_type, sequence_no;
	    err_check();
        }
	exec sql DROP TABLE iitmp_ii_srcobj_encoded;
	err_check();
	exec sql commit;

        if (chp != NULL)
	{
	    /*  commit changes to the CDB */
	    exec sql commit;
	     
	    /*  disconnect from the CDB  */
	    IIDDcd2CdbDisconnect2();
	     
	    /* we are now re-connected to the DDB session  */
	    exec sql remove table ii_srcobj_encoded; 
	    err_check();
	    exec sql register table ii_srcobj_encoded as link;
	    err_check();
	    exec sql commit;
	}

	so_clean_up(OK);
}

static VOID
err_check()
{
	STATUS stat = FEinqerr();

	if (stat != OK)
	{
		exec sql rollback;
		so_clean_up(FAIL);
	}
}

static VOID
so_clean_up(stat)
STATUS	stat;
{
	exec sql set lockmode session where level = page,
		readlock = shared;
	iiuicw0_CatWriteOff();
	PCexit(stat);
}
