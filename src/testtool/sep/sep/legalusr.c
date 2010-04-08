/*
**    Include file
*/

#include <compat.h>
#include <cv.h>
#include <cm.h>
#include <si.h>
#include <st.h>
#include <lo.h>
#include <er.h>
#include <termdr.h>


#define legalusr_c

#include <sepdefs.h>
#include <fndefs.h>

/*
** History:
**	26-apr-1994 (donj)
**	    Created.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


bool
Legal_User(char *User_name, char *errmsg)
{
    bool                   ret_bool = TRUE ;

    /*
    ** ********************************************************* **
    ** If you're adding new usernames, add single user names as
    ** STcompare()'s; add multiple names like name0 thru name99 as
    **
    **  !legal_urange(User_name, ERx("name" ),  0, 99) &&
    **
    ** legal_urange() doesn't allow leading zeros such as name00
    ** thru name09 thru name99. To add those kinds, do the following:
    **
    **  !legal_urange(User_name, ERx("name0"),  0,  9) &&
    **  !legal_urange(User_name, ERx("name" ), 10, 99) &&
    **
    ** ********************************************************* **
    */
    if (STcompare(User_name, ERx("ctdba"   )) &&
        STcompare(User_name, ERx("ingres"  )) &&
        STcompare(User_name, ERx("notauth" )) &&
        STcompare(User_name, ERx("qatest"  )) &&
        STcompare(User_name, ERx("release" )) &&
        STcompare(User_name, ERx("syscat"  )) &&
        STcompare(User_name, ERx("super"   )) &&
        STcompare(User_name, ERx("testenv" )) &&
        STcompare(User_name, ERx("REGULAR_user")) &&
        STcompare(User_name, ERx("Regular_User")) &&
        STcompare(User_name, ERx("Delim User"  )) &&
	STcompare(User_name, ERx("stage11_inst")) &&
        !legal_urange(User_name, ERx("ingusr"),  0, 47) &&
        !legal_urange(User_name, ERx("pvusr" ),  1,  4))
    {
	STprintf(errmsg, ERx("User, %s, isn't authorized\n"), User_name);
	ret_bool = FALSE;
    }

    return ((bool)ret_bool);
}

bool
legal_urange ( char *uname, char *uname_root, i4  i_min, i4  i_max )
{
        char           *cptr1 = uname ;
        bool            ret_bool = FALSE ;
	i4             len_of_root ;
	i4             len_of_uname ;
	i4             i ;

	if ((len_of_uname = STlength(uname))
		> (len_of_root = STlength(uname_root)))
	{
	    for (i=0; i<len_of_root; i++)
		CMnext(cptr1);

	    if ((((len_of_uname - len_of_root) == 1) || (*cptr1 != '0'))
                && (CVan(cptr1,&i) == OK)
                && (( i >= i_min ) && ( i <= i_max )))
		ret_bool = TRUE;
	}
	return (ret_bool);
}
