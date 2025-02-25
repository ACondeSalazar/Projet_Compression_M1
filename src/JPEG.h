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


//reconstruction de l'image à partir des blocs
void reconstructImage(std::vector<Block> & blocks, ImageBase & imIn, int blocksize = 8){

    for(int i = 0; i < imIn.getHeight(); i+=blocksize){
        for(int j = 0; j < imIn.getWidth(); j+=blocksize){

            Block block = blocks[i/blocksize * (imIn.getWidth() / blocksize) + j/blocksize];

            for(int k = 0; k < blocksize; k++){
                for(int l = 0; l < blocksize; l++){
                    if (block.data[k][l] < 0) { //la valeur peut être négative on la seuille pour éviter des erreurs lors du cast en uchar dans l'image
                        block.data[k][l] = 0;
                    } else if (block.data[k][l] > 255) {
                        //n'est pas censé arriver
                        block.data[k][l] = 255;
                    }
                    imIn[i + k][j + l] = block.data[k][l];
                }
            }

        }
    }

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

std::vector<std::vector<int>> quantificationMatrix2 = {
    {3, 5, 7, 9, 10, 12, 15, 17},
    {5, 7, 9, 10, 12, 15, 17, 18},
    {7, 9, 10, 12, 15, 17, 18, 20},
    {9, 10, 12, 15, 17, 18, 20, 22},
    {10, 12, 15, 17, 18, 20, 22, 24},
    {12, 15, 17, 18, 20, 22, 24, 26},
    {15, 17, 18, 20, 22, 24, 26, 28},
    {17, 18, 20, 22, 24, 26, 28, 30}
};

void quantification (Block & block){
    for (int i = 0; i < block.dctMatrix.size(); i++){
        for (int j = 0; j < block.dctMatrix[0].size(); j++){

            block.dctMatrix[i][j] = block.dctMatrix[i][j] / quantificationMatrix[i][j];
        
        }
    }
}

void inverse_quantification (Block & block){
    for (int i = 0; i < block.dctMatrix.size(); i++){
        for (int j = 0; j < block.dctMatrix[0].size(); j++){

            block.dctMatrix[i][j] = block.dctMatrix[i][j] * quantificationMatrix[i][j];
        
        }
    }
}





//Codage RLE

//fonction qui regroupe tout 

void compression( char * cNomImgLue,  char * cNomImgOut, ImageBase & imIn){

    printf("Opening image : %s\n", cNomImgLue);

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
    /* printf("Block 0:\n");
    for (int i = 0; i < blocks[0].data.size(); i++) {
        for (int j = 0; j < blocks[0].data[0].size(); j++) {
            //blocks[0].data[i][j] = 255;
            printf("%d ", blocks[0].data[i][j]);
            
        }
        printf("\n");
    } */

    //DCT
    printf("DCT et quantification ");

    for(Block & block : blocks){// pour chaque bloc : on fait la DCT, on quantifie le résultat de la DCT et on applatit la matrice de DCT
        DCT(block);
        quantification(block);
        flattenZigZag(block);
    }

    // Print block flatDctMatrix
    /* printf("Block 0 flatDctMatrix:\n");
    for (int i = 0; i < blocks[0].flatDctMatrix.size(); i++) {
        printf("%d ", blocks[0].flatDctMatrix[i]);
    }
    printf("\n"); */


    printf("  Fini\n");

    printf("Codage RLE");

    std::vector<std::pair<int,int>> BlocksRLEEncoded;

    for(Block & block : blocks){// pour chaque bloc : on fait la DCT, on quantifie le résultat de la DCT et on applatit la matrice de DCT
        std::vector<std::pair<int,int>> RLEBlock;

        RLECompression(block.flatDctMatrix,RLEBlock);

        BlocksRLEEncoded.insert(BlocksRLEEncoded.end(), RLEBlock.begin(), RLEBlock.end());
    }

    // Print BlocksRLEEncoded
    /* printf(" BlocksRLEEncoded:\n");
    for (const auto& rleBlock : BlocksRLEEncoded) {
        printf("(%d, %d),", rleBlock.first, rleBlock.second);
        //printf("|");
    } */


    printf("  Fini\n");

    printf("Huffman encoding ");

    std::vector<huffmanCodeSingle> codeTable;

    HuffmanEncoding(BlocksRLEEncoded, codeTable);

    /* for (const auto& code : codeTable) {
        cout << "RLE Pair: (" << code.rlePair.first << ", " << code.rlePair.second << "), Code: "; 
        for (int i = code.length - 1; i >= 0; --i) {
            cout << ((code.code >> i) & 1); //obligé de ce truc horrible pour print le code en binaire
        }
        cout << ", Length: " << code.length << endl;
    } */

    /* unordered_map<pair<int, int>, huffmanCodeSingle, pair_hash> codeMap;
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
        } */

        /* printf("\n");
        for (const auto& pair : BlocksRLEEncoded) {
            printf("(%d, %d) ", pair.first, pair.second);
        }
        printf("\n"); */

    printf("  Fini\n");

    std::string outFileName = cNomImgOut;
    writeHuffmanEncoded(BlocksRLEEncoded, codeTable,imIn.getWidth(), imIn.getHeight(),outFileName);


    //decompression ----------------------
    //BlocksRLEEncoded.clear();
    //codeTable.clear();
    

    

}

void decompression(const char * cNomImgIn, const char * cNomImgOut, ImageBase & imOut){
    
    printf("Decompression\n");

    std::string outFileName = cNomImgIn;
    std::vector<huffmanCodeSingle> codeTable;
    std::vector<std::pair<int, int>> BlocksRLEEncoded;

    int imageWidth, imageHeight;

    printf("Reading huffman encoded file\n");

    readHuffmanEncoded(outFileName, codeTable, BlocksRLEEncoded, imageWidth, imageHeight);

    /* unordered_map<pair<int, int>, huffmanCodeSingle, pair_hash> codeMap;
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
        } */

    printf("Blocks decoding\n");

    int currentRLEIndex = 0;
    int currentBlockProgress = 0;
    std::vector<Block> blocks;

    
    while(currentRLEIndex < BlocksRLEEncoded.size()){
        Block currentBlock(8);
        std::vector<std::pair<int,int>> currentBlockRLE;

        //tant que on a pas fini le bloc on lit des RLE
        while(currentBlockProgress < 64){
            currentBlockRLE.push_back(BlocksRLEEncoded[currentRLEIndex]);
            currentBlockProgress += BlocksRLEEncoded[currentRLEIndex].first;
            currentRLEIndex++;
        }
 

        //on inverse les operations de la compression

        RLEDecompression(currentBlockRLE, currentBlock.flatDctMatrix);

        unflattenZigZag(currentBlock);

        inverse_quantification(currentBlock);

        IDCT(currentBlock);

        blocks.push_back(currentBlock);
        currentBlockProgress = 0;
    }

    printf("Reconstructing image from blocks\n");


    //ImageBase imOut(512, 480, false);

    imOut = ImageBase(imageWidth, imageHeight, false);

    reconstructImage(blocks, imOut);

    imOut.save("./decompressed.ppm");

}
