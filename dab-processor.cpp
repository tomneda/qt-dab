/*
 *    Copyright (C) 2014 .. 2020
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
 *    along with Qt-DAB if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include  "dab-processor.h"

#include  "dab-params.h"
#include  "fic-handler.h"
#include  "msc-handler.h"
#include  "process-params.h"
#include  "radio.h"
#include  "timesyncer.h"
#include  <utility>

/**
 *	\brief dabProcessor
 *	The dabProcessor class is the driver of the processing
 *	of the samplestream.
 *	It is the main interface to the qt-dab program,
 *	local are classes ofdmDecoder, ficHandler and mschandler.
 */

dabProcessor::dabProcessor(RadioInterface *mr,
                           deviceHandler  *inputDevice,
                           processParams  *p)
  : mParams(p->dabMode)
  , mMyReader(mr, inputDevice, p->spectrumBuffer)
  , mMy_ficHandler(mr, p->dabMode)
  , mMy_mscHandler(mr, p->dabMode, p->frameBuffer)
  , mPhaseSynchronizer(mr, p)
  , mMy_TII_Detector(p->dabMode, p->tii_depth)
  , mMy_ofdmDecoder(mr, p->dabMode, inputDevice->bitDepth(), p->iqBuffer)
{
  mpMyRadioInterface = mr;
  mpInputDevice      = inputDevice;
  mFrequency         = 220'000'000;  // default
  mThreshold         = p->threshold;
  mpTiiBuffer        = p->tiiBuffer;
  mpSnrBuffer        = p->snrBuffer;
  mT_null            = mParams.get_T_null();
  mT_s               = mParams.get_T_s();
  mT_u               = mParams.get_T_u();
  mT_g               = mT_s - mT_u;
  mT_F               = mParams.get_T_F();
  mNrBlocks          = mParams.get_L();
  mCarriers          = mParams.get_carriers();
  mCarrierDiff       = mParams.get_carrierDiff();

  mTii_delay   = p->tii_delay;
  mTii_counter = 0;

  mOfdmBuffer.resize(2 * mT_s);
  mFineOffset       = 0;
  mCoarseOffset     = 0;
  mCorrectionNeeded = true;
  mAttempts         = 0;

  mGoodFrames  = 0;
  mBadFrames   = 0;
  mTotalFrames = 0;
  mScanMode    = false;

  connect(this, &dabProcessor::setSynced,                mpMyRadioInterface, &RadioInterface::setSynced);
  connect(this, &dabProcessor::setSyncLost,              mpMyRadioInterface, &RadioInterface::setSyncLost);
  connect(this, &dabProcessor::show_Spectrum,            mpMyRadioInterface, &RadioInterface::showSpectrum);
  connect(this, &dabProcessor::show_tii,                 mpMyRadioInterface, &RadioInterface::show_tii);
  connect(this, &dabProcessor::show_tii_spectrum,        mpMyRadioInterface, &RadioInterface::show_tii_spectrum);
  connect(this, qOverload<int>(&dabProcessor::show_snr), mpMyRadioInterface, &RadioInterface::show_snr);
  connect(this, &dabProcessor::show_clockErr,            mpMyRadioInterface, &RadioInterface::show_clockError);
  connect(this, &dabProcessor::show_null,                mpMyRadioInterface, &RadioInterface::show_null);

  mMy_TII_Detector.reset();
}

dabProcessor::~dabProcessor()
{
  if (isRunning())
  {
    mMyReader.setRunning(false);
    // exception to be raised
    // through the getSample(s) functions.
    msleep(100);

    while (isRunning())
    {
      usleep(100);
    }
  }
}

void dabProcessor::set_tiiDetectorMode(bool b)
{
  mMy_TII_Detector.setMode(b);
}

void dabProcessor::start(int frequency)
{
  mFrequency = frequency;

  mMy_ficHandler.reset();
  mTransmitters.clear();

  if (!mScanMode)
  {
    mMy_mscHandler.reset_Channel();
  }

  QThread::start();
}

