/*
** Copyright (c) 2004, 2005 Ingres Corporation
*/

/**
** Name: iffget.c
**
** Description:
**  Module provides the functions to retrieve instance and configuration data.
**
**(E
**  IFFgetInstances     - List Ingres instances for a target node
**  IFFgetConfigValues  - Retreive configuration values
**  IFFsetConfigValues  - Replace configuration values
**  IFFGetMdbName       - Retrieve Mdb database name
**)E
**
**  Instance information is retrieved with reserved credentials
**  using the GCA fastselect protocol connecting with the name
**  server (iigcn) process.
**
**  Configuration parameter values are retrieved with credentials
**  using embedded SQL connecting with imadb.
**
** History:
**      SIR 111718
**      08-Mar-2004 (fanra01)
**          Created.
**      23-Mar-2004 (fanra01)
**          Add instance id to returned instance values.
**          Enclosed strings in the required ERx macro.
**      08-Apr-2004 (fanra01)
**          Change memory allocation for values to used tagged memory.
**          Update trace messages to force message on errors.
**          Remove protocol from the instance information.
**      30-Jun-2004 (noifr01)
**          related to protocol removal from the instance information,
**          don't provide any more WINTCP as a default protocol (which
**          was typically wrong under LINUX). 
**      08-Jul-2004 (noifr01)
**          added IFFsetConfigValues() function
**      21-Jul-2004 (noifr01)
**          Added IFFGetMdbName() function
**      19-Aug-2004 (hweho01)
**          Star #13608350
**          Modified IFFGetMdbName(), MDB information become instance     
**          data, available for retrieval without user authentication.       
**      23-Aug-2004 (fanra01)
**          Sir 112892
**          Update comments for documentation purposes.
**      20-Sep-2004 (hweho01)
**          Corrected the pointer manipulation errors in IFFgetInstances() 
**          and IFFGetMdbName() routines.
**      21-Sep-2004 (hweho01)
**          Modified IFFGetMdbName() function, avoid seg. violation.  
**      24-Sep-2004 (fanra01)
**          Sir 113152
**          Pass status from return of exposed functions through
**          iff_return_status to decrease granularity of error codes.
**          Use error status for internal signalled errors.
**          Update IFFGetNdbName to register an exception handler.
**      12-Oct-2004 (fanra01)
**          Modified comments to reflect return status changes.
**          Imbed example code under the example comment section.
**      11-Jan-2005 (fanra01)
**          Minor changes to examples provided in comments.
**      27-Jan-2005 (fanra01)
**          More minor updates to examples provided in comments.
**      07-Feb-2005 (fanra01)
**          Sir 113881
**          Merge Windows and UNIX sources.
**      21-Feb-2005 (fanra01)
**          Sir 113975
**          Add IFFGetMdbInfo function.
**      23-Mar-2005 (fanra01)
**          Bug 114141
**          Replace sub-string comparisons in iff_get_mdbinfo with full string
**          comparisons.  A database name identical to the introducer element
**          causes an unexpected state change.
**      30-Mar-2005 (fanra01)
**          Bug 114177
**          Update example comments for IFFgetConfigValues.
**      01-Jun-2005 (fanra01)
**          SIR 114614
**          Add optional system path to instance information.
**      22-Sep-2005 (fanra01)
**          SIR 115252
**          The implied limit of one Ingres instance per machine hosting an
**          MDB database is to be removed for reporting purposes from this
**          API.  Extend the IFFGetMdbInfo function to handle multiple Ingres
**          instances hosting MDB databases.
**/
# include <compat.h>
# include <cm.h>
# include <cv.h>
# include <er.h>
# include <ex.h>
# include <me.h>
# include <qu.h>
# include <si.h>
# include <st.h>
# include <er.h>

# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <gca.h>
# include <gcn.h>
# include <gcm.h>

# include "iff.h"
# include "iffint.h"
# include "iffgca.h"

GLOBALDEF NET_PORT net_port[] =
{ 
    {ERx(""), NOPROT, 0}, 
    {ERx("TCP_IP"), TCPIP, DB_INT_TYPE}, 
    {ERx("WINTCP"), WINTCP, DB_INT_TYPE},
    {ERx("LANMAN"), LANMAN, DB_CHR_TYPE},
    { NULL, NOPROT, 0}
};

static II_CHAR* versconf[] = { "major", "minor", "build", NULL };

/*
** Name: iff_instance_data
**
** Description:
**  Places the returned instance data into the ING_INSTANCE
**  structure.
**      
** Inputs:
**      id              Instance identifier
**      addr            gcn address
**      inst            instance version data
**      proto           available protocols
**      buflen          length of available memory for instance data
**      
** Outputs:
**      instances       return structure for instance data
**      num_instances   instance counter
**      len             length of instances buffer used
**
** Returns:
**      status      OK      command completed successfully.
**                  FAIL    an error occurred processing the command.
**
** History:
**      08-Mar-2004 (fanra01)
**          Created.
**      23-Mar-2004 (fanra01)
**          Add instance id to returned values.
*/
static STATUS
iff_instance_data( char* id, char* addr, ING_INS_DATA* inst,
    ING_INS_DATA* proto, i4 buflen, ING_INSTANCE *instances,
    i4* num_instances, i4* len )
{
    STATUS          status = OK;
    GCM_DATA*       data;
    GCM_DATA*       data1;
    ING_INSTANCE*   insts = instances;
    ING_PORT*       port;
    i4              i;
    i4              length = 0;

    insts->num_ports = 0;
    for (data = (GCM_DATA *)inst->list.q_next;
        (QUEUE *)data != &inst->list;
        data = (GCM_DATA *)data->q.q_next)
    {
        STlcopy( data->data[0], insts->ver_string, MAX_IFF_VERS );
        STlcopy( addr, insts->addr, MAX_IFF_ADDR );

        CVal( data->data[2], &insts->version.major );
        CVal( data->data[3], &insts->version.minor );
        CVal( data->data[4], &insts->version.genlevel );
        CVal( data->data[5], &insts->version.byte_type );
        CVal( data->data[6], &insts->version.hardware );
        CVal( data->data[7], &insts->version.os );
        STlcopy( data->data[9], insts->id, MAX_IFF_ID );
        
        insts->conformance = 
            (STbcompare( data->data[11], 0, ERx("ON"), 0, TRUE )==0);
            
        STlcopy( data->data[12], insts->language, MAX_IFF_LANG );
        STlcopy( data->data[13], insts->char_set, MAX_IFF_CHAR );
        switch(inst->fields)
        {
            case 16:
                CVal( data->data[15], &insts->version.bldlevel );
            case 15:
                STlcopy( data->data[14], insts->system, MAX_IFF_SYSTEM );
                break;
            default:
                insts->version.bldlevel = 0;
                STlcopy( ERx("\0"), insts->system, 1 );
                break;
        }
        
        port = &insts->port;
        if (proto != NULL)
        {
            for (data1 = (GCM_DATA *)proto->list.q_next;
                (QUEUE *)data1 != &proto->list;
                data1 = (GCM_DATA *)data1->q.q_next)
            {
                for (i=0; net_port[i].protocol != NULL; i+=1)
                {
                    if (STbcompare( data1->data[0], 0, net_port[i].protocol,
                        0, TRUE) == 0)
                    {
                        port->protocol = net_port[i].prot_id;
                        port->type = net_port[i].type;
                        STlcopy( id, port->id, MAX_IFF_ID );
                        switch(net_port[i].type)
                        {
                            case DB_INT_TYPE:
                                CVal( data1->data[1], (i4*)(&port->port[0]) );
                                break;
                            case DB_CHR_TYPE:
                                STlcopy( data->data[1], port->port, MAX_IFF_PORT );
                                break;
                        }
                        port += 1;
                        insts->num_ports += 1;
                        if (((char*)port + sizeof(ING_PORT)) < ((char*)instances + buflen))
                        {
                            if (insts->num_ports > 1)
                            {
                                length += sizeof(ING_PORT);
                            }
                        }
                        else
                        {
                            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                                ERx("Insufficient memory for ING_INSTANCE\n") );
                            status = FAIL;
                        }
                        break;
                    }
                }
            }
        }
        else
        {
            port->protocol = net_port[0].prot_id;
            port->type = net_port[0].type;
            *(i4*)(&port->port[0]) = 0;
            STlcopy( insts->id, port->id, MAX_IFF_ID );
            insts->num_ports = 1;
            if (((char*)port + sizeof(ING_PORT)) < ((char*)instances + buflen))
            {
                if (insts->num_ports > 1)
                {
                    length += sizeof(ING_PORT);
                }
            }
            else
            {
                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                    ERx("Insufficient memory for ING_INSTANCE\n") );
                    status = FAIL;
            }
        }
        if (status == OK)
        {
            length += sizeof(ING_INSTANCE);
            *num_instances += 1;
            insts->length = length;
            *len = length;
        }
    }
    
    return(status);
}

