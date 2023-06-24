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
#include  <time.h>

static inline int16_t value_for_bit_pos(int16_t b)
{
  assert(b > 0);
  int16_t res = 1;
  while (--b > 0)
  {
    res <<= 1;
  }
  return res;
}

SampleReader::SampleReader(const RadioInterface * mr, deviceHandler * iTheRig, RingBuffer<cmplx> * iSpectrumBuffer) :
  theRig(iTheRig),
  spectrumBuffer(iSpectrumBuffer)
{
  localBuffer.resize(bufferSize);
  dumpfilePointer.store(nullptr);
  dumpScale = value_for_bit_pos(theRig->bitDepth());
  running.store(true);

  for (int i = 0; i < INPUT_RATE; i++)
  {
    oscillatorTable[i] = cmplx((float)cos(2.0 * M_PI * i / INPUT_RATE),
                               (float)sin(2.0 * M_PI * i / INPUT_RATE));
  }

  connect(this, SIGNAL (show_Spectrum(int)), mr, SLOT (showSpectrum(int)));
  connect(this, SIGNAL (show_Corrector(int)), mr, SLOT (set_CorrectorDisplay(int)));
}

void SampleReader::setRunning(bool b)
{
  running.store(b);
}

float SampleReader::get_sLevel() const
{
  return sLevel;
}

cmplx SampleReader::getSample(int32_t phaseOffset)
{
  corrector = phaseOffset;
  if (!running.load())
  {
    throw 21;
  }

  ///	bufferContent is an indicator for the value of ... -> Samples()
  if (bufferContent == 0)
  {
    bufferContent = theRig->Samples();
    while ((bufferContent <= 2048) && running.load())
    {
      constexpr timespec ts{ 0, 10'000L };
      while (nanosleep(&ts, nullptr));
      bufferContent = theRig->Samples();
    }
  }

  if (!running.load())
  {
    throw 20;
  }
  //
  //	so here, bufferContent > 0

  cmplx temp;
  theRig->getSamples(&temp, 1);
  --bufferContent;

  if (dumpfilePointer.load() != nullptr)
  {
    dumpBuffer[2 * dumpIndex + 0] = fixround<int16_t>(real(temp) * dumpScale);
    dumpBuffer[2 * dumpIndex + 1] = fixround<int16_t>(imag(temp) * dumpScale);

    if (++dumpIndex >= DUMPSIZE / 2)
    {
      sf_writef_short(dumpfilePointer.load(), dumpBuffer.data(), dumpIndex);
      dumpIndex = 0;
    }
  }

  if (localCounter < bufferSize)
  {
    localBuffer[localCounter] = temp;
    ++localCounter;
  }
  //
  //	OK, we have a sample!!
  //	first: adjust frequency. We need Hz accuracy
  currentPhase -= phaseOffset;
  currentPhase = (currentPhase + INPUT_RATE) % INPUT_RATE;

  temp *= oscillatorTable[currentPhase];
  sLevel = 0.00001f * jan_abs(temp) + (1.0f - 0.00001f) * sLevel;

  if (++sampleCount > INPUT_RATE / 4)
  {
    show_Corrector(corrector);
    sampleCount = 0;
    if (spectrumBuffer != nullptr)
    {
      spectrumBuffer->putDataIntoBuffer(localBuffer.data(), localCounter);
      emit show_Spectrum(bufferSize);
    }
    localCounter = 0;
  }
  return temp;
}

void SampleReader::getSamples(std::vector<cmplx> & v, int index, int32_t n, int32_t phaseOffset)
{
  std::vector<cmplx> buffer(n);

  corrector = phaseOffset;
  if (!running.load())
  {
    throw 21;
  }
  if (n > bufferContent)
  {
    bufferContent = theRig->Samples();
    while ((bufferContent < n) && running.load())
    {
      constexpr timespec ts{ 0, 10'000L };
      while (nanosleep(&ts, nullptr));
      bufferContent = theRig->Samples();
    }
  }

  if (!running.load())
  {
    throw 20;
  }
  //
  //	so here, bufferContent >= n
  n = theRig->getSamples(buffer.data(), n);
  bufferContent -= n;
  if (dumpfilePointer.load() != nullptr)
  {
    _dump_samples_to_file(v, n);
  }

  //	OK, we have samples!!
  //	first: adjust frequency. We need Hz accuracy
  for (int32_t i = 0; i < n; i++)
  {
    currentPhase -= phaseOffset;
    //
    //	Note that "phase" itself might be negative
    currentPhase = (currentPhase + INPUT_RATE) % INPUT_RATE;
    if (localCounter < bufferSize)
    {
      localBuffer[localCounter] = v[i];
      ++localCounter;
    }
    v[index + i] = buffer[i] * oscillatorTable[currentPhase];
    sLevel = 0.00001f * jan_abs(v[i]) + (1.0f - 0.00001f) * sLevel;
  }

  sampleCount += n;

  if (sampleCount > INPUT_RATE / 4)
  {
    show_Corrector(corrector);
    if (spectrumBuffer != nullptr)
    {
      spectrumBuffer->putDataIntoBuffer(localBuffer.data(), bufferSize);
      emit show_Spectrum(bufferSize);
    }
    localCounter = 0;
    sampleCount = 0;
  }
}

void SampleReader::_dump_samples_to_file(const std::vector<cmplx> & v, int32_t n)
{
  for (int32_t i = 0; i < n; i++)
  {
    dumpBuffer[2 * dumpIndex + 0] = fixround<int16_t>(real(v[i]) * dumpScale);
    dumpBuffer[2 * dumpIndex + 1] = fixround<int16_t>(imag(v[i]) * dumpScale);

    if (++dumpIndex >= DUMPSIZE / 2)
    {
      sf_writef_short(dumpfilePointer.load(), dumpBuffer.data(), dumpIndex);
      dumpIndex = 0;
    }
  }
}

void SampleReader::startDumping(SNDFILE * f)
{
  dumpfilePointer.store(f);
}

void SampleReader::stopDumping()
{
  dumpfilePointer.store(nullptr);
}

