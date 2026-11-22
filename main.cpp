#include <SFML/Graphics.hpp>
#include<fstream>
#include<iostream>
#include<time.h>
#include<cstdlib>
#include<cmath>
#include<cstdint>
#include<deque>
using namespace std;

int bricks[8][10];
int paddleX = 250;
int ballX, ballY;
int dx = 0, dy = 0;
bool ballLaunched = false;
int score = 0;
int lives = 3;
sf::Font font;
const int MENU = 0, PLAYING = 1, GAME_OVER = 2;
int gameState = MENU;
int menuChoice = 0;
bool playerWon = false;
bool noSaveMessage = false;


// ============================================================================
//  THEME: "ABYSS"  -  a bioluminescent deep-sea world
//  VISUAL HELPERS (front-end only - they never touch game state)
// ============================================================================

const float PI_F = 3.14159265f;

float now()
{
	static sf::Clock visualClock;
	return visualClock.getElapsedTime().asSeconds();
}

std::uint8_t u8(float v)
{
	if (v < 0.f) v = 0.f;
	if (v > 255.f) v = 255.f;
	return static_cast<std::uint8_t>(v);
}

sf::Color withAlpha(sf::Color c, int a)
{
	return sf::Color(c.r, c.g, c.b, u8(static_cast<float>(a)));
}

sf::Color mixColor(sf::Color a, sf::Color b, float k)
{
	if (k < 0.f) k = 0.f;
	if (k > 1.f) k = 1.f;
	return sf::Color(u8(a.r + (b.r - a.r) * k), u8(a.g + (b.g - a.g) * k),
		u8(a.b + (b.b - a.b) * k), u8(a.a + (b.a - a.a) * k));
}

// soft pearly rainbow shimmer
sf::Color iridescent(float p, int a)
{
	return sf::Color(u8(200 + 55 * std::sin(p)), u8(205 + 50 * std::sin(p + 2.1f)),
		u8(225 + 30 * std::sin(p + 4.2f)), u8(static_cast<float>(a)));
}

void drawText(sf::RenderWindow& w, const string& s, unsigned size, float x, float y,
	sf::Color col, bool center = false, float outline = 0.f)
{
	sf::Text t(font, s, size);
	if (center)
	{
		sf::FloatRect b = t.getLocalBounds();
		t.setOrigin({ b.position.x + b.size.x / 2.f, 0.f });
	}
	t.setFillColor(col);
	if (outline > 0.f)
	{
		t.setOutlineColor(sf::Color(0, 15, 40, 230));
		t.setOutlineThickness(outline);
	}
	t.setPosition({ x, y });
	w.draw(t);
}

void drawGlowText(sf::RenderWindow& w, const string& s, unsigned size, float x, float y,
	sf::Color col, sf::Color glow, bool center = true)
{
	for (int i = 3; i >= 1; --i)
	{
		float o = i * 1.7f;
		sf::Color g = withAlpha(glow, 26);
		drawText(w, s, size, x - o, y, g, center);
		drawText(w, s, size, x + o, y, g, center);
		drawText(w, s, size, x, y - o, g, center);
		drawText(w, s, size, x, y + o, g, center);
	}
	drawText(w, s, size, x + 2.f, y + 3.f, sf::Color(0, 15, 40, 170), center);
	drawText(w, s, size, x, y, col, center);
}

// pill shape as ONE polygon (no overlap artifacts with translucent colors)
void drawCapsule(sf::RenderWindow& w, float x, float y, float wd, float h, sf::Color fill,
	sf::Color outline = sf::Color::Transparent, float outlineTh = 0.f)
{
	float r = h / 2.f;
	sf::ConvexShape s(26);
	for (int i = 0; i <= 12; ++i)
	{
		float a = PI_F / 2.f + PI_F * i / 12.f;
		s.setPoint(i, { x + r + r * std::cos(a), y + r - r * std::sin(a) });
	}
	for (int i = 0; i <= 12; ++i)
	{
		float a = -PI_F / 2.f + PI_F * i / 12.f;
		s.setPoint(13 + i, { x + wd - r + r * std::cos(a), y + r - r * std::sin(a) });
	}
	s.setFillColor(fill);
	if (outlineTh > 0.f)
	{
		s.setOutlineColor(outline);
		s.setOutlineThickness(outlineTh);
	}
	w.draw(s);
}

void drawPearl(sf::RenderWindow& w, float cx, float cy, float r, float phase)
{
	sf::CircleShape c;
	for (int k = 3; k >= 1; --k)
	{
		float rr = r * (1.f + 0.6f * k);
		c.setRadius(rr);
		c.setOrigin({ rr, rr });
		c.setPosition({ cx, cy });
		c.setFillColor(iridescent(phase + k, 20));
		w.draw(c);
	}
	c.setRadius(r);
	c.setOrigin({ r, r });
	c.setPosition({ cx, cy });
	c.setFillColor(sf::Color(246, 242, 255));
	c.setOutlineColor(iridescent(phase, 255));
	c.setOutlineThickness(r > 5.f ? 1.5f : 1.f);
	w.draw(c);

	c.setOutlineThickness(0.f);
	float r2 = r * 0.55f;
	c.setRadius(r2);
	c.setOrigin({ r2, r2 });
	c.setPosition({ cx + r * 0.2f, cy + r * 0.25f });
	c.setFillColor(iridescent(phase + 1.5f, 90));
	w.draw(c);

	float r3 = r * 0.3f;
	c.setRadius(r3);
	c.setOrigin({ r3, r3 });
	c.setPosition({ cx - r * 0.35f, cy - r * 0.4f });
	c.setFillColor(sf::Color(255, 255, 255, 245));
	w.draw(c);
}

void drawStarfish(sf::RenderWindow& w, float cx, float cy, float ro, float ri, float rot,
	sf::Color fill, sf::Color outline)
{
	sf::ConvexShape s(10);
	for (int i = 0; i < 10; ++i)
	{
		float a = rot + i * PI_F / 5.f;
		float r = (i % 2 == 0) ? ro : ri;
		s.setPoint(i, { cx + r * std::sin(a), cy - r * std::cos(a) });
	}
	s.setFillColor(fill);
	s.setOutlineColor(outline);
	s.setOutlineThickness(1.f);
	w.draw(s);
}

