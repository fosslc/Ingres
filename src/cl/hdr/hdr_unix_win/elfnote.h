/*
**	Copyright (c) 2004 Ingres Corporation
*/
#ifndef _SYS_ELFNOTE_H
#define _SYS_ELFNOTE_H

typedef struct {
	Elf32_Word namesz;
	Elf32_Word descsz;
	Elf32_Word type;
	char name[8];
} Elf32_Note;

#define NT_PRSTATUS	1	
#define NT_PRFPREG	2	
#define NT_PRPSINFO	3	

#endif /* _SYS_ELFNOTE_H */
