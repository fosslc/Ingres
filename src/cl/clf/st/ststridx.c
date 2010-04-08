/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<st.h>

/*
** Name: STstrindex
**
** Description:
**      Returns a pointer to the first occurrence of a string in another
**      string, or NULL if not found. If the parameter len is is > 0. search
**      only the first len bytes of str. If len is 0, str must be NULL
**      terminated. The paramter findStr must be NULL terminated in either
**      case. If the parameter nc is TRUE, the comparison is not case sensitive. 
**
** Inputs:
**      str             string to search
**      findStr         NULL terminated string to find in str
**      len             if > 0, search only the first len chars of str
**      nc              if TRUE, perform a case insensitive search
**
** Outputs:
**      none
**
** Returns:
**      pointer to the start of the findStr in str, or NULL if not found.
**      If str is NULL, returns NULL. findStr is NULL, returns str.
**
** History:
**      06-jan-1997 (wilan06)
**          Created
**      21-apr-1999 (hanch04)
**          replace i4 with size_t
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

char*
STstrindex (
            const char*       str,
            const char*       findStr,
            size_t         len,
            i4         nc)
{
    char*       ret = NULL;
    const char*       sp = str;
    const char*       s1;
    const char*       s2;
    size_t         l;
    
    if (findStr && str)
    {
        if (len <= 0)
        {
            len = STlength (str);
        }
        while (len)
        {
            /*
            ** search forward while the chars are the same and not past the
            ** end
            */
            l = len;
            s1 = sp;
            s2 = findStr;
            if (nc)
            {
                while (l && *s2 && !CMcmpnocase(s1,s2))
                {
                    CMnext (s1);
                    CMnext (s2);
                    l--;
                }
            }
            else
            {
                while (l && *s2 && !CMcmpcase(s1,s2))
                {
                    CMnext (s1);
                    CMnext (s2);
                    l--;
                }
            }
            /* If we reached the end of the search string, we have a match */
            if (!*s2)
            {
                ret = (char *)sp;
                break;
            }
            CMnext (sp);
            len--;
        }
    }
    else
    {
        ret = (char *)str;
    }
    return ret;
}
