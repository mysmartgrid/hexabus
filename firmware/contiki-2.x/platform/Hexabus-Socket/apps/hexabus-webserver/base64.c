#include <stdio.h>

void b64_decode(unsigned char* input, unsigned char* output) {
   
    printf("%c%c%c%c ", input[0], input[1], input[2], input[3]);
    
    int i=0;
    for(;i<4;i++) {
        if(input[i]=='+')
            input[i] = 62;
        else if(input[i]=='/')
            input[i] = 63;
        else if(input[i]=='=')
            input[i] == 0;
        else if(input[i]>=48&&input[i]<=57)
            input[i] +=4;
        else if(input[i]>=97&&input[i]<=122)
            input[i] -= 71;
        else
            input[i] -= 65;
    }
    
    output[0] = (((input[0]<<2)&0xfc) | ((input[1]>>4)&0x03));
    output[1] = (((input[1]<<4)&0xf0) | ((input[2]>>2)&0x0f));
    output[2] = (((input[2]<<6)&0xc0) | (input[3]&0x3f));

    printf("= %hx %hx %hx\n",output[0],output[1],output[2]);

}