void drawSparkle(sf::RenderWindow& w, float x, float y, float s, int alpha)
{
	sf::RectangleShape r({ s, 1.6f });
	r.setOrigin({ s / 2.f, 0.8f });
	r.setPosition({ x, y });
	r.setFillColor(sf::Color(255, 255, 255, u8(static_cast<float>(alpha))));
	w.draw(r);
	r.setSize({ 1.6f, s });
	r.setOrigin({ 0.8f, s / 2.f });
	w.draw(r);
}

void drawJelly(sf::RenderWindow& w, float cx, float cy, float r, float phase, sf::Color col)
{
	float t = now();
	float p = std::sin(t * 1.6f + phase);
	float sx = 1.f + 0.10f * p, sy = 1.f - 0.12f * p;

	sf::CircleShape g(r * 1.9f);
	g.setOrigin({ r * 1.9f, r * 1.9f });
	g.setPosition({ cx, cy });
	g.setFillColor(withAlpha(col, 12));
	w.draw(g);

	// tentacles
	for (int k = 0; k < 5; ++k)
	{
		sf::VertexArray ten(sf::PrimitiveType::LineStrip, 9);
		for (int j = 0; j < 9; ++j)
		{
			ten[j].position = { cx - r * 0.7f + k * r * 0.35f + std::sin(t * 2.f + j * 0.7f + k + phase) * (2.f + j * 0.6f),
				cy + j * r * 0.32f };
			ten[j].color = withAlpha(col, 120 - j * 11);
		}
		w.draw(ten);
	}

	// bell
	sf::ConvexShape bell(13);
	for (int i = 0; i <= 12; ++i)
	{
		float a = PI_F + PI_F * i / 12.f;
		bell.setPoint(i, { cx + r * sx * std::cos(a), cy + r * sy * std::sin(a) });
	}
	bell.setFillColor(withAlpha(col, 62));
	bell.setOutlineColor(withAlpha(col, 150));
	bell.setOutlineThickness(1.f);
	w.draw(bell);

	sf::ConvexShape inner(13);
	for (int i = 0; i <= 12; ++i)
	{
		float a = PI_F + PI_F * i / 12.f;
		inner.setPoint(i, { cx + r * 0.6f * sx * std::cos(a), cy + r * 0.6f * sy * std::sin(a) });
	}
	inner.setFillColor(sf::Color(255, 255, 255, 40));
	w.draw(inner);
}

void drawFish(sf::RenderWindow& w, float x, float y, float size, float dir, sf::Color col, float t)
{
	float wig = std::sin(t * 9.f + x * 0.05f) * size * 0.35f;
	sf::ConvexShape tail(3);
	float tx = x - dir * size * 1.2f;
	tail.setPoint(0, { tx, y });
	tail.setPoint(1, { tx - dir * size * 1.0f, y - size * 0.8f + wig });
	tail.setPoint(2, { tx - dir * size * 1.0f, y + size * 0.8f + wig });
	tail.setFillColor(mixColor(col, sf::Color(255, 255, 255, 255), 0.15f));
	w.draw(tail);

	sf::CircleShape body(size);
	body.setOrigin({ size, size });
	body.setScale({ 1.5f, 0.8f });
	body.setPosition({ x, y });
	body.setFillColor(col);
	w.draw(body);

	sf::CircleShape belly(size);
	belly.setOrigin({ size, size });
	belly.setScale({ 1.2f, 0.35f });
	belly.setPosition({ x, y + size * 0.35f });
	belly.setFillColor(sf::Color(255, 255, 255, 70));
	w.draw(belly);

	sf::CircleShape eye(size * 0.22f);
	eye.setOrigin({ size * 0.22f, size * 0.22f });
	eye.setPosition({ x + dir * size * 0.85f, y - size * 0.15f });
	eye.setFillColor(sf::Color(10, 20, 40));
	w.draw(eye);
}

void drawWhale(sf::RenderWindow& w, float x, float y, float dir)
{
	float t = now();
	float sw = 10.f * std::sin(t * 1.8f);

	sf::ConvexShape tail(5);
	float bx = x - dir * 88.f;
	tail.setPoint(0, { bx, y - 9.f });
	tail.setPoint(1, { bx - dir * 34.f, y - 36.f + sw });
	tail.setPoint(2, { bx - dir * 22.f, y + sw * 0.5f });
	tail.setPoint(3, { bx - dir * 34.f, y + 36.f + sw });
	tail.setPoint(4, { bx, y + 9.f });
	tail.setFillColor(sf::Color(12, 45, 100, 150));
	w.draw(tail);

	sf::CircleShape body(40.f);
	body.setOrigin({ 40.f, 40.f });
	body.setScale({ 2.4f, 0.95f });
	body.setPosition({ x, y });
	body.setFillColor(sf::Color(14, 52, 110, 150));
	w.draw(body);

	sf::CircleShape belly(40.f);
	belly.setOrigin({ 40.f, 40.f });
	belly.setScale({ 2.1f, 0.4f });
	belly.setPosition({ x + dir * 6.f, y + 20.f });
	belly.setFillColor(sf::Color(120, 190, 230, 55));
	w.draw(belly);

	sf::ConvexShape fin(3);
	fin.setPoint(0, { x - dir * 8.f, y + 22.f });
	fin.setPoint(1, { x - dir * 30.f, y + 46.f + sw * 0.4f });
	fin.setPoint(2, { x - dir * 34.f, y + 24.f });
	fin.setFillColor(sf::Color(12, 45, 100, 170));
	w.draw(fin);

	sf::CircleShape eye(3.f);
	eye.setOrigin({ 3.f, 3.f });
	eye.setPosition({ x + dir * 62.f, y - 4.f });
	eye.setFillColor(sf::Color(200, 235, 255, 200));
	w.draw(eye);
}

