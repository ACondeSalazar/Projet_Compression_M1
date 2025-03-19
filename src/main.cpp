#include "ImageBase.h"
#include "JPEG.h"
#include "Utils.h"
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_surface.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <iostream>
#include <vector>

#include <SDL3/SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlrenderer3.h"

#include "imgui/ImGuiFileDialog.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

ImageBase imgOriginal;
ImageBase * imgDecompressed;

long sizeOriginal;
long sizeCompressed;

float tauxCompression = -1;
float psnr = -1;


std::string originalFilePathName = "";
std::string compressedFilePathName = "./compressed";
std::string decompressedFilePathName = "./decompressed.ppm";

SDL_Texture * textureOriginal;
int widthOriginal, heightOriginal;

SDL_Texture * textureDecompressed;
int widthDecompressed, heightDecompressed;

float zoom = 1.0f;
float posX = 0, posY = 0;

bool isMovingView = false;
float mouseX = 0, mouseY = 0;

bool originalInitialized = false;
bool decompressedInitialized = false;


void LoadTexture(std::string & filePathName, int & width, int & height, SDL_Renderer * renderer, SDL_Texture ** texture){ //pointeur vers un pointeur
    int channels;
    unsigned char * imgData = stbi_load(filePathName.data(), &width, &height, &channels, 0);

    *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, width, height);

    if (!texture) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        stbi_image_free(imgData);
        return;
    }

    SDL_UpdateTexture(*texture, NULL, imgData, width * 3);

    stbi_image_free(imgData);

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

void compressJPEG(SDL_Renderer * renderer){
    if (originalFilePathName == "") {
        std::cout << "Erreur : aucun fichier selectionné" << std::endl;
        return; 
    }
    compression(originalFilePathName.data(), compressedFilePathName.data(), imgOriginal);

    sizeOriginal = getFileSize(originalFilePathName);
    sizeCompressed = getFileSize(compressedFilePathName);
    tauxCompression = (double)sizeOriginal/(double)sizeCompressed;

    decompression(compressedFilePathName.data(), decompressedFilePathName.data(), imgDecompressed);
    std::cout << "finished decompression" << std::endl;

    LoadTexture(decompressedFilePathName, widthDecompressed, heightDecompressed, renderer, &textureDecompressed);

    ImageBase imOut2;
    imOut2.load(decompressedFilePathName.data());

    psnr = PSNR(imgOriginal, imOut2);

    decompressedInitialized = true;
}


int main( int argc, char* args[] )
{

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

        ImGui::Begin("Compression");

        ImGui::Text("Click + drag to move around");
        ImGui::Text("Mouse wheel to zoom in/out");
        ImGui::Text("R to reset view");
        ImGui::Text("Original on the left, decompressed on the right");

        if(originalInitialized){
            ImGui::Text("Path to image : %s", originalFilePathName.c_str());
            
            if(ImGui::Button("Compression JPEG like")){
                compressJPEG(renderer);
            }
    
            if(ImGui::Button("Compression JPEG2000 like (a ajouter)")){
                //compressJPEG(renderer);
            }
        
        }

        


        if(decompressedInitialized){
            ImGui::Text("Taux de compression : %f", tauxCompression);
            ImGui::Text("PSNR : %f", psnr);
        }

        


        if (ImGui::Button("Choisir image")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".ppm", config);
        }
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


    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

	return 0;
}