#define BM_IMPLEMENTATION
#include "./bm.h"

char *shift(int *argc, char ***argv)
{
   assert(*argc > 0);
   char *result = **argv;
   *argv += 1;
   *argc -= 1;
   return result;
}

void usage(FILE *stream, const char *program)
{
  fprintf(stream, "Usage: %s <input.basm> <output.bm>\n", program);
}

int main(int argc, char **argv)
{  
  const char *program = shift(&argc, &argv); // skip program name

  if (argc == 0) {
    usage(stderr, program);
    fprintf(stderr, "./basm <input.basm> <output.bm>\n");
    fprintf(stderr, "ERROR: expected input\n");
    exit(1);
  }

  const char *input_file_path = shift(&argc, &argv);
  if (argc == 0) {
    usage(stderr, program);
    fprintf(stderr, "./basm <input.basm> <output.bm>\n");
    fprintf(stderr, "ERROR: expected output\n");
    exit(1);
  }
  
  const char *output_file_path = shift(&argc, &argv);

  String_View source = slurp_file(input_file_path);

  bm.program_size = bm_translate_source(source,bm.program,BM_PROGRAM_CAPACITY);

  bm_save_program_to_file(&bm, output_file_path);

  return 0;
}