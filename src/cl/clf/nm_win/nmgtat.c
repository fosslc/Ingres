/*
**  Copyright (c) 1987, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<lo.h>
# include	<st.h>
# include	"nmlocal.h"
#ifdef NT_GENERIC
# include   <nm.h>
# include   <cm.h>
# include   <me.h>
# include   <meprivate.h>
# include   <stdio.h>

#define CONFIG_FILE "config.ing"
#define CONFIG_PATH "ingres\\files\\" CONFIG_FILE

VOID NMgtUserAt (char* name, char** value);
STATUS NM_get_registry(char *name, char **value);

static void         read_config_file(void);
static SYM_VAL *    read_sym_val_list(char * filename);

STATICDEF  SYM_VAL *   Config_list = NULL;
STATICDEF  bool	       Config_init = FALSE;
#endif /* NT_GENERIC */

STATICDEF bool isOpenROAD_NM = FALSE;

/**
** Name: NMGTAT.C - Get Attribute.
**
**
** Description:
**	Return the value of an attribute which may be systemwide
**	or per-user.  The per-user attributes are searched first.
**	A Null is returned if the name is undefined.
**
**      This file contains the following external routines:
**
**	NMgtAt()	   -  get attribute.
**
** History: Log:	nmgtat.c
 * Revision 1.1  88/08/05  13:43:14  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/13  11:34:25  mikem
**      use nmlocal.h rather than "nm.h" to disambiguate the global nm.h
**	header from he local nm.h header.
**      
**      Revision 1.2  87/11/10  14:44:42  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.5  87/07/27  11:06:08  mikem
**      Updated to meet jupiter coding standards.
** 
**	Revision 1.2  86/06/25  16:24:12  perform
**	Need LO.h due to extern in nm.h (for NM cacheing).
** 
**	Revision 30.2  85/09/17  12:00:56  roger
**	Should not be of type VOID (see concomitant change
**	to INbackend() - use return value to detect improper
**	installation - this is implementation-specific, but...)
**		
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards. 
**	19-jan-1989 (greg)
**	    Don't call NMgtIngAt for II_SYSTEM.
**	    A hack but better than infinite looping.
**	16-feb-1990 (greg)
**	    NMgtAt is a VOID not STATUS
**      14-Oct-1994 (nanpr01)
**          Removed #include "nmerr.h".  Bug # 63157
**	9-dec-1996 (donc)
**	    Hand integrate change from OpenROAD that allows for
**	    lot's of things being in CONFIG.ING
**	16-dec-1996 (reijo01)
**	    Convert TERM_INGRES to generic system terminal. Only for Jasmine
**	    product.
**	07-apr-1997 (canor01)
**	    Add include of <me.h>.
**      03-Jul-98 (fanra01)
**          Add function NMgtUserAt to replace macro definition on NT as
**          getenv does not retrieve the environment from a DLL.
**	09-Sep-1998 (canor01)
**	    Check registry for SystemLocationVariable if it can't be found
**	    in the local environment.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	07-feb-2001 (somsa01)
**	    Correted compiler warnings.
**	15-mar-2001 (mcgem01)
**	    Add a function to set an environment variable in the 
**	    registry and externalize the function to read an environment
**	    variable from the registry.
**	07-feb-2001 (somsa01)
**	    Correted compiler warnings.
**	14-mar-2003 (somsa01)
**	    Added NMgtTLSCleanup(), used to clean up NM's TLS allocations.
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */



