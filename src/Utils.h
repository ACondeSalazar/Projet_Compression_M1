#pragma once
#include <vector>
#include <math.h>
#include <fstream>
#include "ImageBase.h"

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






void DCT(Block & block);

void IDCT(Block & block);

void flattenZigZag(Block &block);

void unflattenZigZag(Block &block);

float PSNR(ImageBase & im1, ImageBase & im2);

void gaussianBlur(ImageBase &imIn, ImageBase &imOut);

long getFileSize(const std::string& filePath);
