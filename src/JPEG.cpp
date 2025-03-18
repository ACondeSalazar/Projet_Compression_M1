#include "Utils.h"
#include "ImageBase.h"
#include <vector>
#include <fstream>

#include "RLE.h"
#include "Huffman.h"
#include <unordered_map>

#include "JPEG.h"

//pour debug 
std::vector<std::pair<int,int>> rlecompression;

std::vector<std::pair<int,int>> rledecompression;


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

void YCbCr_to_RGB(ImageBase & imY, ImageBase & imCb, ImageBase & imCr, ImageBase & imOut) {
    for (int i = 0; i < imY.getHeight(); i++) {
        for (int j = 0; j < imY.getWidth(); j++) {
            int Yij = imY[i][j];
            int Cbij = imCb[i][j] - 128;
            int Crij = imCr[i][j] - 128;

            int R = static_cast<int>(Yij + 1.402 * Crij);
            int G = static_cast<int>(Yij - 0.344136 * Cbij - 0.714136 * Crij);
            int B = static_cast<int>(Yij + 1.772 * Cbij);

            imOut[i * 3][j * 3 + 0] = std::clamp(R, 0, 255);
            imOut[i * 3][j * 3 + 1] = std::clamp(G, 0, 255);
            imOut[i * 3][j * 3 + 2] = std::clamp(B, 0, 255);
        }
    }
}

