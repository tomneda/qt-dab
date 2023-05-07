/*
 *    Copyright (C) 2008, 2009, 2010
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
#include "fft-handler.h"
#include <cstring>
//
//	The basic idea was to have a single instance of the
//	fftHandler, for all DFT's. Makes sense, since they are all
//	of size T_u.
//	However, in the concurrent version this does not work,
//	it seems some locking there is inevitable
//
fftHandler::fftHandler(uint8_t mode) : p(mode)
{
  this->fftSize = p.get_T_u();
  vector = (std::complex<float> *)FFTW_MALLOC(sizeof(std::complex<float>) * fftSize);
  
  planfft = FFTW_PLAN_DFT_1D(fftSize,
                             reinterpret_cast<fftwf_complex *>(vector),
                             reinterpret_cast<fftwf_complex *>(vector),
                             FFTW_FORWARD, FFTW_ESTIMATE);
  
  planifft = FFTW_PLAN_DFT_1D(fftSize,
                              reinterpret_cast<fftwf_complex *>(vector),
                              reinterpret_cast<fftwf_complex *>(vector),
                              FFTW_BACKWARD, FFTW_ESTIMATE);
}

fftHandler::~fftHandler()
{
  FFTW_DESTROY_PLAN(planifft);
  FFTW_DESTROY_PLAN(planfft);
  FFTW_FREE(vector);
}

std::complex<float> *fftHandler::getVector()
{
  return vector;
}

void fftHandler::do_FFT()
{
  FFTW_EXECUTE(planfft);
}

void fftHandler::do_IFFT()
{
  FFTW_EXECUTE(planifft);
}
