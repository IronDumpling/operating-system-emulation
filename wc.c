#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "wc.h"

/*
 * Definition of the WordCount Hash table and its Node
 * you can define this struct to have whatever fields you want.
 * */
typedef struct Node{
    char* key;
    int value;
    struct Node* next;
} node;

struct wc {
    // Hash Table Node Array
    node** nodePointer;
    long nodeCapacity;
};

/*
 * Hash Function
 * */
unsigned int wc_hash(const char* key){
    unsigned int seed = 131;
    unsigned int hash = 0;
    while (*key){
        hash = hash * seed + (*key++);
    }
    return hash & 0x7FFFFFFF;
}

/*
 * Method 1
 * Initiate WordCount Hash Table
 * */
struct wc *
wc_init(char *word_array, long size)
{
	// Initialise WordCount Hash Table
    struct wc *wc;
	wc = (struct wc *)malloc(sizeof(struct wc));
	assert(wc);

	// Step 1. Initialize Node Array
    // Initialise the Node Array Parameters
    wc->nodeCapacity = size;
    // Allocate Memory
    wc->nodePointer = (node**)calloc(wc->nodeCapacity, sizeof(node*));

    // Step 2. Read Words From the Array
    for(long i = 0, j = 0; j < size;){
        // Use j to find the first space
        while(!isspace(word_array[j])) j++;

        // Store key
        char* currKey = (char*)malloc(sizeof(char) * (j-i+1));
        strncpy(currKey, word_array + i, j - i);

        // Use j to find the first non-space char
        while(isspace(word_array[j])) j++;
        // Traverse i to the here
        i = j;

        // Step 3. Insert Nodes
        // Find the hash position of the node
        long keyPosition = wc_hash(currKey) % wc->nodeCapacity;
        node* positionHead = wc->nodePointer[keyPosition];

        // Case 1. New Node Insert as the head of the list
        if(!positionHead){
            node* newNode = (node*)malloc(sizeof(node));
            newNode->value = 1;
            newNode->key = (char*)malloc(sizeof(char)*(strlen(currKey) + 1));
            strcpy(newNode->key, currKey);
            // Attach to the list
            wc->nodePointer[keyPosition] = newNode;
        }else{
            // Case 2. The key exists
            // Check if this key exists in the list of position
            bool exists = false;
            node *prev = NULL;
            while(positionHead){
                if(strcmp(positionHead->key, currKey) == 0){
                    positionHead->value++;
                    exists = true;
                    break;
                }
                // Traverse
                prev = positionHead;
                positionHead = positionHead->next;
            }

            // Case 3. New Node which has collision
            if(!exists){
                node* newNode = (node*)malloc(sizeof(node));
                newNode->value = 1;
                newNode->key = (char*)malloc(sizeof(char)*(strlen(currKey) + 1));
                strcpy(newNode->key, currKey);
                // Attach to the list
                prev->next = newNode;
            }
        }
    }
	return wc;
}

/*
 * Method 2
 * Print WordCount Hash Table
 * */
void
wc_output(struct wc *wc)
{
	for(int i = 0; i < wc->nodeCapacity; i++){
	    if(wc->nodePointer[i]){
	        node *pHead = wc->nodePointer[i];
	        while(pHead){
                printf("%s:%d\n", pHead->key, pHead->value);
                pHead = pHead->next;
	        }
	    }
	}
}

/*
 * Method 3
 * Destroy WordCount Hash Table
 * */
void
wc_destroy(struct wc *wc)
{
	for(int i = 0; i < wc->nodeCapacity; i++){
	    if(wc->nodePointer[i]){
            node *pHead = wc->nodePointer[i];
            while(pHead){
                node* pTemp = pHead;
                pHead = pHead->next;
                free(pTemp->key);
                free(pTemp);
            }
	    }
	}
	free(wc->nodePointer);
	free(wc);
}
