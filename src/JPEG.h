#pragma once
#include "ImageBase.h"
#include <vector>
#include <fstream>
#include "Utils.h"
#include "RLE.h"
#include "Huffman.h"
#include <unordered_map>


//Transformation de couleurs

void RGB_to_YCbCr(ImageBase & imIn, ImageBase & Y, ImageBase & Cb, ImageBase & Cr){

    for(int i = 0; i < imIn.getHeight(); i++){
        for(int j = 0; j < imIn.getWidth(); j++){
            
            int R = imIn[i*3][j*3 + 0];
            int G = imIn[i*3][j*3 + 1];
            int B = imIn[i*3][j*3 + 2];


            int Yij = static_cast<int>(0.299 * R + 0.587 * G + 0.114 * B);
            int Crij = static_cast<int>(0.5 * R - 0.418688 * G - 0.081312 * B ) + 128;
            int Cbij = static_cast<int>(-0.168736 * R - 0.331264 * G + 0.5 * B ) + 128;

            Y[i][j] = Yij;
            Cb[i][j] = Cbij;
            Cr[i][j] = Crij;

        }
    }

}

//Sous échantillonage d'une image, faut trouver un meilleur algo
//imout doit être de largeur Imin.width / 2 et de hauteur Imin.height / 2
void down_sampling(ImageBase & imIn, ImageBase & imOut){

    std::vector<std::vector<int>> kernel = {
        {1, 1},
        {1, 1}
    };

    int kernelSum = 0;
    for (const auto& row : kernel) {
        for (int val : row) {
            kernelSum += val;
        }
    }

    int kernelHeight = kernel.size();
    int kernelWidth = kernel[0].size();

    for (int i = 0; i < imOut.getHeight(); i++) {
        for (int j = 0; j < imOut.getWidth(); j++) {
            int sum = 0;
            for (int ki = 0; ki < kernelHeight; ki++) {
                for (int kj = 0; kj < kernelWidth; kj++) {
                    sum += imIn[i * 2 + ki][j * 2 + kj] * kernel[ki][kj];
                }
            }
            imOut[i][j] = sum / kernelSum;
        }
    }


};

//Découpage en blocs de pixel



//peut mener a des erreurs d'acces memoire si la taille de l'image n'est pas un multiple de blockSize
std::vector<Block> getBlocks(ImageBase & imIn, int blockSize = 8){

    std::vector<Block> blocks;

    for(int i = 0; i < imIn.getHeight(); i+=blockSize){
        for(int j = 0; j < imIn.getWidth(); j+=blockSize){

            Block block(blockSize);

            for(int k = 0; k < blockSize; k++){
                for(int l = 0; l < blockSize; l++){
                    block.data[k][l] = imIn[i + k][j + l];
                }
            }

            blocks.push_back(block);
        }
    }

    return blocks;
}

//Transformée cosinus discrete dans le fichier Utils.h


//Quantification, matrice trouvé sur https://lehollandaisvolant.net/science/jpg/

std::vector<std::vector<int>> quantificationMatrix = {
    {5, 9, 13, 17, 21, 25, 29, 33},
    {9, 13, 17, 21, 25, 29, 33, 37},
    {13, 17, 21, 25, 29, 33, 37, 41},
    {17, 21, 25, 29, 33, 37, 41, 45},
    {21, 25, 29, 33, 37, 41, 45, 49},
    {25, 29, 33, 37, 41, 45, 49, 53},
    {29, 33, 37, 41, 45, 49, 53, 57},
    {33, 37, 41, 45, 49, 53, 57, 61}
};

void quantification (Block & block){
    for (int i = 0; i < block.dctMatrix.size(); i++){
        for (int j = 0; j < block.dctMatrix[0].size(); j++){

            block.dctMatrix[i][j] = block.dctMatrix[i][j] / quantificationMatrix[i][j];
        
        }
    }
}





//Codage RLE

//fonction qui regroupe tout 

