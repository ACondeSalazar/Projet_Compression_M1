#include "FlexibleCompression.h"

#include <iomanip>
#include <thread>


#include "Utils.h"
#include "FormatSamplingBlur.h"
#include "TransformationQuantification.h"
#include "RLE.h"
#include "Huffman.h"




#include "JPEG.h"
#include "JPEG2000.h"


#include <iostream>




void compressionFlex(char *cNomImgLue, char *cNomImgOut, ImageBase &imIn, CompressionSettings settings){

    std::vector<std::thread> threads;


    printf("Opening image : %s\n", cNomImgLue);

    imIn.load(cNomImgLue);

    //##########################Transformation des couleurs#############################
    printf("Transformation de l'espace couleur\n");

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

    //##########################Flou sur les composantes couleurs#############################
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


    //##########################DownSampling des composantes couleurs#############################

    printf("Sous échantillonage des Composantes couleurs\n");

    ImageBase downSampledColor1(imIn.getWidth() / 2, imIn.getHeight() /2, false);
    ImageBase downSampledColor2(imIn.getWidth() / 2, imIn.getHeight() /2, false);



    switch (settings.samplingType) {
        case NORMALSAMPLING:
            std::cout << "Basic sampling" << std::endl;
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


    //##########################Transformation#############################


    std::vector<Block> blocksLuminance;
    std::vector<Block> blocksColor1;
    std::vector<Block> blocksColor2;

    std::vector<std::pair<int,int>> blocksLuminanceRLE; //les blocs applatis et encodés en RLE
    std::vector<std::pair<int,int>> blocksColor1RLE;
    std::vector<std::pair<int,int>> blocksColor2RLE;

    std::vector<Tile> tilesY;
    std::vector<Tile> tilesCb;
    std::vector<Tile> tilesCr;

    int tilewidth = settings.tileWidth;
    int tileHeight = settings.tileHeight;

    if(settings.transformationType == DWTTRANSFORM){
        std::cout << "wavelet transform" << std::endl;

        tilesY = getTiles(luminance, tilewidth,tileHeight); 
        tilesCb = getTiles(downSampledColor1, tilewidth,tileHeight);
        tilesCr = getTiles(downSampledColor2, tilewidth,tileHeight);


        applyWaveletTransform53ToTiles(tilesY);

        applyWaveletTransform53ToTiles(tilesCr);
        
        applyWaveletTransform53ToTiles(tilesCb);

        float scale = getScaling(settings.QuantizationFactor);

        std::pair<int,int> steps = getQuantificationStep(settings.QuantizationFactor);

        std::cout << "Quantification steps: Low = " << steps.first << ", High = " << steps.second << std::endl;

        int stepLow = steps.first;
        int stepHigh = steps.second;

   
    
        for (auto& tile : tilesY) {
            quantificationuniforme(tile, stepLow,stepHigh); 
        }

        std::cout << "First tile of tilesY:" << std::endl;
        if (!tilesY.empty()) {
            std::cout << "First tile of tilesY:" << std::endl;
            for (const auto& row : tilesY.front().data) {
            for (const auto& value : row) {
                std::cout << std::setw(2) << value << " "; //setw pour aligner les valeurs
            }
            std::cout << std::endl;
            }
        } else {
            std::cout << "tilesY is empty." << std::endl;
        }

        for (auto& tile : tilesCb) {
            quantificationuniforme(tile, stepLow,stepHigh);
        }

        for (auto& tile : tilesCr) {
            quantificationuniforme(tile, stepLow,stepHigh);
        }

        bool debug = false;
        for(Tile & tile : tilesY){
            std::vector<std::pair<int,int>> RLETile;
    
            std::vector<int> flatTile = getFlatTile(tile);
            if(!debug){
                std::cout << "First flat tile:" << std::endl;
                for (const auto& value : flatTile) {
                    std::cout << value << " ";
                }
                std::cout << std::endl;
                debug = true;
            }
            
            RLECompression(flatTile,RLETile);
    
            blocksLuminanceRLE.insert(blocksLuminanceRLE.end(),RLETile.begin(),RLETile.end());
        }
    
        for(Tile & tile : tilesCb){
            std::vector<std::pair<int,int>> RLETile;
    
            std::vector<int> flatTile = getFlatTile(tile);
            RLECompression(flatTile,RLETile);
    
            blocksColor1RLE.insert(blocksColor1RLE.end(),RLETile.begin(),RLETile.end());
        }
    
        for(Tile & tile : tilesCr){
            std::vector<std::pair<int,int>> RLETile;
    
            std::vector<int> flatTile = getFlatTile(tile);
            RLECompression(flatTile,RLETile);
    
            blocksColor2RLE.insert(blocksColor2RLE.end(),RLETile.begin(),RLETile.end());
        }
    }
    else {

        blocksLuminance = getBlocks(luminance, 8);
        blocksColor1 = getBlocks(downSampledColor1, 8);
        blocksColor2 = getBlocks(downSampledColor2, 8);

        //quantification_better(blocksLuminance[0],quantificationLuminance , settings.QuantizationFactor);

        threads.emplace_back([&blocksLuminance, &settings] {

            for(Block & block : blocksLuminance){
                switch (settings.transformationType) {
                    case DCTTRANSFORM:
                        DCT(block);
                        //quantification(block);
                        quantification_better(block,quantificationLuminance , settings.QuantizationFactor);
                        break;
                    case INTDCTTRANSFORM:
                        INTDCT(block);
                        quantification_uniforme(block, settings.QuantizationFactor);
                        break;
                    case DCTIVTRANSFORM:
                        DCT_IV(block, true);
                        break;
                    default:
                        std::cerr << "transformation inconnue ?" << std::endl;
                    return;
                }
        

                flattenZigZag(block);
            }
        });
        
        threads.emplace_back([&blocksColor1, &settings] {
            for(Block & block : blocksColor1){
                switch (settings.transformationType) {
                    case DCTTRANSFORM:
                        DCT(block);
                        //quantification(block);
                        quantification_better(block,quantificationChrominance , settings.QuantizationFactor);
                        break;
                    case INTDCTTRANSFORM:
                        INTDCT(block);
                        quantification_uniforme(block, settings.QuantizationFactor);
                        break;
                    case DCTIVTRANSFORM:
                        DCT_IV(block, true);
                        break;
                    default:
                        std::cerr << "transformation inconnue ?" << std::endl;
                    return;
                }
                
                flattenZigZag(block);
            }
        });
        
        threads.emplace_back([&blocksColor2, &settings] {
            for(Block & block : blocksColor2){
                switch (settings.transformationType) {
                    case DCTTRANSFORM:
                        DCT(block);
                        //quantification(block);
                        quantification_better(block,quantificationChrominance , settings.QuantizationFactor);
                        break;
                    case INTDCTTRANSFORM:
                        INTDCT(block);
                        quantification_uniforme(block, settings.QuantizationFactor);
                        break;
                    case DCTIVTRANSFORM:
                        DCT_IV(block, true);
                        break;
                    default:
                        std::cerr << "transformation inconnue ?" << std::endl;
                    return;
                }

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

    //##########################Encodage RLE#############################


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

    //##########################Codage de Huffman et ecriture du fichier#############################


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
        outFileName, settings);

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

    //##########################Lecture du fichier#############################


    printf("Reading huffman encoded file\n");

    readHuffmanEncoded(outFileName,
                        codeTable, BlocksRLEEncoded,
                        imageWidth, imageHeight, downSampledWidth, downSampledHeight,
                        channelYRLESize, channelCbRLESize, channelCrRLESize, settings);

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

    //##########################On sépare les 3 canaux et on décode#############################
    //decompressBlocksRLE fait la transformation inverse et la quantification inverse
    // + upsampling

    std::vector<std::pair<int,int>> tilesYRLE; 
    std::vector<std::pair<int,int>> tilesCbRLE;
    std::vector<std::pair<int,int>> tilesCrRLE;

    std::vector<Tile> tilesY;
    std::vector<Tile> tilesCb;
    std::vector<Tile> tilesCr;
    
    if(settings.transformationType == DWTTRANSFORM){

        //on sépare les 3 canaux
        for (int i = 0; i < channelYRLESize; i++) {
            tilesYRLE.push_back(BlocksRLEEncoded[i]);
        }

        for (int i = channelYRLESize; i < channelYRLESize + channelCbRLESize; i++) {
            tilesCbRLE.push_back(BlocksRLEEncoded[i]);
        }

        for (int i = channelYRLESize + channelCbRLESize; i < channelYRLESize + channelCbRLESize + channelCrRLESize; i++) {
            tilesCrRLE.push_back(BlocksRLEEncoded[i]);
        }

        int tileWidth = settings.tileWidth;
        int tileHeight = settings.tileHeight;

        decompressTilesRLE(tilesCbRLE, tilesCb,tileWidth,tileHeight);
        decompressTilesRLE(tilesCrRLE, tilesCr,tileWidth,tileHeight);
        decompressTilesRLE(tilesYRLE, tilesY,tileWidth,tileHeight);

        std::pair<int, int> steps = getQuantificationStep(settings.QuantizationFactor);
        int stepLow = steps.first;
        int stepHigh = steps.second;

        

        for (auto& tile : tilesCb) {
            inverseQuantificationuniforme(tile,stepLow,stepHigh); 
        }
        for (auto& tile : tilesCr) {
            inverseQuantificationuniforme(tile,stepLow,stepHigh); 
        }
        for (auto& tile : tilesY) {
            inverseQuantificationuniforme(tile,stepLow,stepHigh); 
        }

        inverseWaveletTransform53ToTiles(tilesCb);
        inverseWaveletTransform53ToTiles(tilesCr);
        inverseWaveletTransform53ToTiles(tilesY);

        reconstructImage(tilesCb, imCb,tileWidth,tileHeight);
        reconstructImage(tilesCr, imCr,tileWidth,tileHeight);
        reconstructImage(tilesY, imY,tileWidth,tileHeight);

        switch (settings.samplingType) {
            case NORMALSAMPLING:
                printf("Normal upsampling\n");
                up_sampling(imCb, upSampledCb);
                up_sampling(imCr, upSampledCr);
                break;
            case BILENARSAMPLING:
                printf("Bilinear upsampling\n");
                up_sampling(imCb, upSampledCb);
                up_sampling(imCr, upSampledCr);
                break;
            case BICUBICSAMPLING:
                printf("Bicubic upsampling\n");
                up_sampling_bicubic(imCb, upSampledCb);
                up_sampling_bicubic(imCr, upSampledCr);
                break;
            case LANCZOSSAMPLING:
                printf("Lanczos upsampling\n");
                up_sampling_lanczos(imCb, upSampledCb);
                up_sampling_lanczos(imCr, upSampledCr);
                break;
            default:
                printf("Unknown upsampling type\n");
                return;
        }

    }else{

            //on sépare les 3 canaux
        threads.emplace_back([&BlocksRLEEncoded, &blocksYRLE, channelYRLESize, &blocksY, &imY, &settings] {
            for (int i = 0; i < channelYRLESize; i++) {
                blocksYRLE.push_back(BlocksRLEEncoded[i]);
            }

            decompressBlocksRLE(blocksYRLE, blocksY,quantificationLuminance , &settings);
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

            decompressBlocksRLE(blocksCbRLE, blocksCb, quantificationChrominance, &settings);
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

            decompressBlocksRLE(blocksCrRLE, blocksCr , quantificationChrominance,  &settings);
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


    }

    
    printf("Image channel sizes:\n");
    printf("Y channel: %dx%d\n", imY.getWidth(), imY.getHeight());
    printf("Cb channel: %dx%d\n", upSampledCb.getWidth(), upSampledCb.getHeight());
    printf("Cr channel: %dx%d\n", upSampledCr.getWidth(), upSampledCr.getHeight());

    //##########################Reconstruction de l'image a partir des 3 canaux#############################

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