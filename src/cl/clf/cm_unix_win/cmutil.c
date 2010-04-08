/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: cmutil.c
**
** Description:
**      This module contains the NT specific functions for testing that the
**      installation environment conforms and should allow Ingres to install
**      successfully.
**
**      CMvalidhostname     Test hostname string for invalid characters.
**      CMvalidusername     Test username string for invalid characters.
**      CMvalidpathname     Test pathname string for invalid characters.
**      CMstrtest           List a validity test of character combinations.
**      CMcharmap           Lists a charmap for the current character
**                          mapping file.
**      CMgetDefCS()        Return the global reference CMdefcs for mapping
**                          the default character set.
**      CMvalidinstancecode Tests an instance code for valid characters.
**
** History:
**      25-Jun-2002 (fanra01)
**          Sir 108122
**          Created.
**      25-Mar-2004 (hweho01)
**          Modified CMvalidhostname() and CMvalidusername(), so 
**          they will be available to UNIX platforms. 
**	03-Jun-2005 (drivi01)
**	    Removing CMvalidpath function b/c it is no longer needed.
**	    LOisvalid was modified to perform a proper validity check 
**	    on a path.
**	22-Aug-2005 (drivi01)
**	    Validate first character of username, hostname or path
**	    using CMdigit.
**  17-Oct-2005 (fanra01)
**      Sir 115396
**      Add function to validate an instance code for valid characters.
*/

# include <compat.h>
# include <cm.h>
# include <st.h>

GLOBALREF i4 CMdefcs;
/*
** Name: CMvalidhostname
**
** Description:
**      Test the provided buffer for illegal characters to Ingres in a
**      hostname.
**
** Inputs:
**      hostname        pointer to the hostname
**                      assumed to be a null terminated string
**
** Outputs:
**      None
**
** Returns:
**      FALSE           if invalid
**      TRUE            if valid
*/
bool
CMvalidhostname( char* hostname )
{
    bool    valid = TRUE;
    char*   p = hostname;

    if (p && *p)
    {
        /*
        ** Test the first character is a valid hostname and can start a
        ** name.
        */
#if defined(UNIX)
        if (CMnmstart( p ))
#else
        if (CMhost( p ) && (CMnmstart( p ) || CMdigit( p )))
#endif
        {
            CMnext( p );
            while( p && *p )
            {
                /*
                ** Test each subsequent character for hostname validity.
                */
#if defined(UNIX)
                if (CMnmchar( p ))
#else
                if (CMhost( p ) && CMnmchar( p ))
#endif
                {
                    CMnext(p);
                }
                else
                {
                    valid = FALSE;
                    break;
                }
            }
        }
        else
        {
            valid = FALSE;
        }
    }
    return(valid);
}

/*
** Name: CMvalidusername
**
** Description:
**      Test the provided buffer for illegal characters to Ingres in a
**      user name.
**
** Inputs:
**      username        pointer to the username
**                      assumed to be a null terminated string
**
** Outputs:
**      None
**
** Returns:
**      FALSE           if invalid
**      TRUE            if valid
*/
bool
CMvalidusername( char* username )
{
    bool    valid = TRUE;
    char*   p = username;

    if (p && *p)
    {
        /*
        ** Test the first character is a valid username and can start a
        ** name.
        */
#if defined(UNIX)
        if (CMnmstart( p ))
#else
        if (CMuser( p ) && CMnmstart( p ))
#endif
        {
            CMnext( p );
            while( p && *p )
            {
                /*
                ** Test each subsequent character for hostname validity.
                */
#if defined(UNIX)
                if (CMnmchar( p ))
#else
                if (CMuser( p ) && CMnmchar( p ))
#endif
                {
                    CMnext(p);
                }
                else
                {
                    valid = FALSE;
                    break;
                }
            }
        }
        else
        {
            valid = FALSE;
        }
    }
    return(valid);
}

