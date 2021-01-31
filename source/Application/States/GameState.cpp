#include "GameState.hpp"
#include "raylib.h"
#include "AppStateManager.hpp"
#include <cmath>
#include "kiss_fftr.h"
#include <array>
#include <new>


#define CircleTexture texture_arr[textures::circletex]
#define BackgroundPhoto texture_arr[textures::bgphoto]

#define Linear false
#define MovingAverage true
#define CirclularSpectrum true
#define MovingAverageSmoothOut false

#ifndef TWOPI
#define TWOPI 6.28318530718f
#endif

vis::GameState vis::GameState::instance;


static float spec_inter_width = 4.f;
static float interpolation_factor = .25f;
static Vector2 screenSize;

/*

    TODO: Consider implementing Cubic Interpolation.
    Reason: Better Shape, Linear Interpolation still keeps Sharp edges
    || DONE

*/

namespace vis{
    template<typename T,typename Time = float>
    constexpr inline T mix(const T x, const T y,Time _time ){
        return x*(1.0-_time)+y*_time;
    }

    template<typename T,typename Time = float> 
    constexpr inline T mixCubic(const T y0, const T y1,const T y2,const T y3,const Time time ){
        return y1 + 
                    .5f * time*(y2 - y0 + 
                          time*(2.f*y0  + 4.f*y2 - 5.f*y1 - y3 + 
                          time*(3.f*(y1 - y2) + y3 - y0)));
    }
}


// This should prolly be lambda
float scaleVisualizer(kiss_fft_cpx complex){
    float temp = 2.f*10.f*std::log(  0.07f*(complex.r*complex.r+complex.i*complex.i ) ) ;
    return (temp > 0 )? temp : 0;
}


///////////////////////////////////////////
// TODO: Synchronize Decoder by locking audio thread mutex 
///////////////////////////////////////////
void vis::GameState::Events(Application* app){
    if (IsKeyPressed(KEY_Q)) app->PopState();
    if (IsKeyPressed(KEY_P)){
        if (ma_device_is_started(MainPlayer->GetDevice())) ma_device_stop(MainPlayer->GetDevice());
        else ma_device_start(MainPlayer->GetDevice());
    }
    if (IsKeyPressed(KEY_F11)){
    if (!IsWindowFullscreen()){
   
       
       
        ToggleFullscreen();
        MaximizeWindow();
        screenSize = app->GetScreenCords();
        //SetWindowMonitor(0);
    }
    else {
        ToggleFullscreen(); 
        std::cout<<"WINDOW SMOLL! "<<'\n';
    }
    }
    // Temporary
    // Soon it'll turn into Music Slider
    if (IsKeyPressed(KEY_ONE)){
        ma_device_stop(MainPlayer->GetDevice());//ma_mutex_lock(&MainPlayer->GetDevice()->lock);
        ma_decoder_seek_to_pcm_frame(MainPlayer->GetDecoder(),3*2'000'000);
        ma_device_start(MainPlayer->GetDevice());//ma_mutex_unlock(&MainPlayer->GetDevice()->lock);
    }
    if (IsKeyPressed(KEY_TWO)){
        ma_device_stop(MainPlayer->GetDevice());//ma_mutex_lock(&MainPlayer->GetDevice()->lock);
        ma_decoder_seek_to_pcm_frame(MainPlayer->GetDecoder(),0);
        ma_device_start(MainPlayer->GetDevice());//ma_mutex_unlock(&MainPlayer->GetDevice()->lock);
    }
    if (IsKeyPressed(KEY_THREE)){
        ma_device_stop(MainPlayer->GetDevice());//ma_mutex_lock(&MainPlayer->GetDevice()->lock);
        ma_decoder_seek_to_pcm_frame(MainPlayer->GetDecoder(),2'000'000);
        ma_device_start(MainPlayer->GetDevice());//ma_mutex_unlock(&MainPlayer->GetDevice()->lock);
    }

    //For Shader Hot Reload
    if (IsKeyPressed(KEY_R)){
        for (size_t i = 0; i < shaders_locs.size(); ++i){
            UnloadShader(shader_arr[i]);
            shader_arr[i] = LoadShader(NULL, shaders_locs[i]);
        }
        time_uniform.time_loc = GetShaderLocation(shader_arr[shaders::spectrum],"time");

    }
}




