######################################################################
# Automatically generated by qmake (2.01a) Tue Oct 6 19:48:14 2009
# but modified by me to accomodate for the includes for qwt, hamlib and
# portaudio
######################################################################

TEMPLATE	= app
TARGET		= dabMini-2.0
QT		+= widgets xml
#CONFIG		+= console
CONFIG		-= console
QMAKE_CXXFLAGS	+= -std=c++11
QMAKE_CFLAGS	+=  -flto -ffast-math
MAKE_CXXFLAGS	+=  -flto -ffast-math
#QMAKE_CFLAGS	+=  -g
#QMAKE_CXXFLAGS	+=  -g
#QMAKE_LFLAGS	+=  -g
QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]
RC_ICONS	=  dab-mini.ico
RESOURCES	+= resources.qrc

TRANSLATIONS = i18n/de_DE.ts

DEPENDPATH += . \
	      .. \
	      ../src \
	      ../includes \
	      ../src/ofdm \
	      ../src/protection \
	      ../src/backend \
	      ../src/backend/audio \
	      ../src/backend/data \
	      ../src/backend/data/mot \
	      ../src/backend/data/journaline \
	      ../src/output \
	      ../src/support \
	      ../src/support/viterbi-jan \
	      ../src/support/viterbi-spiral \
	      ../includes/ofdm \
	      ../includes/protection \
	      ../includes/backend \
	      ../includes/backend/audio \
	      ../includes/backend/data \
	      ../includes/backend/data/mot \
	      ../includes/backend/data/journaline \
	      ../includes/output \
	      ../includes/support \
	      ./devices-dab-mini 

INCLUDEPATH += . \
	      .. \
	      ../src \
	      ../includes \
	      ../includes/protection \
	      ../includes/ofdm \
	      ../includes/backend \
	      ../includes/backend/audio \
	      ../includes/backend/data \
	      ../includes/backend/data/mot \
	      ../includes/backend/data/journaline \
	      ../includes/output \
	      ../includes/support \
	      ../includes/support/viterbi-jan \
	      ../includes/support/viterbi-spiral \
	      ./devices-dab-mini 

# Input
HEADERS += ./radio.h \
	   ../dab-processor.h \
	   ../includes/dab-constants.h \
	   ../includes/country-codes.h \
	   ../includes/ofdm/timesyncer.h \
	   ../includes/ofdm/sample-reader.h \
	   ../includes/ofdm/ofdm-decoder.h \
	   ../includes/ofdm/phasereference.h \
	   ../includes/ofdm/phasetable.h \
	   ../includes/ofdm/freq-interleaver.h \
	   ../includes/ofdm/tii_detector.h \
	   ../includes/ofdm/fic-handler.h \
	   ../includes/ofdm/fib-decoder.h  \
	   ../includes/ofdm/fib-table.h \
	   ../includes/ofdm/dab-config.h \
	   ../includes/protection/protTables.h \
	   ../includes/protection/protection.h \
	   ../includes/protection/eep-protection.h \
	   ../includes/protection/uep-protection.h \
	   ../includes/backend/msc-handler.h \
	   ../includes/backend/galois.h \
	   ../includes/backend/reed-solomon.h \
	   ../includes/backend/rscodec.h \
	   ../includes/backend/charsets.h \
	   ../includes/backend/firecode-checker.h \
	   ../includes/backend/frame-processor.h \
	   ../includes/backend/backend.h \
	   ../includes/backend/backend-driver.h \
	   ../includes/backend/backend-deconvolver.h \
	   ../includes/backend/audio/mp2processor.h \
	   ../includes/backend/audio/mp4processor.h \
	   ../includes/backend/audio/bitWriter.h \
	   ../includes/backend/data/data-processor.h \
	   ../includes/backend/data/pad-handler.h \
	   ../includes/backend/data/virtual-datahandler.h \
	   ../includes/backend/data/tdc-datahandler.h \
	   ../includes/backend/data/ip-datahandler.h \
	   ../includes/backend/data/mot/mot-handler.h \
	   ../includes/backend/data/mot/mot-object.h \
	   ../includes/backend/data/mot/mot-dir.h \
	   ../includes/backend/data/journaline-datahandler.h \
	   ../includes/backend/data/journaline/dabdatagroupdecoder.h \
	   ../includes/backend/data/journaline/crc_8_16.h \
	   ../includes/backend/data/journaline/log.h \
	   ../includes/backend/data/journaline/newssvcdec_impl.h \
	   ../includes/backend/data/journaline/Splitter.h \
	   ../includes/backend/data/journaline/dabdgdec_impl.h \
	   ../includes/backend/data/journaline/newsobject.h \
	   ../includes/backend/data/journaline/NML.h \
