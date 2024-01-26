#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, char* argv[])
{
    init_VM();
    Chunk chunk;
    init_chunk(&chunk);
    
    int a = add_constant(&chunk, 1.2);
    write_chunk(&chunk, op_constant, 123);
    write_chunk(&chunk, a, 123);

    int b = add_constant(&chunk, 3.4);
    write_chunk(&chunk, op_constant, 123);
    write_chunk(&chunk, b, 123);

    write_chunk(&chunk, op_add, 123);

    int c = add_constant(&chunk, 5.6);
    write_chunk(&chunk, op_constant, 123);
    write_chunk(&chunk, c, 123);

    write_chunk(&chunk, op_divide, 123);
    write_chunk(&chunk, op_negate, 123);
    write_chunk(&chunk, op_return, 123);
    
    interpret(&chunk);
    
    free_VM();
    free_chunk(&chunk);
    return 0;
}
