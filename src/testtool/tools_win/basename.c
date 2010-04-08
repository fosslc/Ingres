#include <stdio.h>
#include <string.h>

/* Turn off POSIX warning for this file until Microsoft fixes this bug */
#pragma warning(disable: 4996)

main(argc, argv)
int argc;
char **argv;
{
  char *base, *base2;

  if (argc==1)
  {
    printf (".");
    exit (0);
  }
  base = strrchr (argv[1], '\\');
  if (argc==3)
  {
    base2 = strstr (base ? base : argv[1], argv[2]);
    if (base2)
      strset (base2, '\0');
  }
  printf ("%s", base ? base + 1 : argv[1]);
}
