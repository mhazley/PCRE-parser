/*
 * parser.c
 *
 * This program receives an input string
 * and turns this into an appropriate Regex
 * in order to perform matching using the
 * PCRE library.
 *
 * The string can have modifiers:
 *
 * %{#} in an input string will match any
 * characters in its place.
 *
 * %{#S#} in an input string will match
 * any characters including a fixed number
 * of spaces in its place and it will capture.
 *
 * %{#G} in an input string will match
 * any characters in its place and it will
 * capture.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pcre.h>

#include "parser.h"

#define TOKEN_REGEX     ( "%{(\\d+)([G]?|[S]\\d+)?}" )

/* We can match up to 32 groups - each group is masked into this unsigned int */
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

    /* Tokenise the input string - easier to work with */
    buildTokenArray(argv[1], &tokenArray, &tokenCount);

    /* Loop the tokens - need to find out which are modifiers */
    for (j = 0; j < tokenCount; j++ )
    {
        /* Bit dirty - used for memory management in string concatenation */
        oldLineRegex = lineRegex;
        if (lineRegex == NULL) lineRegex = "";

        /* Use the % chatacter to identify a modifer */
        if (tokenArray[j][0] == '%')
        {
            /* This is a modifier so we need to convert to a regex */
            regExSegment = tokenToRegexString(tokenArray[j]);
        }
        else
        {   /* Not a modifer so no need to convert */
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

        /* Freeing the memory used in concatenation */
        if (oldLineRegex != NULL)
        {
            free(oldLineRegex);
        }
    }

    printf("Built the following regex: %s\n", lineRegex);

    // Run the match on std in lines
    matchStdIn(lineRegex);

    return 0;
}

/*
 * This function tokenises an input string using the <space> delimiter.
 * It returns the count and an array of tokens by reference.
 */
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

/*
 * This function takes a modifer token and converts it to a regex.
 *
 * %{#} will match any characters in its place.
 *
 * %{#S#} will match any characters including a fixed number
 * of spaces in its place and it will capture.
 *
 * %{#G} will match any characters in its place and it will
 * capture.
 *
 * It returns a string which is the regex.
 */
char* tokenToRegexString(const char* inputText)
{
    int pcreExecRet;
    int subStrVec[30];
    const char* regexString = TOKEN_REGEX;
    const char* regexForWordAndSpace = "[\\S]{1,}[\\s]";
    const char* regexForClosingWord = "[\\S]{1,})";
    const char* psubStrMatchStr;
    char* regexReturn = NULL;
    char* newString = NULL;
    char* oldString = NULL;
    int j;
    int tokenNumber;
    pcre* reCompiled;

    /* Build a regex to match our modifiers with and attempt to match */
    reCompiled = compileRegex(regexString);
    pcreExecRet = doRegexMatch(inputText, reCompiled, subStrVec);

    /* There was a match */
    if(pcreExecRet > 0)
    {
        /* First make sure we haven't already got a token with this index */
        /* Look at group 1 capture to inspect */
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
            /* Greedy so we need to capture any character in the modifiers place */
            printf("Token %d is a greedy modifier\n", tokenNumber);
            regexReturn = "([\\s\\S]{1,})";
        }
        else if (psubStrMatchStr[0] == 'S')
        {
            /* Space so we need to capture any character in the modifiers place
             * ONLY if there are a certain number of spaces.
             *
             *  Note: This works but its dirty I think - couldnt work out a regex
             *  to search for 'any character with a certain number of non-consecutive
             *  spaces' so I have (very) simply build up a regex including a fixed
             *  number of spaces.
             */
            printf("Token %d is a space modifier\n", tokenNumber);

            /* Add a word and space to the regex for each space */
            for (j = 0; j < atoi(&psubStrMatchStr[1]); j++)
            {
                /* Used for memory management when concatenating */
                oldString = newString;

                if (newString == NULL) newString = "(";

                /* Add a regexForWordAndSpace on to the output string */
                newString = concat(newString, (char*)regexForWordAndSpace);

                /* Freeing the old string used for memory management */
                if (oldString != NULL)
                {
                    free(oldString);
                }
            }


            oldString = newString;
            /* This handles the case where no spaces are wanted */
            if (newString == NULL) newString = "(";
            /* Add a final word and bracket to the regex */
            newString = concat(newString, (char*)regexForClosingWord);

            /* Freeing the old string used for memory management */
            if (oldString != NULL)
            {
                free(oldString);
            }

            /* Set final return string to the completed regex */
            regexReturn = newString;
        }
        else
        {
            /* Token is just basic modifier - no capture required, any amount of any character accepted */
            printf("Token %d requires no capture\n", tokenNumber);
            regexReturn = "[\\s\\S]{1,}";
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

/*
 * Simply carries out the regex matching.
 *
 * Takes a compiled PCRE regex and an input string and returns the match
 * and also the match vector via reference.
 */
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

/*
 * Simply builds a PCRE regex.
 *
 * Takes a text string of the regex as an argument and returns
 * pointer to the PCRE regex.
 */
pcre* compileRegex(const char* regexString)
{
    pcre* reCompiled;
    const char *pcreErrorStr;
    int pcreErrorOffset;

    /* Build regex from string*/
    reCompiled = pcre_compile(regexString, 0, &pcreErrorStr, &pcreErrorOffset, NULL);

    /* Check outcome of pcre_compile */
    if(reCompiled == NULL)
    {
        printf("ERROR: Could not compile regex '%s': %s\n", regexString, pcreErrorStr);
        exit(1);
    }

    return reCompiled;
}

/*
 * Matches lines on stdin to a regex until EOF reached.
 *
 * Takes a text string of the regex as an argument.
 */
void matchStdIn(char* regexString)
{
    int pcreExecRet;
    int subStrVec[30];
    pcre* reCompiled;
    char *line = NULL;
    size_t size;
    int j;
    const char* psubStrMatchStr;

    /* Build the regex */
    reCompiled = compileRegex(regexString);

    /* Keep getting lines until there are none */
    while (getline(&line, &size, stdin) != -1)
    {
        /* Attempt to match the line with compiled regex */
        pcreExecRet = doRegexMatch(line, reCompiled, subStrVec);

        /* There was a match! */
        if(pcreExecRet > 0)
        {
            printf("\n");

            /* Do the output */
            for(j = 0; j < pcreExecRet; j++)
            {
                pcre_get_substring(line, subStrVec, pcreExecRet, j, &(psubStrMatchStr));
                if (j == 0)
                {
                    printf("Matched:\t %s", psubStrMatchStr);
                }
                else
                {
                    printf("Captured:\t %s\n", psubStrMatchStr);
                }

            }
        }
    }

    /* Free the compiled regex - we are done */
    pcre_free(reCompiled);

    return;
}

/*
 * Simple concatenation function for two strings.
 */
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
