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

    for (int i = 0; i < height; i += tileSize) {
        for (int j = 0; j < width; j += tileSize) {
            int tileWidth = std::min(tileSize, width - j);
            int tileHeight = std::min(tileSize, height - i);

            Tile tile(tileWidth, tileHeight, j, i);

            for (int k = 0; k < tileHeight; k++) {
                for (int l = 0; l < tileWidth; l++) {
                    tile.data[k][l] = imIn[i + k][j + l];
                }
            }

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

std::vector<int> getFlatTile(Tile & tile){
    std::vector<int> res;
    int width = tile.width;
    int height = tile.height;
    for(int i = 0; i < width; i++){
        for(int j = 0; j< width; j++){
            res.push_back(tile.data[i][j]);
        }
    }

    return res;
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

    //Découpage en blocs de tiles
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


