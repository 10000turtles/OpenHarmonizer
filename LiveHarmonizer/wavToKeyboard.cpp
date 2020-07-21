
// Compiled with: g++ wavToKeyboard.cpp -lsfml-audio -o out.exe

#include "smbPitchShift.cpp"
#include "unistd.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <bits/stdc++.h>
#include <cstdlib>
#include <iostream>
#include <signal.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

int main()
{
  // Note names                      C       C#/Db   D       D#/Eb   E       F       F#/Gb   G       G#/Ab   A       A#/Bb   B
  float fourthOctaveFrequencies[] = {261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 415.30, 440.00, 466.16, 493.88};

  int   note           = 9;
  int   octave         = 0;
  int   notes          = 88;
  int   frontSampleCut = 1500;  // The beginning of the transformation is a bit yucky so this cuts off some samples
  float noteFrequency;

  sf::SoundBuffer buffer;
  string          filename;

  cout << "Enter the file name (\"note\" if the file is note.wav): " << endl;
  cin >> filename;

  cout << "Enter the frequency of the wav file: " << endl;
  cin >> noteFrequency;

  for (int i = 0; i < notes; i++)
  {
    buffer.loadFromFile("baseSounds/" + filename + ".wav");
    const sf::Int16* newS = buffer.getSamples();

    float     temp[buffer.getSampleCount()];
    sf::Int16 timp[buffer.getSampleCount() - frontSampleCut];

    for (int j = 0; j < buffer.getSampleCount(); j++)
    {
      temp[j] = (float)newS[j] / 32000;
    }

    if (4 - octave > 0)
    {
      for (int q = 0; q < 4 - octave; q++)
      {
        smbPitchShift(0.5, buffer.getSampleCount(), 1024, 32, 44100, temp, temp);
      }
    }

    else if (4 - octave < 0)
    {
      for (int q = 0; q < octave - 4; q++)
      {
        smbPitchShift(2, buffer.getSampleCount(), 1024, 32, 44100, temp, temp);
      }
    }

    smbPitchShift(fourthOctaveFrequencies[note] / noteFrequency, buffer.getSampleCount(), 1024, 32, 44100, temp, temp);

    for (int j = 0; j < buffer.getSampleCount() - frontSampleCut; j++)
    {
      timp[j] = temp[j + frontSampleCut] * 32000;
    }


    buffer.loadFromSamples(timp, buffer.getSampleCount() - frontSampleCut, 2, 44100);
    buffer.saveToFile("soundFiles/" + filename + "/" + filename + to_string(i) + ".ogg");

    note++;
    if (note == 12)
    {
      octave++;
      note = 0;
    }
  }
}