//////////////////////////
// TODO: Lock Free Passing FFT Data from Audio Thread to Render Thread 
//  Consider this library to achieve that
//  @link https://github.com/RossBencina/QueueWorld?fbclid=IwAR0JgZAwVn4dwgJUGheRwKd2bAgApS267O6xE6clRIzbXdY15rhHGxAc5iw
//////////////////////////
void vis::GameState::Update(Application* app){
    screenSize = app->GetScreenCords();
    time_uniform.time += GetFrameTime();

    MainPlayer->SwapFFTBuffers(MainPlayer->fftOutput);

    float delta = MainPlayer->circleBumper-circleBumperSmooth;
    circleBumperSmooth+=delta*0.3f;
    for (size_t i(1); i < smoothSpectrum.size(); ++i){
        float deltaFFT = scaleVisualizer(MainPlayer->fftOutput[i]) - smoothSpectrum[i];
        smoothSpectrum[i]+=deltaFFT*0.2f;
    }
#if MovingAverageSmoothOut 
    const int offset = 20;

    /**     
     *      Perform Moving Average on linear spectrum range, then store it in array
     * 
     */
        // smoothSpectrumCopy[0] = smoothSpectrum[0+offset];
        // smoothSpectrumCopy[1] = smoothSpectrum[1+offset];
        for (small_int i = 0 ; i < smoothSpectrum.size()-2-offset; ++i){
            smoothSpectrumCopy[i] = 0.1f * (smoothSpectrum[i-2+offset] + smoothSpectrum[i+2+offset])
                           + 0.2f * (smoothSpectrum[i-1+offset] + smoothSpectrum[i+1+offset])
                           + 0.4f * (smoothSpectrum[i+offset]);
        }
        {
            auto logIndexSize = smoothSpectrum.size() - 2 - offset;
            smoothSpectrumCopy[logIndexSize - 2 ] = smoothSpectrum[logIndexSize + offset];
            smoothSpectrumCopy[logIndexSize - 1] = smoothSpectrum[logIndexSize + 1 + offset];
        }
#endif

    #if MovingAverage

        /* 
            Perform moving average on log scale
        */

        logSpectrum[0] = smoothSpectrum[logIndex[0]];
        logSpectrum[1] = smoothSpectrum[logIndex[1]];
        for (size_t i = 2; i < logSpectrum.size()-2; ++i){
            logSpectrum[i] = 0.1f * (smoothSpectrum[logIndex[i-2]] + smoothSpectrum[logIndex[i+2]])
                           + 0.2f * (smoothSpectrum[logIndex[i-1]] + smoothSpectrum[logIndex[i+1]])
                           + 0.4f * (smoothSpectrum[logIndex[i]]);
        }
        {
        auto logIndexSize = logSpectrum.size();
        logSpectrum[logIndexSize-2] = smoothSpectrum[logIndex[logIndexSize-2]];
        logSpectrum[logIndexSize-1] = smoothSpectrum[logIndex[logIndexSize-1]];
        }

    #else 
        for (size_t i = 0; i < logSpectrum.size(); ++i){
            logSpectrum[i] = smoothSpectrum[logIndex[i]];
        }
        
    #endif
    
    if (IsWindowResized()) {
        spec_inter_width = (screenSize.x) / (logSpectrum.size()-1.f); 
        interpolation_factor = 1.f  / spec_inter_width ; 

        
    }



}




/*
////////////////////////////////////////////////
    TODO:
    TOP PRIORITY: Wrap Smoothing Unrolled Loops into function 
    1. Fix Shaders || DONE ||
    2. Smooth FFT in Frequency Axis || DONE ||  
    3. Fix Log scale to be legit log scale and not pseudo log scale || DONE ||
    4. Add gui
    5. Add Music Player
    6. More Effects
    7. Ball is fine as of Logic, but sucks as visual = Replace Ball || DONE ||. 
    8. Pass Music fft data as Sampler2D into a shader, consider doing so in Update or Init method
    9. Consider switching Log Magnitude Scale to Max Peak Scale (Basically a Cut-Off Scale)

///////////////////////////////////////////////
*/

