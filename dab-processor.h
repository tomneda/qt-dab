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

#ifndef __DAB_PROCESSOR__
#define __DAB_PROCESSOR__
/*
 *	dabProcessor is the embodying of all functionality related
 *	to the actal DAB processing.
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
#include  "tii_detector.h"


class RadioInterface;
class dabParams;
class processParams;

class dabProcessor: public QThread
{
  Q_OBJECT
public:
  dabProcessor(RadioInterface *, deviceHandler *, processParams *);
  ~dabProcessor();

  void start(int32_t);
  void stop();
  void startDumping(SNDFILE *);
  void stopDumping();
  void set_scanMode(bool);
  void getFrameQuality(int *, int*, int *);

  // inheriting from our delegates
  // for the ficHandler:
  QString findService(uint32_t, int);
  void getParameters(const QString &, uint32_t *, int *);
  std::vector<serviceId>  getServices(int);
  bool is_audioService(const QString &s);
  bool is_packetService(const QString &s);
  void dataforAudioService(const QString &, audiodata *);
  void dataforPacketService(const QString &, packetdata *, int16_t);

  int  getSubChId(const QString &, uint32_t);
  uint8_t get_ecc();
  int32_t get_ensembleId();
  QString get_ensembleName();
  uint16_t get_countryName();

  void set_epgData(int32_t, int32_t, const QString &, const QString &);
  bool has_timeTable(uint32_t);
  std::vector<epgElement> find_epgData(uint32_t);

  QStringList basicPrint();
  int scanWidth();
  void start_ficDump(FILE *);
  void stop_ficDump();

  // for the mscHandler
  void reset_Services();
  void stopService(descriptorType *);
  void stopService(int);
  bool set_audioChannel(audiodata *, RingBuffer<int16_t> *);
  bool set_dataChannel(packetdata *, RingBuffer<uint8_t> *);
  void set_tiiDetectorMode(bool);

private:
  int                               mFrequency;
  int                               mThreshold;
  int                               mTotalFrames;
  int                               mGoodFrames;
  int                               mBadFrames;
  //bool                              mTiiSwitch;
  //int16_t                           mTii_depth;
  //int16_t                           mEcho_depth;
  deviceHandler                     *mpInputDevice;
  dabParams                         mParams;
  RingBuffer<TIQSmpFlt>  *mpTiiBuffer;
  RingBuffer<float>                 *mpSnrBuffer;
  int16_t                           mTii_delay;
  int16_t                           mTii_counter;

  sampleReader                      mMyReader;
  RadioInterface                    *mpMyRadioInterface;
  ficHandler                        mMy_ficHandler;
  mscHandler                        mMy_mscHandler;
  phaseReference                    mPhaseSynchronizer;
  TII_Detector                      mMy_TII_Detector;
  ofdmDecoder                       mMy_ofdmDecoder;

  int16_t                           mAttempts;
  bool                              mScanMode;
  int32_t                           mT_null;
  int32_t                           mT_u;
  int32_t                           mT_s;
  int32_t                           mT_g;
  int32_t                           mT_F;
  int32_t                           mNrBlocks;
  int32_t                           mCarriers;
  int32_t                           mCarrierDiff;
  int16_t                           mFineOffset;
  int32_t                           mCoarseOffset;
  QByteArray                        mTransmitters;
  bool                              mCorrectionNeeded;
  std::vector<TIQSmpFlt> mOfdmBuffer;

  bool wasSecond(int16_t, dabParams *);
  virtual void run();

signals:
  void setSynced(bool);
  void No_Signal_Found();
  void setSyncLost();
  void show_tii(int, int, int);
  void show_tii_spectrum();
  void show_Spectrum(int);
  void show_snr(int);
  void show_snr(float, float, float, float, float);
  void show_clockErr(int);
  void show_null(int);
};
#endif