/*
** Name: iff_instance_srv
**
** Description:
**  Formats the returned server classes as an array of server class
**      strings.
**      
** Inputs:
**      ins_ctx         context data
**      server          list of server class values
**      dbms            count of items in the list
**      
** Outputs:
**      srvclass
**      
** Returns:
**      status      OK      command completed successfully.
**                  FAIL    an error occurred processing the command.
**
** History:
**      08-Mar-2004 (fanra01)
**          Created.
*/
static STATUS
iff_instance_srv( ING_INS_CTX* ins_ctx, ING_INS_DATA* server, i4 dbms,
    ING_ADDR** srvclass )
{
    STATUS      status = OK;
    GCM_DATA*   data;
    ING_ADDR*   srv;
    ING_ADDR*   item;
    i4          i=0;
    
    if (dbms > 0)
    {
        if ((srv=(ING_ADDR*)MEreqmem( ins_ctx->metag, dbms * sizeof(ING_ADDR), TRUE,
            &status )) != NULL)
        {
            item = srv;
            for (data = (GCM_DATA *)server->list.q_next;
                (QUEUE *)data != &server->list;
                data = (GCM_DATA *)data->q.q_next)
            {
                if (data->data[0] != NULL)
                {
                    STlcopy( data->data[0], item->sclass, MAX_IFF_SVRCLASS - 1 );
                    STlcopy( data->data[1], item->addr, MAX_IFF_ADDR - 1 );
                    item += 1;
                    i+=1;
                }
            }
            *srvclass = srv;
        }
    }
    return( status );
}

/*
** Name: iff_instance_cnf
**
** Description:
**  Formats the returned configuration parameter values for return.
**      
** Inputs:
**      ins_ctx             context data
**      config              list of configuration parameter values
**      
** Outputs:
**      cnfcount            count of parameters
**      cnf                 array of configuration values
**      
** Returns:
**      status      OK      command completed successfully.
**                  FAIL    an error occurred processing the command.
**
** History:
**      08-Mar-2004 (fanra01)
**          Created.
**      08-Apr-2004 (fanra01)
**          Use tagged memory string allocation so that values will be freed
**          when the context is removed.
*/
static STATUS
iff_instance_cnf( ING_INS_CTX* ins_ctx, ING_INS_DATA* config, i4* cnfcount,
   ING_CONFIG** cnf )
{
    STATUS      status = OK;
    GCM_DATA*   data;
    i4          numvalues = 0;
    ING_CONFIG* pcnf;
    ING_CONFIG* item;
    
    /*
    ** count the parameters
    */
    for (data = (GCM_DATA *)config->list.q_next;
        (QUEUE *)data != &config->list;
        data = (GCM_DATA *)data->q.q_next)
    {
        numvalues +=1;
    }
    if (numvalues > 0)
    {
        item = pcnf = (ING_CONFIG*)MEreqmem( ins_ctx->metag,
            sizeof(ING_CONFIG)*numvalues, TRUE, &status );
        if (item == NULL)
        {
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("Config params memory allocation %x\n"),
                status );
        }
        else
        {
            for (data = (GCM_DATA *)config->list.q_next;
                (QUEUE *)data != &config->list;
                data = (GCM_DATA *)data->q.q_next)
            {
                item->param_name = STtalloc( ins_ctx->metag, data->data[1] );
                item->param_val = STtalloc( ins_ctx->metag, data->data[2] );
                item->param_val_len = STlength( data->data[2] );
                item += 1;
            }
            *cnf = pcnf;
            *cnfcount = numvalues;
        }
    }    
    return(status);
}

