#include "chunk.h"
#include "debug.h"

int main(int argc, char* argv[]) 
{
    Chunk chunk;
    init_chunk(&chunk);
    write_chunk(&chunk, op_return);
    disassemble_chunk(&chunk, "test chunk");
    free_chunk(&chunk);
    return 0;
}
