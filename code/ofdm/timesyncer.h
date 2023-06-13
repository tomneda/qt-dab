/*
 *    Copyright (C) 2014.. 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB 
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
#ifndef  TIMESYNCER_H
#define  TIMESYNCER_H

#include  "dab-constants.h"

class SampleReader;

class TimeSyncer
{
public:
  enum class EState
  {
    TIMESYNC_ESTABLISHED, NO_DIP_FOUND, NO_END_OF_DIP_FOUND
  };

  TimeSyncer(SampleReader * mr);
  ~TimeSyncer() = default;

  EState read_samples_until_end_of_level_drop(int, int);

private:
  SampleReader * const mpSampleReader;
  const int32_t mcSyncBufferSize = 4096;
  int32_t mSyncBufferIndex = 0;
};

#endif

