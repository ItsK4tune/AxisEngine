

#ifndef AVUTIL_LFG_H
#define AVUTIL_LFG_H

#include <stdint.h>


typedef struct AVLFG {
    unsigned int state[64];
    int index;
} AVLFG;

void av_lfg_init(AVLFG *c, unsigned int seed);


int av_lfg_init_from_data(AVLFG *c, const uint8_t *data, unsigned int length);


static inline unsigned int av_lfg_get(AVLFG *c){
    unsigned a = c->state[c->index & 63] = c->state[(c->index-24) & 63] + c->state[(c->index-55) & 63];
    c->index += 1U;
    return a;
}


static inline unsigned int av_mlfg_get(AVLFG *c){
    unsigned int a= c->state[(c->index-55) & 63];
    unsigned int b= c->state[(c->index-24) & 63];
    a = c->state[c->index & 63] = 2*a*b+a+b;
    c->index += 1U;
    return a;
}


void av_bmg_get(AVLFG *lfg, double out[2]);

#endif 
