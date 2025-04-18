#include "Huffman.h"
#include "LZ77.h"
#include "Utils.h"
#include <queue>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <iostream>
#include <stdint.h>
#include <fstream>

using namespace std;



bool pair_equals(vector<int> pair1, vector<int> pair2){

    return (pair1[0] == pair2[0]) && (pair1[1] == pair2[1]);

}

//on trie les éléments par fréquence croissante peut etre trouver plus rapide ?
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



void getEncodingRecursive(TreeNode* node, int code, int length, vector<huffmanCodeSingle>& codeTable) {
    if (node == nullptr) {
        return;
    }

    //si le noeud est une feuille on le remplie avec le code
    if (node->left == nullptr && node->right == nullptr) {
        huffmanCodeSingle newCode;
        newCode.rlePair = node->rlePair;
        newCode.code = code;
        newCode.length = length;

        codeTable.push_back(newCode);
    }

    getEncodingRecursive(node->left, (code << 1), length + 1, codeTable); // (code << 1) décalage de 1 bit pour rajouter un 0
    getEncodingRecursive(node->right, (code << 1) | 1, length + 1, codeTable); // (code << 1) | 1 decalage de 1 bit et ajoute 1 a la fin 
}

//
void deleteTreeRecursive(TreeNode * node){

    if(node == nullptr){
        return;
    }else{
        deleteTreeRecursive(node->right);
        deleteTreeRecursive(node->left);
        delete node;
    }

}


void HuffmanEncoding(vector<pair<int,int>> & RLEData, vector<huffmanCodeSingle> &  codeTable){

    //std::cout << "RLE Huffman Encoding" << std::endl;
    //std::cout << RLEData.size() << " RLE pairs to encode." << std::endl;

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


    deleteTreeRecursive(huffmanTree[0]);
}

void writeHuffmanEncoded(vector<pair<int, int>>& RLEData,
                         vector<huffmanCodeSingle>& codeTable,
                         int width, int height, int downSampledWidth, int downSampledHeight,
                         int channelYSize, int channelCbSize, int channelCrSize,
                         const string& filename, CompressionSettings & settings) {
    ofstream outFile(filename, ios::binary);
    if (!outFile) {
        cerr << "Error opening file for writing huffman encoded data!" << endl;
        return;
    }


    outFile.write(reinterpret_cast<const char*>(&settings), sizeof(settings));

    //on ecrit la taille de l'image
    outFile.write(reinterpret_cast<const char*>(&width), sizeof(width));
    outFile.write(reinterpret_cast<const char*>(&height), sizeof(height));
    outFile.write(reinterpret_cast<const char*>(&downSampledWidth), sizeof(downSampledWidth));
    outFile.write(reinterpret_cast<const char*>(&downSampledHeight), sizeof(downSampledHeight));

    outFile.write(reinterpret_cast<const char*>(&channelYSize), sizeof(channelYSize)); //le nombre de pair RLE (fréquence,pixel) pour le canal Y
    outFile.write(reinterpret_cast<const char*>(&channelCbSize), sizeof(channelCbSize)); //pareil pour les canaux Cb et Cr
    outFile.write(reinterpret_cast<const char*>(&channelCrSize), sizeof(channelCrSize));
    
    //on ecrit la taille de la table
    int tableSize = static_cast<int>(codeTable.size());
    outFile.write(reinterpret_cast<const char*>(&tableSize), sizeof(tableSize));
    
    //on ecrit les codes présents dans la table
    for (const auto& entry : codeTable) {
        outFile.write(reinterpret_cast<const char*>(&entry.rlePair), sizeof(entry.rlePair));
        outFile.write(reinterpret_cast<const char*>(&entry.code), sizeof(entry.code));
        outFile.write(reinterpret_cast<const char*>(&entry.length), sizeof(entry.length));
    }
    
    //on ecrit le nombre de RLE
    int numRLE = static_cast<int>(RLEData.size());
    outFile.write(reinterpret_cast<const char*>(&numRLE), sizeof(numRLE));

    //hashmap RLE{frequence, valeur}-> {code, longueur code} 
    unordered_map<pair<int, int>, pair<int, int>, pair_hash> codeMap; //bien plus rapide pour la lecture après
    for (auto & entry : codeTable) {
        codeMap[entry.rlePair] = {entry.code, entry.length};
    }
    
    //on ecrit les données RLE encodées
    uint8_t bitBuffer = 0;
    int bitsInBuffer = 0;
    
    //ChatGPT, permet de lire un seul bit a la fois
    auto flushBuffer = [&]() {
        if (bitsInBuffer > 0) {
            // Shift left to pad with zeros on the right.
            bitBuffer <<= (8 - bitsInBuffer);
            outFile.write(reinterpret_cast<const char*>(&bitBuffer), 1);
            bitBuffer = 0;
            bitsInBuffer = 0;
        }
    };

    for (auto & rle : RLEData) {

        if (auto search = codeMap.find(rle); search != codeMap.end()){
            //search first = {frequence, pair}, search second = {code, longueur code}
            int code = search->second.first;
            int length = search->second.second;

            for (int i = length - 1; i >= 0; i--) {
                int bit = (code >> i) & 1;
                bitBuffer = (bitBuffer << 1) | bit;
                bitsInBuffer++;
                if (bitsInBuffer == 8) {
                    outFile.write(reinterpret_cast<const char*>(&bitBuffer), 1);
                    bitBuffer = 0;
                    bitsInBuffer = 0;
                }
            }

        }else{
            std::cout << "pas de code valide pour le rle : " << rle.first << ", " << rle.second << std::endl;
        }


    }

    flushBuffer();
    
    outFile.close();
}





