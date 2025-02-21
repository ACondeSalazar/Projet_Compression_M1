
#include <stdio.h>


#include "ImageBase.h"
#include "JPEG.h"
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

    printf("Opening image : %s\n", cNomImgLue);

    ImageBase imIn;
    imIn.load(cNomImgLue);

    //Transformation des couleurs
    printf("Transformation de l'espace couleur");
    ImageBase imY(imIn.getWidth(), imIn.getHeight(), false);
    ImageBase imCb(imIn.getWidth(), imIn.getHeight(), false);
    ImageBase imCr(imIn.getWidth(), imIn.getHeight(), false);

    RGB_to_YCbCr(imIn, imY, imCb, imCr);

    imY.save("./img/out/Y.pgm");
    imCb.save("./img/out/Cb.pgm");
    imCr.save("./img/out/Cr.pgm");

    printf("  Fini\n");

    //Sous échantillonage
    printf("Sous échantillonage de Cb et Cr");
    ImageBase downSampledCb(imIn.getWidth() / 2, imIn.getHeight() / 2, false);
    ImageBase downSampledCr(imIn.getWidth() / 2, imIn.getHeight() / 2, false);

    down_sampling(imCb, downSampledCb);
    down_sampling(imCr, downSampledCr);

    downSampledCb.save("./img/out/downSampledCb.pgm");
    downSampledCr.save("./img/out/downSampledCr.pgm");

    printf("  Fini\n");

    //Découpage en blocs de pixel
    printf("Découpage en blocs de pixel ");
    std::vector<Block> blocks = getBlocks(imY, 8);


    for (int i = 0; i < blocks.size(); i++){
        std::string filename = "./img/out/blocks/block" + std::to_string(i) + ".pgm";
        blocks[i].savePGM(filename.c_str());
    }


    printf("  Fini\n");

    /* // Print block 0
    printf("Block 0:\n");
    for (int i = 0; i < blocks[0].data.size(); i++) {
        for (int j = 0; j < blocks[0].data[0].size(); j++) {
            blocks[0].data[i][j] = 255;
            printf("%d ", blocks[0].data[i][j]);
            
        }
        printf("\n");
    } */

    //DCT
    printf("DCT ");
    
    std::vector<std::vector<float>> dctMatrix(8, std::vector<float>(8, 0));

    dctTransform(blocks[0].data, dctMatrix);

    printf("  Fini\n");

    /* printf("DCT Matrix:\n");
    for (const auto& row : dctMatrix) {
        for (const auto& val : row) {
            printf("%f ", val);
        }
        printf("\n");
    } */

    return 0;
}