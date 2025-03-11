#pragma once
#include <vector>
#include "Utils.h"

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