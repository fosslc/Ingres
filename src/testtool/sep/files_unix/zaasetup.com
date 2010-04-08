$ !*****************************************************!
$ !	zaasetup.com
$ !
$ !	History
$ !
$ !
$ !	02-apr-1992 (Donj)
$ !	    Changed sepparamdb's definition to septool
$ !	    to match the tests.
$ !	30-nov-1992 (DonJ)
$ !	    Change how we aim at a client.
$ !	14-aug-1993 (DonJ)
$ !	    Change from 63p (ingres63p) to 65 (ingres)
$ !
$ !*****************************************************!
$ !
$ cho test6
$ !
$ sw_c 65 -dj -v
$ !
$ define  SEPPARAMDB      septools
$ define  SEPPARAM_TRACE  "TRUE"
$ !
$ exit
