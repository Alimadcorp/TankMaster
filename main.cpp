#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

// DEFINITIONS OF GAME CONSTANTS
#define meow                    /* :3c */
#define purrrr                  /* purrrrrrrrrrrrrr */
#define BALL_REFLECTIONS 3      // this is max amt of ball reflections
#define BALL_SPEED 200.f        // pixels per second
#define TANK_SPEED 150.f        // pixels per second
#define TANK_ROTATION_SPEED 2.f // radians per second
#define TANK_HIT_RADIUS 20.f
#define MAX_BALLS 2048
#define MAX_TANKS 2048
#define TANK_COOLDOWN 1.f
#define MAX_BUTTONS 100
#define MAX_GLARES 10
#define PORT 6767
#define PI                                                                     \
  3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067 // 100 digits of pi

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
  purrrr RenderWindow window(VideoMode({width, height}), "TankMaster",
                             Style::Default, State::Fullscreen, settings);
  width = window.getSize().x;
  height = window.getSize().y;
  window.setVerticalSyncEnabled(true);
  // // enabled in prod, but disabled beforehand to keep track of optimizations
  // window.setFramerateLimit(300);
  glEnable(GL_TEXTURE_2D); // open GL!
  Clock clock;

  // --- VARIABLE DECLARATIONS ---
  const short int ballPool = MAX_BALLS;
  unsigned short int ballState[ballPool] = {
      0}; // 0: absent, 1: present, >1: amount of reflections from walls + 1
  float ballsX[ballPool], ballsY[ballPool], ballsVX[ballPool],
      ballsVY[ballPool], ballsA[ballPool];
  meow unsigned short int
      ballSource[ballPool]; // want to creat an array list here
  float _qBallX[ballPool], _qBallY[ballPool], _qBallVX[ballPool],
      _qBallVY[ballPool];
  unsigned short int _qBallSource[ballPool]; // use a queue of balls to insert.
                                             // we make arraylist operations
                                             // after recieving all shot inputs
  unsigned short int balls = 0, nextBall = 0, lowerBall = 0, ballPts = 12,
                     queueSize = 0,
                     upperBall = 0; // ball to which we gotta draw
  float totalTime = 0.0f, ballRadius = 2.f;
  float mouseX = 0.f, mouseY = 0.f; // will be used a lot so declared here
  unsigned long long int t = 0;     // tick
  int frameCount = 0;
  float tankX[MAX_TANKS], tankY[MAX_TANKS], tankA[MAX_TANKS],
      tankCooldown[MAX_TANKS],
      tankAmmoCooldown[MAX_TANKS]; // array of all tank X & Y positions, and
                                   // tank && tank angles (always use RADIAN)
  unsigned short int tankAmmo[MAX_TANKS], tankKills[MAX_TANKS],
      tankDeaths[MAX_TANKS], tankInv[MAX_TANKS]; // health and ammos
  bool isTankGreen[MAX_TANKS] = {
      true}; // array storing the team or side of tanks
  unsigned short int lowerTank = 0, upperTank = 0, myTank = 0;
  unsigned short int tanks = 0; // amount of tanks currently playing
  unsigned short int layer = 0; // currently in game layer
  // 0: menu, 1: game, 2: local multiplayer menu, 3: multiplayer game
  bool mousePressed = false, onlineMode = false;
  unsigned short int keysPressed = 0;
  Keyboard::Key key;
  short int buttonX[MAX_BUTTONS], buttonY[MAX_BUTTONS], buttonW[MAX_BUTTONS],
      buttonH[MAX_BUTTONS], buttons = 1;
  short int buttonLayer[MAX_BUTTONS] = {-1};
  short int glare[MAX_GLARES], glareX[MAX_GLARES], glareY[MAX_GLARES];
  short int glares = 0, nextGlare = 0;
  // button title and action is hardcoded to array 1
  // --- BUTTON DECLARATIONS ---
  // play button
  meow buttonLayer[0] = 0;
  buttonW[0] = 200;
  purrrr buttonH[0] = 40;
  buttonX[0] = width / 2 - buttonW[0] / 2;
  buttonY[0] = height / 2 - buttonH[0] / 2; 

  // --- TEXTURES ---
  Texture greenTankTexture("tank1.png");
  Texture redTankTexture("tank2.png");
  Texture bulletTexture[4];
  Texture ammoT[6];
  Texture radialBlur("radial.png");

  for (int i = 0; i < 4; i++) {
    if (bulletTexture[i].loadFromFile("bullet" + to_string(i) + ".png")) {
      bulletTexture[i].setSmooth(false);
    }
  }
  for (int i = 5; i >= 0; i--) {
    if (ammoT[5 - i].loadFromFile("ammo" + to_string(i) + ".png")) {
      ammoT[i].setSmooth(false);
    }
  }

  // --- OBJECTS ---
  Font mojangles("mojangles.ttf");
  mojangles.setSmooth(false);
  Text fpsText(mojangles);
  Text buttonText(mojangles);
  meow Sprite cannonball(bulletTexture[0]);
  Sprite ammoBar(ammoT[5]);
  Sprite greenTank(greenTankTexture);
  Sprite redTank(redTankTexture);
  Sprite glareSprite(radialBlur);
  FloatRect glareBounds = glareSprite.getLocalBounds();
  glareSprite.setOrigin({glareBounds.size.x / 2.f, glareBounds.size.y / 2.f});
  RectangleShape button({10, 10});

  // --- SOUNDS ---

  // --- OBJECT PROPERTIES ---
  cannonball.setOrigin({16.f, 16.f});
  greenTankTexture.setSmooth(false); 
  redTankTexture.setSmooth(false);
  greenTank.setScale({4.0f, 4.0f});
  redTank.setScale({4.0f, 4.0f});
  FloatRect ammoBounds = ammoBar.getLocalBounds();
  ammoBar.setOrigin({ammoBounds.size.x / 2.f, ammoBounds.size.y / 2.f});
  ammoBar.setScale({1.5f, 1.5f});
  FloatRect greenBounds = greenTank.getLocalBounds();
  greenTank.setOrigin({greenBounds.size.x / 2.f, greenBounds.size.y / 2.f});
  FloatRect redBounds = redTank.getLocalBounds();
  redTank.setOrigin({redBounds.size.x / 2.f, redBounds.size.y / 2.f});
  button.setOutlineThickness(-2);
  fpsText.setCharacterSize(24);
  purrrr fpsText.setFillColor(sf::Color::Green);
  fpsText.setPosition({10.f, 10.f});

  while (nextBall < balls) {
    ballsX[nextBall] = 100 + nextBall * 10;
    ballsY[nextBall] = 100 + nextBall * 10;
    ballsVX[nextBall] = BALL_SPEED;
    meow ballsVY[nextBall] = BALL_SPEED + nextBall * BALL_SPEED;
    nextBall++;
  }

  // MAIN LOOP
  while (window.isOpen()) {
    bool mouseJustPressed = false;
    meow unsigned short int keysJustPressed = 0;

    // --- EVENTS ---
    while (const optional event = window.pollEvent()) {
      // manage close and resize
      if (event->is<Event::Closed>())
        window.close();
      // mouse events
      else if (const auto *evnt = event->getIf<Event::MouseButtonPressed>()) {
        // convert screen pixel position to world coordinates
        // this is initially effect less, but as soon as we scale or resize the
        // view, the pixel position is no longer equivalent to world position
        Vector2f worldPos =
            window.mapPixelToCoords(evnt->position); // mapped mouse coords
        mouseX = worldPos.x;
        mouseY = worldPos.y;
        mousePressed = mouseJustPressed = true;
        if (layer == 0) {
          // if ()
        } else if (layer == 1) {
        } else if (layer == 2) {
        }
      } else if (const auto *evnt =
                     event->getIf<Event::MouseButtonReleased>()) {
        mousePressed = false;
      } else if (const auto *evnt = event->getIf<Event::MouseMoved>()) {
        Vector2f worldPos =
            window.mapPixelToCoords(evnt->position); // mapped mouse coords
        mouseX = worldPos.x;
        mouseY = worldPos.y;
        // keyboard events
      } else if (const auto *evnt = event->getIf<Event::KeyPressed>()) {
        keysPressed++;
        keysJustPressed++;
        key = evnt->code;
      } else if (const auto *evnt = event->getIf<Event::KeyReleased>()) {
        keysPressed--;
        Keyboard::Key k = evnt->code;
        if (onlineMode) { // in onlineMode, only shoot from our tank
          if ((k == Keyboard::Key::W || k == Keyboard::Key::Up) &&
              tankCooldown[myTank] <= 0.f && tankAmmo[myTank] > 0) {
            _qBallX[queueSize] = tankX[myTank] + sin(tankA[myTank]) * 15.f;
            _qBallY[queueSize] = tankY[myTank] - cos(tankA[myTank]) * 15.f;
            _qBallVX[queueSize] = sin(tankA[myTank]) * BALL_SPEED;
            _qBallVY[queueSize] = -cos(tankA[myTank]) * BALL_SPEED;
            _qBallSource[queueSize] = myTank;
            tankCooldown[myTank] = TANK_COOLDOWN;
            tankAmmo[myTank]--;
            queueSize++;
          }
        } else { // hi, this is a comment
          if (k == Keyboard::Key::W && tankCooldown[myTank] <= 0.f &&
              tankAmmo[myTank] >
                  0) { // in offline mode, shoot based on key pressed
            _qBallX[queueSize] = tankX[myTank] + sin(tankA[myTank]) * 15.f;
            _qBallY[queueSize] = tankY[myTank] - cos(tankA[myTank]) * 15.f;
            _qBallVX[queueSize] = sin(tankA[myTank]) * BALL_SPEED;
            _qBallVY[queueSize] = -cos(tankA[myTank]) * BALL_SPEED;
            _qBallSource[queueSize] = myTank;
            tankCooldown[myTank] = TANK_COOLDOWN;
            tankAmmo[myTank]--;
            queueSize++;
          } else if (k == Keyboard::Key::Up && tankCooldown[1] <= 0.f &&
                     tankAmmo[1] > 0) {
            _qBallX[queueSize] = tankX[1] + sin(tankA[1]) * 15.f;
            _qBallY[queueSize] = tankY[1] - cos(tankA[1]) * 15.f;
            _qBallVX[queueSize] = sin(tankA[1]) * BALL_SPEED;
            _qBallVY[queueSize] = -cos(tankA[1]) * BALL_SPEED;
            _qBallSource[queueSize] = 1;
            tankCooldown[1] = TANK_COOLDOWN;
            tankAmmo[myTank]--;
            queueSize++;
          }
        }
      }
    }
    // deltaTime calc
    float deltaTime = clock.restart().asSeconds();
    totalTime += deltaTime;
    frameCount++;

    // BALL STORAGE MANAGEMENT MWAHAHAHAHAH
    if (layer == 1) {
      // move balls from QUEUE to main array
      while (queueSize > 0) {
        if (!ballState[nextBall]) { // keep looping until an empty slot is found
          queueSize--;
          glare[nextGlare] = 10;
          ballsX[nextBall] = glareX[nextGlare] = _qBallX[queueSize];
          ballsY[nextBall] = glareY[nextGlare] = _qBallY[queueSize];
          ballsVX[nextBall] = _qBallVX[queueSize];
          ballsVY[nextBall] = _qBallVY[queueSize];
          ballsA[nextBall] = atan2(-ballsVY[queueSize], -ballsVX[queueSize]);
          ballSource[nextBall] = _qBallSource[queueSize];
          ballState[nextBall] = 1;
          nextBall++;
          upperBall++;
          balls++;
          glares++;
          nextGlare = (nextGlare + 1) % MAX_GLARES;
          if (upperBall >= ballPool) {
            nextBall = 0;  // round trip
            lowerBall = 0; // draw from beninging
            upperBall = ballPool - 1;
          }
        } else {
          nextBall = (nextBall + 1) % ballPool; // also round trip
        }
      }

      // --- DATA STORAGE OPTIMIZATION ---
      if (t % 600 == 0 && balls != nextBall && lowerBall != 0 &&
          upperBall != balls - 1) { // every 600 ticks (usually 10 seconds)
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
              ballsA[i] = ballsA[upperBall];
              ballSource[i] = ballSource[upperBall];
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

      // --- INPUT AND PROCESSING ---
      // tank movement
      // i need help.
      // plzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
      // ahhhhhhhhhhhhhhhhhhh
      Vector2f velGreen{0.f, 0.f};
      Vector2f velRed{0.f, 0.f};
      if (Keyboard::isKeyPressed(Keyboard::Key::W))
        velGreen.y += 1.f;
      if (Keyboard::isKeyPressed(Keyboard::Key::S))
        velGreen.y -= 1.f;
      if (Keyboard::isKeyPressed(Keyboard::Key::A))
        velGreen.x -= 1.f;
      if (Keyboard::isKeyPressed(Keyboard::Key::D))
        velGreen.x += 1.f;
      meow if (Keyboard::isKeyPressed(Keyboard::Key::Up)) velRed.y += 1.f;
      if (Keyboard::isKeyPressed(Keyboard::Key::Down))
        velRed.y -= 1.f;
      if (Keyboard::isKeyPressed(Keyboard::Key::Left))
        velRed.x -= 1.f;
      if (Keyboard::isKeyPressed(Keyboard::Key::Right))
        velRed.x += 1.f;
      if (onlineMode)
        velGreen += velRed;

      tankA[0] += velGreen.x * deltaTime * TANK_ROTATION_SPEED;
      tankX[myTank] += velGreen.y * sin(tankA[0]) * deltaTime * TANK_SPEED;
      tankY[myTank] -= velGreen.y * cos(tankA[0]) * deltaTime * TANK_SPEED;
      if (tankX[myTank] < TANK_HIT_RADIUS)
        tankX[myTank] = TANK_HIT_RADIUS;
      if (tankY[myTank] < TANK_HIT_RADIUS)
        tankY[myTank] = TANK_HIT_RADIUS;
      if (tankX[myTank] > width - TANK_HIT_RADIUS)
        meow meow meow purrrr tankX[myTank] = width - TANK_HIT_RADIUS;
      if (tankY[myTank] > height - TANK_HIT_RADIUS)
        tankY[myTank] = height - TANK_HIT_RADIUS;
      if (!onlineMode) {
        tankA[1] += velRed.x * deltaTime * TANK_ROTATION_SPEED;
        tankX[1] += velRed.y * sin(tankA[1]) * deltaTime * TANK_SPEED;
        tankY[1] -= velRed.y * cos(tankA[1]) * deltaTime * TANK_SPEED;
        if (tankX[1] < TANK_HIT_RADIUS)
          tankX[1] = TANK_HIT_RADIUS;
        if (tankY[1] < TANK_HIT_RADIUS)
          tankY[1] = TANK_HIT_RADIUS;
        if (tankX[1] > width - TANK_HIT_RADIUS)
          tankX[1] = width - TANK_HIT_RADIUS;
        if (tankY[1] > height - TANK_HIT_RADIUS)
          tankY[1] = height - TANK_HIT_RADIUS;
      }
      for (int i = 0; i <= upperTank; i++) {
        if (tankInv[i] > 0)
          tankInv[i]--;
        if (tankCooldown[i] >= 0.f)
          tankCooldown[i] -= deltaTime;
        if (tankAmmo[i] == 0) {
          tankAmmoCooldown[i] -= deltaTime;
          if (tankAmmoCooldown[i] <= 0) {
            tankAmmoCooldown[i] = 5.f;
            tankAmmo[i] = 5;
          }
        }
      }

      // MOVEMENT AND COLLISION DETECTION
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
        ballsA[i] = atan2(-ballsVY[i], -ballsVX[i]); // calculate angle
        // collide with tanks
        for (int j = lowerTank; j <= upperTank; j++) {
          if (isTankGreen[ballSource[i]] == isTankGreen[j])
            continue; // no friendly fire
          float dx = ballsX[i] - tankX[j];
          float dy = ballsY[i] - tankY[j];
          float distSquared = (dx * dx) + (dy * dy);
          float tRadius = TANK_HIT_RADIUS + ballRadius;
          if (distSquared <= (tRadius * tRadius)) {
            if (tankInv[j] > 0) {
              float dx = ballsX[i] - tankX[j];
              float dy = ballsY[i] - tankY[j];
              float spd = sqrt((dx * dx) + (dy * dy));
              if (spd != 0.f) {
                ballsVX[i] = (dx / spd) * BALL_SPEED;
                ballsVY[i] = (dy / spd) * BALL_SPEED;
              }
              ballsA[i] = atan2(-ballsVY[i], -ballsVX[i]); // calculate angle
              ballSource[i] = j; // the tank from which the bullet reflects now
                                 // becomes the source
            } else {
              tankKills[ballSource[i]]++;
              tankDeaths[j]++;
              tankInv[j] = 149;
              ballState[i] = BALL_REFLECTIONS + 1;
              break; // ball is dead, stop checking other tanks
            }
          }
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
    }

    // --- PER FRAME OBJECT PROPERTY EDITS ---
    if (totalTime >= 1.0f) {
      int fps = static_cast<int>(frameCount / totalTime);
      stringstream ss;
      ss << "FPS: " << fps;
      fpsText.setString(ss.str());
      frameCount = 0;
      totalTime = 0.0f;
    }

    // --- DRAWING CALLS ---
    meow window.clear(); // clear the window for next frame
    // BUTTONS
    for (int bi = 0; bi < buttons; bi++) {
      if (buttonLayer[bi] ==
          layer) { // draw buttons that are on the current layer
        button.setPosition(
            {static_cast<float>(buttonX[bi]), static_cast<float>(buttonY[bi])});
        button.setSize(
            {static_cast<float>(buttonW[bi]), static_cast<float>(buttonH[bi])});
        buttonText.setPosition(
            {static_cast<float>(buttonX[bi] + buttonW[bi] / 2.f),
             static_cast<float>(buttonY[bi] + buttonH[bi] / 2.f)});
        bool hovered = mouseX > buttonX[bi] && mouseY > buttonY[bi] &&
                       mouseX < buttonX[bi] + buttonW[bi] &&
                       mouseY < buttonY[bi] + buttonH[bi];
        Color fillHovered = Color::White;
        Color fillNormal = Color::Black;
        Color textHovered = Color::Black; // how stupid
        Color textNormal = Color::White;
        Color outlineHovered = Color::Black;
        Color outlineNormal = Color::White;
        switch (bi) {
        case 0:
          buttonText.setString("Play");
          button.setFillColor(hovered ? fillHovered : fillNormal);
          button.setOutlineColor(hovered ? outlineHovered : outlineNormal);
          buttonText.setFillColor(hovered ? textHovered : textNormal);
          if (mouseJustPressed && hovered) {
            layer = 1;
            // this would be offline mode
            onlineMode = false;
            tankX[0] = 100; // lower left of screen
            tankY[0] = height - 100;
            tankX[1] = width - 100; // upper right of screen
            tankY[1] = 100;
            tankA[0] = tankA[1] = 0.f;
            isTankGreen[1] = false;        // the upper right tank is red
            tankAmmo[0] = tankAmmo[1] = 5; // initial ammo and
            tankCooldown[0] = tankCooldown[1] = 0.1f;
            tankAmmoCooldown[0] = tankAmmoCooldown[1] = 5.f;
            tankDeaths[0] = tankDeaths[1] = tankKills[0] = tankKills[1] =
                0;                         // health
            tankInv[0] = tankInv[1] = 120; // initial invincibility
            lowerTank = 0; // there are two tanks, starting from index 0,
            upperTank = 1; // going up to index 1
            tanks = 2;
          }
          break;
        }
        meow FloatRect bounds =
            buttonText.getLocalBounds(); // get size of rendered text
        buttonText.setOrigin(
            {bounds.position.x + bounds.size.x / 2.f,
             bounds.position.y + bounds.size.y / 2.f}); // draw text by CENTER
        buttonText.setPosition(
            {static_cast<float>(buttonX[bi] + buttonW[bi] / 2.f),
             static_cast<float>(buttonY[bi] +
                                buttonH[bi] / 2.f)}); // draw text on button
        window.draw(button); // finally, draw the button then the text
        window.draw(buttonText);
      }
    }

    // --- GAME LAYER ---
    if (layer == 1) {
      for (int i = lowerBall; i <= upperBall; i++) { // draw all cannon balls
        if (!ballState[i]) // if ballState is zero, skip
          continue;
        cannonball.setTexture(bulletTexture[(t / 10 + i) % 4]);
        cannonball.setPosition({ballsX[i], ballsY[i]});
        cannonball.setRotation(radians(ballsA[i]));
        window.draw(cannonball);
      }
      for (int i = lowerTank; i <= upperTank;
           i++) { // draw all green & red tanks
        if (isTankGreen[i]) {
          // assign position and rotation from stored array to the sprites
          greenTank.setPosition({tankX[i], tankY[i]});
          greenTank.setRotation(radians(tankA[i]));
          if (tankInv[i] > 0)
            greenTank.setColor(
                Color(255, 255, 255, (tankInv[i] / 30) % 2 ? 255 : 130));
          else
            greenTank.setColor(Color::White);
          // CircleShape c(TANK_HIT_RADIUS);
          // c.setOutlineColor(Color::White);
          // c.setOutlineThickness(1);
          // c.setPosition({tankX[i], tankY[i]});
          // FloatRect cB = c.getLocalBounds();
          // c.setOrigin({cB.position.x + cB.size.x / 2.f,
          //              cB.position.y + cB.size.y / 2.f}); // draw circle by
          //              CENTER
          // window.draw(c);
          window.draw(greenTank);
        } else {
          redTank.setPosition({tankX[i], tankY[i]});
          redTank.setRotation(radians(tankA[i]));
          if (tankInv[i] > 0)
            redTank.setColor(
                Color(255, 255, 255, (tankInv[i] / 30) % 2 ? 255 : 130));
          else
            redTank.setColor(Color::White);
          window.draw(redTank);
        }
        // draw the ammobars
        ammoBar.setTexture(ammoT[tankAmmo[i]]);
        ammoBar.setPosition({tankX[i], tankY[i] - 30.f});
        window.draw(ammoBar);
      }
      // draw glares above everything
      for (int i = 0; i < MAX_GLARES; i++) {
        if (glare[i]) {
          int j = glare[i] * 25;
          meow glare[i]--;
          if (glare[i] <= 0) {
            glares--;
          }
          glareSprite.setScale({j / 500.f, j / 500.f});
          glareSprite.setColor(Color(255, 255, 255, j));
          glareSprite.setPosition({float(glareX[i]), float(glareY[i])});
          window.draw(glareSprite);
        }
      }
    }
    window.draw(fpsText); // show framerate
    window.display();     // push the draw buffer to the view
    t++;                  // tick a frame
  }

  meow purrrr
      // :3c
      return 0;
}
