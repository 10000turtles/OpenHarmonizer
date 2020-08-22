
#include "MidiCsv/midio.h"
#include "midiToCsv.h"
#include "unistd.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <signal.h>
#include <sstream>  // stringstream
#include <sstream>
#include <stdexcept>  // runtime_error
#include <string>
#include <utility>  // pair
#include <vector>
using namespace std;


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
void write_csv(string filename, vector<vector<string>> data)
{
  fstream fout;
  fout.open(filename, ios::out);

  for (int i = 0; i < data.size(); i++)
  {
    for (int j = 0; j < data[i].size(); j++)
    {
      if (j == data[i].size() - 1)
      {
        fout << data[i][j] << endl;
      }
      else
      {
        fout << data[i][j] << ",";
      }
    }
  }
  fout.close();
}
vector<vector<string>> read_csv(string filename)
{
  vector<vector<string>> result;
  // File pointer
  fstream file;
  file.open(filename);
  string         line;
  vector<string> wop;
  while (getline(file, line, '\n'))
  {
    istringstream templine(line);
    string        data;
    while (getline(templine, data, ','))
    {
      wop.push_back(data);
    }
    result.push_back(wop);
    wop.clear();
  }
  file.close();
  return result;
}

int main()
{
  // Status 9: Press, Status 8: Release
  std::srand(std::time(nullptr));
  map<pNotes, mapWithSize> markovMap;

  string name = "LetsGroove";

  string midiFile   = "MidiFiles/" + name + ".mid";
  string csvFile    = "CsvFiles/" + name + ".csv";
  string newCsvFile = "CsvFiles/" + name + "JazzMachine";

  midiToCsv(midiFile, csvFile);

  vector<vector<string>> midiData;

  midiData = read_csv(csvFile);

  for (int i = 7; i < midiData.size() - 1; i++)
  {
    for (string j : midiData[i])
    {
      cout << j;
    }
    cout << ":   " << i << endl;
  }
  cout << endl;

  int twoAgo   = 0;
  int oneAgo   = 0;
  int newNotes = 0;

  twoAgo = atoi(midiData[5][2].c_str());
  oneAgo = atoi(midiData[6][2].c_str());
  for (int i = 7; i < midiData.size() - 1; i++)
  {
    if (midiData[i][2].compare(" Note_on_c") == 0)
    {
      newNotes++;
      int    temp = atoi(midiData[i][4].c_str());
      pNotes lastTwo(twoAgo, oneAgo);
      markovMap[lastTwo].m[temp]++;
      markovMap[lastTwo].size++;
      twoAgo = oneAgo;
      oneAgo = temp;
    }
  }

  // for (map<pNotes, mapWithSize>::iterator i = markovMap.begin(); i != markovMap.end(); i++)
  // {
  //   pNotes lastTwo(i->first.prevNote, i->first.currentNote);
  //   cout << i->first.prevNote << " " << i->first.currentNote << ": ";
  //   for (map<int, int>::iterator j = i->second.m.begin(); j != i->second.m.end(); j++)
  //   {
  //     cout << "(" << j->first << ")" << j->second << " ";
  //   }
  //   cout << " size: " << i->second.size << endl;
  // }

  vector<int> newNoteList;

  twoAgo = markovMap.begin()->first.prevNote;
  oneAgo = markovMap.begin()->first.currentNote;
  cout << "NewNotes: " << newNotes << endl;
  for (int i = 0; i < newNotes; i++)
  {
    pNotes                             lastTwo(twoAgo, oneAgo);
    map<pNotes, mapWithSize>::iterator q;
    int                                length, newNote, currentNote, randomPNote;
    if (markovMap[lastTwo].size == 0)
    {
      cout << "NO ENTRY" << endl;
      q = markovMap.begin();
      advance(q, rand() % markovMap.size());
      length  = markovMap.at(q->first).size;
      newNote = rand() % length;
      twoAgo  = q->first.prevNote;
      oneAgo  = q->first.currentNote;
    }
    else
    {
      length  = markovMap.at(lastTwo).size;
      newNote = rand() % length;
    }
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

    cout << i << ": " << newNoteList[i] << " " << endl;
  }

  int       counter = 0;
  list<int> currentNotes;

  for (int i = 7; i < midiData.size() - 1; i++)
  {
    if (midiData[i][2].compare(" Note_on_c") == 0)
    {
      midiData[i][4] = to_string(newNoteList[counter]);
      currentNotes.push_back(newNoteList[counter]);
      counter++;
    }
    else if (midiData[i][2].compare(" Note_off_c") == 0)
    {
      if (currentNotes.size() > 0)
      {
        midiData[i][4] = to_string(currentNotes.front());
        currentNotes.pop_front();
      }
    }
  }

  write_csv(csvFile, midiData);
}
