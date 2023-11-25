#ifndef _BM_H_
#define _BM_H_

#include<assert.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<errno.h>
#include<ctype.h>

#define ARRAY_SIZE(xs) (sizeof(xs) / sizeof(xs[0]))
#define BM_STACK_CAPACITY 1024
#define BM_PROGRAM_CAPACITY 1024
#define LABEL_CAPACITY 1024
#define UNRESOLVED_JMPS_CAPACITY 1024

#define MAKE_INST_PUSH(value) {.type = INST_PUSH, .operand = (value)}
#define MAKE_INST_PLUS {.type = INST_PLUS}
#define MAKE_INST_MINUS {.type = INST_MINUS}
#define MAKE_INST_MULT {.type = INST_MULT}
#define MAKE_INST_DIV {.type = INST_DIV}
#define MAKE_INST_JMP(addr) {.type = INST_JMP, .operand = (addr)}
#define MAKE_INST_DUP(addr) {.type = INST_DUP, .operand = (addr)}
#define MAKE_INST_HALT {.type = INST_JMP, .operand = (addr)}

typedef enum {
  TRAP_OK = 0,
  TRAP_STACK_OVERFLOW,
  TRAP_STACK_UNDERFLOW,
  TRAP_ILLEGAL_INST,
  TRAP_ILLEGAL_INST_ACCESS,
  TRAP_ILLEGAL_OPERAND,
  TRAP_DIV_BY_ZERO,
} Trap;

typedef int64_t Word;

typedef enum {
    INST_NOP = 0,
    INST_PUSH,
    INST_DUP,
    INST_PLUS,
    INST_MINUS,
    INST_MULT,
    INST_DIV,
    INST_JMP,
    INST_JMP_IF,
    INST_EQ,
    INST_HALT,
    INST_PRINT_DEBUG,
} INST_Type;

typedef struct {
    INST_Type type;
    Word operand;
} Inst;

typedef struct {
   Word stack[BM_STACK_CAPACITY];
   Word stack_size;

   Word ip;
   Inst program[BM_STACK_CAPACITY];
   Word program_size;

   int halt;
} Bm;


Bm bm = {0};

typedef struct 
{
  size_t count;
  const char *data;
} String_View;

Trap bm_execute_inst(Bm *bm);
void bm_dump_stack(FILE *stream, const Bm *bm);


typedef struct {
  String_View name;
  Word addr;
} Label;

typedef struct {
  Word addr;
  String_View label;
} Unresolved_Jmp;

typedef struct {
  Label labels[LABEL_CAPACITY];
  size_t labels_size;
  Unresolved_Jmp unresolved_jmps[UNRESOLVED_JMPS_CAPACITY];
  size_t unresolved_jmps_size;
} Label_Table;

Word label_table_find(const Label_Table *llt, String_View name);
void label_table_push(Label_Table *lt, String_View name, Word addr);
void label_table_push_unresolved_jmp(Label_Table *lt, Word addr, String_View label);

void bm_translate_source(String_View source, Bm *bm, Label_Table *lt);

#endif // __BM_H_

#ifdef BM_IMPLEMENTATION

const char *trap_as_cstr(Trap trap)
{
  switch (trap) {
      case TRAP_OK:
          return "TRAP_OK";
      case TRAP_STACK_OVERFLOW:
          return "TRAP_STACK_OVERFLOW";
      case TRAP_STACK_UNDERFLOW:
          return "TRAP_STACK_UNDERFLOW";
      case TRAP_ILLEGAL_INST:
          return "TRAP_ILLEGAL_INST";
      case TRAP_ILLEGAL_INST_ACCESS:
          return "TRAP_ILLEGAL_INST_ACCESS";
      case TRAP_ILLEGAL_OPERAND:
          return "TRAP_ILLEGAL_OPERAND";
      case TRAP_DIV_BY_ZERO:
          return "TRAP_DIV_BY_ZERO";
      default:
          assert(0 && "trap_as_cstr: Unreachable");
  }
}



