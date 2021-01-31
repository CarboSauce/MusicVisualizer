#include "AudioContainer.hpp"
#include "miniaudio.h"
#include <cmath>
#include <cassert>

//silence intellisense
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//vis::Visualizer vis::Visualizer::internalVis;
//Temp
template<typename T>
float SumUpAvg(T begin, T end){
    float sum{};
    for (auto it = begin; it < end; ++it){
        sum += std::sqrt( it->i * it->i + it->r * it->r );
    }
    return std::sqrt( sum / (end-begin) ); 
}


/**************************************************************************
This function computes the chebyshev polyomial T_n(x)
***************************************************************************/
double cheby_poly(int n, double x){
    double res;
    if (fabs(x) <= 1) res = cos(n*acos(x));
    else              res = cosh(n*acosh(x));
    return res;
}

/***************************************************************************
 calculate a chebyshev window of size N, store coeffs in out as in Antoniou
-out should be array of size N
-atten is the required sidelobe attenuation (e.g. if you want -60dB atten, use '60')
***************************************************************************/


void cheby_win(float *out, int N, float atten){
    int nn, i;
    double M, n, sum = 0, max=0;
    double tg = pow(10,atten/20);  /* 1/r term [2], 10^gamma [2] */
    double x0 = cosh((1.0/(N-1))*acosh(tg));
    M = (N-1)/2;
    if(N%2==0) M = M + 0.5; /* handle even length windows */
    for(nn=0; nn<(N/2+1); nn++){
        n = nn-M;
        sum = 0;
        for(i=1; i<=M; i++){
            sum += cheby_poly(N-1,x0*cos(M_PI*i/N))*cos(2.0*n*M_PI*i/N);
        }
        out[nn] = tg + 2*sum;
        out[N-nn-1] = out[nn];
        if(out[nn]>max)max=out[nn];
    }
    for(nn=0; nn<N; nn++) out[nn] /= max; /* normalise everything */
    return;
}





/////////////////////////
//      Fixed size framecount callback
//      TODO: Proper handling of fft
/////////////////////////
// @link https://stackoverflow.com/questions/2068022/in-c-is-it-safe-portable-to-use-static-member-function-pointer-for-c-api-call
//One could aswell make them free functions, alldoe not sure if that would work at all
extern "C"{
void vis::Visualizer::song_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount){
    vis::Visualizer& vis_ref = *reinterpret_cast<vis::Visualizer*>(pDevice->pUserData);

    //Fill audio device with decoder samples
    ma_decoder* pDecoder = vis_ref.GetDecoder();
    if (pDecoder == NULL) {
        return;
    }
    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount);
    //allocate fft;
    //
    ma_mutex_lock(&vis_ref.fftMutex);
    //
    // TODO: Make it independent of being stereo sound


    for (ma_uint32 i = 0, j = 0; i < frameCount;++i,j+=2){
        float* const temptr = (float*)pOutput;
        vis_ref.fftBuffer[i] = vis_ref.windowFunction[i]*(temptr[j]+temptr[j+1])/2;
    }
    kiss_fftr(vis_ref.fftConfig,vis_ref.fftBuffer.data(),vis_ref.fftCopyBuffer.data()); 

    vis_ref.canCopyBuffer = true;
    //
    ma_mutex_unlock(&vis_ref.fftMutex);


    (void)pInput;
}
/////////////////////
//   Main Callback which handles Ring Buffer to then pass into constant frame count callback called 'song_callback'
//   TODO: Prolly nothing
//////////////////////


void vis::Visualizer::data_callback (ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    /*
    This is the device's main data callback. This will handle all of the fixed sized buffer management for you and will call data_callback_fixed()
    for you. You should do all of your normal callback stuff in data_callback_fixed().
    */
    vis::Visualizer& vis_ref = *reinterpret_cast<vis::Visualizer*>(pDevice->pUserData);
    
    ma_uint32 pcmFramesAvailableInRB;
    ma_uint32 pcmFramesProcessed = 0;
    ma_uint8* pRunningOutput = (ma_uint8*)pOutput;

    //MA_ASSERT(pDevice->playback.channels == decoder.outputChannels);

    /*
    The first thing to do is check if there's enough data available in the ring buffer. If so we can read from it. Otherwise we need to keep filling
    the ring buffer until there's enough, making sure we only fill the ring buffer in chunks of PCM_FRAME_CHUNK_SIZE.
    */
    while (pcmFramesProcessed < frameCount) {    /* Keep going until we've filled the output buffer. */
        ma_uint32 framesRemaining = frameCount - pcmFramesProcessed;
        pcmFramesAvailableInRB = ma_pcm_rb_available_read(&vis_ref.g_rb);
        if (pcmFramesAvailableInRB > 0) {
            ma_uint32 framesToRead = (framesRemaining < pcmFramesAvailableInRB) ? framesRemaining : pcmFramesAvailableInRB;
            void* pReadBuffer;

            ma_pcm_rb_acquire_read(&vis_ref.g_rb, &framesToRead, &pReadBuffer);
            {
                memcpy(pRunningOutput, pReadBuffer, framesToRead * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));
            }
            ma_pcm_rb_commit_read(&vis_ref.g_rb, framesToRead, pReadBuffer);

            pRunningOutput += framesToRead * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
            pcmFramesProcessed += framesToRead;
        } else {
            /*
            There's nothing in the buffer. Fill it with more data from the callback. We reset the buffer first so that the read and write pointers
            are reset back to the start so we can fill the ring buffer in chunks of PCM_FRAME_CHUNK_SIZE which is what we initialized it with. Note
            that this is not how you would want to do it in a multi-threaded environment. In this case you would want to seek the write pointer
            forward via the producer thread and the read pointer forward via the consumer thread (this thread).
            */
            ma_uint32 framesToWrite = vis_ref.fftSize;
            void* pWriteBuffer;

            ma_pcm_rb_reset(&vis_ref.g_rb);
            ma_pcm_rb_acquire_write(&vis_ref.g_rb, &framesToWrite, &pWriteBuffer);
            {
                //assert(framesToWrite == fftSize);   /* <-- This should always work in this example because we just reset the ring buffer. */
                song_callback(pDevice, pWriteBuffer, NULL, framesToWrite);
            }
            ma_pcm_rb_commit_write(&vis_ref.g_rb, framesToWrite, pWriteBuffer);
        }
    }  
    /* Unused in this example. */
    (void)pInput;
}



} //extern "C"




