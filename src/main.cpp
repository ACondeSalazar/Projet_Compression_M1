#include "ImageBase.h"
#include "LZ77.h"
#include "Utils.h"
#include "FlexibleCompression.h"
#include "JPEG.h"
#include "JPEG2000.h"


#include <cstddef>

#include <string>
#include <iostream>

#include <stdio.h>
#include <filesystem>
#include <SDL3/SDL.h>
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

#include "ImGuiFileDialog.h"

#include <chrono>
#include <fstream>
#include <iomanip>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

ImageBase imgOriginal;
ImageBase * imgDecompressed;

long sizeOriginal;
long sizeCompressed;

float tauxCompression = -1;
float psnr = -1;


std::string originalFilePathName = "";
std::string compressedFilePathName = "./compressed.img";
std::string decompressedFilePathName = "./decompressed.ppm";

SDL_Texture * textureOriginal;
int widthOriginal, heightOriginal;

SDL_Texture * textureDecompressed;
int widthDecompressed, heightDecompressed;

float zoom = 1.0f;
float posX = 0, posY = 0;

bool isMovingView = false;
float mouseX = 0, mouseY = 0;

bool originalInitialized = false; //si on a charger une image
bool decompressedInitialized = false; //si on a lancé la compression sur une image


std::chrono::duration<double> compressionTime;
std::chrono::duration<double> decompressionTime;

CompressionSettings customCompressionSettings;



void ConvertPNGToPPM(const std::string& pngFilePath, std::string& ppmFilePath, int& width, int& height) {
    int channels;
    unsigned char* imgData = stbi_load(pngFilePath.c_str(), &width, &height, &channels, 3);
    if (!imgData) {
        std::cerr << "Impossible de charger l'image " << pngFilePath << std::endl;
        return;
    }

    ppmFilePath = pngFilePath.substr(0, pngFilePath.find_last_of(".")) + ".ppm";
    FILE* ppmFile = fopen(ppmFilePath.c_str(), "wb");
    if (!ppmFile) {
        std::cerr << "Conversion en ppm impossible " << ppmFilePath << std::endl;
        stbi_image_free(imgData);
        return;
    }

    fprintf(ppmFile, "P6\n%d %d\n255\n", width, height);
    fwrite(imgData, sizeof(unsigned char), width * height * 3, ppmFile);
    fclose(ppmFile);

    stbi_image_free(imgData);
}

void LoadTexture(std::string& filePathName, int& width, int& height, SDL_Renderer* renderer, SDL_Texture** texture) { // pointeur vers un pointeur
    int channels;

    // Si l'image est en .png, on convertit en ppm
    if (filePathName.substr(filePathName.find_last_of(".") + 1) == "png") {
        std::string ppmFilePathName;
        ConvertPNGToPPM(filePathName, ppmFilePathName, width, height);
        filePathName = ppmFilePathName; // On met le chemin à jour vers l'image ppm
    }

    unsigned char* imgPPMData = stbi_load(filePathName.data(), &width, &height, &channels, 3);

    *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, width, height);

    if (!texture) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        stbi_image_free(imgPPMData);
        return;
    }

    SDL_UpdateTexture(*texture, NULL, imgPPMData, width * 3);

    SDL_SetTextureScaleMode(*texture, SDL_SCALEMODE_NEAREST); // Pas de traitement

    stbi_image_free(imgPPMData);
}

void resetView(){
    posX = 0;
    posY = 0;

    if(widthOriginal ==0 || heightOriginal == 0){
        zoom = 1.0f;
    }else{
        //on veut que l'image rentre dans la fenetre
        float zoomX = (float)SCREEN_WIDTH / widthOriginal;
        float zoomY = (float)SCREEN_HEIGHT / heightOriginal;
        zoom = std::min(zoomX, zoomY);
    }
    
}

