#pragma once

#include <vector>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <iostream>
#include <bitset>


using namespace std;

// trouvé sur https://stackoverflow.com/questions/32685540/why-cant-i-compile-an-unordered-map-with-a-pair-as-key
//permet d'utiliser des std::pair comme clés d'une unordered_map
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ h2; //pas terrible comme fonction de hashage
    }
};

struct TreeNode {

    pair<int,int> rlePair;
    float frequency;
    TreeNode * left = nullptr;
    TreeNode * right = nullptr;

    TreeNode(pair<int,int> in_rlePair, float in_frequency): rlePair(in_rlePair), frequency(in_frequency){}

};

bool pair_equals(vector<int> pair1, vector<int> pair2){

    return (pair1[0] == pair2[0]) && (pair1[1] == pair2[1]);

}

//on trie les éléments par fréquence croissante
void sortTree(vector<TreeNode*> & huffmanTree){
    sort(huffmanTree.begin(), huffmanTree.end(), [](TreeNode* node1, TreeNode* node2) { return node1->frequency < node2->frequency; }); 
}

void initHuffmanTree (unordered_map<pair<int,int>, int, pair_hash> & frequencyTable,int nb_elements ,vector<TreeNode*> & huffmanTree){
    huffmanTree.clear();

    for(auto & elem : frequencyTable){
        huffmanTree.push_back(new TreeNode(elem.first, (float)elem.second /nb_elements ));
    }

    sortTree(huffmanTree);

}

//construit l'arbre, faire initHuffmanTree() avant d'utiliser celle la 
void buildHuffmanTree(vector<TreeNode*> & huffmanTree){
    int compteur = 0;
    while(huffmanTree.size() >1 ){
        //printf("iteration : %d\n", compteur);
        compteur++;
        //on fusionne les 2 plus petites proba
        TreeNode * leftNode = huffmanTree[0];
        TreeNode * rightNode = huffmanTree[1];
        
        float additionFrequency = leftNode->frequency + rightNode->frequency;
        TreeNode * additionNode = new TreeNode(pair<int,int>(), additionFrequency); //la pair n'importe pas ici
        additionNode->left = leftNode;
        additionNode->right = rightNode;

        //on supprime les 2 probas
        huffmanTree.erase(huffmanTree.begin());
        huffmanTree.erase(huffmanTree.begin());

        huffmanTree.push_back(additionNode);

        sortTree(huffmanTree);

        /* for (const auto& node : huffmanTree) {
            cout << "RLE Pair: (" << node->rlePair.first << ", " << node->rlePair.second << "), Frequency: " << node->frequency << endl;
        } */

    }

}

struct huffmanCodeSingle{

    pair<int,int> rlePair; //la paire frequence, valeur_pixel
    int code; //le codage 
    int length; //la longueur du code

};

void getEncodingRecursive(TreeNode* node, int code, int length, vector<huffmanCodeSingle>& encodingList) {
    if (node == nullptr) {
        return;
    }

    //si le noeud est une feuille on le remplie avec le code
    if (node->left == nullptr && node->right == nullptr) {
        huffmanCodeSingle newCode;
        newCode.rlePair = node->rlePair;
        newCode.code = code;
        newCode.length = length;

        encodingList.push_back(newCode);
    }

    getEncodingRecursive(node->left, (code << 1), length + 1, encodingList); // (code << 1) décalage de 1 bit pour rajouter un 0
    getEncodingRecursive(node->right, (code << 1) | 1, length + 1, encodingList); // (code << 1) | 1 decalage de 1 bit et ajoute 1 a la fin 
}




void HuffmanEncoding(vector<pair<int,int>> & RLEData, vector<huffmanCodeSingle> &  codeTable){

    unordered_map<pair<int,int>, int, pair_hash> frequencyTable; //pas vraiment utile 
    for(auto & pair : RLEData){
        frequencyTable[pair]++;
    }

    vector<TreeNode*> huffmanTree;

    initHuffmanTree(frequencyTable, RLEData.size(), huffmanTree);

    /* printf("tree init \n");
    for (const auto& node : huffmanTree) {
        cout << "RLE Pair: (" << node->rlePair.first << ", " << node->rlePair.second << "), Frequency: " << node->frequency << endl;
    } */


    buildHuffmanTree(huffmanTree);

    /* printf("tree built \n");
    for (const auto& node : huffmanTree) {
        cout << "RLE Pair: (" << node->rlePair.first << ", " << node->rlePair.second << "), Frequency: " << node->frequency << endl;
    } */

    getEncodingRecursive(huffmanTree[0], 0, 0, codeTable);

    
    

}

