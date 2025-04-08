#pragma once

#include "Utils.h"
#include <tuple>
#include <vector>



struct LZ77Triplet {
    int offset;
    int length;
    int next;

    LZ77Triplet(int o, int l, char n) : offset(o), length(l), next(n) {}

    LZ77Triplet(std::tuple<int,int,int> triplet) {
        offset = std::get<0>(triplet);
        length = std::get<1>(triplet);
        next = std::get<2>(triplet);
    }
};

void LZ77Compression(std::vector<int> & data, std::vector<LZ77Triplet> & compressedData, int windowSize = 10000);

void LZ77Decompression(std::vector<LZ77Triplet> & compressedData, std::vector<int> & data, int expectedSize);

void decompressTilesLZ77( std::vector<LZ77Triplet>& tilesYLZ77, std::vector<Tile>& tilesY, int tileWidth, int tileHeight);
