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
#include  "fic-handler.h"
#include  "msc-handler.h"
#include  "radio.h"
#include  "process-params.h"
//#include  "dab-params.h"
#include  "timesyncer.h"

/**
  *	\brief dabProcessor
  *	The dabProcessor class is the driver of the processing
  *	of the samplestream.
  *	It is the main interface to the qt-dab program,
  *	local are classes ofdmDecoder, ficHandler and mschandler.
  */

dabProcessor::dabProcessor(RadioInterface * const mr, deviceHandler * const inputDevice, processParams * const p)
  : mThreshold(p->threshold),
    mpInputDevice(inputDevice),
    mParams(p->dabMode),
    mpTiiBuffer(p->tiiBuffer),
    mpNullBuffer(p->nullBuffer),
    mpSnrBuffer(p->snrBuffer),
    myRadioInterface(mr),
    mReader(mr, inputDevice, p->spectrumBuffer),
    my_ficHandler(mr, p->dabMode),
    my_mscHandler(mr, p->dabMode, p->frameBuffer),
    phaseSynchronizer(mr, p),
    my_TII_Detector(p->dabMode, p->tii_depth),
    my_ofdmDecoder(mr, p->dabMode, inputDevice->bitDepth(), p->iqBuffer),
    my_etiGenerator(p->dabMode, &my_ficHandler),
    tii_delay(p->tii_delay),
    T_null(mParams.get_T_null()),
    T_u(mParams.get_T_u()),
    T_s(mParams.get_T_s()),
    T_g(T_s - T_u),
    T_F(mParams.get_T_F()),
    nrBlocks(mParams.get_L()),
    mK(mParams.get_K()),
    carrierDiff(mParams.get_carrierDiff())
{
  connect(this, SIGNAL (setSynced(bool)), myRadioInterface, SLOT (setSynced(bool)));
  connect(this, SIGNAL (setSyncLost(void)), myRadioInterface, SLOT (setSyncLost(void)));
  connect(this, SIGNAL (show_Spectrum(int)), myRadioInterface, SLOT (showSpectrum(int)));
  connect(this, SIGNAL (show_tii(int, int)), myRadioInterface, SLOT (show_tii(int, int)));
  connect(this, SIGNAL (show_tii_spectrum()), myRadioInterface, SLOT (show_tii_spectrum()));
  connect(this, SIGNAL (show_snr(int)), mr, SLOT (show_snr(int)));
  connect(this, SIGNAL (show_clockErr(int)), mr, SLOT (show_clockError(int)));
  connect(this, SIGNAL (show_null(int)), mr, SLOT (show_null(int)));

  ofdmBuffer.resize(2 * T_s);
  my_TII_Detector.reset();
}

