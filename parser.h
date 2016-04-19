void buildTokenArray(const char* inputText, char*** array, unsigned int* tokenCount);
void tokenToRegexString(const char* inputText);
int doRegexMatch(const char** inputText, const char** regexString, int* subStrVec);