void compressJPEGInterface(SDL_Renderer * renderer){
    if (originalFilePathName == "") {
        std::cout << "Erreur : aucun fichier selectionné" << std::endl;
        return; 
    }
    //compression
    auto startTime = std::chrono::high_resolution_clock::now();

    compression(originalFilePathName.data(), compressedFilePathName.data(), imgOriginal, customCompressionSettings);

    compressionTime = std::chrono::high_resolution_clock::now() - startTime;

    //taux de compression
    sizeOriginal = getFileSize(originalFilePathName);
    sizeCompressed = getFileSize(compressedFilePathName);
    tauxCompression = (double)sizeOriginal/(double)sizeCompressed;

    //decompression
    startTime = std::chrono::high_resolution_clock::now();

    decompression(compressedFilePathName.data(), decompressedFilePathName.data(), imgDecompressed, customCompressionSettings);
    std::cout << "finished decompression" << std::endl;

    decompressionTime = std::chrono::high_resolution_clock::now() - startTime;

    //affichage
    LoadTexture(decompressedFilePathName, widthDecompressed, heightDecompressed, renderer, &textureDecompressed);

    //psnr
    //ImageBase imOut2;
    //imOut2.load(decompressedFilePathName.data());
    psnr = PSNRptr(imgOriginal, imgDecompressed);

    decompressedInitialized = true;
}


void compressJPEG2000Interface(SDL_Renderer * renderer){
    if (originalFilePathName == "") {
        std::cout << "Erreur : aucun fichier selectionné" << std::endl;
        return; 
    }

    //compression
    auto startTime = std::chrono::high_resolution_clock::now();
    compression2000(originalFilePathName.data(), compressedFilePathName.data(), imgOriginal, customCompressionSettings);

    compressionTime = std::chrono::high_resolution_clock::now() - startTime;

    sizeOriginal = getFileSize(originalFilePathName);
    sizeCompressed = getFileSize(compressedFilePathName);
    tauxCompression = (double)sizeOriginal/(double)sizeCompressed;

    //decompression
    startTime = std::chrono::high_resolution_clock::now();

    decompression2000(compressedFilePathName.data(), decompressedFilePathName.data(), imgDecompressed, customCompressionSettings);
    std::cout << "finished decompression" << std::endl;

    decompressionTime = std::chrono::high_resolution_clock::now() - startTime;

    LoadTexture(decompressedFilePathName, widthDecompressed, heightDecompressed, renderer, &textureDecompressed);

    ImageBase imOut2;
    imOut2.load(decompressedFilePathName.data());

    psnr = PSNR(imgOriginal, imOut2);

    decompressedInitialized = true;
}

void compressFlexInterface(SDL_Renderer * renderer){
    if (originalFilePathName == "") {
        std::cout << "Erreur : aucun fichier selectionné" << std::endl;
        return; 
    }

    //compression
    auto startTime = std::chrono::high_resolution_clock::now();

    compressionFlex(originalFilePathName.data(), compressedFilePathName.data(), imgOriginal, customCompressionSettings);

    compressionTime = std::chrono::high_resolution_clock::now() - startTime;

    sizeOriginal = getFileSize(originalFilePathName);
    sizeCompressed = getFileSize(compressedFilePathName);
    tauxCompression = (double)sizeOriginal/(double)sizeCompressed;

    //decompression
    startTime = std::chrono::high_resolution_clock::now();

    decompressionFlex(compressedFilePathName.data(), decompressedFilePathName.data(), imgDecompressed, customCompressionSettings);
    std::cout << "finished decompression" << std::endl;

    decompressionTime = std::chrono::high_resolution_clock::now() - startTime;

    LoadTexture(decompressedFilePathName, widthDecompressed, heightDecompressed, renderer, &textureDecompressed);

    //ImageBase imOut2;
    //imOut2.load(decompressedFilePathName.data());

    //psnr = PSNR(imgOriginal, imOut2);

    psnr = PSNRptr(imgOriginal, imgDecompressed);

    decompressedInitialized = true;
}