void drawBackground(sf::RenderWindow& w)
{
	float t = now();

	// --- water gradient: sunlit turquoise -> midnight ---
	sf::VertexArray bg(sf::PrimitiveType::TriangleStrip, 6);
	sf::Color cTop(20, 155, 180), cMid(10, 78, 142), cBot(6, 14, 54);
	bg[0].position = { 0.f, 0.f };     bg[0].color = cTop;
	bg[1].position = { 600.f, 0.f };   bg[1].color = cTop;
	bg[2].position = { 0.f, 300.f };   bg[2].color = cMid;
	bg[3].position = { 600.f, 300.f }; bg[3].color = cMid;
	bg[4].position = { 0.f, 700.f };   bg[4].color = cBot;
	bg[5].position = { 600.f, 700.f }; bg[5].color = cBot;
	w.draw(bg);

	// --- god rays from the surface ---
	for (int i = 0; i < 7; ++i)
	{
		float x0 = 30.f + i * 92.f + 30.f * std::sin(t * 0.25f + i * 1.7f);
		float wTop = 24.f + 10.f * std::sin(t * 0.4f + i);
		float lean = -110.f + 25.f * std::sin(t * 0.2f + i);
		float a = 24.f + 16.f * std::sin(t * 0.6f + i * 2.f);
		sf::VertexArray ray(sf::PrimitiveType::TriangleStrip, 4);
		ray[0].position = { x0, 0.f };
		ray[1].position = { x0 + wTop, 0.f };
		ray[2].position = { x0 + lean, 540.f };
		ray[3].position = { x0 + lean + wTop * 2.4f, 540.f };
		ray[0].color = ray[1].color = sf::Color(200, 255, 240, u8(a));
		ray[2].color = ray[3].color = sf::Color(200, 255, 240, 0);
		w.draw(ray);
	}

	// --- drifting jellyfish ---
	drawJelly(w, 95.f + 25.f * std::sin(t * 0.3f), 410.f + 45.f * std::sin(t * 0.5f), 26.f, 0.f, sf::Color(200, 130, 255));
	drawJelly(w, 330.f + 35.f * std::sin(t * 0.22f + 1.f), 500.f + 40.f * std::sin(t * 0.4f + 2.f), 34.f, 2.f, sf::Color(255, 130, 200));
	drawJelly(w, 520.f + 20.f * std::sin(t * 0.35f + 3.f), 380.f + 50.f * std::sin(t * 0.45f + 4.f), 22.f, 4.f, sf::Color(120, 255, 230));

	// --- glittering plankton ---
	sf::RectangleShape mote;
	for (unsigned i = 0; i < 45; ++i)
	{
		unsigned h1 = (i + 3) * 2654435761u;
		unsigned h2 = (i + 3) * 2246822519u + 777u;
		float x = static_cast<float>((h1 >> 8) % 600) + 6.f * std::sin(t * 0.5f + i);
		float y = static_cast<float>((h2 >> 8) % 700) + 6.f * std::cos(t * 0.4f + i);
		float tw = 0.5f + 0.5f * std::sin(t * (1.f + (i % 4) * 0.5f) + i);
		mote.setSize({ 2.f, 2.f });
		mote.setPosition({ x, y });
		mote.setFillColor(sf::Color(210, 255, 250, u8(30 + 120 * tw)));
		w.draw(mote);
	}

	// --- rising bubbles ---
	sf::CircleShape bub;
	for (unsigned i = 0; i < 34; ++i)
	{
		unsigned h = (i + 11) * 2654435761u;
		float speed = 14.f + (h % 28);
		float r = 2.f + (h >> 5) % 5;
		float x = static_cast<float>((h >> 9) % 600) + 8.f * std::sin(t * 1.2f + i);
		float y = 730.f - std::fmod(t * speed + ((h >> 3) % 740), 760.f);
		bub.setRadius(r);
		bub.setOrigin({ r, r });
		bub.setPosition({ x, y });
		bub.setFillColor(sf::Color(255, 255, 255, 10));
		bub.setOutlineColor(sf::Color(220, 255, 255, 110));
		bub.setOutlineThickness(1.f);
		w.draw(bub);
		sf::CircleShape sp(1.f);
		sp.setPosition({ x - r * 0.5f, y - r * 0.6f });
		sp.setFillColor(sf::Color(255, 255, 255, 190));
		w.draw(sp);
	}

	// --- swaying kelp ---
	for (int i = 0; i < 15; ++i)
	{
		float x = 16.f + i * 41.f + (i % 3) * 6.f;
		float hgt = 28.f + (i * 7) % 24;
		sf::VertexArray kelp(sf::PrimitiveType::TriangleStrip, 12);
		for (int j = 0; j < 6; ++j)
		{
			float f = j / 5.f;
			float y = 700.f - hgt * f;
			float xs = x + std::sin(t * 1.3f + i * 0.9f + f * 2.2f) * 6.f * f;
			float wd = 3.2f * (1.f - f) + 0.8f;
			kelp[j * 2].position = { xs - wd, y };
			kelp[j * 2 + 1].position = { xs + wd, y };
			sf::Color kc(20, static_cast<std::uint8_t>(110 + (i % 3) * 30), 95, 175);
			kelp[j * 2].color = kc;
			kelp[j * 2 + 1].color = kc;
		}
		w.draw(kelp);
	}

	// --- sandy seabed ---
	sf::VertexArray bed(sf::PrimitiveType::TriangleStrip, 82);
	for (int i = 0; i < 41; ++i)
	{
		float x = i * 15.f;
		float y = 687.f - 6.f * std::sin(x * 0.03f + 0.8f) - 4.f * std::sin(x * 0.09f);
		bed[i * 2].position = { x, y };
		bed[i * 2].color = sf::Color(6, 18, 44, 240);
		bed[i * 2 + 1].position = { x, 700.f };
		bed[i * 2 + 1].color = sf::Color(3, 8, 26, 255);
	}
	w.draw(bed);
}

// ============================================================================
//  GAME LOGIC (UNTOUCHED)
// ============================================================================

void saveGame()
{
	ofstream fout("save.txt");
	if (fout.is_open())
	{
		fout << score << " " << lives << endl;
		for (int r = 0; r < 8; ++r)
		{
			for (int c = 0; c < 10; ++c)
			{
				fout << bricks[r][c] << " ";
			}
			fout << endl;
		}
		fout.close();
	}
}

