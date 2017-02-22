#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>

#include "list.h"
#include "dict.h"

#define LINE_BUF_SIZE 1024
#define SYMBOL_LEN 32
#define SYMBOL_FMT "%32s"
#define ADDR_MASK ((uint32_t)0xfffff)
#define IMMEDIATE_MASK ((uint32_t)0xffff)
#define PORT_MASK ((uint32_t)0xff)

typedef struct InstrInfo InstrInfo;

// process_funcs change a certain type of instruction to instruction code.
// ALl process functions returns non-zero on error.
typedef int process_func_t(InstrInfo *info, char *line, uint32_t *instr_code);

// This structure stores instruction-specific infomation.
struct InstrInfo {
  process_func_t *fp;
  uint32_t instr_code;
};

// Selected forward declarations.
void error(char *msg);
int process_data(char *line, char *keyword, int step);
int process_line(char *line, int step);

// Global variables
static Dict g_label_tbl; // Maps label to address.
static Dict g_var_tbl; // Maps variable name to address.
static Dict g_dispatch_tbl; // Maps keywords to InstrInfos.
static uint32_t curr_cs_addr, curr_ds_addr;
FILE *g_fin, *g_fout;
FILE *g_flist;

// Translate a string to uppercase.
// But characters surrounded by "" will be ignored.
void str_toupper(char *str) {
  for (; *str && '\"' != *str; ++str) {
    *str = toupper(*str);
  }
}

// Translates register name to code.
// Returns -1 on error.
int reg_to_code(char c) {
  if ('Z' == c) return 0;
  if (c >= 'A' && c <= 'G')
    return c-'A'+1;
  else
    return -1;
}

// Encodes an opcode into a piece of instruction code.
void append_opcode(uint32_t opcode, uint32_t *instr_code) {
  *instr_code = opcode << 27;
}

// Encodes a register into a piece of instruction code.
// Returns non-zero on error.
int append_reg(char *reg, int shift, uint32_t *instr_code) {
  int32_t reg_code;
  if (strlen(reg) != 1 || (reg_code = reg_to_code(*reg)) < 0) {
    printf("Unknown register: %s\n", reg);
    return -1;
  }
  *instr_code |= reg_code << shift;
  return 0;
}

// Encodes a addr into a piece of instruction code.
// Return -1 on error.
int append_addr(char *symbol, Dict *table, uint32_t *instr_code) {
  DictData *lb_data;
  
  lb_data = dict_look_up(table, symbol, strlen(symbol)+1);
  if (!lb_data) {
    printf("Undefined label: %s\n", symbol);
    return -1;
  }
  *instr_code |= (*(uint32_t*)lb_data->value) & ADDR_MASK;
  return 0;
}


int process_type_1(InstrInfo *info, char *line, uint32_t *instr_code) {
  append_opcode(info->instr_code, instr_code);
  return 0;
}

int process_type_2(InstrInfo *info, char *line, uint32_t *instr_code) {
  char label[SYMBOL_LEN];

  append_opcode(info->instr_code, instr_code);
  if (1 != sscanf(line, "%*s " SYMBOL_FMT, label)) return -1;
  if (0 != append_addr(label, &g_label_tbl, instr_code)) return -1;
  return 0;
}

int process_type_3(InstrInfo *info, char *line, uint32_t *instr_code) {
  char reg[SYMBOL_LEN];
  
  append_opcode(info->instr_code, instr_code);
  if (1 != sscanf(line, "%*s " SYMBOL_FMT, reg)) return -1;
  if (0 != append_reg(reg, 24, instr_code)) return -1;
  return 0;
}

int process_type_4(InstrInfo *info, char *line, uint32_t *instr_code) {
  char label[SYMBOL_LEN], reg[SYMBOL_LEN];

  append_opcode(info->instr_code, instr_code);
  if (2 != sscanf(line, "%*s" SYMBOL_FMT SYMBOL_FMT, reg, label)) return -1;
  if (0 != append_reg(reg, 24, instr_code)) return -1;
  if (0 != append_addr(label, &g_var_tbl, instr_code)) return -1;
  return 0;
}

