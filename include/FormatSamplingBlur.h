#pragma once
#include "ImageBase.h"

//------Changement de format couleur

void RGB_to_YCbCr(ImageBase & imIn, ImageBase & Y, ImageBase & Cb, ImageBase & Cr);
void YCbCr_to_RGB(ImageBase & imY, ImageBase & imCb, ImageBase & imCr, ImageBase & imOut);

void YCOCG_to_RGB(ImageBase & imIn, ImageBase & Y, ImageBase & Co, ImageBase & Cg);
void RGB_to_YCOCG(ImageBase & imIn, ImageBase & Y, ImageBase & Co, ImageBase & Cg);

void YUV_to_RGB(ImageBase & imIn, ImageBase & Y, ImageBase & U, ImageBase & V);
void RGB_to_YUV(ImageBase & imIn, ImageBase & Y, ImageBase & U, ImageBase & V);

//-- Flou applique sur les composantes de chrominance

void gaussianBlur(ImageBase &imIn, ImageBase &imOut);

void medianBlur(ImageBase &imIn, ImageBase &imOut);

void bilateralBlur(ImageBase &imIn, ImageBase &imOut); //https://stackoverflow.com/questions/6538310/anyone-know-where-i-can-find-a-glsl-implementation-of-a-bilateral-filter-blur

//-- méthode de sampling sur les composantes de chrominance

void down_sampling(ImageBase & imIn, ImageBase & imOut);
void up_sampling(ImageBase & imIn, ImageBase & imOut);

void down_sampling_bilinear(ImageBase &imIn, ImageBase &imOut);

void down_sampling_bicubic(ImageBase &imIn, ImageBase &imOut);
void down_sampling_lanczos(ImageBase &imIn, ImageBase &imOut);

void up_sampling_bicubic(ImageBase &imIn, ImageBase &imOut);
void up_sampling_lanczos(ImageBase &imIn, ImageBase &imOut);