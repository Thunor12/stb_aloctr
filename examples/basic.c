#include <stdio.h>

#define STB_ALOCTR_IMPLEMENTATION
#include "../stb_aloctr.h"

int basic_static(void)
{
    printf("Basic Static\n");

    aloctr aloc = {0};
    uint8_t buf[256] = {0};

    if(!aloctr_static_init(&aloc, sizeof(buf), buf)) { return 1; }
    {

        uint32_t* a = (uint32_t*) aloctr_alloc(&aloc, sizeof(uint32_t));
        float* f = (float*) aloctr_alloc(&aloc, sizeof(float));

        *a = 32;
        *f = 69.0f;

        printf("a = %d\n", *a);
        printf("f = %f\n", *f);

        printf("Allocating 4KB but will fail\n");
        uint8_t* buf = (uint8_t*) aloctr_alloc(&aloc, 4 * 1024);
        if(!buf)
        {
            printf("Failed to alloc as expected\n");
        }

    }
    aloctr_free(&aloc);
    printf("--------------------------------\n");

    return 0;
}

int basic_dynamic(void)
{
    printf("Basic Dynamic\n");

    aloctr aloc = {0};

    if(!aloctr_dynamic_init(&aloc)) { return 1; }
    {
        uint32_t* a = (uint32_t*) aloctr_alloc(&aloc, sizeof(uint32_t));
        float* f = (float*) aloctr_alloc(&aloc, sizeof(float));

        *a = 32;
        *f = 69.0f;

        printf("a = %d\n", *a);
        printf("f = %f\n", *f);

        uint8_t* buf = (uint8_t*) aloctr_alloc(&aloc, 4 * 1024);
        buf[0] = 69;
        buf[1] = 250;
        printf("buf[0] = %d\n", buf[0]);
        printf("buf[1] = %d\n", buf[1]);
    }
    aloctr_free(&aloc);
    printf("--------------------------------\n");

    return 0;
}

int main(void)
{

    if(0 != basic_static()) { return 1; }
    if(0 != basic_dynamic()) { return 1; }

    return 0;
}

