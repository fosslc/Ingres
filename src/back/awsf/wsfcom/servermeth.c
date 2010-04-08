/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <servermeth.h>
#include <erwsf.h>
#include <wsf.h>

#include <asct.h>

/*
**
**  Name: servermeth.c - Server method manager
**
**  Description:
**		Declare and give access to server functions
**
**  History:
**	5-Feb-98 (marol01)
**	    created
**      15-Mar-99 (fanra01)
**          Add initialsation tracing for function loading.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

typedef struct __SERVER_DLL 
{
	char* name;
	DDFHASHTABLE	tab;
} SERVER_DLL, *PSERVER_DLL;

static DDFHASHTABLE server_dlls = NULL;
static DDFHASHTABLE server_functions = NULL;

/*
** Name: WSFAddServerFunction() - Declare a new server function
**
** Description:
**
** Inputs:
**	DDFHASHTABLE*		: function table
**	char*				: function name
**	PTR					: function pointer
**	char*[]				: parameter list
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WSFAddServerFunction (
    DDFHASHTABLE    *tab,
    char            *name,
    PTR             entry,
    char*           params[])
{
    GSTATUS err = GSTAT_OK;
    PSERVER_FUNCTION    func = NULL;
    char* paramsName;
    u_i4 length;
    u_i4 numberOfParams = 0;
    u_i4 i;

    for (numberOfParams = 0; params[numberOfParams] != NULL; numberOfParams++);

    if (tab == NULL)
        tab = &server_functions;

    if ((*tab) == NULL)
        err = DDFmkhash(50, tab);

    if (err == GSTAT_OK)
    {
        length = STlength(name) + 1;
        err = DDFgethash((*tab), name, length, (PTR*) &func);

        if (err == GSTAT_OK && func != NULL)
            func->fct = entry;
        else
        {
            err = G_ME_REQ_MEM(0, func, SERVER_FUNCTION, 1);
            if (err == GSTAT_OK)
            {
                err = G_ME_REQ_MEM(0, func->name, char, length);
                if (err == GSTAT_OK)
                {
                    char fmt[80];

                    MECOPY_VAR_MACRO(name, length, func->name);
                    func->fct = entry;
                    func->params_size = numberOfParams;
                    err = G_ME_REQ_MEM(0, func->params, char*, numberOfParams);
                    if (err == GSTAT_OK)
                    {
                        u_i4    params_length;
                        for (i = 0; err == GSTAT_OK && i < numberOfParams; i++)
                        {
                            paramsName = params[i];
                            params_length = STlength(paramsName) + 1;
                            err = G_ME_REQ_MEM(0, func->params[i], char, params_length);
                            if (err == GSTAT_OK)
                                MECOPY_VAR_MACRO(paramsName, params_length, func->params[i]);
                        }
                    }
                    if (err == GSTAT_OK)
                        err = DDFputhash((*tab), func->name, HASH_ALL, (PTR) func);
                    STprintf( fmt, "External Func=%%s( %%%d[%%s %%]) error=%%08x",
                        numberOfParams );
                    asct_trace( ASCT_INIT | ASCT_EXEC )( ASCT_TRACE_PARAMS,
                        fmt, func->name, func->params, 0, err);
                }
            }
        }
    }
    return(err);
}

/*
** Name: WSFLoadServerFunctions() - Load a Server Function DLL
**
** Description:
**	The function must export a function called InitICEServerExtension
**	This function will return a table with the definition (name and params)
**	of every function contained in this dll.
**
** Inputs:
**	char*		: dll name
**
** Outputs:
**	PSERVER_DLL*: dll structure
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WSFLoadServerFunctions (
    char        *dllname,
    PSERVER_DLL *dll)
{
    GSTATUS err = GSTAT_OK;
    u_i4 length;

    if (server_dlls == NULL)
        err = DDFmkhash(10, &server_dlls);

    length = STlength(dllname) + 1;
    err = DDFgethash(server_dlls, dllname, HASH_ALL, (PTR*) dll);

    if (err == GSTAT_OK && *dll == NULL)
    {
        err = G_ME_REQ_MEM(0, *dll, SERVER_DLL, 1);
        if (err == GSTAT_OK)
        {
            err = G_ME_REQ_MEM(0, (*dll)->name, char, length);
            if (err == GSTAT_OK)
            {
                CL_ERR_DESC err_desc;
                STATUS status;
                PTR handle;

                MECOPY_VAR_MACRO(dllname, length, (*dll)->name);
                if (DLprepare((char *)NULL,
                        dllname,
                        NULL,
                        &handle,
                        &err_desc) != OK)
                    err = DDFStatusAlloc (E_DF0018_OPEN_DRV_ERROR);
                else
                {
                    GSTATUS (*addr) (PSERVER_DLL_METHOD*);
                    PTR server_method;

                    asct_trace( ASCT_INIT | ASCT_EXEC )(ASCT_TRACE_PARAMS,
                        "External Library=%s", ((dllname != NULL) ? dllname:"") );

                    status = DLbind(
                                handle,
                                WSF_DLL_LOAD_FCT,
                                (PTR *) &addr,
                                &err_desc);

                    asct_trace( ASCT_INIT | ASCT_EXEC )(ASCT_TRACE_PARAMS,
                        "External Entry=%s status=%08x", WSF_DLL_LOAD_FCT, status );

                    if (status != OK)
                        err = DDFStatusAlloc (status);
                    else
                    {
                        u_i4 i;

                        PSERVER_DLL_METHOD fcts = NULL;
                        err = addr(&fcts);
                        for (i = 0; err == GSTAT_OK && fcts[i].name != NULL; i++)
                        {
                            status = DLbind(
                                        handle,
                                        fcts[i].name,
                                        &server_method,
                                        &err_desc);
                            asct_trace( ASCT_INIT | ASCT_EXEC )
                                ( ASCT_TRACE_PARAMS,
                                "External Bind=%s status=%08x", fcts[i].name,
                                status );
                            if (status != OK)
                                err = DDFStatusAlloc (status);
                            else
                            {
                                err = WSFAddServerFunction (
                                        &(*dll)->tab,
                                        fcts[i].name,
                                        server_method,
                                        fcts[i].params);
                            }
                        }
                        if (err == GSTAT_OK)
                            err = DDFputhash(server_dlls, (*dll)->name, HASH_ALL, (PTR) (*dll));
                    }
                }
            }
        }
    }
    return(err);
}

/*
** Name: WSFGetServerFunction() - Retrieve the function pointer
**
** Description:
**
** Inputs:
**	char*		: dll name
**	char*		: function name
**
** Outputs:
**	PSERVER_FUNCTION*: function pointer
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WSFGetServerFunction (
	char			*dll,
	char			*name,
	PSERVER_FUNCTION *func)
{
	GSTATUS			err = GSTAT_OK;	
	u_i4			length;
	DDFHASHTABLE	tab;

	length = STlength(name) + 1;
	if (dll == NULL)
		tab = server_functions;
	else
	{
		PSERVER_DLL		server_dll;
		err = WSFLoadServerFunctions(dll, &server_dll);
		if (err == GSTAT_OK && server_dll != NULL)
			tab = server_dll->tab;
	}
	if (err == GSTAT_OK && tab != NULL)
		err = DDFgethash(tab, name, HASH_ALL, (PTR*) func);

	if (err == GSTAT_OK && *func == NULL)
		err = DDFStatusAlloc( E_WS0058_WPS_BAD_SVR_FUNC );
return(err); 
}
