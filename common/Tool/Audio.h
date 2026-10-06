#ifndef AUDIO_H
#define AUDIO_H

#include <utility>
#include <string>

extern bool sound_on;

void init_audio(const std::string& sfx_path_, const std::string& music_path_);
void cleanup_audio();
void beep(std::pair<int, int> info);
int play_sfx(const std::string& name, int loops=0, int channel=-1);
void stop_sfx(int channel);
void do_play_music(const std::string& name);
void play_music(const std::string& name);
void stop_music();

#endif