int process_type_5(InstrInfo *info, char *line, uint32_t *instr_code) {
  int immediate;
  char reg[SYMBOL_LEN];

  append_opcode(info->instr_code, instr_code);
  if (2 != sscanf(line, "%*s" SYMBOL_FMT "%d", reg, &immediate)) return -1;
  if (0 != append_reg(reg, 24, instr_code)) return -1;
  *instr_code |= immediate & IMMEDIATE_MASK;
  return 0;
}

int process_type_6(InstrInfo *info, char *line, uint32_t *instr_code) {
  int port;
  char reg[SYMBOL_LEN];

  append_opcode(info->instr_code, instr_code);
  if (2 != sscanf(line, "%*s" SYMBOL_FMT "%d", reg, &port)) return -1;
  if (0 != append_reg(reg, 24, instr_code)) return -1;
  *instr_code |= port & PORT_MASK;
  return 0;
}

int process_type_7(InstrInfo *info, char *line, uint32_t *instr_code) {
  char reg0[SYMBOL_LEN], reg1[SYMBOL_LEN], reg2[SYMBOL_LEN];

  append_opcode(info->instr_code, instr_code);
  if (3 != sscanf(line, "%*s" SYMBOL_FMT SYMBOL_FMT SYMBOL_FMT,
                  reg0, reg1, reg2)) return -1;
  if (0 != append_reg(reg0, 24, instr_code)) return -1;
  if (0 != append_reg(reg1, 20, instr_code)) return -1;
  if (0 != append_reg(reg2, 16, instr_code)) return -1;
  return 0;
}

int process_type_8(InstrInfo *info, char *line, uint32_t *instr_code) {
  char reg0[SYMBOL_LEN], reg1[SYMBOL_LEN];

  append_opcode(info->instr_code, instr_code);
  if (2 != sscanf(line, "%*s" SYMBOL_FMT SYMBOL_FMT, reg0, reg1)) return -1;
  if (0 != append_reg(reg0, 24, instr_code)) return -1;
  if (0 != append_reg(reg1, 20, instr_code)) return -1;
  return 0;
}

// States used in processing brackets.
typedef enum ProcBracketState {
  START,
  NUM,
  COMMA,
  END,
} ProcBracketState;

// Processes list initializers, returns the number of initial values written.
// Returns negative value on error.
int process_bracket(char **curr_ptr, size_t elem_size) {
  char *chr_ptr = *curr_ptr, *end_ptr;
  int val_num;
  int val;
  ProcBracketState state = START;

  // G_Finite automata for {num, num, num}
  val_num = 0;
  while (*chr_ptr && state != END) {
    while (isspace(*chr_ptr)) chr_ptr++;
    switch (state) {
      case START:
        if ('{' == *chr_ptr) {
          chr_ptr++;
          state = NUM;
        } else {
          return -1;
        }
        break;
      case NUM:
        val = strtol(chr_ptr, &end_ptr, 10);
        if (chr_ptr == end_ptr) return -1;
        fwrite(&val, elem_size, 1, g_fout);
        val_num++;
        chr_ptr = end_ptr;
        state = COMMA;
        break;
      case COMMA:
        if (',' == *chr_ptr) {
          chr_ptr++;
          state = NUM;
        } else if('}' == *chr_ptr) {
          chr_ptr++;
          state = END;
        } else {
          return -1;
        }
        break;
      default:
        break;
    }
  }
  *curr_ptr = chr_ptr;
  
  if (state != END) {
    printf("Unclosed bracket\n");
    return -1;
  } else {
    return val_num;
  }
}

