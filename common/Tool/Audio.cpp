#include <queue>
#include <mutex>
#include <cmath>
#include <unordered_map>
#include <SDL_mixer.h>
#include "Math.h"
#include "Settings.h"
#include "Audio.h"
using namespace std;
using namespace Settings;

struct PairHash{
    size_t operator()(const std::pair<int, int>& p)const{
        return ((uint64_t(p.first))<<32)|uint32_t(p.second);
    }
};

struct BeepCache{
    std::vector<Sint16> samples;
    Mix_Chunk* chunk;
};

unordered_map<pair<int, int>, BeepCache, PairHash> beep_cache;
unordered_map<string, Mix_Chunk*> sfx_cache;
unordered_map<string, Mix_Music*> music_cache;

queue<int> free_channels;
mutex audio_mutex;
bool sound_on=true;
string sfx_path, music_path;
string pending_music;

void init_audio(const string& sfx_path_, const string& music_path_){
    sfx_path=sfx_path_;
    music_path=music_path_;
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    Mix_AllocateChannels(TOTAL_CHANNELS);
    Mix_Volume(-1, MIX_MAX_VOLUME);
    for(int i=RESERVED_CHANNELS; i<TOTAL_CHANNELS; ++i)free_channels.push(i);
    Mix_ChannelFinished([](int ch){
        if(ch>=RESERVED_CHANNELS){
            lock_guard<mutex> lock(audio_mutex);
            free_channels.push(ch);
        }
    });
    Mix_HookMusicFinished([](){
        if(!pending_music.empty()){
            string next=pending_music;
            pending_music.clear();
            do_play_music(next);
        }
    });
}

void cleanup_audio(){
    Mix_CloseAudio();
    for(auto& p : beep_cache)Mix_FreeChunk(p.second.chunk);
    for(auto& s : sfx_cache)Mix_FreeChunk(s.second);
    for(auto& m : music_cache)Mix_FreeMusic(m.second);
}

void beep(pair<int, int> info){
    if(!sound_on)return;
    auto it=beep_cache.find(info);
    if(it==beep_cache.end()){
        int sample_rate;
        Mix_QuerySpec(&sample_rate, nullptr, nullptr);
        const int num_samples=sample_rate*info.second/1000;
        vector<Sint16> samples(num_samples);
        for(int i=0; i<num_samples; ++i){
            samples[i]=Sint16(30000*sin(2.0*PI*info.first*double(i)/sample_rate));
        }
        Mix_Chunk* chunk=Mix_QuickLoad_RAW((Uint8*)(samples.data()), samples.size()*sizeof(Sint16));
        beep_cache[info]={move(samples), chunk};
        it=beep_cache.find(info);
    }
    Mix_PlayChannel(-1, it->second.chunk, 0);
}

int play_sfx(const string& name, int loops, int channel){
    if(!sound_on)return -1;
    Mix_Chunk* mix;
    auto it=sfx_cache.find(name);
    if(it==sfx_cache.end()){
        mix=Mix_LoadWAV((sfx_path+name+".wav").c_str());
        sfx_cache[name]=mix;
    }else{
        mix=it->second;
    }
    if(channel==-1){
        lock_guard<mutex> lock(audio_mutex);
        if(free_channels.empty())return -1;
        channel=free_channels.front();
        free_channels.pop();
    }
    if(Mix_PlayChannel(channel, mix, loops)==-1){
        lock_guard<mutex> lock(audio_mutex);
        free_channels.push(channel);
        return -1;
    }
    return channel;
}

void stop_sfx(int channel){
    Mix_HaltChannel(channel);
}

void do_play_music(const string& name){
    Mix_Music* music;
    auto it=music_cache.find(name);
    if(it==music_cache.end()){
        music=Mix_LoadMUS((music_path+name+".wav").c_str());
        music_cache[name]=music;
    }else{
        music=it->second;
    }
    Mix_HaltMusic();
    Mix_PlayMusic(music, -1);
    Mix_VolumeMusic(64);
}

void play_music(const string& name){
    if(!sound_on)return;
    pending_music=name;
    if(Mix_PlayingMusic()){
        Mix_FadeOutMusic(500);
    }else{
        do_play_music(name);
    }
}

void stop_music(){
    Mix_FadeOutMusic(500);
}

