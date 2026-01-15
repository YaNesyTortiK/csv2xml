#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

#define LINE_LEN 512

#include "file_rw.h"

char usage_str[] = "Usage example: main.exe [-tichf] [-s ';'] input.csv [output.xml]\n" \
    "\t-t - to generate file in table form\n" \
    "\t-i - to generate file in inline form\n" \
    "\t-c - to combine data from multiple files into one output\n"\
    "\t-s '<character>' - to specify separator for csv file (by default ';')\n"\
    "\t-h - to skip parsing headers from file, columns will have names like: c1, c2...\n"\
    "\t-f - to only parse headers from first file (use only when -c is specified), other files will be treated as continuation of first file (if -h specified this option disables)\n"\
    "\tinput.csv - one or more csv file(s) to convert\n"\
    "\toutput.xml - optional output filename. If not specified will be generated from input filename.\n"\
    "\tIf two or more input files are specified:\n"\
    "\t* If equal number of output files are specified or no output file is specified: each file will be converted to each corresponding output file\n"\
    "\t* If number of output files less then numbers of input files: Input files that have corresponding output file will be converted into them, other output filenames will be generated automatically\n"\
    "\tIf only one output file is specified and -c argument is passed: All content (even if headers are different) will be combined into single specified output file\n"\
    "\t* If -c argument is passed and multiple output files: Program will ignore all output files except first one and work as if only one output file is passed\n"\
    "\t* If -c argument is passed and no output file is specified: Program will generate new .xml output file based on filename of first input file\n";

int parse_args(int argc, char* argv[], char*** input_filenames, char*** output_filenames, 
    char* separator, char* mode, int* concat, int* in_files_size, int* out_files_size,
    int* no_headers, int* headers_ff) {
    int c;
    while ((c = getopt(argc, argv, "itchfs:")) != -1) {
        switch (c) {
            case 'i':
                *mode = 'i';
                break;
            case 't':
                *mode = 't';
                break;
            case 'c':
                *concat = 1;
                break;
            case 'h':
                *no_headers = 1;
                break;
            case 'f':
                *headers_ff = 1;
                break;
            case 's':
                *separator = *optarg;
                break;
            case '?':
                if (optopt == 's') {
                    fprintf(stderr, "Option for separator -%c requires an argument.\n", optopt);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                }
                return 4;
            default:
                return 4;
        }
    }
    int index;
    int i = 0;
    // Обработка входных и выходных файлов
    for (index = optind; index < argc; index++) {
        int s_len = strlen(argv[index]);
        if (s_len >= 4) {
            if (
                argv[index][s_len-4] == '.' &&
                argv[index][s_len-3] == 'c' &&
                argv[index][s_len-2] == 's' &&
                argv[index][s_len-1] == 'v'
            ) {
                if (*in_files_size > 0) {
                    // Расширяем массив входных файлов
                    *input_filenames = (char**)realloc(*input_filenames, (*in_files_size+1)*sizeof(char*));
                }
                else {
                    // Выделяем память на массив входных файлов
                    *input_filenames = (char**)malloc((*in_files_size+1)*sizeof(char*));
                }
                // Выделяем память для входного файла и записываем
                (*input_filenames)[*in_files_size] = (char*)malloc((s_len+1)*sizeof(char));
                strcpy((*input_filenames)[*in_files_size], argv[index]);
                *in_files_size += 1;
            } else if (
                argv[index][s_len-4] == '.' &&
                argv[index][s_len-3] == 'x' &&
                argv[index][s_len-2] == 'm' &&
                argv[index][s_len-1] == 'l'
            ) {
                if (*out_files_size > 0)
                    // Расширяем массив выходных файлов
                    *output_filenames = (char**)realloc(*output_filenames, (*out_files_size+1)*sizeof(char*));
                else
                    // Выделяем память на массив выходных файлов
                    *output_filenames = (char**)malloc((*out_files_size+1)*sizeof(char*));
                // Выделяем память для выходного файла и записываем
                (*output_filenames)[*out_files_size] = (char*)malloc((s_len+1)*sizeof(char));
                strcpy((*output_filenames)[*out_files_size], argv[index]);
                *out_files_size += 1;
            } else {
                return 3;  // Если файл не .csv и не .xml
            }
        } else {
            return 3; // Если длина имени файла слишком маленькая
        }
        i++;
    }
    if (i == 0 || *in_files_size == 0) return 2; // Если ни одного файла не было
    return 0;
}

