#include <iostream>
using std::cout;
using std::endl;

#include "CPU.h"

CPU::CPU()
{ 
    regAF=0x01B0;
    regBC=0x0013;
    regDE=0x00D8;
    regHL=0x014D;
}

