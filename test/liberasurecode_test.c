#include "erasurecode.h"

int main()
{
    int blocksize = 4096;
    char **data = NULL, **parity = NULL;
    int desc = liberasurecode_instance_create("flat_xor_hd", 10, 4, 0, NULL);
    liberasurecode_encode(desc, data, parity, blocksize);
    liberasurecode_instance_destroy(desc);
    return 0;
}

