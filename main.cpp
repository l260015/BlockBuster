#include <SFML/Graphics.hpp>
#include<fstream>
#include<iostream>
using namespace std;

int bricks[8][10];

bool loadLevel()
{
	ifstream file("level.txt");
	if (!file)
	{
		cout << "Failed to open level file." << endl;
		return false;
	}

	int rows, cols;
	file >> rows >> cols;

	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 10; ++j)
		{
			file >> bricks[i][j];
		}
	}

	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 10; ++j)
		{
			cout << bricks[i][j] << " ";
		}
		cout << endl;
	}

	file.close();
	return true;
}

void handleInput(sf::RenderWindow& window)
{
    while (const std::optional event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
            window.close();
    }
}

void updateGame()
{
}

void drawGame(sf::RenderWindow& window)
{
    window.clear();
    window.display();
}

int main()
{
    sf::RenderWindow window(sf::VideoMode({ 600, 700 }), "Block Buster");
    window.setFramerateLimit(60);

	int result = loadLevel();
	if (!result)
	{
		return 0;
	}

    while (window.isOpen())
    {
        handleInput(window);
        updateGame();
        drawGame(window);
    }
}