#include "FormatSamplingBlur.h"
#include <cmath>
#include <vector>
#include <algorithm>

//-- Changement de format couleur

void RGB_to_YCbCr(ImageBase & imIn, ImageBase & Y, ImageBase & Cb, ImageBase & Cr){

    for(int i = 0; i < imIn.getHeight(); i++){
        for(int j = 0; j < imIn.getWidth(); j++){
            
            int R = imIn[i*3][j*3 + 0];
            int G = imIn[i*3][j*3 + 1];
            int B = imIn[i*3][j*3 + 2];


            int Yij = static_cast<int>(0.299 * R + 0.587 * G + 0.114 * B);
            int Crij = static_cast<int>(0.5 * R - 0.418688 * G - 0.081312 * B ) + 128;
            int Cbij = static_cast<int>(-0.168736 * R - 0.331264 * G + 0.5 * B ) + 128;

            Y[i][j] = Yij;
            Cb[i][j] = Cbij;
            Cr[i][j] = Crij;

        }
    }

}

void YCbCr_to_RGB(ImageBase & imY, ImageBase & imCb, ImageBase & imCr, ImageBase & imOut) {
    for (int i = 0; i < imY.getHeight(); i++) {
        for (int j = 0; j < imY.getWidth(); j++) {
            int Yij = imY[i][j];
            int Cbij = imCb[i][j] - 128;
            int Crij = imCr[i][j] - 128;

            int R = static_cast<int>(Yij + 1.402 * Crij);
            int G = static_cast<int>(Yij - 0.344136 * Cbij - 0.714136 * Crij);
            int B = static_cast<int>(Yij + 1.772 * Cbij);

            imOut[i * 3][j * 3 + 0] = std::clamp(R, 0, 255);
            imOut[i * 3][j * 3 + 1] = std::clamp(G, 0, 255);
            imOut[i * 3][j * 3 + 2] = std::clamp(B, 0, 255);
        }
    }
}

void RGB_to_YCOCG(ImageBase & imIn, ImageBase & Y, ImageBase & CO, ImageBase & CG){

    for(int i = 0; i < imIn.getHeight(); i++){
        for(int j = 0; j < imIn.getWidth(); j++){
            
            int R = imIn[i*3][j*3 + 0];
            int G = imIn[i*3][j*3 + 1];
            int B = imIn[i*3][j*3 + 2];

            int COij = R - B;
            int tmp = B + COij / 2.0f;
            int CGij = G - tmp;
            int Yij = tmp + CGij / 2.0f;

            Y[i][j] = Yij;
            CO[i][j] = COij;
            CG[i][j] = CGij;

        }
    }

}

void YCOCG_to_RGB( ImageBase &Y, ImageBase &Co, ImageBase &Cg, ImageBase &imOut){

    for(int i = 0; i < Y.getHeight(); i++){
        for(int j = 0; j < Y.getWidth(); j++){

            int Yij = Y[i][j];
            int Coij = Co[i][j];
            int Cgij = Cg[i][j];

            int tmp = Yij - Cgij / 2.0f;
            int G = Cgij + tmp;
            int B = tmp - Coij / 2.0f;
            int R = B + Coij;

            imOut[i*3][j*3 + 0] = R;
            imOut[i*3][j*3 + 1] = G;
            imOut[i*3][j*3 + 2] = B;

        }
    }

}

//-------flou

void gaussianBlur(ImageBase &imIn, ImageBase &imOut) {
    int width = imIn.getWidth();
    int height = imIn.getHeight();

    // Filtre gaussien 3x3 (sigma ≈ 1)
    std::vector<std::vector<int>> kernel = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };

    int kernelSum = 16; // Somme du noyau gaussien

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int sum = 0;

            if(i == 0 || j == 0 || i == height - 1 || j == width -1){
                imOut[i][j] = imIn[i][j];
                continue;
            }

            for (int ki = -1; ki <= 1; ki++) {
                for (int kj = -1; kj <= 1; kj++) {
                    sum += imIn[i + ki][j + kj] * kernel[ki + 1][kj + 1];
                }
            }

            imOut[i][j] = sum / kernelSum;
        }
    }
}

void medianBlur(ImageBase &imIn, ImageBase &imOut){

    int width = imIn.getWidth();
    int height = imIn.getHeight();
    int kernelSize = 1; //  1 = 3x3 kernel

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            std::vector<int> kernel;

            for (int ki = -kernelSize; ki <= kernelSize; ki++) {
                for (int kj = -kernelSize; kj <= kernelSize; kj++) {
                    int ni = i + ki;
                    int nj = j + kj;
                    if (ni >= 0 && ni < height && nj >= 0 && nj < width) {
                        kernel.push_back(imIn[ni][nj]);
                    }
                }
            }

            std::sort(kernel.begin(), kernel.end()); //on trie pour avoir le pixel median
            imOut[i][j] = kernel[kernel.size() / 2];
        }
    }

}

