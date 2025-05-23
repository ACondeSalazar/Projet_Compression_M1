#include "JPEG.h"
#include "ImageBase.h"
#include <thread>
#include <vector>

#include "Utils.h"
#include "FormatSamplingBlur.h"
#include "TransformationQuantification.h"
#include "RLE.h"
#include "Huffman.h"

#include <iostream>


#include <threads.h>

//pour debug 
std::vector<std::pair<int,int>> rlecompression;

std::vector<std::pair<int,int>> rledecompression;




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







//Codage RLE

//fonction qui regroupe tout 

void compression( char * cNomImgLue,  char * cNomImgOut, ImageBase & imIn, CompressionSettings & settings){

    std::vector<std::thread> threads;

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

    printf("  Fini\n");

    printf("DCT et quantification ");
    // pour chaque bloc : on fait la DCT, on quantifie le résultat de la DCT et on applatit la matrice de DCT

    //On peut surement utiliser encore plus de threads 

    threads.emplace_back([&blocksY] {
        for(Block & block : blocksY){
            DCT(block);
            quantification(block);
            flattenZigZag(block);
        }
    });

    threads.emplace_back([&blocksCb] {
        for(Block & block : blocksCb){
            DCT(block);
            quantification(block);
            flattenZigZag(block);
        }
    });
    
    threads.emplace_back([&blocksCr] {
        for(Block & block : blocksCr){
            DCT(block);
            quantification(block);
            flattenZigZag(block);
        }
    });
    
    for (auto &thread : threads) {
        thread.join();
    }
    
    threads.clear();

    printf("  Fini\n");


    printf("Codage RLE \n");

    std::vector<std::pair<int,int>> blocksYRLE; //les blocs applatis et encodés en RLE
    std::vector<std::pair<int,int>> blocksCbRLE;
    std::vector<std::pair<int,int>> blocksCrRLE;

    // pour chaque bloc : on compresse en RLE la matrice applati

    threads.emplace_back([&blocksY, &blocksYRLE] {
        for(Block & block : blocksY){
            std::vector<std::pair<int,int>> RLEBlock;
    
            RLECompression(block.flatDctMatrix,RLEBlock);
    
            blocksYRLE.insert(blocksYRLE.end(), RLEBlock.begin(), RLEBlock.end());
        }
    });

    threads.emplace_back([&blocksCb, &blocksCbRLE] {
        for(Block & block : blocksCb){
            std::vector<std::pair<int,int>> RLEBlock;

            RLECompression(block.flatDctMatrix,RLEBlock);

            blocksCbRLE.insert(blocksCbRLE.end(), RLEBlock.begin(), RLEBlock.end());
        }
    });

    threads.emplace_back([&blocksCr, &blocksCrRLE] {
        for(Block & block : blocksCr){
            std::vector<std::pair<int,int>> RLEBlock;

            RLECompression(block.flatDctMatrix,RLEBlock);

            blocksCrRLE.insert(blocksCrRLE.end(), RLEBlock.begin(), RLEBlock.end());
        }
    });

    for (auto &thread : threads) {
        thread.join();
    }

    std::cout<<"size blocksRLE "<<blocksYRLE.size()<<" "<<blocksCbRLE.size()<<" "<<blocksCrRLE.size()<<std::endl;


    std::vector<std::pair<int, int>> allBlocksRLE; //on fusionne les 3 canaux
    allBlocksRLE.insert(allBlocksRLE.end(), blocksYRLE.begin(), blocksYRLE.end());
    allBlocksRLE.insert(allBlocksRLE.end(), blocksCbRLE.begin(), blocksCbRLE.end());
    allBlocksRLE.insert(allBlocksRLE.end(), blocksCrRLE.begin(), blocksCrRLE.end());


    printf("  Fini\n");

    printf("Huffman encoding ");

    std::vector<huffmanCodeSingle> codeTable;

    //on cree la table de codage
    HuffmanEncoding(allBlocksRLE, codeTable);

    printf("Code table size: %lu\n", codeTable.size());

    printf("  Fini\n");

    std::string outFileName = cNomImgOut;

    //on ecrit le fichier huffman encodé
    writeHuffmanEncoded(allBlocksRLE, codeTable,
                        imIn.getWidth(), imIn.getHeight(), downSampledCb.getWidth(), downSampledCb.getHeight() ,
                        blocksYRLE.size(), blocksCbRLE.size(),blocksCrRLE.size(),
                        outFileName, settings);

}




