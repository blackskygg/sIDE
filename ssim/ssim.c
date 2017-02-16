#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define STACK_SIZE 4096
#define ES_SIZE 4096
#define ADDR_MASK ((uint32_t)0xffffff)
#define IMMEDIATE_MASK ((uint32_t)0xffff)
#define PORT_MASK ((uint32_t)0xff)

// The struct represents the states of a running virtual machine.
typedef struct {
  int16_t general_regs[8]; // Z:0 A:1 B:2 C:3 D:4 E:5 F:6 G:7

  uint8_t *CS;
  uint8_t *DS;
  uint8_t *SS;
  uint8_t *ES; // Uses to store a copy of general_regs during function calls.

  uint32_t PC;
  uint32_t IR;
  uint32_t SS_TOP;
  uint32_t ES_TOP;

  struct {
    unsigned int OF : 1;
    unsigned int CF : 1;
    unsigned int    : 14;
  } PSW;

  bool stopped;
}__attribute__((packed)) VMState;

// Global state.
VMState g_vm_state;

// Shorthands.
#define OPCODE() ((g_vm_state.IR) >> 27)
#define REG0() ((g_vm_state.IR>>24) & 0x7)
#define REG1() ((g_vm_state.IR>>20) & 0xf)
#define REG2() ((g_vm_state.IR>>16) & 0xf)
#define ADDR() (g_vm_state.IR & ADDR_MASK)
#define IMMEDIATE() (uint16_t)(g_vm_state.IR & IMMEDIATE_MASK)
#define PORT() (g_vm_state.IR & PORT_MASK)
#define REG0_VAL (g_vm_state.general_regs[REG0()]) 
#define REG1_VAL (g_vm_state.general_regs[REG1()]) 
#define REG2_VAL (g_vm_state.general_regs[REG2()]) 
#define REGG_VAL (g_vm_state.general_regs[7])

// Forward declarations.
int do_hlt();
int do_jmp();
int do_cjmp();
int do_ojmp();
int do_call();
int do_ret();
int do_push();
int do_pop();
int do_loadb();
int do_loadw();
int do_storeb();
int do_storew();
int do_loadi();
int do_nop();
int do_in();
int do_out();
int do_add();
int do_addi();
int do_sub();
int do_subi();
int do_mul();
int do_div();
int do_and();
int do_or();
int do_nor();
int do_notb();
int do_sal();
int do_sar();
int do_equ();
int do_lt();
int do_lte();
int do_notc();
void error(char *msg);
    
// This table maps instruction code to corresponding simulation function.
int (*g_func_map[])() = {do_hlt, do_jmp, do_cjmp, do_ojmp, do_call,    //4
                         do_ret, do_push, do_pop, do_loadb, do_loadw,  //9
                         do_storeb, do_storew, do_loadi, do_nop, do_in,//14
                         do_out, do_add, do_addi, do_sub, do_subi,     //19
                         do_mul, do_div, do_and, do_or, do_nor,        //24
                         do_notb, do_sal, do_sar, do_equ, do_lt,       //29
                         do_lte, do_notc};                             //31

int do_hlt() {
  g_vm_state.stopped = true;
  return 0;
}

int do_jmp() {
  g_vm_state.PC = ADDR() - 4;
  return 0;
}

int do_cjmp() {
  if (g_vm_state.PSW.CF)
    g_vm_state.PC = ADDR() - 4;
  return 0;
}

int do_ojmp() {
  if (g_vm_state.PSW.OF)
    g_vm_state.PC = ADDR() - 4;
  return 0;
}

int do_call() {
  // We need to save general_regs and PC during CALL. (2*8+4+4 = 24).
  if (g_vm_state.ES_TOP + 24 > ES_SIZE) {
    puts("Extended-Stack overflow!");
    return -1;
  }
  memcpy(g_vm_state.ES + g_vm_state.ES_TOP, g_vm_state.general_regs, 16);
  memcpy(g_vm_state.ES + g_vm_state.ES_TOP + 16, &g_vm_state.PC, 4);
  memcpy(g_vm_state.ES + g_vm_state.ES_TOP + 20, &g_vm_state.PSW, 4);
  g_vm_state.ES_TOP += 24;
  g_vm_state.PC = ADDR() - 4;
  return 0;
}

