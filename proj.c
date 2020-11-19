#include <stdio.h>
#include <string.h>
#include <ctype.h>
int checkWord(char *word){
    int len = strlen(word);
    
    if(len < 1 || len > 10){
        return 0;
    } 

    for(int i = 0; i < len; i++){
        char letter = word[i];
        if(!isalpha(letter) && letter != '_' && !isdigit(letter)){
            return 0;
        }
    }    
    
    return 1;
}
int main()
{ 
    char *valid = "C_10_abcaa";
    char *i1 = "";
    char *i2 = "casj$";
    char *i3 = "aa aa";
    char *i4 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

    printf("%d, %d, %d, %d, %d\n", checkWord(valid), checkWord(i1), checkWord(i2), checkWord(i3), checkWord(i4));
    printf("1, 0, 0, 0, 0\n");
    return(0); 
} 
