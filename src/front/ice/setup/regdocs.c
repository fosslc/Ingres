/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: regdocs.c
**
** Description:
**      Register a list of files into the ice repository either from a
**      command line list of files or from an input file.
**
** History:
**      21-Feb-2001 (fanra01)
**          Sir 103813
**          Created.
**      04-May-2001 (fanra01)
**          Sir 103813
**          Update call to ic_initialize to take additional parameter.
**	09-May-2001 (hanje04)
**	    Added SETUPLIB to NEEDLIBS to build on UNIX.
*/
/*
PROGRAM=        regdocs

DEST=		bin

NEEDLIBS=	ABFRTLIB COMPATLIB C_APILIB DDFLIB ICECLILIB UGLIB SETUPLIB
*/
# include <stdio.h>
# include <iiapi.h>
# include <iceconf.h>

# include "iceutil.h"

typedef struct _tag_params
{
    II_CHAR*    name;
    II_CHAR*    node;
    II_CHAR*    owner;
    II_CHAR*    unit;
    II_CHAR*    loc;
    II_CHAR*    inpfile;
    II_CHAR*    xmlfile;
    II_CHAR**   args;
    II_INT4     nargs;
    II_INT4     type;
    II_INT4     flags;
    II_INT4     buid;
    II_INT4     locid;
}PARAMS;

PARAMS  progparms = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0,
IA_PAGE, IA_EXTERNAL, 0, 0 };
II_CHAR* node = NULL;
II_CHAR  user[80] = "";
II_CHAR  passwd[80]= "";

/*
** Name: dumpparms
**
** Description:
**      Function displays the parameters as gathered from the command line.
**
** Inputs:
**      p               Pointer to structure containing command line
**                      parameter values.
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
II_VOID
dumpparms( PARAMS* p )
{
    II_INT4 i;
    SIprintf("Node:   %s\n", (p->node) ? p->node : "Local");
    SIprintf("Owner:  %s\n", (p->owner) ? p->owner : "Not specified");
    SIprintf("Unit:   %s\n", (p->unit) ? p->unit : "Not specified");
    SIprintf("Loc:    %s\n", (p->loc) ? p->loc : "Not specified");
    SIprintf("Input:  %s\n", (p->inpfile) ? p->inpfile : "Not specified");
    SIprintf("Type:   %08x\n", p->type);
    SIprintf("Flag:   %08x\n", p->flags);

    for(i = 0; (i < p->nargs) && (p->args[i] != NULL); i+=1)
    {
        SIprintf("%03d %s\n", i, p->args[i]);
    }
}

/*
** Name: registerdoc
**
** Description:
**      Function sets up the parameters required to complete the command.
**      Parameters are either global as set on the command line or overriden
**      from the input file.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_UNIT_NAME      Specified unit does not exist.
**      E_IC_INVALID_LOC_NAME       Specified location does not exist.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
registerdoc( HICECTX icectx, PARAMS* prog, PREGPARAMS rp )
{
    ICE_STATUS  status = OK;
    II_CHAR*    owner;
    II_CHAR*    unitname;
    II_CHAR*    location;
    II_CHAR*    filename;
    II_CHAR*    bu;
    II_CHAR*    loc;
    II_CHAR*    file;
    II_CHAR*    ext;
    II_INT4     buid;
    II_INT4     locid;
    II_INT4     docid;
    II_INT4     type;
    II_INT4     flags;

    /*
    ** if the input file didn't explicitly define an option use the
    ** one from the command line.
    */
    owner    = (rp->owner != NULL) ? rp->owner : prog->owner;
    unitname = (rp->unit != NULL) ? rp->unit : prog->unit;
    location = (rp->location != NULL) ? rp->location : prog->loc;
    filename = rp->filename;
    type     = (rp->type != 0) ? rp->type : prog->type;
    flags    = (rp->flags != 0) ? rp->flags : prog->flags;

    /*
    ** Setup the business unit and location id.
    */
    buid     = prog->buid;
    locid    = prog->locid;

    /*
    ** if the unit id has not previously been set or the unit name is
    ** specified in the file get the id of the specified unit.
    */
    if ((buid == 0) || (rp->unit != NULL))
    {
        if ((status = ic_getitem( icectx, ICE_UNIT, "unit_name", unitname,
        "unit_id", &bu )) == OK)
        if (bu != NULL)
        {
            ASCTOI( bu, &buid );
        }
        else
        {
            if (status == E_IC_OUT_OF_DATA)
            {
                status = E_IC_INVALID_UNIT_NAME;
            }
        }
    }
    /*
    ** if the location id has not previously ben set or the location name is
    ** specified in the get the id of the specified location.
    */
    if ((status == OK) &&
        ((locid == 0) || (rp->location != NULL)))
    {
        status = ic_getitem( icectx, ICE_LOCATION, "loc_name", location,
            "loc_id", &loc );
        if((status == OK) && (loc != NULL))
        {
            ASCTOI( loc, &locid );
        }
        else
        {
            if (status == E_IC_OUT_OF_DATA)
            {
                status = E_IC_INVALID_LOC_NAME;
            }
        }
    }
    if ((status = ic_getfileparts( filename, &file, &ext )) == OK)
    {
        if ((status = ic_createdoc( icectx, buid, file, (ext) ? ext : "",
            type, locid, flags, filename, owner, &docid )) == OK)
        {
            SIprintf("%s: Unit:%s Location:%s %03d %s.%s registered\n",
                prog->name, unitname, location, docid, file, ext);
        }
        else
        {
            II_CHAR* errmsg;
            ic_errormsg( icectx, status, &errmsg );
            SIprintf( "%s: Error 0x%08x - %s.%s %s\n",
                prog->name,
                status,
                file, ext,
                (errmsg != NULL) ? errmsg : "\0");
        }
    }
    if (status != OK)
    {
        IIUGerr( status, UG_ERR_ERROR, 0, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL );
    }
    return(status);
}

