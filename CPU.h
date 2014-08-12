#include <stdio.h>


#define FLAG_Z 7;
#define FLAG_N 6;
#define FLAG_H 5;
#define FLAG_C 4;

typedef unsigned char BYTE;
typedef char SIGNED_BYTE;
typedef unsigned short WORD;
typedef signed short SIGNED_WORD;

union Register
{
    WORD reg;
    struct
    {
        BYTE low;
        BYTE high;
    };
};

class CPU
{
public:
    CPU();
   
private:
    Register regAF;
    Register regBC;
    Register regDE;
    Register regHL;  
};


