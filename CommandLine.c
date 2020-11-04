/*
Copyright E J Jaquay 2020
This file is part of VCC (Virtual Color Computer).
    VCC (Virtual Color Computer) is free software: you can redistribute it 
    and/or modify it under the terms of the GNU General Public License as 
    published by the Free Software Foundation, either version 3 of the License, 
    or (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

//-------------------------------------------------------------------
// Parse command arguments and update global settings 
//
// Items on command string are separated by blanks unless within quotes.
//
// Options are specified by a leading dash "-" followed by a single character 
// option code.  This code may be followed by an option value. Blank space
// is permitted but not required between the option code and option value.
// Anything that is not an option is considered to be a positional argument
//
// Caveauts:
// o Supports short (single char) option codes only. 
// o Option codes may not be combined. Each must be preceeded by '-'
// o Command string cannot be longer than 255 characters.
// o Wide character or control character input is not supported.
// o No provision to escape double quotes or other special characters.
//-------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Following are in CommandLine.h
//   char QuickLoadFile[]
//   char IniFilePath[]
//   int ConsoleLogging
//   int CommandLineSettings

#include "CommandLine.h"

//-------------------------------------------------------------------
// Parse the command string and set IniFilePath, QuickLoadFile, and 
// possibly other things.  Variables are globals defined in config.h
//
// vcc.exe [-d [level]] [-i IniFile] [QuickLoadFile] 
//-------------------------------------------------------------------

// Parser is private, defined here for readability
char *ParseCmdString(char *, const char *); 

int CommandLineSettings(char *CmdString) 
{
    char *token;      // token pointer (to item in command string)
    int  argnum = 0;  // non-option argument number

    // Options that want values need to be known by parser
    // Option codes for options that do not expect values
    // should not be included in the ValueWanted string
    static char * ValueWanted = "id";

    // Get the first token from the command string
    token = ParseCmdString(CmdString,ValueWanted);
    while (token) {                         

        // Option?
        if (*token == '-') {               
            switch (*(token+1)) {

                // "-i" specfies the Vcc init file to use.
                case 'i':
                    strncpy(IniFilePath,token+2,MAX_PATH);
                    break;

                // "-d" enables logging console and sets log level (default=1)
                case 'd':
                    if (*(token+2)) {
                        ConsoleLogging = atoi(token+2);
                        if (ConsoleLogging < 1) ConsoleLogging = 0;
                        if (ConsoleLogging > 3) ConsoleLogging = 3;
                    } else {
                        ConsoleLogging = 1;
                    }
                    break;

                // Unknown option code returns an error
                default:
                    printf( "Unknown: %s\n", token); //msgbox? 
                    return 1;
            }

        // Else non-option argument            
        } else { 
            switch (++argnum) {

                // First (currently only) positional arg is Quickload filename 
                case 1:
                    strncpy(QuickLoadFile,token,MAX_PATH);
                    break;

                // Ignore extra positional arguments for now
                default:
                    printf( "Ignored: %s\n", token); //msgbox? 
                    break;
            }
        }

        // Get next token
        token = ParseCmdString(NULL,ValueWanted);
    }            
    return 0;
}
   
//-------------------------------------------------------------------
// Replace blanks in quoted sections of a string with a marker.  This
// permits parser to split command line items on spaces without damage 
// to blanks enbedded in filenames.
//-------------------------------------------------------------------

void ReplaceBlanks(char *string, int marker) 
{
    int quoted = 0;
    for( char * p = string; *p; p++) {
        if (*p == '"') {
            quoted = !quoted;
        } else {
            if (quoted && (*p == ' ')) *p = marker;
        }
    }
}

//-------------------------------------------------------------------
// Return pointer to next space delimited token in string or NULL if 
// there is no more. String pointer is advanced to beyond the token.
//-------------------------------------------------------------------

char *NextToken (char **AddrStringPtr)
{

    // Return NULL if no tokens
    if (**AddrStringPtr == '\0') return NULL; 
   
    // Skip leading blanks
    while(**AddrStringPtr == ' ') {
        (*AddrStringPtr)++;
        if (**AddrStringPtr == '\0') return NULL; 
    }

    // Save token pointer 
    char *token = *AddrStringPtr;

    // Advance to end of token
    while (**AddrStringPtr != ' ') {
        (*AddrStringPtr)++;
        if (**AddrStringPtr == '\0') return token; 
    }

    // Terminate token
    **AddrStringPtr = '\0';
    
    // Advance beyond token if anything is there
    if (*((*AddrStringPtr)+1)) (*AddrStringPtr)++; 

    return token;
}
 
//-------------------------------------------------------------------
// Restore marked blanks in string.
//-------------------------------------------------------------------

void RestoreBlanks(char *string, int marker)
{
    for ( char * p = string; *p; p++) if (*p == marker) *p = ' '; 
}

//-------------------------------------------------------------------
// Command string parser.
//
// Input string is tokenized using one or more blanks as a separator, excepting 
// blanks within quotes. If the input CmdString is not null a pointer to the 
// first token is returned.  Otherwise pointers to subsequent tokens are returned.  
// When there are no more tokens a null pointer is returned.
//
// CmdString:   Command string; for example third arg to WinMain()
// ValueWanted: String containing option codes that expect values.
//
//-------------------------------------------------------------------

char * ParseCmdString(char *CmdString, const char *ValueWanted) 
{
    static char string[256];  // Copy of cmd string
    static char option[256];  // Used to append value to option
    static int marker=3;      // Marker (EOT) used to save quoted blanks
    char *token;

    // Make string pointer that can be modified.  This always
    // points to the remainder of the string not processed
    static char *StringPtr;    

    // Initial call sets command string. Subsequent calls expect a NULL
    if (CmdString) {                   
        strncpy(string,CmdString,256);  // Copy command string 
        string[255]='\0';               // Be sure it is terminated
        ReplaceBlanks(string,marker);   // Mark the embedded blanks
        StringPtr = string;             // Set pointer to it
    }

    if (token = NextToken(&StringPtr)) {      
        RestoreBlanks(token,marker);    // Restore blanks in this token

        // Check if it is an option token.  If so check if a option value
        // is expected.  If there is one append it to a copy and return
        // a pointer to that. 
        if ((token[0] == '-') && (token[1] != '\0')) {
            if (strchr(ValueWanted,token[1])) {     
                if ((token[2] == '\0') &&           
                    (*StringPtr != '\0') &&        
                    (*StringPtr != '-')) {          
                    strcpy(option,token);           
                    if (token = NextToken(&StringPtr)) {  
                        RestoreBlanks(token,marker);
                        strcat(option,token);       
                    }
                    return option;                  
                }
            }     
        }
    }
    return token;   // null, positional argument, or option with no value
}

///////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    *QuickLoadFile='\0';
    *IniFilePath='\0';

    argv++;
    if (*argv) {
        printf ("\nParsing: %s\n",*argv);
        int rc = CommandLineSettings(*argv); 
        if (rc == 0) { 
            printf ("\nQL:%s\nIF:%s\nCO:%d\n",
                            QuickLoadFile,IniFilePath,ConsoleLogging);
        } else {
            printf("\nBad arguments\n");
        }
    }
    return 0;
}
