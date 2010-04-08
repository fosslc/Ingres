/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <cm.h>
# include <cv.h>
# include <st.h>
# include <si.h>
# include <ex.h>
# include <me.h>
# include <er.h>
# include <ci.h>
# include <pc.h>
# include <gl.h>
# include <iicommon.h>
# include <adf.h>
# include <fe.h>
# include <ug.h>
# include <uigdata.h>
# include <rpu.h>
# include "errm.h"

/**
** Name:	repcfg.sc - configure Replicator
**
** Description:
**	Defines
**		main		- configure Replicator
**
** History:
**	10-dec-98 (abbjo03)
**		Created.
**	29-dec-98 (abbjo03)
**		Add -q option for CreateKeys.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB and SHFRAMELIB
**	    to replace its static libraries. Added NEEDLIBSW hint which is 
**	    used for compiling on windows and compliments NEEDLIBS.
**	26-Aug-2009 (kschendel) 121804
**	    Need rpu.h to satisfy gcc 4.3.
**/

/*
PROGRAM =	repcfg

NEEDLIBS =      REPMGRLIB REPCOMNLIB SHFRAMELIB	SHQLIB SHCOMPATLIB

NEEDLIBSW = 	SHEMBEDLIB SHADFLIB

UNDEFS =	II_copyright
*/

# define DIM(a)		(sizeof(a) / sizeof(a[0]))

# define CFG_OBJ_DB		1
# define CFG_OBJ_CDDS		2
# define CFG_OBJ_TABLE		3

# define CFG_ACT_REGISTER	1
# define CFG_ACT_SUPPORT	2
# define CFG_ACT_ACTIVATE	3
# define CFG_ACT_DEACTIVATE	4
# define CFG_ACT_MOVECFG	5
# define CFG_ACT_CRTKEYS	6

# define FIXED_ARGS		4


GLOBALDEF ADF_CB	*RMadf_cb;


/* types of objects that can be configured */
static	char	*obj_types[] =
{
	ERx("database"),
	ERx("cdds"),
	ERx("table")
};

/* configuration actions */
static	char	*actions[] =
{
	ERx("register"),
	ERx("support"),
	ERx("activate"),
	ERx("deactivate"),
	ERx("moveconfig"),
	ERx("createkeys")
};

static	char	*cfg_db_name = NULL;
static	char	*obj_type_str = NULL;
static	char	*action_str = NULL;
static	char	*user_name = ERx("");
static	bool	queue = FALSE;

static	ARG_DESC args[] =
{
	/* required */
	{ERx("database"), DB_CHR_TYPE, FARG_PROMPT, (PTR)&cfg_db_name},
	{ERx("obj_type"), DB_CHR_TYPE, FARG_PROMPT, (PTR)&obj_type_str},
	{ERx("action"), DB_CHR_TYPE, FARG_PROMPT, (PTR)&action_str},
	/* optional */
	{ERx("user"), DB_CHR_TYPE, FARG_FAIL, (PTR)&user_name},
	{ERx("queue"), DB_BOO_TYPE, FARG_FAIL, (PTR)&queue},
	NULL
};


FUNC_EXTERN STATUS FEhandler(EX_ARGS *ex);
FUNC_EXTERN ADF_CB *FEadfcb(void);
FUNC_EXTERN STATUS cdds_activate(i2 cdds_no, bool activate);
FUNC_EXTERN STATUS create_replication_keys(i4 table_no, bool queue_flag);


