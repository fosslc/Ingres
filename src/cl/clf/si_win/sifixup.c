/*
**	Copyright (c) 1994 Ingres Corporation
*/
# include	<stdio.h>
# include	<io.h>
# include	<errno.h>
# include	<process.h>

/* SIfixup
**
**	File pointers that call back into the *.EXE's libcmt.lib file I/O functions.
**
**
**	History
**
**	06-oct-94 (leighb)
**		Created.
*/

#ifdef	THREAD_LOC_STOR
__declspec( thread )	int 	(*lpfn_fclose)()  	=  fclose;
__declspec( thread )	FILE *	(*lpfn_fdopen)()  	=  fdopen;
__declspec( thread )	int 	(*lpfn_fflush)()  	=  fflush;
__declspec( thread )	int 	(*lpfn__commit)()  	=  _commit;
__declspec( thread )	char *	(*lpfn_fgets)()  	=  fgets;
__declspec( thread )	int 	(*lpfn_fileno)()  	=  fileno;
__declspec( thread )	int 	(*lpfn__fileno)()  	=  _fileno;
__declspec( thread )	FILE *	(*lpfn_fopen)()  	=  fopen;
__declspec( thread )	int 	(*lpfn_fputc)()  	=  fputc;
__declspec( thread )	int 	(*lpfn_fputs)()  	=  fputs;
__declspec( thread )	size_t	(*lpfn_fread)()  	=  fread;
__declspec( thread )	FILE *	(*lpfn_freopen)()  	=  freopen;
__declspec( thread )	int 	(*lpfn_fseek)()  	=  fseek;
__declspec( thread )	long	(*lpfn_ftell)()  	=  ftell;
__declspec( thread )	size_t	(*lpfn_fwrite)()  	=  fwrite;
__declspec( thread )	int 	(*lpfn_getc)()  	=  getc;
__declspec( thread )	int 	(*lpfn_putc)()  	=  putc;
__declspec( thread )	void	(*lpfn_setbuf)()  	=  setbuf;
__declspec( thread )	int 	(*lpfn_ungetc)()  	=  ungetc;

__declspec( thread )	int		(*lpfn_dup)()		=  dup;
__declspec( thread )	int		(*lpfn_dup2)()		=  dup2;
__declspec( thread )	int		(*lpfn_isatty)()	=  isatty;
__declspec( thread )	char *	(*lpfn_mktemp)()	=  mktemp;

__declspec( thread )	void	(*lpfn_exit)()		=  exit;

__declspec( thread )	int * 	(*lpfn_errno)()		=  _errno;
#endif

void
SIsetupFixups(
int 	(*p_fclose)(),
FILE *	(*p_fdopen)(),
int 	(*p_fflush)(),
int 	(*p__commit)(),
char *	(*p_fgets)(),
int 	(*p_fileno)(),
int 	(*p__fileno)(),
FILE *	(*p_fopen)(),
int 	(*p_fputc)(),
int 	(*p_fputs)(),
size_t	(*p_fread)(),
FILE *	(*p_freopen)(),
int 	(*p_fseek)(),
long	(*p_ftell)(),
size_t	(*p_fwrite)(),
int 	(*p_getc)(),
int 	(*p_putc)(),
void	(*p_setbuf)(),
int 	(*p_ungetc)(),
int		(*p_dup)(),
int		(*p_dup2)(),
int		(*p_isatty)(),
char *	(*p_mktemp)(),
void	(*p_exit)(),
int * 	(*p_errno)())
{
#ifdef	THREAD_LOC_STOR
	lpfn_fclose  	=  p_fclose;
	lpfn_fdopen  	=  p_fdopen;
	lpfn_fflush  	=  p_fflush;
	lpfn__commit  	=  p__commit;
	lpfn_fgets  	=  p_fgets;
	lpfn_fileno  	=  p_fileno;
	lpfn__fileno  	=  p__fileno;
	lpfn_fopen  	=  p_fopen;
	lpfn_fputc  	=  p_fputc;
	lpfn_fputs  	=  p_fputs;
	lpfn_fread  	=  p_fread;
	lpfn_freopen  	=  p_freopen;
	lpfn_fseek  	=  p_fseek;
	lpfn_ftell  	=  p_ftell;
	lpfn_fwrite  	=  p_fwrite;
	lpfn_getc   	=  p_getc;
	lpfn_putc   	=  p_putc;
	lpfn_setbuf  	=  p_setbuf;
	lpfn_ungetc  	=  p_ungetc;
	lpfn_dup		=  p_dup;
	lpfn_dup2		=  p_dup2;
	lpfn_isatty 	=  p_isatty;
	lpfn_mktemp 	=  p_mktemp;
	lpfn_exit		=  p_exit;
	lpfn_errno		=  p_errno;
#endif
}