const char *inst_type_as_cstr(INST_Type type)
{
   switch(type) {
     case  INST_NOP:  return "INST_NOP";
     case  INST_PUSH: return "INST_PUSH";
     case  INST_PLUS: return "INST_PLUS";
     case INST_MINUS: return "INST_MINUS";
     case  INST_MULT: return "INST_MULT";
     case   INST_DIV: return "INST_DIV";
     case   INST_JMP: return "INST_JMP";
     case  INST_HALT: return "INST_HALT";
     case  INST_JMP_IF: return "INST_JMP_IF";
     case   INST_EQ: return "INST_EQ";
     case INST_PRINT_DEBUG: return " INST_PRINT_DEBUG";
     case INST_DUP: return "INST_DUP";
     default: assert(0 && "trap_as_cstr: Unreachable");
   }
}



Trap bm_execute_program(Bm *bm, int limit)
{
  while (limit != 0 && !bm->halt) {
    Trap trap = bm_execute_inst(bm);
    if (trap != TRAP_OK) {
      return trap;
    }
    if (limit > 0) {
      --limit;
    }
  }
  return TRAP_OK;
}

Trap bm_execute_inst(Bm *bm) 
{
  if (bm->ip < 0 || bm->ip > bm->program_size) {
     return TRAP_ILLEGAL_INST_ACCESS;
  }

  Inst inst = bm->program[bm->ip];

  switch (inst.type) {
    case INST_NOP:
        bm->ip += 1;
        break;
    case INST_PUSH:
       if (bm->stack_size >= BM_STACK_CAPACITY) {
          return TRAP_STACK_OVERFLOW;
       }

       bm->stack[bm->stack_size++] = inst.operand;
       bm->ip += 1;
       break; 

    case INST_PLUS:
       if (bm->stack_size < 2) {
         return TRAP_STACK_UNDERFLOW;
       }

       bm->stack[bm->stack_size - 2] +=  bm->stack[bm->stack_size - 1];
       bm->stack_size -= 1;
       bm->ip += 1;
       break;

    case INST_MINUS:
      if (bm->stack_size < 2) {
         return TRAP_STACK_UNDERFLOW;
       }

       bm->stack[bm->stack_size - 2] -=  bm->stack[bm->stack_size - 1];
       bm->stack_size -= 1;
       bm->ip += 1;
       break;

    case INST_MULT:
       if (bm->stack_size < 2) {
          return TRAP_STACK_UNDERFLOW;
        }
        bm->stack[bm->stack_size - 2] *=  bm->stack[bm->stack_size - 1];
        bm->stack_size -= 1;
        bm->ip += 1;
        break;

    case INST_DIV:
       if (bm->stack_size < 2) {
         return TRAP_STACK_UNDERFLOW;
       }

       if (bm->stack[bm->stack_size - 1] == 0) {
         return TRAP_DIV_BY_ZERO;
       }

       bm->stack[bm->stack_size - 2] /=  bm->stack[bm->stack_size - 1];
       bm->stack_size -= 1;
       bm->ip += 1;
       break;

    case INST_JMP:
      bm->ip = inst.operand;
      break;

    case INST_HALT:
      bm->halt = 1;
      break;

    case INST_JMP_IF:
      if (bm->stack_size < 1) {
         return TRAP_STACK_UNDERFLOW;
      }

      if (bm->stack[bm->stack_size - 1]) {
          bm->stack_size -= 1;
          bm->ip = inst.operand;
      } else {
          bm->ip += 1;
      }
      break;

    case INST_EQ:
      if (bm->stack_size < 2) {
         return TRAP_STACK_UNDERFLOW;
       }

      bm->stack[bm->stack_size - 2] =  bm->stack[bm->stack_size - 1] ==  bm->stack[bm->stack_size - 2];
       bm->stack_size -= 1;
       bm->ip += 1;
      break;

    case INST_PRINT_DEBUG:
      if (bm->stack_size < 2) {
         return TRAP_STACK_UNDERFLOW;
       }
      printf("%ld\n", bm->stack[bm->stack_size - 1]);
      bm->stack_size -= 1;
      bm->ip += 1;
      break;

    case INST_DUP:
      if (bm->stack_size >= BM_STACK_CAPACITY) {
          return TRAP_STACK_OVERFLOW;
       }

      if (bm->stack_size - inst.operand <= 0) {
        return TRAP_STACK_UNDERFLOW;
      }

      if (inst.operand < 0) {
        return TRAP_ILLEGAL_OPERAND;
      }

      bm->stack[bm->stack_size] = bm->stack[bm->stack_size - 1 - inst.operand];

      bm->stack_size += 1;
      bm->ip += 1;
      break;

    default:
      return TRAP_ILLEGAL_INST;    
  }
  return TRAP_OK;
}

