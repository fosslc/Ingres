:
#
# Copyright (c) 2004 Ingres Corporation
#
# {o >o
#  \ -) "Run specified command for each single argument read from stdin,
#        substituting the argument for `@' in command string."
#
# e.g.,
#	each touch < list-of-filenames
#	each "cd @; echo @; ls" < list-of-directory-names
#
# a lot simpler than xargs, and doesn't dump core!
#
## History
##	16-feb-1993 (dianeh)
##		Change first line to colon (:); added History block.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

template=`echo "$*" | sed 's/"/\\\"/g'`

awk '
BEGIN {
	template = "'"$template"'";
}

{
	subject = template;
	command = "";
	while (i = index(subject, "@")) {
		command = command substr(subject, 1, i - 1) $1;
		subject = substr(subject, i + 1);
	}

	if (subject == template)
		command = subject " " $1
	else
		command = command subject

	print command
}' | /bin/sh
