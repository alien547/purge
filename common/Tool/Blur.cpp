#include <cmath>
#include <cstring>
#include <thread>
#include <future>
#include <SDL.h>
#include "Math.h"
#include "Render.h"
#include "Blur.h"
using namespace std;

vector<float> gaussian_kernel(float sigma, int radius){
    vector<float> kernel(2*radius+1);
    float sum=0.0f;
    for(int i=-radius; i<=radius; ++i){
        float x=float(i);
        kernel[i+radius]=exp(-(x*x)/(2.0f*sigma*sigma));
        sum+=kernel[i+radius];
    }
    for(float& w : kernel)w/=sum;
    return kernel;
}

void blur_pass(SDL_Surface* surf, const vector<float>& kernel, bool vertical){
    auto blur_pass_rows=[](SDL_Surface* surf, const vector<int>& int_kernel, bool vertical, int start_y, int end_y, vector<Uint8>& output){
        int w=surf->w, h=surf->h, pitch=surf->pitch, radius=(int_kernel.size()-1)/2;
        const int SCALE_BITS=10;
        Uint8* pixels=(Uint8*)(surf->pixels);
        for(int y=start_y; y<end_y; ++y){
            for(int x=0; x<w; ++x){
                int r=0, g=0, b=0, a=0;
                int c_w=int_kernel[radius];
                Uint8* pc=pixels+y*pitch+x*4;
                r+=c_w*pc[0];
                g+=c_w*pc[1];
                b+=c_w*pc[2];
                a+=c_w*pc[3];
                for(int d=1; d<=radius; ++d){
                    int s_x1=x,s_x2=x,s_y1=y,s_y2=y;
                    if(vertical){
                        s_y1=Math::clamp(y-d, 0, h-1);
                        s_y2=Math::clamp(y+d, 0, h-1);
                    }else{
                        s_x1=Math::clamp(x-d, 0, w-1);
                        s_x2=Math::clamp(x+d, 0, w-1);
                    }
                    if(s_x1==s_x2&&s_y1==s_y2)continue;
                    int w_k=int_kernel[radius+d];
                    Uint8* p1=pixels+s_y1*pitch+s_x1*4;
                    Uint8* p2=pixels+s_y2*pitch+s_x2*4;
                    r+=w_k*(p1[0]+p2[0]);
                    g+=w_k*(p1[1]+p2[1]);
                    b+=w_k*(p1[2]+p2[2]);
                    a+=w_k*(p1[3]+p2[3]);
                }
                int idx=(y*w+x)*4;
                output[idx]=Uint8(r>>SCALE_BITS);
                output[idx+1]=Uint8(g>>SCALE_BITS);
                output[idx+2]=Uint8(b>>SCALE_BITS);
                output[idx+3]=Uint8(a>>SCALE_BITS);
            }
        }
    };
    int w=surf->w, h=surf->h;
    int total_pixels=w*h*4;
    const int SCALE_BITS=10;
    vector<Uint8> output(total_pixels);
    vector<int> int_kernel(kernel.size());
    for(int i=0; i<kernel.size(); ++i)int_kernel[i]=int(kernel[i]*(1<<SCALE_BITS)+0.5f);
    unsigned int num_threads=min(thread::hardware_concurrency(), 8u);
    if(num_threads==0)num_threads=2;
    if(w*h<20000||num_threads<2){
        blur_pass_rows(surf, int_kernel, vertical, 0, h, output);
    }else{
        int rows_per_thread=h/num_threads;
        vector<future<void>> futures;
        for(unsigned int t=0; t<num_threads; ++t){
            int start_y=t*rows_per_thread;
            int end_y=(t==num_threads-1)?h:start_y+rows_per_thread;
            futures.push_back(async(launch::async, [&,start_y, end_y](){blur_pass_rows(surf, int_kernel, vertical, start_y, end_y, output);}));
        }
        for(auto& f : futures)f.wait();
    }
    memcpy(surf->pixels, output.data(), total_pixels);
}

SDL_Texture* create_blurred_texture(SDL_Surface* src, float blur_strength){
    bool converted=false;
    SDL_Surface* work=src;
    if(src->format->BytesPerPixel!=4||src->format->Rmask!=0x000000ff){
        work=SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA8888, 0);
        converted=true;
    }
    int radius=max(int(blur_strength*2.5f), 1);
    vector<float> kernel=gaussian_kernel(blur_strength, radius);
    blur_pass(work, kernel, true);
    blur_pass(work, kernel, false);
    SDL_Texture* result=SDL_CreateTextureFromSurface(renderer, work);
    if(converted)SDL_FreeSurface(work);
    return result;
}