void readHuffmanEncoded(const string& filename,
                        vector<huffmanCodeSingle>& codeTable,
                        vector<pair<int, int>>& RLEData,
                        int & width, int & height, int & downSampledWidth, int & downSampledHeight,
                        int & channelYSize, int & channelCbSize, int & channelCrSize, CompressionSettings & settings) {
    ifstream inFile(filename, ios::binary);
    if (!inFile) {
        cerr << "Error opening file for reading!" << endl;
        return;
    }

    inFile.read(reinterpret_cast<char*>(&settings), sizeof(settings));

    //On lit la taille de l'image
    inFile.read(reinterpret_cast<char*>(&width), sizeof(width));
    inFile.read(reinterpret_cast<char*>(&height), sizeof(height));
    inFile.read(reinterpret_cast<char*>(&downSampledWidth), sizeof(downSampledWidth));
    inFile.read(reinterpret_cast<char*>(&downSampledHeight), sizeof(downSampledHeight));

    //on lit la taille des canaux (encodés en RLE)
    inFile.read(reinterpret_cast<char*>(&channelYSize), sizeof(channelYSize));
    inFile.read(reinterpret_cast<char*>(&channelCbSize), sizeof(channelCbSize));
    inFile.read(reinterpret_cast<char*>(&channelCrSize), sizeof(channelCrSize));
    
    //On lit la taille de la table
    int tableSize = 0;
    inFile.read(reinterpret_cast<char*>(&tableSize), sizeof(tableSize));
    
    //hashmap {code, longueur code} -> RLE{frequence, valeur}
    unordered_map<pair<int, int>, pair<int, int>, pair_hash> codeMap; //bien plus rapide pour la lecture après

    //On remplit la table
    codeTable.clear();
    for (int i = 0; i < tableSize; i++) {
        huffmanCodeSingle entry;
        inFile.read(reinterpret_cast<char*>(&entry.rlePair), sizeof(entry.rlePair));
        inFile.read(reinterpret_cast<char*>(&entry.code), sizeof(entry.code));
        inFile.read(reinterpret_cast<char*>(&entry.length), sizeof(entry.length));
        codeTable.push_back(entry);

        codeMap[{entry.code, entry.length}] = entry.rlePair;
    }

    
    //On lit le nombre de RLE
    int numRLE = 0;
    inFile.read(reinterpret_cast<char*>(&numRLE), sizeof(numRLE));
    
    //on lit bit par bit
    RLEData.clear();
    uint8_t byte = 0;
    int bitsLeft = 0;
    
    //chatGPT, permet de lire un seul bit a la fois
    auto readBit = [&]() -> int {
        if (bitsLeft == 0) {
            if (!inFile.read(reinterpret_cast<char*>(&byte), 1)) {
                return -1; // End of file.
            }
            bitsLeft = 8;
        }
        int bit = (byte >> (bitsLeft - 1)) & 1;
        bitsLeft--;
        return bit;
    };
    
    //on lit un bit et on regarde si il correspond a un code dans la table, sinon on lit un autre bit qu'on ajoute aux précédents.
    while (static_cast<int>(RLEData.size()) < numRLE) {
        int currentCode = 0;
        int currentLength = 0;
        bool matched = false;
        while (!matched) {
            int bit = readBit();
            if (bit == -1) {
                cerr << "Unexpected end of file during bit decoding." << endl;
                return;
            }
            currentCode = (currentCode << 1) | bit;
            currentLength++;
            
            //on cherche si le code est dans la hashmap
            if (auto search = codeMap.find({currentCode,currentLength}); search != codeMap.end()){
                RLEData.push_back(search->second); //1ere element la paire (code, longueur code), le deuxieme la paire rle
                matched = true;
            }


        }
    }
    
    inFile.close();
}



