#include <elf-parser.h>

typedef struct
{
    char    st_name[32];    /* Symbol name (string tbl index) */
    word_t	st_value;		/* Symbol value */
    word_t	st_size;		/* Symbol size */
} FUNC_SYM;

#ifdef CONFIG_FTRACE
#define MAX_FTTRACE_FUNC_NUM 32
static int record_func_syn_num = 0;
static FUNC_SYM RECORD_FUN_SYM[MAX_FTTRACE_FUNC_NUM] = {};
#endif

void read_elf_header(FILE* fp, Elf32_Ehdr *elf_header)
{
    fseek(fp, 0, SEEK_SET);
    fread(elf_header, sizeof(Elf32_Ehdr), 1, fp);
}

bool is_ELF(Elf32_Ehdr eh)
{
    /* ELF magic bytes are 0x7f,'E','L','F'
     * Using  octal escape sequence to represent 0x7f
     */
    if(!strncmp((char*)eh.e_ident, "\177ELF", 4)) {
        // printf("ELFMAGIC \t= ELF\n");
        /* IS a ELF file */
        return true;
    } else {
        // printf("ELFMAGIC mismatch!\n");
        /* Not ELF file */
        return false;
    }
}

bool is64BitELF(Elf32_Ehdr eh) {
    if (eh.e_ident[EI_CLASS] == ELFCLASS64)
        return true;
    else
        return false;
}

void read_section_header_table(FILE* fp, Elf32_Ehdr eh, Elf32_Shdr sh_table[])
{
    // seek to the section header table
    fseek(fp, eh.e_shoff, SEEK_SET);
    fread(sh_table, sizeof(Elf32_Shdr) * eh.e_shnum, 1, fp);
}

void print_section_headers(FILE* fp, Elf32_Ehdr eh, Elf32_Shdr sh_table[])
{
    // seek to the section-header string-table
    fseek(fp, sh_table[eh.e_shstrndx].sh_offset, SEEK_SET);
    char *shstrtab = malloc(sh_table[eh.e_shstrndx].sh_size);
    fread(shstrtab, sh_table[eh.e_shstrndx].sh_size, 1, fp);

    printf("========================================");
    printf("========================================\n");
    printf(" idx offset     load-addr  size       algn"
           " flags      type       section\n");
    printf("========================================");
    printf("========================================\n");

    for(uint32_t i=0; i<eh.e_shnum; i++) {
        printf(" %03d ", i);
        printf("0x%08x ", sh_table[i].sh_offset);
        printf("0x%08x ", sh_table[i].sh_addr);
        printf("0x%08x ", sh_table[i].sh_size);
        printf("%4d ", sh_table[i].sh_addralign);
        printf("0x%08x ", sh_table[i].sh_flags);
        printf("0x%08x ", sh_table[i].sh_type);
        printf("%s\t", (shstrtab + sh_table[i].sh_name));
        printf("\n");
    }
    printf("========================================");
    printf("========================================\n");
    printf("\n");	/* end of section header table */

    free(shstrtab);
}

void print_symbol_table(FILE* fp, Elf32_Ehdr eh, Elf32_Shdr sh_table[])
{
    // seek to the section-header string-table
    fseek(fp, sh_table[eh.e_shstrndx].sh_offset, SEEK_SET);
    char *shstrtab = malloc(sh_table[eh.e_shstrndx].sh_size);
    fread(shstrtab, sh_table[eh.e_shstrndx].sh_size, 1, fp);

    Elf32_Shdr* symtab = NULL;
    Elf32_Shdr* strtab = NULL;

    for (int i = 0; i < eh.e_shnum; i++) {
        if (sh_table[i].sh_type == SHT_SYMTAB) {
            symtab = &sh_table[i];
        } else if (sh_table[i].sh_type == SHT_STRTAB && strcmp(&shstrtab[sh_table[i].sh_name], ".strtab") == 0) {
            strtab = &sh_table[i];
        }
    }

    // symbol table
    fseek(fp, symtab->sh_offset, SEEK_SET);
    Elf32_Sym *symbols = malloc(symtab->sh_size);
    fread(symbols, symtab->sh_size, 1, fp);

    // string table
    fseek(fp, strtab->sh_offset, SEEK_SET);
    char *strtab_data = malloc(strtab->sh_size);
    fread(strtab_data, strtab->sh_size, 1, fp);

    // Print the symbols
    int num_symbols = symtab->sh_size / symtab->sh_entsize;
    printf("%d symbols\n", num_symbols);
    for(int i=0; i< num_symbols; i++) {
        printf("0x%08x ", symbols[i].st_value);
        printf("0x%02x ", ELF32_ST_BIND(symbols[i].st_info));
        printf("0x%02x ", ELF32_ST_TYPE(symbols[i].st_info));
        printf("%s\n", (strtab_data + symbols[i].st_name));
    }

    free(symbols);
    free(strtab_data);
    free(shstrtab);
}

void init_record_func_symbol_table(FILE* fp, Elf32_Ehdr eh, Elf32_Shdr sh_table[])
{
    // seek to the section-header string-table
    fseek(fp, sh_table[eh.e_shstrndx].sh_offset, SEEK_SET);
    char *shstrtab = malloc(sh_table[eh.e_shstrndx].sh_size);
    fread(shstrtab, sh_table[eh.e_shstrndx].sh_size, 1, fp);

    Elf32_Shdr* symtab = NULL;
    Elf32_Shdr* strtab = NULL;

    for (int i = 0; i < eh.e_shnum; i++) {
        if (sh_table[i].sh_type == SHT_SYMTAB) {
            symtab = &sh_table[i];
        } else if (sh_table[i].sh_type == SHT_STRTAB && strcmp(&shstrtab[sh_table[i].sh_name], ".strtab") == 0) {
            strtab = &sh_table[i];
        }
    }

    // symbol table
    fseek(fp, symtab->sh_offset, SEEK_SET);
    Elf32_Sym *symbols = malloc(symtab->sh_size);
    fread(symbols, symtab->sh_size, 1, fp);

    // string table
    fseek(fp, strtab->sh_offset, SEEK_SET);
    char *strtab_data = malloc(strtab->sh_size);
    fread(strtab_data, strtab->sh_size, 1, fp);

    int num_symbols = symtab->sh_size / symtab->sh_entsize;
    for(int i=0; i< num_symbols; i++) {
        if (ELF32_ST_TYPE(symbols[i].st_info) == STT_FUNC && record_func_syn_num < MAX_FTTRACE_FUNC_NUM)
        {
            RECORD_FUN_SYM[record_func_syn_num].st_size = symbols[i].st_value;
            RECORD_FUN_SYM[record_func_syn_num].st_value = symbols[i].st_value;
            strcpy(RECORD_FUN_SYM[record_func_syn_num].st_name, (strtab_data + symbols[i].st_name));
            record_func_syn_num++;
            // printf("0x%08x ", symbols[i].st_value);
            // printf("0x%02x ", symbols[i].st_size);
            // printf("%s\n", (strtab_data + symbols[i].st_name));
        }
    }

    free(symbols);
    free(strtab_data);
    free(shstrtab);
}