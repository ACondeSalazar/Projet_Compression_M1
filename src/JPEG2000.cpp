#pragma once
#include "JPEG.h"
#include "Utils.h"
#include "ImageBase.h"
#include <vector>
#include <fstream>

#include "RLE.h"
#include "Huffman.h"
#include <unordered_map>

#include "JPEG2000.h"


/*
Avec JPEG2000 on ne fait pas des blocks de 8 par 8 mais des Tiles
Ici on va faire des Tiles de taille 128*128 ou 256*256
*/

std::vector<Tile> getTiles(ImageBase &imIn, int tileSize) {
    std::vector<Tile> tiles;
    int height = imIn.getHeight();
    int width = imIn.getWidth();

    // Le nombre de tiles dans chaque direction
    int numTilesX = (width + tileSize - 1) / tileSize;  // pour arrondir vers le haut
    int numTilesY = (height + tileSize - 1) / tileSize;  // pour arrondir vers le haut

    // Remplir le vecteur de tiles
    for (int i = 0; i < numTilesY; i++) {
        for (int j = 0; j < numTilesX; j++) {
            // Calculer la largeur et la hauteur du tile en fonction de la position
            int tileWidth = std::min(tileSize, width - j * tileSize);
            int tileHeight = std::min(tileSize, height - i * tileSize);

            // Créer un tile
            Tile tile(tileWidth, tileHeight, j * tileSize, i * tileSize);

            // Remplir le tile avec les données de l'image
            for (int k = 0; k < tileHeight; k++) {
                for (int l = 0; l < tileWidth; l++) {
                    tile.data[k][l] = imIn[i * tileSize + k][j * tileSize + l];
                }
            }

            // Ajouter le tile au vecteur
            tiles.push_back(tile);
        }
    }

    return tiles;
}

//==========================================================================================================================
//wavelet transform CDF97 

void applyCDF97(std::vector<std::vector<int>>& data) {
    int height = data.size();
    int width = data[0].size();

    // Application de la transformation CDF 9/7 sur les lignes
    for (int i = 0; i < height; ++i) {

        for (int j = 1; j < width - 1; ++j) {
            int temp1 = data[i][j];
            int temp2 = data[i][j + 1];

            data[i][j] = (data[i][j] + data[i][j - 1]) / 2;
            data[i][j + 1] = temp2 - data[i][j + 1];
        }
    }

    // meme chose sur les colones
    for (int j = 0; j < width; ++j) {
        
        for (int i = 1; i < height - 1; ++i) {
            int temp1 = data[i][j];
            int temp2 = data[i + 1][j];

            
            data[i][j] = (data[i][j] + data[i - 1][j]) / 2;
            data[i + 1][j] = temp2 - data[i + 1][j];
        }
    }
}

void applyWaveletTransformToTiles(std::vector<Tile>& tiles) {
    for (auto& tile : tiles) {
        // Appliquer la transformation CDF 9/7 à chaque tile
        applyCDF97(tile.data);
    }
}
//==========================================================================================================================
//quantification
std::vector<std::vector<int>> quantizationMatrix = {
    {1, 1, 2, 2, 2, 2, 4, 4},
    {1, 1, 2, 2, 2, 4, 4, 4},
    {2, 2, 2, 4, 4, 4, 6, 6},
    {2, 2, 4, 4, 6, 6, 8, 8},
    {2, 2, 4, 6, 8, 8, 8, 8},
    {4, 4, 4, 6, 8, 8, 8, 8},
    {4, 4, 6, 8, 8, 8, 8, 8},
    {4, 4, 6, 8, 8, 8, 8, 8}
};

void quantification(Tile & tile, const std::vector<std::vector<int>>& quantizationMatrix) {
    int height = tile.data.size();
    int width = tile.data[0].size();

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            // Appliquer la quantification : diviser le coefficient par la valeur de la matrice de quantification
            tile.data[i][j] = std::round(tile.data[i][j] / static_cast<double>(quantizationMatrix[i][j]));
        }
    }
}

void quantificationForAllTiles(std::vector<Tile> & tiles, const std::vector<std::vector<int>>& quantizationMatrix) {
    for (auto& tile : tiles) {
        quantification(tile, quantizationMatrix);  // Appliquer la quantification à chaque tile
    }
}



//==========================================================================================================================
//bout de code pratique

std::vector<int> getFlatTile(Tile & tile) {
    std::vector<int> res;
    int width = tile.width;
    int height = tile.height;
    for (int i = 0; i < height; i++) { // Parcourir les lignes (height)
        for (int j = 0; j < width; j++) { // Parcourir les colonnes (width)
            res.push_back(tile.data[i][j]);
        }
    }
    return res;
}

//==========================================================================================================================

