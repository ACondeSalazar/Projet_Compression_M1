
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION //important sinon stb ne marche pas
#include "stb_image.h"
#include "ImageBase.h"


int main(int argc, char **argv)
{
    char cNomImgLue[250];
    if (argc < 2)
    {
        printf("Usage: filename \n");
        return 1;
    }
    sscanf(argv[1], "%s", cNomImgLue);

    printf("Opening image : %s\n", cNomImgLue);

    

}