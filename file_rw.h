#pragma once
#include <ctype.h>

char cs[] = {'!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', 
        '+', ',', '/', ';', '<', '=', '>', '?', '@', '[', '\\', ']', 
        '^', '`', '{', '|', '}', '~', ' ', ':', ';'};

int count_elements(char* line, int line_length, char separator) {
    int counter = 0;
    int is_quote = 0;
    for(int i = 0; i < line_length; i++) {
        if (line[i] == '"') {
            if (is_quote) is_quote = 0;
            else is_quote = 1;
        }
        if (!is_quote && line[i] == separator) counter++;
    }
    return counter+1;
}

int valid_tag_char(char c) {
    for (int i = 0; i < 30; i++) {
        if (c == cs[i]) return 0;
    }
    return 1;
}

char** get_elements(char* line, int line_length, char separator, int elements_num) {
    int start_pos = 0;
    char** elements = (char**)malloc(elements_num * sizeof(char*));
    for (int i = 0; i < elements_num; i++) {
        int word_length = 0;
        int additional_word_length = 0;
        int is_quote = 0;
        // Находим длину текущего элемента
        while ((start_pos+word_length) < line_length) {
            switch (line[start_pos+word_length]) {
                case '"': {
                    additional_word_length += 5;
                    if (is_quote) is_quote = 0; else is_quote = 1;
                    break;
                }
                case '<': additional_word_length += 3; break;
                case '>': additional_word_length += 3; break;
                case '&': additional_word_length += 4; break;
                case '\'': additional_word_length += 5; break;
            }
            if (!is_quote && line[start_pos+word_length] == separator) break;
            word_length++;
        }
        // Выделяем память под элемент
        elements[i] = calloc((word_length+additional_word_length+1), sizeof(char));
        int dest_i = 0;
        for (int src_i = 0; src_i < word_length; src_i++) {
            // Переписываем данные из строки в массив
            switch (line[start_pos+src_i]) {
                case '<': {
                    strcat(elements[i], "&lt;");
                    dest_i+=4;
                    break;
                }
                case '>': {
                    strcat(elements[i], "&gt;");
                    dest_i+=4;
                    break;
                }
                case '&': {
                    strcat(elements[i], "&amp;");
                    dest_i+=5;
                    break;
                }
                case '\'': {
                    strcat(elements[i], "&apos;");
                    dest_i+=6;
                    break;
                }
                case '\"': {
                    strcat(elements[i], "&quot;");
                    dest_i+=6;
                    break;
                }
                default: {
                    elements[i][dest_i] = line[start_pos+src_i]; dest_i++;
                }
            }
        }
        elements[i][word_length+additional_word_length] = '\0';        
        start_pos+=word_length+1;
    }
    return elements;
}

void repair_headers(char** elements, int elements_count) {
    for (int i = 0; i < elements_count; i++) {
        int f = 0;
        while(elements[i][f]) {
            if(!valid_tag_char(elements[i][f])) {
                elements[i][f] = '_';
            }
            if (f == 0 && (
                elements[i][f] == '-' || 
                elements[i][f] == '.' || 
                isdigit(elements[i][f]) 
                )) {
                // Если заголовок начинается с . или - ставим перед ними _
                char* new_ptr = calloc(strlen(elements[i])+1, sizeof(char));
                new_ptr[0] = '_';
                strcat(new_ptr, elements[i]);
                free(elements[i]);
                elements[i] = new_ptr;
            }
            f++;
        }
    }
}

char** generate_headers(int elements_num) {
    // Генерируем заголовки вида c1, c2...
    char** elements = (char**)malloc(elements_num * sizeof(char*));
    for (int i = 1; i < elements_num+1; i++) {
        elements[i-1] = (char*)calloc(1, sizeof(char)*((i/10)+3));
        snprintf(elements[i-1], ((i/10)+3), "c%d", i);
    }
    return elements;
}

int* get_headers_lengths(char** headers, int elements_num) {
    int* headers_lengths = (int*)malloc(sizeof(int)*elements_num);
    for (int i = 0; i < elements_num; i++) {
        headers_lengths[i] = (int)strlen(headers[i]);
    }
    return headers_lengths;
}

void free_elements(char** elements, int size) {
    for (int i = 0; i < size; i++) {
        free(elements[i]);
    }
}

void write_file_start(FILE* file) {
    fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root>", file);
}

void write_file_end(FILE* file) {
    fputs("</root>", file);
}

void write_elements(FILE* file, char** headers, char** elements, int element_count, int* headers_lengths) {
    fputs("<row>", file);
    for (int i = 0; i < element_count; i++) {
        if (headers_lengths[i] == 0) continue;
        fprintf(file, "<%s>%s</%s>", headers[i], elements[i], headers[i]);
    }
    fputs("</row>", file);
}

void write_elements_inline(FILE* file, char** headers, char** elements, int element_count, int* headers_lengths) {
    fputs("<row", file);
    for (int i = 0; i < element_count; i++) {
        if (headers_lengths[i] == 0) continue;
        fprintf(file, " %s=\"%s\"", headers[i], elements[i]);
    }
    fputs("/>", file);
}

char* read_line(FILE* file, int size_step) {
    // Выделяем начальную память
    char* line = (char*)malloc(size_step*sizeof(char));
    unsigned int cur_size = 0;
    unsigned int cur_max_size = size_step;
    int c;
    while(cur_size < cur_max_size-1) {
        c = fgetc(file);
        if (cur_size == 0 && c == EOF) {
            return 0;
        }
        // Если достигли конца строки или конца файла, возвращаем получившуюся строку
        if (c == '\n' || c == EOF) {
            line[cur_size++] = 0;
            return line;
        }
        line[cur_size++] = c;
        // Если достигли конца буфера, расширяем его
        if (cur_size == cur_max_size-1) {
            cur_max_size += size_step;
            line = realloc(line, (cur_max_size+size_step)*sizeof(char));
        }
    }
}