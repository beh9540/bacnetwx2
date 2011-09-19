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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"

int parseValue(parse_handle *h,char* testString,char* space);

parse_handle* parseInit(char *file)
{
   parse_handle *h = malloc(sizeof(parse_handle) );
   FILE *fp=fopen(file,"r");
   if (fp==NULL)
      return NULL;
   h->fp=fp;
   return h;
}

int parseGetNumberLines(parse_handle *h)
{
   FILE *fp = h->fp;
   char *pbuffer;
   int counter = 0;
   pbuffer = calloc(LINE_LENGTH,sizeof(char) ); 
   while (fgets(pbuffer,LINE_LENGTH,fp) ) {           
       counter++;
   }
   free(pbuffer);
   return counter;
}

parse_line* parseOptions(parse_handle *h)
{
   parse_line *line = malloc(sizeof(parse_line) );
   line->PID = malloc(sizeof(char)*LINE_LENGTH);
   line->LOCID = malloc(sizeof(char)*LINE_LENGTH);
   line->LKEY = malloc(sizeof(char)*LINE_LENGTH);
   line->FORECASTED_DAYS = malloc(sizeof(char)*LINE_LENGTH);
   parseValue(h,"PID",line->PID);
   parseValue(h,"LOCID",line->LOCID);
   parseValue(h,"LKEY",line->LKEY);
   parseValue(h,"FORECASETED_DAYS",line->FORECASTED_DAYS);
   return line;
}

int parseValue(parse_handle *h,char* testString,char* value) {
   FILE *fp = h->fp;   
   char *pbuffer;
   char *name;
   char *before;
   char *space;
   pbuffer = calloc(LINE_LENGTH,sizeof(char) );
   name = calloc(LINE_LENGTH,sizeof(char) );
   before = calloc(LINE_LENGTH,sizeof(char) );
   rewind(fp);
   while (fgets(pbuffer,LINE_LENGTH,fp) ) {
     if (strchr(pbuffer, '#') ) {
	   continue;
     }
     space=strchr(pbuffer,' ');
     if (space!=NULL) {
       strncpy(before,pbuffer,space-pbuffer+1);
       size_t length = strlen(before);
       *(before+length) = '\0';
       if ( strncmp(before,testString,3) == 0 ) {
         length = space-pbuffer+1;
         //this gets the value of the option
         size_t index=0;
         while (length<strlen(pbuffer)) {
	         *(name+index)=*(pbuffer+length);
	         index++;
	         length++;
         }
         *(name+index-1) = '\0';//replaces newline with null
         strcpy(value,name);
         free(pbuffer);
         free(name);
         free(before);
         return 0;
       }
     }
   }
   fprintf(stderr,"We couldn't fine %s anywhere\n",testString);
   free(pbuffer);
   free(name);
   free(before);
   return -1; 
 }
 
void freeParserHandle(parse_handle *h)
{
   FILE *fp = h->fp;
   fclose(fp);
   free(h);
}

void freeParseOptions(parse_line *line)
{
   free(line->PID);
   free(line->LOCID);
   free(line->LKEY);
   free(line->FORECASTED_DAYS);
   free(line);
}













   