/*
** Name: iff_get_mdbinfo        - retrieve MDB information from configuration
**
** Description:
**  Parse the configuration parameter names until specific elements required
**  for MDB are found.
**  The format for the config parameter name is identical to those found
**  in config.dat.  Element counting is zero based.
**
**  Multi-line format
**      ii.localhost.mdb.mdb_dbname:         mymdb
**      ii.localhost.mdb.mymdb.version.major:    1
**      ii.localhost.mdb.mymdb.version.minor:    2
**      ii.localhost.mdb.mymdb.version.build:    30
**
**      ii.localhost.mdb.1.mdb_name:        mymdb1
**      ii.localhost.mdb.1.mymdb1.version.major:    2
**      ii.localhost.mdb.1.mymdb1.version.minor:    3
**      ii.localhost.mdb.1.mymdb1.version.build:    4
**
**  An array of pointers to each element of the parameter name is
**  used to perform comparisons. A set of bit flags is used to maintain
**  state for multi-line entries.
**
** Inputs:
**  metag       Tag id for memory allocations
**  id          Ingres instance identifier
**  mdbcnf      Array of MDB name value pairs
**  mcnt        Count of entries passed in mdbcnf
**  mdbinfo     Pointer to receive MDB information structure
**              If the value of this pointer is not null it is assumed that
**              the memory has been allocated by a previous call.
**  infocnt     Pointer to receive count of MDB structures returned
**              This field keeps a running total of structures updated and
**              is used to determine the point from which to continue
**              updates.
**
** Outputs:
**  mdbinfo     Updated MDB information
**  infocnt     Number of returned mdbinfo structures
**          
** Returns:
**  OK      successfully completed.
**          There is no error return.
**
** History:
**  21-Feb-2005 (fanra01)
**      Created.
**  23-Mar-2005 (fanra01)
**      Replace length parameters in STbcompare with zeros.
**  22-Sep-2005 (fanra01)
**      Extend returned MDBINFO array to handle multiple Ingres instances
**      each hosting MDB databases.
*/
static STATUS
iff_get_mdbinfo( u_i2 metag, char* id, ING_CONFIG* mdbcnf, i4 mcnt,
    ING_MDBINFO** mdbinfo, i4* infocnt )
{
    STATUS          status = OK;
    i4              i,j;
    i4              complete;
    i4              scan;
    i4              cnfsize;
    i4              element;
    i4              inst = 0;
    i4              flags;
    i4              passes = 0;
    ING_MDBINFO*    resptr;
    char*           cnfstr;
    char*           cnf[MAX_IFF_CNF_STR];
   
    /*
    ** Allocate an array of ING_MDBINFO structures for returned results.
    ** Assumes that MAX_IFF_MDBINFO is the maximum number of versions
    ** for all the Ingres instances detected.
    */
    if ((mdbinfo == NULL) || (infocnt == NULL) ||
        (*infocnt > MAX_IFF_MDBINFO))
    {
        EXsignal( IFF_EX_BAD_PARAM, 0, 0 );
    }
    if (*mdbinfo == NULL)
    {
        /*
        ** Allocate an array of structures for returned ING_MDBINFO.
        ** Assumes a maximum of MAX_IFF_MDBINFO allowed on a machine
        ** in various configurations of MDB databases per instance.
        */
        if ((resptr = (ING_MDBINFO*)MEreqmem( metag,
            sizeof(ING_MDBINFO)*MAX_IFF_MDBINFO, TRUE, &status )) == NULL)
        {
            EXsignal( IFF_EX_RESOURCE, 0, 0 );
        }
    }
    else
    {
        /*
        ** Adjust the starting point for new MDB instances, skipping
        ** previous entries.
        */
        resptr = *mdbinfo + *infocnt;
    }
    i=0;
    do
    {
        /*
        ** The elements are split and assumes MAX_IFF_CNF_STR individual
        ** components.
        ** Duplicate the configuration name string as breakdown into 
        ** strings alters the source string by replacing delimiters with
        ** end-of-string characters.
        */
        cnfsize = MAX_IFF_CNF_STR;
        cnfstr = STtalloc( metag, mdbcnf[i].param_name );
        if ((status = iff_get_strings( cnfstr, ERx("."), &cnfsize, cnf))
            == IFF_SUCCESS)
        {
            /*
            ** If the first element is a number assume there may be
            ** multiple instances.
            */
            if (CMdigit( cnf[0] ))
            {
                if ((CVan(cnf[0], &inst) != OK) && (inst > MAX_IFF_NUM_INST))
                {
                    element = 0;
                    inst = 0;
                }
                else
                {
                    element = 1;
                }
            }
            else
            {
                element = 0;
                inst = 0;
            }
            /*
            ** Scan entries looking for the MDB database name and then
            ** The version numbers.
            */
            if (STbcompare( cnf[element], 0, MDB_MDBNAME, 0, TRUE ) == 0)
            {
                resptr[inst].flags |= (IFF_MDB_INST|IFF_MDB_NAME);
                resptr[inst].name = mdbcnf[i].param_val;
                resptr[inst].id = id;
            }
            else if ((resptr[inst].flags & (IFF_MDB_INST|IFF_MDB_NAME)) &&
                (STbcompare( cnf[element], 0, resptr[inst].name,
                0, TRUE) == 0))
            {
                flags = resptr[inst].flags;
                for (j=0; versconf[j] != NULL; j+=1)
                {
                    if ((STbcompare( cnf[element+1], 0, MDB_MDBVERS,
                       0, TRUE) == 0) &&
                        (STbcompare( cnf[element+2], 0, versconf[j],
                       0, TRUE) == 0)) 
                    {
                        if (CVan( mdbcnf[i].param_val,
                            &resptr[inst].version[j] ) == OK)
                        {
                            resptr[inst].flags |= (IFF_MDB_VMAJ << j);
                            break;
                        }
                    }
                }
                /*
                ** No change to flags free the memory
                */ 
                if (flags == resptr[inst].flags)
                {
                    MEfree( cnfstr );
                }
            }
            else
            {
                MEfree( cnfstr );
            }
            i+=1;
        }

        /*
        ** Scanned elements
        */
        if ((i == mcnt) && (passes < 3))
        {
            for (j=0, complete=0, scan=0; j < MAX_IFF_CNF_STR; j+=1)
            {
                /*
                ** Count active return structures
                */
                scan = (resptr[j].flags != 0) ? scan+1 : scan;
                /*
                ** Count completed return structures
                */
                if ((resptr[j].flags != 0) &&
                    ((resptr[j].flags & IFF_MDB_MASK) == IFF_MDB_MASK))
                {
                    complete+=1;
                    resptr[j].cnf = mdbcnf;
                    resptr[j].numcnf = mcnt;
                }
            }
            /*
            ** If not completed reset counter to rescan elements
            */
            if (complete < scan)
            {
                passes+=1;
                i = 0;
            }
            else
            {
                /*
                ** Bump count of entries by the number found during the
                ** scan.
                */
                *infocnt += scan;
                
                /*
                ** Assign the return pointer only if it has not been
                ** set before.
                */
                if (*mdbinfo == NULL)
                {
                    *mdbinfo = resptr;
                }
            }
        }
    }
    while(i < mcnt);
    
    return(status);
}

/*
** Name: iff_exhandler
**
** Description:
**      Exception handler for IFFgetInstances
**      
** Inputs:
**      ex_args
**      
** Outputs:
**      None.
**      
** Returns:
**      EXDECLARE
**
** History:
**      08-Mar-2004 (fanra01)
**          Created.
*/
static STATUS
iff_exhandler(
EX_ARGS     *ex_args)
{
    char    buffer[256];
    if(EXsys_report(ex_args, buffer) != FALSE)
    {
        iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, ERx("System exception: %s\n"),
            buffer );
    }
    else
    {
        iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, ERx("Programmed exception\n") );
    }
    return (EXDECLARE);
}

