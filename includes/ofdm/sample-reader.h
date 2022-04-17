/*
 *    Copyright (C) 2013 .. 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB program
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
 */
#
#ifndef	__SAMPLE_READER__
#define	__SAMPLE_READER__
/*
 *	Reading the samples from the input device. Since it has its own
 *	"state", we embed it into its own class
 */
#include	"dab-constants.h"
#include	<QObject>
#include	<sndfile.h>
#include	<cstdint>
#include	<atomic>
#include	<vector>
#include	"device-handler.h"
#include	"ringbuffer.h"

//      Note:
//      It was found that enlarging the buffersize to e.g. 8192
//      cannot be handled properly by the underlying system.

#define DUMPSIZE                4096

class	RadioInterface;
class	sampleReader : public QObject
{
  Q_OBJECT
public:
  sampleReader(RadioInterface * mr, deviceHandler * theRig, RingBuffer<TIQSmpFlt> *spectrumBuffer = nullptr);
  ~sampleReader() override = default;

  void setRunning(bool b);
  float get_sLevel();
  TIQSmpFlt getSample(int32_t);
  void getSamples(TIQSmpFlt *v, int32_t n, int32_t phase);
  void startDumping(SNDFILE *);
  void stopDumping();

private:
  RadioInterface                    *mpMyRadioInterface{};
  deviceHandler                     *mpTheRig;
  RingBuffer<TIQSmpFlt>  *mpSpectrumBuffer;
  std::vector<TIQSmpFlt> mLocalBuffer;
  int32_t                           mLocalCounter;
  int32_t                           mBufferSize;
  int32_t                           mCurrentPhase;
  std::atomic<bool>                 mRunning{};
  int32_t                           mBufferContent;
  float                             mSLevel;
  int32_t                           mSampleCount;
  int32_t                           mCorrector;
  //bool                              mDumping{};
  int16_t                           mDumpIndex;
  int16_t                           mDumpScale;
  int16_t                           mDumpBuffer[DUMPSIZE]{};
  std::atomic<SNDFILE *>            mDumpfilePointer{};

signals:
  void show_Spectrum(int);
  void show_Corrector(int);
};

#endif
