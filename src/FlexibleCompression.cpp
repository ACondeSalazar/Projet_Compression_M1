#include "FlexibleCompression.h"
#include "Utils.h"
#include "JPEG2000.h"
#include "JPEG.h"
#include <thread>
#include <threads.h>
#include "Huffman.h"

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

void down_sampling_bicubic(ImageBase &imIn, ImageBase &imOut){};
void down_sampling_lanczos(ImageBase &imIn, ImageBase &imOut){};

void up_sampling_bicubic(ImageBase &imIn, ImageBase &imOut){};
void up_sampling_lanczos(ImageBase &imIn, ImageBase &imOut){};

void DCT_IV(Block &block, bool forward, unsigned int N = 8){
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

void compressionFlex(char *cNomImgLue, char *cNomImgOut, ImageBase &imIn, CompressionSettings settings){

    std::vector<std::thread> threads;


    printf("Opening image : %s\n", cNomImgLue);

    imIn.load(cNomImgLue);

    //Transformation des couleurs
    printf("Transformation de l'espace couleur");

    ImageBase luminance(imIn.getWidth(), imIn.getHeight(), false);
    ImageBase colorChannel1(imIn.getWidth(), imIn.getHeight(), false);
    ImageBase colorChannel2(imIn.getWidth(), imIn.getHeight(),false);
    

    switch (settings.colorFormat) {
        case YCBCRFORMAT:
            std::cout << "separation Y Cb Cr" << std::endl;
            RGB_to_YCbCr(imIn, luminance, colorChannel1, colorChannel2);
            break;
        case YCOCGFORMAT:
            std::cout << "separation Y Co Cg" << std::endl;
            RGB_to_YCOCG(imIn, luminance, colorChannel1, colorChannel2);
            break;
        case YUVFORMAT:
            std::cout << "YUV separation not implemented" << std::endl;
            return ;
            break;
    
    }

    luminance.save("./img/out/Y.pgm");
    colorChannel1.save("./img/out/Cb.pgm");
    colorChannel2.save("./img/out/Cr.pgm");

    printf("  Fini\n");

    printf("Flou \n");

    ImageBase imColorChannel1Flou(imIn.getWidth(), imIn.getHeight(), false);
    ImageBase imColorChannel2Flou(imIn.getWidth(), imIn.getHeight(), false);

    switch (settings.blurType) {
        case GAUSSIANBLUR:
            std::cout << "Gaussian blur" << std::endl;
            gaussianBlur(colorChannel1, imColorChannel1Flou);
            gaussianBlur(colorChannel2, imColorChannel2Flou);
            break;
        case MEDIANBLUR:
            std::cout << "Median blur" << std::endl;
            medianBlur(colorChannel1, imColorChannel1Flou);
            medianBlur(colorChannel2, imColorChannel2Flou);
            break;
        case BILATERALBLUR:
            std::cout << "Bilateral blur" << std::endl;
            bilateralBlur(colorChannel1, imColorChannel1Flou);
            bilateralBlur(colorChannel2, imColorChannel2Flou);
            break;
    }

    printf("Sous échantillonage des Composantes couleurs\n");

    ImageBase downSampledColor1(imIn.getWidth() / 2, imIn.getHeight() /2, false);
    ImageBase downSampledColor2(imIn.getWidth() / 2, imIn.getHeight() /2, false);



    switch (settings.samplingType) {
        case NORMALSAMPLING:
            std::cout << "Normal sampling" << std::endl;
            down_sampling(imColorChannel1Flou, downSampledColor1);
            down_sampling(imColorChannel2Flou, downSampledColor2);
            break;
        case BILENARSAMPLING:
            std::cout << "Bilinear sampling" << std::endl;
            down_sampling_bilinear(imColorChannel1Flou, downSampledColor1);
            down_sampling_bilinear(imColorChannel2Flou, downSampledColor2);
            break;
        case BICUBICSAMPLING:
            std::cout << "Bicubic sampling a implementer" << std::endl;
            //down_sampling_bicubic(imColorChannel1Flou, downSampledColor1);
            //down_sampling_bicubic(imColorChannel2Flou, downSampledColor2);
            break;
        case LANCZOSSAMPLING:
            std::cout << "Lanczos sampling a implementer" << std::endl;
            //down_sampling_lanczos(imColorChannel1Flou, downSampledColor1);
            //down_sampling_lanczos(imColorChannel2Flou, downSampledColor2);
            break;
    
    }

    downSampledColor1.save("./img/out/downSampledCb.pgm");
    downSampledColor2.save("./img/out/downSampledCr.pgm");
    printf("  Fini\n");

    std::vector<Block> blocksLuminance;
    std::vector<Block> blocksColor1;
    std::vector<Block> blocksColor2;

    std::vector<std::pair<int,int>> blocksLuminanceRLE; //les blocs applatis et encodés en RLE
    std::vector<std::pair<int,int>> blocksColor1RLE;
    std::vector<std::pair<int,int>> blocksColor2RLE;

    if(settings.transformationType == DWTTRANSFORM){
        std::cout << "wavelet transform" << std::endl;
        //applyWaveletTransform53ToTiles(tiles);
        //inverseWaveletTransform53ToTiles(tiles);
    }
    else {

        blocksLuminance = getBlocks(luminance, 8);
        blocksColor1 = getBlocks(downSampledColor1, 8);
        blocksColor2 = getBlocks(downSampledColor2, 8);

        threads.emplace_back([&blocksLuminance, &settings] {
            for(Block & block : blocksLuminance){
                switch (settings.transformationType) {
                    case DCTTRANSFORM:
                        DCT(block);
                        break;
                    case INTDCTTRANSFORM:
                        INTDCT(block);
                        break;
                    case DCTIVTRANSFORM:
                        DCT_IV(block, true);
                        break;
                    default:
                        std::cerr << "transformation inconnue ?" << std::endl;
                    return;
                }
        
                quantification(block);
                flattenZigZag(block);
            }
        });
        
        threads.emplace_back([&blocksColor1, &settings] {
            for(Block & block : blocksColor1){
                switch (settings.transformationType) {
                    case DCTTRANSFORM:
                        DCT(block);
                        break;
                    case INTDCTTRANSFORM:
                        INTDCT(block);
                        break;
                    case DCTIVTRANSFORM:
                        DCT_IV(block, true);
                        break;
                    default:
                        std::cerr << "transformation inconnue ?" << std::endl;
                    return;
                }
                quantification(block);
                flattenZigZag(block);
            }
        });
        
        threads.emplace_back([&blocksColor2, &settings] {
            for(Block & block : blocksColor2){
                switch (settings.transformationType) {
                    case DCTTRANSFORM:
                        DCT(block);
                        break;
                    case INTDCTTRANSFORM:
                        INTDCT(block);
                        break;
                    case DCTIVTRANSFORM:
                        DCT_IV(block, true);
                        break;
                    default:
                        std::cerr << "transformation inconnue ?" << std::endl;
                    return;
                }
                quantification(block);
                flattenZigZag(block);
            }
        });
        
        
        for (auto &thread : threads) {
            thread.join();
        }
        threads.clear();

        std::cout << "Last block of blocksLuminance:" << std::endl;
        if (!blocksLuminance.empty()) {
            for (const auto& row : blocksLuminance.back().dctMatrix) {
            for (const auto& value : row) {
                std::cout << value << " ";
            }
            std::cout << std::endl;
            }
        } else {
            std::cout << "blocksLuminance is empty." << std::endl;
        }

        threads.emplace_back([&blocksLuminance, &blocksLuminanceRLE] {
            for(Block & block : blocksLuminance){
            std::vector<std::pair<int,int>> RLEBlock;
        
            RLECompression(block.flatDctMatrix, RLEBlock);
        
            blocksLuminanceRLE.insert(blocksLuminanceRLE.end(), RLEBlock.begin(), RLEBlock.end());
            }
        });
        
        threads.emplace_back([&blocksColor1, &blocksColor1RLE] {
            for(Block & block : blocksColor1){
            std::vector<std::pair<int,int>> RLEBlock;
        
            RLECompression(block.flatDctMatrix, RLEBlock);
        
            blocksColor1RLE.insert(blocksColor1RLE.end(), RLEBlock.begin(), RLEBlock.end());
            }
        });
        
        threads.emplace_back([&blocksColor2, &blocksColor2RLE] {
            for(Block & block : blocksColor2){
            std::vector<std::pair<int,int>> RLEBlock;
        
            RLECompression(block.flatDctMatrix, RLEBlock);
        
            blocksColor2RLE.insert(blocksColor2RLE.end(), RLEBlock.begin(), RLEBlock.end());
            }
        });
    
        for (auto &thread : threads) {
            thread.join();
        }
        threads.clear();

    }

    std::vector<std::pair<int, int>> allBlocksRLE; //on fusionne les 3 canaux
    allBlocksRLE.insert(allBlocksRLE.end(), blocksLuminanceRLE.begin(), blocksLuminanceRLE.end());
    allBlocksRLE.insert(allBlocksRLE.end(), blocksColor1RLE.begin(), blocksColor1RLE.end());
    allBlocksRLE.insert(allBlocksRLE.end(), blocksColor2RLE.begin(), blocksColor2RLE.end());

    printf("  Fini\n");
    printf("Huffman encoding ");

    std::vector<huffmanCodeSingle> codeTable;

    HuffmanEncoding(allBlocksRLE, codeTable);

    printf("Code table size: %lu\n", codeTable.size());

    printf("  Fini\n");

    printf("Writing huffman encoded file\n");
    std::string outFileName = cNomImgOut;

    //on ecrit le fichier huffman encodé
    writeHuffmanEncoded(allBlocksRLE, codeTable,
        imIn.getWidth(), imIn.getHeight(), downSampledColor1.getWidth(), downSampledColor1.getHeight() ,
        blocksLuminanceRLE.size(), blocksColor1RLE.size(),blocksColor2RLE.size(),
        outFileName);

}

void decompressionFlex(const char *cNomImgIn, const char *cNomImgOut, ImageBase *imOut, CompressionSettings settings){

    std::string outFileName = cNomImgIn;
    std::vector<huffmanCodeSingle> codeTable;
    std::vector<std::pair<int, int>> BlocksRLEEncoded;

    int imageWidth, imageHeight;
    int downSampledWidth, downSampledHeight;
    int channelYRLESize, channelCbRLESize, channelCrRLESize;

    std::vector<std::thread> threads;

    int maxThreads = std::thread::hardware_concurrency();

    printf("Reading huffman encoded file\n");

    readHuffmanEncoded(outFileName,
                        codeTable, BlocksRLEEncoded,
                        imageWidth, imageHeight, downSampledWidth, downSampledHeight,
                        channelYRLESize, channelCbRLESize, channelCrRLESize);

    std::cout<<"size downSampledWidth "<<downSampledWidth<<" "<<downSampledHeight<<std::endl;
    printf("Code table size: %lu\n", codeTable.size());

    std::vector<std::pair<int,int>> blocksYRLE; //les blocs applatis et encodés en RLE
    std::vector<std::pair<int,int>> blocksCbRLE;
    std::vector<std::pair<int,int>> blocksCrRLE;

    std::vector<Block> blocksY;
    std::vector<Block> blocksCb;
    std::vector<Block> blocksCr;

    imOut = new ImageBase(imageWidth, imageHeight, true);

    ImageBase imY(imageWidth, imageHeight, false);

    ImageBase imCb(downSampledWidth, downSampledHeight, false);
    ImageBase imCr(downSampledWidth, downSampledHeight, false);

    ImageBase upSampledCb(imageWidth, imageHeight, false);
    ImageBase upSampledCr(imageWidth, imageHeight, false);

    //on sépare les 3 canaux
    threads.emplace_back([&BlocksRLEEncoded, &blocksYRLE, channelYRLESize, &blocksY, &imY, &settings] {
        for (int i = 0; i < channelYRLESize; i++) {
            blocksYRLE.push_back(BlocksRLEEncoded[i]);
        }

        decompressBlocksRLE(blocksYRLE, blocksY, settings.transformationType);
        printf("blocksY size: %lu\n", blocksY.size());

        printf("Reconstructing Y channel\n");
        reconstructImage(blocksY, imY, 8);
        printf("saving Y channel\n");
        imY.save("./img/out/Y_decompressed.pgm");
    });
    
    // reconstruction de Cb
    threads.emplace_back([&BlocksRLEEncoded, &blocksCbRLE, channelYRLESize, channelCbRLESize, &blocksCb, &imCb, &upSampledCb, &settings] {
        for (int i = channelYRLESize; i < channelYRLESize + channelCbRLESize; i++) {
            blocksCbRLE.push_back(BlocksRLEEncoded[i]);
        }

        decompressBlocksRLE(blocksCbRLE, blocksCb, settings.transformationType);
        printf("blocksCb size: %lu\n", blocksCb.size());

        printf("Reconstructing Cb channel\n");
        reconstructImage(blocksCb, imCb, 8);

        switch (settings.samplingType) {
            case NORMALSAMPLING:
                printf("Normal upsampling\n");
                up_sampling(imCb, upSampledCb);
                break;
            case BILENARSAMPLING:
                printf("Bilinear upsampling (same as normal sampling ??)\n");
                up_sampling(imCb, upSampledCb);
                break;
            case BICUBICSAMPLING:
                printf("Bicubic upsampling\n");
                up_sampling_bicubic(imCb, upSampledCb);
                break;
            case LANCZOSSAMPLING:
                printf("Lanczos upsampling\n");
                up_sampling_lanczos(imCb, upSampledCb);
                break;
            default:
                printf("Unknown upsampling type\n");
                return;
        }

        upSampledCb.save("./img/out/Cb_decompressed.pgm");
    });

    //reconstruction de Cr
    threads.emplace_back([&BlocksRLEEncoded, &blocksCrRLE, channelYRLESize, channelCbRLESize, channelCrRLESize, &blocksCr, &imCr, &upSampledCr, &settings] {
        for (int i = channelYRLESize + channelCbRLESize; i < channelYRLESize + channelCbRLESize + channelCrRLESize; i++) {
            blocksCrRLE.push_back(BlocksRLEEncoded[i]);
        }

        decompressBlocksRLE(blocksCrRLE, blocksCr, settings.transformationType);
        printf("blocksCr size: %lu\n", blocksCr.size());

        

        printf("Reconstructing Cr channel\n");
        reconstructImage(blocksCr, imCr, 8);

        switch (settings.samplingType) {
            case NORMALSAMPLING:
                printf("Normal upsampling\n");
                up_sampling(imCr, upSampledCr);
                break;
            case BILENARSAMPLING:
                printf("Bilinear upsampling(il faut une meilleur fonction \n");
                up_sampling(imCr, upSampledCr);
                break;
            case BICUBICSAMPLING:
                printf("Bicubic upsampling\n");
                up_sampling_bicubic(imCr, upSampledCr);
                break;
            case LANCZOSSAMPLING:
                printf("Lanczos upsampling\n");
                up_sampling_lanczos(imCr, upSampledCr);
                break;
            default:
                printf("Unknown upsampling type\n");
                return;
        }

        upSampledCr.save("./img/out/Cr_decompressed.pgm");
    });

    for (auto &thread : threads) {
        thread.join();
    }
    threads.clear();

    std::cout << "Last block of blocksY:" << std::endl;
    if (!blocksY.empty()) {
        for (const auto& row : blocksY.back().dctMatrix) {
            for (const auto& value : row) {
                std::cout << value << " ";
            }
            std::cout << std::endl;
        }
    } else {
        std::cout << "blocksY is empty." << std::endl;
    }

    printf("Image channel sizes:\n");
    printf("Y channel: %dx%d\n", imY.getWidth(), imY.getHeight());
    printf("Cb channel: %dx%d\n", upSampledCb.getWidth(), upSampledCb.getHeight());
    printf("Cr channel: %dx%d\n", upSampledCr.getWidth(), upSampledCr.getHeight());

    printf("Blocks decoding\n");

    printf("Reconstructing image from blocks\n");

    printf("Reconstructing image from channels\n");
    switch (settings.colorFormat) {
        case YCBCRFORMAT:
            std::cout << "Reconstructing YCbCr to RGB" << std::endl;
            YCbCr_to_RGB(imY, upSampledCb, upSampledCr, *imOut);
            break;
        case YCOCGFORMAT:
            std::cout << "Reconstructing YCoCg to RGB" << std::endl;
            YCOCG_to_RGB(imY, upSampledCb, upSampledCr, *imOut);
            break;
        case YUVFORMAT:
            std::cout << "YUV reconstruction not implemented" << std::endl;
            return;
            break;
    }

    printf("Saving decompressed image\n");
    std::string cNomImgOutStr = cNomImgOut;
    (*imOut).save(cNomImgOutStr.data());
    

    printf("Decompression finished\n");

}