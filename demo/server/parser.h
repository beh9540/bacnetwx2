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
#include <stdio.h>
const static size_t LINE_LENGTH = 100;

typedef struct {
   FILE *fp;
} parse_handle;

typedef struct {
   char *PID;
   char *LKEY;
   char *LOCID;
   char *FORECASTED_DAYS;
} parse_line;

/*
 *Initializes the parser
 *RETURN: pointer to struct, NULL if not initialized
*/
parse_handle* parseInit(
   char *file);
/*
 *Gets the number of lines in the file
 *RETURN: number of lines
*/
int parseGetNumberLines(
   parse_handle *h);
/*
 *Search for option  
 *RETURN: NULL if it doesn't parse, POINTER to lines struct otherwise
*/
parse_line* parseOptions(
   parse_handle *h);
/*
 *Frees the memory used during parsing
*/
void freeParserHandle(
   parse_handle *h);

void freeParseOptions(
   parse_line *line);
