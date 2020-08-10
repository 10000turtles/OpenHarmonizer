// Compiled with: g++ -Wall -D__LINUX_ALSA__ -o midiLivePatch.exe midiLivePatch.cpp RtMidi.cpp -lasound -lpthread -lsfml-audio -lsfml-system

#include "RtMidi.h"
#include "smbPitchShift.cpp"
#include "unistd.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <cstdlib>
#include <iostream>
#include <signal.h>
#include <string>

using namespace std;

class MyStream : public sf::SoundStream
{
 public:
  void load(const sf::SoundBuffer& buffer)
  {
    // extract the audio samples from the sound buffer to our own container
    m_samples.assign(buffer.getSamples(), buffer.getSamples() + buffer.getSampleCount());

    // reset the current playing position
    m_currentSample = 0;

    // initialize the base class
    initialize(buffer.getChannelCount(), buffer.getSampleRate());
  }

 private:
  virtual bool onGetData(Chunk& data)
  {
    // number of samples to stream every time the function is called;
    // in a more robust implementation, it should be a fixed
    // amount of time rather than an arbitrary number of samples
    const int samplesToStream = 50000;

    // set the pointer to the next audio samples to be played
    data.samples = &m_samples[m_currentSample];

    // have we reached the end of the sound?
    if (m_currentSample + samplesToStream <= m_samples.size())
    {
      // end not reached: stream the samples and continue
      data.sampleCount = samplesToStream;
      m_currentSample += samplesToStream;
      return true;
    }
    else
    {
      // end of stream reached: stream the remaining samples and stop playback
      data.sampleCount = m_samples.size() - m_currentSample;
      m_currentSample  = m_samples.size();
      return false;
    }
  }

  virtual void onSeek(sf::Time timeOffset)
  {
    // compute the corresponding sample index according to the sample rate and channel count
    m_currentSample = static_cast<std::size_t>(timeOffset.asSeconds() * getSampleRate() * getChannelCount());
  }

  std::vector<sf::Int16> m_samples;
  std::size_t            m_currentSample;
};

class LiveHarmonizer : public sf::SoundRecorder
{
 public:
  RtMidiIn*       midiin;
  sf::SoundBuffer buffer;
  sf::Sound       sound;
  vector<int>     notes;
  MyStream        stream;

  virtual bool onStart()  // optional
  {
    for (int k = 0; k < 88; k++)
    {
      notes.push_back(0);
    }

    midiin = new RtMidiIn();

    unsigned int nPorts = midiin->getPortCount();

    if (nPorts == 0)
    {
      std::cout << "No ports available!\n";
      return false;
    }

    midiin->openPort(1);
    midiin->ignoreTypes(false, false, false);


    return true;
  }

  virtual bool onProcessSamples(const sf::Int16* s, std::size_t sc)
  {
    // int    voices        = 1;
    // double voiceValues[] = {1, 1.2599, 1.4983, 2};
    // // Major Third: 1.2599
    // // Fifth: 1.4983
    // // Octave: 2.0000

    // sf::Int16 newS[voices][sc];

    // for (int j = 0; j < voices; j++)
    // {
    //   for (int i = 0; i < sc; i++)
    //   {
    //     newS[j][i] = s[i];
    //   }
    // }

    // float temp[voices][sc];

    // for (int j = 0; j < voices; j++)
    // {
    //   for (int i = 0; i < sc; i++)
    //   {
    //     temp[j][i] = (float)newS[j][i] / 20675;
    //   }
    // }

    // for (int j = 0; j < voices; j++)
    // {
    //   smbPitchShift(voiceValues[j], sc, 1024, 32, 44100, temp[voices - 1], temp[j]);
    // }

    // for (int j = 0; j < voices; j++)
    // {
    //   for (int i = 0; i < sc; i++)
    //   {
    //     newS[j][i] = temp[j][i] * 20675;
    //   }
    // }

    // sf::Int16 synthesis[sc];

    // for (int i = 0; i < sc; i++)
    // {
    //   for (int j = 0; j < voices; j++)
    //   {
    //     synthesis[i] = newS[j][i] / voices;
    //   }
    // }

    // buffer.loadFromSamples(synthesis, sc, 1, 44100);
    // stream.load(buffer);
    // stream.play();
    // buffer.saveToFile("temp.ogg");
    return true;
  }

  virtual void onStop()  // optional
  {
    cout << "Stopping" << endl;
  }
};


int main()
{
  if (!LiveHarmonizer::isAvailable())
  {
    cerr << "It ain't working rn sorry bruv." << endl;
  }

  string filename;
  cout << "Enter the note you want to play (\"note\" if the file is note.wav): " << endl;
  cin >> filename;

  int keys = 88;

  sf::SoundBuffer buffers[keys];

  LiveHarmonizer recorder[keys];

  std::vector<unsigned char> message;

  int    nBytes;
  double stamp;
  bool   pedal = false;

  for (int i = 0; i < keys; i++)
  {
    recorder[i].start();
  }

  for (int i = 0; i < keys; i++)
  {
    if (!buffers[i].loadFromFile("soundFiles/" + filename + "/" + filename + to_string(i) + ".ogg"))
      cerr << "It ain't working rn sorry bruv." << endl;
    recorder[i].stream.load(buffers[i]);
    recorder[i].stream.setRelativeToListener(true);
  }

  while (true)
  {
    stamp  = recorder[0].midiin->getMessage(&message);
    nBytes = message.size();
    if (nBytes > 0)
    {
      if ((int)message[0] == 144)
      {
        recorder[0].notes[message[1] - 21] = message[2];
        cout << "[ ";
        for (int k = 0; k < 88; k++)
        {
          if (recorder[0].notes[k] > 0)
          {
            cout << k << ": " << recorder[0].notes[k] << ", ";
          }
        }
        if (recorder[0].notes[message[1] - 21] > 0)
        {
          recorder[message[1] - 21].stream.play();
        }
        else if (recorder[0].notes[message[1] - 21] == 0 && !pedal)
        {
          recorder[message[1] - 21].stream.stop();
        }

        cout << " ]" << endl;
      }

      if ((int)message[0] == 176)
      {
        if (message[2] > 0)
        {
          pedal = true;
        }
        if (message[2] == 0)
        {
          pedal = false;
        }
      }

      if ((int)message[0] == 224)
      {
        for (int i = 0; i < 88; i++)
        {
          recorder[i].stream.setPosition((float)(message[2] - 64) * 0.1, 0.f, -0.1f);
        }
      }
      for (int i = 0; i < nBytes; i++)
        cout << "Byte " << i << " = " << (int)message[i] << ", ";
      if (nBytes > 0)
        cout << "stamp = " << stamp << endl;
    }
  }
  for (int i = 0; i < keys; i++)
  {
    recorder[i].stop();
  }
}