// Process string constant initializers.
// Returns the string length on sucess, negative number on failure.
int process_string_const(char **line) {
  char str[LINE_BUF_SIZE], *str_ptr, *line_ptr;
  int len = 0;

  str_ptr = str;
  line_ptr = *line + 1;
  for (; *line_ptr && '\"' != *line_ptr; ++line_ptr) {
    if ('\\' == *line_ptr) {
      if (*++line_ptr) {
        *str_ptr++ = *line_ptr;
      } else {
        return -1;
      }
    } else {
      *str_ptr++ = *line_ptr;
    }
  }
  *str_ptr = 0;

  if ('\"' != *line_ptr) return -1; // Unmatched quotes
  len = str_ptr-str+1;
  fwrite(str, len, 1, g_fout);
  *line = line_ptr + 1;
  
  return len;
}

// Process data definitions.
// If step == 1, this function only calculates the needed space.
int process_data(char *line, char *keyword, int step) {
  char symbol[SYMBOL_LEN], *symbol_ptr = symbol;
  bool has_size = false;  // Has the optional []?
  int elem_num = 1;
  int init_val, zero = 0;
  size_t elem_size;
  char *curr_ptr;

  elem_size = strcmp(keyword, "BYTE") ? 2 : 1;

  // Skip the first word
  if (1 == elem_size)
    curr_ptr = strstr(line, "BYTE");
  else
    curr_ptr = strstr(line, "WORD");
  curr_ptr += 4;

  // Process symbol.
  if ('\0' == *curr_ptr) return -1;
  while (isspace(*curr_ptr)) curr_ptr++;
  for (int n = 0; n < SYMBOL_LEN && isalnum(*curr_ptr); ++n) {
    *symbol_ptr++ = *curr_ptr++;
  }
  *symbol_ptr = '\0';
  if (symbol_ptr - symbol < 1) return -1;
  if (1 == step) { // Pre-processing, add to the table.
    if (dict_look_up(&g_var_tbl, symbol, strlen(symbol)+1)) {
      printf("Duplicated variable name: %s\n", symbol);
      return -1;
    }
    dict_add(&g_var_tbl,
             (DictData) {symbol, strlen(symbol)+1, &curr_ds_addr, 4});
    fprintf(g_flist, "DS: %s= %u\n", symbol, curr_ds_addr);
  }
  

  // Process [n].
  while (isspace(*curr_ptr)) curr_ptr++;
  if ('[' == *curr_ptr) {
    has_size = true;
    curr_ptr++;
    elem_num = strtoul(curr_ptr, &curr_ptr, 10);
    if (!elem_num) return -1; // Invalid size.

    // Find ']'
    while (isspace(*curr_ptr)) curr_ptr++;
    if (']' != *curr_ptr)
      return -1; // Unmatched []
    else
      curr_ptr++;
  }

  // Actual writing out.
  if (2 == step) {
    while (isspace(*curr_ptr)) curr_ptr++;
    if ('=' == *curr_ptr) {  // Process initializers.
      curr_ptr++;
      while (isspace(*curr_ptr)) curr_ptr++;
      
      if (!has_size) { // Single-value initializer.
        char *end_ptr;
        init_val = strtol(curr_ptr, &end_ptr, 10);
        if (end_ptr == curr_ptr) return -1;
        curr_ptr = end_ptr;
        fwrite(&init_val, elem_size, elem_num, g_fout);
        
      } else if ('{' == *curr_ptr) { // List initilizer.
        int val_num = process_bracket(&curr_ptr, elem_size);
        if (val_num < 0 || val_num > elem_num) return -1;
        // Fill out the rest with zeros.
        fwrite(&zero, elem_size, elem_num - val_num, g_fout);
        
      } else if ('\"' == *curr_ptr && 1 == elem_size) { // String
        int val_num = process_string_const(&curr_ptr);
        if (val_num < 0) {
          puts("Illegal string constant");
          return -1;
        }
        if (val_num > elem_num) {
          puts("String constant exceeds the capacity of the array");
          return -1;
        }
        // Fill out the rest with zeros.
        fwrite(&zero, elem_size, elem_num - val_num, g_fout);
        
      } else { // Illegal syntax.
        puts("Illegal data syntax");
        return -1;
      }
      
    } else { // No initializers, fill out with zeros.
      fwrite(&zero, elem_size, elem_num, g_fout);
    }

    // Process ending.
    while (isspace(*curr_ptr)) curr_ptr++;
    if (*curr_ptr != '\0' && *curr_ptr != '#') {
      printf("Trailling garbage: %s\n", curr_ptr);
      return -1;
    }
  }

  curr_ds_addr += elem_num * elem_size;
  return 0;
}

