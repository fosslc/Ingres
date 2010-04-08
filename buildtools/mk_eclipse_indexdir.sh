#!/bin/bash
## Creates an include directory and symlinks all headers for the Eclipse indexer
##
##    24-Dec-2008 (troal01)
##         created.


if [ "$ING_ROOT" == "" ] ; then
  echo "Please set your ING_ROOT variable."
  exit 1
fi

#Find all header files
HEADERS=`find $ING_ROOT/src -name *.h`

#Remove what's already there
rm -rf "$ING_ROOT/include/"
mkdir "$ING_ROOT/include/"

#Create all the links
for F in $HEADERS
do
  FNAME=`echo "$F" | awk '{ sub(/^.+\//, ""); print }'`
  if [ -f "$ING_ROOT/include/$FNAME" ] ; then
    echo "Skipping $F, already linked."
    continue
  fi
  echo "Creating link for $FNAME."
  ln -s "$F" "$ING_ROOT/include/$FNAME"
done

#Now all that has to be done is to go to
#Project Properties->C/C++ General->Paths and Symbols->GNU C
#Adding the include directory should make the indexer work properly
