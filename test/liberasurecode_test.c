#include "erasurecode.h"

int main()
{
    int desc = liberasurecode_instance_create("flat_xor_3", 10, 4, 0, NULL);
    liberasurecode_instance_destroy(desc);
    return 0;
}

