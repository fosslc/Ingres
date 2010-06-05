#!/bin/sh

URL="http://code.ingres.com/ingres/main"
SVNLatest=`svn info http://code.ingres.com/ingres/main | grep "Last Changed Rev:" | awk '{print $4}'`
GitLatest=`tail -1 svn_import/imported_revisions.txt`
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
      echo " "
      echo "Processing revision: ${CurrentRev}"
      echo "================================"
      echo "Pulling patch: ${CurrentRev} from svn"
      PatchFile="${CurrentRev}.patch"
      CommitMessage="${CurrentRev}.message"
      # Generate a patchfile from svn
      svn diff -r${PreviousRev}:${CurrentRev} ${URL} > ${PatchFile}

      # TODO: Check if the patch affects techpub or tst

      # Grab the commit message
      svn log -r${PreviousRev}:${CurrentRev} ${URL} > ${CommitMessage}

      #echo "Applying patch"
      #patch -p0 < ${PatchFile}

      # Git add the files
      #git add -a 

      # Commit, with the same commit message
      #git commit -F ${CommitMessage}

      # Add the revision number to the log

    done
  fi
fi
