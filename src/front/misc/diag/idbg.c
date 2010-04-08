#include <compat.h>
#include <cv.h>
#include <id.h>
#include <st.h>
#include <lo.h>
#include <si.h>
#include <tm.h>
#include <pc.h>
#include <er.h>
#include <evset.h>
#include <gc.h>
#include <gl.h>
#include <iicommon.h>
#include <mu.h>
#include <pm.h>
#include <gca.h>


/*
**  Copyright (c) 2004 Ingres Corporation
**  Name: idbg - Evidence set management interface
**
**  Description:
**      This module provides the code for the evidence set management interface
**
**      The following options are supported:-
**
**      -list                   List details of all evidence sets
**      -list <evset>           List details of evidence set <evset>
**      -delete <evset>         Delete evidence set <evset>
**      -export=<file> <evset>  Export evidence set <evset> to <file>
**      -add=<file> <evset>     Add a file to an evidence set
**
**  History:
**     22-feb-1996 -- ported to OpenIngres
**     01-Apr-1996 -- (prida01)
**	   		Add sunOS defines for port
**     19-May-1998 -- (horda03)
**          Ported for VMS.
**     17-Jun-1998 (horda03)
**          Allow any user with MONITOR privilege to use the
**          utility.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**     28-Jun-2002 (thaju02)
**	    IDname() was revised to determine whether Ingres is
**	    part of a TNG installation, which was accomplished by
**	    calls to PMinit() and PMload(). If PMinit() is called
**	    once again, do not report PM_DUP_INIT. (B108079)
**    25-Oct-2005 (hanje04)
**        Add prototype for copy_file() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**    25-Jan-2009 (horda03) Bug 121811
**        Port to Windows.
*/

/*
NEEDLIBS =      DIAGLIB UFLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
                UILIB SQLCALIB LIBQLIB UGLIB FMTLIB \
                AFELIB LIBQGCALIB CUFLIB GCFLIB ADFLIB ULFLIB \
		COMPATLIB 
*/

/*
** Forward definitions
*/
STATUS chk_priv( char *user_name, char *priv_name );
void main_frame();

static VOID do_list();
VOID static error();
static copy_file();

