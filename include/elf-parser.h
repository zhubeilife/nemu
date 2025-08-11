#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <elf.h>

#include "common.h"

void read_elf_header(FILE* fp, Elf32_Ehdr *elf_header, int file_offset);
bool is_ELF(Elf32_Ehdr eh);
bool is64BitELF(Elf32_Ehdr eh);
void print_elf_header(Elf32_Ehdr elf_header);
void read_section_header_table(FILE* fp, Elf32_Ehdr eh, Elf32_Shdr sh_table[], int file_offset);
void print_section_headers(FILE* fp, Elf32_Ehdr eh, Elf32_Shdr sh_table[], int file_offset);
void print_symbol_table(FILE* fp, Elf32_Ehdr eh, Elf32_Shdr sh_table[], int file_offset);
void add_record_func_symbol_table(FILE* fp, Elf32_Ehdr eh, Elf32_Shdr sh_table[], int file_offset);

void load_user_elf(char* file_name, int file_offset);

void log_ftrace(bool is_func_call, vaddr_t current_pc, vaddr_t next_pc);

#endif //ELF_PARSER_H
