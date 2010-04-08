#include <stdio.h>
#ifndef __NEW_STARLET
#define __NEW_STARLET
#endif
#include <starlet.h>
#include <efndef.h>
#include <iodef.h>
#include <iosbdef.h>
#include <ssdef.h>
#include <descrip.h>
#define FILE_BUFSIZE	128
#define MBOX_BUFSIZE	256

main (argc, argv)
int argc;
char *argv[];
{
    unsigned long rval;

    char buf[1024], infile[FILE_BUFSIZE], outfile[FILE_BUFSIZE];

    FILE *in, *out;

    unsigned short mboxchan;
    IOSB iosb;
    struct dsc$descriptor_s mboxname = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0};

    /* printf("COPIER: Argv[1]=<%s>.\n",argv[1]); */

    mboxname.dsc$a_pointer = argv[1];
    mboxname.dsc$w_length  = strlen(argv[1]);

    /* printf("COPIER: Assigning mbox.\n"); */
    if ((rval = sys$assign(&mboxname, &mboxchan, 0, 0)) != SS$_NORMAL)
	lib$signal(rval);

    printf("COPIER: Starting While loop.\n");
    while (1)
    {
	if ((rval = sys$qiow(EFN$C_ENF, mboxchan, IO$_READVBLK, &iosb, 0, 0, buf,
			     MBOX_BUFSIZE, 0, 0, 0, 0)) != SS$_NORMAL)
	{
	    lib$signal(rval);
	}
	if (!strcmp(buf, "%%END"))
	{
	    exit(1);
	}
	/* printf("COPIER: buf=<%s>.\n", buf); */

	sscanf(buf, "%s %s", infile, outfile);
	if ((in = fopen(infile, "r")) == NULL)
	{
	    printf("COPIER: Can't open input file %s.\n", infile);
	    continue;
	}
	if ((out = fopen(outfile, "a")) == NULL)
	{
	    printf("COPIER: Can't open output file %s.\n", outfile);
	    continue;
	}
	while (fgets(buf, sizeof(buf), in) != NULL)
	    fputs(buf, out);

	fclose(in);
	fclose(out);
	delete(infile);
    }
}
