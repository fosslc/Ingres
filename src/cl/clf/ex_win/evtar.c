/*
** Copyright (c) 2010 Ingres Corporation
**
** EVtar - BASIC TAR functionality on Windows
**
**    Add all files in the specified directory
**    (including all sub-directories) into a TAR
**    ball, or extracts the files/directories in
**    a TAR ball.
**
** Description:
**    A TAR archive is a collection of 512 byte (page)
**    records that concatenate the contents of
**    files into a single file (TARBALL).
**
**    Each file added to the TARBALL is preceded by
**    a Header record (512 bytes), the contents of the
**    file are then inserted in the TARBALL, with the
**    contents being padded out to fill a TARBALL page.
**
**    The Header record contains details on the file
**    (type file/directory), size in bytes, etc. NB,
**    A DIRECTORY will have a header record, but the
**    size is 0, so no content page used.
**
**    The end of the TARBALL is signalled by two
**    (all) zero pages.
**    If the following are to be stored
**            file1   712 bytes
**            file2     0 bytes
**            file3  1986 bytes
**
**    The TAR file will be arranged
**
**    Page   Byte        Description
**       0      0     file1 Header details
**       1    512     Bytes   1-512 of File1
**       2   1024     Bytes 513-712 of File1 (0 filled to 1224 - 1535)
**       3   1536     file2 Header details
**       4   2048     file3 Header details
**       5   2560     Bytes    1- 512 of File3
**       6   3072     Bytes  513-1024 of File3
**       7   3584     Bytes 1025-1536 of File3
**       8   4096     Bytes 1537-1986 of File3
**       9   4608     All 0 (First 0 end of archive record)
**      10   5120     All 0 (Second 0 end of archive record)
**
**
**    TARBALL produced this way can by viewed on WINDOWS using
**    WINZIP (and on UNIX/VMS).
**
**    See below for details of the TAR header record.
**
** History:
**    27-Jan-2010 (horda03) Bug 121811
**       Created,
*/  
#include <compat.h>
#include <cv.h>
#include <di.h>
#include <ex.h>
#include <lo.h>
#include <me.h>
#include <si.h>
#include <st.h>
#include <tr.h>

#include <errno.h>

#include <evtar.h>


#define TAR_PAGE_SIZE       512 /* TAR ball page size */
#define TAR_TRAILER_RECORDS   2 /* Number of TAR ball trailer
                                ** records to write.
                                */
#define TAR_BUFFERED_PAGES   64 /* Number of TAR pages that can
                                ** be buffered for 1 I/O operation
                                */

/* TAR ball header detail sizes */

#define TAR_NAME  100	/* Size of a file name */
#define PROT_SIZE   8	/* Size of the Protection details */
#define GID_SIZE    8   /* Size of the GID details */
#define UID_SIZE    8	/* Size of the UID details */
#define COUNT_SIZE  12  /* Size of File size details */
#define TIME_SIZE   12  /* Size of File creation time details */
#define CHKSUM_SIZE 8   /* Size of Checksum details */

#define TAR_NULL    0
#define TAR_IS_DIR  1
#define TAR_IS_FILE 2

typedef struct
{
   char filenm [TAR_NAME];     /* Name of the file (including path in UNIX format) */
   char prot   [PROT_SIZE];    /* 7 digit Octal value for file protection characteristics
                               ** and a EOS
                               */
   char uid    [UID_SIZE];     /* 7 digit Octal value of UID + EOS - will be 0 */
   char gid    [GID_SIZE];     /* 7 digit Octal value of GID + EOS - will be 0 */
   char count  [COUNT_SIZE];   /* 11 digit Octal value of file size + EOS */
   char time   [TIME_SIZE];    /* 11 digit Octal value of file creation time + EOS */
   char chksum [CHKSUM_SIZE];  /* 6 digit Octal value for checksum of header details + EOS + ' ' */
   char linkcnt;               /* Not used will be '\0' */
   char linknm [TAR_NAME];     /* Not used will be '\0' */
   char pad    [255];          /* Padding to make record 512 bytes */
}
TAR_HDR;

static i8 bytes_written = 0;
static i8 bytes_read = 0;
static char tar_buf [TAR_BUFFERED_PAGES * TAR_PAGE_SIZE];

char out_buf [1024];
static i4
evset_print( PTR arg, i4 len, char *buf )
{
#ifdef TO_STDOUT

   printf( "%*s\n", len, buf );

#else

   TRdisplay( "%#s\n", len, buf );

#endif

   return OK;
}