main(argc,argv)
i4  argc;
char **argv;
{
    i4   c;
    i4   evset;
    STATUS status=OK;
    char buffer[EVSET_MAX_PATH+1];
    char name[32];
    PTR  name_ptr=name;

    /* Check for running from ingres */

    IDname(&name_ptr);

    status = PMinit();
    if ( (status != OK) && (status != PM_DUP_INIT) )
       error ( status, NULL, 0, NULL);

    if ( (status = PMload( (LOCATION *) NULL, (PM_ERR_FUNC *) NULL)) != OK)
       error( status, NULL, 0, NULL );

    PMsetDefault( 0, ERx( "ii" ) );
    PMsetDefault( 1, PMhost() );

    if ( (status = chk_priv( name, GCA_PRIV_MONITOR )) != OK)
       error( status, NULL, 0, NULL );

    /* Check for forms mode use */
    if(argc==1)
    {
        main_frame();
        PCexit(OK);
    }

    /* Process the flags non forms use */
    for(c=1;c<argc;)
    {
        if(STcompare(argv[c],"-list")==0)
        {
            if(argv[++c] && argv[c][0]!='-')
            {
                if(CVal(argv[c++],&evset) || evset < 0)
                {
                    /* Evidence set does not exist */
                    error(E_UNKNOWN_EVSET,NULL,
                       STlength(argv[c-1]),(PTR)argv[c-1]);
                }
                else
                {
                    do_list(evset);
                }
            }
            else
            {
                do_list(-1);
            }
        }
        else if(STcompare(argv[c],"-delete")==0)
        {
            if(argv[++c] && argv[c][0]!='-')
            {
                if(CVal(argv[c++],&evset) || evset < 0)
                {
                    /* Evidence set does not exist */

                    error(E_UNKNOWN_EVSET,NULL,
                       STlength(argv[c-1]),(PTR)argv[c-1]);
                }
                else
                {
                    /* Delete the evidence set */
                    status=EVSetDelete(evset);

                    if(status==E_UNKNOWN_EVSET)
                    {
                        error(E_UNKNOWN_EVSET,NULL,
                          STlength(argv[c-1]),(PTR)argv[c-1]);
                    }
                    else if(status!=OK)
                    {
                        error(status,NULL,0,NULL);
                    }
                }
            }
            else
            {
                /* No evidence set supplied */
                error(E_NO_EVSET,NULL,6,(PTR)"delete");
            }
        }
        else if (STbcompare(argv[c],8,"-export=",8,FALSE)==0)
        {
            char *filename=&argv[c][8];
            if(argv[++c] && argv[c][0]!='-')
            {
                if(CVal(argv[c++],&evset) || evset < 0)
                {
                    /* Evidence set does not exist */

                    error(E_UNKNOWN_EVSET,NULL,
                       STlength(argv[c-1]),(PTR)argv[c-1]);
                }
                else
                {
                    /* Do the export */

                    status=EVSetExport(evset,filename);

                    if(status==E_UNKNOWN_EVSET)
                    {
                        error(E_UNKNOWN_EVSET,NULL,
                           STlength(argv[c-1]),(PTR)argv[c-1]);
                    }
                    else if(status==E_EXPORT_CREATE)
                    {
                        error(E_EXPORT_CREATE,NULL,STlength(filename),
                           (PTR)filename);
                    }
                    else if(status!=OK)
                    {
                        error(status,NULL,0,NULL);
                    }
                }
            }
	}
        else if (STbcompare(argv[c],8,"-import=",8,FALSE)==0)
        {
            char *filename=&argv[c][8];
            if(argv[++c] && argv[c][0]!='-')
            {
                if(CVal(argv[c++],&evset) || evset < 0)
                {
                    /* Evidence set does not exist */

                    error(E_UNKNOWN_EVSET,NULL,
                       STlength(argv[c-1]),(PTR)argv[c-1]);
                }
                else
                {
                    /* Do the Import */
                    status=EVSetImport(filename,&evset);

                    if(status==E_UNKNOWN_EVSET)
                    {
                        error(E_UNKNOWN_EVSET,NULL,
                           STlength(argv[c-1]),(PTR)argv[c-1]);
                    }
                    else if(status==E_UNKNOWN_EVSET)
                    {
                        error(E_EXPORT_CREATE,NULL,STlength(filename),
                           (PTR)filename);
                    }
                    else if(status!=OK)
                    {
                        error(status,NULL,0,NULL);
                    }
                }
            }
            else
            {
                /* No evidence set supplied */
                error(E_UNKNOWN_EVSET,NULL,6,(PTR)"export");
            }
        }
        else if(((i4)STlength(argv[c]) >= 5) &&
                STbcompare(argv[c],5,"-add=",5,FALSE)==0)
        {
            PTR filename=&argv[c][5];
            if(argv[++c] && argv[c][0]!='-')
            {
                if(CVal(argv[c++],&evset) || evset < 0)
                {
                    /* Evidence set does not exist */
                    error(E_UNKNOWN_EVSET,NULL,
                       STlength(argv[c-1]),(PTR)argv[c-1]);
                }
                else
                {
                   /* Take a copy of the file */
                   status=EVSetCreateFile(evset,EVSET_TEXT,filename,buffer,
                      sizeof(buffer));

                   if(status==E_UNKNOWN_EVSET)
                   {
                      error(E_UNKNOWN_EVSET,NULL,6,(PTR)"add");
                   }
                   else if(status!=OK)
                   {
                      error(status,NULL,0,NULL);
                   }
                   else
                   {
                      copy_file(filename,buffer);
                   }
                }
            }
            else
            {
                /* No evidence set supplied */
                error(E_UNKNOWN_EVSET,NULL,6,(PTR)"export");
            }
        }
        else
        {
            /* Unknown flag */
            error(E_UNKNOWN_FLAG,NULL,STlength(argv[c]),(PTR)argv[c]);
        }
    }
    PCexit(OK);
}

/*{
**  Name: do_list - Internal routine to produce -list output
**
**  Description:
**      This routine produces the -list output
**
**  Inputs:
**      i4  evset         Evidence set to display (or -1 for all)
**
**  Side Effects:
**      Reports errors via error() which terminates the program
**
**  History:
*/

static VOID
do_list(id)
i4  id;
{
    EVSET_DETAILS details;
    STATUS status;
    i4  evset=id;

    while((status=EVSetList(&evset,&details))==0)
    {
        char buffer[40];
        SYSTIME temp;
        EVSET_ENTRY fdetails;
        i4  file=0;

        temp.TM_secs=details.created;
        TMstr(&temp,buffer);
        
        SIprintf("%04d %s %s\n",details.id,buffer,details.description);

        while((status=EVSetFileList(details.id,&file,&fdetails))==0)
        {
            SIprintf("\t%-6s %s\n",fdetails.flags&EVSET_TEXT?"TEXT":"BINARY",
               fdetails.description);
        }

        if (status != E_EVSET_NOMORE)
        {
            error(status,NULL,0,NULL);
        }

        if (id != -1)
            break;
    }

    if (status == E_UNKNOWN_EVSET)
    {
        char temp[20];

        CVla(id,temp);
        error(E_UNKNOWN_EVSET,NULL,STlength(temp),(PTR)temp);
    }
    else if(status != OK && status != E_EVSET_NOMORE)
    {
        error(status,NULL,0,NULL);
    }
}

