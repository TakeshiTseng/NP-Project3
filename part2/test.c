#include <stdio.h>
#include "util.h"

int main(int argc, const char *argv[])
{
    char* str = "/blabla/abcde.cgi?efghi";
    printf("%d\n", is_match(str, ".*\\?.*"));
    return 0;
}