/*{
** Name: IFFgetInstances
**
** Description:
**  Connects to the target host defined in the details argument and
**  retrieves a list of Ingres instances, the number of instances found
**  is returned in num_instances.
**  Each instance is identified by an ING_INSTANCE entry.  The length of
**  the entry is encoded in the length field that occupies the first 4
**  octets of the structure.
**
** Inputs:
**      ctx                 context structure
**      details             target host address information
**      buflen              length of instances memory area
**
** Outputs:
**      instances           list of returned instances
**      num_instances       number of instances
**      srvclass            list of server classes
**      params              list of configuration parameters
**      num_params          number of configuration parameters
**
** Returns:
**      status      IFF_SUCCESS             command completed 
**                  IFF_ERR_FAIL            error with no extra info 
**                  IFF_ERR_RESOURCE        error allocating resource
**                  IFF_ERR_PARAM           invalid parameter 
**                  IFF_ERR_STATE           invalid state 
**                  IFF_ERR_LOG_FAIL        unable to open trace log 
**                  IFF_ERR_COMMS           error making connection 
**                  IFF_ERR_SQLCODE         error retrieving data 
**
** Example:
**  # include <iff.h>
**
**  # define NUM_INSTANCES  10
**
**  static II_CHAR* host = { "remotehost" };
**  static II_CHAR* user = { "*" };
**  static II_CHAR* password = { "*" };
**
**  II_INT4         status = IFF_SUCCESS;
**  ING_CTX*        ctx = NULL;
**  ING_CONNECT     tgt;
**  ING_INSTANCE    ins[10];
**  II_INT4         buflen = sizeof(ING_INSTANCE)*NUM_INSTANCES;
**  II_INT4         num_ins = 0;
**
**  if ((status = IFFinitialize( &ctx )) == IFF_SUCCESS)
**  {
**      strcpy( tgt.hostname, host );
**      strcpy( tgt.user, user );
**      strcpy( tgt.password, password );
**
**      status = IFFgetInstances( ctx, &tgt, buflen, ins, &num_ins );
**  }
**
** History:
**      08-Mar-2004 (fanra01)
**          Created.
**      21-Sep-2004 (hweho01)
**          Avoid compiler error "Operand must be a modifiable lvalueon", 
**          on AIX platform, use temp_ptr to hold the intermediate 
**          pointer addition value.  
**      24-Sep-2004 (fanra01)
**          Add iff_return_status call.
}*/
II_INT4
IFFgetInstances(
	ING_CTX* ctx,
	ING_CONNECT* details,
	II_INT4 buflen,
	ING_INSTANCE* instances,
	II_INT4* num_instances
)
{
    STATUS          status = OK;
    EX_CONTEXT      context;
    ING_INS_CTX*    ins_ctx;
    BOOL            active = FALSE;
    ING_INS_DATA    instlist;
    ING_INSTANCE*   insts = instances;
    INST_DATA*      data1;
    ING_INS_DATA    server;
    ING_INS_DATA    instance;
    ING_INS_DATA    config;
    char*           temp_ptr;

    EXCEPT_BEGIN
    {
        if(EXdeclare( iff_exhandler, &context ) != OK)
        {
            EXdelete();
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, 
                ERx("%08x = IFFgetInstances(%p, %p, %x, %p, %p );\n"),
                status, ctx, details, buflen, instances, num_instances );
            return(iff_return_status( status ));
        }
        if ((ctx == NULL) || (details == NULL) || (instances == NULL))
        {
            status = IFF_ERR_PARAM;
            EXsignal( IFF_EX_BAD_PARAM, 0, 0 );
        }
        /*
        ** Check the state of the API at this point
        */
        ins_ctx = (ING_INS_CTX*)ctx->instance_context;
        if (ins_ctx->state < IFF_INITIALIZED)
        {
            status = IFF_ERR_STATE;
            EXsignal( IFF_EX_BAD_STATE, 0, 0 );
        }
        iff_trace( IFF_TRC_API, ins_ctx->trclvl,
                ERx("IFFgetInstances( %p,%p, %x, %p, %p );\n"),
                ctx, details, buflen, instances, num_instances );
        /*
        ** test existance of target
        */
        if ((status == OK) &&
            ((status = iff_gca_ping( details, ins_ctx->msg_buff, &active ))
            == OK))
        {
            if (active == TRUE)
            {
                QUinit( &instlist.list );

                /*
                ** Target has an instance
                ** Connect to the master name server
                ** Retrieve the list of registered instances
                **
                ** For each registered instance
                **      connect to the instance and retrieve server
                **      information
                */
                if ((status = iff_load_instances( ins_ctx, details,
                    &instlist)) == OK)
                {
                    i4              dbms;
                    i4              len;
                    i4              totlen = 0;

                    for (data1 = (INST_DATA *)instlist.list.q_next;
                         (QUEUE *)data1 != &instlist.list;
                         data1 = (INST_DATA *)data1->q.q_next)
                    {
                        iff_trace( IFF_TRC_CALL, ins_ctx->trclvl, 
                            ERx("%%4* %s\t%s\n"), data1->id, data1->addr );
                        QUinit( &server.list );
                        QUinit( &instance.list );
                        QUinit( &config.list );

                        /*
                        ** Determine if there are any dbms servers in the
                        ** instance.  Also get the list of protocols for
                        ** gcc if running.
                        */
                        if ((status = iff_dbms_server( ins_ctx, details,
                            data1->addr, &server, &dbms ))
                            != OK )
                        {
                            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, 
                                ERx("%x = iff_dbms_server( %p, %p, %s, %p, %p )\n"),
                                status, ins_ctx, details, data1->addr, 
                                &server, &dbms );
                            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, ERx("%4* dbms = %d\n"),
                                dbms );
                            break;
                        }
                        if (dbms)
                        {
                            /*
                            ** Get ING_INSTANCE info for instance addr
                            */
                            if ((status = iff_load_instance( ins_ctx, details,
                                data1->addr, &instance )) != OK)
                            {
                                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, 
                                    ERx("%x = iff_load_instance( %p, %p, %s, %p )\n"),
                                    status, ins_ctx, details, data1->addr, 
                                    &instance );
                                break;
                            }
                            /*
                            ** Get ING_CONFIG info
                            */
                            if ((status = iff_load_config( ins_ctx, details,
                                data1->addr, &config )) != OK)
                            {
                                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, 
                                    ERx("%x = iff_load_config( %p, %p, %s, %p )\n"),
                                    status, details, data1->addr, ins_ctx->msg_buff,
                                    &config );
                                break;
                            }
                            /*
                            ** Load data into return structures
                            */
                            iff_display( ins_ctx, &server, NULL, &instance,
                                &config );
                            if ((status = iff_instance_data( data1->id, data1->addr,
                                &instance, NULL, buflen, insts,
                                num_instances, &len )) != OK)
                            {
                                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, 
                                    ERx("%x = iff_instance_data( %s, %s, %p, %p, %d, %p, %p, %p )\n"),
                                    status, data1->id, data1->addr, &instance,
                                    NULL, buflen, insts, num_instances,
                                    &len );
                                break;
                            }
                            totlen += len;
                            if ((status = iff_instance_cnf( ins_ctx, &config,
                                &insts->num_cnf_params, &insts->cnf )) != OK)
                            {
                                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, 
                                    ERx("%x = iff_instance_cnf( %p, %p, %p, %p )\n"),
                                    ins_ctx, &config, &insts->num_cnf_params,
                                    &insts->cnf );
                                break;
                            }
                            if ((status = iff_instance_srv( ins_ctx, &server,
                                dbms, &insts->server )) != OK)
                            {
                                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, 
                                    ERx("%x = iff_instance_srv( %p, %p, %d, %p )\n"),
                                    ins_ctx, &server, dbms, &insts->server );
                                break;
                            }
                            insts->num_srvclass = dbms;
                            if (((char*)insts + len) < ((char*)instances + buflen))
                            {
                               temp_ptr = (char*)insts ;
                               insts = (ING_INSTANCE*) (temp_ptr + insts->length);
                            }
                            else
                            {
                                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, ERx("More instances\n") );
                            }
                            iff_unload_values( &server );
                            iff_unload_values( &instance );
                            iff_unload_values( &config );
                        }
                    }                    
                    if (status != OK)
                    {
                        iff_unload_values( &server );
                        iff_unload_values( &instance );
                        iff_unload_values( &config );
                    }
                    iff_unload_install( &instlist );
                }
                else
                {
                    iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                        ERx("Master name server request error %x\n"),
                        status );
                }
            }
            else
            {
                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                    ERx("Ping reguest to %s failed\n"),
                    details->hostname );
                status = FAIL;
            }
        }
    }
    EXCEPT_END
    EXdelete();
    return(iff_return_status( status ));
}