/*
** Name: regdocs
**
** Description:
**      Function drives the registration loop according to whether the
**      parameters are from an input file or the command line.
**
** Inputs:
**      prog            Program level global parameters.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_UNIT_NAME      Specified unit does not exist.
**      E_IC_INVALID_LOC_NAME       Specified location does not exist.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
**      04-May-2001 (fanra01)
**          Add NULL cookie session parameter to function call. NULL denotes
**          exclusive session.
*/
ICE_STATUS
regdocs( PARAMS* prog )
{
    ICE_STATUS  status = OK;
    LOCATION    inloc;
    II_CHAR     pathstr[MAX_LOC + 1];
    REGPARAMS   regparams;
    II_CHAR     user[80] = "";
    II_CHAR     passwd[80]= "";
    HICECTX     icectx = NULL;
    II_CHAR*    bu;
    II_CHAR*    loc;

    /*
    ** Get user an password information in preparation to connect to the
    ** ice server
    */
    IIUGprompt("ICE Admin User?", 1, 0, user, sizeof(user), 0);
    IIUGprompt("Password?", 1, 1, passwd, sizeof(passwd), 0);

    /*
    ** if the connection to ice server is successful get business unit id
    ** and location id from their names.
    */
    if ((status = ic_initialize( prog->node, user, passwd, NULL, &icectx )) == OK)
    {
        if ((prog->unit != NULL) &&
            ((status = ic_getitem( icectx, ICE_UNIT, "unit_name", prog->unit,
            "unit_id", &bu )) == OK))
        {
            if (bu != NULL)
            {
                ASCTOI( bu, &prog->buid );
            }
        }
        else
        {
            if (status == E_IC_OUT_OF_DATA)
            {
                status = E_IC_INVALID_UNIT_NAME;
            }
        }
        if ((status == OK) && (prog->loc != NULL))
        {
            status = ic_getitem( icectx, ICE_LOCATION, "loc_name",
                prog->loc, "loc_id", &loc );
            if((status == OK) && (loc != NULL))
            {
                ASCTOI( loc, &prog->locid );
            }
            else
            {
                if (status == E_IC_OUT_OF_DATA)
                {
                    status = E_IC_INVALID_LOC_NAME;
                }
            }
        }
        if (prog->inpfile)
        {
            /*
            ** Options contain an input file. Open file and scan.
            **
            ** -o owner -u unit -l location -t doctype -f flags filename.ext
            */
            FILE* inpfile = NULL;

            if (*prog->inpfile == '-')
            {
                inpfile = stdin;
            }
            else
            {
                STcopy( prog->inpfile, pathstr );
                if ((status = LOfroms( (PATH&FILENAME), pathstr, &inloc )) == OK)
                {
                    status = SIopen( &inloc, "r", &inpfile );
                }
            }
            if (inpfile != NULL)
            {
                while ((status == OK) &&
                    (readinpfile( inpfile, &regparams ) == OK))
                {
                    status = registerdoc( icectx, prog, &regparams );
                    if(regparams.owner)
                    {
                        MEMFREE( regparams.owner );
                    }
                    if(regparams.unit)
                    {
                        MEMFREE( regparams.unit );
                    }
                    if(regparams.location)
                    {
                        MEMFREE( regparams.location );
                    }
                    if(regparams.filename)
                    {
                        MEMFREE( regparams.filename );
                    }
                }
                if (inpfile != stdin)
                {
                    SIclose( inpfile );
                }
            }
        }

        if (prog->xmlfile)
        {
            /*
            ** Options contain an XML input file. Open and scan.
            **
            ** <unit name="unit_name" owner="owner_name">
            ** <location name="location_name">
            ** <page name="filename" ext="ext" type="external|global|repository"/>
            ** <facet name="filename" ext="ext" type="external|global|repository"/>
            ** </location>
            ** </unit>
            */
            FILE* xmlfile;

            STcopy( prog->xmlfile, pathstr );
            if ((status = LOfroms( (PATH&FILENAME), pathstr, &inloc )) == OK)
            {
                if ((status = SIopen( &inloc, "r", &xmlfile )) == OK)
                {
                    /*
                    ** No action until XML parser included.
                    ** Just close the file for now.
                    */
                    SIclose( xmlfile );
                }
            }
        }

        if(prog->args != NULL)
        {
            /*
            ** Command line arguments contains a list of filenames.
            */
            II_INT4  i;
            MEMFILL( sizeof(REGPARAMS), 0, &regparams );
            for (i=0; (status == OK) && (i < prog->nargs); i+=1)
            {
                regparams.filename = prog->args[i];
                status = registerdoc( icectx, prog, &regparams );
            }
        }

        /*
        ** Disconnect from ice server.
        */
        status = ic_close( icectx );
        icectx = NULL;
        ic_terminate();
    }
    else
    {
        IIUGerr( status, UG_ERR_ERROR, 0, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL );
    }
    return(status);
}

