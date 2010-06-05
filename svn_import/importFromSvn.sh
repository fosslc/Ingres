#!/bin/sh

URL="http://code.ingres.com/ingres/main"
SVNLatest=`svn info http://code.ingres.com/ingres/main | grep "Last Changed Rev:" | awk '{print $4}'`
GitLatest=`tail -1 imported_revisions.txt`
NOW=`date +"%h%d-%Y-%H%M"`

echo "Now: ${NOW} svn: rev >${SVNLatest}< git: rev >${GitLatest}<"

if [ "${SVNLatest}" == "${GitLatest}" ]
then
  echo "git and svn are in sync"
else
  echo "git and svn are NOT in sync"
  if [ ${GitLatest} -gt ${SVNLatest} ]
  then
    echo "Error: Git is being reported as ahead of svn"
  else
    echo "Stepping through svn updates to update git"
    CurrentRev=${GitLatest}
    while [ ${CurrentRev} -le ${SVNLatest} ]
    do
      PreviousRev=${CurrentRev}
      CurrentRev=`expr ${CurrentRev} + 1` 
      echo "Applying patch for ${CurrentRev}"
      echo "================================"
      echo "Pulling patch from svn"
      svn diff -r${PreviousRev}:${CurrentRev} ${URL} > ${CurrentRev}.diff
      echo "Applying patch"
      # Apply, check return code
      # Grab the commit message
      # Git add the files
      # Commit, with the same commit message
    done
  fi
fi
