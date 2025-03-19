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

int tilewidth = 8;
int tileHeight = 8;
/*
Avec JPEG2000 on ne fait pas des blocks de 8 par 8 mais des Tiles
Ici on va faire des Tiles de taille 128*128 ou 256*256
*/

std::vector<Tile> getTiles(ImageBase &imIn, int tileWidth, int tileHeight) {
    std::vector<Tile> tiles;
    int height = imIn.getHeight();
    int width = imIn.getWidth();

    // Le nombre de tiles dans chaque direction
    int numTilesX = (width + tileWidth - 1) / tileWidth;  // Arrondir vers le haut
    int numTilesY = (height + tileHeight - 1) / tileHeight;  // Arrondir vers le haut

    // Remplir le vecteur de tiles
    for (int i = 0; i < numTilesY; i++) {
        for (int j = 0; j < numTilesX; j++) {
            // Calculer la largeur et la hauteur du tile en fonction de la position
            int currentTileWidth = std::min(tileWidth, width - j * tileWidth);
            int currentTileHeight = std::min(tileHeight, height - i * tileHeight);

            // Créer un tile
            Tile tile(currentTileWidth, currentTileHeight, j * tileWidth, i * tileHeight);

            // Remplir le tile avec les données de l'image
            for (int k = 0; k < currentTileHeight; k++) {
                for (int l = 0; l < currentTileWidth; l++) {
                    tile.data[k][l] = imIn[i * tileHeight + k][j * tileWidth + l];
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


void inverseCDF97(std::vector<std::vector<int>>& data) {
    int height = data.size();
    int width = data[0].size();

    // Inverse de la transformation sur les colonnes
    for (int j = 0; j < width; ++j) {
        for (int i = height - 2; i >= 1; --i) {
            data[i + 1][j] = data[i + 1][j] + data[i][j];  // Restaurer la valeur
            data[i][j] = 2 * data[i][j] - data[i - 1][j];  // Annuler l'opération moyenne
        }
    }

    // Inverse de la transformation sur les lignes
    for (int i = 0; i < height; ++i) {
        for (int j = width - 2; j >= 1; --j) {
            data[i][j + 1] = data[i][j + 1] + data[i][j];  // Restaurer la valeur
            data[i][j] = 2 * data[i][j] - data[i][j - 1];  // Annuler l'opération moyenne
        }
    }
}

void inverseWaveletTransformToTiles(std::vector<Tile>& tiles) {
    for (auto& tile : tiles) {
        inverseCDF97(tile.data);
    }
}
//==========================================================================================================================
void apply53(std::vector<std::vector<int>>& data) {
    int height = data.size();
    int width = data[0].size();

    // Appliquer la transformée sur les lignes
    for (int y = 0; y < height; ++y) {
        // Étape de prédiction (lifting)
        for (int x = 1; x < width - 1; x += 2) {
            data[y][x] -= (data[y][x - 1] + data[y][x + 1] + 1) / 2;
        }
        // Étape de mise à jour
        for (int x = 0; x < width; x += 2) {
            if (x == 0) {
                data[y][x] += (data[y][x + 1] + 1) / 2;
            } else if (x == width - 1) {
                data[y][x] += (data[y][x - 1] + 1) / 2;
            } else {
                data[y][x] += (data[y][x - 1] + data[y][x + 1] + 2) / 4;
            }
        }
    }

    // Appliquer la transformée sur les colonnes
    for (int x = 0; x < width; ++x) {
        // Étape de prédiction (lifting)
        for (int y = 1; y < height - 1; y += 2) {
            data[y][x] -= (data[y - 1][x] + data[y + 1][x] + 1) / 2;
        }
        // Étape de mise à jour
        for (int y = 0; y < height; y += 2) {
            if (y == 0) {
                data[y][x] += (data[y + 1][x] + 1) / 2;
            } else if (y == height - 1) {
                data[y][x] += (data[y - 1][x] + 1) / 2;
            } else {
                data[y][x] += (data[y - 1][x] + data[y + 1][x] + 2) / 4;
            }
        }
    }
}

// Appliquer la transformée inverse en ondelettes 5/3 sur une matrice 2D
void inverse53(std::vector<std::vector<int>>& data) {
    int height = data.size();
    int width = data[0].size();

    // Appliquer l'inverse sur les colonnes
    for (int x = 0; x < width; ++x) {
        // Étape de mise à jour inverse
        for (int y = 0; y < height; y += 2) {
            if (y == 0) {
                data[y][x] -= (data[y + 1][x] + 1) / 2;
            } else if (y == height - 1) {
                data[y][x] -= (data[y - 1][x] + 1) / 2;
            } else {
                data[y][x] -= (data[y - 1][x] + data[y + 1][x] + 2) / 4;
            }
        }
        // Étape de prédiction inverse
        for (int y = 1; y < height - 1; y += 2) {
            data[y][x] += (data[y - 1][x] + data[y + 1][x] + 1) / 2;
        }
    }

    // Appliquer l'inverse sur les lignes
    for (int y = 0; y < height; ++y) {
        // Étape de mise à jour inverse
        for (int x = 0; x < width; x += 2) {
            if (x == 0) {
                data[y][x] -= (data[y][x + 1] + 1) / 2;
            } else if (x == width - 1) {
                data[y][x] -= (data[y][x - 1] + 1) / 2;
            } else {
                data[y][x] -= (data[y][x - 1] + data[y][x + 1] + 2) / 4;
            }
        }
        // Étape de prédiction inverse
        for (int x = 1; x < width - 1; x += 2) {
            data[y][x] += (data[y][x - 1] + data[y][x + 1] + 1) / 2;
        }
    }
}

void applyWaveletTransform53ToTiles(std::vector<Tile>& tiles) {
    for (auto& tile : tiles) {
        apply53(tile.data);
    }
}

void inverseWaveletTransform53ToTiles(std::vector<Tile>& tiles) {
    for (auto& tile : tiles) {
        inverse53(tile.data);
    }
}

//==========================================================================================================================
//quantification



void quantificationuniforme(Tile& tile, int quantizationStepLow, int quantizationStepHigh) {
    int height = tile.data.size();
    int width = tile.data[0].size();

    // On suppose que la sous-bande LL est en haut à gauche (premier quart de la tuile)
    int llHeight = height / 2;
    int llWidth = width / 2;

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            if (i < llHeight && j < llWidth) {
                // Sous-bande LL (basse fréquence) : quantification fine
                tile.data[i][j] = (tile.data[i][j] + quantizationStepLow / 2) / quantizationStepLow;
            } else {
                // Sous-bandes LH, HL, HH (haute fréquence) : quantification plus forte
                tile.data[i][j] = (tile.data[i][j] + quantizationStepHigh / 2) / quantizationStepHigh;
            }
        }
    }
}

void inverseQuantificationuniforme(Tile& tile, int quantizationStepLow, int quantizationStepHigh) {
    int height = tile.data.size();
    int width = tile.data[0].size();

    int llHeight = height / 2;
    int llWidth = width / 2;

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            if (i < llHeight && j < llWidth) {
                // Sous-bande LL
                tile.data[i][j] *= quantizationStepLow;
            } else {
                // Sous-bandes LH, HL, HH
                tile.data[i][j] *= quantizationStepHigh;
            }
        }
    }
}



void quantificationForAllTiles(std::vector<Tile> & tiles) {
    for (auto& tile : tiles) {
        quantificationuniforme(tile, 2,4);  // ici on controle le taux de compression -> 1,2 tres bon -> 2,4 bon -> 4,8 moyen-> 8, 16 mauvais
    }
}

void inverse_quantificationForAllTiles(std::vector<Tile> & tiles) {
    for (auto& tile : tiles) {
        inverseQuantificationuniforme(tile,2,4); // ici on controle le taux de compression -> 1,2 tres bon -> 2,4 bon -> 4,8 moyen-> 8, 16 mauvais
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
    std::vector<int> decompressedData;
    
    // Décompression des données RLE en un tableau linéaire
    for (const auto& pair : tilesYRLE) {
        int count = pair.first;
        int value = pair.second;
        decompressedData.insert(decompressedData.end(), count, value);
    }
    
    // Vérification de la taille des données
    int numTiles = decompressedData.size() / (tileWidth * tileHeight);
    if (numTiles * tileWidth * tileHeight != decompressedData.size()) {
        std::cerr << "Erreur: Données RLE mal formées ou incorrectes." << std::endl;
        return;
    }
    
    // Remplissage des Tiles
    tilesY.clear();
    int index = 0;
    for (int t = 0; t < numTiles; ++t) {
        Tile tile(tileWidth, tileHeight, 0, 0);
        for (int i = 0; i < tileHeight; ++i) {
            for (int j = 0; j < tileWidth; ++j) {
                tile.data[i][j] = decompressedData[index++];
            }
        }
        tilesY.push_back(tile);
    }


}

//==========================================================================================================================

void reconstructImage(std::vector<Tile> & tiles, ImageBase & imIn, int tileWidth, int tileHeight) {
    int height = imIn.getHeight();
    int width = imIn.getWidth();
    
    int numTilesX = width / tileWidth;
    int numTilesY = height / tileHeight;

    if (tiles.size() != numTilesX * numTilesY) {
        std::cerr << "Erreur : Nombre de tiles incorrect (" << tiles.size() << " au lieu de " << numTilesX * numTilesY << ")" << std::endl;
        return;
    }

    for (int i = 0; i < height; i += tileHeight) {  // Correction ici
        for (int j = 0; j < width; j += tileWidth) {  // Correction ici

            int tileX = j / tileWidth;
            int tileY = i / tileHeight;
            int tileIndex = tileY * numTilesX + tileX;

            if (tileIndex >= tiles.size()) {
                std::cerr << "Erreur : tileIndex hors limites (" << tileIndex << ")" << std::endl;
                continue;
            }

            Tile& tile = tiles[tileIndex];

            for (int k = 0; k < tileHeight; k++) {  // Correction ici
                for (int l = 0; l < tileWidth; l++) {  // Correction ici
                    int pixelX = j + l;
                    int pixelY = i + k;

                    // Vérifier que l'on reste dans les limites de l'image
                    if (pixelX >= width || pixelY >= height) continue;

                    int value = tile.data[k][l];
                    value = std::max(0, std::min(255, value)); // Clamp entre 0 et 255

                    imIn[pixelY][pixelX] = value;
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

    std::vector<Tile> tilesY = getTiles(imY, tilewidth,tileHeight); // ici ca devrait etre plus de 8 genre 128 ou plus mais ca plante (problème dans getTiles)
    std::vector<Tile> tilesCb = getTiles(downSampledCb, tilewidth,tileHeight);
    std::vector<Tile> tilesCr = getTiles(downSampledCr, tilewidth,tileHeight);

    printf("number of tiles for Y channel: %d\n", tilesY.size());
    printf("number of tiles for Cb channel: %d\n", tilesCb.size());
    printf("number of tiles for Cr channel: %d\n", tilesCr.size());


    printf("Wavelet transform et quantification : \n"); // wavelet

    
    applyWaveletTransform53ToTiles(tilesY);
    

    applyWaveletTransform53ToTiles(tilesCr);
    

    applyWaveletTransform53ToTiles(tilesCb);
    
    
    quantificationForAllTiles(tilesY);
    
    quantificationForAllTiles(tilesCr);

    quantificationForAllTiles(tilesCb);
    

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

    /*int totalLengthCb = 0;
    for (const auto& pair : tilesCrRLE) {
        totalLengthCb += pair.first;
    }
    std::cout<<"ici trouduc"<<totalLengthCb<<std::endl;*/

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

    for(int i = 10; i<30; i++){
        printf("[%d , %d] ,",tilesYRLE[i].first,tilesYRLE[i].second);
    }


    printf("Blocks decoding\n");

    
    std::vector<Tile> tilesY;
    std::vector<Tile> tilesCb;
    std::vector<Tile> tilesCr;

    printf("tilesCbRLE size: %lu\n", tilesCbRLE.size());
    decompressTilesRLE(tilesCbRLE, tilesCb,tilewidth,tileHeight);
    printf("tilesCb size: %lu\n", tilesCb.size());


    decompressTilesRLE(tilesCrRLE, tilesCr,tilewidth,tileHeight);
    printf("tilesCr size: %lu\n", tilesCr.size());
   
    decompressTilesRLE(tilesYRLE, tilesY,tilewidth,tileHeight);
    printf("tilesY size: %lu\n", tilesY.size()); 


    inverse_quantificationForAllTiles(tilesCb);
    inverse_quantificationForAllTiles(tilesCr);
    inverse_quantificationForAllTiles(tilesY);
    

    inverseWaveletTransform53ToTiles(tilesCb);
    inverseWaveletTransform53ToTiles(tilesCr);
    inverseWaveletTransform53ToTiles(tilesY);


    printf("\n");

    printf("Reconstructing image from tiles\n");


    //ImageBase imOut(512, 480, false);

    imOut = new ImageBase(imageWidth, imageHeight, true);

    ImageBase imY(imageWidth, imageHeight, false);

    ImageBase imCb(downSampledWidth, downSampledHeight, false);
    ImageBase imCr(downSampledWidth, downSampledHeight, false);

    ImageBase upSampledCb(imageWidth, imageHeight, false);
    ImageBase upSampledCr(imageWidth, imageHeight, false);

    printf("Reconstructing Cb channel\n");
    reconstructImage(tilesCb, imCb,tilewidth,tileHeight);
    up_sampling(imCb, upSampledCb);
    upSampledCb.save("./img/out/Cb_decompressed.pgm");

    printf("Reconstructing Cr channel\n");
    reconstructImage(tilesCr, imCr,tilewidth,tileHeight);
    up_sampling(imCr, upSampledCr);
    upSampledCr.save("./img/out/Cr_decompressed.pgm");

    printf("Reconstructing Y channel\n");
    reconstructImage(tilesY, imY,tilewidth,tileHeight);
    printf("saving Y channel\n");
    imY.save("./img/out/Y_decompressed.pgm");

    

    printf("Reconstructing image from YCbCr\n");
    YCbCr_to_RGB(imY, upSampledCb, upSampledCr, (*imOut));

    printf("Saving decompressed image\n");
    std::string cNomImgOutStr = cNomImgOut;
    (*imOut).save(cNomImgOutStr.data());


}