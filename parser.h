void buildTokenArray(const char* inputText, char*** array, unsigned int* tokenCount);
char* tokenToRegexString(const char* inputText);
int doRegexMatch(const char* inputText, pcre* reCompiled, int* subStrVec);
pcre* compileRegex(const char* regexString);
void matchStdIn(char* regexString);
char* concat(char *stringOne, char *stringTwo);
