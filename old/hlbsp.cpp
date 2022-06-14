#include <stdio.h>
#include "hlbsp.h"
#include "brush.h"

int main(int argc, char *argv[])
{
    if(argc != 3) return 1;
    BspExport bsp;
    bsp.read_bsp(argv[1]);
    bsp.save_col(argv[2]);
    return 0;
}