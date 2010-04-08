#include <windows.h>

main (argc, argv)
int argc;
char **argv;
{
  int time;

  time = atoi (argv[1]);
  Sleep (time*1000);
}
