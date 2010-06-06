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
# History:
# ========
#      Jun-6-2010 (rosan01 - Andrew Ross)
#          Initial version.

# TODO:
# Generic improvements:
# =====================
# Urgent
# ------
# - for each file in the svn revision
# - svn export it at the revision to copy it to git
# - git add it

# Minor
# -----
# - getopts for option handling
# - config file for defaults
# - add help/usage
# - consider syslog for logging

# Ingres specific improvements:
# =============================
# - better handling for content under tst: detect and ignore it (since the test cases in tst and pdfs in techpub are not being copied to github)

URL="http://code.ingres.com/ingres/main"
SVNLatest=`svn info ${URL} | grep "Last Changed Rev:" | awk '{print $4}'`
RevLog="svn_import/imported_revisions.txt"
GitLatest=`tail -1 ${RevLog}`
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
    exit 1
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

      # Grab the commit message
      svn log -r${PreviousRev}:${CurrentRev} ${URL} > ${CommitMessage}

      #echo "Applying patch"
      patch -p0 < ${PatchFile}
      
      if [ $? != 0 ]
      then
        echo "ERROR: patch failed for ${CurrentRev}"
        echo "Cannot continue. Exiting"
        exit 1
      fi

      # Add the revision number to the log
      echo "${CurrentRev}" >> ${RevLog}

      # Commit, with the same commit message
      git commit -a -F ${CommitMessage}

    done
  fi
fi

echo "Done! Successuful. Until next time you need to update."
exit 0