static void
print_header( header )
TAR_HDR *header;
{
   TRformat( evset_print, 0, out_buf, sizeof(out_buf),
             "   HDR\n");
   TRformat( evset_print, 0, out_buf, sizeof(out_buf),
             "%6sFile   = \"%s\"\n", "", header->filenm);
   TRformat( evset_print, 0, out_buf, sizeof(out_buf),
             "%6sprot   = \"%s\"\n", "", header->prot);
   TRformat( evset_print, 0, out_buf, sizeof(out_buf),
             "%6suid    = \"%s\"\n", "", header->uid);
   TRformat( evset_print, 0, out_buf, sizeof(out_buf),
             "%6sgid    = \"%s\"\n", "", header->gid);
   TRformat( evset_print, 0, out_buf, sizeof(out_buf),
             "%6scount  = \"%.#s\"\n", "", COUNT_SIZE, header->count);
   TRformat( evset_print, 0, out_buf, sizeof(out_buf),
             "%6stime   = \"%.#s\"\n", "", TIME_SIZE, header->time);
   TRformat( evset_print, 0, out_buf, sizeof(out_buf),
             "%6schksum = \"%.#s\"\n", "", CHKSUM_SIZE, header->chksum);
   TRformat( evset_print, 0, out_buf, sizeof(out_buf),
             "%6slinknm = \"%s\"\n", "", header->linknm);
   TRformat( evset_print, 0, out_buf, sizeof(out_buf),
             "\n");
}

/**/
#define TAR_IO_DIRECT
/**/

#ifdef TAR_IO_DIRECT
#define Tar_Write SIwrite
#define Tar_Read  SIread
#define Tar_Open  SIfopen
#else

/* Debuging aid, local routines so that IO
** calls can be verified centrally.
*/

static STATUS
Tar_Write( bytes, buf, written, stream )
i4 bytes;
char *buf;
i4 *written;
FILE *stream;
{
   size_t no;
   no = fwrite( buf, (size_t) 1, (size_t) bytes, stream );
   *written = (i4) no;

   return ( *written != bytes ) ? FAIL : OK;
}

static STATUS
Tar_Read( stream, bytes, read, buf )
FILE *stream;
i4   bytes;
i4   *read;
char *buf;
{
   extern int errno;
   i4 *p = &errno;

   *read = fread( buf, 1, bytes, stream );

   return ( *read < bytes ) ? ( feof(stream) ? ENDFILE : FAIL ) : OK;
}

static STATUS
Tar_Open( loc, mode, type, len, stream )
LOCATION *loc;
char     *mode;
i4       type;
i4       len;
FILE     **stream;
{
   char   fmode [10];
   char   *file;
   STATUS status;
   char   *c = fmode;
   extern int errno;
   int    *p = &errno;

   LOtos( loc, &file);

   strcpy(fmode, mode);

   if ( (fmode [0] == 'r') && (fmode [1] == 'w') )
   {
      *(++c) = '+';
      *(++c) =  ( (type == SI_BIN) || ( type == SI_VAR) ) ? 'b' :'t'; 
   }
   else if ( (fmode [0] == 'r') || (fmode [1] == 'w') )
   {
      *(++c) =  ( (type == SI_BIN) || ( type == SI_VAR) ) ? 'b' :'t';
   }

   if (c != fmode) *(++c) = '\0';

   *stream = fopen(file, fmode);

   status = (*stream) ? OK : FAIL ;

   return status;
}
#endif

/*
** Name:	chksum_header
**
** Description:
**     Checksum the TAR header record.
**     The checksum is the sum of all the bytes in the
**     TAR header record. The chksum field is considered
**     to be filled with ' ' (0x20).
**
** Inputs:
**	header	 - Pointer to the TAR_HDR record
**
** Output:
**	None.
**
** Returns:
**	Checksum value
*/
static i4
chksum_header( header )
TAR_HDR *header;
{
   i4   chksum;
   char *cp;

   for( chksum = 0, cp = (char *)header;
        cp < (char *)header->pad;
        cp++ )
   {
      if ( (cp >= (char *)header->chksum) &&
           (cp <  (char *)header->chksum + sizeof(header->chksum)) )
      {
         /* Treat byte as a space (0x20) */

         chksum += 0x20; 
      }
      else
      {
         chksum += *cp;
      }
   }

   return chksum;
}
      
