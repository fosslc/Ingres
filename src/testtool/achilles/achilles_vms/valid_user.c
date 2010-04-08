/*  **************************************************************
 ** Valid_User.c
 **
 **	Look up username in SYSUAF, print owner information
 **     and return status code.
 **
 ** HISTORY
 **
 ** 20-sep-91 (DonJ)
 **	    Created.
 **
 ** 20-sep-91 (DonJ)
 **	    Added title, description and history.
 ** 21-oct-93 (Jeffr)
 **	    Added angle brackets for DEC Alpha VMS
 **
 ** **************************************************************
 */
#include <stdio.h>
#include <ssdef.h>
#include <descrip.h>
#include <uaidef.h>

main (argc, argv)
int argc;
char *argv[];
{
    struct ITEM_LIST { unsigned short  len, code;
                       long            *buf,*rtn;
                     };

    char owner_buf [33], *owner, uname [13];

    int i, ret_val, owner_len;

    struct ITEM_LIST itm_1[] = { 
                  32, UAI$_OWNER, owner_buf, &owner_len, 0, 0 };

    struct dsc$descriptor_s
           usr_name = { 12,DSC$K_DTYPE_T,DSC$K_CLASS_S, uname };

    if (argc != 2) exit(0);

    for ( i=0; argv[1][i]; i++ ) uname[i] = argv[1][i];
    for ( ; i<12; i++ ) uname[i] = ' ';
    uname[i] = '\0';

    ret_val = sys$getuai(0, 0, &usr_name, &itm_1, 0, 0, 0);

    for (i=((int)owner_buf[0])+1; owner_buf[i]; i++) owner_buf[i] = '\0';
    owner = &owner_buf[1];

    if ( ret_val==SS$_NORMAL ) printf( "%s\n",owner );
    exit(ret_val);
}
