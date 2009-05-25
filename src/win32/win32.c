#ifdef __WIN32__
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <utime.h>
#include <windows.h>

//#ifndef FILE_FLAG_BACKUP_SEMANTICS
//#define 0
//#endif


void UnixTimeToFileTime (time_t t, LPFILETIME pft)
{
     double cval;
	cval = (double) t * (double) 10000000LL +  (double) 116444736000000000LL;
     pft->dwLowDateTime = (DWORD) cval;
     pft->dwHighDateTime = (((long long) cval) >> 32);
}


int posix_utime (const char *path, struct utimbuf *buf)
{
   int retval = 0;
   HANDLE hFile;
   FILETIME lat;
   FILETIME lwt;
	
   hFile = CreateFile (path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
   if (hFile != INVALID_HANDLE_VALUE)
   {
      if (buf == NULL)
      {		   
         UnixTimeToFileTime (buf->actime, &lat);
         UnixTimeToFileTime (buf->modtime, &lwt);
      }
 	 else 
      {
         GetSystemTimeAsFileTime (&lat);
         GetSystemTimeAsFileTime (&lwt);
      }
      if (!SetFileTime (hFile, (LPFILETIME) NULL, &lat, &lwt))
	 {
         errno=ENOENT;
         retval = -1;		 
	 }
      CloseHandle (hFile);
   }	   
   else
   {
      errno=ENOENT;
      retval = -1;
   }
   return (retval);   
}

int utimes (const char *path, const struct timeval times[2])
{
   struct utimbuf buf;
	
   buf.actime = times[1].tv_sec;
   buf.modtime = times[2].tv_sec;
   posix_utime (path, &buf); 
}
	

typedef BOOL (WINAPI * CreateHardLinkFuncType) (LPCTSTR lpFileName, LPCTSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes); 
static CreateHardLinkFuncType CreateHardLinkProc = NULL; 

int link(const char *existing, const char *newfile)
{
   int retval = 0;     
   HINSTANCE hinstLib = LoadLibrary("kernel32.dll"); 
   if (hinstLib != NULL) 
   { 
      CreateHardLinkProc = (CreateHardLinkFuncType) GetProcAddress(hinstLib, "CreateHardLinkA"); 
      if (NULL != CreateHardLinkProc) 
      {
         if (CreateHardLinkProc(newfile, existing, NULL) == 0)
         {   
            DWORD errval = GetLastError ();
            if (errval == ERROR_ACCESS_DENIED)
               errno = EACCES;
            else if (errval == ERROR_TOO_MANY_LINKS)
               errno = EMLINK;
            else 
               errno = ENOENT;
            retval = -1;
         }
      }
      else
      {
         errno = ENOENT;
         retval = -1;
         FreeLibrary(hinstLib); 
      }
   }
   else
   {
      errno = ENOENT;
      retval = -1;
   }
   return (retval);
}

#endif