//=============================================HUFFMAN LZ77 VERSION=============================================================================

//comparaison entre 2 elements pour la priority queue
struct CompareNodeLZ77 {
    bool operator()(TreeNodeLZ77* node1, TreeNodeLZ77* node2) {
        return node1->frequency > node2->frequency;
    }
};


void initHuffmanTreeLZ77(unordered_map<tuple<int, int, int>, int, triplet_hash> &frequencyTable, int nb_elements, vector<TreeNodeLZ77 *> &huffmanTree){

    huffmanTree.clear();

    for (auto & elem : frequencyTable) {
        huffmanTree.push_back(new TreeNodeLZ77(elem.first, (float)elem.second / nb_elements));
    }

    sortTreeLZ77(huffmanTree);

}

void sortTreeLZ77(vector<TreeNodeLZ77 *> &huffmanTree){
    sort(huffmanTree.begin(), huffmanTree.end(), [](TreeNodeLZ77* node1, TreeNodeLZ77* node2) { return node1->frequency < node2->frequency; }); 
}

void buildHuffmanTreeLZ77(vector<TreeNodeLZ77*> & huffmanTree){
    int compteur = 0;

    std::priority_queue<TreeNodeLZ77 * , vector<TreeNodeLZ77 *>, CompareNodeLZ77> heap; // la proba la plus petite se trouve toujours en haut de la heap

    for(TreeNodeLZ77 * node : huffmanTree){
        heap.push(node);
    }

    while(heap.size() >1 ){
        //printf("iteration : %d\n", compteur);
        compteur++;

        TreeNodeLZ77 * leftNode = heap.top(); //on recup le plus petit
        heap.pop(); //on l'enleve de la heap

        TreeNodeLZ77 * rightNode = heap.top(); //pareil
        heap.pop();

        
        float additionFrequency = leftNode->frequency + rightNode->frequency;
        TreeNodeLZ77 * additionNode = new TreeNodeLZ77(tuple<int,int,int>(), additionFrequency); //le tuple n'importe pas ici
        additionNode->left = leftNode;
        additionNode->right = rightNode;

        heap.push(additionNode); //on rajoute le nouveau noeud


    }

    huffmanTree.clear();
    huffmanTree.push_back(heap.top());

}

void getEncodingRecursiveLZ77(TreeNodeLZ77 * node, int code, int length, vector<huffmanCodeSingleLZ77> &codeTable){
    if (node == nullptr) {
        return;
    }

    //si le noeud est une feuille on le remplie avec le code
    if (node->left == nullptr && node->right == nullptr) {
        huffmanCodeSingleLZ77 newCode;
        newCode.triplet = node->triplet;
        newCode.code = code;
        newCode.length = length;

        codeTable.push_back(newCode);
    }

    getEncodingRecursiveLZ77(node->left, (code << 1), length + 1, codeTable); // (code << 1) décalage de 1 bit pour rajouter un 0
    getEncodingRecursiveLZ77(node->right, (code << 1) | 1, length + 1, codeTable); // (code << 1) | 1 decalage de 1 bit et ajoute 1 a la fin 

}

void deleteTreeRecursiveLZ77(TreeNodeLZ77 * node){

    if(node == nullptr){
        return;
    }else{
        deleteTreeRecursiveLZ77(node->right);
        deleteTreeRecursiveLZ77(node->left);
        delete node;
    }

}

