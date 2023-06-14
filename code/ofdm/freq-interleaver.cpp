/*
 *    Copyright (C) 2013 .. 2017
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
#include  "freq-interleaver.h"

/**
  *	\brief createMapper
  *	create the mapping table  for the (de-)interleaver
  *	formulas according to section 14.6 (Frequency interleaving)
  *	of the DAB standard
  */

void FreqInterleaver::createMapper(int16_t V1, int16_t lwb, int16_t upb)
{
  int16_t tmp[mDabPar.T_u];
  int16_t index = 0;
  int16_t i;

  tmp[0] = 0;
  for (i = 1; i < mDabPar.T_u; i++)
  {
    tmp[i] = (13 * tmp[i - 1] + V1) % mDabPar.T_u;
  }
  for (i = 0; i < mDabPar.T_u; i++)
  {
    if (tmp[i] == mDabPar.T_u / 2)
    {
      continue;
    }
    if ((tmp[i] < lwb) || (tmp[i] > upb))
    {
      continue;
    }
    //	we now have a table with values from lwb .. upb
    //
    mPermTable.at(index++) = tmp[i] - mDabPar.T_u / 2;
    //	we now have a table with values from lwb - T_u / 2 .. lwb + T_u / 2
  }
}

FreqInterleaver::FreqInterleaver(uint8_t iDabMode) :
  mDabPar(DabParams(iDabMode).get_dab_par())
{
  mPermTable.resize(mDabPar.T_u);

  switch (iDabMode)
  {
  case 1: createMapper(511, 256, 256 + mDabPar.K); break;
  case 2: createMapper(127,  64,  64 + mDabPar.K); break;
  case 3: createMapper(63,   32,  32 + mDabPar.K); break;
  case 4: createMapper(255, 128, 128 + mDabPar.K); break;
  default:;
  }
}