////////////////////////////////////
// TODO: Fix magic numbers
///////////////////////////////////
void vis::Visualizer::LoadVisualizerData(const std::string& fileName){

    {
    
    int tempSize = fftSize;
    fftConfig           = kiss_fftr_alloc          (tempSize,0,NULL,NULL);
    this->fftBuffer     = std::vector<float>       (tempSize,0);
    this->fftCopyBuffer = std::vector<kiss_fft_cpx>(tempSize/2+1,kiss_fft_cpx{0.f,0.f});
    this->fftOutput     = this->fftCopyBuffer;


    //windowFunction = std::vector<float>(soundBufferSize);
    
    windowFunction = std::vector<float>();
    for (int i = 0; i < soundBufferSize;++i){
            windowFunction.emplace_back(.54f-.46f*std::cos(2*M_PI*i/soundBufferSize));
        }
    
    }  //temp scope for tempSize caching
    
    //cheby_win(windowFunction.data(), windowFunction.size(),360);

    
    //context allocation for more customization
    contextConfig = ma_context_config_init();
    contextConfig.threadPriority = ma_thread_priority_realtime;
    if (ma_context_init(NULL,0,&contextConfig,&context )!= MA_SUCCESS) printf("chuj\n\n");

    //decoder init, note forced formating to 32bit samples
    decoderConfig = ma_decoder_config_init(ma_format_f32,2,0);
    ma_decoder_init_file(fileName.c_str(),&decoderConfig,&decoder);
    
    //ring buffer alloc
    
    ma_pcm_rb_init(ma_format_f32,decoder.outputChannels,soundBufferSize,NULL,NULL,&g_rb);
    
    
    //device config allocation duh
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate        = decoder.outputSampleRate;
    deviceConfig.dataCallback      = this->data_callback;
    deviceConfig.pUserData         = this;
    //////////////////////////////////
    // updating fft variables, need to be done after decoder init cuz of unknown channels
    // TODO: 
    // Consider should we really multiply by channel count, 
    // channel related samples are each after the other like so: L R L R L R 
    

    //cheby_win(windowFunction.data(),windowFunction.size(),240);

    ma_mutex_init(&fftMutex);

    if (ma_device_init(&context, &deviceConfig, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        ma_decoder_uninit(&decoder);
        return;
    }
    ma_device_set_master_volume(&device,0.3f);
    if (ma_context_uninit(&context) != MA_SUCCESS) std::cout<<"Context Deletion failed.";
}




////////////////////////////////
//  TODO: Reorganize into proper start
////////////////////////////////
void vis::Visualizer::StartPlayer()
{
   

    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        return;
    }

}

/////////////////////////
//  TODO: CleanUp is prolly fine
/////////////////////////
void vis::Visualizer::CleanUp(){
    
    ma_device_uninit(&device);
    if (ma_decoder_uninit(&decoder) != MA_SUCCESS) std::cout<<"Decoder Uninit Failed!\n";

    ma_pcm_rb_uninit(&g_rb);
    ma_mutex_uninit(&fftMutex);
    kiss_fftr_free(fftConfig);
    fftCopyBuffer.clear();
    fftOutput.clear();
    fftBuffer.clear();
    windowFunction.clear();
    std::puts("~vis::Visualizer()");
}

void vis::Visualizer::SwapFFTBuffers(std::vector<kiss_fft_cpx>& output){

    if (canCopyBuffer){
        //puts("RENDER Pre Lock");
        ma_mutex_lock(&fftMutex);//fftLock.lock();
        //puts("RENDER Post Lock");
        canCopyBuffer = false;
        output.swap(fftCopyBuffer);
        //puts("RENDER Pre Unlock");
        ma_mutex_unlock(&fftMutex); // fftLock.unlock();
        //puts("RENDER Post UnLock");
        
        circleBumper = 5*SumUpAvg(output.begin()+10, output.begin()+80);
        
    }


}
