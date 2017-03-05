#include <stdio.h>
#include <string.h>
#include <log.h>

// Temporary log function
void
log_disir_plugin (const char *message, ...)
{
    va_list args;
    va_start (args, message);
    vfprintf (stderr, message, args);
    fprintf (stderr, "\n");
    fflush (stderr);
    va_end (args);
}

// Debug function
// Always outputs to stderr
// TODO: optional output stream (compiletime defined)
void
_log_debug (const char *function, const char *file, int line,
           const char *message, ...)
{
   char output[255];
  int bufsize = 255;
   int res;
   va_list args;
   va_start (args, message);

   res = snprintf (output, bufsize, " (%s:%d)", function, line);

   if (res > 0)
       output[res] = '\0';

   fprintf (stderr, "DEBUG: ");

   vfprintf (stderr, message, args);

   fprintf (stderr, output);

   fprintf (stderr, "\n");
   fflush (stderr);

   va_end (args);

}

