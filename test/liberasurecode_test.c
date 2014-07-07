#include "erasurecode.h"

int main()
{
    ec_backend_t backend;
    
    int err = liberasurecode_backend_create_instance(&backend, 
            "flat_xor_3", 10, 4, 0, 0, 0, 0);
    if (backend)
        liberasurecode_backend_destroy_instance(backend);
    return 0;
}

