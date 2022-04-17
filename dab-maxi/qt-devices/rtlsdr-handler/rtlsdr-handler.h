/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB
 *
 *    Many of the ideas as implemented in Qt-DAB are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
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

#ifndef __RTLSDR_HANDLER__
#define __RTLSDR_HANDLER__

#include  <QObject>
#include  <QSettings>
#include  <QString>
#include  <cstdio>
#include  <atomic>
#include  "dab-constants.h"
#include  "fir-filters.h"
#include  "device-handler.h"
#include  "ringbuffer.h"
#include  "ui_rtlsdr-widget.h"

class dll_driver;
class xml_fileWriter;
//
//	create typedefs for the library functions

typedef struct rtlsdr_dev rtlsdr_dev_t;

extern "C"
{
using rtlsdr_read_async_cb_t        = void     (*)(uint8_t *buf, uint32_t len, void *ctx);
using pfnrtlsdr_open                = int      (*)(rtlsdr_dev_t **, uint32_t);
using pfnrtlsdr_close               = int      (*)(rtlsdr_dev_t *);
using pfnrtlsdr_get_usb_strings     = int      (*)(rtlsdr_dev_t *, char *, char *, char *);
using pfnrtlsdr_set_center_freq     = int      (*)(rtlsdr_dev_t *, uint32_t);
using pfnrtlsdr_set_tuner_bandwidth = int      (*)(rtlsdr_dev_t *, uint32_t);
using pfnrtlsdr_get_center_freq     = uint32_t (*)(rtlsdr_dev_t *);
using pfnrtlsdr_get_tuner_gains     = int      (*)(rtlsdr_dev_t *, int *);
using pfnrtlsdr_set_tuner_gain_mode = int      (*)(rtlsdr_dev_t *, int);
using pfnrtlsdr_set_agc_mode        = int      (*)(rtlsdr_dev_t *, int);
using pfnrtlsdr_set_sample_rate     = int      (*)(rtlsdr_dev_t *, uint32_t);
using pfnrtlsdr_get_sample_rate     = int      (*)(rtlsdr_dev_t *);
using pfnrtlsdr_set_tuner_gain      = int      (*)(rtlsdr_dev_t *, int);
using pfnrtlsdr_get_tuner_gain      = int      (*)(rtlsdr_dev_t *);
using pfnrtlsdr_reset_buffer        = int      (*)(rtlsdr_dev_t *);
using pfnrtlsdr_read_async          = int      (*)(rtlsdr_dev_t *, rtlsdr_read_async_cb_t, void *, uint32_t, uint32_t);
using pfnrtlsdr_set_bias_tee        = int      (*)(rtlsdr_dev_t *, int);
using pfnrtlsdr_cancel_async        = int      (*)(rtlsdr_dev_t *);
using pfnrtlsdr_set_direct_sampling = int      (*)(rtlsdr_dev_t *, int);
using pfnrtlsdr_get_device_count    = uint32_t (*)();
using pfnrtlsdr_set_freq_correction = int      (*)(rtlsdr_dev_t *, int);
using pfnrtlsdr_get_device_name     = char*    (*)(int);
}

//	This class is a simple wrapper around the
//	rtlsdr library that is read in  as dll (or .so file in linux)
//	It does not do any processing
class	rtlsdrHandler: public QObject, public deviceHandler, public  Ui_dabstickWidget
{
  Q_OBJECT
public:
  rtlsdrHandler(QSettings *, QString &);
  ~rtlsdrHandler() override;

  void setVFOFrequency(int32_t) override;
  int32_t getVFOFrequency() override;
  bool    restartReader(int32_t) override;
  void stopReader() override;
  int32_t getSamples(TIQSmpFlt *, int32_t) override;
  int32_t Samples() override;
  void resetBuffer() override;
  int16_t bitDepth() override;
  QString deviceName() override;
  void show() override;
  void hide() override;
  bool isHidden() override;

  int16_t maxGain();

  // These need to be visible for the separate usb handling thread
  RingBuffer<std::complex<uint8_t> > _I_Buffer;
  pfnrtlsdr_read_async               rtlsdr_read_async;
  struct rtlsdr_dev                  *device;
  bool                               isActive;

private:
  QFrame         myFrame;
  QSettings      *rtlsdrSettings;
  int32_t        inputRate;
  int32_t        deviceCount;
  HINSTANCE      Handle;
  dll_driver     *workerHandle;
  int32_t        mLastFrequency;
  int16_t        gainsCount;
  QString        deviceModel;
  QString        recorderVersion;
  FILE           *xmlDumper;
  xml_fileWriter *xmlWriter;

  bool setup_xmlDump();
  void close_xmlDump();

  std::atomic<bool> xml_dumping;
  FILE              *iqDumper;

  bool setup_iqDump();
  void close_iqDump();

  std::atomic<bool> iq_dumping;

  void record_gainSettings(int);
  void update_gainSettings(int);
  bool       save_gainSettings;

  bool       filtering;
  LowPassFIR theFilter;
  int        currentDepth;

  // here we need to load functions from the dll
  bool load_rtlFunctions();

  pfnrtlsdr_open                rtlsdr_open;
  pfnrtlsdr_close               rtlsdr_close;
  pfnrtlsdr_get_usb_strings     rtlsdr_get_usb_strings;
  pfnrtlsdr_set_center_freq     rtlsdr_set_center_freq;
  pfnrtlsdr_set_tuner_bandwidth rtlsdr_set_tuner_bandwidth;
  pfnrtlsdr_get_center_freq     rtlsdr_get_center_freq;
  pfnrtlsdr_get_tuner_gains     rtlsdr_get_tuner_gains;
  pfnrtlsdr_set_tuner_gain_mode rtlsdr_set_tuner_gain_mode;
  pfnrtlsdr_set_agc_mode        rtlsdr_set_agc_mode;
  pfnrtlsdr_set_sample_rate     rtlsdr_set_sample_rate;
  pfnrtlsdr_get_sample_rate     rtlsdr_get_sample_rate;
  pfnrtlsdr_set_tuner_gain      rtlsdr_set_tuner_gain;
  pfnrtlsdr_get_tuner_gain      rtlsdr_get_tuner_gain;
  pfnrtlsdr_reset_buffer        rtlsdr_reset_buffer;
  pfnrtlsdr_cancel_async        rtlsdr_cancel_async;
  pfnrtlsdr_set_bias_tee        rtlsdr_set_bias_tee;
  pfnrtlsdr_set_direct_sampling rtlsdr_set_direct_sampling;
  pfnrtlsdr_get_device_count    rtlsdr_get_device_count;
  pfnrtlsdr_set_freq_correction rtlsdr_set_freq_correction;
  pfnrtlsdr_get_device_name     rtlsdr_get_device_name;

signals:
  void new_gainIndex(int);
  void new_agcSetting(bool);

private slots:
  void set_ExternalGain(const QString &);
  void set_autogain(int);
  void set_ppmCorrection(int);
  void set_xmlDump();
  void set_iqDump();
  void set_filter(int);
  void set_biasControl(int);
};
#endif

