/*
** Copyright (c) 2004 Ingres Corporation
**
**  Name: iicpydas.c -- contains main() of a program which copied 
**	existing JDBC server configuration to DAS server. 
**
**  Usage:
**      iicpydas resource 
**
**  Description:
**      Looks up JDBC configuration parameters in the default PM configuration 
**      file (config.dat) and  set the value to the corresponding DAS erver.
**
**  Exit Status:
**      OK      the value of the resource was displayed.
**      FAIL    the resource is undefined.
**
**  History:
**  09-Apr-2003 (wansh01)
**      Created.
**  29-Sep-2004 (drivi01)
**      Updated NEEDLIBS to link dynamic library SHQLIB to replace
**      its static libraries. Removed MALLOCLIB from NEEDLIBS
**  24-Jan-2007 (Ralph Loen) Bug 117541
**      Add the capability to detect and copy named JDBC server information.
**  19-Feb-2007 (Ralph Loen) Bug 117541
**      Fixed invalid pointer reference.
**  07-Mar-07 (Ralph Loen) Bug 117858
**      Fixed unnecessary string comparison against the JDBC server name.
**      The "name" string was never initialized anyway.
**  20-Aug-2008 (Ralph Loen) Bug 120793
**      Do not silently exit if PMget() fails to find a JDBC entry.  Instead,
**      continue through the search loop and update whatever JDBC items
**      are present.
**  03-Sep-2009 (frima01) 122490
**      Add include of pmutil.h
**      
*/

/*
PROGRAM =   (PROG1PRFX)cpydas
**
NEEDLIBS =  CONFIGLIB UTILLIB SHEMBEDLIB COMPATLIB 
**
UNDEFS =    II_copyright
**
DEST =      utility
*/

# include <compat.h>
# include <cm.h>
# include <me.h>
# include <pc.h>
# include <er.h>
# include <lo.h>
# include <pm.h>
# include <qu.h>
# include <si.h>
# include <st.h>
# include <simacro.h>
# include <util.h>
# include <erst.h>
# include <cr.h>
# include <pmutil.h>


#define DAS_CONFIG_MAX_SIZE    5
char *das_config_string[] =
{
    "client_max", "client_timeout", "connect_pool_expire", "connect_pool_size",
    "connect_pool_status"
};


