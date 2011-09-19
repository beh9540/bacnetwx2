/**************************************************************************
*
* Copyright (C) 2011 Blake Howell <beh9540@rit.edu>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/
#include "parser.h"
#include <stdio.h>

int main()
{
   parse_handle *handle;
   parse_line *line;
   handle = parseInit("test.txt");
   if ( handle == NULL )
   {
      printf("We got a null handle, sucks\n");
      return -1;
   }
   
   printf("We managed to open the file\n");

   line = parseOptions(handle);
   if (line == NULL)
      printf("Couldn't get line\n");
   if (line->PID == NULL)
      printf("We couldn't get PID\n");
   else 
      printf("PID is %s\n",line->PID);
   if (line->LKEY == NULL)
      printf("We couldn't get the LKEY\n");
   else
      printf("LKEY is %s\n",line->LKEY);
   if (line->LOCID == NULL)
      printf("We couldn't get the LOCID\n");
   else
      printf("LOCID is %s\n",line->LOCID);
   return 0;
   freeParserHandle(handle);
   freeParserOptions(line);
}
   

