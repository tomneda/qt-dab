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
#include  "msc-handler.h"
#include  "radio.h"
#include  "process-params.h"

/**
  *	\brief DabProcessor
  *	The DabProcessor class is the driver of the processing
  *	of the samplestream.
  *	It is the main interface to the qt-dab program,
  *	local are classes ofdmDecoder, FicHandler and mschandler.
  */

DabProcessor::DabProcessor(RadioInterface * const mr, deviceHandler * const inputDevice, processParams * const p)
  : mpInputDevice(inputDevice),
    mpTiiBuffer(p->tiiBuffer),
    mpNullBuffer(p->nullBuffer),
    mpSnrBuffer(p->snrBuffer),
    mpRadioInterface(mr),
    mSampleReader(mr, inputDevice, p->spectrumBuffer),
    mFicHandler(mr, p->dabMode),
    mMscHandler(mr, p->dabMode, p->frameBuffer),
    mPhaseReference(mr, p),
    mTiiDetector(p->dabMode, p->tii_depth),
    mOfdmDecoder(mr, p->dabMode, inputDevice->bitDepth(), p->iqBuffer),
    mEtiGenerator(p->dabMode, &mFicHandler),
    mcDabMode(p->dabMode),
    mcThreshold(p->threshold),
    mcTiiDelay(p->tii_delay),
    mDabPar(DabParams(p->dabMode).get_dab_par())
{
  connect(this, SIGNAL (setSynced(bool)), mpRadioInterface, SLOT (setSynced(bool)));
  connect(this, SIGNAL (setSyncLost(void)), mpRadioInterface, SLOT (setSyncLost(void)));
  connect(this, SIGNAL (show_Spectrum(int)), mpRadioInterface, SLOT (showSpectrum(int)));
  connect(this, SIGNAL (show_tii(int, int)), mpRadioInterface, SLOT (show_tii(int, int)));
  connect(this, SIGNAL (show_tii_spectrum()), mpRadioInterface, SLOT (show_tii_spectrum()));
  connect(this, SIGNAL (show_snr(int)), mr, SLOT (show_snr(int)));
  connect(this, SIGNAL (show_clockErr(int)), mr, SLOT (show_clockError(int)));
  connect(this, SIGNAL (show_null(int)), mr, SLOT (show_null(int)));

  mOfdmBuffer.resize(2 * mDabPar.T_s);
  mTiiDetector.reset();
}

DabProcessor::~DabProcessor()
{
  if (isRunning())
  {
    mSampleReader.setRunning(false);
    // exception to be raised
    // through the getSample(s) functions.
    msleep(100);
    while (isRunning())
    {
      usleep(100);
    }
  }
}

void DabProcessor::set_tiiDetectorMode(bool b)
{
  mTiiDetector.setMode(b);
}

void DabProcessor::start()
{
  mFicHandler.restart();
  if (!mScanMode)
  {
    mMscHandler.reset_Channel();
  }
  QThread::start();
}

void DabProcessor::stop()
{
  mSampleReader.setRunning(false);
  while (isRunning())
  {
    wait();
  }
  usleep(10000);
  mFicHandler.stop();
}

/***
   *	\brief run
   *	The main thread, reading samples,
   *	time synchronization and frequency synchronization
   *	Identifying blocks in the DAB frame
   *	and sending them to the ofdmDecoder who will transfer the results
   *	Finally, estimating the small freqency error
   */
