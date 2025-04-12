
#include "Utils.h"

#include <iostream>
#include <vector>
#include <math.h>
#include <fstream>
#include "ImageBase.h"






//tableau qui permet de parcourir la dctMatrix dans l'ordre zigzag, si on veut des bloc de taille != 8, il faudra trouver un algo pour générer ce tableau
int zigzag[8][8] = {
        { 0,  1,  5,  6, 14, 15, 27, 28},
        { 2,  4,  7, 13, 16, 26, 29, 42},
        { 3,  8, 12, 17, 25, 30, 41, 43},
        { 9, 11, 18, 24, 31, 40, 44, 53},
        {10, 19, 23, 32, 39, 45, 52, 54},
        {20, 22, 33, 38, 46, 51, 55, 60},
        {21, 34, 37, 47, 50, 56, 59, 61},
        {35, 36, 48, 49, 57, 58, 62, 63}
};

void flattenZigZag(Block &block) {
    // On suppose que la taille de dctMatrix est 8x8 et que flatDctMatrix peut contenir 64 éléments.
    for (int i = 0; i < 8 * 8; i++) {
        int row = zigzag[i / 8][i % 8] / 8; // Calcul de la ligne dans la matrice
        int col = zigzag[i / 8][i % 8] % 8; // Calcul de la colonne dans la matrice
        block.flatDctMatrix[i] = block.dctMatrix[row][col];
    }
}

void unflattenZigZag(Block &block) {
    for (int i = 0; i < 8 * 8; i++) {
        int row = zigzag[i / 8][i % 8] / 8; // Calcul de la ligne dans la matrice
        int col = zigzag[i / 8][i % 8] % 8; // Calcul de la colonne dans la matrice
        block.dctMatrix[row][col] = block.flatDctMatrix[i];
    }
}


float PSNR(ImageBase & im1, ImageBase & im2){
    float mse = 0;
    for (int i = 0; i < im1.getHeight(); i++){
        for (int j = 0; j < im1.getWidth(); j++){
            mse += pow(im1[i*3][j*3 + 0] - im2[i*3][j*3 + 0], 2);
            mse += pow(im1[i*3][j*3 + 1] - im2[i*3][j*3 + 1], 2);
            mse += pow(im1[i*3][j*3 + 2] - im2[i*3][j*3 + 2], 2);
        }
    }
    mse /= (im1.getHeight() * im1.getWidth() * 3);
    return 10 * log10(pow(255, 2) / mse);
}

float PSNRptr(ImageBase & im1, ImageBase * im2){
    float mse = 0;
    for (int i = 0; i < im1.getHeight(); i++){
        for (int j = 0; j < im1.getWidth(); j++){
            mse += pow(im1[i*3][j*3 + 0] - (*im2)[i*3][j*3 + 0], 2);
            mse += pow(im1[i*3][j*3 + 1] - (*im2)[i*3][j*3 + 1], 2);
            mse += pow(im1[i*3][j*3 + 2] - (*im2)[i*3][j*3 + 2], 2);
        }
    }
    mse /= (im1.getHeight() * im1.getWidth() * 3);
    return 10 * log10(pow(255, 2) / mse);
}



long getFileSize(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate); 
    if (!file) {
        printf("Erreur d'ouverture du fichier");
        return -1;
    }
    return file.tellg();
}


void readSettings(const std::string& filename, CompressionSettings& settings) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile) {
        std::cerr << "Error opening file for reading settings!" << std::endl;
        return;
    }

    inFile.read(reinterpret_cast<char*>(&settings), sizeof(settings));

    if (!inFile) {
        std::cerr << "Error reading settings from file!" << std::endl;
    }

    inFile.close();
}