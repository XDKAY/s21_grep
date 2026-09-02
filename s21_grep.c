#define _GNU_SOURCE

#include "s21_grep.h"

#define REG_BASIC 0

/* === Работа с аргументами командной строки === */

// Установка флагов
void set_flag(int flag, GrepFlags* Flags, bool* is_valid_flag) {
  switch (flag) {
    case 'e':
      Flags->e = true;
      break;
    case 'i':
      Flags->i = REG_ICASE;
      break;
    case 'v':
      Flags->v = true;
      break;
    case 'c':
      Flags->c = true;
      Flags->n = false;
      break;
    case 'l':
      Flags->l = true;
      break;
    case 'n':
      if (!Flags->c) Flags->n = true;
      break;
    case 'h':
      Flags->h = true;
      break;
    case 's':
      Flags->s = true;
      break;
    case 'f':
      Flags->f = true;
      break;
    case 'o':
      Flags->o = true;
      break;
    default:
      *is_valid_flag = false;
  }
}

// Парсинг флагов
bool parsing_flags(int flag, GrepFlags* Flags) {
  bool is_valid_flag = true;
  set_flag(flag, Flags, &is_valid_flag);
  return is_valid_flag;
}

// Аллокация памяти
char** allocate_memory(int capacity) {
  char** array = calloc(capacity, sizeof(char*));
  return array;
}

// Реаллокация памяти
void reallocate_memory(char*** array, int number_values, int* capacity) {
  *capacity = number_values * 2;
  *array = realloc(*array, *capacity * sizeof(char*));
}

// Парсинг паттернов
void parsing_patterns(char* pattern, GrepPatterns* Patterns) {
  if (pattern == NULL) return;
  if (Patterns->list == NULL) {
    Patterns->capacity = 1;
    Patterns->list = allocate_memory(Patterns->capacity);
  } else if (Patterns->number_patterns >= Patterns->capacity) {
    reallocate_memory(&Patterns->list, Patterns->number_patterns,
                      &Patterns->capacity);
  }
  if (Patterns->list != NULL) {
    Patterns->list[Patterns->number_patterns] = malloc(strlen(pattern) + 1);
    if (Patterns->list[Patterns->number_patterns] != NULL) {
      strcpy(Patterns->list[Patterns->number_patterns], pattern);
      Patterns->number_patterns++;
    }
  }
}

// Парсинг файлов
void parsing_files(char* file, GrepFiles* Files) {
  if (file == NULL) return;
  if (Files->list == NULL) {
    Files->capacity = 1;
    Files->list = allocate_memory(Files->capacity);
  } else if (Files->number_files >= Files->capacity) {
    reallocate_memory(&Files->list, Files->number_files, &Files->capacity);
  }
  if (Files->list != NULL) {
    Files->list[Files->number_files] = malloc(strlen(file) + 1);
    if (Files->list[Files->number_files] != NULL) {
      strcpy(Files->list[Files->number_files], file);
      Files->number_files++;
    }
  }
}

void parsing_regex_file(char* filename, GrepPatterns* Patterns,
                        int* grep_status) {
  FILE* stream = fopen(filename, "r");
  if (stream != NULL) {
    char* pattern = NULL;
    size_t len = 0;
    while (getline(&pattern, &len, stream) != -1) {
      pattern[strcspn(pattern, "\r\n")] = '\0';
      parsing_patterns(pattern, Patterns);
    }
    free(pattern);
    fclose(stream);
  } else {
    fprintf(stderr, "File not found\n");
    *grep_status = 2;
  }
}

