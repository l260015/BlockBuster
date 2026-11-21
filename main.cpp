#include <SFML/Graphics.hpp>
#include<fstream>
#include<iostream>
#include<time.h>
#include<cstdlib>
using namespace std;

int bricks[8][10];
int paddleX = 250;

void movePaddle()
{
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
	{
		paddleX -= 8;
		if (paddleX < 0)
			paddleX = 0;

	}
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
	{
		paddleX += 8;
		if (paddleX > 500)
			paddleX = 500;
	}
}

void drawPaddle(sf::RenderWindow& window)
{
	sf::RectangleShape paddle({ 100.f, 14.f });
	paddle.setFillColor(sf::Color::Cyan);
	paddle.setPosition({ static_cast<float>(paddleX), 650.f });
	window.draw(paddle);
}

void starter() {
	ofstream out("level.txt");
	int rows = 8, cols = 10;
	int temp;
	srand(time(0));

	if (out.is_open()) {
		out << rows << " " << cols << endl;

		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				temp = (rand() % 4);
				out << temp << " ";
			}
			out << endl;
		}


		out.close();
	}
	else {
		cout << "Error opening file.\n";
	}
}

void loadlevel()
{
	int rows, cols;
	ifstream fin("level.txt");
	if (!fin.is_open()) {
		cout << "Error opening file";
		return;
	}

	fin >> rows >> cols;

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			fin >> bricks[i][j];
		}
	}
}

void drawBricks(sf::RenderWindow& window)
{
	const float BRICK_W = 60.f, BRICK_H = 25.f; // one brick is 60 x 25 pixels
	const float BRICKS_Y = 100.f; // the bricks start at y = 100
	sf::Color colors[4] = { sf::Color(25, 25, 45), // 0: no brick
	sf::Color::Green, // 1: one hit left
	sf::Color::Yellow, // 2: two hits left
	sf::Color::Red }; // 3: three hits left

	sf::RectangleShape brick({ BRICK_W - 2, BRICK_H - 2 }); // smaller, so a gap shows
	for (int r = 0; r < 8; ++r)
	{
		for (int c = 0; c < 10; ++c)
		{
			if (bricks[r][c] > 0)
			{
				brick.setFillColor(colors[bricks[r][c]]);
				brick.setPosition({ c * BRICK_W + 1, BRICKS_Y + r * BRICK_H + 1 });
				window.draw(brick);
			}
		}
	}
}
void handleInput(sf::RenderWindow& window) {

	while (const std::optional event = window.pollEvent())
	{
		if (event->is<sf::Event::Closed>())
			window.close();
	}
}

void updateGame()
{
	movePaddle();
}

void drawGame(sf::RenderWindow& window)
{
	window.clear();
	drawBricks(window);
	drawPaddle(window);
	window.display();
}

int main()
{
	sf::RenderWindow window(sf::VideoMode({ 600, 700 }), "Block Buster");
	window.setFramerateLimit(60);
	starter();
	loadlevel();


	while (window.isOpen())
	{
		handleInput(window);
		updateGame();
		drawGame(window);
	}
}
