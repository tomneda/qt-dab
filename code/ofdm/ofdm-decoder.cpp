/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of Qt-DAB
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
 *	Once the bits are "in", interpretation and manipulation
 *	should reconstruct the data blocks.
 *	Ofdm_decoder is called for Block_0 and the FIC blocks,
 *	its invocation results in 2 * Tu bits
 */
#include "ofdm-decoder.h"
#include "freq-interleaver.h"
#include "phasetable.h"
#include "radio.h"
#include <vector>

/**
 *	\brief OfdmDecoder
 *	The class OfdmDecoder is
 *	taking the data from the ofdmProcessor class in, and
 *	will extract the Tu samples, do an FFT and extract the
 *	carriers and map them on (soft) bits
 */
OfdmDecoder::OfdmDecoder(RadioInterface * ipMr, uint8_t iDabMode, RingBuffer<cmplx> * ipIqBuffer) :
  mpRadioInterface(ipMr),
  mDabPar(DabParams(iDabMode).get_dab_par()),
  mFreqInterleaver(iDabMode),
  mFftHandler(mDabPar.T_u, false),
  mpIqBuffer(ipIqBuffer)
{
  connect(this, &OfdmDecoder::showIQ, mpRadioInterface, &RadioInterface::showIQ);
  connect(this, &OfdmDecoder::showQuality, mpRadioInterface, &RadioInterface::showQuality);

  mPhaseReference.resize(mDabPar.T_u);
  mFftBuffer.resize(mDabPar.T_u);
  mDataVector.resize(mDabPar.K);
}

void OfdmDecoder::stop()
{}

void OfdmDecoder::reset()
{}

/**
 */
void OfdmDecoder::processBlock_0(std::vector<cmplx> buffer)
{
  mFftHandler.fft(buffer);
  /**
   *	we are now in the frequency domain, and we keep the carriers
   *	as coming from the FFT as phase reference.
   */

  memcpy(mPhaseReference.data(), buffer.data(), mDabPar.T_u * sizeof(cmplx));
}

/**
 *	for the other blocks of data, the first step is to go from
 *	time to frequency domain, to get the carriers.
 *	we distinguish between FIC blocks and other blocks,
 *	only to spare a test. The mapping code is the same
 */

void OfdmDecoder::decode(const std::vector<cmplx> & buffer, int32_t blkno, float iPhaseCorr, std::vector<int16_t> & oBits)
{
  memcpy(mFftBuffer.data(), &(buffer[mDabPar.T_g]), mDabPar.T_u * sizeof(cmplx));

  constexpr float MAX_PHASE_ANGLE = 20.0f / 360.0f * 2.0f * M_PI;
  if (abs(iPhaseCorr) > MAX_PHASE_ANGLE)
  {
    iPhaseCorr = 0;
  }
  // const cmplx phaseRotator = cmplx(cosf(iPhaseCorr), -sinf(iPhaseCorr));
  const cmplx phaseRotator = std::exp(cmplx(0.0f, -iPhaseCorr));

  // fftlabel:
  /**
   *	first step: do the FFT
   */

  mFftHandler.fft(mFftBuffer);

  /**
   *	a little optimization: we do not interchange the
   *	positive/negative frequencies to their right positions.
   *	The de-interleaving understands this
   */
  // toBitsLabel:
  /**
   *	Note that from here on, we are only interested in the
   *	"carriers", the useful carriers of the FFT output
   */
  for (int16_t i = 0; i < mDabPar.K; i++)
  {
    int16_t index = mFreqInterleaver.map_k_to_fft_bin(i);

    if (index < 0)
    {
      index += mDabPar.T_u;
    }

    /**
     *	decoding is computing the phase difference between
     *	carriers with the same index in subsequent blocks.
     *	The carrier of a block is the reference for the carrier
     *	on the same position in the next block
     */

    cmplx r1 = mFftBuffer[index] * conj(mPhaseReference[index]);
    r1 *= phaseRotator;
    mDataVector[i] = r1;
    const float ab1 = abs(r1);

    // split the real and the imaginary part and scale it
    // we make the bits into softbits in the range -127 .. 127 (+/- 255?)
    oBits[i] =             (int16_t)(-(real(r1) * 255.0f) / ab1);
    oBits[mDabPar.K + i] = (int16_t)(-(imag(r1) * 255.0f) / ab1);
  }

  //	From time to time we show the constellation of symbol 2.

  if (blkno == 2)
  {
    if (++mStatisticCnt > 7)
    {
      mpIqBuffer->putDataIntoBuffer(mDataVector.data(), mDabPar.K);
      showIQ(mDabPar.K);

      float quality = compute_mod_quality(mDataVector);
      float timeOffset = compute_time_offset(mFftBuffer, mPhaseReference);
      float freqOffset = compute_frequency_offset(mFftBuffer, mPhaseReference);
      showQuality(quality, timeOffset, freqOffset);

      mStatisticCnt = 0;
    }
  }

  memcpy(mPhaseReference.data(), mFftBuffer.data(), mDabPar.T_u * sizeof(cmplx));
}

