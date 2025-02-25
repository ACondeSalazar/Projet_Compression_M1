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
            if(i == flattenedBlock.size()-1){ //on gère le dernier element au cas ou
                RLEBlock.push_back({counterValue,currentValue});
            }
        }else{
            RLEBlock.push_back({counterValue,currentValue});
            counterValue = 1;
        }

        currentValue = flattenedBlock[i];
    }
    
}

void RLEDecompression(std::vector<std::pair<int,int>> & RLEBlock, std::vector<int> & flatdctMatrix){
    flatdctMatrix.clear();

    for(auto & pair : RLEBlock){
        for(int i= 0; i<pair.first; i++){
            flatdctMatrix.push_back(pair.second);
        }
    }
}