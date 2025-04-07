#pragma once

#include "Utils.h"
#include "ImageBase.h"
#include <vector>



std::vector<Block> getBlocks(ImageBase & imIn, int blockSize = 8);

void reconstructImage(std::vector<Block> & blocks, ImageBase & imIn, int blocksize);

void compression( char * cNomImgLue,  char * cNomImgOut, ImageBase & imIn, CompressionSettings & settings);

void decompression(const char * cNomImgIn, const char * cNomImgOut, ImageBase * imOut, CompressionSettings & settings);