int do_ret() {
  if (g_vm_state.ES_TOP  < 24) {
    // The outter most RET. Program exits normally.
    g_vm_state.stopped = true;
    return 0;
  }
  g_vm_state.ES_TOP -= 24;
  memcpy(g_vm_state.general_regs, g_vm_state.ES + g_vm_state.ES_TOP, 16);
  memcpy(&g_vm_state.PC, g_vm_state.ES + g_vm_state.ES_TOP + 16, 4);
  memcpy(&g_vm_state.PSW, g_vm_state.ES + g_vm_state.ES_TOP + 20, 4);
  return 0;
}

int do_push() {
  if (g_vm_state.SS_TOP + 2 > STACK_SIZE) {
    puts("Stack overflow!");
    return -1;
  }
  *(int16_t *)(g_vm_state.SS+g_vm_state.SS_TOP) = REG0_VAL;
  g_vm_state.SS_TOP += 2;
  return 0;
}

int do_pop() {
  if (REG0() == 0) return -1;
  if (g_vm_state.SS_TOP < 2) {
    puts("Stack underflow!");
    return -1;
  }
  g_vm_state.SS_TOP -= 2;
  REG0_VAL = *(int16_t *)(g_vm_state.SS+g_vm_state.SS_TOP);
  return 0;
}

int do_loadb() {
  REG0_VAL = g_vm_state.DS[ADDR() + REGG_VAL];
  return 0;
}

int do_loadw() {
  if (REG0() == 0) return -1;
  REG0_VAL = *(int16_t *)(g_vm_state.DS + ADDR() + REGG_VAL*2);
  return 0;
}

int do_storeb() {
  g_vm_state.DS[ADDR() + REGG_VAL] =  g_vm_state.general_regs[REG0()];
  return 0;
}

int do_storew() {
  *(int16_t *)(g_vm_state.DS + ADDR() + REGG_VAL*2) = REG0_VAL;
  return 0;
}

int do_loadi() {
  if (REG0() == 0) return -1;
  REG0_VAL = IMMEDIATE();
  return 0;
}

int do_nop() {
  return 0;
}

int do_in() {
  if (REG0() == 0) return -1;
  if (PORT() != 0) {
    puts("Invalid input port!");
    return -1;
  }
  REG0_VAL = getchar();
  return 0;
}

int do_out() {
  if (PORT() != 15) {
    puts("Invalid output port!");
    return -1;
  }
  putchar(REG0_VAL);
  fflush(stdout);
  return 0;
}

void check_overflow(int32_t result) {
  if (result < (int16_t)(0x8000) || result > (int16_t)(0x7fff))
    g_vm_state.PSW.OF = 1;
  else
    g_vm_state.PSW.OF = 0;
}

int do_add() {
  if (REG0() == 0) return -1;
  int32_t result = REG1_VAL + REG2_VAL;
  check_overflow(result);
  REG0_VAL = result;  
  return 0;
}

int do_addi() {
  if (REG0() == 0) return -1;
  int32_t result = REG0_VAL + IMMEDIATE();
  check_overflow(result);
  REG0_VAL = result;  
  return 0;
}

int do_sub() {
  if (REG0() == 0) return -1;
  int32_t result = REG1_VAL - REG2_VAL;
  check_overflow(result);
  REG0_VAL = result;  
  return 0;
}

int do_subi() {
  if (REG0() == 0) return -1;
  int32_t result = REG0_VAL - IMMEDIATE();
  check_overflow(result);
  REG0_VAL = result;  
  return 0;
}

int do_mul() {
  if (REG0() == 0) return -1;
  int32_t result = REG1_VAL * REG2_VAL;
  check_overflow(result);
  REG0_VAL = result;  
  return 0;
}

int do_div() {
  if (REG0() == 0) return -1;
  if (REG2_VAL == 0) {
    puts("0-div!");
    return -1;
  }
  int32_t result = REG1_VAL / REG2_VAL;
  check_overflow(result);
  REG0_VAL = result;  
  return 0;
}

