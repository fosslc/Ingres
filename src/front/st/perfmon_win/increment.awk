#
#  Name: increment.awk
#
#  Description:
#	Quickie program which takes an "oipfctrs.h"-like header file (without
#	comments and blank lines) and properly updates the define values for
#	each entry. thus, for adding more counter defines, append them to
#	the end of the file and run this awk script against it as such:
#
# awk -f increment.awk -v x=<starting number> <header file> > <new header file>
#
#  History:
#	23-aug-1999 (somsa01)
#	    Created.
#
BEGIN { }
{
    a[NR] = $1 " " $2
}
END {
    for (i = 1; i <= NR; i++)
    {
	printf ("%-60s %d\n", a[i], x);
	x = x+2;
    }
}
