#include "trie.h"

#include <string.h>

#include "ctest.h"

CTEST(trie, test1) {
    trieNode *dict = trieCreate();
    char word[1000];

    FILE *fp;

    fp = fopen("twl06.txt", "r");
    while (fscanf(fp, "%s", word) > 0) {
        trieInsert(dict, word);
    }
    fclose(fp);
}



    // // Test

    // fp = fopen(argv[1], "r");
    // while (fscanf(fp, "%s", word) > 0) {
    //     assert(trieSearch(dict, word) == 1 && "Search failed: Didn't find a word in the dictionary.");
    // }

    // char words[7][50] = {"ranran", "abc", "abcd", "adams", "wwu", "adamsitec", "adamsita"};

    // for (int i = 0; i < 7; ++i) {
    //     assert(trieSearch(dict, words[i]) == 0 && "Search failed: Found word that doesn't exist in dictionary");
    // }

    // return 0;