void dabProcessor::stop()
{
  mMyReader.setRunning(false);

  while (isRunning())
  {
    wait();
  }

  usleep(10000);
}

/***
 *	\brief run
 *	The main thread, reading samples,
 *	time synchronization and frequency synchronization
 *	Identifying blocks in the DAB frame
 *	and sending them to the ofdmDecoder who will transfer the results
 *	Finally, estimating the small freqency error
 */
void dabProcessor::run()
{
  int32_t              startIndex;
  int32_t              i;
  TIQSmpFlt  FreqCorr;
  timeSyncer           myTimeSyncer(&mMyReader);
  int                  attempts;
  std::vector<int16_t> ibits;
  int                  frameCount   = 0;
  int                  sampleCount  = 0;
  int                  totalSamples = 0;
  double               cLevel       = 0;
  int                  cCount       = 0;
  //bool                 dumpvlag     = false;
  QVector<TIQSmpFlt> tester(mT_null / 2 + mT_u);

  //inputDevice->resetBuffer ();
  //inputDevice->restartReader (frequency);
  ibits.resize(2 * mParams.get_carriers());

  mFineOffset       = 0;
  mCoarseOffset     = 0;
  mCorrectionNeeded = true;
  attempts         = 0;
  mMyReader.setRunning(true);    // useful after a restart

  // to get some idea of the signal strength
  try
  {
    for (i = 0; i < mT_F / 5; i++)
    {
      mMyReader.getSample(0);
    }

  // Initing:
 notSynced:
    mTotalFrames++;
    totalSamples = 0;
    frameCount   = 0;
    sampleCount  = 0;

    setSynced(false);
    mMy_TII_Detector.reset();

    switch (myTimeSyncer.sync(mT_null, mT_F))
    {
    case TIMESYNC_ESTABLISHED:
      break;          // yes, we are ready

    case NO_DIP_FOUND:
      if (++attempts >= 8)
      {
        emit(No_Signal_Found());
        attempts = 0;
      }
      goto notSynced;

    default:          // does not happen
    case NO_END_OF_DIP_FOUND:
      goto notSynced;
    }

    mMyReader.getSamples(mOfdmBuffer.data(), mT_u, mCoarseOffset + mFineOffset);

    /**
     *	Looking for the first sample of the T_u part of the sync block.
     *	Note that we probably already had 30 to 40 samples of the T_g
     *	part
     */
    startIndex = mPhaseSynchronizer.findIndex(mOfdmBuffer, mThreshold);

    if (startIndex < 0)    // no sync, try again
    {
      if (!mCorrectionNeeded)
      {
        setSyncLost();
      }
      mBadFrames++;
      goto notSynced;
    }

    sampleCount = startIndex;
    goto SyncOnPhase;

 Check_endofNULL:
    mTotalFrames++;
    frameCount++;
    totalSamples += sampleCount;

    if (frameCount > 10)
    {
      show_clockErr(totalSamples - frameCount * 196608);
      totalSamples = 0;
      frameCount   = 0;
    }

    mMyReader.getSamples(mOfdmBuffer.data(), mT_u, mCoarseOffset + mFineOffset);
#ifdef  __SHOW_BLOCK_0_
    static int testteller = 0;
    testteller++;
    if (testteller >= 5)
    {
      testteller = 0;
      for (int i = 0; i < T_u; i++)
      {
        tester [T_null / 2 + i] = ofdmBuffer [i];
      }
      tiiBuffer->putDataIntoBuffer(tester.data(), tester.size());
      show_null(tester.size());
    }
#endif

    /**
     *	We now have to find the exact first sample of the non-null period.
     *	We use a correlation that will find the first sample after the
     *	cyclic prefix.
     */
    startIndex = mPhaseSynchronizer.findIndex(mOfdmBuffer,
                                             3 * mThreshold);
    if (startIndex < 0)    // no sync, try again
    {
      if (!mCorrectionNeeded)
      {
        setSyncLost();
      }
      mBadFrames++;
      goto notSynced;
    }

    sampleCount = startIndex;

 SyncOnPhase:
    mGoodFrames++;
    cLevel = 0;
    cCount = 0;

    /**
     *	Once here, we are synchronized, we need to copy the data we
     *	used for synchronization for block 0
     */
    memmove(mOfdmBuffer.data(),
            &((mOfdmBuffer.data()) [startIndex]),
            (mT_u - startIndex) * sizeof(TIQSmpFlt));
    int ofdmBufferIndex = mT_u - startIndex;

    //Block_0:
    /**
     *	Block 0 is special in that it is used for fine time synchronization,
     *	for coarse frequency synchronization
     *	and its content is used as a reference for decoding the
     *	first datablock.
     *	We read the missing samples in the ofdm buffer
     */
    setSynced(true);
    mMyReader.getSamples(&((mOfdmBuffer.data())[ofdmBufferIndex]),
                        mT_u - ofdmBufferIndex,
                        mCoarseOffset + mFineOffset);

#ifdef  __WITH_JAN__
    static int abc = 0;

    if (++abc > 10)
    {
      phaseSynchronizer.estimate(ofdmBuffer);
      abc = 0;
    }
#endif
    sampleCount += mT_u;
    mMy_ofdmDecoder.processBlock_0(mOfdmBuffer);

    if (!mScanMode)
    {
      mMy_mscHandler.processBlock_0(mOfdmBuffer.data());
    }

//	Here we look only at the block_0 when we need a coarse
//	frequency synchronization.
    mCorrectionNeeded = !mMy_ficHandler.syncReached();

    if (mCorrectionNeeded)
    {
      int correction =
        mPhaseSynchronizer.estimate_CarrierOffset(mOfdmBuffer);
      if (correction != 100)
      {
        mCoarseOffset += 0.4 * correction * mCarrierDiff;
        if (abs(mCoarseOffset) > Khz(35))
        {
          mCoarseOffset = 0;
        }
      }
    }

    /**
     *	after block 0, we will just read in the other (params -> L - 1) blocks
     */

    //Data_blocks:
    /**
     *	The first ones are the FIC blocks these are handled within
     *	the thread executing this "task", the other blocks
     *	are passed on to be handled in the mscHandler, running
     *	in a different thread.
     *	We immediately start with building up an average of
     *	the phase difference between the samples in the cyclic prefix
     *	and the	corresponding samples in the datapart.
     */
    cCount   = 0;
    cLevel   = 0;
    FreqCorr = TIQSmpFlt (0, 0);

    for (int ofdmSymbolCount = 1; ofdmSymbolCount < mNrBlocks; ofdmSymbolCount++)
    {
      mMyReader.getSamples(mOfdmBuffer.data(), mT_s, mCoarseOffset + mFineOffset);
      sampleCount += mT_s;

      for (i = mT_u; i < mT_s; i++)
      {
        FreqCorr += mOfdmBuffer [i] * conj(mOfdmBuffer [i - mT_u]);
        cLevel   += abs(mOfdmBuffer [i]) + abs(mOfdmBuffer [i - mT_u]);
      }

      cCount += 2 * mT_g;

      if (ofdmSymbolCount < 4)
      {
        mMy_ofdmDecoder.decode(mOfdmBuffer,
                              ofdmSymbolCount, ibits.data());
        mMy_ficHandler.process_ficBlock(ibits, ofdmSymbolCount);
      }

      if (!mScanMode)
      {
        mMy_mscHandler.process_Msc(&((mOfdmBuffer.data()) [mT_g]), ofdmSymbolCount);
      }
    }

    /**
     *	OK,  here we are at the end of the frame
     *	Assume everything went well and skip T_null samples
     */
    mMyReader.getSamples(mOfdmBuffer.data(), mT_null, mCoarseOffset + mFineOffset);
#ifdef  __SHOW_BLOCK_0_
    for (int i = 0; i < T_null / 2; i++)
    {
      tester [i] = ofdmBuffer [T_null / 2 + i];
    }
#endif

    sampleCount += mT_null;
    float sum = 0;

    for (i = 0; i < mT_null; i++)
    {
      sum += abs(mOfdmBuffer [i]);
    }

    sum /= mT_null;

    if (mpSnrBuffer != nullptr)
    {
      float snrV = 20 * log10f((cLevel / cCount + 0.005f) / (sum + 0.005f));
      mpSnrBuffer->putDataIntoBuffer(&snrV, 1);
    }

    static float snr = 0;
    static int   ccc = 0;
    ccc++;

    if (ccc >= 5)
    {
      ccc = 0;
      snr = 0.9f * snr +
            0.1f * 20.0f * log10f((mMyReader.get_sLevel() + 0.005f) / (sum + 0.005f));
      show_snr(static_cast<int>(snr));
    }

    /*
     *	The TII data is encoded in the null period of the
     *	odd frames
     */
#ifndef __SHOW_BLOCK_0_
    if (mParams.get_dabMode() == 1)
    {
      if (wasSecond(mMy_ficHandler.get_CIFcount(), &mParams))
      {
        mMy_TII_Detector.addBuffer(mOfdmBuffer);

        if (++mTii_counter >= mTii_delay)
        {
          mpTiiBuffer->putDataIntoBuffer(mOfdmBuffer.data(), mT_u);

          show_tii_spectrum();

          const TII_Detector::STiiInfo tiiInfo = mMy_TII_Detector.processNULL();
          emit show_tii(tiiInfo.Result, tiiInfo.MainId, tiiInfo.SubId);

          mTii_counter = 0;
          mMy_TII_Detector.reset();
        }
      }
    }
#endif
/**
 *	The first sample to be found for the next frame should be T_g
 *	samples ahead. Before going for the next frame, we
 *	we just check the fineCorrector
 */
    //  NewOffset:
    //    we integrate the newly found frequency error with the
    //    existing frequency error.

    mFineOffset += 0.05 * arg(FreqCorr) / (2 * M_PI) * mCarrierDiff;

    if (mFineOffset > mCarrierDiff / 2)
    {
      mCoarseOffset += mCarrierDiff;
      mFineOffset   -= mCarrierDiff;
    }
    else if (mFineOffset < -mCarrierDiff / 2)
    {
      mCoarseOffset -= mCarrierDiff;
      mFineOffset   += mCarrierDiff;
    }

    // ReadyForNewFrame:
    //	and off we go, up to the next frame
    goto Check_endofNULL;
  }
  catch (int e)
  {
    //fprintf (stderr, "dabProcessor is stopping\n");
    ;
  }
//	inputDevice	-> stopReader ();
}

