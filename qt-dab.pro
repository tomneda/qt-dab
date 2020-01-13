######################################################################
# Automatically generated by qmake (2.01a) Tue Oct 6 19:48:14 2009
# but modified by me to accomodate for the includes for qwt, hamlib and
# portaudio
######################################################################

TEMPLATE	= app
TARGET		= qt-dab-3.21-Beta
QT		+= widgets xml
#CONFIG		+= console
CONFIG		-= console
QMAKE_CXXFLAGS	+= -std=c++11
#QMAKE_CFLAGS	+=  -flto -ffast-math
#MAKE_CXXFLAGS	+=  -flto -ffast-math
QMAKE_CFLAGS	+=  -g
QMAKE_CXXFLAGS	+=  -g
QMAKE_LFLAGS	+=  -g
QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]
RC_ICONS	=  qt-dab.ico
RESOURCES	+= resources.qrc

TRANSLATIONS = i18n/de_DE.ts

DEPENDPATH += . \
	      ./src \
	      ./includes \
	      ./service-description \
	      ./src/ofdm \
	      ./src/protection \
	      ./src/backend \
	      ./src/backend/audio \
	      ./src/backend/data \
	      ./src/backend/data/mot \
	      ./src/backend/data/journaline \
	      ./src/output \
	      ./src/support \
	      ./src/support/viterbi-jan \
	      ./src/support/viterbi-spiral \
	      ./includes/ofdm \
	      ./includes/protection \
	      ./includes/backend \
	      ./includes/backend/audio \
	      ./includes/backend/data \
	      ./includes/backend/data/mot \
	      ./includes/backend/data/journaline \
	      ./includes/output \
	      ./includes/support \
	      ./devices \
	      ./devices/rawfiles-new \
	      ./devices/wavfiles-new\
	      ./devices/xml-filereader \
	      ./includes/scopes-qwt6 \
              ./spectrum-viewer \
	      ./correlation-viewer \
	      ./tii-viewer

INCLUDEPATH += . \
	      ./ \
	      ./src \
	      ./includes \
	      ./service-description \
	      ./includes/protection \
	      ./includes/ofdm \
	      ./includes/backend \
	      ./includes/backend/audio \
	      ./includes/backend/data \
	      ./includes/backend/data/mot \
	      ./includes/backend/data/journaline \
	      ./includes/output \
	      ./includes/support \
	      ./includes/support/viterbi-jan \
	      ./includes/support/viterbi-spiral \
	      ./devices \
	      ./devices/rawfiles-new \
	      ./devices/wavfiles-new \
	      ./devices/xml-filereader \
	      ./includes/scopes-qwt6 \
              ./spectrum-viewer \
	      ./correlation-viewer \
	      ./tii-viewer

# Input
HEADERS += ./radio.h \
	   ./dab-processor.h \
	   ./service-description/service-descriptor.h \
	   ./service-description/audio-descriptor.h \
	   ./service-description/data-descriptor.h \
	   ./includes/dab-constants.h \
	   ./includes/country-codes.h \
	   ./includes/ofdm/timesyncer.h \
	   ./includes/ofdm/sample-reader.h \
	   ./includes/ofdm/ofdm-decoder.h \
	   ./includes/ofdm/phasereference.h \
	   ./includes/ofdm/phasetable.h \
	   ./includes/ofdm/freq-interleaver.h \