void repair_output_files(char*** input_filenames, char*** output_filenames, int* in_files_size, int* out_files_size, int concat) {
    if (*in_files_size == *out_files_size || *in_files_size < *out_files_size)
        return; // Если ничего исправлять не требуется
    if (concat) {
        // Если собираем в один файл
        if (*out_files_size > 0)
            return;
        // Если не указан ни один выходной файл
        *output_filenames = (char**)malloc(sizeof(char*));
        (*output_filenames)[0] = (char*)malloc((strlen((*input_filenames)[0])+4)*sizeof(char));
        strcpy((*output_filenames)[0], (*input_filenames)[0]);
        strcat((*output_filenames)[0], ".xml");
        *out_files_size = 1;
    } else {
        int original_out_size = *out_files_size;
        if (*out_files_size > 0) { // Если какие-то файлы уже есть
            // Расширяем массив выходных
            *output_filenames = (char**)realloc(*output_filenames, (*in_files_size)*sizeof(char*));
        } else {
            // Выделяем память под массив выходных файлов
            *output_filenames = (char**)malloc((*in_files_size)*sizeof(char*));
        }
        *out_files_size = *in_files_size;
        // Создаем новые выходные файлы и дописываем их в массив
        for (int i = original_out_size; i < *in_files_size; i++) {
            (*output_filenames)[i] = (char*)malloc((strlen((*input_filenames)[i])+5)*sizeof(char));
            strcpy((*output_filenames)[i], (*input_filenames)[i]);
            strcat((*output_filenames)[i], ".xml");
        }
    }
}

void process_data_line(char* line, int cur_line_length, char separator, int columns_count, char mode, char** headers, FILE* result_file, int* headers_lengths) {
    // Обработка строки с элементами
    if (cur_line_length == 0) {
        free(line);
        return;
    }
    char** elements = get_elements(line, cur_line_length, 
        separator, columns_count);
    if (mode == 't')
        write_elements(result_file, headers, elements, columns_count, headers_lengths);
    else
        write_elements_inline(result_file, headers, elements, columns_count, headers_lengths);
    free_elements(elements, columns_count);
    free(elements);
    free(line);
}