void vis::GameState::Render(Application* app){
    
    

    SetShaderValue(shader_arr[shaders::spectrum],1,&time_uniform.time,UNIFORM_FLOAT);
    SetShaderValue(shader_arr[shaders::SH_BGSHIFT],1,&circleBumperSmooth,UNIFORM_FLOAT);
    SetShaderValue(shader_arr[shaders::circle],1,&time_uniform.time,UNIFORM_FLOAT);

    BeginDrawing();

        ClearBackground(BLACK);

        BeginShaderMode(shader_arr[shaders::SH_BGSHIFT]);
        
        DrawTexturePro(BackgroundPhoto,Rectangle{0 , 0 ,
                                        static_cast<float>(BackgroundPhoto.width ) ,
                                        static_cast<float>(BackgroundPhoto.height)},
                                        Rectangle{0,0,screenSize.x,screenSize.y},Vector2{0,0},0,WHITE);
        

        
        
        #define StaticSize 200.f

        const float circleRadius = (circleBumperSmooth+StaticSize)*.5f;



        BeginShaderMode(shader_arr[shaders::spectrum]);

        #if Linear 
        for (int i = 0;i<logSpectrum.size()-1;++i){

            float temp[2]{logSpectrum[i],logSpectrum[i+1]};  //interpolating requires lower and upper height 
            
            for (int j(0); j < spec_inter_width; ++j){  // 4 stands for width of rectangle, since we gonna interpolate between heights 
                float interpolated = vis::mix<float>(temp[0],temp[1],j*interpolation_factor /*/4.f*/);
                DrawTexturePro(texture_arr[textures::spectrumtex],Rectangle{0,0,1,1},Rectangle{spec_inter_width*i+j,screenSize.y-interpolated,1.f,interpolated},Vector2{0.f,0.f},0.f,BLANK);
            }

        }
        #elif !CirclularSpectrum
        {
        
        float temp[3]{logSpectrum[0],logSpectrum[1],logSpectrum[2]};
        for (float j(0); j < spec_inter_width; ++j){  
                float interpolated = vis::mixCubic<float>(temp[0],temp[0],temp[1],temp[2],j*interpolation_factor );
                DrawTexturePro(texture_arr[textures::spectrumtex],Rectangle{0,0,1,1},Rectangle{j,screenSize.y-interpolated,1.f,interpolated},Vector2{0.f,0.f},0.f,BLANK);
            }
        }
        

        for (int i = 1;i<logSpectrum.size()-2;++i){

            float temp[4]{logSpectrum[i-1],logSpectrum[i],logSpectrum[i+1],logSpectrum[i+2]};  //interpolating requires lower and upper height 
            
            for (float j(0); j < spec_inter_width; ++j){  
                float interpolated = vis::mixCubic<float>(temp[0],temp[1],temp[2],temp[3],j*interpolation_factor );
                DrawTexturePro(texture_arr[textures::spectrumtex],Rectangle{0,0,1,1},Rectangle{spec_inter_width*(i)+j,screenSize.y-interpolated,1.f,interpolated},Vector2{0.f,0.f},0.f,BLANK);
            }

        }
        {
        const auto maxSize = logSpectrum.size()-2;
        float temp[3]{logSpectrum[maxSize-1],logSpectrum[maxSize],logSpectrum[maxSize+1]};
        for (float j(0); j < spec_inter_width; ++j){  
                float interpolated = vis::mixCubic<float>(temp[0],temp[1],temp[2],temp[2],j*interpolation_factor );
                DrawTexturePro(texture_arr[textures::spectrumtex],Rectangle{0,0,1,1},Rectangle{spec_inter_width*(maxSize)+j,screenSize.y-interpolated,1.f,interpolated},Vector2{0.f,0.f},0.f,BLANK);
            }
        }  
        #else 
        
        #define array_to_draw logSpectrum

        const float bar_circleRadius = circleRadius - 2.f;
        const float spectrum_range = 100.f;
        const int   spectrum_range_int = 100;
        const float inter_factor = 15.f;
        const float delta = 0.00001f;
        const float bar_location_x = screenSize.x*.5f;
        const float bar_location_y = screenSize.y*.5f;
        const float bar_rotation_factor = 180.f / ( (spectrum_range-1.f)  * inter_factor ) ;
        float       bar_rotation = -bar_rotation_factor;

        {
        float temp[3]{array_to_draw[0],array_to_draw[1],array_to_draw[2]};
        for (float j(0); j < inter_factor; ++j){  
                bar_rotation += bar_rotation_factor;                   
                float interpolated = vis::mixCubic<float>(temp[0],temp[0],temp[1],temp[2],j*(1.f/inter_factor) );
                
                DrawTexturePro(texture_arr[textures::spectrumtex],Rectangle{0,0,1,1},Rectangle{bar_location_x,
                            bar_location_y,1.f,interpolated},Vector2{0.5f,bar_circleRadius+interpolated},-bar_rotation,BLANK);
            
                DrawTexturePro(texture_arr[textures::spectrumtex],Rectangle{0,0,1,1},Rectangle{bar_location_x,
                            bar_location_y,1.f,interpolated},Vector2{0.5f,bar_circleRadius+interpolated},bar_rotation,BLANK);
           
                            
            }
        }
        for (int i = 1;i< spectrum_range-2 ;++i){

            float temp[4]{array_to_draw[i-1],array_to_draw[i],array_to_draw[i+1],array_to_draw[i+2]};  //interpolating requires lower and upper height 
            
            for (float j(0); j < inter_factor; ++j){
                bar_rotation += bar_rotation_factor;
                float interpolated = vis::mixCubic<float>(temp[0],temp[1],temp[2],temp[3],j*(1.f/inter_factor) );
                DrawTexturePro(texture_arr[textures::spectrumtex],Rectangle{0,0,1,1},Rectangle{bar_location_x,
                            bar_location_y,1.f,interpolated},Vector2{0.5f,bar_circleRadius+interpolated},-bar_rotation,BLANK);
            
                DrawTexturePro(texture_arr[textures::spectrumtex],Rectangle{0,0,1,1},Rectangle{bar_location_x,
                            bar_location_y,1.f,interpolated},Vector2{0.5f,bar_circleRadius+interpolated},bar_rotation,BLANK);
         
            }

        }
        {
        const auto maxSize = spectrum_range-2;
        float temp[3]{array_to_draw[maxSize-1],array_to_draw[maxSize],array_to_draw[maxSize+1]};
        for (float j(0); j < inter_factor; ++j){  
                bar_rotation += bar_rotation_factor; 
                float interpolated = vis::mixCubic<float>(temp[0],temp[1],temp[2],temp[2],j*(1.f/inter_factor) );
                DrawTexturePro(texture_arr[textures::spectrumtex],Rectangle{0,0,1,1},Rectangle{bar_location_x,
                            bar_location_y,1.f,interpolated},Vector2{0.5f,bar_circleRadius+interpolated},-bar_rotation,BLANK);
            
                DrawTexturePro(texture_arr[textures::spectrumtex],Rectangle{0,0,1,1},Rectangle{bar_location_x,
                            bar_location_y,1.f,interpolated},Vector2{0.5f,bar_circleRadius+interpolated},bar_rotation,BLANK);

            }
        } 
        #endif 
        BeginShaderMode(shader_arr[shaders::circle]);
        DrawTexturePro(CircleTexture,Rectangle{0,0,
                                     static_cast<float>(CircleTexture.width),
                                     static_cast<float>(CircleTexture.height)},
                                     Rectangle{0.5f*screenSize.x,0.5f*screenSize.y,circleBumperSmooth+StaticSize,circleBumperSmooth+StaticSize},
                                     Vector2{circleRadius,circleRadius},0.f,WHITE);



        EndShaderMode();

        DrawFPS(10,10);
    EndDrawing();

    // Unused for now
    (void)app;
}

