#
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
#


#include  "dab-constants.h"
#include  "radio.h"
#include  "msc-handler.h"
#include  "backend.h"
#include  "dab-params.h"
//
//	Interface program for processing the MSC.
//	The DabProcessor assumes the existence of an msc-handler, whether
//	a service is selected or not. 

#define  CUSize  (4 * 16)
static int cifTable[] = { 18, 72, 0, 36 };

//	Note CIF counts from 0 .. 3
//
mscHandler::mscHandler(RadioInterface * mr, uint8_t dabMode, RingBuffer<uint8_t> * frameBuffer) :
  params(dabMode),
  myMapper(dabMode),
  fft(params.get_T_u(), false)
{
  myRadioInterface = mr;
  this->frameBuffer = frameBuffer;
  cifVector.resize(55296);
  BitsperBlock = 2 * params.get_K();
  ibits.resize(BitsperBlock);
  nrBlocks = params.get_L();

  phaseReference.resize(params.get_T_u());

  numberofblocksperCIF = cifTable[(dabMode - 1) & 03];
}

mscHandler::~mscHandler()
{
  locker.lock();
  for (auto const & b: theBackends)
  {
    b->stopRunning();
    delete b;
  }
  locker.unlock();
  theBackends.resize(0);
}

//
//	Input is put into a buffer, a the code in a separate thread
//	will handle the data from the buffer
void mscHandler::processBlock_0(cmplx * b)
{
  (void)b;
}

void mscHandler::process_Msc(cmplx * b, int blkno)
{
  cmplx fft_buffer[params.get_T_u()];;
  if (blkno < 3)
  {
    return;
  }

  memcpy(fft_buffer, b, params.get_T_u() * sizeof(cmplx));
  //
  //	block 3 and up are needed as basis for demodulation the "mext" block
  //	"our" msc blocks start with blkno 4
  fft.fft(fft_buffer);
  if (blkno >= 4)
  {
    for (int i = 0; i < params.get_K(); i++)
    {
      int16_t index = myMapper.map_k_to_fft_bin(i);
      if (index < 0)
      {
        index += params.get_T_u();
      }
      cmplx r1 = fft_buffer[index] * conj(phaseReference[index]);
      float ab1 = jan_abs(r1);
      //      Recall:  the viterbi decoder wants 127 max pos, - 127 max neg
      //      we make the bits into softbits in the range -127 .. 127
      ibits[i] = -real(r1) / ab1 * 256.0;
      ibits[params.get_K() + i] = -imag(r1) / ab1 * 256.0;
    }

    process_mscBlock(ibits, blkno);
  }
  memcpy(phaseReference.data(), fft_buffer, params.get_T_u() * sizeof(cmplx));
}

//
//	Note, the set_Channel function is called from within a
//	different thread than the process_mscBlock method is,
//	so, a little bit of locking seems wise while
//	the actual changing of the settings is done in the
//	thread executing process_mscBlock
void mscHandler::reset_Buffers()
{
  reset_Channel();
}

void mscHandler::reset_Channel()
{
  fprintf(stderr, "channel reset: all services will be stopped\n");
  locker.lock();
  for (auto const & b: theBackends)
  {
    b->stopRunning();
    delete b;
  }
  theBackends.resize(0);
  locker.unlock();
}

void mscHandler::stop_service(descriptorType * d, int flag)
{
  fprintf(stderr, "obsolete function stopService\n");
  locker.lock();
  for (size_t i = 0; i < theBackends.size(); i++)
  {
    Backend * b = theBackends.at(i);
    if ((b->subChId == d->subchId) && (b->borf == flag))
    {
      fprintf(stderr, "stopping (sub)service at subchannel %d\n", d->subchId);
      b->stopRunning();
      delete b;
      theBackends.erase(theBackends.begin() + i);
    }
  }
  locker.unlock();
}

void mscHandler::stop_service(int subchId, int flag)
{
  locker.lock();
  for (size_t i = 0; i < theBackends.size(); i++)
  {
    Backend * b = theBackends.at(i);
    if ((b->subChId == subchId) && (b->borf == flag))
    {
      fprintf(stderr, "stopping subchannel %d\n", subchId);
      b->stopRunning();
      delete b;
      theBackends.erase(theBackends.begin() + i);
    }
  }
  locker.unlock();
}

bool mscHandler::set_Channel(descriptorType * d, RingBuffer<int16_t> * audioBuffer, RingBuffer<uint8_t> * dataBuffer, FILE * dump, int flag)
{
  fprintf(stderr, "going to open %s\n", d->serviceName.toLatin1().data());
  //	locker. lock();
  //	for (int i = 0; i < theBackends. size (); i ++) {
  //	   if (d -> subchId == theBackends. at (i) -> subChId) {
  //	      fprintf (stderr, "The service is already running\n");
  //	      theBackends. at (i) -> stopRunning ();
  //	      delete theBackends. at (i);
  //	      theBackends. erase (theBackends. begin () + i);
  //	   }
  //	}
  //	locker. unlock ();
  theBackends.push_back(new Backend(myRadioInterface, d, audioBuffer, dataBuffer, frameBuffer, dump, flag));
  fprintf(stderr, "we have now %d backends running\n", (int)theBackends.size());
  return true;
}

//
//	add blocks. First is (should be) block 4, last is (should be) 
//	nrBlocks -1.
//	Note that this method is called from within the ofdm-processor thread
//	while the set_xxx methods are called from within the 
//	gui thread, so some locking is added
//

void mscHandler::process_mscBlock(std::vector<int16_t> & fbits, int16_t blkno)
{
  int16_t currentblk;

  currentblk = (blkno - 4) % numberofblocksperCIF;
  //	and the normal operation is:
  memcpy(&cifVector[currentblk * BitsperBlock], fbits.data(), BitsperBlock * sizeof(int16_t));
  if (currentblk < numberofblocksperCIF - 1)
  {
    return;
  }

  //	OK, now we have a full CIF and it seems there is some work to
  //	be done.  We assume that the backend itself
  //	does the work in a separate thread.
  locker.lock();
  for (auto const & b: theBackends)
  {
    int16_t startAddr = b->startAddr;
    int16_t Length = b->Length;
    if (Length > 0)
    {    // Length = 0? should not happen
      (void)b->process(&cifVector[startAddr * CUSize], Length * CUSize);
    }
  }
  locker.unlock();
}