//
//
void dabProcessor::set_scanMode(bool b)
{
  mScanMode = b;
  mAttempts = 0;
}

void dabProcessor::getFrameQuality(int  *totalFrames, int  *goodFrames, int  *badFrames)
{
  *totalFrames      = mTotalFrames;
  *goodFrames       = mGoodFrames;
  *badFrames        = mBadFrames;
  mTotalFrames = 0;
  mGoodFrames  = 0;
  mBadFrames   = 0;
}

//	just convenience functions
//	ficHandler abstracts channel data

QString dabProcessor::findService(uint32_t SId, int SCIds)
{
  return mMy_ficHandler.findService(SId, SCIds);
}

void dabProcessor::getParameters(const QString &s, uint32_t * p_SId, int * p_SCIds)
{
  mMy_ficHandler.getParameters(s, p_SId, p_SCIds);
}

std::vector<serviceId>  dabProcessor::getServices(int n)
{
  return mMy_ficHandler.getServices(n);
}

bool dabProcessor::is_audioService(const QString &s)
{
  audiodata ad;

  mMy_ficHandler.dataforAudioService(s, &ad);
  return ad.defined;
}

int dabProcessor::getSubChId(const QString &s, uint32_t SId)
{
  return mMy_ficHandler.getSubChId(s, SId);
}