/*
** Name: main
**
** Description:
**      Program entry point handles processing of command line arguments.
**
** Inputs:
**      argc            Count of number of parameters
**      argv            List of parameters.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      !OK                         Failed.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
int
main(int argc, II_CHAR** argv)
{
    ICE_STATUS  status = OK;
    II_INT4      i;
    II_INT4      opt = 1;
    II_CHAR*    option;
    II_CHAR*    value;

    if (argc == 1)
    {
        IIUGerr( E_WU001A_REGDOCS_USAGE, UG_ERR_ERROR, 0, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
        return(1);
    }

    progparms.name = argv[0];       /* setup for use in messages */

    for(i=1; (status == OK) && (i < argc); i+=1)
    {
        if (opt)
        {
            if (*argv[i] == '-')
            {
                option = argv[i];
                option += 1;
                value = option;
                value += 1;
                if (*value == 0)
                {
                    i+=1;
                    if (i < argc)
                    {
                        value = argv[i];
                    }
                }
                else
                {
                    while((*value==' ') || (*value=='\t'))
                    {
                        value += 1;
                    }
                }
                switch(*option)
                {
                    case 'n':
                        progparms.node = value;
                        break;
                    case 'o':
                        progparms.owner = value;
                        break;
                    case 'u':
                        progparms.unit = value;
                        break;
                    case 'l':
                        progparms.loc = value;
                        break;
                    case 'i':
                        progparms.inpfile = value;
                        break;
                    case 'x':
                        progparms.xmlfile = value;
                        break;
                    case 't':
                        for( ;*value; value+=1)
                        {
                            switch(*value)
                            {
                                case 'p':
                                    progparms.type = IA_PAGE;
                                    break;
                                case 'f':
                                    progparms.type = IA_FACET;
                                    break;

                                default:
                                    SIprintf("Ignored: Unknown type %c\n", *value);
                                    break;
                            }
                        }
                        break;
                    case 'f':
                        for( ;*value; value+=1)
                        {
                            switch(*value)
                            {
                                case 'e':
                                    progparms.flags |= IA_EXTERNAL;
                                    progparms.flags &= ~(IA_PRE_CACHE | IA_FIX_CACHE | IA_SESS_CACHE);
                                    break;
                                case 'r':
                                    progparms.flags &= ~IA_EXTERNAL;
                                    break;
                                case 'g':
                                    progparms.flags |= IA_PUBLIC;
                                    break;
                                case 'p':
                                    if ((progparms.flags & IA_EXTERNAL) == 0)
                                    {
                                        progparms.flags |= IA_PRE_CACHE;
                                    }
                                    break;
                                case 'f':
                                    if ((progparms.flags & IA_EXTERNAL) == 0)
                                    {
                                        progparms.flags |= IA_FIX_CACHE;
                                    }
                                    break;
                                case 's':
                                    if ((progparms.flags & IA_EXTERNAL) == 0)
                                    {
                                        progparms.flags |= IA_SESS_CACHE;
                                    }
                                    break;

                                default:
                                    SIprintf("Ignored: Unknown flags %c\n", *value);
                                    break;
                            }
                        }
                        break;
                    default:
                        SIprintf("Ignored unknown option -%c\n", *option);
                        break;
                }
            }
            else
            {
                opt = 0;
                break;
            }
        }
    }
    /*
    ** if there is a difference between the counter and the end there must
    ** be a list of filenames.
    */
    if (i < argc)
    {
        /*
        ** Setup a pointer to the list starting with the first filename.
        ** Save the number of filenames in the list.
        */
        progparms.args = &argv[i];
        progparms.nargs = argc - i;
    }
    dumpparms( &progparms );
    status = regdocs( &progparms );
    return(status);
}