void bm_dump_stack(FILE *stream, const Bm *bm)
{
   fprintf(stream, "Stack:\n");
   if (bm->stack_size > 0) {
      for (Word i = 0; i < bm->stack_size; ++i) {
          fprintf(stream, " %ld\n", bm->stack[i]);
       }
   } else {
     fprintf(stream, "  [empty]\n");
   }
}

void bm_load_program_from_memory(Bm *bm, Inst *program, size_t program_size)
{
  assert(program_size < BM_PROGRAM_CAPACITY);
  memcpy(bm->program, program, sizeof(program[0]) * program_size);
  bm->program_size = program_size;
}

void bm_load_program_from_file(Bm *bm, const char *file_path)
{
   FILE *f = fopen(file_path, "rb");
   if (f == NULL) {
     fprintf(stderr, "ERROR: could not open file %s : %s\n", file_path, strerror(errno));
     exit(1);
   }
  if (fseek(f, 0, SEEK_END) < 0) {
     fprintf(stderr, "ERROR: could not read file %s : %s\n", file_path, strerror(errno));
     exit(1);
  }

  long m = ftell(f);
  if (m < 0) {
     fprintf(stderr, "ERROR: could not read file %s : %s\n", file_path, strerror(errno));
     exit(1);
  }

  assert(m % sizeof(bm->program[0]) == 0);
  assert((size_t) m <= BM_PROGRAM_CAPACITY * sizeof(bm->program[0]));

  if (fseek(f, 0, SEEK_SET) < 0) {
     fprintf(stderr, "ERROR: could not read file %s : %s\n", file_path, strerror(errno));
     exit(1);
  }

  bm->program_size = fread(bm->program, sizeof(bm->program[0]), m / sizeof(bm->program[0]), f);
  if (ferror(f)) {
    fprintf(stderr, "ERROR: could not read file %s : %s\n", file_path, strerror(errno));
     exit(1);
  }

  fclose(f);
}

void bm_save_program_to_file(const Bm *bm, const char *file_path)
{
  FILE *f = fopen(file_path, "wb");
  if (f == NULL) {
    fprintf(stderr, "ERROR: could not open file %s : %s\n", file_path, strerror(errno));
    exit(1);
  }

  fwrite(bm->program, sizeof(bm->program[0]), bm->program_size, f);

  if (ferror(f)) {
    fprintf(stderr, "ERROR: could not write to file %s : %s\n", file_path, strerror(errno));
    exit(1);
  }

  fclose(f);
}

String_View cstr_as_sv(const char *cstr)
{
  return (String_View) {
    .count = strlen(cstr),
    .data = cstr,
  };
}

String_View sv_trim_left(String_View sv)
{
  size_t i = 0;
  while (i < sv.count && isspace(sv.data[i])) {
    i += 1;
  }

  return (String_View) {
    .count = sv.count - i,
    .data = sv.data + i,
  };
}

String_View sv_trim_right(String_View sv)
{
  size_t i = 0;
  while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
     i += 1;
  }
 return (String_View) {
   .count = sv.count - i,
   .data = sv.data,
 };
}

String_View sv_trim(String_View sv)
{
    return sv_trim_right(sv_trim_left(sv));
}

String_View sv_chop_by_delim(String_View *sv, char delim)
{
  size_t i = 0;
  while (i < sv->count && sv->data[i] != delim) {
       i += 1;
  }

  String_View result = {
     .count = i,
     .data = sv->data,
  };

  if (i < sv->count) {
    sv->count -= i + 1;
    sv->data += i + 1;
  } else {
    sv->count -= i;
    sv->data += i;
  }

  return result;
}

int sv_eq(String_View a, String_View b)
{
  if (a.count != b.count) {
    return 0;
  } else {
    return memcmp(a.data, b.data, a.count) == 0;
  }
}

int sv_to_int(String_View sv)
{
   int result = 0;

   for (size_t i = 0; i < sv.count && isdigit(sv.data[i]); ++i) {
     result = result * 10 + sv.data[i] - '0';
   }

  return result;
}

Word label_table_find(const Label_Table *lt, String_View name)
{
    for (size_t i = 0; i < lt->labels_size; ++i) {
      if (sv_eq(lt->labels[i].name, name)) {
         return lt->labels[i].addr;
      }
    }
    fprintf(stderr, "ERROR: label `%.*s` does not exist\n", (int) name.count, name.data);
    exit(1);
}

