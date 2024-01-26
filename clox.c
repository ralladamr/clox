#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, char* argv[])
{
    init_VM();
    Chunk chunk;
    init_chunk(&chunk);
    int constant = add_constant(&chunk, 1.2);
    write_chunk(&chunk, op_constant, 123);
    write_chunk(&chunk, constant, 123);
    write_chunk(&chunk, op_return, 123);
    disassemble_chunk(&chunk, "test chunk");
    interpret(&chunk);
    free_VM();
    free_chunk(&chunk);
    return 0;
}
