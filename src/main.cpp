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

    /*Tile tile(4, 4, 0, 0);
    tile.data = {
        {10, 20, 30, 40},
        {50, 60, 70, 80},
        {90, 100, 110, 120},
        {130, 140, 150, 160}
    };
    
    std::vector<Tile> tiles = {tile};
    
    // Appliquer la transformée
    applyWaveletTransform53ToTiles(tiles);
    
    // Afficher le résultat
    for (const auto& row : tiles[0].data) {
        for (int val : row) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }

    printf("\n \n");
    
    // Appliquer l'inverse
    inverseWaveletTransform53ToTiles(tiles);
    
    // Afficher le résultat inverse
    for (const auto& row : tiles[0].data) {
        for (int val : row) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }*/




    imIn.load(cNomImgLue.data());

    compression2000(cNomImgLue.data(), cNomImgOut.data(), imIn);

    printf("compression finie \n ================================================ \n");

    long sizeImin = getFileSize(cNomImgLue); //fontion dans Utils.h
    long sizeComp = getFileSize(cNomImgOut);
    double taux = (double)sizeImin/(double)sizeComp;

    printf("taux de compression : %f \n",taux);


    printf("debut décompression \n ================================================ \n");

    ImageBase * imOut;
    std::string fileDecomp = "./img/out/decompressed.ppm";

    decompression2000(cNomImgOut.c_str(), fileDecomp.c_str(), imOut);

    printf("Decompression fini\n\n");

    ImageBase imOut2;
    imOut2.load(fileDecomp.data());

    float psnr = PSNR(imIn, imOut2);

    printf("PSNR : %f\n", psnr , "Db");
    printf("taux de compression : %f \n",taux);

    //free(imIn);
    //free(imOut);

    return 0;
}