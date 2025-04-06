#pragma once

#include "ImageBase.h"
#include "Utils.h"

void medianBlur(ImageBase &imIn, ImageBase &imOut);
void bilateralBlur(ImageBase &imIn, ImageBase &imOut); //https://stackoverflow.com/questions/6538310/anyone-know-where-i-can-find-a-glsl-implementation-of-a-bilateral-filter-blur

void YCOCG_to_RGB(ImageBase & imIn, ImageBase & Y, ImageBase & Co, ImageBase & Cg, ImageBase & imOut);
void RGB_to_YCOCG(ImageBase & imIn, ImageBase & Y, ImageBase & Co, ImageBase & Cg);

void YUV_to_RGB(ImageBase & imIn, ImageBase & Y, ImageBase & U, ImageBase & V, ImageBase & imOut);
void RGB_to_YUV(ImageBase & imIn, ImageBase & Y, ImageBase & U, ImageBase & V);

void down_sampling_bicubic(ImageBase &imIn, ImageBase &imOut);
void down_sampling_lanczos(ImageBase &imIn, ImageBase &imOut);

void up_sampling_bicubic(ImageBase &imIn, ImageBase &imOut);
void up_sampling_lanczos(ImageBase &imIn, ImageBase &imOut);

void INTDCT(Block & block);
void INTIDCT(Block & block);

//https://arm-software.github.io/CMSIS_5/DSP/html/group__DCT4__IDCT4.html
void DCT_IV(Block & block, bool forward, unsigned int N); //permet de faire la dct IV ET son inverse




void compressionFlex( char * cNomImgLue,  char * cNomImgOut, ImageBase & imIn, CompressionSettings settings);
void decompressionFlex(const char * cNomImgIn, const char * cNomImgOut, ImageBase * imOut, CompressionSettings settings);