float OfdmDecoder::compute_mod_quality(const std::vector<cmplx> & v) const
{
  /*
   * Since we do not equalize, we have a kind of "fake" reference point.
   * The key parameter here is the phase offset, so we compute the std.
   * deviation of the phases rather than the computation from the Modulation
   * Error Ratio as specified in Tr 101 290
   */

  constexpr cmplx rotator = cmplx(1.0f, -1.0f); // this is the reference phase shift to get the phase zero degree
  float squareVal = 0;

  for (int i = 0; i < mDabPar.K; i++)
  {
    float x1 = arg(cmplx(abs(real(v[i])), abs(imag(v[i]))) * rotator); // map to top right section and shift phase to zero (nominal)
    squareVal += x1 * x1;
  }

  return sqrtf(squareVal / (float)mDabPar.K) / (float)M_PI * 180.0f; // in degree
}

//	While DAB symbols do not carry pilots, it is known that
//	arg (carrier [i, j] * conj (carrier [i + 1, j])
//	should be K * M_PI / 4,  (k in {1, 3, 5, 7}) so basically
//	carriers in decoded symbols can be used as if they were pilots
//
//	so, with that in mind we experiment with formula 5.39
//	and 5.40 from "OFDM Baseband Receiver Design for Wireless
//	Communications (Chiueh and Tsai)"
float OfdmDecoder::compute_time_offset(const std::vector<cmplx> & r, const std::vector<cmplx> & v) const
{
  cmplx leftTerm;
  cmplx rightTerm;
  cmplx sum = cmplx(0, 0);

  for (int i = -mDabPar.K / 2; i < mDabPar.K / 2; i += 6)
  {
    int index_1 = i < 0 ? i + mDabPar.T_u : i;
    int index_2 = (i + 1) < 0 ? (i + 1) + mDabPar.T_u : (i + 1);

    cmplx s = r[index_1] * conj(v[index_2]);

    s = cmplx(abs(real(s)), abs(imag(s)));
    leftTerm = s * conj(cmplx(fabs(s) / sqrtf(2), fabs(s) / sqrtf(2)));

    s = r[index_2] * conj(v[index_2]);
    s = cmplx(abs(real(s)), abs(imag(s)));
    rightTerm = s * conj(cmplx(fabs(s) / sqrtf(2), fabs(s) / sqrtf(2)));

    sum += conj(leftTerm) * rightTerm;
  }

  return arg(sum);
}

float OfdmDecoder::compute_frequency_offset(const std::vector<cmplx> & r, const std::vector<cmplx> & c) const
{
  cmplx theta = cmplx(0, 0);

  for (int i = -mDabPar.K / 2; i < mDabPar.K / 2; i += 6)
  {
    int index = i < 0 ? i + mDabPar.T_u : i;
    cmplx val = r[index] * conj(c[index]);
    val = cmplx(abs(real(val)), abs(imag(val)));
    theta += val * cmplx(1, -1);
  }

  return arg(theta) / (2.0f * (float)M_PI) * 2048000.0f / (float)mDabPar.T_u; // TODO: hard coded 2048000?
}

float OfdmDecoder::compute_clock_offset(const cmplx * r, const cmplx * v) const
{
  float offsa = 0;
  int offsb = 0;

  for (int i = -mDabPar.K / 2; i < mDabPar.K / 2; i += 6)
  {
    int index = i < 0 ? (i + mDabPar.T_u) : i;
    int index_2 = i + mDabPar.K / 2;
    cmplx a1 = cmplx(abs(real(r[index])), abs(imag(r[index])));
    cmplx a2 = cmplx(abs(real(v[index])), abs(imag(v[index])));
    float s = abs(arg(a1 * conj(a2)));
    offsa += (float)index * s;
    offsb += index_2 * index_2;
  }

  return offsa / (2.0f * (float)M_PI * (float)mDabPar.T_s / (float)mDabPar.T_u * (float)offsb);
}
