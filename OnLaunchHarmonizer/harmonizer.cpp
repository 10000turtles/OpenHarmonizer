#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

int main()
{
	sf::RenderWindow window(sf::VideoMode(sf::VideoMode::getDesktopMode()), "SFML");
	sf::SoundBuffer buffer;
    if (!buffer.loadFromFile("mainMusic.wav"))
        return -1;
	sf::Sound sound;
	sound.setBuffer(buffer);
	sound.play();
	while(window.isOpen())
	{
		sf::Event event;
		while(window.pollEvent(event))
		{
			if(event.type == sf::Event::Closed)
				window.close();
		}
	}
}
