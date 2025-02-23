
#include <stdio.h>


#include "ImageBase.h"
#include "JPEG.h"
#include "Utils.h"
#include <string>
#include <iostream>
#include <vector>



int main(int argc, char **argv)
{
    char cNomImgLue[250];
    if (argc < 2)
    {
        printf("Usage: filename \n");
        return 1;
    }
    sscanf(argv[1], "%s", cNomImgLue);

    compression(cNomImgLue);


   

    

    

    return 0;
}