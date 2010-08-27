#!/bin/sh
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#  Description:
#  ============
#    This tool is a very simple tool for automatically propping code
#    from subversion to git. Doing so allows people to choose their
#    choice of an either centralized or distributed model to work with
#    Ingres code. Particularly, git allows local branching, local commit
#    and thus lends itself well to experimental work and working offline.
#
#    This tool keeps a manifest of revisions it has successfully merged
#    and thus preserves state through comitting this file in the repository.
#
#    This tool could be modified to be generic fairly easily.
#
# History:
# ========
#      Jun-06-2010 (Andrew Ross)
#          Initial version.
#      Aug-26-2010 (Andrew Ross)
#          Updates to handle binaries better. Copy the changes over top rather
#          than apply patches.
#          Sped up the tool by only downloading files we care about.
#          Added proper support for deleted files.

# TODO:
# Generic improvements:
# =====================
# Urgent
# ------
# - Add error handling function with msg & return params
# - put the stuff we want to exclude into a file instead

# Minor
# -----
# - getopts for option handling
# - config file for defaults
# - add help/usage
# - consider syslog for logging

URL="http://code.ingres.com/ingres/main"
SVNLatest=`svn info ${URL} | grep "Last Changed Rev:" | awk '{print $4}'`
RevLog="svn_import/imported_revisions.txt"
GitLatest=`tail -1 ${RevLog}`
NOW=`date +"%h%d-%Y-%H%M"`
OriginalDir=`pwd`

echo "Now: ${NOW} svn: rev >${SVNLatest}< git: rev >${GitLatest}<"

if [ "${SVNLatest}" == "${GitLatest}" ]
then
  echo "git and svn are in sync"
else
  echo "git and svn are NOT in sync"
  if [ ${GitLatest} -gt ${SVNLatest} ]
  then
    echo "Error: Git is being reported as ahead of svn"
    exit 1
  else
    echo "Stepping through svn updates to update git"
    CurrentRev=${GitLatest}
    while [ ${CurrentRev} -lt ${SVNLatest} ]
    do
      PreviousRev=${CurrentRev}
      CurrentRev=`expr ${CurrentRev} + 1` 
      echo " "
      echo "Processing revision: ${CurrentRev}"
      echo "================================"
      echo "Pulling patch: ${CurrentRev} from svn"
      ChangeList="${CurrentRev}.txt"
      TempList="${CurrentRev}.tmp"
      CommitMessage="${CurrentRev}.message"

      # Get a list of affected files for this revision
      # we're going to update each one in turn by downloading from svn
      # then delete any files that have been deleted
      svn log -vq -r${PreviousRev}:${CurrentRev} ${URL} > ${TempList}

      # make sure we've got the list... we can't do anything without it
      if [ $? != 0 ]
      then
        echo "ERROR: pulling patch failed for ${CurrentRev}"
        echo "Cannot continue. Exiting"
        exit 1
      fi

      # Format the output so that we can process it
      cat ${TempList} | egrep '^ {3}[ADMR] ' | sort -u |\
                 sed "s/\/main\///g" | grep -v "src/tst" |\
                 grep -v "src/tools/techpub/pdf" > ${ChangeList}

      # First, process all the files that were added or modified
      cat ${ChangeList} | grep -v "^D" | awk '{print $2}' |\
      while read File
      do
        echo "    Updating: ${File}"
        TargetDir=`dirname ${File}`
        Target="${URL}/${File}"
        JustFileName=`basename ${File}`
 
        # check if the directory involved exists, if not, add it
        if [ ! -d ${TargetDir} ]
        then
          mkdir -p ${TargetDir}
        fi

        cd ${TargetDir}
        if [ $? != 0 ]
        then
          echo "ERROR: I can't get into directory: ${TargetDir}"
          echo "Cannot continue. Exiting"
          exit 1
        fi
        
        # Get the file from subversion and overwrite the local copy
        svn export -r ${CurrentRev} ${Target}
        # make sure this worked...
        if [ $? != 0 ]
        then
          echo "ERROR: pulling file failed for revision: ${CurrentRev} and file: ${Target}"
          echo "Please re-run the import for this revision and it should recover."
          echo "Cannot continue. Exiting"
          exit 1
        fi

        git add ${JustFileName}
        cd ${OriginalDir}
      done # Looping for each changed file

      # Then, delete all the files that were deleted
      # This is a local-only operation at this point so it's best to do last
      # after we know the updates worked successfully
      cat ${ChangeList} | grep "^D" |\
      while read DeletedFile
      do
        echo "    Deleting: ${DeletedFile}"
        git rm ${DeletedFile}
        #TODO: double check that it actually removed
        #      complain and exit if we didn't find it to avoid a bigger mess
      done

      # Grab the commit message
      svn log -r${PreviousRev}:${CurrentRev} ${URL} > ${CommitMessage}
      # make sure this worked...
      if [ $? != 0 ]
      then
        echo "ERROR: pulling commit message for revision: ${CurrentRev} failed"
        echo "Please re-run the import for this revision and it should recover."
        echo "Cannot continue. Exiting"
        exit 1
      fi

      # Add the revision number to the log
      echo "${CurrentRev}" >> ${RevLog}

      # Commit, with the same commit message
      git commit -F ${CommitMessage}

    done # Looping for each revision
  fi
fi

echo "Done! Successuful. Until next time you need to update."
exit 0