/*{
** Name: IFFgetConfigValues  - Retreive configuration values
**
** Description:
**  Function connects to the target and retrieves the values for
**  the requested parameters.
**
**  The values for the configuration parameters are requested and returned in
**  the value field of the parameter item.
**
**  The named list of requested configuration parameters is initialized in
**  the param_name field of the ING_CONFIG parameter argument.
**  If the param_val_len field of an ING_CONFIG parameter element is set on
**  entry it is assumed that the param_val has been provided by the calling
**  application.
**
**  If a parameter value for a specified param_name in parameters does not
**  exist or is not available then the param_val_len is zero.
**  If the param_len is zero and param_val is NULL on entry then the IFF API
**  allocates memory for the value.  This memory is freed when the function
**  IFFterminate is called.
**
**  Parameter names can be specified in full or as an SQL wildcard expression.
**(E
**      e.g.
**          ii.localhost.dbms.*.psf_memory  retrieves the value for psf_memory.
**          %memory                         retrieves all parameters ending
**                                          with the word memory.
**)E
**
** Inputs:
**
**      ctx                 context returned from IFFinitialize
**      details             target connection details
**      num_parms           number of requested parameters
**      parameters          parameter specifications
**
** Outputs:
**
**      num_ret_parms       number of returned parameter values
**      ret_parms           returned parameter values
**
** Returns:
**      status      IFF_SUCCESS             command completed 
**                  IFF_ERR_FAIL            error with no extra info 
**                  IFF_ERR_RESOURCE        error allocating resource
**                  IFF_ERR_PARAM           invalid parameter 
**                  IFF_ERR_STATE           invalid state 
**                  IFF_ERR_LOG_FAIL        unable to open trace log 
**                  IFF_ERR_COMMS           error making connection 
**                  IFF_ERR_SQLCODE         error retrieving data 
**
** Example:
**
**  # include <iff.h>
**  
**  # define NUM_INSTANCES  10
**  
**  static II_CHAR* host = { "hostname" };
**  static II_CHAR* user = { "ingres" };
**  static II_CHAR* password = { "ingrespwd" };
**  static ING_CONFIG  conf[] =
**  {
**      { "name like \'%RECOVERY%\'", 0, NULL  },
**      { "name like \'II.%\'"    , 0, NULL  },
**      { NULL, 0, NULL }
**  };
**  static II_INT4     num_parms = (sizeof(conf) / sizeof((conf)[0])) - 1;
**
**  II_INT4         status = IFF_SUCCESS;
**  ING_CTX*        ctx = NULL;
**  ING_CONNECT     tgt;
**  ING_INSTANCE    ins[10];
**  ING_INSTANCE*   inst;
**  ING_CONFIG*     cnf = NULL;
**  ING_ADDR*       addr;
**  ING_PORT*       port;
**  II_INT4         buflen = sizeof(ING_INSTANCE)*NUM_INSTANCES;
**  II_INT4         num_ins = 0;
**  II_INT4         num_cnf = 0;
**  II_INT4         i;
**
**  if ((status = IFFinitialize( &ctx )) == IFF_SUCCESS)
**  {
**      strcpy( tgt.hostname, host );
**      strcpy( tgt.user, user );
**      strcpy( tgt.password, password );
**      tgt.port.protocol = NOPROT;
**      tgt.port.type = 0;
**
**      status = IFFgetInstances( ctx, &tgt, buflen, ins, &num_ins );
**      if (status == IFF_SUCCESS)
**      {
**          inst=&ins[0];
**          addr = inst->server;
**          port=&inst->port;
**          tgt.port.protocol = port->protocol;
**          tgt.port.type = port->type;
**          strcpy( tgt.port.id, port->id);
**          strcpy( tgt.address.addr, inst->server->addr );
**          status = IFFgetConfigValues( ctx, &tgt, num_parms, conf, &num_cnf,
**              &cnf );
**          if (status != IFF_SUCCESS)
**          {
**              printf( "Error: %d\n", status );
**          }
**          else
**          {
**              for (i=0; i < num_cnf; i+=1)
**              {
**                  printf("%s\t%s\n", cnf[i].param_name, cnf[i].param_val );
**              }
**          }
**      }
**  }
**
** History:
**      08-Mar-2004 (fanra01)
**          Created.
**      24-Sep-2004 (fanra01)
**          Add iff_return_status call.
}*/
II_INT4
IFFgetConfigValues(
	ING_CTX* ctx,
	ING_CONNECT* details,
	II_INT4 num_parms,
	ING_CONFIG* parameters,
    II_INT4* num_ret_parms,
    ING_CONFIG** ret_parms
)
{
    STATUS          status = OK;
    EX_CONTEXT      context;
    ING_INS_CTX*    ins_ctx;
    
    EXCEPT_BEGIN
    {
        if(EXdeclare( iff_exhandler, &context ) != OK)
        {
            EXdelete();
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("%08x = IFFgetConfigValues(%p, %p, %x, %p, %p, %p);\n"),
                status, ctx, details, num_parms, parameters, num_ret_parms,
                ret_parms );
            return(iff_return_status( status ));
        }
        if ((ctx == NULL) || (details == NULL) || (parameters == NULL) ||
            (num_ret_parms == NULL) || (ret_parms == NULL))
        {
            status = IFF_ERR_PARAM;
            EXsignal( IFF_EX_BAD_PARAM, 0, 0 );
        }
        /*
        ** Check the state of the API at this point
        */
        ins_ctx = (ING_INS_CTX*)ctx->instance_context;
        if (ins_ctx->state < IFF_INITIALIZED)
        {
            status = IFF_ERR_STATE;
            EXsignal( IFF_EX_BAD_STATE, 0, 0 );
        }
        iff_trace( IFF_TRC_API, ins_ctx->trclvl,
                ERx("IFFgetConfigValues( %p, %p, %d, %p, %p, %p );\n"),
                ctx, details, num_parms, parameters, num_ret_parms, ret_parms );
        
        status = iff_query_config( ctx, details, num_parms, parameters,
            num_ret_parms, ret_parms );
        if (status != OK)
        {
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("%x = iff_query_config( %p, %p, %d, %p, %p, %p )\n"),
                status, ctx, details, num_parms, parameters, num_ret_parms,
                ret_parms );
        }
    }
    EXCEPT_END
    EXdelete();
    return(iff_return_status( status ));
}

