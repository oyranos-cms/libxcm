#include <X11/Xcm/XcmEdidParse.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv)
{
  XCM_EDID_ERROR_e err = 0;
  char * txt = 0;
  FILE * fp = 0;
  void * mem = 0;
  size_t size = 0;
  int s = 0;

  if(argc > 1)
  {
    fp = fopen(argv[1],"rb");
    if(fp)
    {
      fseek(fp,0L,SEEK_END); 
      size = ftell (fp);
      rewind(fp);
      mem = malloc(size);
      s = fread(mem, sizeof(char), size, fp);
      fclose (fp);
    }
    err = XcmEdidPrintString( mem, &txt, malloc );
    if(err)
      fprintf(stderr, "Error: %s\n", XcmEdidErrorToString(err));
    else
      fprintf(stderr, "%s\n", txt);
    free(txt);
  }

  return 0;
}
