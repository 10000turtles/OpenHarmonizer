// Compiled with: g++ -Wall -D__LINUX_ALSA__ -o out liveHarmonizer.cpp RtMidi.cpp -lasound -lpthread -lsfml-audio -lsfml-system

#include "RtMidi.h"
#include "unistd.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <cstdlib>
#include <iostream>
#include <signal.h>

using namespace std;


class LiveHarmonizer : public sf::SoundRecorder
{
 public:
  RtMidiIn*       midiin;
  sf::SoundBuffer buffer;
  sf::Sound       sound;
  vector<int>     notes;

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
    sf::Int16 newS[sc];
    for (int i = 0; i < sc; i++)
    {
      newS[i] = s[i];
    }
    buffer.loadFromSamples(newS, sc, 1, 20000);

    sound.setBuffer(buffer);
    sound.play();
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

  LiveHarmonizer recorder;

  recorder.start();

  std::vector<unsigned char> message;
  int                        nBytes, i;
  double                     stamp;

  while (true)
  {
    stamp  = recorder.midiin->getMessage(&message);
    nBytes = message.size();
    if (nBytes > 0)
    {
      if ((int)message[0] == 144)
      {
        recorder.notes[message[1] - 21] = message[2];
        cout << "[ ";
        for (int k = 0; k < 88; k++)
        {
          if (recorder.notes[k] > 0)
          {
            cout << k << ": " << recorder.notes[k] << ", ";
          }
        }
        cout << " ]" << endl;
      }
    }
    // for (i = 0; i < nBytes; i++)
    //   cout << "Byte " << i << " = " << (int)message[i] << ", ";
    // if (nBytes > 0)
    //   cout << "stamp = " << stamp << endl;
  }
  recorder.stop();
}