//distribution gaussienne en 2D
float computeGaussianCoeff(int x, int y, float sigmaSpace) {
    float distanceSquared = x *x + y * y;
    return exp(-distanceSquared / (2.0 * sigmaSpace * sigmaSpace));
}


void bilateralBlur(ImageBase &imIn, ImageBase &imOut){

    int width = imIn.getWidth();
    int height = imIn.getHeight();
    int kernelSize = 1; //  1 = 3x3 kernel

    double sigmaSpace = 1.0; // ecart type de la distribution gaussienne

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

            double result = 0.0;
            double normalization = 0.0;
            int sampleCenter = imIn[i][j];

            for (int ki = -kernelSize; ki <= kernelSize; ki++) {
                for (int kj = -kernelSize; kj <= kernelSize; kj++) {
                    int ni = i + ki;
                    int nj = j + kj;

                    if (ni >= 0 && ni < height && nj >= 0 && nj < width) {
                        int sample = imIn[ni][nj];

                        float gaussianCoef = computeGaussianCoeff(ki, kj, sigmaSpace);

                        float closeness = 1.0f - std::abs(sample - sampleCenter) / 255.0f;
                        float weight = gaussianCoef * closeness;

                        result += sample * weight;
                        normalization += weight;
                    }
                }
            }

            imOut[i][j] = (int)(result / normalization + 0.5);
        }
    }


}


//----sampling

//Sous échantillonage d'une image, faut trouver un meilleur algo
//imout doit être de largeur Imin.width / 2 et de hauteur Imin.height / 2
void down_sampling(ImageBase & imIn, ImageBase & imOut){ // filtre moyenneur

    std::vector<std::vector<int>> kernel = {
        {1, 1},
        {1, 1}
    };

    int kernelSum = 0;
    for (const auto& row : kernel) {
        for (int val : row) {
            kernelSum += val;
        }
    }

    int kernelHeight = kernel.size();
    int kernelWidth = kernel[0].size();

    for (int i = 0; i < imOut.getHeight(); i++) {
        for (int j = 0; j < imOut.getWidth(); j++) {
            int sum = 0;
            for (int ki = 0; ki < kernelHeight; ki++) {
                for (int kj = 0; kj < kernelWidth; kj++) {
                    sum += imIn[i * 2 + ki][j * 2 + kj] * kernel[ki][kj];
                }
            }
            imOut[i][j] = sum / kernelSum;
        }
    }


};

//cette fonction fait un sous-échantillonnage avec interpolation bilinéaire
//Devrait etre meilleur que la fonction de base
void down_sampling_bilinear(ImageBase &imIn, ImageBase &imOut) {
    int newWidth = imOut.getWidth();
    int newHeight = imOut.getHeight();
    int oldWidth = imIn.getWidth();
    int oldHeight = imIn.getHeight();

    for (int i = 0; i < newHeight; i++) {
        for (int j = 0; j < newWidth; j++) {

            // Coordonnees image originale
            float x = j * (oldWidth - 1) / (float)(newWidth - 1);
            float y = i * (oldHeight - 1) / (float)(newHeight - 1);

            int x1 = (int) x;
            int y1 = (int) y;
            int x2 = std::min(x1 + 1, oldWidth - 1);
            int y2 = std::min(y1 + 1, oldHeight - 1);

            
            float dx = x - x1;
            float dy = y - y1;

            // Interpolation bilineaire
            float val = (1 - dx) * (1 - dy) * imIn[y1][x1] +
                        dx * (1 - dy) * imIn[y1][x2] +
                        (1 - dx) * dy * imIn[y2][x1] +
                        dx * dy * imIn[y2][x2];

            imOut[i][j] = (int) std::round(val);
        }
    }
}




void up_sampling(ImageBase & imIn, ImageBase & imOut){

    for (int i = 0; i < imOut.getHeight(); i++) {
        for (int j = 0; j < imOut.getWidth(); j++) {
            imOut[i][j] = imIn[i / 2][j / 2];
        }
    }

}

void down_sampling_bicubic(ImageBase &imIn, ImageBase &imOut){};
void down_sampling_lanczos(ImageBase &imIn, ImageBase &imOut){};

void up_sampling_bicubic(ImageBase &imIn, ImageBase &imOut){};
void up_sampling_lanczos(ImageBase &imIn, ImageBase &imOut){};