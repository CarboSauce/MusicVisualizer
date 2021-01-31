#pragma once
#include "AppStateManager.hpp"
#include "AudioContainer.hpp"
#include <string>
#include <array>

typedef int small_int;

/*
    TODO: 
    MORE COMMENTS!!!

*/

namespace vis{


    class GameState: public StateManager {
      public:
            
            void Init();
            void Cleanup();
            
            ///////////////////////////
            
            void Events(Application* app);
            void Update(Application* app);
            void Render(Application* app);
            static StateManager* Instance(){
                return &instance;
            };
        protected:
            GameState(){};
            ~GameState(){std::cout<<"~GameState()\n";} // Debug
        private:
            enum shaders: unsigned char {
                spectrum = 0,
                circle,
                SH_BGSHIFT,
                sh_num_elements
            } ;
            enum textures : unsigned char {
                spectrumtex =0,
                bgphoto,
                circletex,
                tex_num_elements
            } ;
            float circleBumperSmooth = 0.f;
            struct {
                float time;
                int time_loc;
            } time_uniform;
            struct {
                float shift;
                int shift_loc;
            } bgshift_uniform;
            std::vector<small_int> logIndex;
            std::vector<float> logSpectrum;
            std::vector<float> smoothSpectrum;
            std::vector<float> smoothSpectrumCopy; 
            std::string fileName;
            vis::Visualizer* MainPlayer;
            alignas(vis::Visualizer) unsigned char vis_buffer[sizeof(vis::Visualizer)];
            std::string optionText{"Game!!!"};

            static GameState instance;


            //Textures + Shaders
            //Texture2D rectangleTex;
            //Texture2D BackgroundPhoto; 
            //Texture2D CircleTexture;
            std::array<Texture2D, textures::tex_num_elements> texture_arr;
            std::array<Shader, shaders::sh_num_elements> shader_arr;
            //Shader spectrumShader;
            //Shader circleShader;
            std::array<const char*, textures::tex_num_elements> texture_locs{ nullptr , "../resources/background.jpg" , "../resources/circle.jpg"};
            std::array<const char*, shaders::sh_num_elements > shaders_locs{ 
                                                                            "../shaders/AfterImage.frag" ,
                                                                            "../shaders/CircleShader.frag",
                                                                            "../shaders/bgShader.frag"};

    };

    
}
