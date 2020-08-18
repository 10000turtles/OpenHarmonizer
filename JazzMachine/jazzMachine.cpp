#include "MIDIFile-master/MIDIFile.h"
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
#include <vector>


class pNotes
{
 public:
  // Lowest key is 0, highest 87
  int prevNote;
  int currentNote;

  pNotes(int p, int c)
  {
    prevNote    = p;
    currentNote = c;
  }
  bool operator<(const pNotes& rhs) const
  {
    if (prevNote > rhs.prevNote)
    {
      return true;
    }
    else if (prevNote < rhs.prevNote)
    {
      return false;
    }
    return currentNote > rhs.currentNote;
  }
};

class mapWithSize
{
 public:
  map<int, int> m;
  int           size;

  mapWithSize()
  {
    m    = map<int, int>();
    size = 0;
  }
};

int main()
{
  /* Useable midi file stuff
  ifstream midiFile;
  midiFile.open("path/to/midi/file.mid");
  MIDIFile* myMIDIFile = new MIDIFile(&midiFile);

  cout << "Header chunk length: " << myMIDIFile->getHeaderChunkLength() << endl;
  cout << "File format type: " << myMIDIFile->getFormat() << endl;
  cout << "Number of tracks: " << myMIDIFile->getNumberOfTracks() << endl;
  cout << "Ticks per quarter note: " << myMIDIFile->getTicksPerQuarterNote() << endl;

  Track* myTrack = myMIDIFile->getTrack(0);

  for (int i = 0; i < myTrack->length; i++)
  {
    std::vector<MIDIEvent*> eventList = myTrack->MIDIEvents;
    MIDIEvent*              event     = eventList[i];
    cout << "deltaT: " << event->getDeltaT() << endl;
    cout << "status: " << event->getStatus() << endl;
    cout << "channel: " << event->getChannel() << endl;
    cout << "pitch: " << event->getParam1() << endl;
    cout << "velocity: " << event->getParam2() << endl;
  }
  */

  // Status 9: Press, Status 8: Release

  map<pNotes, mapWithSize> markovMap;

  ifstream midiFile;

  midiFile.open("MidiFiles/TTLS.mid");

  //There is a problem here with longer midi files
  //AHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
  MIDIFile* myMIDIFile = new MIDIFile(&midiFile);
  cout << "I do not get here" << endl;
  Track* myTrack = myMIDIFile->getTrack(0);

  int twoAgo = 0;
  int oneAgo = 0;

  std::vector<MIDIEvent*> eventList = myTrack->MIDIEvents;

  MIDIEvent* event1 = eventList[0];
  MIDIEvent* event2 = eventList[2];

  twoAgo = event1->getParam1();
  oneAgo = event2->getParam1();

  for (int i = 4; i < myTrack->length; i++)
  {
    MIDIEvent* event = eventList[i];

    if (event->getStatus() == 9)
    {
      pNotes lastTwo(twoAgo, oneAgo);
      markovMap[lastTwo].m[event->getParam1()]++;
      markovMap[lastTwo].size++;
      twoAgo = oneAgo;
      oneAgo = event->getParam1();
    }
  }


  for (map<pNotes, mapWithSize>::iterator i = markovMap.begin(); i != markovMap.end(); i++)
  {
    pNotes lastTwo(i->first.prevNote, i->first.currentNote);
    cout << i->first.prevNote << " " << i->first.currentNote << ": ";
    for (map<int, int>::iterator j = i->second.m.begin(); j != i->second.m.end(); j++)
    {
      cout << "(" << j->first << ")" << j->second << " ";
    }
    cout << " size: " << i->second.size << endl;
  }

  int         newNotes = 16;
  vector<int> newNoteList;

  twoAgo = markovMap.begin()->first.prevNote;
  oneAgo = markovMap.begin()->first.currentNote;

  for (int i = 0; i < newNotes; i++)
  {
    pNotes lastTwo(twoAgo, oneAgo);
    int    length      = markovMap.at(lastTwo).size;
    int    newNote     = rand() % length;
    int    currentNote = 0;
    for (map<int, int>::iterator j = markovMap.at(lastTwo).m.begin(); j != markovMap.at(lastTwo).m.end(); j++)
    {
      if (newNote >= j->second)
      {
        newNote -= j->second;
        continue;
      }
      newNoteList.push_back(j->first);
      twoAgo = oneAgo;
      oneAgo = j->first;
      break;
    }
    cout << newNoteList[i] << " " << endl;
  }
}