#	   ../includes/output/fir-filters.h \
	   ../includes/output/audio-base.h \
	   ../includes/output/newconverter.h \
	   ../includes/output/audiosink.h \
	   ../includes/support/process-params.h \
	   ../includes/support/viterbi-jan/viterbi-handler.h \
	   ../includes/support/viterbi-spiral/viterbi-spiral.h \
           ../includes/support/fft-handler.h \
	   ../includes/support/ringbuffer.h \
#	   ../includes/support/Xtan2.h \
	   ../includes/support/dab-params.h \
	   ../includes/support/band-handler.h \
	   ../includes/support/text-mapper.h \
	   ../includes/support/dab-tables.h \
	   ../includes/support/ensemble-printer.h \
	   ../includes/support/preset-handler.h \
	   ../includes/support/smallcombobox.h \
	   ../includes/support/smallspinbox.h \
	   ../includes/support/smallpushbutton.h \
	   ../includes/support/verysmallpushbutton.h \
	   ../includes/support/smallqlistview.h \
	   ../includes/support/presetcombobox.h \
	   ./devices-dab-mini/device-handler.h

FORMS	+= ./dab-mini.ui

SOURCES += ./main.cpp \
	   ./radio.cpp \
	   ../dab-processor.cpp \
	   ../src/ofdm/timesyncer.cpp \
	   ../src/ofdm/sample-reader.cpp \
	   ../src/ofdm/ofdm-decoder.cpp \
	   ../src/ofdm/phasereference.cpp \
	   ../src/ofdm/phasetable.cpp \
	   ../src/ofdm/freq-interleaver.cpp \
#	   ../src/ofdm/tii_table.cpp \
	   ../src/ofdm/tii_detector.cpp \
	   ../src/ofdm/fic-handler.cpp \
	   ../src/ofdm/fib-decoder.cpp  \
	   ../src/protection/protTables.cpp \
	   ../src/protection/protection.cpp \
	   ../src/protection/eep-protection.cpp \
	   ../src/protection/uep-protection.cpp \
	   ../src/backend/msc-handler.cpp \
	   ../src/backend/galois.cpp \
	   ../src/backend/reed-solomon.cpp \
	   ../src/backend/rscodec.cpp \
	   ../src/backend/charsets.cpp \
	   ../src/backend/firecode-checker.cpp \
#	   ../src/backend/frame-processor.cpp \
	   ../src/backend/backend.cpp \
           ../src/backend/backend-driver.cpp \
           ../src/backend/backend-deconvolver.cpp \
	   ../src/backend/audio/mp2processor.cpp \
	   ../src/backend/audio/mp4processor.cpp \
	   ../src/backend/audio/bitWriter.cpp \
	   ../src/backend/data/pad-handler.cpp \
	   ../src/backend/data/data-processor.cpp \
#	   ../src/backend/data/virtual-datahandler.cpp \
	   ../src/backend/data/tdc-datahandler.cpp \
	   ../src/backend/data/ip-datahandler.cpp \
	   ../src/backend/data/mot/mot-handler.cpp \
	   ../src/backend/data/mot/mot-object.cpp \
	   ../src/backend/data/mot/mot-dir.cpp \
	   ../src/backend/data/journaline-datahandler.cpp \
	   ../src/backend/data/journaline/crc_8_16.c \
	   ../src/backend/data/journaline/log.c \
	   ../src/backend/data/journaline/newssvcdec_impl.cpp \
	   ../src/backend/data/journaline/Splitter.cpp \
	   ../src/backend/data/journaline/dabdgdec_impl.c \
	   ../src/backend/data/journaline/newsobject.cpp \
	   ../src/backend/data/journaline/NML.cpp \
	   ../src/output/audio-base.cpp \
	   ../src/output/newconverter.cpp \
	   ../src/output/audiosink.cpp \
	   ../src/support/viterbi-jan/viterbi-handler.cpp \
	   ../src/support/viterbi-spiral/viterbi-spiral.cpp \
           ../src/support/fft-handler.cpp \
#	   ../src/support/Xtan2.cpp \
	   ../src/support/dab-params.cpp \
	   ../src/support/band-handler.cpp \
	   ../src/support/text-mapper.cpp \
	   ../src/support/dab-tables.cpp \
	   ../src/support/ensemble-printer.cpp \
	   ../src/support/preset-handler.cpp \
	   ../src/support/presetcombobox.cpp \
	   ../src/support/smallcombobox.cpp \
	   ../src/support/smallspinbox.cpp \
	   ../src/support/smallpushbutton.cpp \
	   ../src/support/verysmallpushbutton.cpp \
	   ../src/support/smallqlistview.cpp \
	   ./devices-dab-mini/device-handler.cpp
