#pragma once
#include <vector>
#include <math.h>
#include <fstream>
#include "ImageBase.h"


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

struct CompressionSettings {

    ColorFormat colorFormat;

    BlurType blurType;

    SamplingType samplingType;

    TransformationType transformationType;
    int QuantizationFactor;




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
};


void quantification (Block & block);

void inverse_quantification (Block & block);

void decompressBlocksRLE(const std::vector<std::pair<int,int>> & encodedRLE, std::vector<Block> & blocks, TransformationType transformationType);

void DCT(Block & block);

void IDCT(Block & block);

void flattenZigZag(Block &block);

void unflattenZigZag(Block &block);

float PSNR(ImageBase & im1, ImageBase & im2);

void gaussianBlur(ImageBase &imIn, ImageBase &imOut);

long getFileSize(const std::string& filePath);
