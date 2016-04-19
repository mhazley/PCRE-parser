PACKAGE = pcre_parser
VERSION = 1

# -ansi == ANSI C
# -std=iso9899:199409 == ANSI C w/ Amendment 1
# -std=c99 == ISO C99
GCC_CMD = gcc -g

RELEASE_FILE = $(PACKAGE)

GCC_COMPILE = $(GCC_CMD) -I/usr/local/include
GCC_LINK = $(GCC_CMD)
GCC_LINK_ALL = $(GCC_LINK) parser.o -lpcre
PCRE_LINK = /usr/local/lib/libpcre.a

all: clean build_main
				$(GCC_LINK_ALL) -o $(RELEASE_FILE) $(PCRE_LINK)

build_main:
				$(GCC_COMPILE) -c parser.c -o parser.o

clean:
				rm -f ./*.o