#
#
unix {
DESTDIR		= ./linux-bin
exists ("./.git") {
   GITHASHSTRING = $$system(git rev-parse --short HEAD)
   !isEmpty(GITHASHSTRING) {
       message("Current git hash = $$GITHASHSTRING")
       DEFINES += GITHASH=\\\"$$GITHASHSTRING\\\"
   }
}
isEmpty(GITHASHSTRING) {
    DEFINES += GITHASH=\\\"------\\\"
}

INCLUDEPATH	+= /usr/local/include
LIBS		+= -lfftw3f  -lfftw3 -lusb-1.0 -ldl  #
LIBS		+= -lportaudio
LIBS		+= -lz
LIBS		+= -lsndfile
LIBS		+= -lsamplerate
#
# comment or uncomment for the devices you want to have support for
# (you obviously have libraries installed for the selected ones)
CONFIG		+= dabstick
CONFIG		+= sdrplay
#CONFIG		+= sdrplay-v3
CONFIG		+= airspy
CONFIG		+= hackrf
CONFIG		+= lime
CONFIG		+= pluto
#
CONFIG		+= faad
#CONFIG		+= fdk-aac

#if you want to listen remote, uncomment
#CONFIG		+= tcp-streamer		# use for remote listening
#otherwise, if you want to use the default qt way of sound out

#CONFIG		+= try-epg		# do not use
CONFIG		+= PC
#CONFIG		+= RPI
}
#
# an attempt to have it run under W32 through cross compilation
win32 {

exists ("../.git") {
   GITHASHSTRING = $$system(git rev-parse --short HEAD)
   !isEmpty(GITHASHSTRING) {
       message("Current git hash = $$GITHASHSTRING")
       DEFINES += GITHASH=\\\"$$GITHASHSTRING\\\"
   }
}
isEmpty(GITHASHSTRING) {
    DEFINES += GITHASH=\\\"------\\\"
}

#for 64 bit
#	TARGET		= dabMini-64-1.0
#	DEFINES		+= __BITS64__
#	DESTDIR		= /usr/shared/sdr-j-development/w64-programs/windows-dab64-mini
#	INCLUDEPATH	+= /usr/x64-w64-mingw32/sys-root/mingw/include
#	LIBS		+= -L/usr/x64-w64-mingw32/sys-root/mingw/lib
#for 32 bit
	TARGET		= dabMini-32-1.0
	DESTDIR		= /usr/shared/sdr-j-development/w32-programs/windows-dab32-mini
	INCLUDEPATH	+= /usr/i686-w64-mingw32/sys-root/mingw/include
	LIBS		+= -L/usr/i686-w64-mingw32/sys-root/mingw/lib

#	common:
INCLUDEPATH	+= /usr/local/include
LIBS		+= -lfftw3f -lfftw3
LIBS		+= -lportaudio
LIBS		+= -lsndfile
LIBS		+= -lsamplerate
LIBS		+= -lole32
LIBS		+= -lwinpthread
LIBS		+= -lwinmm
LIBS 		+= -lstdc++
LIBS		+= -lws2_32
LIBS		+= -lusb-1.0
LIBS		+= -lz
CONFIG		+= faad
CONFIG		+= airspy
CONFIG		+= dabstick
CONFIG		+= sdrplay
CONFIG		+= hackrf
CONFIG		+= pluto
#CONFIG		+= lime

#CONFIG		+= PC
CONFIG		+= NO_SSE
}
#
try-epg	{
	DEFINES		+= TRY_EPG
	QT		+= xml
	DEPENDPATH	+= ../src/backend/data/epg \
	                   ../includes/backend/data/epg 
	INCLUDEPATH	+= ../includes/backend/data/epg 
	HEADERS		+= ../includes/backend/data/epg/epgdec.h 
	SOURCES		+= ../src/backend/data/epg/epgdec.cpp 
}

# for RPI2 use:
RPI	{
	DEFINES		+= __MSC_THREAD__
	DEFINES		+= __THREADED_BACKEND
	DEFINES		+= NEON_AVAILABLE
	QMAKE_CFLAGS	+=  -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4  
	QMAKE_CXXFLAGS	+=  -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4  
	HEADERS		+= ./src/support/viterbi-spiral/spiral-neon.h
	SOURCES		+= ./src/support/viterbi-spiral/spiral-neon.c
}

PC	{
#	DEFINES		+= __THREADED_BACKEND
#	DEFINES		+= __MSC_THREAD__
	DEFINES		+= SSE_AVAILABLE
	HEADERS		+= ../src/support/viterbi-spiral/spiral-sse.h
	SOURCES		+= ../src/support/viterbi-spiral/spiral-sse.c
}

