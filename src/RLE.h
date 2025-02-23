#pragma once
#include <vector>
#include "Utils.h"

void RLECompression(std::vector<int> & flattenedBlock, std::vector<std::vector<int>> & RLEBlock){
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