bool loadSave()
{
	int rows, cols;
	ifstream fin("save.txt");
	if (!(fin >> score >> lives))
	{
		return false;
	}
	for (int i = 0;i < 8;i++) {
		for (int j = 0;j < 10;j++) {
			fin >> bricks[i][j];
		}
	}
	paddleX = 250;
	dx = 0, dy = 0;
	ballLaunched = false;
	gameState = PLAYING;
	playerWon = false;
	return true;
}

void clearSave()
{
	ofstream file("savegame.txt");
	file.close();
}

// ============================================================================
//  END SCREEN - a brass porthole (visual only)
// ============================================================================

void drawEndScreen(sf::RenderWindow& window)
{
	window.clear();
	drawBackground(window);
	float t = now();

	sf::Color accent = playerWon ? sf::Color(255, 215, 90) : sf::Color(255, 110, 120);

	sf::RectangleShape tint({ 600.f, 700.f });
	tint.setFillColor(playerWon ? sf::Color(255, 200, 80, 22) : sf::Color(2, 6, 24, 150));
	window.draw(tint);

	if (playerWon)
	{
		// raining gold coins
		sf::CircleShape coin(7.f);
		for (unsigned i = 0; i < 30; ++i)
		{
			unsigned h = (i + 5) * 2654435761u;
			float speed = 50.f + (h % 60);
			float x = static_cast<float>((h >> 8) % 600) + 12.f * std::sin(t * 1.5f + i);
			float y = std::fmod(t * speed + (h >> 4) % 700, 760.f) - 30.f;
			float sq = std::fabs(std::cos(t * 3.f + i));
			if (sq < 0.18f) sq = 0.18f;
			coin.setOrigin({ 7.f, 7.f });
			coin.setScale({ sq, 1.f });
			coin.setPosition({ x, y });
			coin.setFillColor(sf::Color(255, 208, 70, 235));
			coin.setOutlineColor(sf::Color(190, 130, 20));
			coin.setOutlineThickness(1.5f);
			window.draw(coin);
		}
	}
	else
	{
		// angler-fish lure dangling from above
		sf::VertexArray stalk(sf::PrimitiveType::LineStrip, 12);
		float lx = 0.f, ly = 0.f;
		for (int j = 0; j < 12; ++j)
		{
			float f = j / 11.f;
			lx = 585.f - 105.f * f + 10.f * std::sin(t * 1.5f + f * 3.f) * f;
			ly = -10.f + 150.f * f + 18.f * std::sin(f * PI_F);
			stalk[j].position = { lx, ly };
			stalk[j].color = sf::Color(60, 90, 120, 200);
		}
		window.draw(stalk);
		float pulse = 0.5f + 0.5f * std::sin(t * 3.f);
		sf::CircleShape orb;
		for (int k = 4; k >= 1; --k)
		{
			float rr = 5.f + k * 6.f + pulse * 3.f;
			orb.setRadius(rr);
			orb.setOrigin({ rr, rr });
			orb.setPosition({ lx, ly });
			orb.setFillColor(sf::Color(190, 255, 130, u8(10.f + 8.f * k * 0.5f)));
			window.draw(orb);
		}
		orb.setRadius(6.f);
		orb.setOrigin({ 6.f, 6.f });
		orb.setPosition({ lx, ly });
		orb.setFillColor(sf::Color(230, 255, 170));
		window.draw(orb);

		// eerie eyes in the dark
		if (std::fmod(t, 4.f) > 0.18f)
		{
			sf::CircleShape eye(7.f);
			for (int e = 0; e < 2; ++e)
			{
				eye.setOrigin({ 7.f, 7.f });
				eye.setScale({ 1.f, 0.7f });
				eye.setPosition({ 80.f + e * 44.f, 600.f });
				eye.setFillColor(sf::Color(255, 225, 90, 230));
				window.draw(eye);
				sf::RectangleShape slit({ 2.f, 9.f });
				slit.setOrigin({ 1.f, 4.5f });
				slit.setPosition({ 80.f + e * 44.f, 600.f });
				slit.setFillColor(sf::Color(30, 10, 0));
				window.draw(slit);
			}
		}
	}

	// ---- porthole ----
	float cx = 300.f, cy = 350.f;
	sf::CircleShape halo;
	for (int k = 4; k >= 1; --k)
	{
		float rr = 200.f + k * 9.f;
		halo.setRadius(rr);
		halo.setOrigin({ rr, rr });
		halo.setPosition({ cx, cy });
		halo.setFillColor(withAlpha(accent, 12));
		window.draw(halo);
	}

	sf::CircleShape ring(200.f);
	ring.setOrigin({ 200.f, 200.f });
	ring.setPosition({ cx, cy });
	ring.setFillColor(sf::Color(198, 146, 62));
	ring.setOutlineColor(sf::Color(105, 66, 18));
	ring.setOutlineThickness(3.f);
	window.draw(ring);

	sf::CircleShape sheen(190.f);
	sheen.setOrigin({ 190.f, 190.f });
	sheen.setPosition({ cx, cy });
	sheen.setFillColor(sf::Color::Transparent);
	sheen.setOutlineColor(sf::Color(255, 230, 160, 110));
	sheen.setOutlineThickness(1.5f);
	window.draw(sheen);

	sf::CircleShape glass(176.f);
	glass.setOrigin({ 176.f, 176.f });
	glass.setPosition({ cx, cy });
	glass.setFillColor(playerWon ? sf::Color(8, 80, 112, 235) : sf::Color(6, 22, 52, 240));
	glass.setOutlineColor(sf::Color(105, 66, 18));
	glass.setOutlineThickness(4.f);
	window.draw(glass);

	// rivets
	sf::CircleShape rivet(4.f);
	rivet.setOrigin({ 4.f, 4.f });
	rivet.setFillColor(sf::Color(236, 196, 110));
	rivet.setOutlineColor(sf::Color(105, 66, 18));
	rivet.setOutlineThickness(1.f);
	for (int i = 0; i < 18; ++i)
	{
		float a = i * 2.f * PI_F / 18.f;
		rivet.setPosition({ cx + 187.f * std::cos(a), cy + 187.f * std::sin(a) });
		window.draw(rivet);
	}

	// glass glare
	sf::CircleShape glare(60.f);
	glare.setOrigin({ 60.f, 60.f });
	glare.setScale({ 1.5f, 0.45f });
	glare.setRotation(sf::degrees(-38.f));
	glare.setPosition({ 215.f, 235.f });
	glare.setFillColor(sf::Color(255, 255, 255, 26));
	window.draw(glare);

	// contents
	string message = playerWon ? "YOU WIN" : "GAME OVER";
	float bob = 3.f * std::sin(t * 2.5f);
	drawGlowText(window, message, 44, 300.f, 262.f + bob, sf::Color::White, accent);

	sf::RectangleShape line({ 170.f, 2.f });
	line.setPosition({ 215.f, 326.f });
	line.setFillColor(withAlpha(accent, 170));
	window.draw(line);
	drawPearl(window, 300.f, 327.f, 5.f, t * 2.f);

	drawText(window, "FINAL SCORE", 16, 300.f, 346.f, sf::Color(170, 225, 240), true);
	drawGlowText(window, to_string(score), 48, 300.f, 368.f, sf::Color(255, 255, 255), sf::Color(110, 240, 255));

	int pulse = static_cast<int>(120 + 135 * (0.5f + 0.5f * std::sin(t * 4.f)));
	drawText(window, "Press ENTER", 20, 300.f, 452.f, sf::Color(255, 255, 255, u8(static_cast<float>(pulse))), true);

	window.display();
}

