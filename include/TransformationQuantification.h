#pragma once 
#include "Utils.h"

//-- transformation 

void DCT(Block & block);
void IDCT(Block & block);

void INTDCT(Block & block);
void INTIDCT(Block & block);

//https://arm-software.github.io/CMSIS_5/DSP/html/group__DCT4__IDCT4.html
void DCT_IV(Block & block, bool forward, unsigned int N = 8); //permet de faire la dct IV ET son inverse

// ondelette 
void apply53(std::vector<std::vector<int>>& data);
void inverse53(std::vector<std::vector<int>>& data);

void applyWaveletTransform53ToTiles(std::vector<Tile>& tiles);
void inverseWaveletTransform53ToTiles(std::vector<Tile>& tiles);

//-- quantification

float getScaling(int quantificationFactor);
std::pair<int, int> getQuantificationStep(int quantificationFactor);

void quantification (Block & block);
void inverse_quantification (Block & block);

void quantification_better (Block & block, const std::vector<std::vector<int>> & matrix, int quantificationFactor);
void inverse_quantification_better (Block & block, const std::vector<std::vector<int>> & matrix, int quantificationFactor);

void inverse_quantification_uniforme (Block & block, int quantificationFactor);
void quantification_uniforme (Block & block, int quantificationFactor);


// quantification ondelette
void quantificationuniforme(Tile& tile, int quantizationStepLow, int quantizationStepHigh);
void inverseQuantificationuniforme(Tile& tile, int quantizationStepLow, int quantizationStepHigh);
void quantificationForAllTiles(std::vector<Tile> & tiles);
void inverse_quantificationForAllTiles(std::vector<Tile> & tiles);