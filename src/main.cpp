#include <stdio.h>


#include "ImageBase.h"
#include "JPEG.h"
#include "Utils.h"
#include <string>
#include <iostream>
#include <vector>




int main(int argc, char **argv)
{
    std::string cNomImgLue;
    std::string cNomImgOut;
    if (argc < 3)
    {
        printf("Usage: fileIn fileOut \n");
        return 1;
    }
    cNomImgLue = argv[1];
    cNomImgOut = argv[2];

    ImageBase imIn;

    compression(cNomImgLue.data(), cNomImgOut.data(), imIn);

    ImageBase imOut;

    std::string fileDecomp = "./decompressed.ppm";

    decompression(cNomImgOut.c_str(), fileDecomp.c_str(), imOut);
    
    ImageBase imY;
    imY.load("./img/out/Y.pgm");

    printf("PSNR : %f\n", PSNR(imY, imOut));

    

    return 0;
}