// Process a line.
// If step == 1, this function only calculates the the label info.
// If step == 2, this function generates the final output.
int process_line(char *line, int step) {
  char first_word[SYMBOL_LEN],label[SYMBOL_LEN];
  char *colon, *sharp; // Potential ':' and '#'.
  uint32_t instr_code;
  DictData *data;
  InstrInfo *info;

  // Delete comments and convert line to upper.
  sharp = strstr(line, "#");
  if (sharp) *sharp = '\0';
  str_toupper(line);
  
  // Label?
  for (colon = line; *colon && ('\"' != *colon) && (':' != *colon); ++colon)
    ;
  if (':' == *colon) {
    *colon = '\0';
    if (1 != sscanf(line, SYMBOL_FMT, label)) return -1;
    if (1 == step) { // Pre-processing, add to the table.
      if (dict_look_up(&g_label_tbl, label, strlen(label)+1)) {
        printf("Duplicated label: %s\n", colon);
        return -1;
      }
      dict_add(&g_label_tbl,
               (DictData){label, strlen(label)+1, &curr_cs_addr, 4});
      fprintf(g_flist, "CS: %s= %u\n", line, curr_cs_addr);
    }
    if (2 == step) {
      fprintf(g_flist, "%s:", line); // We want labels in list file.
    }
    line = colon + 1; // Cut off the label part.
  }

  // Empty line?
  if (1 != sscanf(line, SYMBOL_FMT, first_word)) return 0;

  // Dispatch to handlers.
  data = dict_look_up(&g_dispatch_tbl, first_word, strlen(first_word)+1);
  if (data) {  // Normal instructions?
    if (2 == step) {
      info = (InstrInfo *)data->value;
      if (0 != (*info->fp)(info, line, &instr_code)) return -1;
      fprintf(g_flist, "%s#PC:%d\n", line, curr_cs_addr);
      fprintf(g_flist, "0x%08x\n", instr_code);
      fwrite(&instr_code, 4, 1, g_fout);
    }
    curr_cs_addr += 4;
  } else if(!strcmp(first_word, "BYTE") || !strcmp(first_word, "WORD")) { // DS?
    if (':' == *colon) return -1;  //Data definitons can't have colons
    if (0 != process_data(line, first_word, step)) return -1;
  } else {  // Unkonwn token.
    printf("Unknown token: %s\n", first_word);
    return -1;
  }
      
  return 0;
}

// Print error massage and exit.
void error(char *msg) {
  printf("Error: %s\n", msg);
  exit(EXIT_FAILURE);
}

