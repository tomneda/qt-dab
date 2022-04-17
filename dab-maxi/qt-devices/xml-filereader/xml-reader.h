/*
 *    Copyright (C) 2014 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB
 *
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-DAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef __XML_READER__
#define __XML_READER__

#include  "dab-constants.h"
#include  "ringbuffer.h"
#include  <QMessageBox>
#include  <QThread>
#include  <atomic>
#include  <complex>
#include  <stdint.h>
#include  <stdio.h>
#include  <vector>

class xml_fileReader;
class xmlDescriptor;

class	xml_Reader:public QThread
{
Q_OBJECT
public:
  xml_Reader(xml_fileReader * mr,
             FILE * f,
             xmlDescriptor * fd,
             uint32_t filePointer,
             RingBuffer<TIQSmpFlt> *b);
  ~xml_Reader ();
  void stopReader();
  void handle_continuousButton();

private:
  std::atomic<bool>      continuous;
  FILE                   *file;
  xmlDescriptor          *fd;
  uint32_t               filePointer;
  RingBuffer<TIQSmpFlt> *sampleBuffer;
  xml_fileReader         *parent;
  int                    nrElements;
  int                    samplesToRead;
  std::atomic<bool>      running;

  void run() override;

  int compute_nrSamples(FILE *f, int blockNumber);
  int readSamples(FILE * f, void (xml_Reader::*)(FILE *, TIQSmpFlt *, int));
  void readElements_IQ(FILE *f, TIQSmpFlt *, int amount);
  void readElements_QI(FILE *f, TIQSmpFlt *, int amount);
  void readElements_I(FILE *f, TIQSmpFlt *, int amount);
  void readElements_Q(FILE *f, TIQSmpFlt *, int amount);

  // for the conversion - if any
  int16_t                convBufferSize;
  int16_t                convIndex;
  std::vector<TIQSmpFlt> convBuffer;
  int16_t                mapTable_int[2048];
  float                  mapTable_float[2048];

signals:
  void setProgress(int, int);
};

#endif