/*{
** Name: IFFsetConfigValues  - Replace configuration values
**
** Description:
**  Function connects to the target and sets the values for
**  the requested parameters.
**
**  Scans the list of parameters and identifies any where either the parameter
**  name or the value will exceed the size of the request buffers.
**
**  A connection string is formed from the details in the ING_CONNECT
**  structure and a connection is established to the target.
**
**  For each parameter in the list the command
**(E
**      iisetres <param> <value>
**)E
**  is issued.
**(E
**      param       specifies the full parameter name
**                  e.g.
**                      'ii.localhost.dbms.*.psf_memory' size of psf_memory
**      value       specifies the value to replace the existing value.
**)E
**  The command is passed to the rmcmd server by executing the database
**  procedure launchremotecmd with the command string as the parameter.
**
**  Any output from each command is returned in a list of ING_CONFIG items.
**
** Inputs:
**
**      ctx                 context returned from IFFinitialize
**      details             target connection details
**      num_parms           number of config.dat parameters to set
**      parameters          parameter specifications
**
** Outputs:
**
**      num_ret_parms       number of output lines
**      ret_parms           output lines (in param_name), including, for each
**                          variable to be set:
**                          - the text of the iisetres command that has been
**                            internally executed for setting the variable
**                          - the ouput of that command (if any. typically,
**                            there is no output if the command has been 
**                            successful
**
**
** Returns:
**      status      IFF_SUCCESS             command completed 
**                                          iisetres was executed for all
**                                          parameters (however, the output
**                                          lines must be tested for ensuring
**                                          no incorrect output was generated)
**                  IFF_ERR_FAIL            error with no extra info 
**                  IFF_ERR_RESOURCE        error allocating resource
**                  IFF_ERR_PARAM           invalid parameter 
**                  IFF_ERR_STATE           invalid state 
**                  IFF_ERR_LOG_FAIL        unable to open trace log 
**                  IFF_ERR_COMMS           error making connection 
**                  IFF_ERR_SQLCODE         error retrieving data 
**                  IFF_ERR_PARAMNAME_LEN   name parameter too long 
**                  IFF_ERR_PARAMVALUE_LEN  value parameter too long 
**                  IFF_ERR_MDBBUFSIZE      Buffer size exceeded 
**
** Example:
**  # include <iff.h>
**
**  # define NUM_INSTANCES  10
**
**  static II_CHAR* host = { "remotehost" };
**  static II_CHAR* user = { "ingres" };
**  static II_CHAR* password = { "ingrespwd" };
**  static ING_CONFIG  confset[] =
**  { 
**      { "a.a" ,0, "hello" },
**      { "a.c" ,0, "c"     },
**      { "a.e" ,0, "e"     },
**      { "a.f" ,0, "f"     },
**      { "a.g" ,0, "g"     },
**      { "a.h" ,0, "h"     },
**      { NULL, 0, NULL } 
**  };
**  static II_INT4     num_parms = (sizeof(confset) / sizeof((confset)[0])) - 1;
**
**  II_INT4         status = IFF_SUCCESS;
**  ING_CTX*        ctx = NULL;
**  ING_CONNECT     tgt;
**  ING_INSTANCE    ins[10];
**  ING_INSTANCE*   inst;
**  ING_ADDR*       addr;
**  ING_PORT*       port;
**  II_INT4         buflen = sizeof(ING_INSTANCE) * NUM_INSTANCES;
**  II_INT4         num_ins = 0;
**  II_INT4         num_cnf = 0;
**
**  if ((status = IFFinitialize( &ctx )) == IFF_SUCCESS)
**  {
**      strcpy( tgt.hostname, host );
**      strcpy( tgt.user, user );
**      strcpy( tgt.password, password );
**
**      status = IFFgetInstances( ctx, &tgt, buflen, ins, &num_ins );
**      if (status == IFF_SUCCESS)
**      {
**          inst=&ins[0];
**          addr = inst->server;
**          port=&inst->port;
**          tgt.port.protocol = port->protocol;
**          tgt.port.type = port->type;
**          strcpy( tgt.port.id, port->id);
**          strcpy( tgt.address.addr, inst->server->addr );
**          status = IFFsetConfigValues( ctx,&tgt,num_parms,confset, &num_cnf,
**              &cnf );
**          if (status != IFF_SUCCESS)
**          {
**              printf( "Error: %d\n", status );
**          }
**      }
**  }
**
** History:
**      07-Jul-2004 (noifr01)
**          Created (derived from IFFgetConfigValues)
**      24-Sep-2004 (fanra01)
**          Add iff_return_status call.
}*/
II_INT4
IFFsetConfigValues(
	ING_CTX* ctx,
	ING_CONNECT* details,
	II_INT4 num_parms,
	ING_CONFIG* parameters,
    II_INT4* num_ret_parms,
    ING_CONFIG** ret_parms
)
{
    STATUS          status = OK;
    EX_CONTEXT      context;
    ING_INS_CTX*    ins_ctx;
    
    EXCEPT_BEGIN
    {
        if(EXdeclare( iff_exhandler, &context ) != OK)
        {
            EXdelete();
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("%08x = IFFsetConfigValues(%p, %p, %x, %p, %p, %p);\n"),
                status, ctx, details, num_parms, parameters, num_ret_parms,
                ret_parms );
            return(iff_return_status( status ));
        }
        if ((ctx == NULL) || (details == NULL) || (parameters == NULL) ||
            (num_ret_parms == NULL) || (ret_parms == NULL))
        {
            status = IFF_ERR_PARAM;
            EXsignal( IFF_EX_BAD_PARAM, 0, 0 );
        }
        /*
        ** Check the state of the API at this point
        */
        ins_ctx = (ING_INS_CTX*)ctx->instance_context;
        if (ins_ctx->state < IFF_INITIALIZED)
        {
            status = IFF_ERR_STATE;
            EXsignal( IFF_EX_BAD_STATE, 0, 0 );
        }
        iff_trace( IFF_TRC_API, ins_ctx->trclvl,
                ERx("IFFsetConfigValues( %p, %p, %d, %p, %p, %p );\n"),
                ctx, details, num_parms, parameters, num_ret_parms, ret_parms );
        
        status = iff_set_config( ctx, details, num_parms, parameters,
            num_ret_parms, ret_parms );
        if (status != OK)
        {
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("%x = iff_set_config( %p, %p, %d, %p, %p, %p )\n"),
                status, ctx, details, num_parms, parameters, num_ret_parms,
                ret_parms );
        }
    }
    EXCEPT_END
    EXdelete();
    return(iff_return_status( status ));
}

