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
// This program is designed to learn how to handle command-line arguments                      //
// using the getopt() function in C and to implement behavior based on given options.          //
// The program supports -a, -b, and -c options, and checks whether each option is provided,    //
// then outputs the corresponding results.                                                     //
// In particular, the -c option requires an additional argument, which is received             //
// and processed through optarg.                                                               //
// The getopt() function determines valid options based on the optstring and                   //
// sequentially analyzes each option, while optind manages the index of the next               //
// argument to be processed.                                                                   //
// Additionally, by setting opterr to 0, the default error message for invalid options         //
// is suppressed, allowing custom error handling.                                              //
// Finally, after processing all options, the remaining non-option arguments are               //
// iterated and printed starting from the position indicated by optind.                        //
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