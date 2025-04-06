#pragma once

#include "Utils.h"
#include "ImageBase.h"
#include <vector>

void RGB_to_YCbCr(ImageBase & imIn, ImageBase & Y, ImageBase & Cb, ImageBase & Cr);

void YCbCr_to_RGB(ImageBase & imY, ImageBase & imCb, ImageBase & imCr, ImageBase & imOut);

void down_sampling(ImageBase & imIn, ImageBase & imOut);

void down_sampling_bilinear(ImageBase &imIn, ImageBase &imOut);

void up_sampling(ImageBase & imIn, ImageBase & imOut);

std::vector<Block> getBlocks(ImageBase & imIn, int blockSize = 8);

void reconstructImage(std::vector<Block> & blocks, ImageBase & imIn, int blocksize);

void compression( char * cNomImgLue,  char * cNomImgOut, ImageBase & imIn);

void decompression(const char * cNomImgIn, const char * cNomImgOut, ImageBase * imOut);


