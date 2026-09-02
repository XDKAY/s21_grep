#ifndef S21_GREP_H

#include <getopt.h>
#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  bool e;
  int i;
  bool v;
  bool c;
  bool l;
  bool n;
  bool h;
  bool s;
  bool f;
  bool o;
} GrepFlags;

typedef struct {
  int number_patterns;
  int capacity;
  char** list;
} GrepPatterns;

typedef struct {
  int number_files;
  int capacity;
  char** list;
} GrepFiles;

#endif