void HuffmanEncodingLZ77(vector<LZ77Triplet> & LZ77Data, vector<huffmanCodeSingleLZ77> &codeTable){

    //std::cout << "LZ77 Huffman Encoding" << std::endl;
    //std::cout << LZ77Data.size() << " LZ77 triplets to encode." << std::endl;

    //return;
    unordered_map<tuple<int,int, int>, int, triplet_hash> frequencyTable; 

    for (auto & triplet : LZ77Data) {
        frequencyTable[{triplet.offset, triplet.length, triplet.next}]++;
    }

    vector<TreeNodeLZ77*> huffmanTree;

    initHuffmanTreeLZ77(frequencyTable, LZ77Data.size(), huffmanTree);

    buildHuffmanTreeLZ77(huffmanTree);

    getEncodingRecursiveLZ77(huffmanTree[0], 0, 0, codeTable);

    deleteTreeRecursiveLZ77(huffmanTree[0]);

}


void writeHuffmanEncodedLZ77(vector<LZ77Triplet>& LZ77Data,
                              vector<huffmanCodeSingleLZ77>& codeTable,
                              int width, int height, int downSampledWidth, int downSampledHeight,
                              int channelYSize, int channelCbSize, int channelCrSize,
                              const string& filename, CompressionSettings & settings) {
    
                            
    ofstream outFile(filename, ios::binary);
    if (!outFile) {
        cerr << "Error opening file for writing huffman encoded data!" << endl;
        return;
    }


    outFile.write(reinterpret_cast<const char*>(&settings), sizeof(settings));

    //on ecrit la taille de l'image
    outFile.write(reinterpret_cast<const char*>(&width), sizeof(width));
    outFile.write(reinterpret_cast<const char*>(&height), sizeof(height));
    outFile.write(reinterpret_cast<const char*>(&downSampledWidth), sizeof(downSampledWidth));
    outFile.write(reinterpret_cast<const char*>(&downSampledHeight), sizeof(downSampledHeight));

    outFile.write(reinterpret_cast<const char*>(&channelYSize), sizeof(channelYSize)); //le nombre de triplet LZ77 pour le canal Y
    outFile.write(reinterpret_cast<const char*>(&channelCbSize), sizeof(channelCbSize)); //pareil pour les canaux Cb et Cr
    outFile.write(reinterpret_cast<const char*>(&channelCrSize), sizeof(channelCrSize));
    
    //on ecrit la taille de la table
    int tableSize = static_cast<int>(codeTable.size());
    outFile.write(reinterpret_cast<const char*>(&tableSize), sizeof(tableSize));
    
    //on ecrit les codes présents dans la table
    for (const auto& entry : codeTable) {
        outFile.write(reinterpret_cast<const char*>(&entry.triplet), sizeof(entry.triplet));
        outFile.write(reinterpret_cast<const char*>(&entry.code), sizeof(entry.code));
        outFile.write(reinterpret_cast<const char*>(&entry.length), sizeof(entry.length));
    }
    
    //on ecrit le nombre de triplet
    int numTriplet = static_cast<int>(LZ77Data.size());
    outFile.write(reinterpret_cast<const char*>(&numTriplet), sizeof(numTriplet));

    //hashmap LZ77{offset, length, next}-> {code, longueur code} 
    unordered_map<tuple<int,int,int>, pair<int, int>, triplet_hash> codeMap; //bien plus rapide pour la lecture après

    for (auto & entry : codeTable) {
        codeMap[entry.triplet] = {entry.code, entry.length};
    }
    
    //on ecrit les données LZ77 encodées
    uint8_t bitBuffer = 0;
    int bitsInBuffer = 0;
    
    //ChatGPT, permet de lire un seul bit a la fois
    auto flushBuffer = [&]() {
        if (bitsInBuffer > 0) {
            // Shift left to pad with zeros on the right.
            bitBuffer <<= (8 - bitsInBuffer);
            outFile.write(reinterpret_cast<const char*>(&bitBuffer), 1);
            bitBuffer = 0;
            bitsInBuffer = 0;
        }
    };

    for (LZ77Triplet & triplet : LZ77Data) {

        if (auto search = codeMap.find({triplet.offset, triplet.length, triplet.next}); search != codeMap.end()){
            //search first = {frequence, pair}, search second = {code, longueur code}
            int code = search->second.first;
            int length = search->second.second;

            for (int i = length - 1; i >= 0; i--) {
                int bit = (code >> i) & 1;
                bitBuffer = (bitBuffer << 1) | bit;
                bitsInBuffer++;
                if (bitsInBuffer == 8) {
                    outFile.write(reinterpret_cast<const char*>(&bitBuffer), 1);
                    bitBuffer = 0;
                    bitsInBuffer = 0;
                }
            }

        }else{
            std::cout << "pas de code valide pour le triplet : (" << triplet.offset << ", " << triplet.length << ", " << triplet.next << ")" << std::endl;
        }


    }

    flushBuffer();
    
    outFile.close();
}