/*{
** Name: CMvalidinstancecode - Test the characters composing an instance code
**
** Description:
**  Valid instance code
**      <instance code> ::= <simple instance code> |
**                          <port specific instance code>
**      <simple instance code> ::= <character> <alpha numeric>
**      <port specific instance code> ::= <character> <alpha numeric> <subport>
**      <alphanumeric> ::=  <character> | <digit>
**      <character> ::= <latin lower case character> |
**                      <latin upper case character>
**      <latin lower case character> ::= [a-z]
**      <latin upper case character> ::= [A-Z]
**      <digit> ::= [0-9]
**      <subport> ::= [0-7]
**
** Inputs:
**      ii_install      Pointer to instance code string.
**
** Outputs:
**      None.
**
** Returns:
**      TRUE            The string value matches is a valid instance code.
**      FALSE           The string value does not match a valid instance code.
**
** History:
**      17-Oct-2005 (fanra01)
**          Created.
}*/
bool
CMvalidinstancecode( char* ii_install )
{
    bool    ret = FALSE;
    i4      len;
    i4      subport = 0;

    while(TRUE)
    {
        /*
        ** If the input parameter is invalid return error
        */
        if ((ii_install == NULL) ||
            (*ii_install == '\0'))
            break;

        switch(len = STlength( ii_install ))
        {
            case 0:
            case 1:
                /*
                ** There are too few characters in the string for a valid
                ** instance code.
                */
            default:
                /*
                ** There are too many characters in the string for a valid
                ** instance code.
                */
                break;
            case 3:
                if (!CMdigit( &ii_install[2] ))
                    break;
                if (CVan( &ii_install[2], &subport ) != OK)
                    break;
                if (subport > 7)
                    break;
            case 2:
                if (!CMalpha( &ii_install[0] ))
                    break;
                if (!(CMalpha( &ii_install[1] ) || CMdigit( &ii_install[1] )))
                    break;
                ret = TRUE;
                break;
        }
        break;
    }
    return(ret);
}

# if defined(NT_GENERIC)
/*
** Name: CMvalidpathname
**
** Description:
**      Test the provided buffer for illegal characters to Ingres in
**      path objects.
**
** Inputs:
**      path            pointer to the path
**                      assumed to be a null terminated string
**
** Outputs:
**      None
**
** Returns:
**      FALSE           if invalid
**      TRUE            if valid
*/
bool
CMvalidpathname( char* path )
{
    bool    valid = TRUE;

    char*   p = path;
	
    if (p && *p && !CMnmstart( p ) && !CMdigit( p ))
    {
	valid = FALSE;
	return (valid);
    }
    while( p && *p )
    {
        /*
        ** Test each character for path validity excluding operators and
        ** path control characters.  Operators are valid in a path but not
        ** in a path component.  Path control characters may appear in
        ** path components.
        ** e.g. ingres.26       is a valid directory name but
        **      ingres/26       is not when using this function.
        */
        if (CMpath( p ) && (CMpathctrl( p ) || !CMoper( p ) || CMdigit( p )))
        {
            CMnext(p);
        }
        else
        {
            valid = FALSE;
            break;
        }
    }
    return(valid);
}

static char attrnm[] =
{
    'u', 'l', 'a', 'd', 'w', 'p', 'c', '1', '2', 's', 'n', 'x', 'o', 'H',
    'P', 'U', '\0'
};
static char chars[256] = { 0 };
static char teststr[5] = { 0 };
i4 count = 1;

/*
** Name: CMstrtest
**
** Description:
**      This function generates a combination of two character strings and
**      attempts to validate them using the functions.
**
** Inputs:
**      None
**
**
** Outputs:
**      None
**
** Returns:
**      None
**
*/
void
CMstrtest()
{
# if defined(xDEBUG)
    i4 i;
    i4 j;

    for (i=1; i < 256; i+=1)
    {
        chars[i] = i;
    }

    for (i=1; i < 256; i+=1)
    {
        teststr[0] = chars[i];
        for (j = 255; j > 0; j-=1)
        {
            teststr[1] = chars[j];
            teststr[3] = 0;
            printf( "%06d 0x%0X 0x%0X %s\n", count,
                (unsigned char)teststr[0],
                (unsigned char)teststr[1], teststr );
            printf( "%06d %s %s\n", count, CMvalidhostname( teststr ) ? "VH":"IH", teststr );
            printf( "%06d %s %s\n", count, CMvalidusername( teststr ) ? "VU":"IU", teststr );
            printf( "%06d %s %s\n", count, CMvalidpathname( teststr ) ? "VN":"IN", teststr );
            count += 1;
        }
    }
# endif /* xDEBUG */
}

/*
** Name: CMcharmap
**
** Description:
**      This function lists the attributes for each character.
**      Attribute names are defined in the charst files.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      None
**
*/
void
CMcharmap()
{
# if defined(xDEBUG)
    i4 i;
    i4 j;
    char str[2] = { 0, 0 };
    i4 attr;

    count = 1;
    for (i=1; i < 256; i+=1, count+=1, printf("\n"))
    {
        printf ( "%06d %c 0x%02X ", count, isprint(i) ? i : i, i );
        attr = CMGETATTRTAB[i&0377];
        for (j=0; j < (BITSPERBYTE * sizeof(i4)); j+=1)
        {
            if ((attr >> j) & 1 )
            {
                printf( "%c", attrnm[j] );
            }
            else
            {
                printf( " " );
            }
        }
    }
    printf( "\n" );
# endif /* xDEBUG */
}
# endif /* NT_GENERIC */

i4 CMgetDefCS()
{
    return CMdefcs;
}