#	   ./includes/ofdm/tii_table.h \
	   ./includes/ofdm/tii_detector.h \
	   ./includes/ofdm/fic-handler.h \
	   ./includes/ofdm/fib-decoder.h  \
	   ./includes/protection/protTables.h \
	   ./includes/protection/protection.h \
	   ./includes/protection/eep-protection.h \
	   ./includes/protection/uep-protection.h \
	   ./includes/backend/msc-handler.h \
	   ./includes/backend/galois.h \
	   ./includes/backend/reed-solomon.h \
	   ./includes/backend/rscodec.h \
	   ./includes/backend/charsets.h \
	   ./includes/backend/firecode-checker.h \
	   ./includes/backend/frame-processor.h \
	   ./includes/backend/backend.h \
	   ./includes/backend/backend-driver.h \
	   ./includes/backend/backend-deconvolver.h \
	   ./includes/backend/audio/mp2processor.h \
	   ./includes/backend/audio/mp4processor.h \
	   ./includes/backend/audio/bitWriter.h \
	   ./includes/backend/audio/faad-decoder.h \
	   ./includes/backend/data/data-processor.h \
	   ./includes/backend/data/pad-handler.h \
	   ./includes/backend/data/virtual-datahandler.h \
	   ./includes/backend/data/tdc-datahandler.h \
	   ./includes/backend/data/ip-datahandler.h \
	   ./includes/backend/data/mot/mot-handler.h \
	   ./includes/backend/data/mot/mot-object.h \
	   ./includes/backend/data/mot/mot-dir.h \
	   ./includes/backend/data/journaline-datahandler.h \
	   ./includes/backend/data/journaline/dabdatagroupdecoder.h \
	   ./includes/backend/data/journaline/crc_8_16.h \
	   ./includes/backend/data/journaline/log.h \
	   ./includes/backend/data/journaline/newssvcdec_impl.h \
	   ./includes/backend/data/journaline/Splitter.h \
	   ./includes/backend/data/journaline/dabdgdec_impl.h \
	   ./includes/backend/data/journaline/newsobject.h \
	   ./includes/backend/data/journaline/NML.h \
#	   ./includes/output/fir-filters.h \
	   ./includes/output/audio-base.h \
	   ./includes/output/newconverter.h \
	   ./includes/output/audiosink.h \
	   ./includes/support/viterbi-jan/viterbi-handler.h \
	   ./includes/support/viterbi-spiral/viterbi-spiral.h \
           ./includes/support/fft-handler.h \
	   ./includes/support/ringbuffer.h \
#	   ./includes/support/Xtan2.h \
	   ./includes/support/dab-params.h \
	   ./includes/support/band-handler.h \
	   ./includes/support/text-mapper.h \
	   ./includes/support/dab_tables.h \
	   ./includes/support/ensemble-printer.h \
	   ./includes/support/preset-handler.h \
	   ./includes/support/presetcombobox.h \
	   ./includes/support/smallcombobox.h \
	   ./includes/support/smallpushbutton.h \
	   ./includes/support/verysmallpushbutton.h \
	   ./includes/support/smallqlistview.h \
	   ./includes/support/history-handler.h \
	   ./includes/scopes-qwt6/spectrogramdata.h \
	   ./includes/scopes-qwt6/iqdisplay.h \
	   ./devices/virtual-input.h \
	   ./devices/xml-filewriter.h \
	   ./devices/filereader-widget.h \
	   ./devices/rawfiles-new/rawfiles.h \
	   ./devices/rawfiles-new/raw-reader.h \
           ./devices/wavfiles-new/wavfiles.h \
           ./devices/wavfiles-new/wav-reader.h \
	   ./devices/xml-filereader/xml-filereader.h \
	   ./devices/xml-filereader/xml-reader.h \
	   ./devices/xml-filereader/xml-descriptor.h \
	   ./spectrum-viewer/spectrum-viewer.h \
	   ./correlation-viewer/correlation-viewer.h \
	   ./tii-viewer/tii-viewer.h

FORMS	+= ./forms/technical_data.ui
FORMS	+= ./forms/dabradio.ui 
FORMS	+= ./forms/audio-description.ui
FORMS	+= ./forms/data-description.ui
FORMS	+= ./spectrum-viewer/scopewidget.ui
FORMS	+= ./correlation-viewer/correlation-widget.ui
FORMS	+= ./tii-viewer/tii-widget.ui
#FORMS	+= ./devices/filereader-widget.ui 
FORMS	+= ./devices/xml-filereader/xmlfiles.ui

SOURCES += ./main.cpp \
	   ./radio.cpp \
	   ./dab-processor.cpp \
	   ./service-description/audio-descriptor.cpp \
	   ./service-description/data-descriptor.cpp \
	   ./src/ofdm/timesyncer.cpp \
	   ./src/ofdm/sample-reader.cpp \
	   ./src/ofdm/ofdm-decoder.cpp \
	   ./src/ofdm/phasereference.cpp \
	   ./src/ofdm/phasetable.cpp \
	   ./src/ofdm/freq-interleaver.cpp \
