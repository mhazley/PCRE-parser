void buildTokenArray(const char* inputText, char*** array, unsigned int* tokenCount);
char* tokenToRegexString(const char* inputText);
int doRegexMatch(const char** inputText, const char** regexString, int* subStrVec);
char* concat(char *stringOne, char *stringTwo);
