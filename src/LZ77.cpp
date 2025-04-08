#include "LZ77.h"

#include "Utils.h"
#include <cstddef>
#include <vector>

void LZ77Compression(std::vector<int> & data, std::vector<LZ77Triplet> & compressedData, int windowSize) {
    compressedData.clear();
    int dataSize = data.size();

    int index = 0;
    while(index < dataSize){

        int matchOffset = 0;
        int matchLength = 0;
        int next = 0;
        
        int start = std::max(0, index - windowSize);
        int end = index;

        for(int i = start; i < end; i++){
            int length = 0;
            while(length < windowSize && (index + length) < dataSize &&  data[i + length] == data[index + length]){
                length+= 1;
            }
            if(length > matchLength){
                matchOffset = index - i;
                matchLength = length;

                if (index + length < dataSize){
                    next = data[index + length];
                }
                
            }
        }

        if(matchLength == 0){
            matchOffset = 0;
            matchLength = 0;
            next = data[index];
            compressedData.push_back(LZ77Triplet(matchOffset, matchLength, next));
            index += 1;
        }else {

            if(index + matchLength < dataSize) {
                compressedData.push_back(LZ77Triplet(matchOffset, matchLength, next));
                index += matchLength + 1;
            } else {
                compressedData.push_back(LZ77Triplet(matchOffset, matchLength, 0));
                index += matchLength;
            }
        }
    }
    
}

void LZ77Decompression(std::vector<LZ77Triplet> & compressedData, std::vector<int> & data, int expectedSize) {
    data.clear();

    for (LZ77Triplet & triplet : compressedData) {
        int offset = triplet.offset;
        int length = triplet.length;
        int next = triplet.next;

        int start = data.size() - offset;
        for (int j = 0; j < length && data.size() < expectedSize; j++) {
            data.push_back(data[start + j]);
        }

        if (data.size() < expectedSize) {
            data.push_back(next);
        }
    }
}


/* void decompressTilesLZ77(std::vector<LZ77Triplet> &tilesYLZ77, std::vector<Tile> &tilesY, int tileWidth, int tileHeight){

        std::vector<int> decompressedData;

        LZ77Decompression(tilesYLZ77, decompressedData, tileWidth * tileHeight);
        std::cout << "Decompressed data size: " << decompressedData.size() << std::endl;
        if (decompressedData.size() != tileWidth * tileHeight) {
            std::cout << "Expected size: " << (tileWidth * tileHeight) << ", Decompressed size: " << decompressedData.size() << std::endl;
            std::cerr << "Decompressed data size does not match expected size!" << std::endl;
            return;
        }

        int index = 0;
        while (index < decompressedData.size()) {
            Tile newTile(tileWidth, tileHeight, 0, 0);
            for (int i = 0; i < tileWidth; i++) {
                for (int j = 0; j < tileHeight; j++) {

                    if (index < decompressedData.size()) {
                        newTile.data[i][j] = decompressedData[index++];
                    } else {
                        std::cout << "out of bound en decompressant tiles lz77" << std::endl;
                    }

                }
            }
            tilesY.push_back(newTile);
            std::cout << "Decompressed tile added with dimensions: " << tileWidth << "x" << tileHeight << std::endl;
        }

} */

void decompressTilesLZ77(std::vector<LZ77Triplet> &tilesYLZ77, std::vector<Tile> &tilesY, int tileWidth, int tileHeight) {
    tilesY.clear();

    std::vector<int> decompressedData;
    int expectedSize = tileWidth * tileHeight;

    LZ77Decompression(tilesYLZ77, decompressedData, expectedSize);

    
}