void DabProcessor::run()
{
  int32_t startIndex;
  TimeSyncer myTimeSyncer(&mSampleReader);
  int attempts;
  int frameCount = 0;
  int sampleCount = 0;
  int totalSamples = 0;
  double cLevel = 0;
  int cCount = 0;

  mFineOffset = 0;
  mCoarseOffset = 0;
  mCorrectionNeeded = true;
  attempts = 0;
  mSampleReader.setRunning(true);  // useful after a restart

  enum class EState
  {
    NOT_SYNCED, TIME_SYNC_ESTABLISHED, CHECK_END_OF_NULL, SYNC_ON_PHASE, QUIT
  };

  EState state = EState::NOT_SYNCED;

  try
  {
    // To get some idea of the signal strength
    for (int32_t i = 0; i < mDabPar.T_F / 5; i++)
    {
      mSampleReader.getSample(0);
    }

    while (state != EState::QUIT)
    {
      switch (state)
      {
      case EState::NOT_SYNCED:
      {
        const bool ok = _run_state_not_synced(myTimeSyncer, attempts, frameCount, sampleCount, totalSamples);
        state = (ok ? EState::TIME_SYNC_ESTABLISHED : EState::NOT_SYNCED);
        break;
      }

      case EState::TIME_SYNC_ESTABLISHED:
      {
        const bool ok = _run_state_sync_established(startIndex, sampleCount);
        state = (ok ? EState::SYNC_ON_PHASE : EState::NOT_SYNCED);
        break;
      }

      case EState::CHECK_END_OF_NULL:
      {
        const bool ok = _run_state_check_end_of_null(startIndex, frameCount, sampleCount, totalSamples);
        state = (ok ? EState::SYNC_ON_PHASE : EState::NOT_SYNCED);
        break;
      }

      case EState::SYNC_ON_PHASE:
      {
        _run_state_sync_on_phase(startIndex, cLevel, cCount, sampleCount);
        state = EState::CHECK_END_OF_NULL;
        break;
      }

      case EState::QUIT:
        // not handled
        break;
      } // switch
    } // while (!QUIT)
  }
  catch (int e)
  {
    fprintf(stderr, "DabProcessor is stopping\n");
  }
}

