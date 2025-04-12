#pragma once

#include "Utils.h"


void compressionFlex( char * cNomImgLue,  char * cNomImgOut, ImageBase & imIn, CompressionSettings & settings);
void decompressionFlex(const char * cNomImgIn, const char * cNomImgOut, ImageBase*& imOut, CompressionSettings & settings);