bool dabProcessor::is_packetService(const QString &s)
{
  packetdata pd;

  mMy_ficHandler.dataforPacketService(s, &pd, 0);
  return pd.defined;
}

void dabProcessor::dataforAudioService(const QString &s, audiodata *d)
{
  mMy_ficHandler.dataforAudioService(s, d);
}

void dabProcessor::dataforPacketService(const QString &s, packetdata *pd, int16_t compnr)
{
  mMy_ficHandler.dataforPacketService(s, pd, compnr);
}

uint8_t dabProcessor::get_ecc()
{
  return mMy_ficHandler.get_ecc();
}

uint16_t dabProcessor::get_countryName()
{
  return mMy_ficHandler.get_countryName();
}

int32_t dabProcessor::get_ensembleId()
{
  return mMy_ficHandler.get_ensembleId();
}

QString dabProcessor::get_ensembleName()
{
  return mMy_ficHandler.get_ensembleName();
}

void dabProcessor::set_epgData(int SId, int32_t theTime, const QString &s, const QString &d)
{
  mMy_ficHandler.set_epgData(SId, theTime, s, d);
}

bool dabProcessor::has_timeTable(uint32_t SId)
{
  return mMy_ficHandler.has_timeTable(SId);
}

std::vector<epgElement> dabProcessor::find_epgData(uint32_t SId)
{
  return mMy_ficHandler.find_epgData(SId);
}