int do_and() {
  if (REG0() == 0) return -1;
  REG0_VAL = REG1_VAL & REG2_VAL;
  return 0;
}

int do_or() {
  if (REG0() == 0) return -1;
  REG0_VAL = REG1_VAL | REG2_VAL;
  return 0;
}

int do_nor() {
  if (REG0() == 0) return -1;
  REG0_VAL = REG1_VAL ^ REG2_VAL;
  return 0;
}

int do_notb() {
  if (REG0() == 0) return -1;
  REG0_VAL = ~REG1_VAL;
  return 0;
}

int do_sal() {
  if (REG0() == 0) return -1;
  REG0_VAL = REG1_VAL << REG2_VAL;
  return 0;
}

int do_sar() {
  if (REG0() == 0) return -1;
  uint16_t result = REG1_VAL;
  for (int i = 0; i < REG2_VAL && i < 16; ++i) {
    result >>= 1;
    result |= (result << 1) & 0x8000;
  }
  REG0_VAL = result;
  return 0;
}

int do_equ() {
  if (REG0_VAL == REG1_VAL)
    g_vm_state.PSW.CF = 1;
  else
    g_vm_state.PSW.CF = 0;
  return 0;
}

int do_lt() {
  if (REG0_VAL < REG1_VAL)
    g_vm_state.PSW.CF = 1;
  else
    g_vm_state.PSW.CF = 0;
  return 0;
}

int do_lte() {
  if (REG0_VAL <= REG1_VAL)
    g_vm_state.PSW.CF = 1;
  else
    g_vm_state.PSW.CF = 0;
  return 0;
}

int do_notc() {
  g_vm_state.PSW.CF = ~g_vm_state.PSW.CF;
  return 0;
}

// Print error massage and exit.
void error(char *msg) {
  printf("Error: %s\n", msg);
  exit(EXIT_FAILURE);
}

void init_vm_state(char *argv[]) {
  uint32_t ds_size, cs_size;
  FILE *fp;

  fp = fopen(argv[1], "rb");
  if (!fp) error("Can't open input file!");
  fread(&ds_size, 4, 1, fp);
  fread(&cs_size, 4, 1, fp);

  memset(&g_vm_state, 0, sizeof(VMState));
  
  // Load code and data.
  g_vm_state.DS = malloc(ds_size);
  if (!g_vm_state.DS) error("Out of memory!");
  if (1 != fread(g_vm_state.DS, ds_size, 1, fp))
    error("Input file corrupted!");
  
  g_vm_state.CS = malloc(cs_size);
  if (!g_vm_state.CS) error("Out of memory!");
  if (1 != fread(g_vm_state.CS, cs_size, 1, fp))
    error("Input file corrupted!");

  // Allocate stack and extended segment.
  g_vm_state.SS = malloc(STACK_SIZE);
  if (!g_vm_state.SS) error("Out of memory!");

  g_vm_state.ES = malloc(ES_SIZE);
  if (!g_vm_state.ES) error("Out of memory!");

  fclose(fp);
}

// Release all the resources.
void destroy_vm_state() {
  free(g_vm_state.DS);
  free(g_vm_state.CS);
  free(g_vm_state.SS);
  free(g_vm_state.ES);
}

void usage_and_die() {
  printf("Usage: ssim file_name\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  if (argc < 2) usage_and_die();

  uint32_t opcode;
  
  atexit(destroy_vm_state); // For a clean exit;
  init_vm_state(argv);
  g_vm_state.IR = *(uint32_t *)(g_vm_state.CS + g_vm_state.PC);
  // The load-decode-eval loop.
  while (!g_vm_state.stopped) {
    opcode = OPCODE();
    if (opcode > 31) error("Invalid opcode!");
    //    printf("PC: %d\n", g_vm_state.PC);
    if (0 != (*g_func_map[opcode])()) {
      printf("PC: %d\n", g_vm_state.PC);
      error("Execution error!");
    }
    g_vm_state.PC += 4;
    g_vm_state.IR = *(uint32_t *)(g_vm_state.CS + g_vm_state.PC);
  }
  
  return 0;
}