/*{
**  Name: copy_file - Copy a file
**
**  Description:
**     This internal routine is used to copy a file to an evidence set
**
**  Inputs:
**     PTR from        File to copy from
**     PTR to          File to copy to (in evidence set)
**
**  Exceptions:
**     Calls the error() routine on errors
**
**  History:
*/

static
copy_file(from,to)
PTR from;
PTR to;
{
    LOCATION 	from_loc;
    LOCATION 	to_loc;
    FILE 	*fdes;

    if(LOfroms(PATH&FILENAME,from,&from_loc)!=OK ||
       LOfroms(PATH&FILENAME,to,&to_loc)!=OK ||
       SIopen(&to_loc,"w",&fdes)!=OK ||
       SIcat(&from_loc,fdes)!=OK)
    {
        error(E_COPY_FAIL,NULL,STlength(from),(PTR)from);
    }

    SIclose(fdes);
}

/*
**  Name: error - Report an error
**
**  Description:
**     This routine reports an error to stderr and terminates the
**     program
**
**  Inputs:
**     i4 msg_id  - The id to use in ERlookup call
**     CL_ERR_DESC err - Any reported CL error.
**     i4  len         - Length of parameter insert
**     PTR param       - Parameter insert for message
**
*/
VOID static
error(msg_id,err,len,param)
STATUS 	msg_id;
CL_ERR_DESC *err;
i4  	len;
PTR 	param;
{
	STATUS status=OK;
	i4 lang_code;
	CL_ERR_DESC err_desc;
        ER_ARGUMENT er_arg;
	char msg_buf[ ER_MAX_LEN];
	i4 msg_len;
	i4 length=0;

        /* Package up parameter insert */

        er_arg.er_size = len;
        er_arg.er_value = param;

	ERlangcode( ( char*) NULL, &lang_code);

	status = ERlookup( msg_id, CLERROR(msg_id) ? err : (CL_ERR_DESC *) 0,
		        ER_TEXTONLY, NULL, &msg_buf[length], 
                        ER_MAX_LEN,lang_code, &msg_len, &err_desc, 1, &er_arg);
				
	if ( status)
	{
	    CL_ERR_DESC    junk;

	    STprintf(&msg_buf[length],
			"ERlookup failed to look up message %x ", msg_id);
	    length = STlength(msg_buf);

	    STprintf(&msg_buf[length], "(reason: ER error %x)\n",status);
	    length = STlength(msg_buf);
	    status = ERlookup( (i4) status,
				&err_desc,
				(i4) 0,
				NULL,
				&msg_buf[length],
				(i4) (sizeof(msg_buf) - length),
				(i4) lang_code,
				&msg_len,
				&junk,
				0,
				(ER_ARGUMENT *) NULL
			     );
	    if (status)
	    {
		STprintf(&msg_buf[length],
		    "... ERlookup failed twice:  status = %x", status);
		length = STlength(msg_buf);
	    }
	    else
		length += msg_len;
	}
	else
	    length += msg_len;

	/* Get system message text. */

	if ( err)
	{
	    msg_buf[length++] = '\n';

	    status = ERlookup(	(i4) 0,
				err,
				(i4) 0,
				NULL,
				&msg_buf[length],
				(i4) (sizeof(msg_buf) - length),
				(i4) lang_code,
				&msg_len,
				&err_desc,
				0,
				(ER_ARGUMENT *) NULL
			      );
	    if (status)
	    {
	        CL_ERR_DESC    junk;

		STprintf(&msg_buf[length],
			"ERlookup failed to look up system error ");
		length = STlength(msg_buf);

	        STprintf(&msg_buf[length],
		    "(reason: ER error %x)\n", status);
	        length = STlength(msg_buf);
	        status = ERlookup( (i4) status,
				    &err_desc,
				    (i4) 0,
				    NULL,
				    &msg_buf[length],
				    (i4) (sizeof(msg_buf) - length),
				    (i4) lang_code,
				    &msg_len,
				    &junk,
				    0,
				    (ER_ARGUMENT *) NULL
			         );
	        if (status != OK)
	        {
		    STprintf(&msg_buf[length],
		        "... ERlookup failed twice:  status = %x", status);
		    length = STlength(msg_buf);
	        }
		else
		    length += msg_len;
	    }
	    else
		length += msg_len;
	}

        SIfprintf(stderr,"%s\n",msg_buf);
        PCexit(FAIL);
}