void logResultsToCSV(const std::string& filename, const std::string& image,
                     const std::string& transform, const std::string& encoding,
                     int quantFactor, int windowSize,
                     int sizeOriginal, int sizeCompressed,
                     double tauxCompression, double psnr) {
    
    std::ofstream file;
    bool fileExists = std::ifstream(filename).good();

    file.open(filename, std::ios::app);

    if (!fileExists) {
        file << "Image,Transformation,Encoding,QuantizationFactor,WindowSize,"
             << "OriginalSize,CompressedSize,CompressionRate,PSNR\n";
    }

    file << image << ","
         << transform << ","
         << encoding << ","
         << quantFactor << ","
         << windowSize << ","
         << sizeOriginal << ","
         << sizeCompressed << ","
         << std::fixed << std::setprecision(2) << tauxCompression << ","
         << std::fixed << std::setprecision(2) << psnr << "\n";

    file.close();
}


void launchComparator(){

        std::vector<std::string> originalFilePathNames = {
         "./img/cygne4k.png","./img/town.png", "./img/wall.png",
        "./img/forest4K.png", "./img/wood4K.png",
        "./img/bridge4K.ppm", "./img/sample4k.ppm", "./img/ice4K.ppm",
        "./img/cat4K.ppm", "./img/4Kmyrtille.ppm"
    };

    std::vector<TransformationType> transformationTypes = {DCTTRANSFORM, DWTTRANSFORM};
    std::vector<EncodingType> encodingTypes = {RLE, LZ77};
    std::vector<int> tileSizes = {120, 120, 240, 240, 480, 270, 1920, 1080};
    std::vector<int> quantizationFactors = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    std::vector<int> windowSizes = {5, 10, 20, 100, 500};

    CompressionSettings settings;

    for (auto& filePath : originalFilePathNames) {
        int width, height;
        if (filePath.substr(filePath.find_last_of(".") + 1) == "png") {
            std::string ppmFilePathName;
            ConvertPNGToPPM(filePath, ppmFilePathName, width, height);
            filePath = ppmFilePathName; // On met le chemin à jour vers l'image ppm
        }

        imgOriginal.load(filePath.data());

        for (const auto& transform : transformationTypes) {
            bool useTileSize = (transform == DWTTRANSFORM);

            int tileLoopEnd = useTileSize ? tileSizes.size() : 2;
            for (int i = 0; i < tileLoopEnd; i += 2) {
                int tileWidth = useTileSize ? tileSizes[i] : 0;
                int tileHeight = useTileSize ? tileSizes[i + 1] : 0;

                for (const auto& encoding : encodingTypes) {
                    bool useWindowSize = (encoding == LZ77);
                    const std::vector<int>& windows = useWindowSize ? windowSizes : std::vector<int>{0};

                    for (const auto& windowSize : windows) {
                        for (const auto& quantFactor : quantizationFactors) {
                            
                            settings.colorFormat = YCBCRFORMAT;
                            settings.blurType = BILATERALBLUR;
                            settings.samplingType = BILENARSAMPLING;
                            settings.transformationType = transform;
                            settings.QuantizationFactor = quantFactor;
                            settings.tileWidth = tileWidth;
                            settings.tileHeight = tileHeight;
                            settings.encodingType = encoding;
                            settings.encodingWindowSize = windowSize;
                            
                            

                            
                            compressionFlex(filePath.data(), compressedFilePathName.data(), imgOriginal, settings);

                            sizeOriginal = getFileSize(filePath);
                            sizeCompressed = getFileSize(compressedFilePathName);
                            tauxCompression = (double)sizeOriginal / (double)sizeCompressed;

                            decompressionFlex(compressedFilePathName.data(), decompressedFilePathName.data(), imgDecompressed, settings);
                            //ImageBase imOut2;
                            //imOut2.load(decompressedFilePathName.data());
                            //psnr = PSNRptr(imgOriginal, imgDecompressed);
                            psnr = PSNRptr(imgOriginal, imgDecompressed);

                            logResultsToCSV(
                                "results.csv",
                                filePath,
                                transform == DCTTRANSFORM ? "DCT" : "DWT",
                                encoding == RLE ? "RLE" : "LZ77",
                                quantFactor,
                                windowSize,
                                sizeOriginal,
                                sizeCompressed,
                                tauxCompression,
                                psnr
                            );
                        }
                    }
                }
            }
        }
    }


}

