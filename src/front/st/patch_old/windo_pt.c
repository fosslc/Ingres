/* This is a simple program to pop up a message box at the successful completion        */
/* of a patch installation. This is used by the CA-Installer.				*/
/*											*/
/* History:										*/
/*	17-may-1996 (somsa01)								*/
/*		Created.								*/

#include <stdio.h>
#include <string.h>
#include <windows.h>

main()
{
  char message[2048];

  strcpy (message, "This OpenIngres Patch has been installed!");
  MessageBox(NULL, message, "OpenIngres Patch Installation", 
             MB_ICONINFORMATION | MB_OK);
}