/*{
** Name: IFFGetMdbName       - Retrieve Mdb database name
**
** Description:
**      Function will parse the instance data returned from
**      IFFgetInstances(), and store the MDB information  
**      into the ING_MDBNAME structure for returning. 
**
** Inputs:
**
**(E
**      ctx                context returned from IFFinitialize
**      details            target connection details
**      ing_mdbptr         pointer to a pointer array of ING_MDBNAME structure  
**      mdb_cnt            for storing the number of MDB in the server 
**)E
**
** Outputs:
**
**   If MDB signature data is found : 
**      mdb_cnt will be equal or greater than one, it is possible there 
**      are multiple MDBs in a server.    
**      For each MDB name, A pair of data will be returned in ING_DBNAME
**      structure as :   
**        MDB name string      ==>    mdb_name field    
**        instance ID string   ==>    id  field   
**
**        The structure of ING_MDBNAME is defined as :
**(E
**         {  
**           II_CHAR*     mdb_name;
**           II_CHAR*     id;
**         };
**)E
**      The structure pointer array of ING_DBNAME is pointed by ing_mdbptr
**      which is passed by the caller.
**
**   If MDB signature data is not found, the mdb_cnt will be zero. 
**
**
** Returns:
**      status      IFF_SUCCESS             command completed 
**                  IFF_ERR_FAIL            error with no extra info 
**                  IFF_ERR_RESOURCE        error allocating resource
**                  IFF_ERR_PARAM           invalid parameter
**                  IFF_ERR_STATE           invalid state 
**                  IFF_ERR_LOG_FAIL        unable to open trace log 
**                  IFF_ERR_COMMS           error making connection 
**                  IFF_ERR_SQLCODE         error retrieving data 
**
** Example:
**  # include <iff.h>
**
**  # define NUM_INSTANCES  10
**
**  static II_CHAR* host = { "remotehost" };
**  static II_CHAR* user = { "*" };
**  static II_CHAR* password = { "*" };
**
**  II_INT4         status = IFF_SUCCESS;
**  ING_CTX*        ctx = NULL;
**  ING_CONNECT     tgt;
**  ING_MDBNAME*    ing_mdbptr = NULL;
**  II_INT4         mdb_cnt = 0; 
**
**  if ((status = IFFinitialize( &ctx )) == IFF_SUCCESS)
**  {
**      strcpy( tgt.hostname, host );
**      strcpy( tgt.user, user );
**      strcpy( tgt.password, password );
**
**      status = IFFGetMdbName( ctx, &tgt, &ing_mdbptr, &mdb_cnt );
**  }
**
** History:
**      21-Jul-2004 (noifr01)
**          Created (derived from IFFgetConfigValues/IFFsetConfigValues)
**      26-Aug-2004 (hweho01)
**          Modified the implementation,  MDB name can be retrieved 
**          as instance data, no user authentication is required.
**      24-Sep-2004 (fanra01)
**          Add iff_return_status call.
**          Add exception handler registration.
}*/
II_INT4
IFFGetMdbName(
    ING_CTX*     ctx,
    ING_CONNECT* details,
    ING_MDBNAME** ing_mdbptr,
    II_INT4*      mdb_cnt 
        )
{
    STATUS          status = OK;
    EX_CONTEXT      context;
    ING_INS_CTX*    ins_ctx;

    II_INT4         buflen = sizeof(ING_INSTANCE)*MAX_IFF_NUM_INST;
    II_INT4         num_ins = 0; 
    II_INT4         i, j, len; 
    ING_INSTANCE    instances[MAX_IFF_NUM_INST];
    ING_INSTANCE*   inst;
    ING_MDBNAME*    temp_ptr; 
    ING_MDBNAME*    first_ptr = NULL;

    
    EXCEPT_BEGIN
    {
        if(EXdeclare( iff_exhandler, &context ) != OK)
        {
            EXdelete();
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("%08x = IFFGetMdbName(%p, %p, %p, %p);\n"),
                status, ctx, details, ing_mdbptr, mdb_cnt );
            return(iff_return_status( status ));
        }
        if ((ctx == NULL) || (details == NULL) || (ing_mdbptr == NULL) ||
            (mdb_cnt == NULL))
        {
            status = IFF_ERR_PARAM;
            EXsignal( IFF_EX_BAD_PARAM, 0, 0 );
        }
        /*
        ** Check the state of the API at this point
        */
        ins_ctx = (ING_INS_CTX*)ctx->instance_context;
        if (ins_ctx->state < IFF_INITIALIZED)
        {
            status = IFF_ERR_STATE;
            EXsignal( IFF_EX_BAD_STATE, 0, 0 );
        }
        iff_trace( IFF_TRC_API, ins_ctx->trclvl,
            ERx("IFFGetMdbName(%p, %p, %p, %p);\n"),
            ctx, details, ing_mdbptr, mdb_cnt );

        *ing_mdbptr = NULL; 
        *mdb_cnt = 0 ; 
        status = IFFgetInstances( ctx, details, buflen, instances, &num_ins );
        if( status == OK && num_ins > 0 )
        {
            first_ptr = (ING_MDBNAME*)MEreqmem( ins_ctx->metag, 
                sizeof(ING_MDBNAME)*MAX_IFF_NUM_INST, TRUE, &status );
            if ( first_ptr == NULL)
            {
                EXsignal( IFF_EX_RESOURCE, 0, 0 );
            }
            temp_ptr = first_ptr; 
            for (i=0, inst=&instances[0] ; i < num_ins; i+=1)
            {
                for(j=0; j < inst->num_cnf_params; j+=1)
                {
                    if(STstrindex(inst->cnf[j].param_name, MDB_MDBNAME,
                        0 , TRUE ) != NULL )
                    {
                        temp_ptr->mdb_name=inst->cnf[j].param_val;  
                        temp_ptr->id = inst->id;
                        temp_ptr += 1; 
                        *mdb_cnt += 1;  
                        break; 
                    }
                }
                len = inst->length;
                inst = (ING_INSTANCE*) ( (char*)inst + len );
            }
        }
        else
        {
            if( status != OK )
               iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                  ERx("%08x = IFFgetInstances(%p, %p, %x, %p, %d );\n"),
                  status, ctx, details, buflen, instances, num_ins );
        }

        if( *mdb_cnt > 0 )
        {
            *ing_mdbptr = first_ptr;
        }
        else
        {
            if (first_ptr != NULL)
            {
                MEfree( (PTR)first_ptr );
            }
        }
    }
    EXCEPT_END
    EXdelete();

    return(iff_return_status( status ));
}

