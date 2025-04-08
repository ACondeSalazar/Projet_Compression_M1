#pragma once

#include <vector>
#include <unordered_map>
#include <utility>

#include <string>
#include <stdint.h>
#include <utility>
#include "Utils.h"

#include "LZ77.h"

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

struct triplet_hash {
    template <typename T1, typename T2, typename T3>
    size_t operator () (const std::tuple<T1, T2, T3> & triplet) const {
        size_t hash1 = std::hash<T1>{}(std::get<0>(triplet));
        size_t hash2 = std::hash<T2>{}(std::get<1>(triplet));
        size_t hash3 = std::hash<T3>{}(std::get<2>(triplet));

        // Better combination of the hashes, using prime numbers and multiplication
        size_t hash = hash1;
        hash = hash * 31 + hash2;  // 31 is a small prime number, commonly used for hash combining
        hash = hash * 31 + hash3;  // Ensures better mixing of all three hash components

        return hash;
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
                         const string& filename, CompressionSettings & settings);

void readHuffmanEncoded(const string& filename,
                        vector<huffmanCodeSingle>& codeTable,
                        vector<pair<int, int>>& RLEData,
                        int & width, int & height, int & downSampledWidth, int & downSampledHeight,
                        int & channelYSize, int & channelCbSize, int & channelCrSize, CompressionSettings & settings);



//---------------LZ77------------------

struct huffmanCodeSingleLZ77{

    tuple<int,int,int> triplet; //offset, length, next
    int code; //le codage 
    int length; //la longueur du code

};

struct TreeNodeLZ77{

    tuple<int,int,int> triplet; //offset, length, next

    float frequency;

    TreeNodeLZ77 * left = nullptr;

    TreeNodeLZ77 * right = nullptr;

    TreeNodeLZ77(tuple<int,int,int> in_triplet, float in_frequency): triplet(in_triplet), frequency(in_frequency){}

};

void sortTreeLZ77(vector<TreeNodeLZ77 *> &huffmanTree);

void initHuffmanTreeLZ77(unordered_map<tuple<int, int, int>, int, triplet_hash> &frequencyTable, int nb_elements, vector<TreeNodeLZ77 *> &huffmanTree);

void buildHuffmanTreeLZ77(vector<TreeNodeLZ77 *> & huffmanTree);

void getEncodingRecursiveLZ77(TreeNodeLZ77 * node, int code, int length, vector<huffmanCodeSingleLZ77> &codeTable);

void HuffmanEncodingLZ77(vector<LZ77Triplet> & RLEData, vector<huffmanCodeSingleLZ77> &  codeTable);

void writeHuffmanEncodedLZ77(vector<LZ77Triplet>& LZ77Data,
                             vector<huffmanCodeSingleLZ77>& codeTable,
                             int width, int height, int downSampledWidth, int downSampledHeight,
                             int channelYSize, int channelCbSize, int channelCrSize,
                             const string& filename, CompressionSettings & settings);

void readHuffmanEncodedLZ77(const string& filename, 
                            vector<huffmanCodeSingleLZ77>& codeTable,
                            vector<LZ77Triplet>& LZ77Data,
                            int & width, int & height, int & downSampledWidth, int & downSampledHeight,
                            int & channelYSize, int & channelCbSize, int & channelCrSize, CompressionSettings & settings);