void readHuffmanEncodedLZ77(const string &filename,
     vector<huffmanCodeSingleLZ77> &codeTable,
      vector<LZ77Triplet> &LZ77Data,
       int &width, int &height, int &downSampledWidth, int &downSampledHeight,
        int &channelYSize, int &channelCbSize, int &channelCrSize,
         CompressionSettings &settings){



    ifstream inFile(filename, ios::binary);
    if (!inFile) {
        cerr << "Error opening file for reading!" << endl;
        return;
    }

    inFile.read(reinterpret_cast<char*>(&settings), sizeof(settings));

    //On lit la taille de l'image
    inFile.read(reinterpret_cast<char*>(&width), sizeof(width));
    inFile.read(reinterpret_cast<char*>(&height), sizeof(height));
    inFile.read(reinterpret_cast<char*>(&downSampledWidth), sizeof(downSampledWidth));
    inFile.read(reinterpret_cast<char*>(&downSampledHeight), sizeof(downSampledHeight));

    //on lit la taille des canaux (encodés en RLE)
    inFile.read(reinterpret_cast<char*>(&channelYSize), sizeof(channelYSize));
    inFile.read(reinterpret_cast<char*>(&channelCbSize), sizeof(channelCbSize));
    inFile.read(reinterpret_cast<char*>(&channelCrSize), sizeof(channelCrSize));

    //On lit la taille de la table
    int tableSize = 0;
    inFile.read(reinterpret_cast<char*>(&tableSize), sizeof(tableSize));

    //hashmap {code, longueur code} -> RLE{frequence, valeur}
    unordered_map<pair<int, int>, tuple<int, int,int>, pair_hash> codeMap; //bien plus rapide pour la lecture après

    //On remplit la table
    codeTable.clear();
    for (int i = 0; i < tableSize; i++) {
        huffmanCodeSingleLZ77 entry;
        inFile.read(reinterpret_cast<char*>(&entry.triplet), sizeof(entry.triplet));
        inFile.read(reinterpret_cast<char*>(&entry.code), sizeof(entry.code));
        inFile.read(reinterpret_cast<char*>(&entry.length), sizeof(entry.length));
        codeTable.push_back(entry);

        codeMap[{entry.code, entry.length}] = entry.triplet;
    }


    //On lit le nombre triplet LZ77
    int numLZ77 = 0;
    inFile.read(reinterpret_cast<char*>(&numLZ77), sizeof(numLZ77));

    //on lit bit par bit
    LZ77Data.clear();
    uint8_t byte = 0;
    int bitsLeft = 0;

    //chatGPT, permet de lire un seul bit a la fois
    auto readBit = [&]() -> int {
        if (bitsLeft == 0) {
            if (!inFile.read(reinterpret_cast<char*>(&byte), 1)) {
                return -1; // End of file.
            }
            bitsLeft = 8;
        }
        int bit = (byte >> (bitsLeft - 1)) & 1;
        bitsLeft--;
        return bit;
    };

    //on lit un bit et on regarde si il correspond a un code dans la table, sinon on lit un autre bit qu'on ajoute aux précédents.
    while (LZ77Data.size() < numLZ77) {
        int currentCode = 0;
        int currentLength = 0;
        bool matched = false;
        while (!matched) {
            int bit = readBit();
            if (bit == -1) {
                cerr << "Unexpected end of file during bit decoding." << endl;
                return;
            }
            currentCode = (currentCode << 1) | bit;
            currentLength++;
            
            //on cherche si le code est dans la hashmap
            if (auto search = codeMap.find({currentCode,currentLength}); search != codeMap.end()){
                LZ77Data.push_back(LZ77Triplet(search->second)); //1ere element la paire (code, longueur code), le deuxieme le triplet
                matched = true;
            }


        }
    }

    inFile.close();


}