#	   ./src/ofdm/tii_table.cpp \
	   ./src/ofdm/tii_detector.cpp \
	   ./src/ofdm/fic-handler.cpp \
	   ./src/ofdm/fib-decoder.cpp  \
	   ./src/protection/protTables.cpp \
	   ./src/protection/protection.cpp \
	   ./src/protection/eep-protection.cpp \
	   ./src/protection/uep-protection.cpp \
	   ./src/backend/msc-handler.cpp \
	   ./src/backend/galois.cpp \
	   ./src/backend/reed-solomon.cpp \
	   ./src/backend/rscodec.cpp \
	   ./src/backend/charsets.cpp \
	   ./src/backend/firecode-checker.cpp \
	   ./src/backend/frame-processor.cpp \
	   ./src/backend/backend.cpp \
           ./src/backend/backend-driver.cpp \
           ./src/backend/backend-deconvolver.cpp \
	   ./src/backend/audio/mp2processor.cpp \
	   ./src/backend/audio/mp4processor.cpp \
	   ./src/backend/audio/bitWriter.cpp \
	   ./src/backend/audio/faad-decoder.cpp \
	   ./src/backend/data/pad-handler.cpp \
	   ./src/backend/data/data-processor.cpp \
	   ./src/backend/data/virtual-datahandler.cpp \
	   ./src/backend/data/tdc-datahandler.cpp \
	   ./src/backend/data/ip-datahandler.cpp \
	   ./src/backend/data/mot/mot-handler.cpp \
	   ./src/backend/data/mot/mot-object.cpp \
	   ./src/backend/data/mot/mot-dir.cpp \
	   ./src/backend/data/journaline-datahandler.cpp \
	   ./src/backend/data/journaline/crc_8_16.c \
	   ./src/backend/data/journaline/log.c \
	   ./src/backend/data/journaline/newssvcdec_impl.cpp \
	   ./src/backend/data/journaline/Splitter.cpp \
	   ./src/backend/data/journaline/dabdgdec_impl.c \
	   ./src/backend/data/journaline/newsobject.cpp \
	   ./src/backend/data/journaline/NML.cpp \
	   ./src/output/audio-base.cpp \
	   ./src/output/newconverter.cpp \
	   ./src/output/audiosink.cpp \
	   ./src/support/viterbi-jan/viterbi-handler.cpp \
	   ./src/support/viterbi-spiral/viterbi-spiral.cpp \
           ./src/support/fft-handler.cpp \
#	   ./src/support/Xtan2.cpp \
	   ./src/support/dab-params.cpp \
	   ./src/support/band-handler.cpp \
	   ./src/support/text-mapper.cpp \
	   ./src/support/dab_tables.cpp \
	   ./src/support/ensemble-printer.cpp \
	   ./src/support/preset-handler.cpp \
	   ./src/support/presetcombobox.cpp \
	   ./src/support/smallcombobox.cpp \
	   ./src/support/smallpushbutton.cpp \
	   ./src/support/verysmallpushbutton.cpp \
	   ./src/support/smallqlistview.cpp \
	   ./src/support/history-handler.cpp \
	   ./src/scopes-qwt6/iqdisplay.cpp \
	   ./devices/virtual-input.cpp \
	   ./devices/xml-filewriter.cpp \
	   ./devices/rawfiles-new/rawfiles.cpp \
	   ./devices/rawfiles-new/raw-reader.cpp \
           ./devices/wavfiles-new/wavfiles.cpp \
           ./devices/wavfiles-new/wav-reader.cpp \
	   ./devices/xml-filereader/xml-filereader.cpp \
	   ./devices/xml-filereader/xml-reader.cpp \
	   ./devices/xml-filereader/xml-descriptor.cpp \
	   ./spectrum-viewer/spectrum-viewer.cpp \
	   ./correlation-viewer/correlation-viewer.cpp \
	   ./tii-viewer/tii-viewer.cpp
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
INCLUDEPATH	+= /usr/local/include /usr/include/qt4/qwt /usr/include/qt5/qwt /usr/include/qt4/qwt /usr/include/qwt /usr/local/qwt-6.1.4-svn/
LIBS		+= -lfftw3f  -lfftw3 -lusb-1.0 -ldl  #
LIBS		+= -lportaudio
LIBS		+= -lz
LIBS		+= -lsndfile
LIBS		+= -lsamplerate
LIBS		+= -lfaad
#correct this for the correct path to the qwt6 library on your system
#LIBS		+= -lqwt
LIBS		+= -lqwt-qt5
#
# comment or uncomment for the devices you want to have support for
# (you obviously have libraries installed for the selected ones)
CONFIG		+= dabstick
CONFIG		+= sdrplay-v2
CONFIG		+= sdrplay-v3		# pretty experimental
CONFIG		+= lime
CONFIG		+= rtl_tcp
CONFIG		+= airspy
CONFIG		+= hackrf
CONFIG		+= soapy
#CONFIG		+= elad_s1	# does not work yet