main(int argc, char **argv)
{
    char *value, *protocol, *regexp, *scanName, *scanValue;
    char expbuf[ BIG_ENOUGH ];
    PM_CONTEXT *pm_id;
    PM_SCAN_REC state;
    int  i; 
    STATUS status;
    char *dupName[2];
    
    typedef struct
    {
         QUEUE name_q;
         char *name;
    }  NAME_LIST;

    NAME_LIST nm, *n;

    QUEUE *q = NULL;

    MEadvise( ME_INGRES_ALLOC );
    
    if( argc != 2 )
    {
        SIprintf( ERx( "\nUsage: %s name\n\n" ), argv[0] );
        PCexit( FAIL );
    }
    
    QUinit(&nm.name_q);

    PMmInit( &pm_id );
    
    PMmLowerOn( pm_id );  
    
    if( PMmLoad( pm_id, NULL, PMerror ) != OK )
    {
        SIfprintf( stderr, ERx( "\n%s\n\n" ),
        ERget( S_ST0315_bad_config_read ));
        PCexit( FAIL );
    }
    
    CRinit( pm_id );
    
    STprintf( expbuf, ERx( "%s.ingstart.%%.jdbc" ), argv[1]);

    /*
    **  Search for named JDBC servers and store their names in a
    **  linked list, if found.
    */
    regexp = PMmExpToRegExp( pm_id, expbuf );
    for( status = PMmScan( pm_id, regexp, &state, NULL,
        &scanName, &scanValue ); status == OK; status =
            PMmScan( pm_id, NULL, &state, NULL, &scanName,
                &scanValue ) )
    {
         scanName = PMmGetElem( pm_id, 3, scanName );
         n = (NAME_LIST *)MEreqmem(0, sizeof(NAME_LIST), TRUE, NULL );
         n->name = scanName;
         QUinsert((QUEUE *)n, &nm.name_q);
    }
    /*
    **  Search through the list of JDBC servers and copy their
    **  contents to the corresponding DAS server entries.  Set the
    **  startup counts of the JDBC servers to zero.
    */
    for (q = nm.name_q.q_prev; q != &nm.name_q; q = q->q_prev)
    {
        n = (NAME_LIST *)q;
        STprintf( expbuf, ERx( "%s.ingstart.%s.jdbc" ), argv[1], 
            n->name);
        CRsetPMval( expbuf, "0", stderr, TRUE, FALSE );
        if( PMmWrite( pm_id, NULL ) != OK )
        {
            SIfprintf( stderr, ERx( "\n%s\n\n" ),
            ERget( S_ST0314_bad_config_write ));
            PCexit( FAIL );
        }
        STprintf( expbuf, ERx("%s.ingstart.%s.gcd"), argv[1], 
            n->name );
        CRsetPMval( expbuf, scanValue, stderr, TRUE, FALSE );
        if( PMmWrite( pm_id, NULL ) != OK )
        {
            SIfprintf( stderr, ERx( "\n%s\n\n" ),
                ERget( S_ST0314_bad_config_write ));
                PCexit( FAIL );
        }
    
        for (i=0; i< DAS_CONFIG_MAX_SIZE; i++) 
        {
            STprintf( expbuf, ERx("%s.jdbc.%s.%s"), argv[1], n->name,
                    das_config_string[i] );
        
            if( PMmGet( pm_id, expbuf, &value ) == OK )
            {
                STprintf( expbuf, ERx("%s.gcd.%s.%s"), argv[1], 
                    n->name, das_config_string[i] );
                CRsetPMval( expbuf, value, stderr, TRUE, FALSE );
                if( PMmWrite( pm_id, NULL ) != OK )
                {
                    SIfprintf( stderr, ERx( "\n%s\n\n" ),
                           ERget( S_ST0314_bad_config_write ));
                    PCexit( FAIL );
                }
            }
        } /* for (i=0; i< DAS_CONFIG_MAX_SIZE; i++)  */
        
        /*   
        ** Retrieve JDBC protocol value.  
        */
        
        STprintf( expbuf, ERx("%s.jdbc.%s.protocol"), argv[1], n->name);
        
        if( PMmGet( pm_id, expbuf, &protocol ) == OK )
        {
            /*   
            ** Set DAS protocol status to "ON".   
            */
            STprintf( expbuf, ERx("%s.gcd.%s.%s.status"), argv[1], 
                        n->name, protocol);
            CRsetPMval( expbuf, ERx("ON"), stderr, TRUE, FALSE );
            if( PMmWrite( pm_id, NULL ) != OK )
            {
                SIfprintf( stderr, ERx( "\n%s\n\n" ),
                   ERget( S_ST0314_bad_config_write ));
                PCexit( FAIL );
            }
        }
    
        /*   
        ** Retrieve JDBC listening port value.  
        */
        STprintf( expbuf, ERx("%s.jdbc.%s.port"), argv[1], n->name);
        
        if( PMmGet( pm_id, expbuf, &value ) == OK )
        {
            /*   set DAS listening port value  
            */
            STprintf( expbuf, ERx("%s.gcd.%s.%s.port"), argv[1], 
                        n->name, protocol);
            CRsetPMval( expbuf, value, stderr, TRUE, FALSE );
            if( PMmWrite( pm_id, NULL ) != OK )
            {
                SIfprintf( stderr, ERx( "\n%s\n\n" ),
                   ERget( S_ST0314_bad_config_write ));
                PCexit( FAIL );
            }
        }
    } /* for (q = nm.name_q.q_prev; q != &nm.name_q; q = q->q_prev) */
    
    /*
    ** Free the linked list structure and PM resources.
    */
    for (q = nm.name_q.q_prev; q != &nm.name_q; q = nm.name_q.q_prev)
    {
        QUremove (q);
        MEfree((PTR)q);
    }

    PMmFree( pm_id );

    PCexit( OK );
}
    /* Return null function to pathname verification routine. */
BOOLFUNC *CRpath_constraint( CR_NODE *n, u_i4 un )
{
    return NULL;
}