/*
** Name:	write_header
**
** Description:
**     Write a TAR header record.
**
** Inputs:
**	flags	 - Function flags
**
**	header	 - Pointer to the TAR_HDR record
**
**	out_file - Stream to write to.
**
** Output:
**	None.
**
** Returns:
**	OK	- Record written to TAR archive
**	other	- Record not written.
*/
static STATUS
write_header( flags, header, out_file)
i4      flags;
TAR_HDR *header;
FILE    *out_file;
{
   i4     written;
   STATUS status;

   MEfill(TAR_PAGE_SIZE, '\0', tar_buf);

   MEcopy(header, sizeof( TAR_HDR ), tar_buf);

   if (flags & EVTAR_VERBOSE_HDR)
   {
      print_header( header );
   }

   if ( (status = Tar_Write( TAR_PAGE_SIZE, (PTR) tar_buf, &written, out_file) ) )
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: header write failed (%d)\n", status);

      return status;
   }

   if (TAR_PAGE_SIZE != written)
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: Only %d bytes written for header, not %d\n",
                 written, TAR_PAGE_SIZE);

      return FAIL;
   }

   bytes_written += TAR_PAGE_SIZE;

   return OK;
}     

/*
** Name:	write_trailer
**
** Description:
**     Write TAR trailer record.
**
** Inputs:
**	out_file - Stream to write to.
**
** Output:
**	None.
**
** Returns:
**	OK    - Trailer records written
**      other - write failure.
*/
static STATUS
write_trailer( out_file )
FILE *out_file;
{
   TAR_HDR header;
   i4      i;
   STATUS  status;

   MEfill(sizeof(TAR_HDR), '\0', &header);

   for( i = 0; i < TAR_TRAILER_RECORDS; i++ )
   {
      if ( (status = write_header(0, &header, out_file)) )
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "ERROR: Failure %d writing TRAILER record (%d)\n",
                   status, i);

         break;
      }
   }

   return status;
}

/*
** Name:	read_header
**
** Description:
**     Read a TAR header record.
**
** Inputs:
**	flags	 - Function flags
**
**	header	 - Pointer to the TAR_HDR record
**
**	in_file  - Stream to read from.
**
** Output:
**	f_type   - Type of file
**
** Returns:
**	OK	- Record written to TAR archive
**	other	- Record not written.
*/
static STATUS
read_header( flags, header, f_type, in_file )
i4 flags;
TAR_HDR *header;
i4      *f_type;
FILE    *in_file;
{
   i4     bytes;
   i4     chksum;
   i4     cur_chksum;
   char   *cp;
   STATUS status;

   status = Tar_Read(in_file, TAR_PAGE_SIZE, &bytes, tar_buf);

   if (status)
   {
      if (status != ENDFILE)
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "ERROR: Failure reading HEADER record. Status = %d\n",
                   status);
      }

      return status;
   }

   if (bytes != TAR_PAGE_SIZE)
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: HEADER read %d bytes. %d expected\n",
                bytes, TAR_PAGE_SIZE);

      return FAIL;
   }

   bytes_read += bytes;

   MEcopy(  tar_buf, sizeof( TAR_HDR ), header );

   /* Verify HEADER details OK */

   for(cp = header->chksum; *cp == ' '; cp++);

   if ( (status = CVol( cp, &chksum)) )
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: Bad checksum in HEADER (%s)\n", header->chksum);

      return status;
   }

   cur_chksum = chksum_header( header );

   if (chksum != cur_chksum)
   {
      if (cur_chksum == (sizeof(header->chksum) * 0x20))
      {
         /* The only non-zero field in the header was the chksum
         ** which is because it is treated as spaces (0x20). So
         ** this is one of the NULL Headers at the end of the TABALL.
         */
         *f_type = TAR_NULL;

         return OK;
      }

      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: Checksum mismatch. Checksum = %d Expected %d\n",
                 cur_chksum, chksum);

      print_header( header );

      return FAIL;
   }

   /* Change '/' to '\' */
   for( cp = header->filenm; *cp; cp++)
   {
      if (*cp == '/') *cp ='\\';
   }

   /* Determine the file type */

   if ( *(--cp) == '\\')
   {
      *cp = '\0';

      *f_type = TAR_IS_DIR;
   }
   else if (++cp != header->filenm)
   {
      *f_type = TAR_IS_FILE;
   }
   else
   {
      *f_type = TAR_NULL;
   }

   if (flags & EVTAR_VERBOSE_HDR)
   {
      print_header( header );
   }

    return OK;
}
   
