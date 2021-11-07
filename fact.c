#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Factorial Function
int fact(int n){
    if(n <=1) return 1;
    else return n * fact(n-1);
}

// Main Function
int
main(int argc, char **argv)
{
    bool isCase1 = true;

    // Trivial Detection:
    if(strlen(argv[1]) == 1 && argv[1][0] == '0'){
        isCase1 = false;
    }

    // Other Detections:
    for(int i = 0; i < strlen(argv[1]); i++){
        if(argv[1][i] == '-' || argv[1][i] == '.' || argv[1][i] - 57 > 0){
            isCase1 = false;
            break;
        }
    }

	// Case 1. Normal case: pass in positive integer <= 12
	if(isCase1){
        // Modify string into numbers
        int num = atoi(argv[1]);

        // Case 2. Too Large: pass in positive integer > 12
        if(num > 12){
            printf("Overflow\n");
        }else{
            num = fact(num);
            printf("%d\n", num);
        }
	}
	// Case 3. No arguments, non-positive integer
	else{
	    printf("Huh?\n");
	}

	return 0;
}
