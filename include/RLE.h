#pragma once
#include <vector>
#include "Utils.h"


void RLECompression(std::vector<int> & flattenedBlock, std::vector<std::pair<int,int>> & RLEBlock);

void RLEDecompression(std::vector<std::pair<int,int>> & RLEBlock, std::vector<int> & flatdctMatrix);

void decompressBlocksRLE(const std::vector<std::pair<int,int>> & encodedRLE, std::vector<Block> & blocks,  const std::vector<std::vector<int>> & quantificationMatrix, CompressionSettings * settings);