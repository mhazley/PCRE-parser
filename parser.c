#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pcre.h>

#include "parser.h"

int main(int argc, char ** argv)
{
    unsigned int j;
    unsigned int tokenCount;
    char** tokenArray;

    buildTokenArray(argv[1], &tokenArray, &tokenCount);

    // printf("%d\n", tokenCount);

    // for (j = 0; j < tokenCount; j++ )
    // {
    //     printf("%s\n", tokenArray[j]);
    // }

    return 0;
}

void buildTokenArray(const char* inputText, char*** tokenArray, unsigned int* tokenCount)
{
    const char* inputTextCopy;
    const char delimiters[] = " ";

    char* token;
    unsigned int j;

    *tokenCount = 0;

    inputTextCopy = strdup(inputText);

    /* get the first token */
    token = strtok((char *)inputText, delimiters);

    /* count the tokens */
    while( token != NULL )
    {
        (*tokenCount)++;
        token = strtok(NULL, delimiters);
    }

    /* Allocate array to hold the tokens */
    *tokenArray = (char**) malloc((*tokenCount) * sizeof(char*));

    /* Get the first token again using the copy... */
    token = strtok((char *)inputTextCopy, delimiters);

    /* Insert the tokens */
    //while( token != NULL )
    for (j = 0; j < (*tokenCount); j++)
    {
        if(token != NULL)
        {
            (*tokenArray)[j] = malloc(strlen(token) + 1);
            strcpy((*tokenArray)[j], token);
            token = strtok(NULL, delimiters);
        }
    }

    return;
}