/*{
** Name: NMgtAt - Get attribute.
**
** Description:
**    Return the value of an attribute which may be systemwide
**    or per-user.  The per-user attributes are searched first.
**    II_SYSTEM will only be searched for per-user
**
**    A Null is returned if the name is undefined.
**
** Inputs:
**	name				    attribute to get
**
** Output:
**	value				    On success value of the attribute.
**
**      Returns:
**          VOID
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	10-oct-1996 (canor01)
**	    Allow generic system parameter overrides.
**	16-dec-1996 (reijo01)
**	    Convert TERM_INGRES to generic system terminal. Only for Jasmine
**	    product.
*/
VOID
NMgtAt(char *name, char **value)
{
    char newname[MAXLINE];
#ifdef NT_GENERIC
    VOID NMgtAt_or();
#endif

    if (!isOpenROAD_NM)
    {
        if ( MEcmp( name, "II", 2 ) == OK )
            STpolycat( 2, SystemVarPrefix, name+2, newname );
        else
            STcopy( name, newname );

        (VOID)NMgtUserAt(newname, value);

        if ( *value == (char *)NULL )
        {
            if ( STcompare( SystemLocationVariable, newname ) )
            {
                if (NMgtIngAt(newname, value))
                {
    	            *value = (char *)NULL;
                }
            }
            else
            {
                /* OK to look for II_SYSTEM in registry */
	        NM_get_registry(newname, value);
            }
        }
    }
    else /* Use OpenROAD/NT style NMloc */
    {
#ifdef NT_GENERIC
        NMgtAt_or(name, value); /* This entrpoint only defined under NT */
#else
	return; 	        /* under UNIX there is no way to get here */
#endif
    }
}

#ifdef NT_GENERIC 

SYM_VAL *
NM_get_sym_val_list()
{
    if (!Config_init)
    {
	read_config_file();
	Config_init = TRUE;
    }
    return Config_list;
}

VOID
NMgtAt_or(
char *  name,
char ** ret_val)
{
    char *      value_ptr;
    SYM_VAL *   psv;

    if (!Config_init)
    {
	   read_config_file();
	   Config_init = TRUE;
    }

    value_ptr = NULL;
    for (psv = Config_list; psv != NULL; psv = psv->next)
    {
	   if (strcmp(name, psv->symbol) == 0)
	   {
	      value_ptr = psv->value;
	      break;
	   }
    }

    if(value_ptr == NULL)
    	value_ptr = getenv(name);

    *ret_val = value_ptr;
}

/*
**  read_config_file
**
**  Reads the configuration file and creates a linked list of SYM_VALs.
**
**  The configuration file is
**
**      1)  %II_CONFIG%\config.ing              if II_CONFIG is defined.
**      2)  %II_SYSTEM%\ingres\files\config.ing if II_SYSTEM is defined.
**      3)  \ingres\files\config.ing            if neither is defined.
*/


static void
read_config_file()
{
    i4		len;
    char	*p;
    char	filename[MAX_LOC];

    *filename = EOS;

    if ((p = getenv(II_CONFIG)) != NULL && (*p != EOS))
    {
        len = (i4)strlen(p);
        if (len + sizeof(CONFIG_FILE) < sizeof(filename))
        {
            strcpy(filename, p);
            if (filename[len - 1] != SLASH)
                filename[len++] = SLASH;
            strcpy(&filename[len], CONFIG_FILE);
        }
    }
    else
    {
        if ((p = getenv(II_SYSTEM)) != NULL && (*p != EOS))
        {
            len = (i4)strlen(p);
            if (len + sizeof(CONFIG_PATH) < sizeof(filename))
            {
                strcpy(filename, p);
                if (filename[len - 1] != SLASH)
                    filename[len++] = SLASH;
                strcpy(&filename[len], CONFIG_PATH);
            }
        }
    }
    if (*filename == EOS)
        strcpy(filename, "\\" CONFIG_PATH);

    Config_list = read_sym_val_list(filename);
}

/*
**  read_sym_val_list
**
**  Reads symbol-value list from a file.  Allocates memory as necessary
**  and creates a linked list of SYM_VAL structures;
**
**  Each line in the configuration has the form:
**
**      symbol = value
**
**  The symbol is translated to upper case before storing it.
**
**  This is a separate routine in case we want to maintain more than
**  one such list in the future.
**
**  Note: the code assumes that no line in the configuration is longer
**  than (MAX_CONFIG_LINE - 1) characters.  It will not work correctly
**  with longer lines.
*/


