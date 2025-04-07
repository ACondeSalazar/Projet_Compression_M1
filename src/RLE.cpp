#pragma once
#include "RLE.h"
#include <vector>
#include "Utils.h"
#include "TransformationQuantification.h"
#include <iostream>

void RLECompression(std::vector<int> & flattenedBlock, std::vector<std::pair<int,int>> & RLEBlock){
    RLEBlock.clear();
    int currentValue = flattenedBlock[0];
    int counterValue = 1;
    
    for(int i = 1; i < flattenedBlock.size(); i++){
        if(flattenedBlock[i] == currentValue){
            counterValue++;
        } else {
            RLEBlock.push_back({counterValue, currentValue});
            currentValue = flattenedBlock[i];
            counterValue = 1;
        }
    }

    RLEBlock.push_back({counterValue, currentValue});
}


void RLEDecompression(std::vector<std::pair<int,int>> & RLEBlock, std::vector<int> & flatdctMatrix){
    flatdctMatrix.clear();

    for(auto & pair : RLEBlock){

        for(int i= 0; i<pair.first; i++){
            flatdctMatrix.push_back(pair.second);
        }

    }
}

//permet de décompresser une liste de blocs compressés en RLE
void decompressBlocksRLE(const std::vector<std::pair<int,int>> & encodedRLE, std::vector<Block> & blocks,  const std::vector<std::vector<int>> & quantificationMatrix, CompressionSettings * settings = nullptr){

    //std::cout << " encodedRLE size " << encodedRLE.size() << std::endl;

    int currentRLEIndex = 0;
    int currentBlockProgress = 0;
    
    while(currentRLEIndex < encodedRLE.size()){
        Block currentBlock(8);
        std::vector<std::pair<int,int>> currentBlockRLE;

        //tant que on a pas fini le bloc on lit des RLE
        while(currentBlockProgress < 64){


            currentBlockRLE.push_back(encodedRLE[currentRLEIndex]);
            
            
            //std::cout << " blocks (" << encodedRLE[currentRLEIndex].first << ", "<< encodedRLE[currentRLEIndex].second << ")" <<std::endl;
            currentBlockProgress += encodedRLE[currentRLEIndex].first;
            
            //std::cout << "block progress" << currentBlockProgress << std::endl;

            currentRLEIndex++;

            //if(currentRLEIndex >= encodedRLE.size()) {std::cout << "ERROR" << std::endl; return;}
            
        }

        //on inverse les operations de la compression

  

        RLEDecompression(currentBlockRLE, currentBlock.flatDctMatrix);

        unflattenZigZag(currentBlock);

        //inverse_quantification(currentBlock);

        if(settings != nullptr){
            switch ((settings->transformationType)) {
                case DCTTRANSFORM:
                    inverse_quantification_better(currentBlock, quantificationMatrix, settings->QuantizationFactor);
                    IDCT(currentBlock);
                    break;
                case DCTIVTRANSFORM:
                    inverse_quantification_uniforme(currentBlock, settings->QuantizationFactor);
                    DCT_IV(currentBlock, false, 8);
                    break;
                case INTDCTTRANSFORM:
                    INTIDCT( currentBlock);
                    break;
                default:
                    std::cerr << "Unknown transformation type!" << std::endl;
                    break;
            }
        } else {
            inverse_quantification(currentBlock);
            IDCT(currentBlock);
        }
        

        blocks.push_back(currentBlock);
        currentBlockProgress = 0;
    }

}