void decompressTilesRLE(const std::vector<std::pair<int, int>>& tilesYRLE, std::vector<Tile>& tilesY, int tileWidth, int tileHeight) {
    int currentX = 0;
    int currentY = 0;

    for (const auto& rlePair : tilesYRLE) {
        int value = rlePair.first;
        int length = rlePair.second;

        for (int i = 0; i < length; ++i) {
            // Créer une tuile et la remplir avec la valeur
            Tile tile(tileWidth, tileHeight, currentX, currentY);
            for (int row = 0; row < tileHeight; ++row) {
                for (int col = 0; col < tileWidth; ++col) {
                    tile.data[row][col] = value;
                }
            }

            // Ajouter la tuile au vecteur
            tilesY.push_back(tile);

            // Mettre à jour la position
            currentX += tileWidth;
            if (currentX >= 512) { // Supposons une image de 512x512
                currentX = 0;
                currentY += tileHeight;
            }
        }
    }
}

//==========================================================================================================================

void reconstructImage(std::vector<Tile> & tiles, ImageBase & imIn, int blocksize) {
    int height = imIn.getHeight();
    int width = imIn.getWidth();
    float bls = 1.0 / (float)blocksize;

    for (int i = 0; i < height; i += blocksize) {
        for (int j = 0; j < width; j += blocksize) {
            // Calculer l'indice du tile à partir des coordonnées
            int tileIndex = (i / blocksize) * (width / blocksize) + (j / blocksize);
            Tile& tile = tiles[tileIndex];

            for (int k = 0; k < blocksize; k++) {
                for (int l = 0; l < blocksize; l++) {
                    if (tile.data[k][l] < 0) { // La valeur peut être négative, on la seuille pour éviter des erreurs lors du cast en uchar dans l'image
                        tile.data[k][l] = 0;
                        //std::cout << " aie "<< std::endl;
                    } else if (tile.data[k][l] > 255) {
                        tile.data[k][l] = 255;
                        //std::cout << "houla" << std::endl;
                    }
                    imIn[i + k][j + l] = tile.data[k][l];
                }
            }
        }
    }
}


//==========================================================================================================================
//==========================================================================================================================
//fonction a adapter

void compression2000( char * cNomImgLue,  char * cNomImgOut, ImageBase & imIn){

    printf("Compression JPEG 2000: Opening image : %s\n", cNomImgLue);

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

    // Jusqu'ici rien n'a changé ======================================

    //Découpage en tiles de tiles
    printf("Découpage en tiles de pixel \n");

    std::vector<Tile> tilesY = getTiles(imY, 8); // ici ca devrait etre plus de 8 genre 128 ou plus mais ca plante (problème dans getTiles)
    std::vector<Tile> tilesCb = getTiles(downSampledCb, 8);
    std::vector<Tile> tilesCr = getTiles(downSampledCr, 8);

    printf("number of tiles for Y channel: %d\n", tilesY.size());
    printf("number of tiles for Cb channel: %d\n", tilesCb.size());
    printf("number of tiles for Cr channel: %d\n", tilesCr.size());

    

    printf("Wavelet transform et quantification "); // wavelet

    
    applyWaveletTransformToTiles(tilesY);

    applyWaveletTransformToTiles(tilesCr);

    applyWaveletTransformToTiles(tilesCb);
    

    quantificationForAllTiles(tilesY,quantizationMatrix);
    quantificationForAllTiles(tilesCr,quantizationMatrix);
    quantificationForAllTiles(tilesCb,quantizationMatrix);

    //Plus que RLE et Huffmann

    printf("  Fini\n");

    printf("Codage RLE \n");

    std::vector<std::pair<int,int>> tilesYRLE; //les blocs applatis et encodés en RLE
    std::vector<std::pair<int,int>> tilesCbRLE;
    std::vector<std::pair<int,int>> tilesCrRLE;

    for(Tile & tile : tilesY){
        std::vector<std::pair<int,int>> RLETile;

        std::vector<int> flatTile = getFlatTile(tile);
        RLECompression(flatTile,RLETile);

        tilesYRLE.insert(tilesYRLE.end(),RLETile.begin(),RLETile.end());
    }

    int totalLengthCb = 0;
    for (const auto& pair : tilesYRLE) {
        totalLengthCb += pair.second;
    }
    std::cout<<"ici trouduc"<<totalLengthCb<<std::endl;

    for(Tile & tile : tilesCb){
        std::vector<std::pair<int,int>> RLETile;

        std::vector<int> flatTile = getFlatTile(tile);
        RLECompression(flatTile,RLETile);

        tilesCbRLE.insert(tilesCbRLE.end(),RLETile.begin(),RLETile.end());
    }

    for(Tile & tile : tilesCr){
        std::vector<std::pair<int,int>> RLETile;

        std::vector<int> flatTile = getFlatTile(tile);
        RLECompression(flatTile,RLETile);

        tilesCrRLE.insert(tilesCrRLE.end(),RLETile.begin(),RLETile.end());
    }

    std::vector<std::pair<int, int>> allTilesRLE; //on fusionne les 3 canaux
    allTilesRLE.insert(allTilesRLE.end(), tilesYRLE.begin(), tilesYRLE.end());
    allTilesRLE.insert(allTilesRLE.end(), tilesCbRLE.begin(), tilesCbRLE.end());
    allTilesRLE.insert(allTilesRLE.end(), tilesCrRLE.begin(), tilesCrRLE.end());

    //plus que Huffman

    printf("  Fini\n");

    printf("Huffman encoding ");

    std::vector<huffmanCodeSingle> codeTable;

    HuffmanEncoding(allTilesRLE, codeTable);

    printf("Code table size: %lu\n", codeTable.size());



    printf("  Fini\n");

    std::string outFileName = cNomImgOut;

    //on ecrit le fichier huffman encodé
    writeHuffmanEncoded(allTilesRLE, codeTable,
                        imIn.getWidth(), imIn.getHeight(), downSampledCb.getWidth(), downSampledCb.getHeight() ,
                        tilesYRLE.size(), tilesCbRLE.size(),tilesCrRLE.size(),
                        outFileName);

}


