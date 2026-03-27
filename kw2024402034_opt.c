#include <unistd.h>
#include <stdio.h>

extern int opterr;
extern char *optarg;
extern int optind;

/////////////////////////////////////////////////////////////////////////////////////////////////
// File Name : Main.c                                                                          //
// Date : 2026/03/26                                                                           //
// OS: Ubuntu 20.04.6 LTS 64bits                                                               //
// Author : Gwak Cheol-i                                                                       // 
// Student ID : 2024402034                                                                     //
// ------------------------------------------------------------------------------------------- //
// Title : System Programming Assignment #1-1 ( ftp server )                                   //
// Description :                                                                               //
// This program demonstrates how to parse command-line arguments                               //
// using the getopt() function in C.                                                           //
// It handles -a, -b, and -c options, where -c receives an additional argument via optarg.     //
// The parsing process is managed using optstring, optind,                                     //
// and optional error control through opterr.                                                  //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char **argv)
{
    int aflag = 0, bflag = 0;
    char *cvalue = NULL;
    int index, c;

    opterr = 0;
    
    while ((c = getopt (argc, argv, "abc:")) != -1){
        switch(c){
//////////////////////////////Row insert //////////////////////////////
            case 'a':
                aflag++;
                break;
            case 'b':
                bflag++;
                break;
            case 'c':
                cvalue = optarg;
                break;
            case '?':
                // invalid option, ignore
                break;
////////////////////////////// End of row insert //////////////////////////////
        }
    }

    printf("aflag = %d, bflag = %d, cvalue = %s\n", aflag, bflag, cvalue);
    
//////////////////////////////Row insert //////////////////////////////
    for(index = optind; index < argc; index++){
        printf("Non-option argument %s\n", argv[index]);
    }
////////////////////////////// End of row insert //////////////////////////////

    return 0;
}