void decompression(const char * cNomImgIn, const char * cNomImgOut, ImageBase * imOut, CompressionSettings & settings){
    
    printf("Decompression\n");

    std::string outFileName = cNomImgIn;
    std::vector<huffmanCodeSingle> codeTable;
    std::vector<std::pair<int, int>> BlocksRLEEncoded;

    int imageWidth, imageHeight;
    int downSampledWidth, downSampledHeight;
    int channelYRLESize, channelCbRLESize, channelCrRLESize;

    std::vector<std::thread> threads;

    int maxThreads = std::thread::hardware_concurrency();

    printf("Reading huffman encoded file\n");

    readHuffmanEncoded(outFileName,
                        codeTable, BlocksRLEEncoded,
                        imageWidth, imageHeight, downSampledWidth, downSampledHeight,
                        channelYRLESize, channelCbRLESize, channelCrRLESize, settings);

    std::cout<<"size downSampledWidth "<<downSampledWidth<<" "<<downSampledHeight<<std::endl;

    printf("Code table size: %lu\n", codeTable.size());

    std::vector<std::pair<int,int>> blocksYRLE; //les blocs applatis et encodés en RLE
    std::vector<std::pair<int,int>> blocksCbRLE;
    std::vector<std::pair<int,int>> blocksCrRLE;

    std::vector<Block> blocksY;
    std::vector<Block> blocksCb;
    std::vector<Block> blocksCr;

    imOut = new ImageBase(imageWidth, imageHeight, true);

    ImageBase imY(imageWidth, imageHeight, false);

    ImageBase imCb(downSampledWidth, downSampledHeight, false);
    ImageBase imCr(downSampledWidth, downSampledHeight, false);

    ImageBase upSampledCb(imageWidth, imageHeight, false);
    ImageBase upSampledCr(imageWidth, imageHeight, false);

    //on sépare les 3 canaux
    threads.emplace_back([&BlocksRLEEncoded, &blocksYRLE, channelYRLESize, &blocksY, &imY] {
        for (int i = 0; i < channelYRLESize; i++) {
            blocksYRLE.push_back(BlocksRLEEncoded[i]);
        }

        decompressBlocksRLE(blocksYRLE, blocksY, quantificationLuminance, nullptr);
        printf("blocksY size: %lu\n", blocksY.size());

        printf("Reconstructing Y channel\n");
        reconstructImage(blocksY, imY, 8);
        printf("saving Y channel\n");
        imY.save("./img/out/Y_decompressed.pgm");
    });

    threads.emplace_back([&BlocksRLEEncoded, &blocksCbRLE, channelYRLESize, channelCbRLESize, &blocksCb, &imCb, &upSampledCb] {
        for (int i = channelYRLESize; i < channelYRLESize + channelCbRLESize; i++) {
            blocksCbRLE.push_back(BlocksRLEEncoded[i]);
        }

        decompressBlocksRLE(blocksCbRLE, blocksCb, quantificationChrominance, nullptr);
        printf("blocksCb size: %lu\n", blocksCb.size());

        printf("Reconstructing Cb channel\n");
        reconstructImage(blocksCb, imCb, 8);
        up_sampling(imCb, upSampledCb);
        upSampledCb.save("./img/out/Cb_decompressed.pgm");
    });

    threads.emplace_back([&BlocksRLEEncoded, &blocksCrRLE, channelYRLESize, channelCbRLESize, channelCrRLESize, &blocksCr, &imCr, &upSampledCr] {
        for (int i = channelYRLESize + channelCbRLESize; i < channelYRLESize + channelCbRLESize + channelCrRLESize; i++) {
            blocksCrRLE.push_back(BlocksRLEEncoded[i]);
        }

        decompressBlocksRLE(blocksCrRLE, blocksCr, quantificationChrominance, nullptr);
        printf("blocksCr size: %lu\n", blocksCr.size());

        printf("Reconstructing Cr channel\n");
        reconstructImage(blocksCr, imCr, 8);
        up_sampling(imCr, upSampledCr);
        upSampledCr.save("./img/out/Cr_decompressed.pgm");
    });

    for (auto &thread : threads) {
        thread.join();
    }
    threads.clear();

    printf("Blocks decoding\n");

    printf("Reconstructing image from blocks\n");


    printf("Reconstructing image from YCbCr\n");
    YCbCr_to_RGB(imY, upSampledCb, upSampledCr, (*imOut));

    printf("Saving decompressed image\n");
    std::string cNomImgOutStr = cNomImgOut;
    (*imOut).save(cNomImgOutStr.data());


}
