#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pcre.h>

#include "parser.h"

#define TOKEN_REGEX     ( "%{(\\d+)([G]?|[S]\\d+)?}" )

int main(int argc, char ** argv)
{
    unsigned int j;
    unsigned int tokenCount;
    char** tokenArray;

    buildTokenArray(argv[1], &tokenArray, &tokenCount);

    // printf("%d\n", tokenCount);

    for (j = 0; j < tokenCount; j++ )
    {
        if (tokenArray[j][0] == '%')
        {
            // printf("%s\n", tokenArray[j]);
            /* This is a token capture sequence so we need to convert to a regex string */
            tokenToRegexString(tokenArray[j]);

        }
    }

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

void tokenToRegexString(const char* inputText)
{
    pcre *reCompiled;
    pcre_extra *pcreExtra;
    int pcreExecRet;
    int subStrVec[30];
    const char *pcreErrorStr;
    int pcreErrorOffset;
    const char* regexString = TOKEN_REGEX;
    const char *psubStrMatchStr;
    int j;

    /* Build regex to decipher token */
    reCompiled = pcre_compile(regexString, 0, &pcreErrorStr, &pcreErrorOffset, NULL);

    /* Check outcome of pcre_compile */
    if(reCompiled == NULL)
    {
        printf("ERROR: Could not compile '%s': %s\n", regexString, pcreErrorStr);
        exit(1);
    }

    printf("String to test: %s\n", inputText);

    /* Try to find the regex in inputText */
    pcreExecRet = pcre_exec(reCompiled,
                            NULL,
                            inputText,
                            (int)strlen(inputText), // length of string
                            0,                      // Start looking at this point
                            0,                      // OPTIONS
                            subStrVec,
                            30);                    // Length of subStrVec

    /* There was an error */
    if(pcreExecRet < 0)
    {
        switch(pcreExecRet)
        {
            case PCRE_ERROR_NOMATCH      : printf("String did not match the pattern\n");        break;
            case PCRE_ERROR_NULL         : printf("Something was null\n");                      break;
            case PCRE_ERROR_BADOPTION    : printf("A bad option was passed\n");                 break;
            case PCRE_ERROR_BADMAGIC     : printf("Magic number bad (compiled re corrupt?)\n"); break;
            case PCRE_ERROR_UNKNOWN_NODE : printf("Something kooky in the compiled re\n");      break;
            case PCRE_ERROR_NOMEMORY     : printf("Ran out of memory\n");                       break;
            default                      : printf("Unknown error\n");                           break;
        }
    }
    else
    {
        printf("Result: We have a match!\n");

        // PCRE contains a handy function to do the above for you:
        for(j=0; j<pcreExecRet; j++)
        {
            pcre_get_substring(inputText, subStrVec, pcreExecRet, j, &(psubStrMatchStr));
            printf("Match(%2d/%2d): (%2d,%2d): '%s'\n", j, pcreExecRet-1, subStrVec[j*2], subStrVec[j*2+1], psubStrMatchStr);
        }

        // Free up the substring
        pcre_free_substring(psubStrMatchStr);
    }

    printf("\n");

    // Free up the regular expression.
    pcre_free(reCompiled);

    // We are all done..
    return;
}
