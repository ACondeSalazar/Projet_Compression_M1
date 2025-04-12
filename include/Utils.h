#pragma once
#include <vector>
#include <math.h>
#include <string>
#include "ImageBase.h"
#include <iostream>

enum BlurType{
    GAUSSIANBLUR,
    MEDIANBLUR,
    BILATERALBLUR,
};

enum ColorFormat{
    YCBCRFORMAT,
    YCOCGFORMAT,
    YUVFORMAT,
};

enum SamplingType{
    NORMALSAMPLING,
    BILENARSAMPLING,
    BICUBICSAMPLING,
    LANCZOSSAMPLING,
};

enum TransformationType{
    DCTTRANSFORM, //dct classique
    DWTTRANSFORM, //ondelette
    INTDCTTRANSFORM, //dct entiere
    DCTIVTRANSFORM, // dct IV
};

enum EncodingType{
    RLE,
    LZ77
};

struct CompressionSettings {

    ColorFormat colorFormat;

    BlurType blurType;

    SamplingType samplingType;

    TransformationType transformationType;
    int QuantizationFactor;

    int tileWidth = 120;
    int tileHeight = 120;

    EncodingType encodingType = LZ77;
    int encodingWindowSize = 10000;

    void printSettings() const {
        std::cout << "Color Format: " << colorFormat << std::endl;
        std::cout << "Blur Type: " << blurType << std::endl;
        std::cout << "Sampling Type: " << samplingType << std::endl;
        std::cout << "Transformation Type: " << transformationType << std::endl;
        std::cout << "Quantization Factor: " << QuantizationFactor << std::endl;
        std::cout << "Tile Width: " << tileWidth << std::endl;
        std::cout << "Tile Height: " << tileHeight << std::endl;
        std::cout << "Encoding Type: " << encodingType << std::endl;
        std::cout << "Encoding Window Size: " << encodingWindowSize << std::endl;
    }
};


struct Block{

    Block() : data(8, std::vector<int>(8, 0)), dctMatrix(8, std::vector<double>(8, 0)), flatDctMatrix(64) {}

    Block(int size) : data(size, std::vector<int>(size, 0)), dctMatrix(size, std::vector<double>(size, 0)), flatDctMatrix(size*size) {}

    std::vector<std::vector<int>> data;
    std::vector<std::vector<double>> dctMatrix;
    std::vector<int> flatDctMatrix;

};

struct Tile {
    std::vector<std::vector<int>> data;
    int x, y, width, height;

    Tile(int w, int h, int xPos, int yPos) : width(w), height(h), x(xPos), y(yPos) {
        data.resize(h, std::vector<int>(w, 0));  // Initialisation
    }

    void print() const {
        for (const auto& row : data) {
            for (const auto& value : row) {
                std::cout << value << " ";
            }
            std::cout << std::endl;
        }
    }
};

//Quantification, matrice trouvé sur https://lehollandaisvolant.net/science/jpg/

static const std::vector<std::vector<int>> quantificationMatrix = {
    {5, 9, 13, 17, 21, 25, 29, 33},
    {9, 13, 17, 21, 25, 29, 33, 37},
    {13, 17, 21, 25, 29, 33, 37, 41},
    {17, 21, 25, 29, 33, 37, 41, 45},
    {21, 25, 29, 33, 37, 41, 45, 49},
    {25, 29, 33, 37, 41, 45, 49, 53},
    {29, 33, 37, 41, 45, 49, 53, 57},
    {33, 37, 41, 45, 49, 53, 57, 61}
};

const std::vector<std::vector<int>> quantificationMatrix2 = {
    {1, 3, 5, 9, 10, 12, 15, 17},
    {3, 5, 9, 10, 12, 15, 17, 18},
    {5, 9, 10, 12, 15, 17, 18, 20},
    {9, 10, 12, 15, 17, 18, 20, 22},
    {10, 12, 15, 17, 18, 20, 22, 24},
    {12, 15, 17, 18, 20, 22, 24, 26},
    {15, 17, 18, 20, 22, 24, 26, 28},
    {17, 18, 20, 22, 24, 26, 28, 30}
};

const std::vector<std::vector<int>> quantificationMatrix3 = {
    {2, 3, 4, 6, 7, 9, 11, 13},
    {3, 4, 6, 7, 9, 11, 13, 14},
    {4, 6, 7, 9, 11, 13, 14, 16},
    {6, 7, 9, 11, 13, 14, 16, 18},
    {7, 9, 11, 13, 14, 16, 18, 20},
    {9, 11, 13, 14, 16, 18*2, 20*2, 22*2},
    {11, 13, 14, 16, 18, 20*2, 22*2, 24*2},
    {13, 14, 16, 18, 20, 22*2, 24*2, 26*2}
};


const std::vector<std::vector<int>> quantificationMatrix_test = {
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 99, 99, 99, 99},
    {1, 1, 1, 1, 99, 99, 99, 99},
    {1, 1, 1, 1, 99, 99, 99, 99},
    {1, 1, 1, 1, 99, 99, 99, 99}
};

const std::vector<std::vector<int>> quantificationMatrixUniform = {
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1}
};

static const std::vector<std::vector<int>> quantificationLuminance = {
    {16,11,10,16,24,40,51,61},
    {12,12,14,19,26,58,60,55},
    {14,13,16,24,40,57,69,56},
    {14,17,22,29,51,87,80,62},
    {18,22,37,56,68,109,103,77},
    {24,35,55,64,81,104,113,92},
    {49,64,78,87,103,121,120,101},
    {72,92,95,98,112,100,103,99}
};

static const std::vector<std::vector<int>> quantificationChrominance = {
    {17,18,24,47,99,99,99,99},
    {18,21,26,66,99,99,99,99},
    {24,26,56,99,99,99,99,99},
    {47,66,99,99,99,99,99,99},
    {99,99,99,99,99,99,99,99},
    {99,99,99,99,99,99,99,99},
    {99,99,99,99,99,99,99,99},
    {99,99,99,99,99,99,99,99}
};


void flattenZigZag(Block &block);

void unflattenZigZag(Block &block);

float PSNR(ImageBase & im1, ImageBase & im2);

float PSNRptr(ImageBase & im1, ImageBase * im2);


long getFileSize(const std::string& filePath);

void readSettings(const std::string& filename, CompressionSettings& settings);