// Compiled with: g++ -std=c++17 -o keyConfig.exe keyConfig.cpp -lstdc++fs

#include <cstdlib>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <signal.h>
#include <string>
#include <vector>

using namespace std;

int* load(string filename)
{
  int* values = (int*)malloc(sizeof(int) * 88);

  fstream fin;
  fin.open("configFiles/" + filename + ".csv", ios::in);

  int temp;

  for (int i = 0; i < 88; i++)
  {
    fin >> temp;
    values[i] = temp;
  }

  return values;
}

void save(int* values, string filename)
{
  fstream fout, fin;
  fout.open("configFiles/" + filename + ".csv", ios::out | ios::app);
  cout << "Values: ";
  string temp;

  std::ofstream ofs;
  ofs.open("configFiles/" + filename + ".csv", std::ofstream::out | std::ofstream::trunc);
  ofs.close();

  for (int i = 0; i < 88; i++)
  {
    cout << values[i] << " ";
    fout << values[i] << endl;
  }
}

void edit(int* values, string filename)
{
  map<char, int> notes = {{'A', 0}, {'B', 2}, {'C', 3}, {'D', 5}, {'E', 7}, {'F', 8}, {'G', 10}};
  while (true)
  {
    char   key;
    string note;

    cout << "Enter which note you would like to change (Ex: A4,C#0,Cb4) (\"!!\" to finish): ";
    cin >> note;
    cout << "Enter which key you would like to modify (Computer keyboard key): ";
    cin >> key;

    if (note[0] == '!' && note[1] == '!')
    {
      save(values, filename);
      break;
    }
    if (note.length() == 2)
    {
      int index = notes.at(note[0]) + 12 * ((int)note[1] - 48);
      cout << index << endl;
      values[index] = key;
    }
    if (note.length() == 3)
    {
      int index = notes.at(note[0]) + 12 * ((int)note[2] - 48) + (note[1] == '#' ? 1 : 0) - (note[1] == 'b' ? 1 : 0);
      cout << index << endl;
      values[index] = key;
    }
  }
}

int main()
{
  cout << "Hi. Enter which option you would like to take:" << endl
       << "(1) Edit an existing keyboard config file." << endl
       << "(2) Create a new keyboard config file." << endl
       << "(3) List all keyboard config files." << endl;
  int choice;
  cin >> choice;
  if (choice == 1)
  {
    cout << "Enter the name of the file: " << endl;

    string filename;
    cin >> filename;

    int* values = load(filename);

    edit(values, filename);
  }
  if (choice == 2)
  {
    cout << "Enter the name of the new file: " << endl;

    string filename;
    cin >> filename;

    int values[88];
    for (int i = 0; i < 88; i++)
    {
      values[i] = 0;
    }

    edit(values, filename);
  }
  if (choice == 3)
  {
    std::string path = "configFiles/";
    for (const auto& entry : std::experimental::filesystem::directory_iterator(path))
      std::cout << entry.path() << std::endl;
  }
}