/*
** Name:	fill_header
**
** Description:
**     Set the TAR_HEADER record, If EVTAR_REMOVE_LEAD
**     set the EVSET path is removed from the file name
**     stored in the TAR_HDR record.
**
**     A directory name in the TAR_HDR must end with a '/'.
**     This is added to the name if required.
**
** Inputs:
**	flags	 - Function flags
**
**	tar_len  - Length of ROOT directory path.
**
**	name	 - name of the file being stored.
**
**	info	 - File details (TYPE and perms).
**
**	header	 - Pointer to the TAR_HDR record
**
** Output:
**	header record filled in.
**
** Returns:
**	None
*/
static void
fill_header( flags, tar_len, name, bytecnt, info, header )
i4            flags;
i4            tar_len;
char          *name;
OFFSET_TYPE   bytecnt;
LOINFORMATION *info;
TAR_HDR       *header;
{
   char buf [200];
   char *cp;
   int  prot = 0;

   MEfill(sizeof(TAR_HDR), '\0', header);

   if (name)
   {
      if (info->li_perms & LO_P_READ)    prot |= 0444;
      if (info->li_perms & LO_P_WRITE)   prot |= 0200;
      if (info->li_perms & LO_P_EXECUTE) prot |= 0111;

      if (flags & EVTAR_REMOVE_LEAD)
      {
         name += tar_len + 1;
      }

      if ( (name [0] == '.') && (name [1] == '\\'))
      {
         name += 2;
      }

      STcopy(name, header->filenm);

      /* Change '\' to '/' */
      for( cp = header->filenm; *cp; cp++)
      {
         if (*cp == '\\') *cp ='/';
      }

      if (info->li_type == LO_IS_DIR)
      {
         if ( *(cp - 1) != '/')
         {
           STprintf( cp, "/" );
         }
      }

      STprintf(header->prot, "%07o", prot);
      STprintf(header->uid,  "%07o", 0);
      STprintf(header->gid,  "%07o", 0);
      STprintf(buf, "%011o", bytecnt);
      MEcopy(buf, COUNT_SIZE, header->count);
      STprintf(buf, "%011o", info->li_last.TM_secs);
      MEcopy(buf, TIME_SIZE, header->time);
      MEfill(CHKSUM_SIZE, ' ', header->chksum);
      STprintf(header->chksum, "%06o", chksum_header( header ));
   }
}
   
/*
** Name:	Create_File
**
** Description:
**     Creates and opens for writing a file.
**
** Inputs:
**	tar_loc	 - LOCATION of file to create
**
** Output:
**	out_file - Write channel to created file
**
** Returns:
**	OK	- File created and channel opened.
**	other	- No write channel.
*/
static STATUS
Create_File( tar_loc, out_file)
LOCATION *tar_loc;
FILE     **out_file;
{
   char *tar_file;
   STATUS status;

   /* Delete the file if it already exists */
   if (!LOexist( tar_loc ))
   {
      if ( (status = LOdelete( tar_loc )) )
      {
         LOtos( tar_loc, &tar_file);

         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "ERROR: Failed deleting existing %s\n", tar_file);

         return status;
      }
   }

   if ( (status = SIcreate( tar_loc ) ) )
   {
      LOtos( tar_loc, &tar_file);

      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: Failed to create %s\n", tar_file);

      return status;
   }
      
   if ((status = Tar_Open( tar_loc, "rw", SI_BIN, TAR_PAGE_SIZE, out_file)))
   {
      LOtos( tar_loc, &tar_file);

      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: Failed trying to open %s\n", tar_file);

      return status;
   }

   return OK;
}