void DabProcessor::_run_state_sync_on_phase(int32_t startIndex, double cLevel, int cCount, int & sampleCount)
{
  std::vector<int16_t> ibits(2 * mDabPar.K);
  //SyncOnPhase:
  mGoodFrames++;
  cLevel = 0;
  cCount = 0;
  /**
    *	Once here, we are synchronized, we need to copy the data we
    *	used for synchronization for block 0
    */
  const int32_t ofdmBufferIndex = mDabPar.T_u - startIndex;
  assert(ofdmBufferIndex >= 0);
  memmove(mOfdmBuffer.data(), &(mOfdmBuffer[startIndex]), ofdmBufferIndex * sizeof(cmplx));

  //Block_0:
  /**
    *	Block 0 is special in that it is used for fine time synchronization,
    *	for coarse frequency synchronization
    *	and its content is used as a reference for decoding the
    *	first datablock.
    *	We read the missing samples in the ofdm buffer
    */
  setSynced(true);
  mSampleReader.getSamples(mOfdmBuffer, ofdmBufferIndex, mDabPar.T_u - ofdmBufferIndex, mCoarseOffset + mFineOffset);

  sampleCount += mDabPar.T_u;
  mOfdmDecoder.processBlock_0(mOfdmBuffer);

  if (!mScanMode)
  {
    mMscHandler.processBlock_0(mOfdmBuffer.data());
  }

  // Here we look only at the block_0 when we need a coarse frequency synchronization.
  mCorrectionNeeded = !mFicHandler.syncReached();

  if (mCorrectionNeeded)
  {
    const int32_t correction = mPhaseReference.estimate_carrier_offset(mOfdmBuffer);

    if (correction != PhaseReference::IDX_NOT_FOUND)
    {
      mCoarseOffset += (int32_t)(0.4 * correction * mDabPar.CarrDiff);

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
  cmplx freqCorr = cmplx(0, 0);

  for (int ofdmSymbolCount = 1; ofdmSymbolCount < mDabPar.L; ofdmSymbolCount++)
  {
    mSampleReader.getSamples(mOfdmBuffer, 0, mDabPar.T_s, mCoarseOffset + mFineOffset);
    sampleCount += mDabPar.T_s;

    for (int32_t i = mDabPar.T_u; i < mDabPar.T_s; i++)
    {
      freqCorr += mOfdmBuffer[i] * conj(mOfdmBuffer[i - mDabPar.T_u]);
      cLevel += abs(mOfdmBuffer[i]) + abs(mOfdmBuffer[i - mDabPar.T_u]);
    }
    cCount += 2 * mDabPar.T_g;


    if ((ofdmSymbolCount <= 3) || mEti_on)
    {
      mOfdmDecoder.decode(mOfdmBuffer, ofdmSymbolCount, ibits);
    }

    if (ofdmSymbolCount <= 3)
    {
      mFicHandler.process_ficBlock(ibits, ofdmSymbolCount);
    }

    if (mEti_on)
    {
      mEtiGenerator.processBlock(ibits, ofdmSymbolCount);
    }

    if (!mScanMode)
    {
      mMscHandler.process_Msc(&(mOfdmBuffer[mDabPar.T_g]), ofdmSymbolCount);
    }
  }

  /**
    *	OK,  here we are at the end of the frame
    *	Assume everything went well and skip T_null samples
    */
  mSampleReader.getSamples(mOfdmBuffer, 0, mDabPar.T_n, mCoarseOffset + mFineOffset);
  sampleCount += mDabPar.T_n;
  float sum = 0;

  for (int32_t i = 0; i < mDabPar.T_n; i++)
  {
    sum += abs(mOfdmBuffer[i]);
  }
  sum /= (float)mDabPar.T_n;

  if (mpSnrBuffer != nullptr)
  {
    auto snrV = (float)(20 * log10((cLevel / cCount + 0.005) / (sum + 0.005)));
    mpSnrBuffer->putDataIntoBuffer(&snrV, 1);
  }

  if (++mSnrCounter >= 5)
  {
    constexpr float ALPHA_SNR = 0.1f;
    mSnrCounter = 0;
    mSnrdB = (1.0f - ALPHA_SNR) * mSnrdB + ALPHA_SNR * 20.0f * log10f((mSampleReader.get_sLevel() + 0.005f) / (sum + 0.005f));
    show_snr((int)mSnrdB);
  }
  /*
   *	The TII data is encoded in the null period of the
   *	odd frames
   */

  auto wasSecond = [](int32_t iCF, uint8_t iDabMode) -> bool
  {
    switch (iDabMode)
    {
    default:
    case 1: return (iCF & 07) >= 4;
    case 2:
    case 3: return (iCF & 02);
    case 4: return (iCF & 03) >= 2;
    }
  };

  if (mcDabMode == 1)
  {
    if (wasSecond(mFicHandler.get_CIFcount(), mcDabMode))
    {
      mTiiDetector.addBuffer(mOfdmBuffer);
      if (++mTiiCounter >= mcTiiDelay)
      {
        mpTiiBuffer->putDataIntoBuffer(mOfdmBuffer.data(), mDabPar.T_u);
        show_tii_spectrum();
        uint16_t res = mTiiDetector.processNULL();
        if (res != 0)
        {
          uint8_t mainId = res >> 8;
          uint8_t subId = res & 0xFF;
          show_tii(mainId, subId);
        }
        mTiiCounter = 0;
        mTiiDetector.reset();
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

  mFineOffset += (int32_t)(0.05 * arg(freqCorr) / (2 * M_PI) * mDabPar.CarrDiff);

  if (mFineOffset > mDabPar.CarrDiff / 2)
  {
    mCoarseOffset += mDabPar.CarrDiff;
    mFineOffset -= mDabPar.CarrDiff;
  }
  else if (mFineOffset < -mDabPar.CarrDiff / 2)
  {
    mCoarseOffset -= mDabPar.CarrDiff;
    mFineOffset += mDabPar.CarrDiff;
  }

  //ReadyForNewFrame:
  ///	and off we go, up to the next frame
  //return EState::CHECK_END_OF_NULL;
}

bool DabProcessor::_run_state_check_end_of_null(int32_t & startIndex, int & frameCount, int & sampleCount, int & totalSamples)
{
  mTotalFrames++;
  frameCount++;
  bool null_shower = false;
  totalSamples += sampleCount;
  if (frameCount > 10)
  {
    show_clockErr(totalSamples - frameCount * mDabPar.T_F);
    totalSamples = 0;
    frameCount = 0;
    null_shower = true;
  }

  QVector<cmplx> tester(mDabPar.T_u / 2);

  if (null_shower)
  {
    for (int i = 0; i < mDabPar.T_u / 4; i++)
    {
      tester[i] = mOfdmBuffer[mDabPar.T_n - mDabPar.T_u / 4 + i];
    }
  }

  mSampleReader.getSamples(mOfdmBuffer, 0, mDabPar.T_u, mCoarseOffset + mFineOffset);

  if (null_shower)
  {
    for (int i = 0; i < mDabPar.T_u / 4; i++)
    {
      tester[1 * mDabPar.T_u / 4 + i] = mOfdmBuffer[i];
    }
    mpNullBuffer->putDataIntoBuffer(tester.data(), mDabPar.T_u / 2);
    show_null(mDabPar.T_u / 2);
  }

  /**
    *	We now have to find the exact first sample of the non-null period.
    *	We use a correlation that will find the first sample after the
    *	cyclic prefix.
    */
  startIndex = mPhaseReference.find_index(mOfdmBuffer, 3 * mcThreshold);

  if (startIndex < 0)
  { // no sync, try again
    if (!mCorrectionNeeded)
    {
      setSyncLost();
    }
    mBadFrames++;
    return false;
    //return EState::NOT_SYNCED;
  }
  else
  {
    sampleCount = startIndex;
    return true;
    //return EState::SYNC_ON_PHASE;
  }
}

bool DabProcessor::_run_state_sync_established(int32_t & startIndex, int & sampleCount)
{
  // get first OFDM symbol after time sync marker
  mSampleReader.getSamples(mOfdmBuffer, 0, mDabPar.T_u, mCoarseOffset + mFineOffset);

  /**
    *	Looking for the first sample of the T_u part of the sync block.
    *	Note that we probably already had 30 to 40 samples of the T_g
    *	part
    */

  startIndex = mPhaseReference.find_index(mOfdmBuffer, mcThreshold);

  if (startIndex < 0)
  { // no sync, try again
    if (!mCorrectionNeeded)
    {
      setSyncLost();
    }
    mBadFrames++;
    return false;
    //return EState::NOT_SYNCED;
  }
  else
  {
    sampleCount = startIndex;
    return true;
    //return EState::SYNC_ON_PHASE;
  }
}

bool DabProcessor::_run_state_not_synced(TimeSyncer & myTimeSyncer, int attempts, int & frameCount, int & sampleCount, int & totalSamples)
{
  //notSynced:
  mTotalFrames++;
  totalSamples = 0;
  frameCount = 0;
  sampleCount = 0;

  setSynced(false);
  mTiiDetector.reset();

  switch (myTimeSyncer.read_samples_until_end_of_level_drop(mDabPar.T_n, mDabPar.T_F))
  {
  case TimeSyncer::EState::TIMESYNC_ESTABLISHED:
    return true;
    //return EState::TIME_SYNC_ESTABLISHED;
  case TimeSyncer::EState::NO_DIP_FOUND:
    if (++attempts >= 8)
    {
      emit No_Signal_Found();
      attempts = 0;
    }
    break;
  case TimeSyncer::EState::NO_END_OF_DIP_FOUND:
    break;
  }
  return false;
  //return EState::NOT_SYNCED;
}

void DabProcessor::set_scanMode(bool b)
{
  mScanMode = b;
}

void DabProcessor::get_frame_quality(int32_t & oTotalFrames, int32_t & oGoodFrames, int32_t & oBadFrames)
{
  oTotalFrames = mTotalFrames;
  oGoodFrames = mGoodFrames;
  oBadFrames = mBadFrames;

  mTotalFrames = 0;
  mGoodFrames = 0;
  mBadFrames = 0;
}

//	just convenience functions
//	FicHandler abstracts channel data

QString DabProcessor::findService(uint32_t SId, int SCIds)
{
  return mFicHandler.findService(SId, SCIds);
}

void DabProcessor::getParameters(const QString & s, uint32_t * p_SId, int * p_SCIds)
{
  mFicHandler.getParameters(s, p_SId, p_SCIds);
}

std::vector<serviceId> DabProcessor::getServices(int n)
{
  return mFicHandler.getServices(n);
}

int DabProcessor::getSubChId(const QString & s, uint32_t SId)
{
  return mFicHandler.getSubChId(s, SId);
}

bool DabProcessor::is_audioService(const QString & s)
{
  audiodata ad;
  mFicHandler.dataforAudioService(s, &ad);
  return ad.defined;
}

bool DabProcessor::is_packetService(const QString & s)
{
  packetdata pd;
  mFicHandler.dataforPacketService(s, &pd, 0);
  return pd.defined;
}

void DabProcessor::dataforAudioService(const QString & s, audiodata * d)
{
  mFicHandler.dataforAudioService(s, d);
}

void DabProcessor::dataforPacketService(const QString & s, packetdata * pd, int16_t compnr)
{
  mFicHandler.dataforPacketService(s, pd, compnr);
}

uint8_t DabProcessor::get_ecc()
{
  return mFicHandler.get_ecc();
}

[[maybe_unused]] uint16_t DabProcessor::get_countryName()
{
  return mFicHandler.get_countryName();
}

int32_t DabProcessor::get_ensembleId()
{
  return mFicHandler.get_ensembleId();
}

[[maybe_unused]] QString DabProcessor::get_ensembleName()
{
  return mFicHandler.get_ensembleName();
}

void DabProcessor::set_epgData(int SId, int32_t theTime, const QString & s, const QString & d)
{
  mFicHandler.set_epgData(SId, theTime, s, d);
}

bool DabProcessor::has_timeTable(uint32_t SId)
{
  return mFicHandler.has_timeTable(SId);
}

std::vector<epgElement> DabProcessor::find_epgData(uint32_t SId)
{
  return mFicHandler.find_epgData(SId);
}

QStringList DabProcessor::basicPrint()
{
  return mFicHandler.basicPrint();
}

int DabProcessor::scanWidth()
{
  return mFicHandler.scanWidth();
}

//
//	for the mscHandler:
[[maybe_unused]] void DabProcessor::reset_Services()
{
  if (!mScanMode)
  {
    mMscHandler.reset_Channel();
  }
}

[[maybe_unused]] void DabProcessor::stop_service(descriptorType * d, int flag)
{
  fprintf(stderr, "function obsolete\n");
  if (!mScanMode)
  {
    mMscHandler.stop_service(d->subchId, flag);
  }
}

void DabProcessor::stop_service(int subChId, int flag)
{
  if (!mScanMode)
  {
    mMscHandler.stop_service(subChId, flag);
  }
}

bool DabProcessor::set_audioChannel(audiodata * d, RingBuffer<int16_t> * b, FILE * dump, int flag)
{
  if (!mScanMode)
  {
    return mMscHandler.set_Channel(d, b, (RingBuffer<uint8_t> *)nullptr, dump, flag);
  }
  else
  {
    return false;
  }
}

bool DabProcessor::set_dataChannel(packetdata * d, RingBuffer<uint8_t> * b, int flag)
{
  if (!mScanMode)
  {
    return mMscHandler.set_Channel(d, (RingBuffer<int16_t> *)nullptr, b, nullptr, flag);
  }
  else
  {
    return false;
  }
}

void DabProcessor::startDumping(SNDFILE * f)
{
  mSampleReader.startDumping(f);
}

void DabProcessor::stopDumping()
{
  mSampleReader.stopDumping();
}

void DabProcessor::start_ficDump(FILE * f)
{
  mFicHandler.start_ficDump(f);
}

void DabProcessor::stop_ficDump()
{
  mFicHandler.stop_ficDump();
}

uint32_t DabProcessor::julianDate()
{
  return mFicHandler.julianDate();
}

[[maybe_unused]] bool DabProcessor::start_etiGenerator(const QString & s)
{
  if (mEtiGenerator.start_etiGenerator(s))
  {
    mEti_on = true;
  }
  return mEti_on;
}

[[maybe_unused]] void DabProcessor::stop_etiGenerator()
{
  mEtiGenerator.stop_etiGenerator();
  mEti_on = false;
}

[[maybe_unused]] void DabProcessor::reset_etiGenerator()
{
  mEtiGenerator.reset();
}

