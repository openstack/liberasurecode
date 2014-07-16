#include "erasurecode.h"

int main()
{
    int blocksize = 4096;
    char **data = NULL, **parity = NULL;

    struct ec_args args = {
        .k = 10,
        .m = 4,
        .priv_args1.flat_xor_hd_args.hd = 3,
    };

    int desc = liberasurecode_instance_create("flat_xor_hd", &args);

    // liberasurecode_encode(desc, (const char *) *data, 0, data, parity);
    liberasurecode_instance_destroy(desc);
    return 0;
}

