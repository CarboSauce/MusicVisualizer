#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>



#include "kiss_fftr.h"
#include "miniaudio.h"

/////////////////////////////////////////////
//          This class contains everything related to audio and is used for processing data
/////////////////////////////////////////////



namespace vis{

    extern "C" typedef void ma_callback_c_cpp(ma_device*,void*,const void*,ma_uint32);

    class Visualizer{
        
            
        public:
            Visualizer() = default;
            ~Visualizer(){
                CleanUp();
            }
            void LoadVisualizerData(const std::string& fileName);
            void StartPlayer();
            void CleanUp();
            

            void SwapFFTBuffers(std::vector<kiss_fft_cpx>& output);      
            
            inline ma_decoder* GetDecoder() noexcept { return &decoder; };
            inline ma_device*  GetDevice () noexcept { return &device ; };
        
        
            //////////////////////////////////
            //      Friends : )
            //
        private: 

            //static ma_device_callback_proc data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount); 
            //static ma_device_callback_proc song_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
            static ma_callback_c_cpp data_callback;
            static ma_callback_c_cpp song_callback;
            //static Visualizer internalVis;


        protected:
            
            ///////////////////////////////////
            //      Audio Variables
            //
        public:    
            float circleBumper = 0.f;


        
        private:
            
            ma_context context;
            ma_context_config contextConfig;
            ma_pcm_rb g_rb;
            ma_decoder_config decoderConfig;
            ma_decoder decoder;
            ma_device_config deviceConfig;
            ma_device device;
            
            
            ////////////////////////////////////
            //      FFT Sample Data
            //
            kiss_fftr_cfg fftConfig;
            std::vector<kiss_fft_cpx> fftCopyBuffer;
            std::vector<float> fftBuffer;
            std::vector<float> windowFunction;
        
            //////////////////////////////////////
            //      MULTITHREADING SUCKS
            /////////////

            ma_mutex fftMutex;
            bool canCopyBuffer = false;
            
        public: 
            const short soundBufferSize = 3072; // lower the number, lower the accuracy, higher the number, less often buffer is filled up
            const int fftSize = 16384; //2^15
            std::vector<kiss_fft_cpx> fftOutput;

    };
} //namespace vis