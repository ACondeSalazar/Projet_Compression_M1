#pragma once
#include <vector>
#include <math.h>
#include "JPEG.h"

struct Block{

    Block() : data(8, std::vector<int>(8, 0)), dctMatrix(8, std::vector<double>(8, 0)), flatDctMatrix(64) {}

    Block(int size) : data(size, std::vector<int>(size, 0)), dctMatrix(size, std::vector<double>(size, 0)), flatDctMatrix(size*size) {}

    std::vector<std::vector<int>> data;
    std::vector<std::vector<double>> dctMatrix;
    std::vector<int> flatDctMatrix;

    

    void savePGM( const char * filename) {
        std::ofstream file(filename, std::ios::out | std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open file for writing");
        }

        int height = data.size();
        int width = data[0].size();

        file << "P5\n" << width << " " << height << "\n255\n";

        for ( auto& row : data) {
            for (int val : row) {
                file.put(static_cast<unsigned char>(val));
            }
        }

        file.close();
    }
};

void DCT(Block & block){

    double PI = 3.14159265358979323846;
    //PI = M_PI;


    int N = block.data.size();

    double mul = PI / (2 * N); // petite optimisation
    const double inv_sqrt2 = 1.0 / sqrt(2.0); //micro opti

    for (int u = 0; u < N; u++){
        for (int v = 0; v < N; v++){ //pour chaque fréquence
            double sum = 0;
            for (int x = 0; x < N; x++){
                for (int y = 0; y < N; y++){

                    sum += block.data[x][y] * cos((2 * x + 1) * u * mul) * cos((2 * y + 1) * v * mul);
                
                }
            }
            double Cu = (u == 0) ? inv_sqrt2 : 1;
            double Cv = (v == 0) ? inv_sqrt2 : 1;
            block.dctMatrix[u][v] = 0.25 * Cu * Cv * sum;
        }
    }
}

void IDCT(Block & block) {
    double PI = 3.14159265358979323846;
    
    int N = block.dctMatrix.size();

    double mul = PI / (2 * N); // petite optimisation
    const double inv_sqrt2 = 1.0 / sqrt(2.0); //micro opti

    for (int x = 0; x < N; x++) {
        for (int y = 0; y < N; y++) { // reconstruct pixel values
            double sum = 0;
            for (int u = 0; u < N; u++) {
                for (int v = 0; v < N; v++) {
                    
                    double Cu = (u == 0) ? inv_sqrt2 : 1.0;
                    double Cv = (v == 0) ? inv_sqrt2 : 1.0;
                    
                    sum += Cu * Cv * block.dctMatrix[u][v] * cos((2 * x + 1) * u * mul) * cos((2 * y + 1) * v * mul);
                }
            }
            block.data[x][y] = 0.25 * sum;
        }
    }
}


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


void gaussianBlur(ImageBase &imIn, ImageBase &imOut) {
    int width = imIn.getWidth();
    int height = imIn.getHeight();

    // Filtre gaussien 3x3 (sigma ≈ 1)
    std::vector<std::vector<int>> kernel = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };

    int kernelSum = 16; // Somme du noyau gaussien

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int sum = 0;

            if(i == 0 || j == 0 || i == height - 1 || j == width -1){
                imOut[i][j] = imIn[i][j];
                continue;
            }

            for (int ki = -1; ki <= 1; ki++) {
                for (int kj = -1; kj <= 1; kj++) {
                    sum += imIn[i + ki][j + kj] * kernel[ki + 1][kj + 1];
                }
            }

            imOut[i][j] = sum / kernelSum;
        }
    }
}

long getFileSize(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate); 
    if (!file) {
        printf("Erreur d'ouverture du fichier");
        return -1;
    }
    return file.tellg();
}
