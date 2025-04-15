#include "Utils.h"

void DCT(Block & block){

    double PI = 3.14159265358979323846;
    //PI = M_PI;


    int N = block.data.size();

    double mul = PI / (2 * N); // petite optimisation
    const double inv_sqrt2 = 1.0 / sqrt(2.0); //micro opti

    for (int u = 0; u < N; u++){
        for (int v = 0; v < N; v++){ //pour chaque fréquence
            double sum = 0;
            for (int x = 0; x < N; x++){
                for (int y = 0; y < N; y++){

                    sum += block.data[x][y] * cos((2 * x + 1) * u * mul) * cos((2 * y + 1) * v * mul);
                
                }
            }
            double Cu = (u == 0) ? inv_sqrt2 : 1;
            double Cv = (v == 0) ? inv_sqrt2 : 1;
            block.dctMatrix[u][v] = 0.25 * Cu * Cv * sum;
        }
    }
}

void IDCT(Block & block) {
    double PI = 3.14159265358979323846;
    
    int N = block.dctMatrix.size();

    double mul = PI / (2 * N); // petite optimisation
    const double inv_sqrt2 = 1.0 / sqrt(2.0); //micro opti

    for (int x = 0; x < N; x++) {
        for (int y = 0; y < N; y++) { // reconstruct pixel values
            double sum = 0;
            for (int u = 0; u < N; u++) {
                for (int v = 0; v < N; v++) {
                    
                    double Cu = (u == 0) ? inv_sqrt2 : 1.0;
                    double Cv = (v == 0) ? inv_sqrt2 : 1.0;
                    
                    sum += Cu * Cv * block.dctMatrix[u][v] * cos((2 * x + 1) * u * mul) * cos((2 * y + 1) * v * mul);
                }
            }
            block.data[x][y] = 0.25 * sum;
        }
    }
}

void DCT_IV(Block &block, bool forward, unsigned int N){
    double PI = 3.14159265358979323846;
    double scale = 2.0 / N;

    if (forward) {
        block.data.resize(N, std::vector<int>(N, 0));
    } else {
        block.dctMatrix.resize(N, std::vector<double>(N, 0));
    }

    for (int u = 0; u < N; u++) {
        for (int v = 0; v < N; v++) {
            double sum = 0.0;
            for (int x = 0; x < N; x++) {
                for (int y = 0; y < N; y++) {
                    double value = forward ? block.data[x][y] : block.dctMatrix[x][y];
                    sum += value * std::cos((PI / N) * (x +0.5) * (u + 0.5)) * std::cos((PI/ N) * (y + 0.5) * (v + 0.5));
                }
            }
            if (forward) {
                block.dctMatrix[u][v] = scale * sum;
            } else {
                block.data[u][v] = scale * sum;
            }
        }
    }

  /*   if (forward) {
        std::cout << "Forward DCT-IV Transformation" << std::endl;
    } else {
        std::cout << "Inverse DCT-IV Transformation" << std::endl;
    }
    for (int x = 0; x < N; x++) {
        for (int y = 0; y < N; y++) {
            std::cout << (forward ? block.dctMatrix[x][y] : block.data[x][y]) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << std::endl; */
}

void INTDCT(Block &block){}
void INTIDCT(Block &block){}

//transformation ondelette


void apply53(std::vector<std::vector<int>>& data) {
    int height = data.size();
    int width = data[0].size();

    // Appliquer la transformée sur les lignes
    for (int y = 0; y < height; ++y) {
        // Étape de prédiction (lifting)
        for (int x = 1; x < width - 1; x += 2) {
            data[y][x] -= (data[y][x - 1] + data[y][x + 1] + 1) / 2;
        }
        // Étape de mise à jour
        for (int x = 0; x < width; x += 2) {
            if (x == 0) {
                data[y][x] += (data[y][x + 1] + 1) / 2;
            } else if (x == width - 1) {
                data[y][x] += (data[y][x - 1] + 1) / 2;
            } else {
                data[y][x] += (data[y][x - 1] + data[y][x + 1] + 2) / 4;
            }
        }
    }

    // Appliquer la transformée sur les colonnes
    for (int x = 0; x < width; ++x) {
        // Étape de prédiction (lifting)
        for (int y = 1; y < height - 1; y += 2) {
            data[y][x] -= (data[y - 1][x] + data[y + 1][x] + 1) / 2;
        }
        // Étape de mise à jour
        for (int y = 0; y < height; y += 2) {
            if (y == 0) {
                data[y][x] += (data[y + 1][x] + 1) / 2;
            } else if (y == height - 1) {
                data[y][x] += (data[y - 1][x] + 1) / 2;
            } else {
                data[y][x] += (data[y - 1][x] + data[y + 1][x] + 2) / 4;
            }
        }
    }
}

