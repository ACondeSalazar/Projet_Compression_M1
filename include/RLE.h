#pragma once
#include <vector>
#include "Utils.h"


void RLECompression(std::vector<int> & flattenedBlock, std::vector<std::pair<int,int>> & RLEBlock);

void RLEDecompression(std::vector<std::pair<int,int>> & RLEBlock, std::vector<int> & flatdctMatrix);