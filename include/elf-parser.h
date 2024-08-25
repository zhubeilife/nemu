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

void read_elf_header(FILE* fp, Elf32_Ehdr *elf_header);
bool is_ELF(Elf32_Ehdr eh);
bool is64BitELF(Elf32_Ehdr eh);
void print_elf_header(Elf32_Ehdr elf_header);
void read_section_header_table(FILE* fp, Elf32_Ehdr eh, Elf32_Shdr sh_table[]);
void print_section_headers(FILE* fp, Elf32_Ehdr eh, Elf32_Shdr sh_table[]);
void print_symbol_table(FILE* fp, Elf32_Ehdr eh, Elf32_Shdr sh_table[]);
void init_record_func_symbol_table(FILE* fp, Elf32_Ehdr eh, Elf32_Shdr sh_table[]);

#endif //ELF_PARSER_H