// Allocate resources.
int sas_init(char *argv[]) {
  // Open input and output files.
  g_fin = fopen(argv[1], "r");
  if (!g_fin) error("Can't open input file!");
  g_fout = fopen(argv[2], "wb");
  if (!g_fout) error("Can't open output file!");
  //Debug
  g_flist = fopen("list.txt", "w");
  if (!g_flist) error("Can't open list file!");

  curr_cs_addr = curr_ds_addr = 0;

  // Set up tabels.
  dict_init(&g_label_tbl);
  dict_init(&g_var_tbl);
  
  dict_init(&g_dispatch_tbl);
  
// Adds an entry in dispatch table.
#define DISPATCH_ENTRY(instr, code, fp) dict_add(                       \
    &g_dispatch_tbl,                                                    \
    (DictData){instr, strlen(instr)+1,                                  \
          &(InstrInfo){fp,code}, sizeof(InstrInfo)})

  // Initialize the dispatch table.
  DISPATCH_ENTRY("HLT", 0, process_type_1);
  DISPATCH_ENTRY("JMP", 1, process_type_2);
  DISPATCH_ENTRY("CJMP", 2, process_type_2);
  DISPATCH_ENTRY("OJMP", 3, process_type_2);
  DISPATCH_ENTRY("CALL", 4, process_type_2);
  DISPATCH_ENTRY("RET", 5, process_type_1);
  DISPATCH_ENTRY("PUSH", 6, process_type_3);
  DISPATCH_ENTRY("POP", 7, process_type_3);
  DISPATCH_ENTRY("LOADB", 8, process_type_4);
  DISPATCH_ENTRY("LOADW", 9, process_type_4);
  DISPATCH_ENTRY("STOREB", 10, process_type_4);
  DISPATCH_ENTRY("STOREW", 11, process_type_4);
  DISPATCH_ENTRY("LOADI", 12, process_type_5);  
  DISPATCH_ENTRY("NOP", 13, process_type_1);
  DISPATCH_ENTRY("IN",  14, process_type_6);
  DISPATCH_ENTRY("OUT", 15, process_type_6);
  DISPATCH_ENTRY("ADD", 16, process_type_7);
  DISPATCH_ENTRY("ADDI", 17, process_type_5);
  DISPATCH_ENTRY("SUB", 18, process_type_7);
  DISPATCH_ENTRY("SUBI", 19, process_type_5);
  DISPATCH_ENTRY("MUL", 20, process_type_7);
  DISPATCH_ENTRY("DIV", 21, process_type_7);
  DISPATCH_ENTRY("AND", 22, process_type_7);
  DISPATCH_ENTRY("OR",  23, process_type_7);
  DISPATCH_ENTRY("NOR", 24, process_type_7);
  DISPATCH_ENTRY("NOTB", 25, process_type_8);
  DISPATCH_ENTRY("SAL", 26, process_type_7);
  DISPATCH_ENTRY("SAR", 27, process_type_7);
  DISPATCH_ENTRY("EQU", 28, process_type_8);
  DISPATCH_ENTRY("LT", 29, process_type_8);
  DISPATCH_ENTRY("LTE", 30, process_type_8);
  DISPATCH_ENTRY("NOTC", 31, process_type_1);

#undef DISPATCH_ENTRY

  return 0;
}

// Release resources.
int sas_end() {
  dict_destroy(&g_label_tbl);
  dict_destroy(&g_var_tbl);
  dict_destroy(&g_dispatch_tbl);
  fclose(g_fin);
  fclose(g_fout);
  return 0;
}

// Print usage and die
void usage_and_die() {
  puts("Usage: sas in_file out_file");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  if (argc != 3) usage_and_die();
  
  char line_buf[LINE_BUF_SIZE];
  int line_num;

  sas_init(argv);
  atexit((void (*)(void))sas_end); // For a clean exit.
  
  // Step 1 scan through the file to get label information.
  for (line_num = 1; fgets(line_buf, LINE_BUF_SIZE, g_fin); ++line_num) {
    if (0 != process_line(line_buf, 1)) {
      printf("Line: %d\n", line_num);
      error("Pre-process error!");
    }
  }

  fwrite(&curr_ds_addr, 4, 1, g_fout);
  fwrite(&curr_cs_addr, 4, 1, g_fout);
  printf("DS_SIZE: %d, CS_SIZE: %d\n", curr_ds_addr, curr_cs_addr);

  // Step 2: Assemble.
  curr_ds_addr = 0;
  curr_cs_addr = 0;
  rewind(g_fin);
  for (line_num = 1; fgets(line_buf, LINE_BUF_SIZE, g_fin); ++line_num) {
    if (0 != process_line(line_buf, 2)) {
      printf("Line: %d\n", line_num);
      error("Process error!");
    }
  }

  puts("Assemble Success");
  return 0;
}
