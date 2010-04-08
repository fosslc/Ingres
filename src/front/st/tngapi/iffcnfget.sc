/*
** Copyright (c) 2004, 2005 Ingres Corporation
*/

/**
** Name: iffcnfget
**
** Description:
**      Module provides the functions to retrieve and store config data.
**
**(E
**      iff_query_config    - retrieve values of configuration items
**      iff_set_config      - replace values of configuration items
**      iff_getmdbname      - retrieve mdb database name
**)E
**
** History:
**      SIR 111718
**      24-Apr-2004 (fanra01)
**          Created.
**      08-Jul-2004 (noifr01)
**          added iff_set_config() function
**      08-Jul-2004 (noifr01)
**          cleanup (related to error and output report) in iff_set_config()
**      21-Jul-2004 (noifr01)
**          - in iff_query_config, now raise rmcmdnewcmd dbevent for faster
**            activation of rmcmd (for fixing a performance issue under
**            windows)
**          - removed 2 unused variables
**          - added iff_getmdbname() function
**      23-Aug-2004 (fanra01)
**          Sir 112892
**          Updated comments for documentation purposes.
**      19-Aug-2004 (hweho01)
**          Star #13608350
**          The MDB name will be retrieved as a public data, the operation  
**          will be done in IFFGetMdbName() only, remove iff_getmdbname()
**          function.
**      22-Sep-2004 (noifr01)
**          fixed potential issue of iff_set_config possibly returning FAIL
**          even if the command is executed successfully, in the situation 
**          where another, independent remote command completes immedialy
**          after the execution of a "get dbevent" statement performed in
**          iff_set_config
**          a timeout of 20 seconds will now always be given to rmcmd for
**          notifying that the command has been taken into account (but it
**          can take more to have it completed)
**      08-Oct-2004 (noifr01)
**          Bug 113205
**          iff_query_config and iff_set_config weren't reporting errors
**          correctly
**      08-Oct-2004 (noifr01)
**          Bug 113205
**          additional fix for the case of an sql return value of 100 
**          in one or more queries performed in an iff_query_config
**          call (the design being to return OK even if one or more
**          more of the queries, within the loop, has returned no row)
**      14-Oct-2004 (noifr01)
**          Bug 113217 (deadlocks encountered upon concurrent rmcmd requests)
**          In iff_set_config:
**          - set lockmode session where readlock = exclusive
**          - inserted a commit between 2 select statements in 2 different
**            rmcmd tables
**          - simplified the query sequence through usage of the value
**            returned by the launchremotecmd procedure
**          - manage SQL warnings 710 that could result in erroneous failure
**            in the case of concurrent rmcmd calls.
**      07-Feb-2005 (fanra01)
**          Sir 113881
**          Merge Windows and UNIX sources.
**      20-Mar-2005 (fanra01)
**          Bug 114177
**          A NULL parameter name field passed as a query qualifier causes
**          an exception.
**      01-Aug-2005 (fanra01)
**          Bug 114967
**          Add Declarations and definitions to enable multi-threaded ESQL-C
**          sessions for configuration retrieval.
**          Serialize the retrieval.
**/
# include <compat.h>
# include <er.h>
# include <me.h>
# include <mu.h>
# include <qu.h>
# include <st.h>
# include <tm.h>

# include "iff.h"
# include "iffint.h"
extern NET_PORT net_port[];

exec sql include sqlca;

/*
** Definitions for multi-threaded communications area.
** See Embedded SQL Companion Guide.
** This is equivalent to specifying a -multi flag on the command line.
*/
IISQLCA *IIsqlca();
#define sqlca (*(IIsqlca()))

GLOBALDEF   MU_SEMAPHORE iffconfig_sem = { 0 };

static char dbname[] = { ERx("imadb") };
static char query[] =
    { ERx("select name, value from ima_config") };
static char cond[] = { ERx(" where ") };