/*
** Name:	put_file
**
** Description:
**     Create and populate file from TAR archive
**
** Inputs:
**	header	 - TAR_HDR for file to create
**
**	in_file  - TAR file channel
**
**	dir_loc	 - ROOT LOCATION for file
**
** Output:
**	None
**
** Returns:
**	OK	- File created.
**	other	- Failure.
*/
static STATUS
put_file( header, in_file, dir_loc)
TAR_HDR *header;
FILE    *in_file;
LOCATION *dir_loc;
{
   i4        bytes;
   char      *cp;
   LOCATION  dir;
   char      dummy [ MAX_LOC ];
   i4        file_size;
   i2        flag;
   i4        get_bytes;
   LOCATION  new_file;
   LOCATION  orig_file;
   FILE      *out_file = 0;
   i4        pages;
   char      path_buf [ MAX_LOC ];
   char      *path = path_buf;
   i4        read_bytes;
   STATUS    status;
   char      target_file [ MAX_LOC ];
   i8        tot_bytes = 0;
   i4        write_bytes;

   /* Get the size of the file (in bytes).*/
   for(cp = header->count; *cp == ' '; cp++);

   if ( (status = CVol( cp, &file_size)) )
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: File size (%s) not an octal value for \"%s\"\n",
                 cp, header->filenm);

      return status;
   }

   /* new_file set to relative path for the file from the .TAR */
   if ( (status = LOfroms(PATH&FILENAME, header->filenm, &orig_file)) )
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: Failed get filename (%s)\n", header->filenm);

      return status;
   }

   /* Get the relative path */
   if ( (status = LOdetail( &orig_file, dummy, path, dummy, dummy, dummy)) )
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: Failed getting directory path for \"%s\"\n", header->filenm);

      return status;
   }

   /* Prep new_file for the absolute location */
   LOcopy( dir_loc, target_file, &new_file );

   if ( *path )
   {
      if ( *path == '\\' )
      {
         /* Path is absolute, so probably not an EVSET output, but
         ** just in case strip off the EVSETxxx.
         */
         path++;

         cp = STstrindex( path, "\\EVSET", MAX_LOC, TRUE );

         if (cp)
         {
            path = cp + 10;
         }
      }

      if (*path)
      {
         /* Note path is now relative any leading '\' has been stripped */

         if ( (status = LOfroms(PATH, path, &dir)) )
         {
            TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                      "ERROR: Failed setting path (%s)\n", header->filenm);

            return status;
         }
         else if ( (status = LOaddpath( dir_loc, &dir, &new_file)) )
         {
            TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                      "ERROR: Failed adding path \"%s\" to target location (%s)\n", 
                      path, header->filenm);

            return status;
         }
      }
   }

   /* new_file should be a PATH at this point, so check that the
   ** location is a directory (if it exists) or create the directory.
   */
   if ( !LOexist( &new_file ) )
   {
      if ( LOisdir( &new_file, &flag )  || !(flag & ISDIR) )
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "ERROR: \"%s\" exists and is not a directory. (%s)\n",
                   path, header->filenm);

         return FAIL;
      }
   }
   else if ( (status = LOcreate( &new_file ) ) )
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: Failed to create \"%s\". (%s)\n",
                path, header->filenm);

      return status;
   }

   /* Now add the file name to the location */
   if ( (status = LOstfile( &orig_file, &new_file )) )
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: Failed adding filename\n");

      return status;
   }
      
   if ( (status = Create_File( &new_file, &out_file)) )
   {
      return status;
   }

   /* Read the file details from the TAR file, remember it has been
   ** stored in TAR_PAGE_SIZE records.
   */
   for(read_bytes = file_size, write_bytes = sizeof(tar_buf);
       read_bytes > 0;
       read_bytes -= get_bytes)
   {
      if (read_bytes >= sizeof(tar_buf))
      {
         get_bytes = sizeof(tar_buf);
      }
      else
      {
         pages = (read_bytes + TAR_PAGE_SIZE -1) / TAR_PAGE_SIZE;

         get_bytes = pages * TAR_PAGE_SIZE;
      }

      if ( (status = Tar_Read(in_file, get_bytes, &bytes, tar_buf) ) )
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "ERROR: Failure reading file (%s). Status = %d\n",
                   header->filenm, status);

         break;
      }

      if (bytes != get_bytes)
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "ERROR: Read %d/%d bytes from %s.\n",
                   bytes, get_bytes, header->filenm);

         status = FAIL;

         break;
      }

      if (read_bytes < get_bytes)
      {
         write_bytes = read_bytes;
      }

      status = Tar_Write(write_bytes, tar_buf, &bytes, out_file);
     
      if (status || bytes != write_bytes)
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "ERROR: Failed adding to %s. Status = %d Bytes = %d/%d\n",
                   header->filenm, status, bytes, write_bytes);

         if (!status) status = FAIL;

         break;
      }

      tot_bytes  += write_bytes;
      bytes_read += bytes;
   }

   bytes_written += tot_bytes;

   SIflush( out_file );
   SIclose( out_file );

   return status;
}