#very experimental, simple server for connecting to a tdc handler
#CONFIG		+= datastreamer

#to handle output of embedded an IP data stream, uncomment
CONFIG		+= send_datagram

#if you want to listen remote, uncomment
#CONFIG		+= tcp-streamer		# use for remote listening
#otherwise, if you want to use the default qt way of soud out
#CONFIG		+= qt-audio
#comment both out if you just want to use the "normal" way

CONFIG		+= try-epg		# do not use
DEFINES		+= PRESET_NAME
DEFINES		+= __THREADED_BACKEND
#DEFINES	+= SHOW_MISSING

#For x64 linux system uncomment SSE
#For any other system comment SSE out and uncomment NO_SSE
#CONFIG	+= SSE
CONFIG	+= NO_SSE
}
#
# an attempt to have it run under W32 through cross compilation
win32 {
#DESTDIR	= ../../../dab-win
DESTDIR		=  ../../windows-qt-dab
#DESTDIR	= /usr/shared/sdr-j-development/windows-qt-dab
# includes in mingw differ from the includes in fedora linux

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

INCLUDEPATH	+= /usr/i686-w64-mingw32/sys-root/mingw/include
INCLUDEPATH	+= /usr/local/include /usr/include/qt4/qwt /usr/include/qt5/qwt /usr/include/qt4/qwt /usr/include/qwt /usr/local/qwt-6.1.4-svn/
INCLUDEPATH	+= /mingw32/include
INCLUDEPATH	+= /mingw32/include/qwt
INCLUDEPATH	+= /usr/local/include
LIBS		+= -L/usr/i686-w64-mingw32/sys-root/mingw/lib
#INCLUDEPATH	+= /mingw/include
#INCLUDEPATH	+= /mingw64/include/qwt
#INCLUDEPATH	+= C:/msys64/mingw64/include/qwt
LIBS		+= -lfaad
LIBS		+= -lfftw3f -lfftw3
LIBS		+= -lportaudio
LIBS		+= -lsndfile
LIBS		+= -lsamplerate
LIBS		+= -lole32
LIBS		+= -lwinpthread
LIBS		+= -lwinmm
LIBS 		+= -lstdc++
LIBS		+= -lws2_32
LIBS		+= -lfaad
LIBS		+= -lusb-1.0
LIBS		+= -lz
#correct this for the correct path to the qwt6 library on your system
#mingw64 wants the first one, cross compiling mingw64-32 the second one
#LIBS		+= -lqwt
LIBS		+= -lqwt-qt5

#CONFIG		+= extio
CONFIG		+= airspy
CONFIG		+= rtl_tcp
CONFIG		+= dabstick
CONFIG		+= sdrplay-v2
CONFIG		+= sdrplay-v3
CONFIG		+= hackrf
#CONFIG		+= lime
CONFIG		+= NO_SSE

#very experimental, simple server for connecting to a tdc handler
#CONFIG		+= datastreamer

#if you want to listen remote, uncomment
#CONFIG		+= tcp-streamer		# use for remote listening
#otherwise, if you want to use the default qt way of soud out
#CONFIG		+= qt-audio
#comment both out if you just want to use the "normal" way

CONFIG		+= try-epg		# do not use
DEFINES		+= PRESET_NAME
}
#	devices
#
#	dabstick
dabstick {
	DEFINES		+= HAVE_RTLSDR
	DEPENDPATH	+= ./devices/rtlsdr-handler
	INCLUDEPATH	+= ./devices/rtlsdr-handler
	HEADERS		+= ./devices/rtlsdr-handler/rtlsdr-handler.h \
	                   ./devices/rtlsdr-handler/rtl-dongleselect.h
	SOURCES		+= ./devices/rtlsdr-handler/rtlsdr-handler.cpp \
	                   ./devices/rtlsdr-handler/rtl-dongleselect.cpp
	FORMS		+= ./devices/rtlsdr-handler/rtlsdr-widget.ui
}

