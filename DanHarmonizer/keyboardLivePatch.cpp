// Compiled with: g++ -Wall -D__LINUX_ALSA__ -o keyboardLivePatch.exe keyboardLivePatch.cpp RtMidi.cpp -lasound -lpthread -lsfml-audio -lsfml-system -lsfml-window

#include "RtMidi.h"
#include "smbPitchShift.cpp"
#include "unistd.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <cstdlib>
#include <fstream>
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

int* load(string filename)
{
  // int  values[88];
  int* values = (int*)malloc(sizeof(int) * 88);

  fstream fin;
  fin.open("configFiles/" + filename + ".csv", ios::in);

  int temp;

  for (int i = 0; i < 88; i++)
  {
    fin >> temp;
    values[i] = temp;
    cout << values[i] << " ";
  }

  return values;
}

int main()
{
  if (!LiveHarmonizer::isAvailable())
  {
    cerr << "It ain't working rn sorry bruv." << endl;
  }

  string filename;
  cout << "Enter the note you want to play (\"note\" if the file is note.wav): " << endl;
  cin >> filename;

  int keys      = 19;
  int keyOffset = 48;

  sf::SoundBuffer buffers[keys];

  LiveHarmonizer recorder[keys];

  std::vector<unsigned char> message;

  int    nBytes;
  double stamp;
  bool   pedal = false;


  map<int, sf::Keyboard::Key> conversion = {
      {(int)'a', sf::Keyboard::A},
      {(int)'b', sf::Keyboard::B},
      {(int)'c', sf::Keyboard::C},
      {(int)'d', sf::Keyboard::D},
      {(int)'e', sf::Keyboard::E},
      {(int)'f', sf::Keyboard::F},
      {(int)'g', sf::Keyboard::G},
      {(int)'h', sf::Keyboard::H},
      {(int)'i', sf::Keyboard::I},
      {(int)'j', sf::Keyboard::J},
      {(int)'k', sf::Keyboard::K},
      {(int)'l', sf::Keyboard::L},
      {(int)'m', sf::Keyboard::M},
      {(int)'n', sf::Keyboard::N},
      {(int)'o', sf::Keyboard::O},
      {(int)'p', sf::Keyboard::P},
      {(int)'q', sf::Keyboard::Q},
      {(int)'r', sf::Keyboard::R},
      {(int)'s', sf::Keyboard::S},
      {(int)'t', sf::Keyboard::T},
      {(int)'u', sf::Keyboard::U},
      {(int)'v', sf::Keyboard::V},
      {(int)'w', sf::Keyboard::W},
      {(int)'x', sf::Keyboard::X},
      {(int)'y', sf::Keyboard::Y},
      {(int)'z', sf::Keyboard::Z},
      {(int)'1', sf::Keyboard::Num1},
      {(int)'2', sf::Keyboard::Num2},
      {(int)'3', sf::Keyboard::Num3},
      {(int)'4', sf::Keyboard::Num4},
      {(int)'5', sf::Keyboard::Num5},
      {(int)'6', sf::Keyboard::Num6},
      {(int)'7', sf::Keyboard::Num7},
      {(int)'8', sf::Keyboard::Num8},
      {(int)'9', sf::Keyboard::Num9},
      {(int)'0', sf::Keyboard::Num0},
      {(int)'[', sf::Keyboard::LBracket},
      {(int)']', sf::Keyboard::RBracket},
      {(int)'-', sf::Keyboard::Dash},
      {(int)'=', sf::Keyboard::Equal},
      {(int)'\\', sf::Keyboard::Slash},
      {(int)';', sf::Keyboard::SemiColon},
      {(int)'\'', sf::Keyboard::Quote},
      {(int)',', sf::Keyboard::Comma},
      {(int)'.', sf::Keyboard::Period},
      {(int)'/', sf::Keyboard::BackSlash},
      {(int)'`', sf::Keyboard::Tilde}};
  cout << "Enter the filename of the config file (no csv extention): ";
  string filename2;
  cin >> filename2;

  int*              values = load(filename2);
  sf::Keyboard::Key keyCodes[keys];

  for (int i = 0; i < keys; i++)
  {
    cout << i + keyOffset << endl;
    cout << values[i + keyOffset] << endl;
    keyCodes[i] = conversion.at(values[i + keyOffset]);
  }

  for (int i = 0; i < keys; i++)
  {
    recorder[i].start();
  }

  for (int i = 0; i < keys; i++)
  {
    if (!buffers[i].loadFromFile("soundFiles/" + filename + "/" + filename + to_string(i + keyOffset) + ".ogg"))
      cerr << "It ain't working rn sorry bruv." << endl;
    recorder[i].stream.load(buffers[i]);
  }

  bool pressedKeys[keys];
  for (int i = 0; i < keys; i++)
  {
    pressedKeys[i] = false;
  }
  //sf::Keyboard::Key keyCodes[] = {sf::Keyboard::A, sf::Keyboard::W, sf::Keyboard::S, sf::Keyboard::E, sf::Keyboard::D, sf::Keyboard::F, sf::Keyboard::T, sf::Keyboard::G, sf::Keyboard::Y, sf::Keyboard::H, sf::Keyboard::U, sf::Keyboard::J, sf::Keyboard::K, sf::Keyboard::O, sf::Keyboard::L, sf::Keyboard::P, sf::Keyboard::SemiColon};

  while (true)
  {
    for (int i = 0; i < keys; i++)
    {
      if (sf::Keyboard::isKeyPressed(keyCodes[i]) && !pressedKeys[i])
      {
        recorder[i].stream.play();
        pressedKeys[i] = true;
      }
      if (!sf::Keyboard::isKeyPressed(keyCodes[i]) && pressedKeys[i])
      {
        recorder[i].stream.stop();
        pressedKeys[i] = false;
      }
    }
  }
  for (int i = 0; i < keys; i++)
  {
    recorder[i].stop();
  }
}