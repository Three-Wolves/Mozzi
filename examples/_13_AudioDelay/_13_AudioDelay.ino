/*  Example of modulating a delayed_10_Table_Resolution triangle wave,
 *  using Mozzi sonification library.
 *
 *  Demonstrates AudioDelay.
 *
 *  Circuit: Audio output on digital pin 9.
 *
 *  Mozzi help/discussion/announcements:
 *  https://groups.google.com/forum/#!forum/mozzi-users
 *
 *  Tim Barrass 2012.
 *  This example code is in the public domain.
 */

#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/triangle_analogue512_int8.h> // wavetable
#include <tables/cos8192_int8.h> // wavetable
#include <AudioDelay.h>
#include <utils.h> // for mtof

#define CONTROL_RATE 256 // powers of 2 please

Oscil<TRIANGLE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aTriangle(TRIANGLE_ANALOGUE512_DATA);
Oscil<COS8192_NUM_CELLS, CONTROL_RATE> kFreq(COS8192_DATA);

AudioDelay <256> aDel;
int del_samps;

void setup(){
  startMozzi(CONTROL_RATE);
  aTriangle.setFreq(mtof(60.f));
  kFreq.setFreq(.63f);
}

void loop(){
  audioHook();
}

void updateControl(){
  del_samps = 128+kFreq.next();
}

int updateAudio(){
  char asig = aDel.next(aTriangle.next(), del_samps);
  return (int) asig;
}






