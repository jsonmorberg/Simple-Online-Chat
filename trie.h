#include <stdbool.h>

typedef struct node {
    char letter;
    bool leaf;
    struct node *child;
    struct node *next;
} trieNode;

trieNode *trieCreate(void);
int trieInsert(trieNode *root, char *word);
int trieSearch(trieNode *root, char *word);