/*
** Name: iff_query_config    - retrieve values of configuration items
**
** Description:
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
**      status      OK      command completed successfully.
**                  FAIL    an error occurred processing the command.
**
** History:
**      24-Apr-2004 (fanra01)
**          Created.
**      30-Mar-2005 (fanra01)
**          Ensure that parameter qualifier is valid before attempting to use
**          the pointer.
*/
STATUS
iff_query_config(
    ING_CTX* ctx,
	ING_CONNECT* details,
	II_INT4 num_parms,
	ING_CONFIG* parameters,
    II_INT4* num_ret_parms,
    ING_CONFIG** ret_parms
)
{
    STATUS status = OK, status1;
    ING_INS_CTX*    ins_ctx = (ING_INS_CTX*)ctx->instance_context;
    char            addr[MAX_IFF_ADDR];
    ING_INS_DATA    lst;
    ING_CNF_DATA*   cnf;
    ING_CNF_DATA*   tmp;
    i4              i;
    ING_CONFIG*     param;
    i4              stmtlen;
exec sql begin declare section;    
    char*           address;
    char            pmname[MAX_IFF_PMNAME];
    char            pmvalue[MAX_IFF_PMVALUE];
    char*           statement;
    IISQLCA        sqlca_ptr = sqlca;
exec sql end declare section;

    /*
    ** Form a direct connection target string.
    ** If the protocol is not specified use the default registry
    ** protocol and port.
    */
    if (details->port.protocol > 0)
    {
        STprintf( addr, ERx("@%s,%s,%s[%s,%s]::%s/@%s"), details->hostname,
            net_port[details->port.protocol].protocol, details->port.id,
            details->user, details->password, dbname,
            details->address.addr );
    }
    else
    {
        /*
        ** No protocol and port specified.
        ** Use default registry protocol and port.
        */
        STprintf( addr, ERx("@%s[%s,%s]::%s/@%s"), details->hostname,
            details->user, details->password, dbname, 
            details->address.addr );
    }

    MEfill( sizeof(ING_INS_DATA), 0, &lst );
    address = addr;
    QUinit( &lst.list );
    lst.fields = 0;
    /*
    ** Serialize the retrieval
    */
    MUp_semaphore( &iffconfig_sem );

    /*
    ** Connect to target.
    */
exec sql connect :address;
    if (sqlca.sqlcode == 0)
    {
        /*
        ** If vnode domain fails the select statement should return no rows.
        */
exec sql execute procedure ima_set_vnode_domain;
        /*
        ** For each config parameter item
        **      execute a query with a 'where clause'.
        **      retrieve the results into a results area.
        */
        for (i=0, param=&parameters[i]; i < num_parms; i+=1, param=&parameters[i])
        {
            stmtlen = STlength( query );
            stmtlen += (param->param_name != NULL) ?
                (STlength( param->param_name ) + sizeof(cond)): 0;
            stmtlen += 1;               /* include string terminator */
            if ((statement = MEreqmem( 0, stmtlen, TRUE, &status1 )) != NULL)
            {
                if ((param->param_name != NULL) && (*param->param_name != '\0'))
                {
                    STpolycat( 3, query, cond, param->param_name, statement );
                }
                else
                {
                    STcopy( query, statement );
                }
exec sql raise dbevent rmcmdnewcmd;                
exec sql execute immediate :statement into :pmname, :pmvalue;
exec sql begin;
                if (sqlca.sqlcode == 0)
                {
                    if ((cnf = (ING_CNF_DATA*)MEreqmem( ins_ctx->metag,
                        sizeof(ING_CNF_DATA), TRUE, &status )) != NULL)
                    {
                        cnf->data[0] = STtalloc( ins_ctx->metag, pmname );
                        cnf->data[1] = STtalloc( ins_ctx->metag, pmvalue );
                        QUinsert( &cnf->list, lst.list.q_prev );
                        lst.fields += 1;
                    }
                }
                else
                {
exec sql endselect;
                }
exec sql end;
                MEfree( statement );
                status = sqlca.sqlcode;
                /* no returned row in one (or more) query within the loop */
                /* shouldn't result in an error nor in exiting the loop   */
                if (status == 100)
                   status = 0;
                if (status)
                   break;
            }
        }
        /*
        ** Load values into return structure
        */
        if (lst.fields > 0)
        {
            if ((param = (ING_CONFIG*)MEreqmem( ins_ctx->metag,
                sizeof(ING_CONFIG) * lst.fields, TRUE, &status1 )) != NULL)
            {
                *ret_parms = param;
                *num_ret_parms = lst.fields;
                for (cnf = (ING_CNF_DATA *)lst.list.q_next;
                    (QUEUE *)cnf != &lst.list;
                    cnf = (ING_CNF_DATA *)cnf->list.q_next, param+=1)
                {
                    param->param_name = cnf->data[0];
                    param->param_val = cnf->data[1];
                    param->param_val_len = STlength( cnf->data[1] );
                }
            }
            for (cnf = (ING_CNF_DATA *)lst.list.q_next;
                (QUEUE *)cnf != &lst.list;
                cnf = tmp)
            {
                tmp = (ING_CNF_DATA *)cnf->list.q_next;
                QUremove( &cnf->list );
                MEfree( (PTR)cnf );
            }
        }
        else
           *num_ret_parms = 0;
        
exec sql disconnect;
    }
    else
    {
        status = sqlca.sqlcode;
    }
    MUv_semaphore( &iffconfig_sem );
    return(status);
}

    static char   iisetres[]="iisetres %s %s";

