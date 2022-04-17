/*
 *    Copyright (C) 2014 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB.
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

//
//	Simple spectrum scope object
//	Shows the spectrum of the incoming data stream
//	If made invisible, it is a "do nothing"
//
#ifndef   __SPECTRUM_VIEWER__
#define   __SPECTRUM_VIEWER__

#include "dab-constants.h"
#include <QFrame>
#include <QObject>
#include <fftw3.h>
#include "ui_scopewidget.h"
#include "ringbuffer.h"
#include <qwt.h>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_color_map.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_textlabel.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_layout.h>
#include <qwt_picker_machine.h>
#include <qwt_scale_widget.h>
#include <QBrush>
#include <QTimer>


class RadioInterface;
class QSettings;
class IQDisplay;

class spectrumViewer : public QObject, Ui_scopeWidget
{
Q_OBJECT
public:
  spectrumViewer  (RadioInterface * const,
                   QSettings * const,
                   RingBuffer<TIQSmpFlt> * const,
                   RingBuffer<TIQSmpFlt> * const);
  ~spectrumViewer();

  void showSpectrum(int32_t, int32_t);
  void showIQ(int32_t);
  void showQuality(float, float, float, float);
  void show_clockErr(int);
  void setBitDepth(int16_t);
  void show();
  void hide();
  bool isHidden();

private:
  RadioInterface *mpMyRadioInterface;
  QSettings *mpDabSettings;
  RingBuffer<TIQSmpFlt> *mpSpectrumBuffer;
  RingBuffer<TIQSmpFlt> *mpIQBuffer;
  QwtPlotPicker *mpLm_picker;
  QColor mDisplayColor;
  QColor mGridColor;
  QColor mCurveColor;

  int16_t mDisplaySize;
  int16_t mSpectrumSize;
  TIQSmpFlt *mpSpectrum;
  std::vector<double> mDisplayBuffer;
  std::vector<float> mWindow;
  fftwf_plan mPlan;
  QFrame *mpMyFrame;
  QwtPlotMarker *mpMarker;
  QwtPlot *mpPlotgrid;
  QwtPlotGrid *mpGrid;
  QwtPlotCurve *mpSpectrumCurve;
  QBrush *mpOurBrush;
  int32_t mIndexforMarker;
  IQDisplay *mpMyIQDisplay;
  float mNormalizer;

private:
  void ViewSpectrum(double *, double *, double, int);
  float get_db(float);

private slots:
  void rightMouseClick(const QPointF &);
};

#endif

