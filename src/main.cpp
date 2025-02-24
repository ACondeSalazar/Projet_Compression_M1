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

    compression(cNomImgLue.data(), cNomImgOut.data());

    std::string fileDecomp = "./decompressed.ppm";

    decompression(cNomImgOut.c_str(), fileDecomp.c_str());
   

    

    

    return 0;
}