int main(int argc, char* argv[]) {

    setlocale(LC_ALL, "");

    char** input_filenames;
    char** output_filenames;
    char separator = ',', mode = 't';
    int concat = 0;
    int in_files_size = 0, out_files_size = 0, no_headers = 0, headers_ff = 0;

    switch (parse_args(argc, argv, &input_filenames, &output_filenames, 
        &separator, &mode, &concat, &in_files_size, &out_files_size,
        &no_headers, &headers_ff)) {
        case 0:
            break; // Без ошибок
        case 1: {
            printf("Error: Too many arguments passed!\n%s", usage_str);
            return 1;
        }
        case 2: {
            printf("Error: No input filename specified!\n%s", usage_str);
            return 1;
        }
        case 3: {
            printf("Unknown file type in arguments! Only .csv and .xml filetypes are supported!\n%s", usage_str);
            return 1;
        }
        default: {
            printf("Error parsing arguments!\n%s", usage_str);
            return 1;
        }
    }
    // Исправляем массив входных файлов
    repair_output_files(&input_filenames, &output_filenames, &in_files_size, &out_files_size, concat);

    // Вывод вспомогательной информации
    printf("<{[ CSV2XML ]}>\nMode: '%c'. Separator: '%c'. Concat: %d\nNo headers: %d. Headers only from first file: %d.\n", mode, 
        separator, concat, no_headers, headers_ff);
    printf("Input files:\n");
    for (int i = 0; i < in_files_size; i++) {
        printf("\t%s\n", input_filenames[i]);
    }
    printf("Output files:\n");
    for (int i = 0; i < out_files_size; i++) {
        if (i == 1 && concat) break;
        printf("\t%s\n", output_filenames[i]);
    }

    // Описание и инициализация
    int columns_count;
    int cur_line_length = 0;
    char** glob_headers = NULL;
    int glob_headers_num = 0;

    // Цикл по всем входным файлам
    for (int file_ind = 0; file_ind < in_files_size; file_ind++) {
        // Открытие входного файла
        FILE* file = fopen(input_filenames[file_ind], "r");
        if (file == NULL) {
            printf("Error: unable to open input file \"%s\"!\n", input_filenames[file_ind]);
            return 1;
        }
        // Открытие выходного файла
        FILE* result_file;
        if (concat && file_ind > 0) {
            result_file = fopen(output_filenames[0], "a"); 
        } else {
            result_file = fopen(output_filenames[file_ind], "w"); 
        }
        if (result_file == NULL) {
            if (concat && file_ind > 0)
                printf("Error: unable to open output file \"%s\" to add data!\n", output_filenames[0]);
            else 
                printf("Error: unable to open for rewrite or create output file \"%s\"!\n", output_filenames[file_ind]);
            return 1;
        }
        // Если требуется то записываем в файл начальный код xml
        if (!concat || (concat && file_ind == 0))
            write_file_start(result_file);

        // обработка файла
        printf("Processing file \"%s\"... ", input_filenames[file_ind]);
        char* line;
        line = read_line(file, LINE_LEN);
        cur_line_length = strlen(line);
        if (cur_line_length == 0) {
            printf("ERROR! First line of file is empty where data was expected!\n");
            return 1;
        }
        columns_count = count_elements(line, cur_line_length, separator);

        char** headers = NULL;
        int* headers_lengths = NULL;
        if (!no_headers && !headers_ff) {
            // Обычная обработка
            headers = get_elements(line, cur_line_length, 
                separator, columns_count);
            repair_headers(headers, columns_count); // Оставляем в заголовках только буквы и числа
            headers_lengths = get_headers_lengths(headers, columns_count);
            free(line);
        } else if (!no_headers && headers_ff) {
            // Если нужно брать заголовки только из первого файла
            if (file_ind == 0) {
                // Если это первый входной файл (взять заголовки)
                headers = get_elements(line, cur_line_length, separator, columns_count);
                repair_headers(headers, columns_count); // Оставляем в заголовках только буквы и числа
                glob_headers = headers;
                glob_headers_num = columns_count;
                headers_lengths = get_headers_lengths(headers, columns_count);
                free(line);
            } else {
                // Не первый входной файл (записать первую строку в выходной файл)
                if (columns_count != glob_headers_num) {
                    printf("\nERROR! Argument -f is specified but file contains different number of columns then first file!\n");
                    return 1;
                }
                headers = glob_headers;
                headers_lengths = get_headers_lengths(headers, columns_count);
                process_data_line(line, cur_line_length, separator, columns_count, mode, headers, result_file, headers_lengths);
            }
        } else {
            // Если без заголовков из входного файла
            headers = generate_headers(columns_count); // Генерируем заголовки
            headers_lengths = get_headers_lengths(headers, columns_count);
            process_data_line(line, cur_line_length, separator, columns_count, mode, headers, result_file, headers_lengths);
        }

        // Обрабатываем все остальные строки
        while (line = read_line(file, LINE_LEN)) {
            cur_line_length = strlen(line);
            process_data_line(line, cur_line_length, separator, columns_count,
                mode, headers, result_file, headers_lengths);
        }
        
        if (!headers_ff || file_ind == in_files_size-1) {
            free_elements(headers, columns_count);
            free(headers);
        }
            
        if (!concat || (concat && file_ind == in_files_size-1))
            write_file_end(result_file);

        free(headers_lengths);
        
        fclose(file);
        fclose(result_file);
        printf("Done!\n");
    }

    free_elements(input_filenames, in_files_size);
    free(input_filenames);
    free_elements(output_filenames, out_files_size);
    free(output_filenames);
    
    printf("Program done!\n");

    return 0;
}