void label_table_push(Label_Table *lt, String_View name, Word addr)
{
  assert(lt->labels_size < LABEL_CAPACITY);
  lt->labels[lt->labels_size++] = (Label) {.name = name, .addr = addr};
}

void label_table_push_unresolved_jmp(Label_Table *lt, Word addr, String_View label)
{
  assert(lt->unresolved_jmps_size < UNRESOLVED_JMPS_CAPACITY);
  lt->unresolved_jmps[lt->unresolved_jmps_size++] =    
      (Unresolved_Jmp) {.addr = addr, .label = label};
}

void bm_translate_source(String_View source, Bm *bm, Label_Table *lt)
{
   bm->program_size = 0;

   // First pass
   while (source.count > 0) {
     assert(bm->program_size < BM_PROGRAM_CAPACITY);
     String_View line = sv_trim(sv_chop_by_delim(&source, '\n'));
     if (line.count > 0 && *line.data != '#') {
        String_View inst_name = sv_chop_by_delim(&line, ' ');
        String_View operand = sv_trim(sv_chop_by_delim(&line, '#'));

        if (inst_name.count > 0 && inst_name.data[inst_name.count - 1] == ':') {
          String_View label = {
             .count = inst_name.count - 1,
             .data = inst_name.data,
          };

          label_table_push(lt, label, bm->program_size);
        } else if (sv_eq(inst_name, cstr_as_sv("nop"))) {
           bm->program[bm->program_size++] = (Inst) {
             .type = INST_NOP,
           };
        } else if (sv_eq(inst_name, cstr_as_sv("push"))) {
           bm->program[bm->program_size++] = (Inst) {
             .type = INST_PUSH, 
             .operand = sv_to_int(operand)
           };
        } else if (sv_eq(inst_name, cstr_as_sv("dup"))) {
          bm->program[bm->program_size++] = (Inst) { 
            .type = INST_DUP, 
            .operand = sv_to_int(operand)
          };
        } else if (sv_eq(inst_name, cstr_as_sv("plus"))) {
          bm->program[bm->program_size++] = (Inst) { 
            .type = INST_PLUS
          };
        } else if (sv_eq(inst_name, cstr_as_sv("jmp"))) {
          if (operand.count > 0 && isdigit(*operand.data)) {
            bm->program[bm->program_size++] =(Inst) {  
              .type = INST_JMP,
              .operand = sv_to_int(operand),
            };
          } else {
            label_table_push_unresolved_jmp(
              lt, bm->program_size, operand);

            bm->program[bm->program_size++] =(Inst) {  
              .type = INST_JMP
            };
          }
        } else {
          fprintf(stderr, "ERROR: unknown unstruction `%.*s`", (int) inst_name.count, inst_name.data);
          exit(1);
        }
     }
   }
    // Second pass
    for (size_t i = 0; i < lt->unresolved_jmps_size; ++i) {

        Word addr = label_table_find(lt,lt->unresolved_jmps[i].label);
        bm->program[lt->unresolved_jmps[i].addr].operand = addr;

    }
}

String_View sv_slurp_file(const char *file_path)
{
  FILE *f = fopen(file_path, "r");
  if (f == NULL) {
     fprintf(stderr, "ERROR: could not read file %s : %s\n", file_path, strerror(errno));
     exit(1);
  }

  if (fseek(f, 0, SEEK_END) < 0) {
     fprintf(stderr, "ERROR: could not read file %s : %s\n", file_path, strerror(errno));
     exit(1);
  }

  long m = ftell(f);
  if (m < 0) {
     fprintf(stderr, "ERROR: could not read file %s : %s\n", file_path, strerror(errno));
     exit(1);
  }

  char *buffer = malloc(m);
  if (buffer == NULL) {
    fprintf(stderr, "ERROR: could not allocate memory for file: %s\n", strerror(errno));
     exit(1);
  }

  if (fseek(f, 0, SEEK_SET) < 0) {
     fprintf(stderr, "ERROR: could not read file %s : %s\n", file_path, strerror(errno));
     exit(1);
  }

  size_t n = fread(buffer, 1, m, f);
  if (ferror(f)) {
     fprintf(stderr, "ERROR: could not read file %s : %s\n", file_path, strerror(errno));
     exit(1);
  }

  fclose(f);

  return (String_View) {
    .count = n,
    .data = buffer,
  };
}

#endif// BM_IMPLEMENTATION