QStringList dabProcessor::basicPrint()
{
  return mMy_ficHandler.basicPrint();
}

int dabProcessor::scanWidth()
{
  return mMy_ficHandler.scanWidth();
}
//
//	for the mscHandler:
void dabProcessor::reset_Services()
{
  if (!mScanMode)
  {
    mMy_mscHandler.reset_Channel();
  }
}

void dabProcessor::stopService(descriptorType *d)
{
  fprintf(stderr, "function obsolete\n");
  if (!mScanMode)
  {
    mMy_mscHandler.stopService(d->subchId);
  }
}

void dabProcessor::stopService(int subChId)
{
  if (!mScanMode)
  {
    mMy_mscHandler.stopService(subChId);
  }
}

bool dabProcessor::set_audioChannel(audiodata *d, RingBuffer<int16_t> *b)
{
  if (!mScanMode)
  {
    return mMy_mscHandler.set_Channel(d, b, (RingBuffer<uint8_t> *) nullptr);
  }
  else
  {
    return false;
  }
}

bool dabProcessor::set_dataChannel(packetdata *d, RingBuffer<uint8_t> *b)
{
  if (!mScanMode)
  {
    return mMy_mscHandler.set_Channel(d, (RingBuffer<int16_t> *) nullptr, b);
  }
  else
  {
    return false;
  }
}

void dabProcessor::startDumping(SNDFILE *f)
{
  mMyReader.startDumping(f);
}

void dabProcessor::stopDumping()
{
  mMyReader.stopDumping();
}

bool dabProcessor::wasSecond(int16_t cf, dabParams *p)
{
  switch (p->get_dabMode())
  {
  default:
  case 1:
    return (cf & 07) >= 4;

  case 2:
  case 3:
    return(cf & 02);

  case 4:
    return (cf & 03) >= 2;
  }
}

void dabProcessor::start_ficDump(FILE *f)
{
  mMy_ficHandler.start_ficDump(f);
}

void dabProcessor::stop_ficDump()
{
  mMy_ficHandler.stop_ficDump();
}