/*
** Name: iff_set_config      - replace values of configuration items
**
** Description:
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
**                          - the ouput of that command (if any. Typically,
**                            there is no output if the command has been 
**                            successful)
**
** Returns:
**    status
**    OK                      iisetres was executed for all parameters
**                            (however, the output lines must be tested for
**                             ensuring no incorrect output was generated)
**    FAIL                    an error occurred processing the command
**    IFF_ERR_PARAMNAME_LEN   parameter name length  >limit
**    IFF_ERR_PARAMVALUE_LEN  parameter value length >limit
**    (others)                sql error
**
** History:
**      07-Jul-2004 (noifr01)
**          Created.
*/

STATUS
iff_set_config(
    ING_CTX* ctx,
    ING_CONNECT* details,
    II_INT4 num_parms,
    ING_CONFIG* parameters,
    II_INT4* num_ret_parms,
    ING_CONFIG** ret_parms
)
{
    STATUS status = FAIL, status1;
    ING_INS_CTX*    ins_ctx = (ING_INS_CTX*)ctx->instance_context;
    char            addr[MAX_IFF_ADDR];
    ING_INS_DATA    lst;
    ING_CNF_DATA*   cnf;
    ING_CNF_DATA*   tmp;
    i4              i;
    ING_CONFIG*     param;
    i4              stmtlen;
exec sql begin declare section;    
    char*           address;
    char            iisetrescmd[MAX_IFF_PMNAME + MAX_IFF_PMVALUE + sizeof(iisetres)+1];
    char            buf[4000];
    int cnt1,cnt2, ordno, istatus, handl;
    int lastlineread;
exec sql end declare section;

    /* test length of input parameters */
    status=OK;
    for (i=0, param=&parameters[i]; i < num_parms; i+=1, param=&parameters[i])
    {
        if (STlength(param->param_name) > MAX_IFF_PMNAME)
        {
            status = IFF_ERR_PARAMNAME_LEN;
            break;
        }
        if (STlength(param->param_val) > MAX_IFF_PMVALUE)
        {
            status = IFF_ERR_PARAMVALUE_LEN;
            break;
        }
    }
    if (status !=OK)
        return (status);

    /*
    ** Form a direct connection target string.
    ** If the protocol is not specified use the default registry
    ** protocol and port.
    */
    if (details->port.protocol > 0)
    {
        STprintf( addr, ERx("@%s,%s,%s[%s,%s]::%s/@%s"), details->hostname,
            net_port[details->port.protocol].protocol, details->port.id,
            details->user, details->password, dbname,
            details->address.addr );
    }
    else
    {
        /*
        ** No protocol and port specified.
        ** Use default registry protocol and port.
        */
        STprintf( addr, ERx("@%s[%s,%s]::%s/@%s"), details->hostname,
            details->user, details->password, dbname, 
            details->address.addr );
    }

    MEfill( sizeof(ING_INS_DATA), 0, &lst );
    address = addr;
    QUinit( &lst.list );
    lst.fields = 0;
    
    /*
    ** Connect to target.
    */
    exec sql connect :address;
    status = sqlca.sqlcode;
    if (status!=OK)
       return (status);
       
    exec sql set lockmode session where readlock = exclusive;
    status = sqlca.sqlcode;
    if (status!=OK)
    {
        exec sql disconnect;
        return (status);
    }

    /*
    ** For each config parameter item
    **      execute "iisetres <param> <value>" through rmcmd command
    */
    for (i=0, param=&parameters[i]; i < num_parms; i+=1, param=&parameters[i])
    {
        u_i4 nsecs0, nsecscur;
        STprintf(iisetrescmd,iisetres,param->param_name,param->param_val); 
        exec sql execute procedure $ingres.launchremotecmd(rmcmd=:iisetrescmd) into :handl;
        status = sqlca.sqlcode;
        if (handl<0)
            status=FAIL;
        if (status!=OK  && status!=710)
            break;
        exec sql register dbevent $ingres.rmcmdcmdend;
        status = sqlca.sqlcode;
        if (status!=OK  && status !=710)
            break;
        exec sql commit;
        status = sqlca.sqlcode;
        if (status!=OK && status !=710)
           break;
        if ((cnf = (ING_CNF_DATA*)MEreqmem( ins_ctx->metag,
            sizeof(ING_CNF_DATA), TRUE, &status )) != NULL)
        {
            cnf->data[0] = STtalloc( ins_ctx->metag, iisetrescmd );
            cnf->data[1] = STtalloc( ins_ctx->metag, ERx(""));
            QUinsert( &cnf->list, lst.list.q_prev );
            lst.fields += 1;
        }
        lastlineread = -1;
        nsecs0= TMsecs();
        while (1) {
            exec sql get dbevent with wait=20;
            exec sql repeated select orderno, outstring into :ordno, :buf from $ingres.remotecmdoutview where handle = :handl and orderno > :lastlineread order by orderno ;
            exec sql begin;
                lastlineread++;
                status = sqlca.sqlcode;
                if ((status && status!=710) || (lastlineread!=ordno)) {
                   status=-1;
                   break;
                }
                if ((cnf = (ING_CNF_DATA*)MEreqmem( ins_ctx->metag,
                    sizeof(ING_CNF_DATA), TRUE, &status )) != NULL)
                {
                    cnf->data[0] = STtalloc( ins_ctx->metag, buf );
                    cnf->data[1] = STtalloc( ins_ctx->metag, ERx("N/A"));
                    QUinsert( &cnf->list, lst.list.q_prev );
                    lst.fields += 1;
                }
            exec sql end;
            exec sql commit;
            exec sql select status into :istatus from $ingres.remotecmdview where handle = :handl;
            status = sqlca.sqlcode;
            if (status  && status!=710)
               break;

            if (istatus == (-1)) { /* error */
                status = FAIL;
                break;
            
            }
            if (istatus == 0) { /* not (yet) taken into account by rmcmd svr */
                 nsecscur=TMsecs();
                 if ( (nsecscur - nsecs0) > 20)
                 {
                    status=FAIL;
                    break;
                 }
            
            }
            if (istatus == 2) { /* finished */
                status = OK;
                break;
            }
            exec sql commit;
            if (status && status!=710)
               break;
        }
        exec sql remove dbevent $ingres.rmcmdcmdend;
        exec sql repeated delete from $ingres.remotecmdinview  where handle =:handl;
        exec sql repeated delete from $ingres.remotecmdview    where handle =:handl;
        exec sql repeated delete from $ingres.remotecmdoutview where handle =:handl;
        exec sql commit;
        if (status !=OK  && status!=710)
            break;
    }
    
    /*
    ** Load values into return structure
    */
    if (lst.fields > 0)
    {
        if ((param = (ING_CONFIG*)MEreqmem( ins_ctx->metag,
            sizeof(ING_CONFIG) * lst.fields, TRUE, &status1 )) != NULL)
        {
            *ret_parms = param;
            *num_ret_parms = lst.fields;
            for (cnf = (ING_CNF_DATA *)lst.list.q_next;
                (QUEUE *)cnf != &lst.list;
                cnf = (ING_CNF_DATA *)cnf->list.q_next, param+=1)
            {
                param->param_name = cnf->data[0];
                param->param_val = cnf->data[1];
                param->param_val_len = STlength( cnf->data[1] );
            }
        }
        for (cnf = (ING_CNF_DATA *)lst.list.q_next;
            (QUEUE *)cnf != &lst.list;
            cnf = tmp)
        {
            tmp = (ING_CNF_DATA *)cnf->list.q_next;
            QUremove( &cnf->list );
            MEfree( (PTR)cnf );
        }
    }
    else
        *num_ret_parms = 0;
    exec sql disconnect;
    return(status);
}

