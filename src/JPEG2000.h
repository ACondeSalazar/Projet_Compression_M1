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

void applyCDF97(std::vector<std::vector<int>>& data);
void applyWaveletTransformToTiles(std::vector<Tile>& tiles);

void inverseCDF97(std::vector<std::vector<int>>& data);
void inverseWaveletTransformToTiles(std::vector<Tile>& tiles);

void apply53(std::vector<std::vector<int>>& data);
void inverse53(std::vector<std::vector<int>>& data);
void applyWaveletTransform53ToTiles(std::vector<Tile>& tiles);
void inverseWaveletTransform53ToTiles(std::vector<Tile>& tiles);


void quantification(Tile & tile, const std::vector<std::vector<int>>& quantizationMatrix);
void inverse_quantification(Tile & tile, const std::vector<std::vector<int>>& quantizationMatrix);
void quantificationForAllTiles(std::vector<Tile> & tiles, const std::vector<std::vector<int>>& quantizationMatrix);

std::vector<int> getFlatTile(Tile & tile);

void decompressTilesRLE(const std::vector<std::pair<int, int>>& tilesYRLE, std::vector<Tile>& tilesY, int tileWidth, int tileHeight);

void reconstructImage(std::vector<Tile> & tiles, ImageBase & imIn, int tileWidth, int tileHeight);

void compression2000( char * cNomImgLue,  char * cNomImgOut, ImageBase & imIn);

void decompression2000(const char * cNomImgIn, const char * cNomImgOut, ImageBase * imOut);