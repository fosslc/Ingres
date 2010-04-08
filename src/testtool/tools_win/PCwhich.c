#include <stdio.h>
#include <windows.h>

main(argc, argv)
int argc;
char *argv[];
{
  char full_prog_name[4096];

  if (SearchPath(NULL, argv[1], ".com", sizeof(full_prog_name), 
                 full_prog_name, NULL)==0)
    if (SearchPath(NULL, argv[1], ".exe", sizeof(full_prog_name), 
                   full_prog_name, NULL)==0)
      if (SearchPath(NULL, argv[1], ".bat", sizeof(full_prog_name), 
                     full_prog_name, NULL)==0)
        if (SearchPath(NULL, argv[1], ".cmd", sizeof(full_prog_name), 
                       full_prog_name, NULL)==0)
        {
          printf ("%s", "");
          exit(1);
        } 
  printf ("%s", full_prog_name);
}
