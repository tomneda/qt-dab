/*
 *    Copyright (C) 2014 .. 2017
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
//	Simple viewer for correlation
//
#ifndef    CORRELATION_VIEWER_H
#define    CORRELATION_VIEWER_H

#include  "dab-constants.h"
#include  <QFrame>
#include  <QSettings>
#include  "ringbuffer.h"
#include  <QObject>
//#include  "ui_correlation-widget.h"
#include  "ui_scopewidget.h"
#include  <qwt.h>
#include  <qwt_plot.h>
#include  <qwt_plot_marker.h>
#include  <qwt_plot_grid.h>
#include  <qwt_plot_curve.h>
#include  <qwt_color_map.h>
#include  <qwt_plot_zoomer.h>
#include  <qwt_plot_textlabel.h>
#include  <qwt_plot_panner.h>
#include  <qwt_plot_layout.h>
#include  <qwt_picker_machine.h>
#include  <qwt_scale_widget.h>
#include  <QBrush>
#include  <QVector>


class RadioInterface;

class correlationViewer : public QObject/*, Ui_scopeWidget*/
{
Q_OBJECT
public:
  correlationViewer(QwtPlot *, QLabel *, QSettings *, RingBuffer<float> *);
  ~correlationViewer();
  void showCorrelation(int32_t dots, int marker, const QVector<int> & v);
  void showIndex(int32_t);
//  void show();
//  void hide();
//  bool isHidden();

private:
  RadioInterface * myRadioInterface;
  QSettings * dabSettings;
  //QFrame myFrame;
  QwtPlotCurve spectrumCurve;
  QwtPlotGrid grid;
  std::vector<int> indexVector;
  float get_db(float);
  RingBuffer<float> * responseBuffer;
  int16_t displaySize;
  QwtPlot * plotgrid;
  QLabel * mpIndexDisplay;
  QwtPlotPicker * lm_picker;
  QColor displayColor;
  QColor gridColor;
  QColor curveColor;
  int plotLength;
private slots:
  void rightMouseClick(const QPointF &);
  //void handle_correlationLength(int);
};

#endif