dabProcessor::~dabProcessor()
{
  if (isRunning())
  {
    mReader.setRunning(false);
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
  my_TII_Detector.setMode(b);
}

void dabProcessor::start()
{
  my_ficHandler.restart();
  //transmitters.clear();
  if (!mScanMode)
  {
    my_mscHandler.reset_Channel();
  }
  QThread::start();
}

void dabProcessor::stop()
{
  mReader.setRunning(false);
  while (isRunning())
  {
    wait();
  }
  usleep(10000);
  my_ficHandler.stop();
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
  int32_t startIndex;
  cmplx FreqCorr;
  timeSyncer myTimeSyncer(&mReader);
  int attempts;
  std::vector<int16_t> ibits;
  int frameCount = 0;
  int sampleCount = 0;
  int totalSamples = 0;
  double cLevel = 0;
  int cCount = 0;
  bool null_shower;

  QVector<cmplx> tester(T_u / 2);
  ibits.resize(2 * mParams.get_K());
  mFineOffset = 0;
  mCoarseOffset = 0;
  correctionNeeded = true;
  attempts = 0;
  mReader.setRunning(true);  // useful after a restart
  //
  //	to get some idea of the signal strength
  try
  {
    for (int32_t i = 0; i < T_F / 5; i++)
    {
      mReader.getSample(0);
    }
    //Initing:
    notSynced:
    mTotalFrames++;
    totalSamples = 0;
    frameCount = 0;
    sampleCount = 0;

    setSynced(false);
    my_TII_Detector.reset();
    switch (myTimeSyncer.sync(T_null, T_F))
    {
    case TIMESYNC_ESTABLISHED: break;      // yes, we are ready

    case NO_DIP_FOUND:
      if (++attempts >= 8)
      {
        emit (No_Signal_Found());
        attempts = 0;
      }
      goto notSynced;

    default:      // does not happen
    case NO_END_OF_DIP_FOUND: goto notSynced;
    }
    mReader.getSamples(ofdmBuffer, 0, T_u, mCoarseOffset + mFineOffset);
    /**
      *	Looking for the first sample of the T_u part of the sync block.
      *	Note that we probably already had 30 to 40 samples of the T_g
      *	part
      */
    startIndex = phaseSynchronizer.findIndex(ofdmBuffer, mThreshold);
    if (startIndex < 0)
    { // no sync, try again
      if (!correctionNeeded)
      {
        setSyncLost();
      }
      mBadFrames++;
      goto notSynced;
    }
    sampleCount = startIndex;
    goto SyncOnPhase;
    //
    Check_endofNULL:
    mTotalFrames++;
    frameCount++;
    null_shower = false;
    totalSamples += sampleCount;
    if (frameCount > 10)
    {
      show_clockErr(totalSamples - frameCount * 196608);
      totalSamples = 0;
      frameCount = 0;
      null_shower = true;
    }

    if (null_shower)
    {
      for (int i = 0; i < T_u / 4; i++)
      {
        tester[i] = ofdmBuffer[T_null - T_u / 4 + i];
      }
    }

    mReader.getSamples(ofdmBuffer, 0, T_u, mCoarseOffset + mFineOffset);

    if (null_shower)
    {
      for (int i = 0; i < T_u / 4; i++)
      {
        tester[1 * T_u / 4 + i] = ofdmBuffer[i];
      }
      mpNullBuffer->putDataIntoBuffer(tester.data(), T_u / 2);
      show_null(T_u / 2);
    }
    /**
      *	We now have to find the exact first sample of the non-null period.
      *	We use a correlation that will find the first sample after the
      *	cyclic prefix.
      */
    startIndex = phaseSynchronizer.findIndex(ofdmBuffer, 3 * mThreshold);

    if (startIndex < 0)
    { // no sync, try again
      if (!correctionNeeded)
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
    memmove(ofdmBuffer.data(), &(ofdmBuffer[startIndex]), (T_u - startIndex) * sizeof(cmplx));
    int ofdmBufferIndex = T_u - startIndex;

    //Block_0:
    /**
      *	Block 0 is special in that it is used for fine time synchronization,
      *	for coarse frequency synchronization
      *	and its content is used as a reference for decoding the
      *	first datablock.
      *	We read the missing samples in the ofdm buffer
      */
    setSynced(true);
    mReader.getSamples(ofdmBuffer, ofdmBufferIndex, T_u - ofdmBufferIndex, mCoarseOffset + mFineOffset);

#ifdef  __WITH_JAN__
    static int abc = 0;
    if (++abc > 10) {
       phaseSynchronizer. estimate (ofdmBuffer);
       abc = 0;
    }
#endif
    sampleCount += T_u;
    my_ofdmDecoder.processBlock_0(ofdmBuffer);
    if (!mScanMode)
    {
      my_mscHandler.processBlock_0(ofdmBuffer.data());
    }

    //	Here we look only at the block_0 when we need a coarse
    //	frequency synchronization.
    correctionNeeded = !my_ficHandler.syncReached();
    if (correctionNeeded)
    {
      const int32_t correction = phaseSynchronizer.estimate_carrier_offset(ofdmBuffer);

      if (correction != 100)
      {
        mCoarseOffset += (int32_t)(0.4 * correction * carrierDiff);

        if (abs(mCoarseOffset) > kHz(35))
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
    cCount = 0;
    cLevel = 0;
    FreqCorr = cmplx(0, 0);
    for (int ofdmSymbolCount = 1; ofdmSymbolCount < nrBlocks; ofdmSymbolCount++)
    {
      mReader.getSamples(ofdmBuffer, 0, T_s, mCoarseOffset + mFineOffset);
      sampleCount += T_s;

      for (int32_t i = T_u; i < T_s; i++)
      {
        FreqCorr += ofdmBuffer[i] * conj(ofdmBuffer[i - T_u]);
        cLevel += abs(ofdmBuffer[i]) + abs(ofdmBuffer[i - T_u]);
      }
      cCount += 2 * T_g;

      if ((ofdmSymbolCount <= 3) || mEti_on)
      {
        my_ofdmDecoder.decode(ofdmBuffer, ofdmSymbolCount, ibits);
      }


      if (ofdmSymbolCount <= 3)
      {
        my_ficHandler.process_ficBlock(ibits, ofdmSymbolCount);
      }
      if (mEti_on)
      {
        my_etiGenerator.processBlock(ibits, ofdmSymbolCount);
      }

      if (!mScanMode)
      {
        my_mscHandler.process_Msc(&(ofdmBuffer[T_g]), ofdmSymbolCount);
      }
    }
    /**
      *	OK,  here we are at the end of the frame
      *	Assume everything went well and skip T_null samples
      */
    mReader.getSamples(ofdmBuffer, 0, T_null, mCoarseOffset + mFineOffset);
    sampleCount += T_null;
    float sum = 0;

    for (int32_t i = 0; i < T_null; i++)
    {
      sum += abs(ofdmBuffer[i]);
    }
    sum /= (float)T_null;

    if (this->mpSnrBuffer != nullptr)
    {
      auto snrV = (float)(20 * log10((cLevel / cCount + 0.005) / (sum + 0.005)));
      mpSnrBuffer->putDataIntoBuffer(&snrV, 1);
    }
    static float snr = 0;
    static int ccc = 0;
    ccc++;
    if (ccc >= 5)
    {
      ccc = 0;
      snr = (float)(0.9 * snr + 0.1 * 20 * log10((mReader.get_sLevel() + 0.005) / (sum + 0.005)));
      show_snr((int)snr);
    }
    /*
     *	The TII data is encoded in the null period of the
     *	odd frames
     */
    if (mParams.get_dabMode() == 1)
    {
      if (wasSecond(my_ficHandler.get_CIFcount(), &mParams))
      {
        my_TII_Detector.addBuffer(ofdmBuffer);
        if (++tii_counter >= tii_delay)
        {
          mpTiiBuffer->putDataIntoBuffer(ofdmBuffer.data(), T_u);
          show_tii_spectrum();
          uint16_t res = my_TII_Detector.processNULL();
          if (res != 0)
          {
            uint8_t mainId = res >> 8;
            uint8_t subId = res & 0xFF;
            show_tii(mainId, subId);
          }
          tii_counter = 0;
          my_TII_Detector.reset();
        }
      }
    }
    /**
      *	The first sample to be found for the next frame should be T_g
      *	samples ahead. Before going for the next frame, we
      *	we just check the fineCorrector
      */
    //NewOffset:
    //     we integrate the newly found frequency error with the
    //     existing frequency error.
    //

    mFineOffset += (int32_t)(0.05 * arg(FreqCorr) / (2 * M_PI) * carrierDiff);

    if (mFineOffset > carrierDiff / 2)
    {
      mCoarseOffset += carrierDiff;
      mFineOffset -= carrierDiff;
    }
    else if (mFineOffset < -carrierDiff / 2)
    {
      mCoarseOffset -= carrierDiff;
      mFineOffset += carrierDiff;
    }

    //ReadyForNewFrame:
    ///	and off we go, up to the next frame
    goto Check_endofNULL;
  }
  catch (int e)
  {
    fprintf(stderr, "dabProcessor is stopping\n");
  }
  //	inputDevice	-> stopReader ();
}

void dabProcessor::set_scanMode(bool b)
{
  mScanMode = b;
}

void dabProcessor::get_frame_quality(int32_t & oTotalFrames, int32_t & oGoodFrames, int32_t & oBadFrames)
{
  oTotalFrames = mTotalFrames;
  oGoodFrames = mGoodFrames;
  oBadFrames = mBadFrames;

  mTotalFrames = 0;
  mGoodFrames = 0;
  mBadFrames = 0;
}

//	just convenience functions
//	ficHandler abstracts channel data

QString dabProcessor::findService(uint32_t SId, int SCIds)
{
  return my_ficHandler.findService(SId, SCIds);
}

void dabProcessor::getParameters(const QString & s, uint32_t * p_SId, int * p_SCIds)
{
  my_ficHandler.getParameters(s, p_SId, p_SCIds);
}

std::vector<serviceId> dabProcessor::getServices(int n)
{
  return my_ficHandler.getServices(n);
}

int dabProcessor::getSubChId(const QString & s, uint32_t SId)
{
  return my_ficHandler.getSubChId(s, SId);
}

bool dabProcessor::is_audioService(const QString & s)
{
  audiodata ad;
  my_ficHandler.dataforAudioService(s, &ad);
  return ad.defined;
}

bool dabProcessor::is_packetService(const QString & s)
{
  packetdata pd;
  my_ficHandler.dataforPacketService(s, &pd, 0);
  return pd.defined;
}

void dabProcessor::dataforAudioService(const QString & s, audiodata * d)
{
  my_ficHandler.dataforAudioService(s, d);
}

void dabProcessor::dataforPacketService(const QString & s, packetdata * pd, int16_t compnr)
{
  my_ficHandler.dataforPacketService(s, pd, compnr);
}

uint8_t dabProcessor::get_ecc()
{
  return my_ficHandler.get_ecc();
}

[[maybe_unused]] uint16_t dabProcessor::get_countryName()
{
  return my_ficHandler.get_countryName();
}

int32_t dabProcessor::get_ensembleId()
{
  return my_ficHandler.get_ensembleId();
}

[[maybe_unused]] QString dabProcessor::get_ensembleName()
{
  return my_ficHandler.get_ensembleName();
}

void dabProcessor::set_epgData(int SId, int32_t theTime, const QString & s, const QString & d)
{
  my_ficHandler.set_epgData(SId, theTime, s, d);
}

bool dabProcessor::has_timeTable(uint32_t SId)
{
  return my_ficHandler.has_timeTable(SId);
}

std::vector<epgElement> dabProcessor::find_epgData(uint32_t SId)
{
  return my_ficHandler.find_epgData(SId);
}

QStringList dabProcessor::basicPrint()
{
  return my_ficHandler.basicPrint();
}

int dabProcessor::scanWidth()
{
  return my_ficHandler.scanWidth();
}

//
//	for the mscHandler:
[[maybe_unused]] void dabProcessor::reset_Services()
{
  if (!mScanMode)
  {
    my_mscHandler.reset_Channel();
  }
}

[[maybe_unused]] void dabProcessor::stop_service(descriptorType * d, int flag)
{
  fprintf(stderr, "function obsolete\n");
  if (!mScanMode)
  {
    my_mscHandler.stop_service(d->subchId, flag);
  }
}

void dabProcessor::stop_service(int subChId, int flag)
{
  if (!mScanMode)
  {
    my_mscHandler.stop_service(subChId, flag);
  }
}

bool dabProcessor::set_audioChannel(audiodata * d, RingBuffer<int16_t> * b, FILE * dump, int flag)
{
  if (!mScanMode)
  {
    return my_mscHandler.set_Channel(d, b, (RingBuffer<uint8_t> *)nullptr, dump, flag);
  }
  else
  {
    return false;
  }
}

bool dabProcessor::set_dataChannel(packetdata * d, RingBuffer<uint8_t> * b, int flag)
{
  if (!mScanMode)
  {
    return my_mscHandler.set_Channel(d, (RingBuffer<int16_t> *)nullptr, b, nullptr, flag);
  }
  else
  {
    return false;
  }
}

void dabProcessor::startDumping(SNDFILE * f)
{
  mReader.startDumping(f);
}

void dabProcessor::stopDumping()
{
  mReader.stopDumping();
}

bool dabProcessor::wasSecond(int32_t cf, const dabParams * p) const
{
  switch (p->get_dabMode())
  {
  default:
  case 1: return (cf & 07) >= 4;
  case 2:
  case 3: return (cf & 02);
  case 4: return (cf & 03) >= 2;
  }
}

void dabProcessor::start_ficDump(FILE * f)
{
  my_ficHandler.start_ficDump(f);
}

void dabProcessor::stop_ficDump()
{
  my_ficHandler.stop_ficDump();
}

uint32_t dabProcessor::julianDate()
{
  return my_ficHandler.julianDate();
}

[[maybe_unused]] bool dabProcessor::start_etiGenerator(const QString & s)
{
  if (my_etiGenerator.start_etiGenerator(s))
  {
    mEti_on = true;
  }
  return mEti_on;
}

[[maybe_unused]] void dabProcessor::stop_etiGenerator()
{
  my_etiGenerator.stop_etiGenerator();
  mEti_on = false;
}

[[maybe_unused]] void dabProcessor::reset_etiGenerator()
{
  my_etiGenerator.reset();
}