void writeHuffmanEncoded(vector<pair<int, int>>& RLEData, vector<huffmanCodeSingle>& codeTable, const string& filename) {
    // Create an output file stream to write the data
    ofstream outFile(filename, ios::binary);

    if (!outFile) {
        cerr << "Error opening file for writing!" << endl;
        return;
    }

    // 1. Write the size of the code table (how many unique codes we have)
    int tableSize = codeTable.size();
    outFile.write(reinterpret_cast<const char*>(&tableSize), sizeof(tableSize));

    // 2. Write the Huffman code table (pixel value -> Huffman code, code length)
    for (const auto& entry : codeTable) {
        // Write the pixel value and its frequency (RLEPair)
        outFile.write(reinterpret_cast<const char*>(&entry.rlePair), sizeof(entry.rlePair));

        // Write the Huffman code and its length
        outFile.write(reinterpret_cast<const char*>(&entry.code), sizeof(entry.code));
        outFile.write(reinterpret_cast<const char*>(&entry.length), sizeof(entry.length));
    }

    // 3. Write the RLE data (pixel value -> frequency pair)
    for (const auto& rle : RLEData) {
        // Find the Huffman code corresponding to the pixel value
        bool found = false;
        for (const auto& entry : codeTable) {
            if (entry.rlePair.second == rle.second) { // Match pixel value
                // Write the Huffman code and its length
                outFile.write(reinterpret_cast<const char*>(&entry.code), sizeof(entry.code));
                outFile.write(reinterpret_cast<const char*>(&entry.length), sizeof(entry.length));
                found = true;
                break;
            }
        }

        if (!found) {
            cerr << "Error: No Huffman code found for pixel value " << rle.second << endl; // Notice rle.second for the pixel value
        }
    }

    outFile.close();
}





void readHuffmanEncoded(const string& filename, vector<huffmanCodeSingle>& codeTable, vector<pair<int, int>>& RLEData) {
    // Create an input file stream to read the data
    ifstream inFile(filename, ios::binary);

    if (!inFile) {
        cerr << "Error opening file for reading!" << endl;
        return;
    }

    // 1. Read the size of the code table
    int tableSize = 0;
    inFile.read(reinterpret_cast<char*>(&tableSize), sizeof(tableSize));

    // 2. Read the Huffman code table
    codeTable.clear(); // Clear any existing data in the table
    for (int i = 0; i < tableSize; ++i) {
        huffmanCodeSingle entry;
        
        // Read the rlePair (frequency, pixel value)
        inFile.read(reinterpret_cast<char*>(&entry.rlePair), sizeof(entry.rlePair));

        // Read the Huffman code and its length
        inFile.read(reinterpret_cast<char*>(&entry.code), sizeof(entry.code));
        inFile.read(reinterpret_cast<char*>(&entry.length), sizeof(entry.length));

        // Add the entry to the codeTable
        codeTable.push_back(entry);
    }

    // 3. Read the RLE data from the file
    RLEData.clear(); // Clear any existing RLE data
    while (inFile.peek() != EOF) {
        int code = 0, length = 0;
        inFile.read(reinterpret_cast<char*>(&code), sizeof(code));
        inFile.read(reinterpret_cast<char*>(&length), sizeof(length));

        // Find the corresponding pixel value and frequency from the codeTable
        bool found = false;
        for (const auto& entry : codeTable) {
            if (entry.code == code && entry.length == length) {
                // Match the code and length, add the RLE pair
                RLEData.push_back(entry.rlePair);
                found = true;
                break;
            }
        }

        if (!found) {
            cerr << "Error: No matching Huffman code found for the encoded data!" << endl;
        }
    }

    inFile.close();
}
