#include "ImageBase.h"
#include <vector>
#include <fstream>
#include "Utils.h"


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

struct Block{
    std::vector<std::vector<int>> data;

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

//peut mener a des erreurs d'acces memoire si la taille de l'image n'est pas un multiple de blockSize
std::vector<Block> getBlocks(ImageBase & imIn, int blockSize = 8){

    std::vector<Block> blocks;

    for(int i = 0; i < imIn.getHeight(); i+=blockSize){
        for(int j = 0; j < imIn.getWidth(); j+=blockSize){

            Block block;

            for(int k = 0; k < blockSize; k++){
                std::vector<int> row;
                for(int l = 0; l < blockSize; l++){
                    row.push_back(imIn[i + k][j + l]);
                }
                block.data.push_back(row);
            }


            blocks.push_back(block);
        }
    }

    return blocks;
}

//Transformée cosinus discrete

//Quantification

//Codage RLE et Huffman -> Fin 

//fonction qui regroupe tout 

void compression(ImageBase & imIn){}