void compression( char * cNomImgLue,  char * cNomImgOut){

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

    printf("number of blocks : %d\n", blocks.size());

    /* for (int i = 0; i < blocks.size(); i++){
        std::string filename = "./img/out/blocks/block" + std::to_string(i) + ".pgm";
        blocks[i].savePGM(filename.c_str());
    } */


    printf("  Fini\n");

    // Print block 0
    printf("Block 0:\n");
    for (int i = 0; i < blocks[0].data.size(); i++) {
        for (int j = 0; j < blocks[0].data[0].size(); j++) {
            //blocks[0].data[i][j] = 255;
            printf("%d ", blocks[0].data[i][j]);
            
        }
        printf("\n");
    }

    //DCT
    printf("DCT et quantification ");

    for(Block & block : blocks){// pour chaque bloc : on fait la DCT, on quantifie le résultat de la DCT et on applatit la matrice de DCT
        DCT(block);
        quantification(block);
        flattenZigZag(block);
    }

    // Print block flatDctMatrix
    printf("Block 0 flatDctMatrix:\n");
    for (int i = 0; i < blocks[0].flatDctMatrix.size(); i++) {
        printf("%d ", blocks[0].flatDctMatrix[i]);
    }
    printf("\n");


    printf("  Fini\n");

    printf("Codage RLE");

    std::vector<std::pair<int,int>> BlocksRLEEncoded;

    for(Block & block : blocks){// pour chaque bloc : on fait la DCT, on quantifie le résultat de la DCT et on applatit la matrice de DCT
        std::vector<std::pair<int,int>> RLEBlock;

        RLECompression(block.flatDctMatrix,RLEBlock);

        BlocksRLEEncoded.insert(BlocksRLEEncoded.end(), RLEBlock.begin(), RLEBlock.end());
    }

    // Print BlocksRLEEncoded
    printf(" BlocksRLEEncoded:\n");
    for (const auto& rleBlock : BlocksRLEEncoded) {
        printf("(%d, %d),", rleBlock.first, rleBlock.second);
        //printf("|");
    }


    printf("  Fini\n");

    printf("Huffman encoding ");

    std::vector<huffmanCodeSingle> codeTable;

    HuffmanEncoding(BlocksRLEEncoded, codeTable);

    for (const auto& code : codeTable) {
        cout << "RLE Pair: (" << code.rlePair.first << ", " << code.rlePair.second << "), Code: "; 
        for (int i = code.length - 1; i >= 0; --i) {
            cout << ((code.code >> i) & 1); //obligé de ce truc horrible pour print le code en binaire
        }
        cout << ", Length: " << code.length << endl;
    }

    unordered_map<pair<int, int>, huffmanCodeSingle, pair_hash> codeMap;
        for (const auto& code : codeTable) {
            codeMap[code.rlePair] = code;
        }

        printf("RLE Blocks encoded binary : \n");
        for (const auto& pair : BlocksRLEEncoded) {
            huffmanCodeSingle code = codeMap[pair];
            for (int i = code.length - 1; i >= 0; --i) {
                cout << ((code.code >> i) & 1);
            }
            cout << " ";
        }

        /* printf("\n");
        for (const auto& pair : BlocksRLEEncoded) {
            printf("(%d, %d) ", pair.first, pair.second);
        }
        printf("\n"); */

    printf("  Fini\n");

    std::string outFileName = cNomImgOut;
    writeHuffmanEncoded(BlocksRLEEncoded, codeTable, outFileName);


    //decompression ----------------------
    BlocksRLEEncoded.clear();
    codeTable.clear();
    readHuffmanEncoded(outFileName, codeTable, BlocksRLEEncoded );

    printf("Huffman decoded binary :      \n");
    for (const auto& pair : BlocksRLEEncoded) {
        huffmanCodeSingle code = codeMap[pair];
        for (int i = code.length - 1; i >= 0; --i) {
            cout << ((code.code >> i) & 1);
        }
        cout << " ";
    }

    /* printf("\n");
    for (const auto& pair : BlocksRLEEncoded) {
        printf("(%d, %d) ", pair.first, pair.second);
    }
    printf("\n");
    
    for (const auto& code : codeTable) {
        cout << "RLE Pair: (" << code.rlePair.first << ", " << code.rlePair.second << "), Code: "; 
        for (int i = code.length - 1; i >= 0; --i) {
            cout << ((code.code >> i) & 1); //obligé de ce truc horrible pour print le code en binaire
        }
        cout << ", Length: " << code.length << endl;
    } */

}

void decompression(const char * cNomImgIn, const char * cNomImgOut){
    

}