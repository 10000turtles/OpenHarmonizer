#include <TGUI/TGUI.hpp>
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

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "TGUI demo");
    tgui::Gui gui(window);

    if (gui.setGlobalFont("../../fonts/DejaVuSans.ttf") == false)
       return 1;

    // Load all the widgets from a file
    gui.loadWidgetsFromFile("gui.txt");

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            gui.handleEvent(event);
        }

        window.clear();
        gui.draw();
        window.display();

        sf::sleep(sf::milliseconds(1));
    }

    return EXIT_SUCCESS;
}
