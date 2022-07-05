#include "util.h"

unsigned int powof2(unsigned int num)
{
    if (num != 0) {
	num--;
	num |= (num >> 1);
	num |= (num >> 2);
	num |= (num >> 4);
	num |= (num >> 8);
	num |= (num >> 16);
	num++;
    }

    return num;
}

int ispow2(int num)
{
    return (num && !(num & (num - 1)));
}