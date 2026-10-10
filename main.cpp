#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Network.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

#define BALL_REFLECTIONS 3 // this is max amt of ball reflections
#define PI 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067 // 100 digits of pi

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
                      State::Fullscreen, settings);
  width = window.getSize().x;
  height = window.getSize().y;
  // window.setVerticalSyncEnabled(true);
  // // enabled in prod, but disabled beforehand to keep track of optimizations
  // window.setFramerateLimit(300);
  glEnable(GL_TEXTURE_2D); // open GL!
  Clock clock;

  // all variable declarations go here
  const short int ballPool = 2048;
  unsigned short int ballState[ballPool] = {
      0}; // 0: absent, 1: present, >1: amount of reflections from walls + 1
  float ballsX[ballPool], ballsY[ballPool], ballsVX[ballPool],
      ballsVY[ballPool]; // want to creat an array list here
  float _qBallX[ballPool], _qBallY[ballPool], _qBallVX[ballPool],
      _qBallVY[ballPool]; // use a queue of balls to insert. we make arraylist
                          // operations after recieving all shot inputs
  bool isBallGreen[2048]; // store whether the ball was shot by a red or green tank
  unsigned short int balls = 0, nextBall = 0, lowerBall = 0, ballPts = 12,
                     queueSize = 0,
                     upperBall = 0; // ball to which we gotta draw
  float totalTime = 0.0f, ballRadius = 2.f, ballSpeed = 50.f;
  float mouseX = 0.f, mouseY = 0.f; // will be used a lot so declared here
  unsigned long long int t = 0;     // tick
  int frameCount = 0;
  float tankX[2048], tankY[2048], tankA[2048]; // array of all tank X & Y positions, and tank angles (always use RADIAN)
  unsigned short int tankAmmo[2048], tankHealth[2048]; // health and ammos
  bool isTankGreen[2048]; // array storing the team or side of tanks
  unsigned short int tanks = 0; // amount of tanks currently playing
  unsigned short int layer = 0; // currently in game layer
  // 0: menu, 1: game, 2: local multiplayer menu, 3: multiplayer game

  // texture imports
  Texture greenTankTexture("tank1.png");
  Texture redTankTexture("tank2.png");
  
  // all object declarations go here
  CircleShape cannonball(ballRadius, ballPts);
  Font mojangles("mojangles.ttf");
  Text fpsText(mojangles);
  Sprite greenTank(greenTankTexture);
  Sprite redTank(redTankTexture);

  // sound imports

  // all object property edits go here
  cannonball.setFillColor(Color::White);
  cannonball.setOrigin(
      {ballRadius, ballRadius}); // set origin to draw circle around the center
  greenTankTexture.setSmooth(false); redTankTexture.setSmooth(false);
  greenTank.setScale({4.0f, 4.0f}); redTank.setScale({4.0f, 4.0f});
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
        mouseX = evnt->position.x;
        mouseY = evnt->position.y;
        // mouseX = worldPos.x;
        // mouseY = worldPos.y;
        _qBallX[queueSize] = mouseX;
        _qBallY[queueSize] = mouseY;
        _qBallVX[queueSize] = mouseX - static_cast<float>(width) / 2;
        _qBallVY[queueSize] = mouseY - static_cast<float>(height) / 2;
        queueSize++;
      }
    }
    // deltaTime calc
    float deltaTime = clock.restart().asSeconds();
    totalTime += deltaTime;
    frameCount++;

    // move balls from queue to main array
    while (queueSize > 0) {
      if (!ballState[nextBall]) { // keep looping until an empty slot is found
        queueSize--;
        ballsX[nextBall] = _qBallX[queueSize];
        ballsY[nextBall] = _qBallY[queueSize];
        ballsVX[nextBall] = _qBallVX[queueSize];
        ballsVY[nextBall] = _qBallVY[queueSize];
        ballState[nextBall] = 1;
        nextBall++;
        upperBall++;
        balls++;
        if (upperBall >= ballPool) {
          nextBall = 0;  // round trip
          lowerBall = 0; // draw from beninging
          upperBall = ballPool - 1;
        }
      } else {
        nextBall = (nextBall + 1) % ballPool; // also round trip
      }
    }

    if (t % 600 == 0 &&
        balls != nextBall) { // every 600 ticks (usually 100 seconds)
      // clear up the ball array to optimize
      for (int i = 0; i <= upperBall; i++) {
        if (!ballState[i]) { // if the current slot is empty
          while (upperBall > i && !ballState[upperBall]) {
            upperBall--; // move upperBall down to surely select the next
                         // non-empty slot
          }
          if (upperBall > i && ballState[upperBall]) {
            // move the ball at the upper limit to the current slot
            ballsX[i] = ballsX[upperBall];
            ballsY[i] = ballsY[upperBall];
            ballsVX[i] = ballsVX[upperBall];
            ballsVY[i] = ballsVY[upperBall];
            ballState[i] = 1;
            ballState[upperBall] = 0;
            upperBall--;
          }
        }
      }
      lowerBall = 0; // the cleanup ensures that the lower ball is always zero
      while (upperBall > 0 && !ballState[upperBall]) {
        upperBall--;
      }
      balls = upperBall;
      nextBall = (upperBall + 1) % ballPool;
    }

    // processing goes here
    for (int i = lowerBall; i <= upperBall; i++) {
      if (!ballState[i])
        continue; // skip processing if theres no ball
      // velocity operations
      ballsX[i] += ballsVX[i] * deltaTime;
      ballsY[i] += ballsVY[i] * deltaTime;
      // reflect from walls
      if (ballsX[i] + ballRadius > width) {
        ballsX[i] = width - ballRadius;
        ballsVX[i] *= -1.f;
        ballState[i]++; // increment reflections
      }
      if (ballsY[i] + ballRadius > height) {
        ballsY[i] = height - ballRadius;
        ballsVY[i] *= -1.f;
        ballState[i]++;
      }
      if (ballsX[i] - ballRadius < 0) {
        ballsX[i] = ballRadius;
        ballsVX[i] *= -1.f;
        ballState[i]++;
      }
      if (ballsY[i] - ballRadius < 0) {
        ballsY[i] = ballRadius;
        ballsVY[i] *= -1.f;
        ballState[i]++;
      }
      if (ballState[i] > BALL_REFLECTIONS) {
        ballState[i] = 0;
        if (upperBall == i) {
          while (upperBall > 0 && !ballState[upperBall]) {
            upperBall--;
          }
        } else if (lowerBall == i) {
          while (lowerBall < ballPool && !ballState[lowerBall]) {
            lowerBall++;
          }
          lowerBall %= ballPool;
        }
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
    for (int i = lowerBall; i <= upperBall; i++) {
      if (!ballState[i])
        continue;
      cannonball.setPosition({ballsX[i], ballsY[i]});
      window.draw(cannonball);
    }
    window.draw(fpsText);
    window.display();
    t++; // tick a frame
  }

  return 0;
}