//==========================================================================================================================
//==========================================================================================================================


void decompression2000(const char * cNomImgIn, const char * cNomImgOut, ImageBase * imOut){
    
    printf("Decompression 2000\n");

    std::string outFileName = cNomImgIn;
    std::vector<huffmanCodeSingle> codeTable;
    std::vector<std::pair<int, int>> TilesRLEEncoded;

    int imageWidth, imageHeight;
    int downSampledWidth, downSampledHeight;
    int channelYRLESize, channelCbRLESize, channelCrRLESize;



    printf("Reading huffman encoded file\n");

    readHuffmanEncoded(outFileName,
                        codeTable, TilesRLEEncoded,
                        imageWidth, imageHeight, downSampledWidth, downSampledHeight,
                        channelYRLESize, channelCbRLESize, channelCrRLESize);

    std::cout<<"size downSampledWidth "<<downSampledWidth<<" "<<downSampledHeight<<std::endl;

    printf("Code table size: %lu\n", codeTable.size());

    std::vector<std::pair<int,int>> tilesYRLE; //les blocs applatis et encodés en RLE
    std::vector<std::pair<int,int>> tilesCbRLE;
    std::vector<std::pair<int,int>> tilesCrRLE;

    //on sépare les 3 canaux
    for (int i = 0; i < channelYRLESize; i++) {
        tilesYRLE.push_back(TilesRLEEncoded[i]);
    }




    for (int i = channelYRLESize; i < channelYRLESize + channelCbRLESize; i++) {
        tilesCbRLE.push_back(TilesRLEEncoded[i]);
    }

    for (int i = channelYRLESize + channelCbRLESize; i < channelYRLESize + channelCbRLESize + channelCrRLESize; i++) {
        tilesCrRLE.push_back(TilesRLEEncoded[i]);
    }

    printf("Blocks decoding\n");

    
    std::vector<Tile> tilesY;
    std::vector<Tile> tilesCb;
    std::vector<Tile> tilesCr;

    printf("tilesCbRLE size: %lu\n", tilesCbRLE.size());
    decompressTilesRLE(tilesCbRLE, tilesCb,8,8);
    printf("tilesCb size: %lu\n", tilesCb.size());

    decompressTilesRLE(tilesCrRLE, tilesCr,8,8);
    printf("tilesCr size: %lu\n", tilesCr.size());
   
    decompressTilesRLE(tilesYRLE, tilesY,8,8);
    printf("tilesY size: %lu\n", tilesY.size()); 



    printf("Reconstructing image from tiles\n");


    //ImageBase imOut(512, 480, false);

    imOut = new ImageBase(imageWidth, imageHeight, true);

    ImageBase imY(imageWidth, imageHeight, false);

    ImageBase imCb(downSampledWidth, downSampledHeight, false);
    ImageBase imCr(downSampledWidth, downSampledHeight, false);

    ImageBase upSampledCb(imageWidth, imageHeight, false);
    ImageBase upSampledCr(imageWidth, imageHeight, false);

    printf("Reconstructing Cb channel\n");
    reconstructImage(tilesCb, imCb,8);
    up_sampling(imCb, upSampledCb);
    upSampledCb.save("./img/out/Cb_decompressed.pgm");

    printf("Reconstructing Cr channel\n");
    reconstructImage(tilesCr, imCr,8);
    up_sampling(imCr, upSampledCr);
    upSampledCr.save("./img/out/Cr_decompressed.pgm");

    

    printf("Reconstructing Y channel\n");
    reconstructImage(tilesY, imY,8);
    printf("saving Y channel\n");
    imY.save("./img/out/Y_decompressed.pgm");

    



    printf("Reconstructing image from YCbCr\n");
    YCbCr_to_RGB(imY, upSampledCb, upSampledCr, (*imOut));

    printf("Saving decompressed image\n");
    std::string cNomImgOutStr = cNomImgOut;
    (*imOut).save(cNomImgOutStr.data());


}