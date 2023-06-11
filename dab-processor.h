/*
 *    Copyright (C) 2015 .. 2020
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
#ifndef  DAB_PROCESSOR_H
#define  DAB_PROCESSOR_H
/*
 *	DabProcessor is the embodying of all functionality related
 *	to the actual DAB processing.
 */
#include  "dab-constants.h"
#include  <QThread>
#include  <QObject>
#include  <QByteArray>
#include  <QStringList>
#include  <vector>
#include  <cstdint>
#include  <sndfile.h>
#include  "sample-reader.h"
#include  "phasereference.h"
#include  "ofdm-decoder.h"
#include  "fic-handler.h"
#include  "msc-handler.h"
#include  "device-handler.h"
#include  "ringbuffer.h"
#include  "tii-detector.h"
#include  "eti-generator.h"
#include "timesyncer.h"

class RadioInterface;
class DabParams;
class processParams;

class DabProcessor : public QThread
{
Q_OBJECT
public:
  DabProcessor(RadioInterface * mr, deviceHandler * inputDevice, processParams * p);
  ~DabProcessor() override;

  void start();
  void stop();
  void startDumping(SNDFILE *);
  void stopDumping();
  bool start_etiGenerator(const QString &);
  void stop_etiGenerator();
  void reset_etiGenerator();
  void set_scanMode(bool);
  void get_frame_quality(int32_t & oTotalFrames, int32_t & oGoodFrames, int32_t & oBadFrames);

  //	inheriting from our delegates
  //	for the FicHandler:
  QString findService(uint32_t, int);
  void getParameters(const QString &, uint32_t *, int *);
  std::vector<serviceId> getServices(int);
  bool is_audioService(const QString & s);
  bool is_packetService(const QString & s);
  void dataforAudioService(const QString &, audiodata *);
  void dataforPacketService(const QString &, packetdata *, int16_t);
  int getSubChId(const QString &, uint32_t);
  uint8_t get_ecc();
  int32_t get_ensembleId();
  QString get_ensembleName();
  uint16_t get_countryName();
  void set_epgData(int32_t, int32_t, const QString &, const QString &);
  bool has_timeTable(uint32_t);
  std::vector<epgElement> find_epgData(uint32_t);
  uint32_t julianDate();
  QStringList basicPrint();
  int scanWidth();
  void start_ficDump(FILE *);
  void stop_ficDump();

  //	for the mscHandler
  void reset_Services();
  void stop_service(descriptorType *, int);
  void stop_service(int, int);
  bool set_audioChannel(audiodata *, RingBuffer<int16_t> *, FILE *, int);
  bool set_dataChannel(packetdata *, RingBuffer<uint8_t> *, int);
  void set_tiiDetectorMode(bool);

private:
  deviceHandler * const mpInputDevice;
  RingBuffer<cmplx> * const mpTiiBuffer;
  RingBuffer<cmplx> * const mpNullBuffer;
  RingBuffer<float> * const mpSnrBuffer;
  RadioInterface * const mpRadioInterface;
  SampleReader mSampleReader;
  FicHandler mFicHandler;
  mscHandler mMscHandler;
  PhaseReference mPhaseReference;
  TII_Detector mTiiDetector;
  ofdmDecoder mOfdmDecoder;
  etiGenerator mEtiGenerator;
  const uint8_t mcDabMode;
  const float mcThreshold;
  const int16_t mcTiiDelay;
  const DabParams::SDabPar mDabPar;
  bool mScanMode{ false };
  int32_t mTotalFrames = 0;
  int32_t mGoodFrames = 0;
  int32_t mBadFrames = 0;
  int16_t mTiiCounter = 0;
  bool mEti_on = false;
  int32_t mFineOffset = 0;
  int32_t mCoarseOffset = 0;
  int mSnrCounter = 0;
  float mSnrdB = 0;

  bool mCorrectionNeeded{ true };
  std::vector<cmplx> mOfdmBuffer;
  enum class EState
  {
    NOT_SYNCED,
    TIME_SYNC_ESTABLISHED,
    CHECK_END_OF_NULL,
    SYNC_ON_PHASE,
    QUIT
  };

  void run() override; // the new QThread

  EState _run_state_not_synced(TimeSyncer & myTimeSyncer, int attempts, int & frameCount, int & sampleCount, int & totalSamples);
  EState _run_state_sync_established(int32_t & startIndex, int & sampleCount);
  EState _run_state_check_end_of_null(bool null_shower, QVector<cmplx> & tester, int32_t & startIndex, int & frameCount, int & sampleCount, int & totalSamples);
  EState _run_state_sync_on_phase(int32_t startIndex, std::vector<int16_t> & ibits, double cLevel, int cCount, int & sampleCount);

signals:
  void setSynced(bool);
  void No_Signal_Found();
  void setSyncLost();
  void show_tii(int, int);
  void show_tii_spectrum();
  void show_Spectrum(int);
  void show_snr(int);
  void show_snr(float, float, float, float, float);
  void show_clockErr(int);
  void show_null(int);
};

#endif
