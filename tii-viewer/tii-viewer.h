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

#ifndef   __TII_VIEWER__
#define   __TII_VIEWER__

#include  <QSettings>
#include  "dab-constants.h"
#include  <QFrame>
#include  <QObject>
#include  <QByteArray>
#include  <fftw3.h>
#include  "ui_tii-widget.h"
#include  "ringbuffer.h"
#include  <qwt.h>
#include  <qwt_plot.h>
#include  <qwt_plot_marker.h>
#include  <qwt_plot_grid.h>
#include  <qwt_plot_curve.h>
#include  <qwt_plot_marker.h>
#include  <qwt_color_map.h>
#include  <qwt_plot_zoomer.h>
#include  <qwt_plot_textlabel.h>
#include  <qwt_plot_panner.h>
#include  <qwt_plot_layout.h>
#include  <qwt_picker_machine.h>
#include   <qwt_scale_widget.h>
#include   <QBrush>

class	RadioInterface;

class	tiiViewer: public QObject, Ui_tiiWidget
{
  Q_OBJECT
public:
  tiiViewer(RadioInterface *, QSettings *, RingBuffer<TIQSmpFlt> *);
  ~tiiViewer() override;

  void showSpectrum(int32_t);
  void showTransmitters(const QByteArray &);
  void setBitDepth(int16_t);
  void show();
  void hide();
  bool isHidden();
  void clear();
  void show_nullPeriod(const QVector<float> &, double);

private:
  RadioInterface                   *mpMyRadioInterface;
  QSettings                        *mpDabSettings;
  QFrame                           mMyFrame;
  QwtPlotCurve                     mSpectrumCurve;
  QwtPlotGrid                      mGrid;
  RingBuffer<TIQSmpFlt> *mpTiiBuffer;
  int16_t                          mDisplaySize;
  int16_t                          mSpectrumSize;
  TIQSmpFlt              *mpSpectrum;
  std::vector<double>              mDisplayBuffer;
  std::vector<float>               mWindow;

  QwtPlotPicker                    *mpLm_picker;
  QColor                           mDisplayColor;
  QColor                           mGridColor;
  QColor                           mCurveColor;

  fftwf_plan                       mPlan;
  QwtPlotMarker                    *mpMarker;
  QwtPlot                          *mpPlotgrid;
  //QBrush                           *mpOurBrush;
  int32_t                          mIndexforMarker;
  float                            mNormalizer;

  void ViewSpectrum(double *, double *, double, int);
  float get_db(float);

private slots:
  void rightMouseClick(const QPointF &);
};
#endif

