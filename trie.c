/* trie.c - code for trie data structure.

Reference: https://www.cs.bu.edu/teaching/c/tree/trie/

*/


#include "trie.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


/**
 * Create an empty trie with one node
 *
 * @return root node of an empty trie
 */
trieNode *trieCreate(void) {
    trieNode *n;

    n = malloc(sizeof(trieNode));
    n->child = NULL;
    n->next = NULL;
    n->leaf = false;

    return n;
}

/**
 * Insert word in a given trie.
 * @param root node of a trie
 * @param word to insert
 * @return 1 on success, 0 on failure.
 */
int trieInsert(trieNode *root, char *word) {
    if (root == NULL || word == NULL) {
        return 0;
    }
    trieNode *level = root;
    trieNode *curr;
    trieNode *match = NULL;
    for (int i = 0; i < strlen(word); ++i) {
        match = NULL;
        for (curr = level->child; curr != NULL ; curr = curr->next) {
            if (curr->letter == word[i]) {
                match = curr;
                break;
            }
        }
        if (match == NULL) {
            // Didn't find a match for word[i]. Add letter at this level
            trieNode *new = trieCreate();
            new->letter = word[i];
            new->next = level->child;
            level->child = new;
            match = new;
        }
        level = match;
    }
    if (match == NULL) {
        return 0;
    }
    match->leaf = true;
    return 1;
}


/**
 * @param root node of a trie
 * @param word
 * @return 1 if the word exists in the trie, otherwise returns 0
 */
int trieSearch(trieNode *root, char *word) {
    trieNode *level = root->child;
    trieNode *curr, *match;
    for (int i = 0; i < strlen(word); ++i) {
        match = NULL;
        for (curr = level; curr != NULL ; curr = curr->next) {
            if (curr->letter == word[i]) {
                match = curr;
                break;
            }
        }
        if (match == NULL) {
            // Didn't find a match for word[i] at level i+1. Word doesn't exist in the given trie
            return 0;
        }
        level = match->child;
    }
    if (match != NULL && match->leaf) {
        return 1;
    }
    return 0;
}