/*{
** Name: IFFGetMdbInfo       - Retrieve Mdb database information
**
** Description:
**  The function parses the instance configuration data returned from
**  IFFgetInstances(), and returns the MDB version information into an array
**  of ING_MDBINFO structures.  The function will also return a list of
**  configuration items retrieved within the ING_MDBINFO structure allowing
**  the caller to extract miscellaneous information.  The list of ING_CONFIG
**  items is not guaranteed to be ordered or that grouping will be maintained.
**
**  Instance configuration data is returned as a parameter name, value
**  pair.  The format of the parameter name is identical to those found
**  in the config.dat file, composed of dot '.' delimited elements, except
**  that ii.hostname.mdb introducer is removed.
**
**  For the purposes of extracting MDB related values from the configuration
**  data, a predefined element format is assumed.  It is also assumed that
**  only one instance of the MDB will be defined in the config.dat data.
**
**  The name element multi-line format is as follows:
**
**  Multi-line format - database name
**      ii.hostname.mdb.mdb_dbname
**  
**  Element 0   ii
**          1   hostname
**          2   mdb
**          3   mdb_dbname
**
**  Multi-line format - version
**      ii.hostname.mdb.dbname.version.part
**
**  Element 0   ii
**          1   hostname
**          2   mdb
**          3   dbname, element the value of mdb_dbname
**          4   version
**          5   part, of either major, minor, or build
**
**  Multi-line format - miscellaneous fields
**      ii.hostname.mdb.mymdb.*
**
**  Element 0   ii
**          1   hostname
**          2   mdb
**          3   dbname, element the value of mdb_dbname
**          4   * field name
**
**  Example of a multi-line format
**      ii.hostname.mdb.mdb_dbname:             mymdb
**      ii.hostname.mdb.mymdb.version.major:    1
**      ii.hostname.mdb.mymdb.version.minor:    2
**      ii.hostname.mdb.mymdb.version.build:    30
**      ii.hostname.mdb.mymdb.description:      Sales department
**
**  In the processing of the MDB version the fields mdb_dbname, version.major,
**  version.minor, and version.build must be defined.  If any one of these
**  fields is omitted the function is unable to reconstruct a numeric
**  version number and no information is returned in the ING_MDBINFO
**  structure.
**
**
** Inputs:
**
**  ctx                context returned from IFFinitialize
**  details            target connection details
**  mdbinfo            pointer for returned array of ING_MDBINFO structures
**  infocnt            count for returned mdbinfo structures 
**
** Outputs:
**  mdbinfo            array of ING_MDBINFO structures
**  infocnt            count of mdbinfo structures returned 
**
** Returns:
**  status     IFF_SUCCESS             command completed 
**             IFF_ERR_FAIL            error with no extra info 
**             IFF_ERR_RESOURCE        error allocating resource
**             IFF_ERR_PARAM           invalid parameter
**             IFF_ERR_STATE           invalid state 
**             IFF_ERR_LOG_FAIL        unable to open trace log 
**             IFF_ERR_COMMS           error making connection 
**             IFF_ERR_SQLCODE         error retrieving data 
**
** Example:
**  # include <iff.h>
**
**  static II_CHAR* host = { "remotehost" };
**  static II_CHAR* user = { "*" };
**  static II_CHAR* password = { "*" };
**
**  II_INT4         status = IFF_SUCCESS;
**  ING_CTX*        ctx = NULL;
**  ING_CONNECT     tgt;
**  ING_MDBINFO*    mdbinfo = NULL;
**  II_INT4         infocnt = 0;
**  II_INT4         i,j;
**
**  if ((status = IFFinitialize( &ctx )) == IFF_SUCCESS)
**  {
**      strcpy( tgt.hostname, host );
**      strcpy( tgt.user, user );
**      strcpy( tgt.password, password );
**
**      status = IFFGetMdbInfo( ctx, &tgt, &mdbinfo, &infocnt );
**      if (status == IFF_SUCCESS)
**      {
**          for(i=0; i < infocnt; i+=1)
**          {
**              printf( "Name    = %s\n", mdbinfo.name);
**              printf( "Instance= %s\n", mdbinfo.id);
**              printf( "Version = %d.%d.%d\n",
**                  mdbinfo[i].version[IFF_MDB_MAJOR],
**                  mdbinfo[i].version[IFF_MDB_MINOR],
**                  mdbinfo[i].version[IFF_MDB_BUILD] );
**              for (j=0; j < mdbinfo[i].numcnf; j+=1)
**              {
**                  printf( "%s:    %s\n", mdbinfo[i].cnf[j].param_name,
**                      mdbinfo[i].cnf[j].param_val );
**              }
**          }
**      }
**  }
**
** History:
**      21-Feb-2005 (fanra01)
**          Created.
}*/
II_INT4
IFFGetMdbInfo(
    ING_CTX*        ctx,
    ING_CONNECT*    details,
    ING_MDBINFO**   mdbinfo,
    II_INT4*        infocnt 
        )
{
    STATUS          status = OK;
    EX_CONTEXT      context;
    ING_INS_CTX*    ins_ctx;

    II_INT4         buflen = sizeof(ING_INSTANCE)*MAX_IFF_NUM_INST;
    II_INT4         num_ins = 0; 
    II_INT4         i, j, len; 
    ING_INSTANCE*   inst;
    ING_MDBINFO*    resptr = NULL;
    II_CHAR*        cnfstr = NULL;
    II_CHAR*        cnfver = NULL;
    II_INT4         mdbflags = 0;
    ING_CONFIG*     mdbcnf;
    II_INT4         mcnt;
    II_CHAR*        p;

    EXCEPT_BEGIN
    {
        if(EXdeclare( iff_exhandler, &context ) != OK)
        {
            EXdelete();
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("%08x = IFFGetMdbInfo(%p, %p, %p, %p);\n"),
                status, ctx, details, mdbinfo, infocnt );
            return(iff_return_status( status ));
        }
        if ((ctx == NULL) || (details == NULL) || (mdbinfo == NULL) ||
            (infocnt == NULL))
        {
            status = IFF_ERR_PARAM;
            EXsignal( IFF_EX_BAD_PARAM, 0, 0 );
        }
        /*
        ** Check the state of the API at this point
        */
        ins_ctx = (ING_INS_CTX*)ctx->instance_context;
        if (ins_ctx->state < IFF_INITIALIZED)
        {
            status = IFF_ERR_STATE;
            EXsignal( IFF_EX_BAD_STATE, 0, 0 );
        }
        iff_trace( IFF_TRC_API, ins_ctx->trclvl,
            ERx("IFFGetMdbInfo(%p, %p, %p, %p);\n"),
            ctx, details, mdbinfo, infocnt );

        inst = (ING_INSTANCE*)MEreqmem( ins_ctx->metag, buflen, TRUE,
            &status );
        if (inst == NULL)
        {
            EXsignal( IFF_EX_RESOURCE, 0, 0 );
        }
        *mdbinfo = NULL; 
        *infocnt = 0 ; 
        status = IFFgetInstances( ctx, details, buflen, inst, &num_ins );
        if( status == OK && num_ins > 0 )
        {
            for (i=0; i < num_ins; i+=1)
            {
                mdbcnf = (ING_CONFIG*)MEreqmem( ins_ctx->metag,
                    sizeof(ING_CONFIG)*inst->num_cnf_params, TRUE, &status );
                for(j=0, mcnt=0; (j < inst->num_cnf_params); j+=1)
                {
                    /*
                    ** Extract a list of MDB entries
                    */
                    if ((p = STstrindex( inst->cnf[j].param_name,
                        MDB_ELEMENT, 0, TRUE )) != NULL)
                    {
                        p+=STlength( MDB_ELEMENT );
                        mdbcnf[mcnt].param_name = STtalloc( ins_ctx->metag, p );
                        mdbcnf[mcnt].param_val = inst->cnf[j].param_val;
                        mdbcnf[mcnt].param_val_len = STlength( inst->cnf[j].param_val );
                        mcnt+=1;
                    }
                }
                if (mcnt > 0)
                {
                    status = iff_get_mdbinfo( ins_ctx->metag, inst->id, mdbcnf,
                        mcnt, mdbinfo, infocnt );
                }
                len = inst->length;
                inst = (ING_INSTANCE*) ( (char*)inst + len );
            }
        }
        else
        {
            if( status != OK )
               iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                  ERx("%08x = IFFgetInstances(%p, %p, %x, %p, %d );\n"),
                  status, ctx, details, buflen, inst, num_ins );
        }
    }
    EXCEPT_END
    EXdelete();

    return(iff_return_status( status ));
}

