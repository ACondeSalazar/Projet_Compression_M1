#pragma once

#include "Utils.h"
#include <cstdint>
#include <tuple>
#include <vector>
#include "TransformationQuantification.h"



struct LZ77Triplet {
    uint8_t offset;
    uint8_t length;
    int16_t next;

    LZ77Triplet(uint8_t o, uint8_t l, int n) : offset(o), length(l), next(n) {}

    LZ77Triplet(std::tuple<uint8_t, uint8_t, int16_t> triplet) {
        offset = std::get<0>(triplet);
        length = std::get<1>(triplet);
        next = std::get<2>(triplet);
    }
};;

void LZ77Compression(std::vector<int> & data, std::vector<LZ77Triplet> & compressedData, int windowSize = 10000);

void LZ77Decompression(std::vector<LZ77Triplet> & compressedData, std::vector<int> & data, int expectedSize);

void decompressTilesLZ77( std::vector<LZ77Triplet>& tilesYLZ77, std::vector<Tile>& tilesY, int tileWidth, int tileHeight, int totalTiles);

void decompressBlocksLZ77(std::vector<LZ77Triplet> & tilesLZ77, std::vector<Block> & blocks, int nbBlocks,CompressionSettings & settings, const std::vector<std::vector<int>> quantificationMatrix ,int blockSize = 8);