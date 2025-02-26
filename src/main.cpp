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

    ImageBase  imIn;

    imIn.load(cNomImgLue.data());

    compression(cNomImgLue.data(), cNomImgOut.data(), imIn);

    ImageBase * imOut;

    std::string fileDecomp = "./img/out/decompressed.ppm";

    decompression(cNomImgOut.c_str(), fileDecomp.c_str(), imOut);

    printf("Decompression fini\n");

    ImageBase imOut2;
    imOut2.load(fileDecomp.data());

    float psnr = PSNR(imIn, imOut2);

    printf("PSNR : %f\n", psnr);

    //free(imIn);
    free(imOut);

    return 0;
}