NO_SSE	{
	HEADERS		+= ../src/support/viterbi-spiral/spiral-no-sse.h
	SOURCES		+= ../src/support/viterbi-spiral/spiral-no-sse.c
}

faad	{
	DEFINES		+= __WITH_FAAD__
	HEADERS		+= ../includes/backend/audio/faad-decoder.h 
	SOURCES		+= ../src/backend/audio/faad-decoder.cpp 
	LIBS		+= -lfaad
}

fdk-aac	{
	DEFINES		+= __WITH_FDK_AAC__
	INCLUDEPATH	+= ../specials/fdk-aac
	HEADERS		+= ../includes/backend/audio/fdk-aac.h 
	SOURCES		+= ../src/backend/audio/fdk-aac.cpp 
	LIBS		+= -lfdk-aac
}

#	devices
#       dabstick
dabstick {
        DEFINES         += HAVE_RTLSDR
        DEPENDPATH      += ./devices-dab-mini/rtlsdr-handler
        INCLUDEPATH     += ./devices-dab-mini/rtlsdr-handler
        HEADERS         += ./devices-dab-mini/rtlsdr-handler/rtlsdr-handler.h
        SOURCES         += ./devices-dab-mini/rtlsdr-handler/rtlsdr-handler.cpp
}
#
#       the SDRplay
#
sdrplay {
        DEFINES         += HAVE_SDRPLAY
        DEPENDPATH      += ./devices-dab-mini/sdrplay-handler
        INCLUDEPATH     += ./devices-dab-mini/sdrplay-handler
        HEADERS         += ./devices-dab-mini/sdrplay-handler/sdrplay-handler.h
        SOURCES         += ./devices-dab-mini/sdrplay-handler/sdrplay-handler.cpp
}
#
sdrplay-v3 {
	DEFINES		+= HAVE_SDRPLAY_V3
	DEPENDPATH	+= ./devices-dab-mini/sdrplay-handler-v3
	INCLUDEPATH	+= ./devices-dab-mini/sdrplay-handler-v3 \
	                   ./devices-dab-mini/sdrplay-handler-v3/include
	HEADERS		+= ./devices-dab-mini/sdrplay-handler-v3/sdrplay-handler-v3.h \
	                   ./devices-dab-mini/sdrplay-handler-v3/sdrplay-commands.h 
	SOURCES		+= ./devices-dab-mini/sdrplay-handler-v3/sdrplay-handler-v3.cpp 
	FORMS		+= ./devices-dab-mini/sdrplay-handler-v3/sdrplay-widget-v3.ui
}
#
# airspy support
#
airspy {
	DEFINES		+= HAVE_AIRSPY
	DEPENDPATH	+= ./devices-dab-mini/airspy 
	INCLUDEPATH	+= ./devices-dab-mini/airspy-handler \
	                   ./devices-dab-mini/airspy-handler/libairspy
	HEADERS		+= ./devices-dab-mini/airspy-handler/airspy-handler.h \
	                   ./devices-dab-mini/airspy-handler/airspyfilter.h \
	                   ./devices-dab-mini/airspy-handler/libairspy/airspy.h
	SOURCES		+= ./devices-dab-mini/airspy-handler/airspy-handler.cpp \
	                   ./devices-dab-mini/airspy-handler/airspyfilter.cpp
}
#
#       the HACKRF One
#
hackrf {
        DEFINES         += HAVE_HACKRF
        DEPENDPATH      += ./devices-dab-mini/hackrf-handler
        INCLUDEPATH     += ./devices-dab-mini/hackrf-handler
        HEADERS         += ./devices-dab-mini/hackrf-handler/hackrf-handler.h
        SOURCES         += ./devices-dab-mini/hackrf-handler/hackrf-handler.cpp
}
#
#       the Lime SDR
#
lime {
        DEFINES         += HAVE_LIME
        DEPENDPATH      += ./devices-dab-mini/lime-handler
        INCLUDEPATH     += ./devices-dab-mini/lime-handler
        HEADERS         += ./devices-dab-mini/lime-handler/lime-handler.h \
	                   ./devices-dab-mini/lime-handler/LimeSuite.h \
	                   ./devices-dab-mini/lime-handler/LMS7002M_parameters.h
        SOURCES         += ./devices-dab-mini/lime-handler/lime-handler.cpp
}
#
pluto   {
        DEFINES         += HAVE_PLUTO
        QT              += network
        INCLUDEPATH     += ./devices-dab-mini/pluto-handler
        HEADERS         += ./devices-dab-mini/pluto-handler/pluto-handler.h
        SOURCES         += ./devices-dab-mini/pluto-handler/pluto-handler.cpp
	LIBS            += -liio -lad9361
}