/*
///////////////////////////////
 TODO: 
  1. Change Copy Constructors to whatever is more efficient
  2. Should indexArray be of size 2048? lol
  3. Fix magic number mess
  4. Once Shaders are set and done, change shaders to be String Literals (aka. Hardcoded)
  5. Method of resizing Linear Scale to Log Scale is Ugly, but prolly can't do anything with that
///////////////////////////////
*/

void vis::GameState::Init(){


    //MainPlayer = vis::Visualizer::GetInstance();
    MainPlayer = new (vis_buffer) vis::Visualizer;

    
    
    time_uniform = {0};

    int temp = 0;    
    fileName = GetDroppedFiles(&temp)[0];
    MainPlayer->LoadVisualizerData(fileName);
    ClearDroppedFiles();
    
    //Compute log scale indexes
    small_int indexArray[2048];
    small_int prev = 3;
    small_int counter = 0;
    float cacheSampleRate = MainPlayer->GetDecoder()->outputSampleRate/2;
    float cache_fftSize   = MainPlayer->fftOutput.size();
    smoothSpectrum = std::vector<float>(cache_fftSize,0);
    smoothSpectrumCopy = smoothSpectrum;

    for (float i(3); i < cache_fftSize; i*=1.02f){
            small_int cacheRound = (small_int)i; 
            if (cacheRound*cacheSampleRate/cache_fftSize > 15000.f) break;
            if (prev!=cacheRound){
                indexArray[counter] = cacheRound;
                ++counter;
            }

            prev = cacheRound;
    }

    logIndex    = std::vector<small_int>(indexArray,indexArray+counter)  ;
    logSpectrum = std::vector<float>(counter,0);



    Image im_temp = GenImageColor(1,1,BLANK);
    texture_arr[textures::spectrumtex] = LoadTextureFromImage(im_temp);
    UnloadImage(im_temp);

    for (size_t i = textures::bgphoto; i < texture_arr.size(); ++i){
        texture_arr[i] = LoadTexture(texture_locs[i]);
    }

    for (size_t i = shaders::spectrum; i < shader_arr.size(); ++i ){
        shader_arr[i] = LoadShader(NULL, shaders_locs[i]);
    }

    time_uniform.time_loc = GetShaderLocation(shader_arr[shaders::spectrum],"time");
   


    MainPlayer->StartPlayer();
}


///////////////////////////////////
// TODO: Shouldnt this be a Destructor at all?
///////////////////////////////////

void vis::GameState::Cleanup(){
    for (const auto& v : shader_arr){
        UnloadShader(v);
    }
    for (const auto& v: texture_arr){
        UnloadTexture(v);
    }
    
    MainPlayer->~Visualizer();
    smoothSpectrum.clear();
    smoothSpectrumCopy.clear();
    logIndex.clear();
    logSpectrum.clear();
    fileName.clear();
}


 
