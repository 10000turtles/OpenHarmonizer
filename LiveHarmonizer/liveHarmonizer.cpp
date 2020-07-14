// Compiled with: g++ -Wall -D__LINUX_ALSA__ -o out liveHarmonizer.cpp RtMidi.cpp -lasound -lpthread -lsfml-audio -lsfml-system

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

  int voices = 88;
  int notes  = 88;

  sf::SoundBuffer buffers[notes];

  LiveHarmonizer recorder[voices];


  std::vector<unsigned char> message;
  int                        nBytes, i;
  double                     stamp;
  //                                 C       C#/Db   D       D#/Eb   E       F       F#/Gb   G       G#/Ab   A       A#/Bb   B
  float fourthOctaveFrequencies[] = {261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 415.30, 440.00, 466.16, 493.88};

  //before everything
  float psalm  = fourthOctaveFrequencies[6];
  int   note   = 9;
  int   octave = 0;

  for (int i = 0; i < 0; i++)
  {
    recorder[0].buffer.loadFromFile("psalm.wav");

    const sf::Int16* newS = recorder[0].buffer.getSamples();

    float     temp[recorder[0].buffer.getSampleCount()];
    sf::Int16 bruh[recorder[0].buffer.getSampleCount()];

    for (int j = 0; j < recorder[0].buffer.getSampleCount(); j++)
    {
      temp[j] = (float)newS[j] / 50;
    }

    if (4 - octave > 0)
    {
      for (int q = 0; q < 4 - octave; q++)
      {
        smbPitchShift(0.5, recorder[0].buffer.getSampleCount(), 1024, 32, 44100, temp, temp);
      }
    }

    else if (4 - octave < 0)
    {
      for (int q = 0; q < octave - 4; q++)
      {
        cout << "I am pitch shifting up" << endl;
        smbPitchShift(2, recorder[0].buffer.getSampleCount(), 1024, 32, 44100, temp, temp);
      }
    }

    cout << recorder[0].buffer.getSampleCount() << " " << fourthOctaveFrequencies[note] / psalm << endl;
    smbPitchShift(fourthOctaveFrequencies[note] / psalm, recorder[0].buffer.getSampleCount(), 1024, 32, 44100, temp, temp);

    for (int j = 0; j < recorder[0].buffer.getSampleCount(); j++)
    {
      bruh[j] = temp[j] * 50;
    }

    string filename = "psalm/psalm" + to_string(i) + ".ogg";
    recorder[0].buffer.loadFromSamples(bruh, recorder[0].buffer.getSampleCount(), 2, 44100);
    recorder[0].buffer.saveToFile(filename);

    note++;
    if (note == 12)
    {
      octave++;
      note = 0;
    }
  }


  for (int i = 0; i < voices; i++)
  {
    recorder[i].start();
  }

  for (int i = 0; i < notes; i++)
  {
    buffers[i].loadFromFile("psalm/psalm" + to_string(i) + ".ogg");
    recorder[i].stream.load(buffers[i]);
  }


  int counter = 0;
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

          // counter++;
          // if (counter == voices)
          // {
          //   counter = 0;
          // }
        }
        cout << " ]" << endl;
      }
      // for (i = 0; i < nBytes; i++)
      //   cout << "Byte " << i << " = " << (int)message[i] << ", ";
      // if (nBytes > 0)
      //   cout << "stamp = " << stamp << endl;
    }
  }
  for (int i = 0; i < voices; i++)
  {
    recorder[i].stop();
  }
}