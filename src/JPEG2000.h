#pragma once

#include "Utils.h"
#include "ImageBase.h"
#include <vector>
#include <fstream>
#include "RLE.h"
#include "Huffman.h"
#include <unordered_map>

struct Tile {
    std::vector<std::vector<int>> data;
    int x, y, width, height;

    Tile(int w, int h, int xPos, int yPos) : width(w), height(h), x(xPos), y(yPos) {
        data.resize(h, std::vector<int>(w, 0));  // Initialisation
    }
};

std::vector<Tile> getTiles(ImageBase &imIn, int tileSize);


void apply53(std::vector<std::vector<int>>& data);
void inverse53(std::vector<std::vector<int>>& data);
void applyWaveletTransform53ToTiles(std::vector<Tile>& tiles);
void inverseWaveletTransform53ToTiles(std::vector<Tile>& tiles);


void quantificationuniforme(Tile& tile, int quantizationStepLow, int quantizationStepHigh);
void inverseQuantificationuniforme(Tile& tile, int quantizationStepLow, int quantizationStepHigh);
void quantificationForAllTiles(std::vector<Tile> & tiles);
void inverse_quantificationForAllTiles(std::vector<Tile> & tiles);

std::vector<int> getFlatTile(Tile & tile);

void decompressTilesRLE(const std::vector<std::pair<int, int>>& tilesYRLE, std::vector<Tile>& tilesY, int tileWidth, int tileHeight);

void reconstructImage(std::vector<Tile> & tiles, ImageBase & imIn, int tileWidth, int tileHeight);

void compression2000( char * cNomImgLue,  char * cNomImgOut, ImageBase & imIn);

void decompression2000(const char * cNomImgIn, const char * cNomImgOut, ImageBase * imOut);