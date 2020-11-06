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
//
// This code brings command line arguments and options to VCC for those who
// like to start vcc from a cmd prompt or who want to produce shortcuts to 
// start instances of VCC with differing settings.  
//
// Since VCC is a windows application traditional Unix command arguments (argc,argV) 
// are not availiable. However the third argument to WinMain() does provide the 
// command string less the program name.  Historically this argument is used
// to supply the path of a Color Computer image file that is loaded into memory
// after VCC starts. 
//
// This program permits the command string to supply other information to VCC when 
// it starts up, most notable it allows the specification of a custom VCC init file. 
//
// When writing this I tried to make as flexible as possible as possible without
// adding any new dependancies to VCC. By making simple changes to this file
// and to the functions that use them it should be possible to add other things. 
// It maybe. however, that the ability to specify a custom ini file and
// possibly logging options will be sufficent for most. 
//
// Syntax:
//
// Items on command string are separated by blanks unless within quotes.
//
// Options are specified by a leading dash "-" followed by a single character 
// option code.  This may be followed by an option value. 
//
// Anything that is not an option is considered to be a positional argument.
// It is intended that the legacy image file path name will remain the first
// such argument so that existing usage is not affected.  If there is no longer
// a need to specify an image file on the command line this could change.
//
// Examples:
//
// $ VCC -i C:\users\fred\my-vcc.ini
//
// Specifies the an init file fred wants to use.
//
// $ VCC "C:\barney's good image.ima"
//
// Loads the legacy image file.  Quotes all blanks in the file path.
//
// Caveauts:
//
// o Supports short (single char) option codes only. 
// o Option codes may not be combined. Each must be preceeded by '-'
// o Command string cannot be longer than 255 characters.
// o Wide character or control character input is not supported.
// o No provision to escape double quotes or other special characters.
//
//-------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "CommandLine.h"

#define SEPMARK 3

char *ParseCmdString(char *, const char *); 
char *GetNextToken ();
static char *NxtTokenPtr;    

//-------------------------------------------------------------------
// Parse the VCC command string and set IniFilePath, QuickLoadFile, and 
// possibly other things.  Variables set by this routine are global and 
// are defined in CommandLine.h
//
// vcc.exe [-d[level]] [-i IniFile] [QuickLoadFile] 
//
// CmdString:  The third arg to WinMain()
//-------------------------------------------------------------------

int CommandLineSettings(char *CmdString) 
{
    char *token;      // token pointer (to item in command string)
    int  argnum = 0;  // non-option argument number

    // Options that require values need to be known by the parser
    // to allow seperation between option code and value.  
    // OPtion codes for these should be placed in the ValueRequired
    // String.  If the value is optional it can not be seperated from 
    // the code.  These should not be included in the Valurequired
    // string.

    static char * ValueRequired = "i";

    // Initialize the global variables set by this routine
    *QuickLoadFile = '\0';
    *IniFilePath = '\0';
    ConsoleLogging = 0;

    // Get the first token from the command string
    token = ParseCmdString(CmdString,ValueRequired);
    while (token) {                         

        // Option?
        if (*token == '-') {               
            switch (*(token+1)) {

                // "-i" specfies the Vcc init file to use.
                // The value is required and will be found starting
                // at the third character of the option string.
                case 'i':
                    strncpy(IniFilePath,token+2,MAX_PATH);
                    break;

                // "-d[level]" enables logging console and sets log level (default=1)
                // level is optional. It defaults to 1 and is forced to be
                // a positive integer 0 to 3.
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
                    return 1;
            }

        // else Positional argument            
        // argnum indicates argument position starting at one. 
        } else { 
            switch (++argnum) {

                // First (currently only) positional arg is Quickload filename.
                case 1:
                    strncpy(QuickLoadFile,token,MAX_PATH);
                    break;

                // Ignore extra positional arguments for now
                default:
                    break;
            }
        }

        // Get next token
        token = ParseCmdString(NULL,ValueRequired);
    }            
    return 0;
}

//-------------------------------------------------------------------
// Command string parser.
//
// Input string is tokenized using one or more blanks as a separator, excepting 
// blanks within quotes. If the input String is not null a pointer to the 
// first token is returned.  Otherwise pointers to subsequent tokens are returned.  
// When there are no more tokens a null pointer is returned.
//
// ValueRequired: String containing option codes that require values.
//
//-------------------------------------------------------------------

char * ParseCmdString(char *CmdString, const char *ValueRequired) 
{
    static char string[256];  // Copy of cmd string
    static char option[256];  // Used to append value to option
    int quoted;
    char *token;
    char *value;

    // Initial call sets command string. Subsequent calls expect a NULL
    if (CmdString) {                   
        while (*CmdString == ' ') CmdString++;  // Skip leading blanks
        strncpy(string,CmdString,256);          // Make a copy of what is left
        string[255]='\0';                       // Be sure it is terminated
        NxtTokenPtr = string;                   // Save it's location

        // Mark unquoted blanks
        quoted = 0;
        for( char * p = NxtTokenPtr; *p; p++) {
            if (*p == '"') {
                quoted = !quoted;
            } else if (!quoted) {
                if (*p == ' ') *p = SEPMARK;
            }
        }
    }

    if (token = GetNextToken()) {      
        // Check if it is an option token.  If a option value is
        // required and value is seperated make a copy and append
        // next token to it
        if ((token[0] == '-') && (token[1] != '\0') && (token[2] == '\0')) {
            if (strchr(ValueRequired,token[1])) {
                if ((NxtTokenPtr[0] != '\0') && (NxtTokenPtr[0] != '-')) {
                    if (value = GetNextToken()) {  
                        strcpy(option,token);      
                        strcat(option,value);       
                        return option;
                    }
                }
            }
        }     
    }
    return token;   // null, positional argument, or option with no value
}
   
//-------------------------------------------------------------------
// Terminate token, find next token, return pointer to current 
//-------------------------------------------------------------------

char *GetNextToken ()
{
    // Return NULL if no tokens
    if (*NxtTokenPtr == '\0') return NULL;

    // Save token pointer 
    char *token = NxtTokenPtr++;

    // Advance to end of token
    while (*NxtTokenPtr != SEPMARK) NxtTokenPtr++; 

    // If anything remains
    if (*NxtTokenPtr != '\0') { 

        // Terminate current token
        *NxtTokenPtr++ = '\0';

        // Skip past extra separaters to start of next
        while (*NxtTokenPtr == SEPMARK) NxtTokenPtr++; 
    }  

    // Return address of current token
    return token;
}