// Appliquer la transformée inverse en ondelettes 5/3 sur une matrice 2D
void inverse53(std::vector<std::vector<int>>& data) {
    int height = data.size();
    int width = data[0].size();

    // Appliquer l'inverse sur les colonnes
    for (int x = 0; x < width; ++x) {
        // Étape de mise à jour inverse
        for (int y = 0; y < height; y += 2) {
            if (y == 0) {
                data[y][x] -= (data[y + 1][x] + 1) / 2;
            } else if (y == height - 1) {
                data[y][x] -= (data[y - 1][x] + 1) / 2;
            } else {
                data[y][x] -= (data[y - 1][x] + data[y + 1][x] + 2) / 4;
            }
        }
        // Étape de prédiction inverse
        for (int y = 1; y < height - 1; y += 2) {
            data[y][x] += (data[y - 1][x] + data[y + 1][x] + 1) / 2;
        }
    }

    // Appliquer l'inverse sur les lignes
    for (int y = 0; y < height; ++y) {
        // Étape de mise à jour inverse
        for (int x = 0; x < width; x += 2) {
            if (x == 0) {
                data[y][x] -= (data[y][x + 1] + 1) / 2;
            } else if (x == width - 1) {
                data[y][x] -= (data[y][x - 1] + 1) / 2;
            } else {
                data[y][x] -= (data[y][x - 1] + data[y][x + 1] + 2) / 4;
            }
        }
        // Étape de prédiction inverse
        for (int x = 1; x < width - 1; x += 2) {
            data[y][x] += (data[y][x - 1] + data[y][x + 1] + 1) / 2;
        }
    }
}

void applyWaveletTransform53ToTiles(std::vector<Tile>& tiles) {
    for (auto& tile : tiles) {
        apply53(tile.data);
    }
}

void inverseWaveletTransform53ToTiles(std::vector<Tile>& tiles) {
    for (auto& tile : tiles) {
        inverse53(tile.data);
    }
}


//------ quantification


void quantification (Block & block){
    for (int i = 0; i < block.dctMatrix.size(); i++){
        for (int j = 0; j < block.dctMatrix[0].size(); j++){

            block.dctMatrix[i][j] = (int)(block.dctMatrix[i][j] / quantificationMatrix2[i][j]);
        
        }
    }
}

void inverse_quantification (Block & block){
    for (int i = 0; i < block.dctMatrix.size(); i++){
        for (int j = 0; j < block.dctMatrix[0].size(); j++){

            block.dctMatrix[i][j] = (int)(block.dctMatrix[i][j] * quantificationMatrix2[i][j]);
        
        }
    }
}



void quantification_uniforme (Block & block, int quantificationFactor = 1){
    for (int i = 0; i < block.dctMatrix.size(); i++){
        for (int j = 0; j < block.dctMatrix[0].size(); j++){

            block.dctMatrix[i][j] = (int)(block.dctMatrix[i][j] / (quantificationMatrixUniform[i][j] * quantificationFactor));
        
        }
    }
}

void inverse_quantification_uniforme (Block & block, int quantificationFactor = 1){
    for (int i = 0; i < block.dctMatrix.size(); i++){
        for (int j = 0; j < block.dctMatrix[0].size(); j++){

            block.dctMatrix[i][j] = (int)(block.dctMatrix[i][j] * (quantificationMatrixUniform[i][j] * quantificationFactor));
        
        }
    }
}