bool allBricksGone()
{
	for (int r = 0; r < 8; ++r)
	{
		for (int c = 0; c < 10; ++c)
		{
			if (bricks[r][c] > 0)
				return false;
		}
	}
	return true;
}

void checkBallLost()
{
	if (ballY > 700)
	{
		lives -= 1;
		ballLaunched = false;
		dx = 0;
		dy = 0;
	}
}



bool brickAt(int cx, int cy)
{
	if (cx < 0 || cx >= 600 || cy < 100 || cy >= 300)
		return false;
	int row = (cy - 100) / 25;
	int col = cx / 60;
	if (bricks[row][col] > 0)
		return true;
	else
		return false;
}


bool ballTouchesBrick()
{
	if (brickAt(ballX, ballY) || brickAt(ballX + 11, ballY) || brickAt(ballX, ballY + 11) || brickAt(ballX + 11, ballY + 11))
		return true;
	return false;
}

void hitBricks()
{
	int hitRow[4];
	int hitCol[4];
	int hitCount = 0;
	int cornerX[4] = { ballX, ballX + 11, ballX, ballX + 11 };
	int cornerY[4] = { ballY, ballY, ballY + 11, ballY + 11 };


	for (int i = 0;i < 4;++i)
	{
		if (cornerX[i] < 0 || cornerX[i] >= 600 || cornerY[i] < 100 || cornerY[i] >= 300)
			continue;
		int row = (cornerY[i] - 100) / 25;
		int col = cornerX[i] / 60;
		bool alreadyHit = false;
		for (int j = 0;j < hitCount;++j)
		{
			if (hitRow[j] == row && hitCol[j] == col)
				alreadyHit = true;
		}
		if (alreadyHit)
			continue;
		if (bricks[row][col] > 0)
		{
			bricks[row][col] -= 1;
			if (bricks[row][col] == 0)
				score += 50;
			else
				score += 10;
			hitRow[hitCount] = row;
			hitCol[hitCount] = col;
			hitCount++;

		}

	}
}

void moveBall()
{
	if (ballLaunched)
	{
		ballX += dx;
		if (ballX < 0 || ballX>588 || ballTouchesBrick())
		{
			hitBricks();
			ballX -= dx;
			dx = -dx;
		}


		ballY += dy;
		if (ballY < 50 || ballTouchesBrick())
		{
			hitBricks();
			ballY -= dy;
			dy = -dy;
		}
	}

}

void ballBounce()
{


	if (ballY + 12 > 650 && ballY < 664 && ballX + 12 > paddleX && ballX < paddleX + 100 && dy>0)
	{
		int part = (ballX + 6 - paddleX) * 4 / 100;

		if (part < 0) part = 0;      // 2. clamp the low end
		if (part > 3) part = 3;      //    clamp the high end

		int speeds[4] = { -6, -3, 3, 6 };
		dx = speeds[part];              // 3. which index?

		ballY = 638;                   // 4. sit on top of the paddle
		dy = -5;                      // 5. go back up
	}


}

// ============================================================================
//  BALL - a glowing pearl leaving a trail of bubbles (visual only)
// ============================================================================

void drawBall(sf::RenderWindow& window)
{
	static deque<sf::Vector2f> trail;
	if (ballLaunched)
	{
		trail.push_back({ static_cast<float>(ballX) + 6.f, static_cast<float>(ballY) + 6.f });
		if (trail.size() > 10)
			trail.pop_front();
	}
	else
		trail.clear();

	float t = now();

	// bubble trail
	sf::CircleShape c;
	for (size_t i = 0; i < trail.size(); ++i)
	{
		float k = static_cast<float>(i + 1) / static_cast<float>(trail.size() + 1);
		float r = 1.5f + 4.5f * k;
		c.setRadius(r);
		c.setOrigin({ r, r });
		c.setPosition({ trail[i].x + 2.f * std::sin(t * 8.f + i), trail[i].y - (1.f - k) * 6.f });
		c.setFillColor(sf::Color(200, 245, 255, u8(14.f * k)));
		c.setOutlineColor(sf::Color(200, 250, 255, u8(150.f * k)));
		c.setOutlineThickness(1.f);
		window.draw(c);
	}

	drawPearl(window,
		static_cast<float>(ballX) + 6.f,
		static_cast<float>(ballY) + 6.f,
		6.f, t * 2.5f);
}

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

// ============================================================================
//  PADDLE - a little yellow submarine (visual only)
// ============================================================================