#
#	the SDRplay
#
sdrplay-v2 {
	DEFINES		+= HAVE_SDRPLAY_V2
	DEPENDPATH	+= ./devices/sdrplay-handler-v2
	INCLUDEPATH	+= ./devices/sdrplay-handler-v2
	HEADERS		+= ./devices/sdrplay-handler-v2/sdrplay-handler-v2.h \
	                   ./devices/sdrplay-handler-v2/sdrplayselect.h 
	SOURCES		+= ./devices/sdrplay-handler-v2/sdrplay-handler-v2.cpp \
	                   ./devices/sdrplay-handler-v2/sdrplayselect.cpp 
	FORMS		+= ./devices/sdrplay-handler-v2/sdrplay-widget-v2.ui
}
#
#	the SDRplay
#
sdrplay-v3 {
	DEFINES		+= HAVE_SDRPLAY_V3
	DEPENDPATH	+= ./devices/sdrplay-handler-v3
	INCLUDEPATH	+= ./devices/sdrplay-handler-v3
	HEADERS		+= ./devices/sdrplay-handler-v3/sdrplay-handler-v3.h \
	                   ./devices/sdrplay-handler-v3/control-queue.h \
	                   ./devices/sdrplay-handler-v3/sdrplay-controller.h 
	SOURCES		+= ./devices/sdrplay-handler-v3/sdrplay-handler-v3.cpp \
	                   ./devices/sdrplay-handler-v3/control-queue.cpp \
	                   ./devices/sdrplay-handler-v3/sdrplay-controller.cpp 
	FORMS		+= ./devices/sdrplay-handler-v3/sdrplay-widget-v3.ui
}
#
#	limeSDR
#
lime  {
	DEFINES		+= HAVE_LIME
	INCLUDEPATH	+= ./devices/lime-handler
	DEPENDPATH	+= ./devices/lime-handler
        HEADERS         += ./devices/lime-handler/lime-handler.h \	
	                   ./devices/lime-handler/lime-widget.h
        SOURCES         += ./devices/lime-handler/lime-handler.cpp 
}

#
#	the hackrf
#
hackrf {
	DEFINES		+= HAVE_HACKRF
	DEPENDPATH	+= ./devices/hackrf-handler 
	INCLUDEPATH	+= ./devices/hackrf-handler 
	HEADERS		+= ./devices/hackrf-handler/hackrf-handler.h 
	SOURCES		+= ./devices/hackrf-handler/hackrf-handler.cpp 
	FORMS		+= ./devices/hackrf-handler/hackrf-widget.ui
}
#
#
# airspy support
#
airspy {
	DEFINES		+= HAVE_AIRSPY
	DEPENDPATH	+= ./devices/airspy 
	INCLUDEPATH	+= ./devices/airspy-handler \
	                   ./devices/airspy-handler/libairspy
	HEADERS		+= ./devices/airspy-handler/airspy-handler.h \
	                   ./devices/airspy-handler/airspyfilter.h \
	                   ./devices/airspy-handler/libairspy/airspy.h
	SOURCES		+= ./devices/airspy-handler/airspy-handler.cpp \
	                   ./devices/airspy-handler/airspyfilter.cpp
	FORMS		+= ./devices/airspy-handler/airspy-widget.ui
}

#	extio dependencies, windows only
#
extio {
	DEFINES		+= HAVE_EXTIO
	INCLUDEPATH	+= ./devices/extio-handler
	HEADERS		+= ./devices/extio-handler/extio-handler.h \
	                   ./devices/extio-handler/common-readers.h \
	                   ./devices/extio-handler/virtual-reader.h
	SOURCES		+= ./devices/extio-handler/extio-handler.cpp \
	                   ./devices/extio-handler/common-readers.cpp \
	                   ./devices/extio-handler/virtual-reader.cpp
}

#
rtl_tcp {
	DEFINES		+= HAVE_RTL_TCP
	QT		+= network
	INCLUDEPATH	+= ./devices/rtl_tcp
	HEADERS		+= ./devices/rtl_tcp/rtl_tcp_client.h
	SOURCES		+= ./devices/rtl_tcp/rtl_tcp_client.cpp
	FORMS		+= ./devices/rtl_tcp/rtl_tcp-widget.ui
}

