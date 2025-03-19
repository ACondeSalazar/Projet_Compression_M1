#pragma once

#include <vector>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <iostream>
#include <bitset>
#include <stdint.h>
#include <utility>

using namespace std;

// trouvé sur https://stackoverflow.com/questions/32685540/why-cant-i-compile-an-unordered-map-with-a-pair-as-key
//permet d'utiliser des std::pair comme clés d'une unordered_map
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};


struct TreeNode {

    pair<int,int> rlePair;
    float frequency;
    TreeNode * left = nullptr;
    TreeNode * right = nullptr;

    TreeNode(pair<int,int> in_rlePair, float in_frequency): rlePair(in_rlePair), frequency(in_frequency){}

};

struct huffmanCodeSingle{

    pair<int,int> rlePair; //la paire frequence, valeur_pixel
    int code; //le codage 
    int length; //la longueur du code

};


bool pair_equals(vector<int> pair1, vector<int> pair2);

void sortTree(vector<TreeNode*> & huffmanTree);

void initHuffmanTree (unordered_map<pair<int,int>, int, pair_hash> & frequencyTable,int nb_elements ,vector<TreeNode*> & huffmanTree);

void buildHuffmanTree(vector<TreeNode*> & huffmanTree);

void getEncodingRecursive(TreeNode* node, int code, int length, vector<huffmanCodeSingle>& codeTable);

void HuffmanEncoding(vector<pair<int,int>> & RLEData, vector<huffmanCodeSingle> &  codeTable);

void writeHuffmanEncoded(vector<pair<int, int>>& RLEData,
                         vector<huffmanCodeSingle>& codeTable,
                         int width, int height, int downSampledWidth, int downSampledHeight,
                         int channelYSize, int channelCbSize, int channelCrSize,
                         const string& filename);

void readHuffmanEncoded(const string& filename,
                        vector<huffmanCodeSingle>& codeTable,
                        vector<pair<int, int>>& RLEData,
                        int & width, int & height, int & downSampledWidth, int & downSampledHeight,
                        int & channelYSize, int & channelCbSize, int & channelCrSize);