void drawPaddle(sf::RenderWindow& window)
{
	float t = now();
	float px = static_cast<float>(paddleX);
	float py = 650.f;

	// exhaust bubbles when the sub is moving (draw-only state)
	struct Puff { float x, y, vy, life, r; };
	static Puff puffs[30];
	static int puffIdx = 0;
	static int lastX = -1;
	if (lastX != -1 && lastX != paddleX)
	{
		bool goingLeft = paddleX < lastX;
		Puff& p = puffs[puffIdx];
		puffIdx = (puffIdx + 1) % 30;
		p.x = goingLeft ? px + 100.f : px;
		p.y = py + 7.f + static_cast<float>((paddleX * 7) % 5) - 2.f;
		p.vy = 0.6f + static_cast<float>((paddleX * 3) % 10) * 0.08f;
		p.life = 1.f;
		p.r = 2.f + static_cast<float>(paddleX % 3);
	}
	lastX = paddleX;
	sf::CircleShape pb;
	for (int i = 0; i < 30; ++i)
	{
		Puff& p = puffs[i];
		if (p.life <= 0.f)
			continue;
		p.y -= p.vy;
		p.x += std::sin(t * 10.f + i) * 0.4f;
		p.life -= 0.035f;
		if (p.life <= 0.f)
			continue;
		pb.setRadius(p.r);
		pb.setOrigin({ p.r, p.r });
		pb.setPosition({ p.x, p.y });
		pb.setFillColor(sf::Color(255, 255, 255, 0));
		pb.setOutlineColor(sf::Color(210, 250, 255, u8(170.f * p.life)));
		pb.setOutlineThickness(1.f);
		window.draw(pb);
	}

	// sonar glow
	float pulse = 0.5f + 0.5f * std::sin(t * 3.f);
	sf::RectangleShape g;
	for (int i = 4; i >= 1; --i)
	{
		g.setSize({ 100.f + i * 6.f, 14.f + i * 5.f });
		g.setPosition({ px - i * 3.f, py - i * 2.5f });
		g.setFillColor(sf::Color(110, 255, 240, u8(8.f + 5.f * pulse)));
		window.draw(g);
	}

	// hull
	drawCapsule(window, px, py, 100.f, 14.f, sf::Color(255, 190, 40),
		sf::Color(150, 85, 10), 1.5f);
	// hull shading
	sf::RectangleShape hi({ 80.f, 3.f });
	hi.setPosition({ px + 10.f, py + 2.f });
	hi.setFillColor(sf::Color(255, 245, 170, 210));
	window.draw(hi);
	sf::RectangleShape lo({ 78.f, 2.f });
	lo.setPosition({ px + 11.f, py + 10.f });
	lo.setFillColor(sf::Color(200, 120, 10, 190));
	window.draw(lo);

	// portholes
	sf::CircleShape ph;
	for (int i = 0; i < 4; ++i)
	{
		float cx = px + 24.f + i * 16.f;
		float cy = py + 6.f;
		ph.setRadius(3.4f);
		ph.setOrigin({ 3.4f, 3.4f });
		ph.setPosition({ cx, cy });
		ph.setFillColor(sf::Color(110, 255, 255, u8(150.f + 100.f * std::sin(t * 4.f + i * 1.3f))));
		ph.setOutlineColor(sf::Color(150, 85, 10));
		ph.setOutlineThickness(1.2f);
		window.draw(ph);
	}

	// spinning propeller
	float spin = std::fabs(std::cos(t * 18.f));
	sf::RectangleShape blade({ 3.f, 11.f });
	blade.setOrigin({ 1.5f, 5.5f });
	blade.setScale({ 1.f, 0.2f + 0.8f * spin });
	blade.setPosition({ px + 94.f, py + 7.f });
	blade.setFillColor(sf::Color(120, 70, 10));
	window.draw(blade);
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

// ============================================================================
//  BRICKS - faceted coral gems (visual only)
// ============================================================================

void setOct(sf::ConvexShape& s, float x, float y, float wd, float h, float c)
{
	s.setPointCount(8);
	s.setPoint(0, { x + c, y });
	s.setPoint(1, { x + wd - c, y });
	s.setPoint(2, { x + wd, y + c });
	s.setPoint(3, { x + wd, y + h - c });
	s.setPoint(4, { x + wd - c, y + h });
	s.setPoint(5, { x + c, y + h });
	s.setPoint(6, { x, y + h - c });
	s.setPoint(7, { x, y + c });
}

void drawBricks(sf::RenderWindow& window)
{
	const float BRICK_W = 60.f, BRICK_H = 25.f; // one brick is 60 x 25 pixels
	const float BRICKS_Y = 100.f; // the bricks start at y = 100

	// index = hits left: 1 sea-glass, 2 golden coral, 3 magenta coral
	sf::Color mainC[4] = { sf::Color(0, 0, 0, 0), sf::Color(40, 205, 205), sf::Color(255, 170, 50), sf::Color(240, 60, 150) };
	sf::Color lightC[4] = { sf::Color(0, 0, 0, 0), sf::Color(175, 255, 245), sf::Color(255, 232, 155), sf::Color(255, 165, 220) };
	sf::Color darkC[4] = { sf::Color(0, 0, 0, 0), sf::Color(8, 105, 130),   sf::Color(165, 88, 8),    sf::Color(115, 10, 70) };

	float t = now();
	sf::RectangleShape glow;
	sf::ConvexShape oct, facet(4);
	sf::CircleShape dot(2.5f);
	dot.setOrigin({ 2.5f, 2.5f });

	for (int r = 0; r < 8; ++r)
	{
		for (int c = 0; c < 10; ++c)
		{
			if (bricks[r][c] > 0)
			{
				int lv = bricks[r][c] > 3 ? 3 : bricks[r][c];
				float x = c * BRICK_W + 1;
				float y = BRICKS_Y + r * BRICK_H + 1;
				float w = BRICK_W - 2, h = BRICK_H - 2;

				// underwater glow
				glow.setSize({ w + 8.f, h + 8.f });
				glow.setPosition({ x - 4.f, y - 4.f });
				glow.setFillColor(withAlpha(mainC[lv], 24));
				window.draw(glow);

				// dark rim
				oct.setOutlineThickness(0.f);
				setOct(oct, x, y, w, h, 6.f);
				oct.setFillColor(darkC[lv]);
				window.draw(oct);

				// body
				setOct(oct, x + 2.f, y + 2.f, w - 4.f, h - 4.f, 4.5f);
				oct.setFillColor(mainC[lv]);
				window.draw(oct);

				// upper facet
				facet.setPoint(0, { x + 5.f, y + 3.f });
				facet.setPoint(1, { x + w - 5.f, y + 3.f });
				facet.setPoint(2, { x + w - 10.f, y + h * 0.46f });
				facet.setPoint(3, { x + 10.f, y + h * 0.46f });
				facet.setFillColor(withAlpha(lightC[lv], 140));
				window.draw(facet);

				// sturdier coral gets inner ring (and a pearl for the toughest)
				if (lv >= 2)
				{
					setOct(oct, x + 8.f, y + 7.f, w - 16.f, h - 14.f, 3.f);
					oct.setFillColor(sf::Color::Transparent);
					oct.setOutlineColor(withAlpha(lightC[lv], 190));
					oct.setOutlineThickness(1.f);
					window.draw(oct);
					oct.setOutlineThickness(0.f);
				}
				if (lv == 3)
				{
					dot.setPosition({ x + w / 2.f, y + h / 2.f });
					dot.setFillColor(sf::Color(255, 255, 255, 230));
					window.draw(dot);
				}

				// twinkle
				float s = std::sin(t * 2.f + r * 1.3f + c * 2.1f);
				if (s > 0.9f)
					drawSparkle(window, x + 12.f + (c % 3) * 12.f, y + 6.f, 5.f + (s - 0.9f) * 50.f, 230);
			}
		}
	}
}

void startNewGame()
{
	paddleX = 250;
	dx = 0, dy = 0;
	ballLaunched = false;
	score = 0;
	lives = 3;
	gameState = PLAYING;
	playerWon = false;
	loadlevel();

}



void handleInput(sf::RenderWindow& window)
{
	while (const std::optional event = window.pollEvent())
	{
		if (event->is<sf::Event::Closed>())
			window.close();
		else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
		{
			switch (gameState)
			{
			case MENU:
				if (key->code == sf::Keyboard::Key::Up)
				{
					if (menuChoice > 0)
						menuChoice--;
				}
				else if (key->code == sf::Keyboard::Key::Down)
				{
					if (menuChoice < 2)
						menuChoice++;
				}
				else if (key->code == sf::Keyboard::Key::Enter)
				{
					if (menuChoice == 0)
					{
						startNewGame();
						gameState = PLAYING;
					}
					else if (menuChoice == 1)
					{
						if (loadSave())
						{
							noSaveMessage = false;
							gameState = PLAYING;
						}
						else
							noSaveMessage = true;
					}
					else if (menuChoice == 2)
						window.close();
				}
				break;

			case PLAYING:
				if (key->code == sf::Keyboard::Key::Space && !ballLaunched)
				{
					ballLaunched = true;
					dy = -5;
					if (rand() % 2 == 0)
						dx = -3;
					else
						dx = 3;
				}
				else if (key->code == sf::Keyboard::Key::Escape)
				{
					saveGame();
					gameState = MENU;
				}
				break;

			case GAME_OVER:
				if (key->code == sf::Keyboard::Key::Enter)
					gameState = MENU;
				break;
			}
		}
	}
}



void updateGame()
{
	switch (gameState)
	{
	case MENU:
		// menu stuff
		break;
	case PLAYING:
		movePaddle();
		if (!ballLaunched)
		{
			ballX = paddleX + 44;
			ballY = 638;
		}
		else
		{
			moveBall();
			checkBallLost();
			ballBounce();
		}
		if (lives == 0)
		{
			playerWon = false;
			clearSave();
			gameState = GAME_OVER;
		}
		if (allBricksGone())
		{
			playerWon = true;
			clearSave();
			gameState = GAME_OVER;
		}
		break;
	case GAME_OVER:

		break;
	}


}

// ============================================================================
//  HUD - glass bar with a rippling waterline (visual only)
// ============================================================================

void drawHUD(sf::RenderWindow& window)
{
	float t = now();

	sf::VertexArray strip(sf::PrimitiveType::TriangleStrip, 4);
	sf::Color top(10, 70, 115, 245), bot(4, 28, 68, 245);
	strip[0].position = { 0.f, 0.f };    strip[0].color = top;
	strip[1].position = { 600.f, 0.f };  strip[1].color = top;
	strip[2].position = { 0.f, 50.f };   strip[2].color = bot;
	strip[3].position = { 600.f, 50.f }; strip[3].color = bot;
	window.draw(strip);

	// rippling waterline
	for (int k = -1; k <= 1; ++k)
	{
		sf::VertexArray wave(sf::PrimitiveType::LineStrip, 76);
		for (int i = 0; i < 76; ++i)
		{
			float x = i * 8.f;
			wave[i].position = { x, 47.f + k + 2.f * std::sin(x * 0.07f + t * 3.f) };
			wave[i].color = sf::Color(130, 245, 255, k == 0 ? 235 : 80);
		}
		window.draw(wave);
	}

	// score
	drawPearl(window, 24.f, 26.f, 7.f, t * 2.f);
	drawText(window, "SCORE", 12, 42.f, 5.f, sf::Color(160, 225, 240));
	drawText(window, to_string(score), 26, 42.f, 16.f, sf::Color::White, false, 1.f);

	// lives = starfish
	drawText(window, "LIVES", 12, 585.f - 3 * 34.f + 8.f, 5.f, sf::Color(160, 225, 240));
	for (int i = 0; i < 3; ++i)
	{
		float cx = 585.f - (2 - i) * 34.f - 12.f;
		float cy = 29.f;
		float rot = std::sin(t * 1.5f + i) * 0.25f;
		if (i < lives)
		{
			float beat = (lives == 1) ? 1.f + 0.14f * std::sin(t * 8.f) : 1.f;
			drawStarfish(window, cx, cy, 11.f * beat, 4.8f * beat, rot, sf::Color(255, 140, 60), sf::Color(190, 70, 20));
			sf::CircleShape core(2.2f);
			core.setOrigin({ 2.2f, 2.2f });
			core.setPosition({ cx, cy });
			core.setFillColor(sf::Color(255, 220, 160));
			window.draw(core);
		}
		else
			drawStarfish(window, cx, cy, 11.f, 4.8f, rot, sf::Color(20, 55, 95), sf::Color(70, 120, 160));
	}
}

// ============================================================================
//  MENU (visual only)
// ============================================================================

void drawMenu(sf::RenderWindow& window)
{
	window.clear();
	drawBackground(window);
	float t = now();

	// fish schools
	for (int i = 0; i < 8; ++i)
	{
		float fx = std::fmod(t * 38.f + i * 34.f, 720.f) - 60.f;
		float fy = 262.f + 14.f * std::sin(t * 1.2f + i * 0.8f) + (i % 3) * 10.f;
		drawFish(window, fx, fy, 5.f + (i % 2), 1.f, mixColor(sf::Color(255, 150, 60), sf::Color(255, 210, 80), (i % 4) / 3.f), t);
	}
	for (int i = 0; i < 7; ++i)
	{
		float fx = 660.f - std::fmod(t * 30.f + i * 40.f, 760.f);
		float fy = 588.f + 12.f * std::sin(t * 1.1f + i) + (i % 2) * 12.f;
		drawFish(window, fx, fy, 5.f, -1.f, mixColor(sf::Color(80, 220, 255), sf::Color(180, 140, 255), (i % 4) / 3.f), t);
	}

	// majestic whale
	drawWhale(window, std::fmod(t * 16.f, 980.f) - 190.f, 635.f, 1.f);

	// wavy rainbow-pearl title, letter by letter
	const string title = "BLOCK BUSTER";
	float adv[16];
	float totalW = 0.f;
	for (size_t i = 0; i < title.size(); ++i)
	{
		adv[i] = totalW;
		totalW += font.getGlyph(static_cast<char32_t>(title[i]), 56u, false).advance;
		if (i + 1 < title.size())
			totalW += font.getKerning(static_cast<char32_t>(title[i]), static_cast<char32_t>(title[i + 1]), 56u, false);
	}
	float startX = 300.f - totalW / 2.f;
	for (size_t i = 0; i < title.size(); ++i)
	{
		if (title[i] == ' ')
			continue;
		float yy = 118.f + 7.f * std::sin(t * 2.2f + i * 0.55f);
		float k = 0.5f + 0.5f * std::sin(t * 1.4f + i * 0.45f);
		sf::Color col = mixColor(sf::Color(110, 245, 255), sf::Color(255, 150, 215), k);
		string s(1, title[i]);
		float lx = startX + adv[i];
		drawText(window, s, 56, lx - 2.f, yy, withAlpha(col, 38));
		drawText(window, s, 56, lx + 2.f, yy, withAlpha(col, 38));
		drawText(window, s, 56, lx, yy - 2.f, withAlpha(col, 38));
		drawText(window, s, 56, lx, yy + 2.f, withAlpha(col, 38));
		drawText(window, s, 56, lx + 2.f, yy + 4.f, sf::Color(0, 15, 40, 170));
		drawText(window, s, 56, lx, yy, col, false, 2.f);
	}
	drawText(window, "~  dive into the deep  ~", 16, 300.f, 200.f, sf::Color(190, 240, 250), true);

	// options as glass pills with pearl bullets
	string options[3] = { "New Game", "Continue", "Exit" };
	for (int i = 0; i < 3; ++i)
	{
		float y = 300.f + i * 60.f;
		bool sel = (i == menuChoice);
		if (sel)
		{
			float pulse = 0.5f + 0.5f * std::sin(t * 5.f);
			for (int k = 3; k >= 1; --k)
				drawCapsule(window, 160.f - k * 4.f, y - 3.f - k * 4.f, 280.f + k * 8.f, 46.f + k * 8.f,
					sf::Color(120, 255, 240, u8(8.f + 7.f * pulse)));
			drawCapsule(window, 160.f, y - 3.f, 280.f, 46.f, sf::Color(70, 230, 225, 80),
				sf::Color(140, 255, 245, 235), 2.f);

			float bob = 3.f * std::sin(t * 4.f);
			drawPearl(window, 188.f, y + 20.f + bob, 7.f, t * 2.f);
			drawPearl(window, 412.f, y + 20.f - bob, 7.f, t * 2.f + 2.f);
		}
		else
		{
			drawCapsule(window, 160.f, y - 3.f, 280.f, 46.f, sf::Color(8, 40, 90, 130),
				sf::Color(100, 190, 220, 110), 1.f);
		}
		drawText(window, options[i], 30, 300.f, y, sel ? sf::Color(255, 255, 255) : sf::Color(180, 225, 240), true, sel ? 1.f : 0.f);
	}

	if (noSaveMessage)
	{
		int a = static_cast<int>(150 + 105 * (0.5f + 0.5f * std::sin(t * 6.f)));
		drawText(window, "No saved game", 24, 300.f, 500.f, sf::Color(255, 110, 120, u8(static_cast<float>(a))), true, 1.f);
	}

	drawText(window, "UP / DOWN  select      ENTER  confirm", 14, 300.f, 548.f, sf::Color(150, 205, 225), true);

	window.display();
}

void drawGame(sf::RenderWindow& window)
{

	switch (gameState)
	{
	case MENU:
		drawMenu(window);
		break;
	case PLAYING:
		window.clear();
		drawBackground(window);
		drawBricks(window);
		drawPaddle(window);
		drawBall(window);
		drawHUD(window);

		if (!ballLaunched)
		{
			int a = static_cast<int>(120 + 120 * (0.5f + 0.5f * std::sin(now() * 4.f)));
			drawText(window, "PRESS SPACE TO LAUNCH", 20, 300.f, 520.f, sf::Color(255, 255, 255, u8(static_cast<float>(a))), true, 1.f);
			drawText(window, "A / D or Arrows to move   -   ESC to save & quit to menu", 12, 300.f, 676.f, sf::Color(160, 215, 235), true);
		}
		window.display();
		break;
	case GAME_OVER:
		drawEndScreen(window);
		break;
	}


}

int main()
{

	sf::RenderWindow window(sf::VideoMode({ 600, 700 }), "Block Buster");
	window.setFramerateLimit(60);
	starter();
	loadlevel();

	if (!font.openFromFile("arial.ttf"))
		return 0;

	while (window.isOpen())
	{
		handleInput(window);
		updateGame();
		drawGame(window);
	}
}
