#ifndef __AUDIO_DESCRIPTOR__
#define __AUDIO_DESCRIPTOR__

#include "dab-constants.h"
#include "service-descriptor.h"
#include "text-mapper.h"
#include "ui_audio-description.h"
#include <QFrame>
#include <QObject>
#include <QSettings>
#include <atomic>

class	audioDescriptor : public serviceDescriptor, public Ui_audioDescription
{
public:
  audioDescriptor(audiodata * ad);
  ~audioDescriptor() override = default;

private:
  QFrame     mMyFrame;
  textMapper mThe_textMapper;
};

#endif

	