/*
** Name:	add_file
**
** Description:
**     Copy the contents of a file into the TAR archive.
**
** Inputs:
**	flags    - Function flags
**
**      tar_len  - Length of ROOT directory path.
**
**	file_loc - LOCATION of file
**
**	tar_loc  - LOCATION of TAR archive
**
**	loc_info - File details.
**
**      out_file - TAR archive write stream.
**
**
** Output:
**	out_file - TAR archive write stream.
**
** Returns:
**	OK	- File added to TAR.
**	other	- Failure.
*/
static STATUS
add_file( flags, tar_len, file_loc, tar_loc, loc_info, out_file)
i4             flags;
i4             tar_len;
LOCATION       *file_loc;
LOCATION       *tar_loc;
LOINFORMATION  *loc_info;
FILE           **out_file;
{
   int         bytes;
   int         pages;
   int         tot_bytes = 0;
   char        *ofile;
   TAR_HDR     header;
   FILE        *in_file;
   STATUS      status;

   if (!*out_file)
   {
      if ( (status = Create_File( tar_loc, out_file)) )
      {
         return status;
      }
   }

   LOtos( file_loc, &ofile);

   if (flags & EVTAR_VERBOSE_DIAG)
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "   Adding %s ...\n", ofile);
   }

   fill_header(flags, tar_len, ofile, loc_info->li_size, loc_info, &header);
   write_header(0, &header, *out_file);

   /* Now copy the file in TAR_PAGE_SIZE byte blocks to the TAR file */

   if ( (status = Tar_Open( file_loc, "r", SI_VAR, TAR_PAGE_SIZE, &in_file)) )
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: Failed to open %s\n", ofile);

      return status;
   }

   while( (status = Tar_Read( in_file, sizeof(tar_buf), &bytes, tar_buf)) == OK )
   {
      tot_bytes += bytes;
      status = Tar_Write(sizeof(tar_buf), tar_buf, &bytes, *out_file);

      if (status || bytes != sizeof(tar_buf))
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "ERROR: Failed adding %s to TAR file. Status = %d Bytes = %d\n",
                   ofile, status, bytes);

         SIclose( in_file );

         if (!status) status = FAIL;

         return status;
      }

      bytes_written += bytes;
   }

   if ( status != ENDFILE) 
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: Failed reading %s. Status = %d Bytes = %d\n",
                ofile, status, bytes);

      SIclose( in_file );

      return status;
   }

   if (bytes)
   {
      /* File has been written out all bar the last read which
      ** was less than the "TAR_BUF" size. So pad the TAR_BUF
      ** with 0's to the next TAR_PAGE boundary and then
      ** write out the TAR_PAGES.
      */
      tot_bytes += bytes;

      pages = (bytes + TAR_PAGE_SIZE -1) / TAR_PAGE_SIZE;

      MEfill( (pages * TAR_PAGE_SIZE) - bytes, '\0', &(tar_buf [bytes]));

      status = Tar_Write(pages * TAR_PAGE_SIZE, tar_buf, &bytes, *out_file);

      if (status || bytes != pages * TAR_PAGE_SIZE)
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "ERROR: Failed adding tail of %s to TAR file. Status = %d Bytes = %d/%d\n",
                   ofile, status, bytes, pages * TAR_PAGE_SIZE);

         SIclose( in_file );

         if (!status) status = FAIL;

         return status;
      }

      bytes_written += bytes;
   }

   SIclose( in_file );

   if (tot_bytes != loc_info->li_size)
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: Read %d bytes. Expected to read %d\n",
                tot_bytes, loc_info->li_size);

      return FAIL;
   }

   bytes_read += tot_bytes;

   return OK;
}

/*
** Name:	untar
**
** Description:
**     Extract files from a TAR archive.
**
** Inputs:
**	flags    - Function flags
**
**	dir_loc  - Root LOCATION for extracted files
**
**	tar_loc  - LOCATION of TAR archive
**
** Output:
**	None
**
** Returns:
**	OK	- File added to TAR.
**	other	- Failure.
*/
static STATUS
untar(flags, dir_loc, tar_loc)
i4       flags;
LOCATION *dir_loc;
LOCATION *tar_loc;
{
   i4             f_type;
   TAR_HDR        header;
   FILE           *in_file = 0;
   LOINFORMATION  info;
   i4             lo_flags = LO_I_TYPE;
   LOCATION       new_file;
   STATUS         status;
   char           *tar_file;

   if ((status = Tar_Open( tar_loc, "r", SI_BIN, TAR_PAGE_SIZE, &in_file)))
   {
      LOtos( tar_loc, &tar_file);

      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "ERROR: failed trying to open %s Status = %d\n", tar_file, status);

      return status;
   }

   while( !(status = read_header(flags, &header, &f_type, in_file) ))
   {
      if (f_type == TAR_IS_DIR)
      {
         /* Obtained details for a Directory. */
         if ( (status = LOfroms(PATH, header.filenm, &new_file)) )
         {
            TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                      "ERROR: Failed get DIRECTORY location (%s)\n",
                      header.filenm);

            break;
         }

         if (!LOexist( &new_file ))
         {
            if ( (status = LOinfo( &new_file, &lo_flags, &info) ) ||
                 (info.li_type != LO_IS_DIR) )
            {
               TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                         "ERROR: Location \"%s\" already exists and is not a directory\n",
                         header.filenm);

               if (!status) status = FAIL;

               break;
            }
         }
         else if ( (status = LOcreate( &new_file ) ) )
         {
            TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                      "ERROR: Failed to create dir (%s). Status = %d\n",
                      header.filenm, status);

            break;
         }
      }
      else if (f_type == TAR_IS_FILE)
      {
         if ( (status = put_file( &header, in_file, dir_loc ) ) )
         {
            break;
         }
      }
   }   

   SIclose( in_file );

   if (status == ENDFILE) status = OK;

   return status;
}