// Парсинг аргументов
void parsing_arguments(int argc, char* argv[], GrepFlags* Flags,
                       GrepPatterns* Patterns, GrepFiles* Files,
                       int* grep_status) {
  int flag;
  bool is_valid_flag = true, next_file = false;
  while ((flag = getopt(argc, argv, "e:ivclnhsf:o")) != -1 && is_valid_flag) {
    is_valid_flag = parsing_flags(flag, Flags);
    if (flag == 'e') {
      parsing_patterns(optarg, Patterns);
    } else if (flag == 'f') {
      parsing_regex_file(optarg, Patterns, grep_status);
    }
  }
  for (int i = optind; i < argc && is_valid_flag; i++) {
    if (argv[i][0] == '-')
      ;
    else {
      if (Flags->e || Flags->f)
        parsing_files(argv[i], Files);
      else {
        if (!next_file) {
          parsing_patterns(argv[i], Patterns);
          next_file = true;
        } else
          parsing_files(argv[i], Files);
      }
    }
  }
}

// Очиста структтуры паттернов
void free_Patterns(GrepPatterns* Patterns) {
  for (int i = 0; i < Patterns->number_patterns; i++) {
    free(Patterns->list[i]);
  }
  free(Patterns->list);
}

// Очиста структуры файлов
void free_Files(GrepFiles* Files) {
  for (int i = 0; i < Files->number_files; i++) {
    free(Files->list[i]);
  }
  free(Files->list);
}

/* === Работа с шаблонами === */

// Заполнение шаблона
void filling_regex_pattern(char* regex_pattern, const char* pattern,
                           int* current_position) {
  regex_pattern[(*current_position)++] = '\\';
  regex_pattern[(*current_position)++] = '(';
  for (size_t j = 0; j < strlen(pattern); j++) {
    regex_pattern[(*current_position)++] = pattern[j];
  }
  regex_pattern[(*current_position)++] = '\\';
  regex_pattern[(*current_position)++] = ')';
}

// Получение итогово шаблона
char* build_regex_pattern(GrepPatterns* Patterns) {
  if (Patterns->number_patterns <= 0) return NULL;
  char* regex_pattern = NULL;
  int size = 0;
  for (int i = 0; i < Patterns->number_patterns; i++) {
    size += 4 + strlen(Patterns->list[i]);  // Для \(pattern_1\)
  }
  size += (Patterns->number_patterns - 1) * 2;  // Для \|
  size += 1;                                    // Для \0
  regex_pattern = malloc((size_t)size);
  if (regex_pattern != NULL) {
    int current_position = 0;
    for (int i = 0; i < Patterns->number_patterns; i++) {
      const char* pattern = Patterns->list[i];
      if (i > 0) {
        regex_pattern[current_position++] = '\\';
        regex_pattern[current_position++] = '|';
      }
      filling_regex_pattern(regex_pattern, pattern, &current_position);
    }
    regex_pattern[current_position++] = '\0';
  }
  return regex_pattern;
}

/* === Работа с файлами и выводом == */

FILE* get_stream(const char* filename) {
  FILE* stream = filename == NULL ? stdin : fopen(filename, "r");
  return stream;
}

// Стримим файл если флаг l вкл
void process_file_for_flag_l_no_patterns(const char* filename, GrepFlags* Flags,
                                         int* grep_status) {
  FILE* stream = get_stream(filename);
  if (stream == NULL) {
    if (!Flags->s) fprintf(stderr, "File not found\n");
    *grep_status = 2;
    return;
  }
  if (Flags->v) {
    char* line = NULL;
    size_t len = 0;
    if (getline(&line, &len, stream) != -1) {
      printf("%s\n", filename ? filename : "(standard input)");
      if (*grep_status != 2) *grep_status = 0;
    }
    free(line);
  }
  if (stream != stdin) fclose(stream);
}

