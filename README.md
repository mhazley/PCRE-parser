# PCRE-parser
Programming problem for Alert Logic second stage interview. 

This program receives an input string
and turns this into an appropriate Regex
in order to perform matching using the
PCRE library.

eg: "foo %{0} is a %{1}"

The string can have modifiers:

%{#} in an input string will match any
characters in its place.

%{#S#} in an input string will match
any characters including a fixed number
of spaces in its place and it will capture.

%{#G} in an input string will match
any characters in its place and it will
capture.

### Building

Run `make` to build but be wary there may be linker issues with libpcre - I had some issues statically linking it on OS X and got it to work this way. You may need to simply add `-lpcre`. 

### Running

eg. `cat input.txt | ./pcre_parser "foo %{0} is a %{1}"`