//Sous échantillonage d'une image, faut trouver un meilleur algo
//imout doit être de largeur Imin.width / 2 et de hauteur Imin.height / 2
void down_sampling(ImageBase & imIn, ImageBase & imOut){ // filtre moyenneur

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

//cette fonction fait un sous-échantillonnage avec interpolation bilinéaire
//Devrait etre meilleur que la fonction de base
void down_sampling_bilinear(ImageBase &imIn, ImageBase &imOut) {
    int newWidth = imOut.getWidth();
    int newHeight = imOut.getHeight();
    int oldWidth = imIn.getWidth();
    int oldHeight = imIn.getHeight();

    for (int i = 0; i < newHeight; i++) {
        for (int j = 0; j < newWidth; j++) {

            // Coordonnees image originale
            float x = j * (oldWidth - 1) / (float)(newWidth - 1);
            float y = i * (oldHeight - 1) / (float)(newHeight - 1);

            int x1 = (int) x;
            int y1 = (int) y;
            int x2 = std::min(x1 + 1, oldWidth - 1);
            int y2 = std::min(y1 + 1, oldHeight - 1);

            
            float dx = x - x1;
            float dy = y - y1;

            // Interpolation bilineaire
            float val = (1 - dx) * (1 - dy) * imIn[y1][x1] +
                        dx * (1 - dy) * imIn[y1][x2] +
                        (1 - dx) * dy * imIn[y2][x1] +
                        dx * dy * imIn[y2][x2];

            imOut[i][j] = (int) std::round(val);
        }
    }
}




void up_sampling(ImageBase & imIn, ImageBase & imOut){

    for (int i = 0; i < imOut.getHeight(); i++) {
        for (int j = 0; j < imOut.getWidth(); j++) {
            imOut[i][j] = imIn[i / 2][j / 2];
        }
    }

}

//Découpage en blocs de pixel



//peut mener a des erreurs d'acces memoire si la taille de l'image n'est pas un multiple de blockSize
//problème reglé cette fonction marche peu importe la taille
std::vector<Block> getBlocks(ImageBase & imIn, int blockSize) {
    std::vector<Block> blocks;
    int height = imIn.getHeight();
    int width = imIn.getWidth();


    for (int i = 0; i < height; i += blockSize) {
        for (int j = 0; j < width; j += blockSize) {
            Block block(blockSize);

            for (int k = 0; k < blockSize; k++) {
                for (int l = 0; l < blockSize; l++) {

                    int x = i + k;
                    int y = j + l;

                    if (x < height && y < width) {
                        block.data[k][l] = imIn[x][y];
                    } else {
                        block.data[k][l] = 0;  // Remplissage avec 0
                    }
                }
            }

            blocks.push_back(block);
            
        }
    }

    return blocks;
}



//reconstruction de l'image en niveau de gris à partir des blocs 
void reconstructImage(std::vector<Block> & blocks, ImageBase & imIn, int blocksize){

    int height = imIn.getHeight();
    int width = imIn.getWidth();
    float bls = 1.0/(float)blocksize;

    for(int i = 0; i < height; i += blocksize){
        for(int j = 0; j < width; j += blocksize){

            Block block = blocks[i * bls * (width * bls) + j * bls];

            for(int k = 0; k < blocksize; k++){
                for(int l = 0; l < blocksize; l++){
                    if (block.data[k][l] < 0) { //la valeur peut être négative on la seuille pour éviter des erreurs lors du cast en uchar dans l'image
                        block.data[k][l] = 0;
                        //std::cout << " aie "<< std::endl;
                    } else if (block.data[k][l] > 255) {
                        block.data[k][l] = 255;
                        //std::cout << "houla" << std::endl;
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
    {1, 3, 5, 9, 10, 12, 15, 17},
    {3, 5, 9, 10, 12, 15, 17, 18},
    {5, 9, 10, 12, 15, 17, 18, 20},
    {9, 10, 12, 15, 17, 18, 20, 22},
    {10, 12, 15, 17, 18, 20, 22, 24},
    {12, 15, 17, 18, 20, 22, 24, 26},
    {15, 17, 18, 20, 22, 24, 26, 28},
    {17, 18, 20, 22, 24, 26, 28, 30}
};

std::vector<std::vector<int>> quantificationMatrix3 = {
    {2, 3, 4, 6, 7, 9, 11, 13},
    {3, 4, 6, 7, 9, 11, 13, 14},
    {4, 6, 7, 9, 11, 13, 14, 16},
    {6, 7, 9, 11, 13, 14, 16, 18},
    {7, 9, 11, 13, 14, 16, 18, 20},
    {9, 11, 13, 14, 16, 18*2, 20*2, 22*2},
    {11, 13, 14, 16, 18, 20*2, 22*2, 24*2},
    {13, 14, 16, 18, 20, 22*2, 24*2, 26*2}
};


std::vector<std::vector<int>> quantificationMatrix_test = {
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 99, 99, 99, 99},
    {1, 1, 1, 1, 99, 99, 99, 99},
    {1, 1, 1, 1, 99, 99, 99, 99},
    {1, 1, 1, 1, 99, 99, 99, 99}
};

void quantification (Block & block){
    for (int i = 0; i < block.dctMatrix.size(); i++){
        for (int j = 0; j < block.dctMatrix[0].size(); j++){

            block.dctMatrix[i][j] = (int)(block.dctMatrix[i][j] / quantificationMatrix2[i][j]);
        
        }
    }
}

void inverse_quantification (Block & block){
    for (int i = 0; i < block.dctMatrix.size(); i++){
        for (int j = 0; j < block.dctMatrix[0].size(); j++){

            block.dctMatrix[i][j] = (int)(block.dctMatrix[i][j] * quantificationMatrix2[i][j]);
        
        }
    }
}



//Codage RLE

//fonction qui regroupe tout 

void compression( char * cNomImgLue,  char * cNomImgOut, ImageBase & imIn){

    printf("Opening image : %s\n", cNomImgLue);

    imIn.load(cNomImgLue);

    int width = imIn.getWidth();
    int height = imIn.getHeight();

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
    printf("Sous échantillonage de Cb et Cr \n");

    printf("Flou Gaussien sur Cr et Cb");
    ImageBase imCbFlou(imIn.getWidth(), imIn.getHeight(), false);
    ImageBase imCrFlou(imIn.getWidth(), imIn.getHeight(), false);

    gaussianBlur(imCb,imCbFlou); //fonction dans Utils.h
    gaussianBlur(imCr,imCrFlou);


    ImageBase downSampledCb(imIn.getWidth() / 2, imIn.getHeight() /2, false);
    ImageBase downSampledCr(imIn.getWidth() / 2, imIn.getHeight() /2, false);


    down_sampling_bilinear(imCbFlou, downSampledCb);
    down_sampling_bilinear(imCrFlou, downSampledCr);

    downSampledCb.save("./img/out/downSampledCb.pgm");
    downSampledCr.save("./img/out/downSampledCr.pgm");

    printf("  Fini\n");

    //Découpage en blocs de pixel
    printf("Découpage en blocs de pixel \n");
    std::vector<Block> blocksY = getBlocks(imY, 8);
    std::vector<Block> blocksCb = getBlocks(downSampledCb, 8);
    std::vector<Block> blocksCr = getBlocks(downSampledCr, 8);

    printf("number of blocks for Y channel: %d\n", blocksY.size());
    printf("number of blocks for Cb channel: %d\n", blocksCb.size());
    printf("number of blocks for Cr channel: %d\n", blocksCr.size());

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

    for(Block & block : blocksY){// pour chaque bloc : on fait la DCT, on quantifie le résultat de la DCT et on applatit la matrice de DCT
        DCT(block);
        quantification(block);
        flattenZigZag(block);
    }

    for(Block & block : blocksCb){
        DCT(block);
        quantification(block);
        flattenZigZag(block);
    }

    for(Block & block : blocksCr){
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

    printf("Codage RLE \n");

    std::vector<std::pair<int,int>> blocksYRLE; //les blocs applatis et encodés en RLE
    

    
    std::vector<std::pair<int,int>> blocksCbRLE;
    std::vector<std::pair<int,int>> blocksCrRLE;

    for(Block & block : blocksY){// pour chaque bloc : on fait la DCT, on quantifie le résultat de la DCT et on applatit la matrice de DCT
        std::vector<std::pair<int,int>> RLEBlock;

        RLECompression(block.flatDctMatrix,RLEBlock);

        blocksYRLE.insert(blocksYRLE.end(), RLEBlock.begin(), RLEBlock.end());
    }

    rlecompression = blocksYRLE;

    std::cout << "blocksY RLE size : " << blocksYRLE.size() << std::endl; 

    for (int i = std::max(0, static_cast<int>(blocksYRLE.size()) - 10); i < blocksYRLE.size(); ++i) {
        std::cout << "(" << blocksYRLE[i].first << ", " << blocksYRLE[i].second << ") ";
    }
    std::cout << std::endl;

    for(Block & block : blocksCb){
        std::vector<std::pair<int,int>> RLEBlock;

        RLECompression(block.flatDctMatrix,RLEBlock);

        blocksCbRLE.insert(blocksCbRLE.end(), RLEBlock.begin(), RLEBlock.end());
    }

    for(Block & block : blocksCr){
        std::vector<std::pair<int,int>> RLEBlock;

        RLECompression(block.flatDctMatrix,RLEBlock);

        blocksCrRLE.insert(blocksCrRLE.end(), RLEBlock.begin(), RLEBlock.end());
    }

    //std::cout<<"size blocksRLE "<<blocksYRLE.size()<<" "<<blocksCbRLE.size()<<" "<<blocksCrRLE.size()<<std::endl;


    std::vector<std::pair<int, int>> allBlocksRLE; //on fusionne les 3 canaux
    allBlocksRLE.insert(allBlocksRLE.end(), blocksYRLE.begin(), blocksYRLE.end());
    allBlocksRLE.insert(allBlocksRLE.end(), blocksCbRLE.begin(), blocksCbRLE.end());
    allBlocksRLE.insert(allBlocksRLE.end(), blocksCrRLE.begin(), blocksCrRLE.end());


    printf("  Fini\n");

    printf("Huffman encoding ");

    std::vector<huffmanCodeSingle> codeTable;

    HuffmanEncoding(allBlocksRLE, codeTable);

    printf("Code table size: %lu\n", codeTable.size());

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

    //on ecrit le fichier huffman encodé
    writeHuffmanEncoded(allBlocksRLE, codeTable,
                        imIn.getWidth(), imIn.getHeight(), downSampledCb.getWidth(), downSampledCb.getHeight() ,
                        blocksYRLE.size(), blocksCbRLE.size(),blocksCrRLE.size(),
                        outFileName);

}

void decompressBlocksRLE(const std::vector<std::pair<int,int>> & encodedRLE, std::vector<Block> & blocks){

    std::cout << " encodedRLE size " << encodedRLE.size() << std::endl;

    /* // Print the last 10 elements of the encoded RLE
    int startIdx = std::max(0, static_cast<int>(encodedRLE.size()) - 10);
    for (int i = startIdx; i < encodedRLE.size(); ++i) {
        std::cout << "(" << encodedRLE[i].first << ", " << encodedRLE[i].second << ") ";
    }
    std::cout << std::endl; */

    int currentRLEIndex = 0;
    int currentBlockProgress = 0;

    int nbBlockProgress = 0;
    
    while(currentRLEIndex < encodedRLE.size()){
        Block currentBlock(8);
        std::vector<std::pair<int,int>> currentBlockRLE;

        //tant que on a pas fini le bloc on lit des RLE
        while(currentBlockProgress < 64){

            

            currentBlockRLE.push_back(encodedRLE[currentRLEIndex]);
            
            
            //std::cout << " blocks (" << encodedRLE[currentRLEIndex].first << ", "<< encodedRLE[currentRLEIndex].second << ")" <<std::endl;
            currentBlockProgress += encodedRLE[currentRLEIndex].first;
            
            //std::cout << "block progress" << currentBlockProgress << std::endl;

            currentRLEIndex++;

            //if(currentRLEIndex >= encodedRLE.size()) {std::cout << "ERROR" << std::endl; return;}
            
        }

        nbBlockProgress++;

        if (nbBlockProgress > 206 && nbBlockProgress < 210) {
            std::cout << nbBlockProgress << " :\n";
            for (const auto& rle : currentBlockRLE) {
                std::cout << "(" << rle.first << ", " << rle.second << ") ";
            }
            std::cout << std::endl;
        }


        //on inverse les operations de la compression

  

        RLEDecompression(currentBlockRLE, currentBlock.flatDctMatrix);

        // Print flatDctMatrix
        /* std::cout << "Block flatDctMatrix:\n";
        for (const auto& val : currentBlock.flatDctMatrix) {
            std::cout << val << " ";
        }
        std::cout << "\n"; */

        unflattenZigZag(currentBlock);

        /* std::cout << "Block DCT matrix before:\n";
        for (const auto& row : currentBlock.dctMatrix) {
            for (const auto& val : row) {
                std::cout << val << " ";
            }
            std::cout << "\n";
        } */

        inverse_quantification(currentBlock);

        /* std::cout << "Block DCT matrix after:\n";
        for (const auto& row : currentBlock.dctMatrix) {
            for (const auto& val : row) {
                std::cout << val << " ";
            }
            std::cout << "\n";
        } */

        IDCT(currentBlock);

        // Print block data
        /* std::cout << "Block data:\n";
        for (const auto& row : currentBlock.data) {
            for (const auto& val : row) {
            std::cout << val << " ";
            }
            std::cout << "\n";
        }
         */

        //std::string blockname("./img/out/blocks/block_"+ std::to_string(nbBlockProgress) +".pgm");
        //currentBlock.savePGM(blockname.c_str());

        blocks.push_back(currentBlock);
        currentBlockProgress = 0;
    }
    

    std::cout << "Finale RLE index " << currentRLEIndex << std::endl;
    std::cout << "Nb Block Progress" << nbBlockProgress << std::endl;

}

void decompression(const char * cNomImgIn, const char * cNomImgOut, ImageBase * imOut){
    
    printf("Decompression\n");

    std::string outFileName = cNomImgIn;
    std::vector<huffmanCodeSingle> codeTable;
    std::vector<std::pair<int, int>> BlocksRLEEncoded;

    int imageWidth, imageHeight;
    int downSampledWidth, downSampledHeight;
    int channelYRLESize, channelCbRLESize, channelCrRLESize;



    printf("Reading huffman encoded file\n");

    readHuffmanEncoded(outFileName,
                        codeTable, BlocksRLEEncoded,
                        imageWidth, imageHeight, downSampledWidth, downSampledHeight,
                        channelYRLESize, channelCbRLESize, channelCrRLESize);

    std::cout<<"size downSampledWidth "<<downSampledWidth<<" "<<downSampledHeight<<std::endl;

    printf("Code table size: %lu\n", codeTable.size());

    std::vector<std::pair<int,int>> blocksYRLE; //les blocs applatis et encodés en RLE
    std::vector<std::pair<int,int>> blocksCbRLE;
    std::vector<std::pair<int,int>> blocksCrRLE;

    //on sépare les 3 canaux
    for (int i = 0; i < channelYRLESize; i++) {
        blocksYRLE.push_back(BlocksRLEEncoded[i]);
    }

    rledecompression = blocksYRLE;

    for (int i = channelYRLESize; i < channelYRLESize + channelCbRLESize; i++) {
        blocksCbRLE.push_back(BlocksRLEEncoded[i]);
    }

    for (int i = channelYRLESize + channelCbRLESize; i < channelYRLESize + channelCbRLESize + channelCrRLESize; i++) {
        blocksCrRLE.push_back(BlocksRLEEncoded[i]);
    }

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

    
    std::vector<Block> blocksY;
    std::vector<Block> blocksCb;
    std::vector<Block> blocksCr;

    decompressBlocksRLE(blocksCbRLE, blocksCb);
    printf("blocksCb size: %lu\n", blocksCb.size());

    decompressBlocksRLE(blocksCrRLE, blocksCr);
    printf("blocksCr size: %lu\n", blocksCr.size());
   
    decompressBlocksRLE(blocksYRLE, blocksY);
    printf("blocksY size: %lu\n", blocksY.size()); 

    /* if (rlecompression.size() != rledecompression.size()) {
        std::cout << "The sizes of rlecompression and rledecompression are not equal." << std::endl;
    } else {
        bool areEqual = true;
        for (size_t i = 0; i < rlecompression.size(); ++i) {
            if (rlecompression[i] != rledecompression[i]) {
                std::cout << "Difference found at index " << i << ": "
                          << "rlecompression = (" << rlecompression[i].first << ", " << rlecompression[i].second << "), "
                          << "rledecompression = (" << rledecompression[i].first << ", " << rledecompression[i].second << ")" << std::endl;
                areEqual = false;
            }
        }
        if (areEqual) {
            std::cout << "All elements of rlecompression and rledecompression are equal." << std::endl;
        }
    }

    int sumFirstElements = 0;
    for (const auto& pair : rledecompression) {
        sumFirstElements += pair.first;
    }

    if (sumFirstElements == 64 * 256) {
        std::cout << "The sum of the first elements of all RLE pairs is equal to 64 * 256." << std::endl;
    } else {
        std::cout << "The sum of the first elements of all RLE pairs is not equal to 64 * 256. : " << sumFirstElements << std::endl;
    } */

    printf("Reconstructing image from blocks\n");


    //ImageBase imOut(512, 480, false);

    imOut = new ImageBase(imageWidth, imageHeight, true);

    ImageBase imY(imageWidth, imageHeight, false);

    ImageBase imCb(downSampledWidth, downSampledHeight, false);
    ImageBase imCr(downSampledWidth, downSampledHeight, false);

    ImageBase upSampledCb(imageWidth, imageHeight, false);
    ImageBase upSampledCr(imageWidth, imageHeight, false);

    printf("Reconstructing Cb channel\n");
    reconstructImage(blocksCb, imCb,8);
    up_sampling(imCb, upSampledCb);
    upSampledCb.save("./img/out/Cb_decompressed.pgm");

    printf("Reconstructing Cr channel\n");
    reconstructImage(blocksCr, imCr,8);
    up_sampling(imCr, upSampledCr);
    upSampledCr.save("./img/out/Cr_decompressed.pgm");

    

    printf("Reconstructing Y channel\n");
    reconstructImage(blocksY, imY,8);
    printf("saving Y channel\n");
    imY.save("./img/out/Y_decompressed.pgm");

    



    printf("Reconstructing image from YCbCr\n");
    YCbCr_to_RGB(imY, upSampledCb, upSampledCr, (*imOut));

    printf("Saving decompressed image\n");
    std::string cNomImgOutStr = cNomImgOut;
    (*imOut).save(cNomImgOutStr.data());


}
