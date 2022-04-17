/*
 *    Copyright (C) 2013 .. 2017
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

#include  "sample-reader.h"
#include  "radio.h"

static inline int16_t valueFor(int16_t b)
{
  int16_t res = 1;

  while (--b > 0)
  {
    res <<= 1;
  }

  return res;
}

static TIQSmpFlt oscillatorTable[INPUT_RATE];

sampleReader::sampleReader(RadioInterface *mr, deviceHandler *theRig, RingBuffer<TIQSmpFlt> *spectrumBuffer)
{
  mpTheRig         = theRig;
  mBufferSize      = 32768;
  mpSpectrumBuffer = spectrumBuffer;

  connect(this, &sampleReader::show_Spectrum, mr, &RadioInterface::showSpectrum);

  mLocalBuffer.resize(mBufferSize);
  mLocalCounter = 0;

  connect(this, &sampleReader::show_Corrector, mr, &RadioInterface::set_CorrectorDisplay);

  mCurrentPhase = 0;
  mSLevel       = 0;
  mSampleCount  = 0;

  for (int i = 0; i < INPUT_RATE; i++)
  {
    oscillatorTable [i] = TIQSmpFlt(cos(2.0 * M_PI * i / INPUT_RATE), sin(2.0 * M_PI * i / INPUT_RATE));
  }

  mBufferContent = 0;
  mCorrector     = 0;

  mDumpfilePointer.store(nullptr);

  mDumpIndex = 0;
  mDumpScale = valueFor(theRig->bitDepth());

  mRunning.store(true);
}

void sampleReader::setRunning(bool b)
{
  mRunning.store(b);
}

float sampleReader::get_sLevel()
{
  return mSLevel;
}

TIQSmpFlt sampleReader::getSample(int32_t phaseOffset)
{
  TIQSmpFlt temp;

  mCorrector = phaseOffset;

  if (!mRunning.load())
  {
    throw 21;
  }

  // bufferContent is an indicator for the value of ... -> Samples()
  if (mBufferContent == 0)
  {
    mBufferContent = mpTheRig->Samples();

    while ((mBufferContent <= 2048) && mRunning.load())
    {
      usleep(10);
      mBufferContent = mpTheRig->Samples();
    }
  }

  if (!mRunning.load())
  {
    throw 20;
  }

  // so here, bufferContent > 0
  mpTheRig->getSamples(&temp, 1);

  mBufferContent--;

  if (mDumpfilePointer.load() != nullptr)
  {
    mDumpBuffer[2 * mDumpIndex    ] = real(temp) * mDumpScale;
    mDumpBuffer[2 * mDumpIndex + 1] = imag(temp) * mDumpScale;

    if (++mDumpIndex >= DUMPSIZE / 2)
    {
      sf_writef_short(mDumpfilePointer.load(),
                      mDumpBuffer, mDumpIndex);
      mDumpIndex = 0;
    }
  }

  if (mLocalCounter < mBufferSize)
  {
    mLocalBuffer [mLocalCounter++] = temp;
  }
//
//	OK, we have a sample!!
//	first: adjust frequency. We need Hz accuracy
  mCurrentPhase -= phaseOffset;
  mCurrentPhase  = (mCurrentPhase + INPUT_RATE) % INPUT_RATE;

  temp  *= oscillatorTable [mCurrentPhase];
  mSLevel = 0.00001 * jan_abs(temp) + (1 - 0.00001) * mSLevel;

  #define N    4

  if (++mSampleCount > INPUT_RATE / N)
  {
    show_Corrector(mCorrector);
    mSampleCount = 0;

    if (mpSpectrumBuffer != nullptr)
    {
      mpSpectrumBuffer->putDataIntoBuffer(mLocalBuffer.data(),
                                        mLocalCounter);
      emit show_Spectrum(mBufferSize);
    }

    mLocalCounter = 0;
  }

  return temp;
}

void sampleReader::getSamples(TIQSmpFlt  *v, int32_t n, int32_t phaseOffset)
{
  int32_t i;

  mCorrector = phaseOffset;

  if (!mRunning.load())
  {
    throw 21;
  }

  if (n > mBufferContent)
  {
    mBufferContent = mpTheRig->Samples();

    while ((mBufferContent < n) && mRunning.load())
    {
      usleep(10);
      mBufferContent = mpTheRig->Samples();
    }
  }

  if (!mRunning.load())
  {
    throw 20;
  }

  // so here, bufferContent >= n
  n = mpTheRig->getSamples(v, n);
  mBufferContent -= n;

  if (mDumpfilePointer.load() != nullptr)
  {
    for (i = 0; i < n; i++)
    {
      mDumpBuffer[2 * mDumpIndex    ] = real(v [i]) * mDumpScale;
      mDumpBuffer[2 * mDumpIndex + 1] = imag(v [i]) * mDumpScale;

      if (++mDumpIndex >= DUMPSIZE / 2)
      {
        sf_writef_short(mDumpfilePointer.load(), mDumpBuffer, mDumpIndex);
        mDumpIndex = 0;
      }
    }
  }

  // OK, we have samples!!
  // first: adjust frequency. We need Hz accuracy
  for (i = 0; i < n; i++)
  {
    mCurrentPhase -= phaseOffset;

    // Note that "phase" itself might be negative
    mCurrentPhase = (mCurrentPhase + INPUT_RATE) % INPUT_RATE;

    if (mLocalCounter < mBufferSize)
    {
      mLocalBuffer[mLocalCounter++] = v [i];
    }

    v [i] *= oscillatorTable [mCurrentPhase];
    mSLevel = 0.00001 * jan_abs(v [i]) + (1 - 0.00001) * mSLevel;
  }

  mSampleCount += n;

  if (mSampleCount > INPUT_RATE / N)
  {
    show_Corrector(mCorrector);

    if (mpSpectrumBuffer != nullptr)
    {
      mpSpectrumBuffer->putDataIntoBuffer(mLocalBuffer.data(), mBufferSize);
      emit show_Spectrum(mBufferSize);
    }

    mLocalCounter = 0;
    mSampleCount  = 0;
  }
}

void sampleReader::startDumping(SNDFILE *f)
{
  mDumpfilePointer.store(f);
}

void sampleReader::stopDumping()
{
  mDumpfilePointer.store(nullptr);
}