float getScaling(int quantificationFactor){

    float scale;
    if (quantificationFactor < 50){
        scale = 5000.0 / quantificationFactor;
    } else {
        scale = 200 - 2 * quantificationFactor;
    }

    return scale;

}



void quantification_better (Block & block, const std::vector<std::vector<int>> & matrix, int quantificationFactor){

    float scale = getScaling(quantificationFactor);

    for (int i = 0; i < block.dctMatrix.size(); i++){
        for (int j = 0; j < block.dctMatrix[0].size(); j++){

            int quantif = matrix[i][j];

            quantif = std::max((int)((quantif * scale + 50) / 100.0), 1);

            block.dctMatrix[i][j] = (int)(block.dctMatrix[i][j] / quantif);
        
        }
    }

}

void inverse_quantification_better (Block & block, const std::vector<std::vector<int>> & matrix, int quantificationFactor){

    float scale = getScaling(quantificationFactor);

    for (int i = 0; i < block.dctMatrix.size(); i++){
        for (int j = 0; j < block.dctMatrix[0].size(); j++){

            int quantif = matrix[i][j];
            quantif = std::max((int)((quantif * scale + 50) / 100.0), 1);

            block.dctMatrix[i][j] = (int)(block.dctMatrix[i][j] * quantif);
        
        }
    }

}






//quantification ondelette

std::pair<int, int> getQuantificationStep(int quantificationFactor) {

    float scale = getScaling(quantificationFactor);

    int stepLow = 1;
    int stepHigh = 1;

    if (quantificationFactor >= 100) {
        stepLow = 2;
    } else if (quantificationFactor >= 80) {
        stepLow = 3;
    } else if (quantificationFactor >= 60) {
        stepLow = 4;
    } else if (quantificationFactor >= 50) {
        stepLow = 6;
    }else if (quantificationFactor >= 40) {
        stepLow = 8;
    } else if (quantificationFactor >= 30) {
        stepLow = 16;
    } else {
        stepLow = 32;
    }


    stepHigh = stepLow;

    return {stepLow, stepHigh};

}

void quantificationuniforme(Tile& tile, int quantizationStepLow, int quantizationStepHigh) {
    int height = tile.data.size();
    int width = tile.data[0].size();

    //ll en haut a gauche
    int llHeight = height / 2;
    int llWidth = width / 2;

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            if (i < llHeight && j < llWidth) {
                // Sous-bande LL
                tile.data[i][j] =(int) ((float)tile.data[i][j] + (float)quantizationStepLow / 2) / quantizationStepLow;
            } else {
                // Sous-bandes LH, HL, HH
                tile.data[i][j] = (int) ((float)tile.data[i][j] +(float) quantizationStepHigh / 2) / quantizationStepHigh;
            }
        }
    }
}

void inverseQuantificationuniforme(Tile& tile, int quantizationStepLow, int quantizationStepHigh) {
    int height = tile.data.size();
    int width = tile.data[0].size();

    int llHeight = height / 2;
    int llWidth = width / 2;

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            if (i < llHeight && j < llWidth) {
                // Sous-bande LL
                tile.data[i][j] *= quantizationStepLow;
            } else {
                // Sous-bandes LH, HL, HH
                tile.data[i][j] *= quantizationStepHigh;
            }
        }
    }
}



void quantificationForAllTiles(std::vector<Tile> & tiles) {
    for (auto& tile : tiles) {
        quantificationuniforme(tile, 2,4);  // ici on controle le taux de compression -> 1,2 tres bon -> 2,4 bon -> 4,8 moyen-> 8, 16 mauvais
    }
}

void inverse_quantificationForAllTiles(std::vector<Tile> & tiles) {
    for (auto& tile : tiles) {
        inverseQuantificationuniforme(tile,2,4); // ici on controle le taux de compression -> 1,2 tres bon -> 2,4 bon -> 4,8 moyen-> 8, 16 mauvais
    }
}