static SYM_VAL *
read_sym_val_list(
char *      filename)   /* Full file name */
{
    i4          sym_len;
    i4          val_len;
    i4          len;
    char *      p;
    char *      sym;
    char *      val;
    bool        done;
    FILE *      fp;
    SYM_VAL *   first;
    SYM_VAL *   last;
    SYM_VAL *   svp;
    char        buf[MAX_CONFIG_LINE];

    first = NULL;

    if ((fp = fopen(filename, "r")) == NULL)
        return(NULL);

    for (done = FALSE; !done; )
    {
        if (fgets(buf, MAX_CONFIG_LINE, fp) == NULL)
        {
            done = TRUE;
            break;
        }

	if(buf[0] == '#')	/* comment line.  ignore */
	    continue;

        /*
        **  Parse line to find symbol and value.  Trim leading and
        **  trailing whitespace.
        */

        for (p = buf; CMwhite(p); p++)
            ;
        sym = p;

	if(!(p = strchr(p, '=')))
            continue;	 /* No '='; ignore this line */

        *p++ = EOS;

	if(!(val = getenv(sym)))
	{
	    len = (i4)strlen(p);
	    if (p[len-1] == '\n')
		p[len-1] = EOS;	/* trim trailing newline */
	    while (CMwhite(p)) /* Skip leading white space */
		p++;
	    if (*p == EOS)      /* If no value, ignore line */
		continue;
	    val = p;
	}

        sym_len = (i4)STtrmwhite(sym);
        val_len = (i4)STtrmwhite(val);

        if (sym_len == 0 || val_len == 0)
            continue;

        len = sizeof(SYM_VAL) + sym_len + val_len + 2;
        if(!(p = (char *) MEreqmem(0, len, FALSE, NULL)))
	    return NULL;

        svp = (SYM_VAL *) p;
        p += sizeof(SYM_VAL);
        svp->symbol = p;
        p += sym_len + 1;
        svp->value = p;

        strcpy(svp->symbol, sym);
        strcpy(svp->value, val);
        svp->next = NULL;

        if (first == NULL)
        {
            first = last = svp;
        }
        else
        {
            last->next = svp;
            last = svp;
        }
    }
    fclose(fp);
    return(first);
}

VOID
NMpinit()
{
	/* Flag that one is to do an OpenROAD-style Name lookup */
	isOpenROAD_NM = TRUE;
}

# ifdef OS_THREADS_USED
static ME_TLS_KEY       NMenvkey = 0;
static ME_TLS_KEY       NMregkey = 0;
static char             staticbuf[256];
# endif

/*{
** Name: NMgtUserAt - Get user attribute.
**
** Description:
**    Return the value of an attribute which may be defined in the users
**    environment.
**
**    A Null is returned if the name is undefined.
**
** Inputs:
**	name				    attribute to get
**
** Output:
**	value				    On success value of the attribute.
**
**      Returns:
**          VOID
**
** History:
**      03-Jul-98 (fanra01)
**          Created to replace macro definition.
*/
VOID
NMgtUserAt (char* name, char** value)
{
    STATUS  local_status;
    char    envbuffer[MAXLINE];

    struct  envlist
    {
        struct envlist* next;
        char*           name;
        char*           value;
    };

    struct envlist*     buffer;
    struct envlist*     tmp;

    if (GetEnvironmentVariable (name, envbuffer, MAXLINE) == 0)
    {
        *value = NULL;
    }
    else
    {
        if ( NMenvkey == 0 )
        {
            ME_tls_createkey( &NMenvkey, &local_status );
            ME_tls_set( NMenvkey, NULL, &local_status );
        }
        if ( NMenvkey == 0 )
        {
            /* not linked with threading library */
            NMenvkey = -1;
        }
        if ( NMenvkey != -1 )
        {
            ME_tls_get( NMenvkey, (PTR *)&buffer, &local_status );
            for (tmp = buffer;(buffer != NULL) && (tmp != NULL);
                 tmp = tmp->next)
            {
                if (!STbcompare (name, 0, tmp->name, 0, TRUE))
                {
                    /*
                    ** Update it just in case
                    */
                    MEfree (tmp->value);
                    tmp->value = STalloc (envbuffer);
                    break;
                }
            }
            if (tmp == NULL)
            {
                tmp = (struct envlist*)MEreqmem(0, sizeof(struct envlist),
                                                TRUE, NULL );
                tmp->name = STalloc (name);
                tmp->value = STalloc (envbuffer);
                if (buffer != NULL)
                {
                    tmp->next = buffer;
                }
                buffer = tmp;
                ME_tls_set( NMenvkey, (PTR)buffer, &local_status );
            }
            *value = tmp->value;
        }
        else
        {
            *value = STalloc (envbuffer);
        }
    }
}

