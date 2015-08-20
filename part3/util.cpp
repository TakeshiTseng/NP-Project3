#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <regex>

using namespace std;





void str_split(char* str, const char* tok, char*** res, int* count) {

    // To avoid Segmentation fault we need to convert to char array....
    // Maybe I need to write a new strtok function?

	// WTF!
	char tmp_tok[10];
	memset(tmp_tok, 0, 10);
	strcpy(tmp_tok, tok);

    char buff[4096];
	memset(buff, 0, 4096);
    strcpy(buff, str);

	

    // some var for split
    int _count = 0;
    char* _res[5000];
    char* tmp_str;
	

    tmp_str = strtok(buff, tmp_tok);

    while(tmp_str != NULL) {
        int _len = strlen(tmp_str) + 1;
        _res[_count] = (char*)malloc(sizeof(char)*_len);
        strcpy(_res[_count], tmp_str);
		
        _count++;
		tmp_str = strtok(NULL, tmp_tok);
    }

    *count = _count;
    *res = _res;

}

int is_match(const char* str, char* regex_str) {
	std::string s(str);
	std::regex e(regex_str);

	if (std::regex_match(s, e)) {
		return 1;
	}
	else {
		return 0;
	}
}

void str_replace_one_world(char* str, char match, char to_replace) {
    int c=0;
    while(str[c] != '\0') {
        if(str[c] == match) {
            str[c] = to_replace;
        }
        c++;
    }
}

int count_char_num(char* str, char c) {
    int count = 0;
    while(*str != '\0') {
        if(*str == c) {
            count++;
        }
        str++;
    }
    return count;
}

int str_ends_with(char* str, char c) {
    int len = strlen(str);
    return str[len-1] == c;
}

int str_starts_with(char* str, char c) {
    return str[0] == c;
}
void replace_to_html(char* str) {

    int c, d;
    c = 0;
    d = 0;
    char tmp_str[10001];
    memset(tmp_str, 0, 10001);
    while(str[c] != '\0') {
        if(str[c] == '\n') {
            tmp_str[d++] = '<';
            tmp_str[d++] = 'b';
            tmp_str[d++] = 'r';
            tmp_str[d++] = '>';
            c++;
        } else if(str[c] == '\r') {
            c++;
        } else if(str[c] == '<') {
            tmp_str[d++] = '&';
            tmp_str[d++] = 'l';
            tmp_str[d++] = 't';
            tmp_str[d++] = ';';
            c++;
        } else if(str[c] == '>') {
            tmp_str[d++] = '&';
            tmp_str[d++] = 'g';
            tmp_str[d++] = 't';
            tmp_str[d++] = ';';
            c++;
        } else {
            tmp_str[d++] = str[c++];
        }
    }
    strcpy(str, tmp_str);

}
