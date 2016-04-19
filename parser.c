#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pcre.h>

#include "parser.h"

#define TOKEN_REGEX     ( "%{(\\d+)([G]?|[S]\\d+)?}" )

// We can match up to <wordsize> groups
static unsigned int tokenMask = 0;

int main(int argc, char ** argv)
{
    unsigned int j;
    unsigned int tokenCount;
    char** tokenArray;
    char* lineRegex = NULL;
    char* oldLineRegex = NULL;
    char* regExSegment = NULL;

    buildTokenArray(argv[1], &tokenArray, &tokenCount);

    // printf("%d\n", tokenCount);

    for (j = 0; j < tokenCount; j++ )
    {

        oldLineRegex = lineRegex;

        if (lineRegex == NULL) lineRegex = "";

        if (tokenArray[j][0] == '%')
        {
            /* This is a token capture sequence so we need to convert to a regex string */
            // printf("%s\n", tokenToRegexString(tokenArray[j]));
            regExSegment = tokenToRegexString(tokenArray[j]);
        }
        else
        {
            regExSegment = tokenArray[j];
        }

        lineRegex = concat(lineRegex, regExSegment);

        /* Freeing the memory used in concatenation */
        if (oldLineRegex != NULL)
        {
            free(oldLineRegex);
        }
    }

    printf("%s\n", lineRegex);

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

char* tokenToRegexString(const char* inputText)
{
    int pcreExecRet;
    int subStrVec[30];
    const char* regexString = TOKEN_REGEX;
    const char* regexForWordAndSpace = "[\\S]*[\\s]";
    const char* regexForWord = "[\\S]*";
    const char* psubStrMatchStr;
    char* regexReturn = NULL;
    char* newString = NULL;
    char* oldString = NULL;

    int j;
    int tokenNumber;

    pcreExecRet = doRegexMatch(&inputText, &regexString, subStrVec);

    /* There was a match */
    if(pcreExecRet > 0)
    {
        /* First make sure we haven't already got a token with this index */
        /* Look at group 1 to inspect */
        pcre_get_substring(inputText, subStrVec, pcreExecRet, 1, &(psubStrMatchStr));

        tokenNumber = atoi(psubStrMatchStr);

        if( (tokenMask & (0x01 << tokenNumber ) ) != 0)
        {
            /* We already have this token */
            printf("%s\n", psubStrMatchStr);
            printf("Duped token %d\n", tokenMask);
            return regexReturn;
        }
        else
        {
            /* Mark this token in the mask */
            tokenMask = tokenMask | (0x01 << tokenNumber);
        }

        /* Lets look at group 2 to see if we have a capture modifier */
        pcre_get_substring(inputText, subStrVec, pcreExecRet, 2, &(psubStrMatchStr));
        if (psubStrMatchStr[0] == 'G')
        {
            printf("Token %d is agreedy modifier\n", tokenNumber);
            regexReturn = "([\\s\\S]*)";
        }
        else if (psubStrMatchStr[0] == 'S')
        {
            printf("Token %d is a space modifier\n", tokenNumber);

            /* Add a word and space to the regex for each space */
            for (j = 0; j < atoi(&psubStrMatchStr[1]); j++)
            {
                oldString = newString;

                if (newString == NULL) newString = "";

                newString = concat(newString, (char*)regexForWordAndSpace);

                if (oldString != NULL)
                {
                    free(oldString);
                }
            }

            /* Add a final word to the regex */
            oldString = newString;
            newString = concat(newString, (char*)regexForWord);

            if (oldString != NULL)
            {
                free(oldString);
            }

            /* Set final return string to the completed regex */
            regexReturn = newString;
        }
        else
        {
            printf("Token %d requires no capture\n", tokenNumber);
            regexReturn = "[\\s\\S]*";
        }

        // Free up the substring
        pcre_free_substring(psubStrMatchStr);
    }

    return regexReturn;
}

int doRegexMatch(const char** inputText, const char** regexString, int* subStrVec)
{
    int ret;
    pcre *reCompiled;
    const char *pcreErrorStr;
    int pcreErrorOffset;

    /* Build regex to decipher token */
    reCompiled = pcre_compile(*regexString, 0, &pcreErrorStr, &pcreErrorOffset, NULL);

    /* Check outcome of pcre_compile */
    if(reCompiled == NULL)
    {
        printf("ERROR: Could not compile regex '%s': %s\n", *regexString, pcreErrorStr);
        return PCRE_ERROR_BADOPTION;
    }

    ret = pcre_exec(reCompiled,
                      NULL,
                      *inputText,
                      (int)strlen(*inputText),
                      0,
                      0,
                      subStrVec,
                      30);

    // Free up the regular expression.
    pcre_free(reCompiled);

    return ret;

}

char* concat(char *stringOne, char *stringTwo)
{
    size_t len1 = strlen(stringOne);
    size_t len2 = strlen(stringTwo);

    char* result = malloc(len1+len2+1);//+1 for the zero-terminator

    /* memcpy to new string */
    memcpy(result, stringOne, len1);
    memcpy(result+len1, stringTwo, (len2 + 1));

    return result;
}