/*
** Name:	directory_scan
**
** Description:
**     Scan a directory for all files/directories.
**     This function recurses down through directories.
**
** Inputs:
**	flags    - Function flags
**
**	tar_len  - Length of ROOT directory path.
**
**	dir_loc  - Root LOCATION for extracted files
**
**	tar_loc  - LOCATION of TAR archive
**
**	out_file - Stream to the TAR archive.
**
** Output:
**	None
**
** Returns:
**	OK	- Files in directory added to TAR.
**	other	- Failure.
*/
static STATUS
directory_scan( flags, tar_len, dir_loc, tar_loc, out_file)
i4       flags;
i4       tar_len;
LOCATION *dir_loc;
LOCATION *tar_loc;
FILE     **out_file;
{
   LO_DIR_CONTEXT dir_ctx;
   char           dir_name [MAX_PATH];
   i4             lo_flags;
   LOINFORMATION  loc_info;
   STATUS         status;
   char           *wfile;
   TAR_HDR        header;

   LOtos( dir_loc, &wfile);

   STcopy( wfile, dir_name );

   if (flags & EVTAR_VERBOSE_DIAG)
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "DIAG: Scanning %s\\ ...\n", dir_name);
   }

   if ( (status = LOwcard( dir_loc, NULL, NULL, NULL, &dir_ctx)) )
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "Failure beginning wildcard directory scan of (%s)\n", dir_name);

      return status;
   }

   do
   {
      LOtos( dir_loc, &wfile);

      lo_flags = LO_I_TYPE | LO_I_LAST | LO_I_PERMS | LO_I_SIZE;

      MEfill( sizeof(loc_info), '\0', (PTR)&loc_info);

      if ( (status = LOinfo( dir_loc, &lo_flags, &loc_info)) != OK)
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "Failed to get info on %s\n", wfile);

         return status;
      }
      else if (loc_info.li_type == LO_IS_DIR)
      {
         /* Add the directory name to the TAR file, then scan it */
         fill_header( flags, tar_len, wfile, (OFFSET_TYPE)0, &loc_info, &header );
         write_header( flags, &header, *out_file);

         if ( (status = directory_scan( flags, tar_len, dir_loc, tar_loc, out_file)))
         {
            return status;
         }
      }
      else if (loc_info.li_type == LO_IS_FILE)
      {
         if ( (status = add_file( flags, tar_len, dir_loc, tar_loc, &loc_info, out_file)))
         {
            TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                      "Failure adding %s to tar_file\n", wfile);

            return status;
         }
      }
      else
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "ERROR: Unknow file type %d for %s\n",
                   loc_info.li_type, wfile);

         return FAIL;
      }

      status = LOwnext( &dir_ctx, dir_loc);
   }
   while (status == OK);

   if (status != ENDFILE)
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "Failure while scanning dir(%s). Last file = %s. Status = %d\n", 
                dir_name, wfile, status);

      return status;
   } 

   return OK;
}

