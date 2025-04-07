#pragma once

#include "Utils.h"
#include "ImageBase.h"
#include <vector>



std::vector<Tile> getTiles(ImageBase &imIn, int tileWidth, int tileHeight);

std::vector<int> getFlatTile(Tile & tile);

void decompressTilesRLE(const std::vector<std::pair<int, int>>& tilesYRLE, std::vector<Tile>& tilesY, int tileWidth, int tileHeight);

void reconstructImage(std::vector<Tile> & tiles, ImageBase & imIn, int tileWidth, int tileHeight);

void compression2000( char * cNomImgLue,  char * cNomImgOut, ImageBase & imIn, CompressionSettings & settings);

void decompression2000(const char * cNomImgIn, const char * cNomImgOut, ImageBase * imOut, CompressionSettings & settings);