/*{
** Name:	main	- configure Replicator
**
** Description:
**	Invokes Replicator configuration commands.
**
** Inputs:
**	argc - the number of arguments on the command line
**	argv - the command line arguments
**		argv[1]		- the database to connect to
**		argv[2]		- type of object to be configured
**			database
**			cdds
**			table
**		argv[3]		- configuration action
**			register	(for table only)
**			support		(for table only)
**			activate
**			moveconfig	(for database only)
**			createkeys	(for table only)
**		argv[4] ...	- object name(s)
**
** Outputs:
**	none
**
** Returns:
**	OK on success
*/
i4
main(
i4	argc,
char	*argv[])
{
	i4		i;
	i4		obj_type_len;
	i4		action_len;
	i4		obj_type;
	i4		action;
	STATUS		status;
	ARGRET		arg;
	i4		pos;
	i4		num_objs;
	char		**objs;
	i4		obj_num;
	EX_CONTEXT	context;
	u_i4		num_args;
	i4		fmode;

	/* Tell EX this is an INGRES tool */
	(void)EXsetclient(EX_INGRES_TOOL);

	/* Use the INGRES memory allocator */
	(void)MEadvise(ME_INGRES_ALLOC);

	/* Initialize character set attribute table */
	if (IIUGinit() != OK)
		PCexit(FAIL);

	/* Parse the command line arguments */
	if (IIUGuapParse(argc, argv, ERx("repcfg"), args) != OK)
		PCexit(FAIL);

	obj_type = 0;
	obj_type_len = STlength(obj_type_str);
	for (i = 0; i < DIM(obj_types); ++i)
	{
		if (STbcompare(obj_type_str, obj_type_len, obj_types[i],
			STlength(obj_types[i]), TRUE) == 0)
		{
			obj_type = i + 1;
			break;
		}
	}
	if (!obj_type)
	{
		SIprintf("Invalid object type: %s\n", obj_type_str);
		PCexit(FAIL);
	}
	action = 0;
	action_len = STlength(action_str);
	for (i = 0; i < DIM(actions); ++i)
	{
		if (STbcompare(action_str, action_len, actions[i],
			STlength(actions[i]), TRUE) == 0)
		{
			action = i + 1;
			break;
		}
	}
	if (!action)
	{
		SIprintf("Invalid action: %s\n", action_str);
		PCexit(FAIL);
	}
	switch (action)
	{
	case CFG_ACT_REGISTER:
	case CFG_ACT_SUPPORT:
	case CFG_ACT_CRTKEYS:
		if (obj_type != CFG_OBJ_TABLE)
		{
			SIprintf("Invalid action '%s' for object type '%s'\n",
				action_str, obj_type_str);
			PCexit(FAIL);
		}
		break;

	case CFG_ACT_MOVECFG:
		if (obj_type != CFG_OBJ_DB)
		{
			SIprintf("Invalid action '%s' for object type '%s'\n",
				action_str, obj_type_str);
			PCexit(FAIL);
		}
		break;
	
	case CFG_ACT_ACTIVATE:
	case CFG_ACT_DEACTIVATE:
		break;
	}

	status = FEutaopen(argc, argv, ERx("repcfg"));
	if (status != OK)
		PCexit(status);
	if (argc > FIXED_ARGS)
	{
		num_args = argc - FIXED_ARGS;
		fmode = FARG_FAIL;
	}
	else
	{
		num_args = 1;
		fmode = FARG_OPROMPT;
	}
	objs = (char **)MEreqmem(0, sizeof(char *) * num_args, FALSE, NULL);
	if (!objs)
		PCexit(FAIL);
	num_objs = 0;
	while (FEutaget(ERx("object"), num_objs, fmode, &arg, &pos) == OK)
	{
		if (*arg.dat.name == EOS)
			break;
		objs[num_objs] = arg.dat.name;
		++num_objs;
	}
	FEutaclose();
	if (!num_objs)
		PCexit(FAIL);

	/* Set up the generic exception handler */
	if (EXdeclare(FEhandler, &context) != OK)
	{
		EXdelete();
		PCexit(FAIL);
	}

	RMadf_cb = FEadfcb();

	/* Open the database */
	if (FEningres(NULL, (i4)0, cfg_db_name, user_name, NULL) != OK)
	{
		EXdelete();
		PCexit(FAIL);
	}
	EXEC SQL SET AUTOCOMMIT OFF;
	EXEC SQL SET_SQL (ERRORTYPE = 'genericerror');

	if (!IIuiIsDBA)
	{
		EXdelete();
		IIUGerr(E_RM0002_Must_be_DBA, UG_ERR_FATAL, 0);
	}

	if (!RMcheck_cat_level())
	{
		EXdelete();
		IIUGerr(E_RM0003_No_rep_catalogs, UG_ERR_FATAL, 0);
	}

	status = OK;
	for (i = 0; i < num_objs; ++i)
	{
		if (CMdigit(objs[i]))
			CVal(objs[i], &obj_num);
		switch (obj_type)
		{
		case CFG_OBJ_DB:
			switch (action)
			{
			case CFG_ACT_ACTIVATE:
				break;

			case CFG_ACT_DEACTIVATE:
				break;

			case CFG_ACT_MOVECFG:
				break;
			}
			break;

		case CFG_OBJ_CDDS:
			switch (action)
			{
			case CFG_ACT_ACTIVATE:
				status = cdds_activate((i2)obj_num, TRUE);
				break;

			case CFG_ACT_DEACTIVATE:
				status = cdds_activate((i2)obj_num, FALSE);
				break;
			}
			break;

		case CFG_OBJ_TABLE:
			switch (action)
			{
			case CFG_ACT_REGISTER:
				break;

			case CFG_ACT_SUPPORT:
				break;

			case CFG_ACT_ACTIVATE:
				break;

			case CFG_ACT_DEACTIVATE:
				break;

			case CFG_ACT_CRTKEYS:
				status = create_replication_keys((i4)obj_num,
					queue);
			}
		}
		if (status != OK)
			break;
	}

	FEing_exit();
	EXdelete();
	PCexit(status);
}