STATUS
NM_get_registry( char *name, char **value )
{
    STATUS status;
    char *syskey = "System\\CurrentControlSet\\Control\\Session Manager\\Environment";
    char *userkey = "Environment";
    HKEY queryKey;
    char *buffer;

    if ( NMregkey == 0 )
    {
        ME_tls_createkey( &NMregkey, &status );
        ME_tls_set( NMregkey, NULL, &status );
    }
    if ( NMregkey == 0 )
    {
        /* not linked with threading library */
        NMregkey = -1;
    }
    if ( NMregkey != -1 )
    {
        ME_tls_get( NMregkey, (PTR *)&buffer, &status );
        if ( buffer == NULL )
            buffer = MEreqmem( 0, sizeof(staticbuf), TRUE, NULL );
    }
    else
    {
        buffer = staticbuf;
    }

    status = FAIL;

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       syskey,
                       0, KEY_QUERY_VALUE, &queryKey ) == ERROR_SUCCESS )
    {
        DWORD dwType;
        DWORD sizeData = sizeof(staticbuf); 
        if ( RegQueryValueEx( queryKey, 
                              name, NULL, 
                              &dwType, buffer, &sizeData ) == ERROR_SUCCESS )
            status = OK;
        RegCloseKey( queryKey );
    }
    
    if ( status == OK )
        *value = buffer;
    else if ( RegOpenKeyEx( HKEY_CURRENT_USER,
                       userkey,
                       0, KEY_QUERY_VALUE, &queryKey ) == ERROR_SUCCESS )
    {
        DWORD dwType;
        DWORD sizeData = sizeof(staticbuf);
        if ( RegQueryValueEx( queryKey,
                              name, NULL,
                              &dwType, buffer, &sizeData ) == ERROR_SUCCESS )
            status = OK;
        RegCloseKey( queryKey );
    }

    if ( status == OK )
        *value = buffer;

    return (status);
}

/*{
** Name: NM_set_registry - Set user attribute in the registry.
**
** Description:
**    Set the value of an attribute in the users environment.
**
**
** Inputs:
**	name				    attribute to set
**	value				    value of the attribute.
**
**      Returns:
**          OK - Success
**	    FAIL
**
** History:
**      14-Mar-2001 (mcgem01)
**          Created.
*/
STATUS
NM_set_registry (char* name, char* value)
{
    STATUS status = FAIL;
    char *syskey = "System\\CurrentControlSet\\Control\\Session Manager\\Environment";
    char *userkey = "Environment";
    HKEY queryKey;
	
    if ( RegOpenKeyEx( HKEY_CURRENT_USER, userkey,
		0, KEY_ALL_ACCESS, &queryKey ) == ERROR_SUCCESS )
    {
		if (RegSetValueEx (queryKey, 
			name, (DWORD)0, REG_SZ, value, (DWORD)strlen(value)+1) == ERROR_SUCCESS)
			status = OK;
		RegCloseKey (queryKey);
    }
    SendMessage( HWND_BROADCAST, WM_SETTINGCHANGE,0L, (LPARAM)"Environment");
    return (status);  
}

/*{
**  Name: NMgtTLSCleanup
**
**  Description:
**	Cleans up thread-local storage for NMgt module.
**
**  Inputs:
**	none
**
**      Returns:
**          VOID
**
**  History:
**	14-mar-2003 (somsa01)
**	    Created.
*/
VOID
NMgtTLSCleanup()
{
    STATUS	status;

    if (NMenvkey > 0)
    {
	struct  envlist
	{
	    struct envlist	*next;
	    char		*name;
	    char		*value;
	};
	struct envlist	*buffer, *tmp;

	ME_tls_get(NMenvkey, (PTR *)&buffer, &status);
	if (buffer != NULL)
	{
	    MEfree(buffer->name);
	    MEfree(buffer->value);
	    buffer = buffer->next;
	    while (buffer != NULL)
	    {
		tmp = buffer;
		buffer = tmp->next;
		MEfree(tmp->name);
		MEfree(tmp->value);
		MEfree((PTR)tmp);
	    }
	}
    }
}

#endif /* NT_GENERIC */