// Функция стриминга с флагом l (Подумать над работай если )
void proccess_file_for_flag_l(const char* filename, GrepFlags* Flags,
                              GrepPatterns* Patterns, int* grep_status) {
  if (Patterns->number_patterns <= 0) {
    process_file_for_flag_l_no_patterns(filename, Flags, grep_status);
    return;
  }
  FILE* stream = get_stream(filename);
  char* regex_pattern = NULL;
  regex_t preg;
  bool found_match = false;
  if (stream != NULL) {
    regex_pattern = build_regex_pattern(Patterns);
    if (regex_pattern != NULL) {
      if (regcomp(&preg, regex_pattern, Flags->i ? Flags->i : REG_BASIC) == 0) {
        char* line = NULL;
        size_t len = 0;
        regmatch_t match;
        while (getline(&line, &len, stream) != -1 && !found_match) {
          line[strcspn(line, "\r\n")] = '\0';
          int result = regexec(&preg, line, 1, &match, 0);
          bool is_match = Flags->v ? (result != 0) : (result == 0);
          if (is_match) found_match = true;
        }
        if (found_match) {
          printf("%s\n", filename);
          if (*grep_status != 2) *grep_status = 0;
        }
        free(line);
        regfree(&preg);
      } else {
        fprintf(stderr, "Error compile regex\n");
        *grep_status = 2;
      }
    } else {
      *grep_status = 2;
    }
    free(regex_pattern);
    if (stream != stdin) fclose(stream);
  } else {
    if (!Flags->s) fprintf(stderr, "File not found\n");
    *grep_status = 2;
  }
}

void print_matching_parts(int regex_result, regex_t* preg, regmatch_t* match,
                          char* line, const char* filename, int number_files,
                          int* number_line, GrepFlags* Flags) {
  char* p = line;
  while (!regexec(preg, p, 1, match, 0) && !regex_result && *p != '\0') {
    bool is_continue = false;
    if (match->rm_eo == match->rm_so) {
      p++;
      is_continue = true;
    }
    if (!is_continue) {
      if (number_files > 1 && !Flags->h) printf("%s:", filename);
      if (Flags->n) printf("%d:", *number_line);
      printf("%.*s\n", (int)(match->rm_eo - match->rm_so), p + match->rm_so);
      p = p + match->rm_eo;
    }
  }
}

// Печать строки
void print_string(int regex_result, GrepFlags* Flags, const char* filename,
                  char* line, int number_files, int* number_matches,
                  int* number_line) {
  if (!Flags->c && !Flags->n && regex_result) {
    if (number_files > 1 && !Flags->h) printf("%s:", filename);
    printf("%s\n", line);
  } else if (Flags->c && regex_result) {
    (*number_matches)++;
  } else if (Flags->n && regex_result) {
    if (number_files > 1 && !Flags->h) printf("%s:", filename);
    printf("%d:%s\n", *number_line, line);
  }
}

// Работа со строками
void process_string(regex_t* preg, const char* filename, int number_files,
                    char* line, GrepFlags* Flags, int* number_matches,
                    int* number_line, int* grep_status) {
  regmatch_t match;
  int result = regexec(preg, line, 1, &match, 0);
  bool is_match = Flags->v ? (result != 0) : (result == 0);

  if (is_match) {
    if (Flags->c) {
      (*number_matches)++;
    } else if (Flags->o) {
      if (!Flags->v) {
        print_matching_parts(result, preg, &match, line, filename, number_files,
                             number_line, Flags);
      }
    } else {
      print_string(1, Flags, filename, line, number_files, number_matches,
                   number_line);
    }
    if (*grep_status != 2) *grep_status = 0;
  }
}

// Обрабатываем файл если у нас 0 паттернов
void process_file_no_patterns(char* filename, int number_files,
                              GrepFlags* Flags, int* grep_status) {
  FILE* stream = get_stream(filename);
  if (stream == NULL) {
    if (!Flags->s) fprintf(stderr, "File not found error\n");
    *grep_status = 2;
    return;
  }
  char* line = NULL;
  size_t len = 0;
  int number_matches = 0;
  int number_line = 0;
  while (getline(&line, &len, stream) != -1) {
    line[strcspn(line, "\r\n")] = '\0';
    if (Flags->n) ++number_line;
    bool is_match = Flags->v;
    if (is_match) {
      if (Flags->c) {
        number_matches++;
      } else if (!Flags->o) {
        if (number_files > 1 && !Flags->h) printf("%s:", filename);
        if (Flags->n)
          printf("%d:%s\n", number_line, line);
        else
          printf("%s\n", line);
      }
      if (*grep_status != 2) *grep_status = 0;
    }
  }
  if (Flags->c) {
    if (number_files > 1 && !Flags->h) printf("%s:", filename);
    printf("%d\n", number_matches);
    if (number_matches > 0 && *grep_status != 2) *grep_status = 0;
  }
  free(line);
  if (stream != stdin) fclose(stream);
}