/*
** Name:	EVtar
**
** Description:
**     Entry point for EVtar function. Either TAR or
**     UNTAR a directory.
**
** Inputs:
**	flags    - Function flags
**
**	tar_dir  - PATH to directory to be TAR'd or ROOT for untarring.
**
**	tar_file - PATH of TAR file to be created/untar'd.
**
** Output:
**	None
**
** Returns:
**	OK	- Success.
**	other	- Failure.
*/
STATUS
EVtar(flags, tar_dir, tar_file)
i4   flags;
char *tar_dir;
char *tar_file;
{
   FILE          *out_file = 0;
   LOCATION      dir_loc, tar_loc;
   i4            lo_flags;
   i4            tar_len = STlength( tar_dir );
   LOINFORMATION loc_info;
   LOINFORMATION loc_info1;
   STATUS        status;

   if (flags & EVTAR_VERBOSE)
   {
      TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                "Tar F = %s Tar_Dir = %s\n\n",
                tar_file, tar_dir);
   }

   do
   {
      /* Verify the location for the TAR file is OK */

      if ( (status = LOfroms(PATH&FILENAME, tar_file, &tar_loc)) )
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "Failure setting LOCATION for tar_file (%s)\n", tar_file);

         break;
      }

      lo_flags = LO_I_TYPE;

      if ( (status = LOinfo( &tar_loc, &lo_flags, &loc_info)) == OK)
      {
         if (loc_info.li_type == LO_IS_DIR)
         {
            TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                      "Error: tar_file (%s) is a directory\n", tar_file);
   
            status = FAIL;

            break;
         }

         if ( !(flags & EVTAR_UNTAR) )
         {
            TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                      "Failure tar_file(%s) exists\n", tar_file);

            status = FAIL;

            break;
         }
      }
      else if (flags & EVTAR_UNTAR)
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "ERROR: tar_file (%s) does not exist\n", tar_file);

         status = LO_NOT_FILE;

         break;
      }

      /* Convert the ROOT directory to a LOCATION */

      if ( (status = LOfroms(PATH, tar_dir, &dir_loc)) )
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "Failure setting LOCATION for tar_dir (%s)\n", tar_dir);

         break;
      }

      lo_flags = LO_I_TYPE | LO_I_LAST;

      if ( (status = LOinfo( &dir_loc, &lo_flags, &loc_info1)) )
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "Failure getting LOCATION info for tar_dir (%s)\n", tar_dir);

         break;
      }

      if (loc_info1.li_type != LO_IS_DIR)
      {
         TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "Error: tar_dir (%s) is not a directory\n", tar_dir);

         status = LO_NOT_PATH;

         break;
      }

      if (flags & EVTAR_UNTAR)
      {
         if ( (status = untar( flags, &dir_loc, &tar_loc )) )
         {
            TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                   "ERROR: Untar of tar file (%s) failed\n", tar_file);

            break;
         }     
      }
      else
      {
         if ( (status = directory_scan(flags, tar_len, &dir_loc, &tar_loc, &out_file)))
         {
            TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                      "ERROR: Scan of tar_dir (%s) failed\n", tar_dir);

            break;
         }

         if (out_file)
         {
            write_trailer( out_file );

            SIflush( out_file );
            SIclose( out_file );
         }
      }

      if (flags & EVTAR_VERBOSE)
      {
         TRformat( evset_print, (PTR) 0, out_buf, sizeof(out_buf),
                   "Read  %ld bytes\n", bytes_read);
         TRformat( evset_print, (PTR) 0, out_buf, sizeof(out_buf),
                   "Wrote %ld bytes\n", bytes_written);
      }

      return OK;
   }
   while( FALSE );

   if (out_file)
   {
      SIflush( out_file );
      SIclose( out_file );
   }

   return status;
}

#ifdef STANDALONE
i4
main(argc, argv)
i4 argc;
char **argv;
{
   char *cp;
   CL_ERR_DESC err_code;
   i4   flags = 0;
   char *tar_dir;
   char *tar_file;

   TRset_setfile( TR_T_OPEN, "stdio", 5, &err_code );

   ++argv;

   while( --argc > 0)
   {
      if ((*argv)[0] == '-')
      {
         switch( ((*argv)[1] ) )
         {
            case 'r':
               flags |= EVTAR_REMOVE_LEAD;
               break;

            case 'u':
               flags |= EVTAR_UNTAR;
               break;

            case 'v':
               flags |= EVTAR_VERBOSE;

               for (cp = &((*argv)[2]); *cp; cp++)
               {
                  switch( *cp )
                  {
                     case 'd':
                        flags |= EVTAR_VERBOSE_DIAG;
                        break;

                     case 'h':
                        flags |= EVTAR_VERBOSE_HDR;
                        break;

                     default:
                        TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                                  "ERROR: Unrecognised Verbose flags \"%s\" (%s)\n",
                                  *argv, cp);

                        return FAIL;
                  }
               }

               break;

            default:
               TRformat( evset_print, 0, out_buf, sizeof(out_buf),
                         "ERROR: Unknown flag \"%s\"\n", *argv);

               return FAIL;
         }
      }
      else if ( !tar_file )
      {
         tar_file = *argv;
      }
      else if ( !tar_dir )
      {
         tar_dir = *argv;
      }

      argv++;
   }

   EVtar( flags, tar_dir, tar_file );
}
#endif
   
