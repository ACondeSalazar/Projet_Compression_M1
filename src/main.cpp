#include <stdio.h>

#include "ImageBase.h"
#include "JPEG.h"
#include "JPEG2000.h"
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

    compression2000(cNomImgLue.data(), cNomImgOut.data(), imIn);

    printf("compression finie \n ================================================ \n");

    /*long sizeImin = getFileSize(cNomImgLue); //fontion dans Utils.h
    long sizeComp = getFileSize(cNomImgOut);
    double taux = (double)sizeImin/(double)sizeComp;

    printf("taux de compression : %f \n",taux);


    printf("debut décompression \n ================================================ \n");

    ImageBase * imOut;
    std::string fileDecomp = "./img/out/decompressed.ppm";

    decompression(cNomImgOut.c_str(), fileDecomp.c_str(), imOut);

    printf("Decompression fini\n\n");

    ImageBase imOut2;
    imOut2.load(fileDecomp.data());

    float psnr = PSNR(imIn, imOut2);

    printf("PSNR : %f\n", psnr , "Db");
    printf("taux de compression : %f \n",taux);

    //free(imIn);
    //free(imOut);*/

    return 0;
}