int main(int argc, char **argv)
{   
    customCompressionSettings.colorFormat = YCBCRFORMAT;
    customCompressionSettings.blurType = GAUSSIANBLUR;
    customCompressionSettings.samplingType = BILENARSAMPLING;
    customCompressionSettings.transformationType = DWTTRANSFORM;
    customCompressionSettings.QuantizationFactor = 100;
    customCompressionSettings.tileHeight = 120;
    customCompressionSettings.tileWidth = 120;
    customCompressionSettings.encodingType = LZ77;
    customCompressionSettings.encodingWindowSize = 200;

    namespace fs = std::filesystem;

    try {
        if (!fs::exists("./img")) {
            fs::create_directory("./img");
        }
        if (!fs::exists("./img/out")) {
            fs::create_directory("./img/out");
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Create a ./img and ./img/out folder !! " << e.what() << std::endl;
    }

    if ( !SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow( "Projet compression", SCREEN_WIDTH, SCREEN_HEIGHT, 0 );
    if (window == NULL) {
        SDL_Log("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL) {
        SDL_Log("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    

    bool quit = false;
    SDL_Event event;

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            }
            ImGui_ImplSDL3_ProcessEvent(&event);
            if(io.WantCaptureMouse){break;}

            //déplacer la vue
            if (event.type == SDL_EVENT_MOUSE_MOTION) {

                if (isMovingView) {
                    float deltaX = event.motion.x - mouseX;
                    float deltaY = event.motion.y - mouseY;
                    posX += deltaX;
                    posY += deltaY;
                }
                mouseX = event.motion.x;
                mouseY = event.motion.y;
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    isMovingView = true;
                    mouseX = event.button.x;
                    mouseY = event.button.y;
                }
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    isMovingView = false;
                }
            }

            //zoom
            if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                if (event.wheel.y > 0) {
                    zoom *= 1.1f;
                    posX = mouseX - (mouseX - posX) * 1.1f; //on veut zoomer avec la souris comme centre
                    posY = mouseY - (mouseY - posY) * 1.1f;
                } else if (event.wheel.y < 0) {
                    zoom /= 1.1f;
                    posX = mouseX- (mouseX - posX) / 1.1f;
                    posY = mouseY - (mouseY- posY) / 1.1f;
                }
            }

            const bool * keyboardState = SDL_GetKeyboardState(NULL);

            if(keyboardState[SDL_SCANCODE_R]){
                resetView();
            }
            
        }

        //new frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(350,300), ImGuiCond_Once);

        ImGui::Begin("Compression");

        if(ImGui::TreeNode("Help")){
            ImGui::Text("Clique + mouvement : déplacement");
            ImGui::Text("Molette : zoom");
            ImGui::Text("R : réinitialiser la vue");
            ImGui::Text("Image Originale à gauche et image compressée à droite");
            ImGui::TreePop();
        }
        
        if(ImGui::Button("Launch tests")){
                launchComparator();
            }

        if(originalInitialized){
            ImGui::Text("Chemin vers l'image : %s", originalFilePathName.c_str());
            ImGui::Text("Taille Image : %d x %d", widthOriginal, heightOriginal);
            
            if(ImGui::Button("Compression JPEG like")){
                compressJPEGInterface(renderer);
            }

            if(ImGui::Button("Compression JPEG2000 like")){
                compressJPEG2000Interface(renderer);
            }

            if(ImGui::Button("Compression custom")){
                compressFlexInterface(renderer);
            }

            ImGui::Combo("Format couleur", (int *)&customCompressionSettings.colorFormat, "YCBCR\0YCOCG\0YUV(implementer)\0");
            ImGui::Combo("Type flou", (int *)&customCompressionSettings.blurType, "Gaussian\0Median\0Bilateral\0");
            ImGui::Combo("Type sampling", (int *)&customCompressionSettings.samplingType, "Normal\0Bilinear\0Bicubic (implementer)\0Lanczos(implémenter)\0");
            ImGui::Combo("Type transformation", (int *)&customCompressionSettings.transformationType, "DCT\0DWT\0INTDCT(implementer?)\0DCTIV(implementer?)\0");
            if (customCompressionSettings.transformationType == DWTTRANSFORM) {
                ImGui::InputInt("Tile Width", &customCompressionSettings.tileWidth);
                ImGui::InputInt("Tile Height", &customCompressionSettings.tileHeight);
            }
            ImGui::SliderInt("Qualité (quantification)", &customCompressionSettings.QuantizationFactor, 1, 100);
            ImGui::Combo("Type d'encodage", (int *)&customCompressionSettings.encodingType, "RLE\0LZ77\0");

            if(customCompressionSettings.encodingType == LZ77){
                ImGui::InputInt("Taille fenetre encodage", &customCompressionSettings.encodingWindowSize);
            }

        
        }

        


        if(decompressedInitialized){
            ImGui::Text("Taux de compression : %.2f", tauxCompression);
            ImGui::Text("PSNR : %.2f", psnr);

            if(ImGui::TreeNode("More compression info")){
                ImGui::Text("De %.0f KB à %.0f KB",  sizeOriginal / 1000.0, sizeCompressed / 1000.0);
                ImGui::Text("Temps compression : %.1f seconds", compressionTime.count());
                ImGui::Text("Temps decompression : %.1f seconds", decompressionTime.count());

                ImGui::Text("Chemin fichier compressé: %s", compressedFilePathName.c_str());
                ImGui::Text("Chemin image decompressé: %s", decompressedFilePathName.c_str());
                ImGui::TreePop();
            }
            
        }

        


        if (ImGui::Button("Choisir image")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".ppm, .png", config);
        }
        
        ImGui::SetNextWindowSize(ImVec2(800,500), ImGuiCond_Once);
        if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
            if (ImGuiFileDialog::Instance()->IsOk()) { 
                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                std::cout << filePath << std::endl;
                std::cout << filePathName << std::endl;

                originalFilePathName = filePathName;

                LoadTexture(originalFilePathName, widthOriginal, heightOriginal, renderer, &textureOriginal);

                originalInitialized = true;
                decompressedInitialized = false;

                textureDecompressed = nullptr;


                resetView();
                
                
                
            }

            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGui::Button("Choisir fichier")) {
            IGFD::FileDialogConfig config2;
            config2.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("chooseCompressed", "Choose Compressed file", ".img", config2);
        }
        ImGui::SetNextWindowSize(ImVec2(800,500), ImGuiCond_Once);
        if (ImGuiFileDialog::Instance()->Display("chooseCompressed")) {
            if (ImGuiFileDialog::Instance()->IsOk()) { 
                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                std::cout << filePath << std::endl;
                std::cout << filePathName << std::endl;

                ImageBase imOut;
                decompressionFlex(filePathName.data(), decompressedFilePathName.data(), imgDecompressed, customCompressionSettings);
    
                std::cout << "finished decompression" << std::endl;
                originalFilePathName = decompressedFilePathName;

                widthOriginal = imgDecompressed->getWidth();
                heightOriginal = imgDecompressed->getHeight();

                LoadTexture(decompressedFilePathName, widthOriginal, heightOriginal, renderer, &textureOriginal);

                originalInitialized = true;
                decompressedInitialized = false;

                textureDecompressed = nullptr;


                resetView();
                
                
                
            }

            ImGuiFileDialog::Instance()->Close();
        }


        ImGui::End();
        
        //render
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);
        

        SDL_FRect ogRect = { posX, posY, widthOriginal * zoom, heightOriginal * zoom };
        SDL_RenderTexture(renderer, textureOriginal, NULL, &ogRect);

        SDL_FRect compressedRect = { posX + (widthOriginal*zoom) + 10, posY , widthDecompressed * zoom, heightDecompressed * zoom };
        SDL_RenderTexture(renderer, textureDecompressed, NULL, &compressedRect);



        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    delete imgDecompressed;

    SDL_DestroyTexture(textureOriginal);
    SDL_DestroyTexture(textureDecompressed);

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

	return 0;
}