void print_count_or_filename(const char* filename, GrepFlags* Flags,
                             int number_matches, int number_files,
                             int* grep_status) {
  if (Flags->c) {
    if (Flags->l) {
      if (number_matches > 0) {
        printf("%s\n", filename);
      } else {
        if (number_files > 1 && !Flags->h) printf("%s:", filename);
        printf("0\n");
      }
    } else {
      if (number_files > 1 && !Flags->h) printf("%s:", filename);
      printf("%d\n", number_matches);
    }
    if (number_matches > 0 && *grep_status != 2) *grep_status = 0;
  }
}

// Функция стриминга без флага l
void process_file(char* filename, int number_files, GrepFlags* Flags,
                  GrepPatterns* Patterns, int* grep_status) {
  if (Patterns->number_patterns <= 0) {
    process_file_no_patterns(filename, number_files, Flags, grep_status);
    return;
  }
  FILE* stream = get_stream(filename);
  char* regex_pattern = NULL;
  regex_t preg;
  if (stream != NULL) {
    regex_pattern = build_regex_pattern(Patterns);
    if (regex_pattern != NULL) {
      if (regcomp(&preg, regex_pattern, Flags->i ? Flags->i : REG_BASIC) == 0) {
        char* line = NULL;
        size_t len = 0;
        int number_matches = 0, number_line = 0;
        while (getline(&line, &len, stream) != -1) {
          line[strcspn(line, "\r\n")] = '\0';
          if (Flags->n) ++number_line;
          process_string(&preg, filename, number_files, line, Flags,
                         &number_matches, &number_line, grep_status);
        }
        print_count_or_filename(filename, Flags, number_matches, number_files,
                                grep_status);
        free(line);
        regfree(&preg);
      } else {
        fprintf(stderr, "Error compile regex\n");
        *grep_status = 2;
      }
    } else {
      *grep_status = 2;
    }
    free(regex_pattern);
    if (stream != stdin) fclose(stream);
  } else {
    if (!Flags->s) fprintf(stderr, "File not found error\n");
    *grep_status = 2;
  }
}

// Передача либо стандартного потока, либо файла
void process_files(GrepFiles* Files, GrepFlags* Flags, GrepPatterns* Patterns,
                   int* grep_status) {
  if (Files->list == NULL) {
    if (Flags->l && !Flags->c) {
      proccess_file_for_flag_l(NULL, Flags, Patterns, grep_status);
    } else {
      process_file(NULL, 0, Flags, Patterns, grep_status);
    }
  } else {
    for (int i = 0; i < Files->number_files; i++) {
      if (Flags->l && !Flags->c) {
        proccess_file_for_flag_l(Files->list[i], Flags, Patterns, grep_status);
      } else {
        process_file(Files->list[i], Files->number_files, Flags, Patterns,
                     grep_status);
      }
    }
  }
}

/* Запуск всей шайтан-машины*/

int Grep(int argc, char* argv[]) {
  int grep_status = 1;
  GrepFlags Flags = {false};
  GrepPatterns Patterns = {0, 0, NULL};
  GrepFiles Files = {0, 0, NULL};
  parsing_arguments(argc, argv, &Flags, &Patterns, &Files, &grep_status);
  if (grep_status != 2) {
    process_files(&Files, &Flags, &Patterns, &grep_status);
  }
  free_Patterns(&Patterns);
  free_Files(&Files);
  return grep_status;
}

int main(int argc, char* argv[]) { return Grep(argc, argv); }