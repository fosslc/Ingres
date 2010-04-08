/*
**  Name: PCread
**
**  Reads a line of a given file, at a given line number.
**
**  Input:
**	argv[1] = filename
**	argv[2] = line number
**
**  Returns:
**	0 = successful
**	1 = successful, line read just contains a newline or is blank
**	2 = error
**
**  History:
**	??-???-?? (somsa01)
**	    Created.
**	15-oct-1998 (somsa01)
**	    Added returning an error code if a line read is blank.
*/

#include <stdio.h>
#include <string.h>

main(argc, argv)
int argc;
char *argv[];
{
  FILE *fptr;
  int count, i;
  char line[4096], *status;

  count = atoi(argv[2]);
  if (count==0)
    exit(2);

  fptr = fopen(argv[1], "r");

  for (i=1; i<=count; i++)
    status = fgets(line, sizeof(line)-1, fptr);

  fclose(fptr);

  if (status != NULL)
  {
    printf ("%s", line);
    if ((!strcmp(line, "\n")) || (!strcmp(line, "")))
	exit(1);
    else
	exit(0);
  }
  else
    exit(2);
}
