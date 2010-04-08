$!
$! INGRES build file for front!misc!xmlimp
$!
$! History:
$!	09-oct-2003 (schka70/abbjo03)
$!	    Created.
$!
$ set noon
$!
$! For some crazy reason, CXX doesn't want to understand includes like
$! #include <util/foo.hpp> unless the /INCLUDE path to .../util is given
$! in unix-like form!  Even though xerces itself seems to compile OK.
$! Not real sure what is going on here, but whatever...
$! Define a CXX_INCLUDE for the compile script.
$!
$ CXX_INCLUDE = "../../../../ice/" + f$trnlnm("front_vers") + "/src/sax"
$!
$ compile -o SAXIMPORT.CPP
$ compile SAXIMPORTHANDLERS.CPP
$ compile SAXINGRES.CPP
$
$ del/sym CXX_INCLUDE
