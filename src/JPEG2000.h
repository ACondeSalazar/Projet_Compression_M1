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

void quantification(Tile & tile, const std::vector<std::vector<int>>& quantizationMatrix);
void quantificationForAllTiles(std::vector<Tile> & tiles, const std::vector<std::vector<int>>& quantizationMatrix);

std::vector<int> getFlatTile(Tile & tile);

void compression2000( char * cNomImgLue,  char * cNomImgOut, ImageBase & imIn);