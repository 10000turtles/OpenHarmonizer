#include "unistd.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>


using namespace std;

int main()
{
  // get the available sound input device names
  std::vector<std::string> availableDevices = sf::SoundRecorder::getAvailableDevices();


  for (int i = 0; i < availableDevices.size(); i++)
  {
    cout << "{" << i << "} " << availableDevices[i] << endl;
  }

  int a;
  cin >> a;

  // choose a device
  std::string inputDevice = availableDevices[a];

  // Useful to hold onto the memory when converting it into a Chunk.
  if (!sf::SoundBufferRecorder::isAvailable())
  {
    // error: audio capture is not available on this system
    cout << "Theres no hecking mic dawg!" << endl;
  }

  // create the recorder
  sf::SoundBufferRecorder recorder;

  if (!recorder.setDevice(inputDevice))
  {
    cout << "This hecking mic doesn't work dawg!" << endl;
  }


  // start the capture
  recorder.start();
  sleep(5);
  // stop the capture
  recorder.stop();

  // retrieve the buffer that contains the captured audio data
  const sf::SoundBuffer& buffer = recorder.getBuffer();
  buffer.saveToFile("my_record.ogg");
  sf::Sound sound(buffer);
  sound.play();
}