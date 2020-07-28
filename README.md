# OpenHarmonizer

Hi. And welcome to open harmonizer, a music making tool for controlling and enhancing your midi controller/keyboard. First to give credit where credit is due:

RtMidi.cpp was written mainly by Gary P. Scavone and links to his github and website are here:
https://github.com/thestk/rtmidi
http://www.music.mcgill.ca/~gary/rtmidi/

smbPitchShift.cpp was written by Stephan M. Bernsee and the link to his website is here:
http://blogs.zynaptiq.com/bernsee

Both of these pieces of code are extremely important to the functionality of this program and without them would not be possible. As well, a good chunk of this project relies on SFML, a multimedia library for C++. It is extremely well maintained and deserves all the praise it gets.

# How to run
Linux:
Simply download the github materials and go into linuxRelease. From there, open up a terminal in that folder. From there, there are three commands that can be run:

./midiLivePatch.exe - Allows for your midi device to be used to play samples.
./keyboardLivePatch.exe - Allows for your computer keyboard to be used to play samples.
./wavToKeyboard.exe - Allows conversion of samples to every key on a keyboard.

# Using the software

wavToKeyboard - This file requires that you both have the wav file in the baseSounds directory AND that you know the frequency of the note you are trying to play. This can be done in many ways but the easiest is to figure out what musical note is being played and then use this chart to convert:
https://pages.mtu.edu/~suits/notefreqs.html

Figuring out the note is kinda hard if you don't play music but I am willing to bet that people who don't play music, won't use this software so I don't have to figure it out for you! If it is hard to figure out the exact tone quality, (microtonal would be a good example) this tool can also help for pinpointing the correct frequency:
https://www.szynalski.com/tone-generator/

After you know those two things, enter the name of the file into the terminal ("note" if the file is note.wav) and the frequency of the wav file and 88 new files will be made in the soundFiles folder, one for each key on a piano.


keyboardLivePatch - This file turns your keyboard into a piano. It requires that you have already run wavToKeyboard for any file you wish to play. After running, enter the name of the file you wish to play and badabing badaboom, you're playing your pitch shifted samples. The keyboard is roughly bigger than one octave:

 W E   T Y U   I O

A S D F G H J K L ;

More functionality for this will be added later after more user friendly stuff is made.


midiLivePatch - This file does the same as before except it is binded to your midi controller/keyboard. As well, the damper pedal will not end whatever note you play even if you release the key.
