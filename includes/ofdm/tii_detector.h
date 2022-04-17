/*
 *    Copyright (C) 2014 .. 2017
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

#ifndef __TII_DETECTOR__
#define __TII_DETECTOR__

#include <cstdint>
#include "dab-params.h"
#include "fft-handler.h"
#include <vector>

class TII_Detector
{
public:
  struct STiiInfo
  {
    enum /*class*/ EResult
    {
      NONE,
      NO_PK_FND,  // No Peak Found
      PK_FND_ML,  // Maximum Likelihood
      PK_FND_MPS  // Maximum Peak Search
    };

    void set(EResult iResult, uint8_t iMainId, uint8_t iSubId) {  Result = iResult; MainId = iMainId; SubId = iSubId; }
    EResult Result = EResult::NONE;
    uint8_t MainId = 0xFF;
    uint8_t SubId = 0xFF;
  };

public:
  TII_Detector(uint8_t dabMode, int16_t);
  ~TII_Detector();

  void reset();
  void setMode(bool);
  void addBuffer(std::vector<TIQSmpFlt>);
  STiiInfo processNULL();

private:
	void collapse(TIQSmpFlt *, float *);

  bool                         mDetectMode_new;
  int16_t                      mDepth;
  uint8_t                      mpInvTable[256];
  dabParams                    mParams;
  fftHandler                   mMyFFTHandler;
  int16_t                      mT_u;
  int16_t                      mCarriers;
  TIQSmpFlt          *mpFFTBuffer;
  std::vector<TIQSmpFlt> mTheBuffer;
  std::vector<float>           mWindow;
};

#endif
