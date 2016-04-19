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
    char* oldRegExSegment = NULL;

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

        /* If we arent at the last segment.... */
        if (j < (tokenCount - 1))
        {
            /* ...add a space on at the end to complete the segment */
            regExSegment = concat(regExSegment, " ");
        }

        /* Now, join the whole segment onto the final regex */
        lineRegex = concat(lineRegex, regExSegment);

        /* Freeing the memory used in  main concatenation */
        if (oldLineRegex != NULL)
        {
            free(oldLineRegex);
        }
    }

    printf("Built the following regex: %s\n", lineRegex);

    // Run the match on std in
    matchStdIn(lineRegex);

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
    const char* regexForClosingWord = "[\\S]*)";
    const char* psubStrMatchStr;
    char* regexReturn = NULL;
    char* newString = NULL;
    char* oldString = NULL;

    int j;
    int tokenNumber;

    pcre* reCompiled;

    reCompiled = compileRegex(regexString);
    pcreExecRet = doRegexMatch(inputText, reCompiled, subStrVec);

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
            printf("ERROR: Duplicated token! It's best to make them sequential.\n");
            exit(1);
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
            printf("Token %d is a greedy modifier\n", tokenNumber);
            regexReturn = "([\\s\\S]*)";
        }
        else if (psubStrMatchStr[0] == 'S')
        {
            printf("Token %d is a space modifier\n", tokenNumber);

            /* Add a word and space to the regex for each space */
            for (j = 0; j < atoi(&psubStrMatchStr[1]); j++)
            {
                oldString = newString;

                if (newString == NULL) newString = "(";

                newString = concat(newString, (char*)regexForWordAndSpace);

                if (oldString != NULL)
                {
                    free(oldString);
                }
            }

            /* Add a final word to the regex */
            oldString = newString;
            newString = concat(newString, (char*)regexForClosingWord);

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
    else
    {
        printf("ERROR: The token does not match specified format\n");
        exit(1);
    }

    // Free up the regular expression.
    pcre_free(reCompiled);

    return regexReturn;
}

int doRegexMatch(const char* inputText, pcre* reCompiled, int* subStrVec)
{
    int ret;

    ret = pcre_exec(reCompiled,
                      NULL,
                      inputText,
                      (int)strlen(inputText),
                      0,
                      0,
                      subStrVec,
                      30);

    return ret;
}

pcre* compileRegex(const char* regexString)
{
    pcre* reCompiled;
    const char *pcreErrorStr;
    int pcreErrorOffset;

    /* Build regex to decipher token */
    reCompiled = pcre_compile(regexString, 0, &pcreErrorStr, &pcreErrorOffset, NULL);

    /* Check outcome of pcre_compile */
    if(reCompiled == NULL)
    {
        printf("ERROR: Could not compile regex '%s': %s\n", regexString, pcreErrorStr);
        exit(1);
    }

    return reCompiled;
}

void matchStdIn(char* regexString)
{
    int pcreExecRet;
    int subStrVec[30];
    pcre* reCompiled;
    char *line = NULL;
    size_t size;
    int j;
    const char* psubStrMatchStr;

    reCompiled = compileRegex(regexString);

    while (getline(&line, &size, stdin) != -1)
    {
        //printf("%s\n", line);
        pcreExecRet = doRegexMatch(line, reCompiled, subStrVec);

        /* There was a match */
        if(pcreExecRet > 0)
        {
            for(j = 0; j < pcreExecRet; j++)
            {
                pcre_get_substring(line, subStrVec, pcreExecRet, j, &(psubStrMatchStr));
                printf("Match(%2d/%2d): (%2d,%2d): '%s'\n", j, pcreExecRet-1, subStrVec[j*2], subStrVec[j*2+1], psubStrMatchStr);
            }
        }
    }

    pcre_free(reCompiled);
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
