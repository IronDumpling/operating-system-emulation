#include <errno.h>
#include "common.h"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
/* make sure to use syserror() when a system call fails. see common.h */
// Function 0. Input Handling
void usage(){
	fprintf(stderr, "Usage: cpr srcdir dstdir\n");
	exit(1);
}

// Function 1. Check If it's Directory
int isDirectory(char *path){
    struct stat state;
    stat(path, &state);

    // Check if it's a directory
    if(S_ISDIR(state.st_mode)) return 1;
    else return 0;
}

// Function 2. Check if path is end with '/'
int endWith(char *s, char c){
    if(s[strlen(s) - 1] == c) return 1;
    else return 0;
}

// Function 3. Append String
char* strAppend(char *prefix, char *suffix){
    // Initialize Result
    char *result = (char*)malloc(strlen(prefix) + strlen(suffix) + 1);

    // Allocate Memory Error
    if(!result) exit(1);

    // Append String
    strcat(result, prefix);
    strcat(result, suffix);

    return result;
}

// Function 4. Copy File
void copyFile(char *sourcePath, char *destinationPath){
    char buffer
}

// Function 5. Copy Directory
void copyFolder(char *sourcePath, char *destinationPath){
    // Step 1. Open Destination
    if(!opendir(destinationPath)){
        // Make Directory if it doesn't exist
        if(mkdir(destinationPath, 0777)){
            // If Error Happens
            syserror(mkdir, destinationPath);
        }

        //
        char *path = (char*)malloc(512);
        path =
    }

    // Step 2.
}

int main(int argc, char *argv[]){
    // Input Error Handling
	if (argc != 3) {
		usage();
	}

	// Fetch Directory And copy recursively
    copyFolder(argv[1], argv[2]);

	return 0;
}
