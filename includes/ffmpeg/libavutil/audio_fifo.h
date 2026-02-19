



#ifndef AVUTIL_AUDIO_FIFO_H
#define AVUTIL_AUDIO_FIFO_H

#include "attributes.h"
#include "samplefmt.h"




typedef struct AVAudioFifo AVAudioFifo;


void av_audio_fifo_free(AVAudioFifo *af);


AVAudioFifo *av_audio_fifo_alloc(enum AVSampleFormat sample_fmt, int channels,
                                 int nb_samples);


av_warn_unused_result
int av_audio_fifo_realloc(AVAudioFifo *af, int nb_samples);


int av_audio_fifo_write(AVAudioFifo *af, void * const *data, int nb_samples);


int av_audio_fifo_peek(const AVAudioFifo *af, void * const *data, int nb_samples);


int av_audio_fifo_peek_at(const AVAudioFifo *af, void * const *data,
                          int nb_samples, int offset);


int av_audio_fifo_read(AVAudioFifo *af, void * const *data, int nb_samples);


int av_audio_fifo_drain(AVAudioFifo *af, int nb_samples);


void av_audio_fifo_reset(AVAudioFifo *af);


int av_audio_fifo_size(AVAudioFifo *af);


int av_audio_fifo_space(AVAudioFifo *af);



#endif 
