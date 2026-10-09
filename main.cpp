#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Network.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

using namespace sf;
using namespace std;

int main() {
  // define main window, rendering, settings & clock
  unsigned int width = 500, height = 500;
  ContextSettings settings;
  settings.depthBits = 24;
  settings.stencilBits = 8;
  settings.antiAliasingLevel = 4;
  settings.majorVersion = 3;
  settings.minorVersion = 0;
  RenderWindow window(VideoMode({width, height}), "TankMaster", Style::Default,
                      State::Windowed, settings);
  // window.setVerticalSyncEnabled(true);
  // // enabled in prod, but disabled beforehand to keep track of optimizations
  // window.setFramerateLimit(300);
  glEnable(GL_TEXTURE_2D); // open GL!
  Clock clock;

  // all variable declarations go here
  const short int ballPool = 2048;
  bool isBall[ballPool];
  float ballsX[ballPool], ballsY[ballPool], ballsVX[ballPool],
      ballsVY[ballPool]; // want to creat an array list here
  short int balls = 0, nextBall = 0, lowerBall = 0, ballPts = 12,
            frameCount = 0;
  float totalTime = 0.0f, ballRadius = 2.f, ballSpeed = 50.f;
  float mouseX = 0.f, mouseY = 0.f; // will be used a lot so declared here

  // all object declarations go here
  CircleShape cannonball(ballRadius, ballPts);
  Font mojangles("mojangles.ttf");
  Text fpsText(mojangles);
  RectangleShape greenTank({0, 0});
  RectangleShape redTank({0, 0});

  // texture imports

  // sound imports

  // all object property edits go here
  cannonball.setFillColor(Color::White);
  cannonball.setOrigin(
      {ballRadius, ballRadius}); // set origin to draw circle around the center
  fpsText.setCharacterSize(24);
  fpsText.setFillColor(sf::Color::Green);
  fpsText.setPosition({10.f, 10.f});

  while (nextBall < balls) {
    ballsX[nextBall] = 100 + nextBall * 10;
    ballsY[nextBall] = 100 + nextBall * 10;
    ballsVX[nextBall] = ballSpeed;
    ballsVY[nextBall] = ballSpeed + nextBall * ballSpeed;
    nextBall++;
  }

  // main loop
  while (window.isOpen()) {
    // events
    while (const optional event = window.pollEvent()) {
      // manage close and resize
      if (event->is<Event::Closed>())
        window.close();
      // mouse events
      else if (const auto *evnt = event->getIf<Event::MouseButtonPressed>()) {
        // convert screen pixel position to world coordinates
        // this is initially effect less, but as soon as we scale or resize the
        // view, the pixel position is no longer equivalent to world position
        Vector2f worldPos = window.mapPixelToCoords(evnt->position);
        mouseX = worldPos.x;
        mouseY = worldPos.y;
      }
    }
    // deltaTime calc
    float deltaTime = clock.restart().asSeconds();
    totalTime += deltaTime;
    frameCount++;

    // processing goes here
    for (int i = 0; i < balls; i++) {
      // velocity operations
      ballsX[i] += ballsVX[i] * deltaTime;
      ballsY[i] += ballsVY[i] * deltaTime;
      // reflect from walls
      if (ballsX[i] + ballRadius > 500.f) {
        ballsX[i] = 500.f - ballRadius;
        ballsVX[i] *= -1.f;
      }
      if (ballsY[i] + ballRadius > 500.f) {
        ballsY[i] = 500.f - ballRadius;
        ballsVY[i] *= -1.f;
      }
      if (ballsX[i] - ballRadius < 0) {
        ballsX[i] = ballRadius;
        ballsVX[i] *= -1.f;
      }
      if (ballsY[i] - ballRadius < 0) {
        ballsY[i] = ballRadius;
        ballsVY[i] *= -1.f;
      }
    }
    if (totalTime >= 1.0f) {
      int fps = static_cast<int>(frameCount / totalTime);
      stringstream ss;
      ss << "FPS: " << fps;
      fpsText.setString(ss.str());
      frameCount = 0;
      totalTime = 0.0f;
    }

    // shape updates go here

    // draw starts here
    window.clear();
    for (int i = 0; i < balls; i++) {
      cannonball.setPosition({ballsX[i], ballsY[i]});
      window.draw(cannonball);
    }
    window.draw(fpsText);
    window.display();
  }

  return 0;
}
