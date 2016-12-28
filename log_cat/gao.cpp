/* **********************************************

  File Name: gao.cpp

  Author: zhengdongjian@tju.edu.cn

  Created Time: Tue, Dec 27, 2016  2:14:23 PM

*********************************************** */
#include <cstdio>
#include <cstring>
#include <cctype>
#include <stdlib.h>

#include "libxl.h"
using namespace libxl;

const int MAX = 10086;  // line buffer size
char buffer[MAX];  // line buffer

const char HEADER[] = "Ethernet";  // paragraph header
const char HEADER_PATTERNS[2][256] = {
	"Input bandwidth utilization(%)",
	"Output bandwidth utilization(%)",
	// "Last 300 seconds input rate(bits/s)",
	// "Last 300 seconds output rate(bits/s)",
};
const char PATTERNS[2][256] = {
	/* all possible patterns */
	"Input bandwidth utilization",
	"Output bandwidth utilization",
	// "Last 300 seconds input rate",
	// "Last 300 seconds output rate",
};
const int PATTERN_NUM = sizeof(PATTERNS) / 256;

char* match_str(char* str, const char* pattern) {
	while (*pattern != '\0') {
		if (*str == '\0' || *pattern != *str) {
			return NULL;
		}
		++pattern;
		++str;
	}
	return str;
}

char* match_num(char* str) {
	bool flag = false;
	while (*str != '\0' && isdigit(*str)) {
		flag = true;
		++str;
	}
	return flag ? str : NULL;
}

//Eth-Trunk4.3 current state
//Eth-Trunk4 current state
//GigabitEthernet0/0/0 current state
//Ethernet0/0/0 current state
//~~Aux0/0/1 current state~~
bool match_ether(char* str) {
	char* tmp = match_str(str, "Ethernet");
	tmp || (tmp = match_str(str, "GigabitEthernet"));
	// tmp || (tmp = match_str(str, "Aux"));
	tmp && (tmp = match_num(tmp));
	tmp && (tmp = match_str(tmp, "/"));
	tmp && (tmp = match_num(tmp));
	tmp && (tmp = match_str(tmp, "/"));
	tmp && (tmp = match_num(tmp));
	tmp && (tmp = match_str(tmp, " current state"));
	return tmp != NULL;
}

bool match_trunk(char* str) {
	char* tmp = match_str(str, "Eth-Trunk");
	tmp && (tmp = match_num(tmp));
	if (tmp != NULL) {
		char* tmp2 = match_str(tmp, ".");
		if (tmp2 != NULL) {
			tmp = tmp2;
			tmp = match_num(tmp);
		}
	}
	tmp && (tmp = match_str(tmp, " current state"));
	return tmp != NULL;
}

bool match_header(char* line) {
	return match_ether(line) || match_trunk(line);
}

void gao(FILE* fd, Sheet* sheet) {
	sheet->writeStr(1, 0, "Device");
	for (int i = 0; i < PATTERN_NUM; ++i) {
		sheet->writeStr(1, i + 1, HEADER_PATTERNS[i]);
	}
	int device_id = 1;
	while (fgets(buffer, MAX, fd) != NULL) {
		if (match_header(buffer)) {
			/* we meet a new paragraph */
			char* end = buffer;
			while (*end != ' ' && *end != '\0') {
				++end;
			}
			*end = '\0';
			++device_id;
			sheet->writeStr(device_id, 0, buffer);
			printf("---- Device: %s ----\n", buffer);
			*end = ' ';
			continue;
		}
		
		for (int i = 0; i < PATTERN_NUM; ++i) {
			const char* pattern = PATTERNS[i];
			char* sub = strstr(buffer, pattern);
			//printf("try [%s] match pattern [%s]...\n", buffer, pattern);
			if (sub != NULL) {
				//printf("[%s] match pattern [%s]\n", buffer, pattern);
				while (!isdigit(*sub)) {
					++sub;
				}
				double rate = 0.0;
				while (isdigit(*sub)) {
					rate *= 10.0;
					rate += *sub - '0';
					++sub;
				}
				if (*sub == '.') {
					double tmp = 1.0;
					sub++;
					while (isdigit(*sub)) {
						tmp *= 0.1;
						rate += tmp * (*sub - '0');
						++sub;
					}
				}
				/* do anything needed with 'rate' here */
				sheet->writeNum(device_id, i + 1, rate);
				if (pattern[0] == 'L') {
					/* pattern(s) like: Last xxx */
					printf("%s: %.0f bits/s\n", pattern, rate);
				} else {
					/* pattern(s) like: Input/Output xxx */
					printf("%s: %.2f%%\n", pattern, rate);
				}
			}
		}
	}
	sheet->setAutoFitArea(0, 0, device_id + 1, PATTERN_NUM + 1);
}

char* parse_file_name(char* file_name) {
    char* s = file_name + strlen(file_name);
    while (s != file_name && *s != '/') {
        --s;
    }
    if (s != file_name) {
        ++s;
    }
    return s;
}

int main(int argc, char** argv) {
	Book* book = xlCreateBook();  // a new xlsx book :)
	if (book == NULL) {
		printf("o.o something is wrong\n");
		exit(1);
	}

    char file_name[1024];
	if (argc == 1) {
		/* no file name(s) specificed, read from stdin */
		int id = 1;
		while (true) {
			printf("Please input file name. one line each, press Enter directly to terminate...\n");
			fgets(file_name, 1024, stdin);
			int len = strlen(file_name);
			if (len <= 1) {
				break;
			}
			file_name[strlen(file_name) - 1] = '\0';
			Sheet* sheet = book->addSheet(parse_file_name(file_name));
			if (sheet == NULL) {
				printf("T.T something is wrong\n");
				exit(2);
			}
			printf("\tid = %d, file name: %s...\n", id++, file_name);
			FILE* fd = fopen(file_name, "r");
            if (fd == NULL) {
                strcpy(file_name + strlen(file_name), ".txt");
                fd = fopen(file_name, "r");
            }
			if (fd == NULL) {
				printf("open file %s failed...\n", file_name);
				continue;
			}
			gao(fd, sheet);
			fclose(fd);
			printf("--------------------------------------------\n");
		}
		if (id == 1) {
			exit(0);
		}
	} else {
		/* handle each of args */
		for (int i = 1; i < argc; ++i) {
			Sheet* sheet = book->addSheet(parse_file_name(argv[i]));
			if (sheet == NULL) {
				printf("T.T something is wrong\n");
				exit(2);
			}
			printf("\tid = %d, file name: %s...\n", i, argv[i]);
			FILE* fd = fopen(argv[i], "r");
            if (fd == NULL) {
                strcpy(file_name, argv[i]);
                strcpy(file_name + strlen(file_name), ".txt");
                fd = fopen(file_name, "r");
            }
			if (fd == NULL) {
				printf("open file %s failed...\n", file_name);
				continue;
			}
			gao(fd, sheet);
			fclose(fd);
			printf("--------------------------------------------\n");
		}
	}
	book->save("gao.xls");
	book->release(); 
	return 0;
}