soapy {
	DEFINES		+= HAVE_SOAPY
	INCLUDEPATH     += ./devices/soapy
        HEADERS         += ./devices/soapy/soapy-handler.h \
	                   ./devices/soapy/soapy-worker.h \
	                   ./devices/soapy/soapy_CS16.h \
	                   ./devices/soapy/soapy_CF32.h
        SOURCES         += ./devices/soapy/soapy-handler.cpp \
	                   ./devices/soapy/soapy-worker.cpp \
	                   ./devices/soapy/soapy_CS16.cpp \
	                   ./devices/soapy/soapy_CF32.cpp
        FORMS           += ./devices/soapy/soapy-widget.ui
	LIBS		+= -lSoapySDR -lm
}

send_datagram {
	DEFINES		+= _SEND_DATAGRAM_
	QT		+= network
}

elad_s1	{
	DEFINES		+= HAVE_ELAD_S1
	DEPENDPATH	+= ./devices/elad-s1-handler
	INCLUDEPATH	+= ./devices/elad-s1-handler
	HEADERS		+= ./devices/elad-s1-handler/elad-handler.h \
	                   ./devices/elad-s1-handler/elad-filter.h \
	                   ./devices/elad-s1-handler/elad-worker.h \
	                   ./devices/elad-s1-handler/elad-loader.h 
	SOURCES		+= ./devices/elad-s1-handler/elad-handler.cpp \
	                   ./devices/elad-s1-handler/elad-filter.cpp \
	                   ./devices/elad-s1-handler/elad-worker.cpp \
	                   ./devices/elad-s1-handler/elad-loader.cpp 
	FORMS		+= ./devices/elad-s1-handler/widget.ui
}

try-epg	{
	DEFINES		+= TRY_EPG
	QT		+= xml
	DEPENDPATH	+= ./src/backend/data/epg \
	                   ./includes/backend/data/epg 
	INCLUDEPATH	+= ./includes/backend/data/epg 
	HEADERS		+= ./includes/backend/data/epg/epgdec.h 
	SOURCES		+= ./src/backend/data/epg/epgdec.cpp 
}

tcp-streamer	{
	DEFINES		+= TCP_STREAMER
	QT		+= network
	HEADERS		+= ./includes/output/tcp-streamer.h
	SOURCES		+= ./src/output/tcp-streamer.cpp
}

qt-audio	{
	DEFINES		+= QT_AUDIO
	QT		+= multimedia
	HEADERS		+= ./includes/output/Qt-audio.h \
	                   ./includes/output/Qt-audiodevice.h
	SOURCES		+= ./src/output/Qt-audio.cpp \
	                   ./src/output/Qt-audiodevice.cpp
}

datastreamer	{
	DEFINES		+= DATA_STREAMER
	INCLUDEPATH	+= ./server-thread
	HEADERS		+= ./server-thread/tcp-server.h
	SOURCES		+= ./server-thread/tcp-server.cpp
}


# for RPI2 use:
NEON_RPI2	{
	DEFINES		+= NEON_AVAILABLE
	QMAKE_CFLAGS	+=  -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4  
	QMAKE_CXXFLAGS	+=  -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4  
	HEADERS		+= ./src/support/viterbi-spiral/spiral-neon.h
	SOURCES		+= ./src/support/viterbi-spiral/spiral-neon.c
}

# for RPI3 use:
NEON_RPI3	{
	DEFINES		+= NEON_AVAILABLE
#	QMAKE_CFLAGS	+=  -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -mneon-for-64bits
#	QMAKE_CXXFLAGS	+=  -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -mneon-for-64bits
	HEADERS		+= ./src/support/viterbi-spiral/spiral-neon.h
	SOURCES		+= ./src/support/viterbi-spiral/spiral-neon.c
}

SSE	{
	DEFINES		+= SSE_AVAILABLE
	HEADERS		+= ./src/support/viterbi-spiral/spiral-sse.h
	SOURCES		+= ./src/support/viterbi-spiral/spiral-sse.c
}

NO_SSE	{
	HEADERS		+= ./src/support/viterbi-spiral/spiral-no-sse.h
	SOURCES		+= ./src/support/viterbi-spiral/spiral-no-sse.c
}


