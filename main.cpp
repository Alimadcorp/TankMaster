#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#if defined(_WIN32) || defined(_WIN64)
#include <cmath>
#endif
#ifdef __APPLE__
#include <sstream>
#endif

#pragma region CONSTANTS
// DEFINITIONS OF GAME CONSTANTS
#define meow                    /* :3c */
#define purrrr                  /* purrrrrrrrrrrrrr */
#define BALL_REFLECTIONS 4      // this is max amt of ball reflections
#define BALL_SPEED 300.f        // pixels per second
#define TANK_SPEED 200.f        // pixels per second
#define TANK_ROTATION_SPEED 2.f // radians per second
#define TANK_HIT_RADIUS 20.f
#define ROCK_HIT_RADIUS 16.f
#define MAX_BALLS 2048
#define MAX_TANKS 2048
#define TANK_COOLDOWN 0.5f
#define MAX_BUTTONS 100
#define MAX_GLARES 10
#define CLOCK_TIME 120 // seconds
#define PORT 6767
#define PI                                                                     \
  3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067 // 100 digits of pi
#define TANK_ACCEL 450.f    // pixels per second squared
#define TANK_ROT_ACCEL 10.f // radians per second squared
#define MAX_TRACKS 600
#define MAX_SCORCH 32
#define TRACK_SPACING 14.f
#define TRACK_LIFE 8.f
#define SCORCH_LIFE 20.f
#define MAX_POWERUPS 4
#define POWERUP_INTERVAL 8.f
#define POWERUP_TIME 8.f
#define BOOST_MULT 1.6f
#define SHOT_SPREAD 0.18f
#define MAX_PEERS 6
#define MAX_PLAYERS 8
#define MAX_NET_BALLS 150
#define NET_INTERVAL (1.f / 60.f)
#define NET_TIMEOUT 6.f
#define U8(x) static_cast<uint8_t>(x)
#define U16(x) static_cast<uint16_t>(x)
#define I16(x) static_cast<int16_t>(x)
#pragma endregion

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
  purrrr RenderWindow window(VideoMode::getDesktopMode(), "TankMaster",
                             Style::Default, State::Fullscreen, settings);
  width = window.getSize().x;
  height = window.getSize().y;
  // the size of our own screen. width & height can change to the host's size
  // in online matches, so we remember the real one here
  unsigned int baseW = width, baseH = height;
  window.setVerticalSyncEnabled(true);
  // // enabled in prod, but disabled beforehand to keep track of optimizations
  // window.setFramerateLimit(300);
  glEnable(GL_TEXTURE_2D); // open GL!
  Clock clock;
  srand(static_cast<unsigned int>(clock.getElapsedTime().asMicroseconds()) +
        1234u); // seed for power up spots

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
  float totalTime = 0.0f, ballRadius = 3.f;
  float mouseX = 0.f, mouseY = 0.f; // will be used a lot so declared here
  unsigned long long int t = 0;     // tick
  int frameCount = 0;
  float tankX[MAX_TANKS], tankY[MAX_TANKS], tankLX[MAX_TANKS],
      tankLY[MAX_TANKS], tankA[MAX_TANKS], tankLA[MAX_TANKS],
      tankCooldown[MAX_TANKS],
      tankAmmoCooldown[MAX_TANKS]; // array of all tank X & Y positions, and
                                   // tank && tank angles (always use RADIAN)
  unsigned short int tankAmmo[MAX_TANKS], tankKills[MAX_TANKS],
      tankDeaths[MAX_TANKS], tankInv[MAX_TANKS]; // health and ammos
  bool isTankGreen[MAX_TANKS] = {
      true}; // array storing the team or side of tanks
  unsigned short int lowerTank = 0, upperTank = 0, myTank = 0;
  unsigned short int tanks = 0; // amount of tanks currently playing
  unsigned short int layer = 7; // currently in game layer
  // 0: menu, 1: game, 2: win screen, 3: multiplayer menu, 4: settings,
  // 5: credits, 6: pause menu, 7: intro
  bool mousePressed = false, onlineMode = false;
  unsigned short int keysPressed = 0;
  Keyboard::Key key;
  short int buttonX[MAX_BUTTONS], buttonY[MAX_BUTTONS], buttonW[MAX_BUTTONS],
      buttonH[MAX_BUTTONS], buttons = 1;
  short int buttonLayer[MAX_BUTTONS] = {-1};
  short int glare[MAX_GLARES] = {0}, glareX[MAX_GLARES], glareY[MAX_GLARES];
  short int glares = 0, nextGlare = 0;
  unsigned int greenKills = 0, redKills = 0;
  float timeLeft = CLOCK_TIME;
  float tankV[MAX_TANKS] = {0}, tankW[MAX_TANKS] = {0};
  // power ups. shield and boost are seconds left, shots is 1, 2 or 3 balls
  float tankShield[MAX_TANKS] = {0}, tankBoost[MAX_TANKS] = {0},
        tankShotsTime[MAX_TANKS] = {0};
  unsigned short int tankShots[MAX_TANKS] = {1};
  char tankShoot[MAX_TANKS] = {0};     // wants to shoot
  bool tankSpawn[MAX_TANKS] = {false}; // wants to spawn
  float tankLastX[MAX_TANKS] = {0},
        tankLastY[MAX_TANKS] = {0};           // last tire mark
  float tankSeen[MAX_TANKS] = {0};            // last seen
  unsigned char tankShotSeq[MAX_TANKS] = {0}; // shots counted from the network
  string tankName[MAX_TANKS];
  unsigned int tankDevice[MAX_TANKS] = {0}, tankIp[MAX_TANKS] = {0};
  float trackX[MAX_TRACKS], trackY[MAX_TRACKS], trackA[MAX_TRACKS],
      trackLife[MAX_TRACKS] = {0};
  float scorchX[MAX_SCORCH], scorchY[MAX_SCORCH], scorchA[MAX_SCORCH],
      scorchLife[MAX_SCORCH] = {0};
  int nextTrack = 0, nextScorch = 0;
  float powerX[MAX_POWERUPS], powerY[MAX_POWERUPS];
  short int powerType[MAX_POWERUPS] = {0}; // 0 none, 1 shield, 2 boost, 3 two
                                           // shots, 4 three shots
  float powerTimer = 3.f;
  int shootIdx = 0, hitIdx = 0; // which of the sound copies plays next
  // menu stuff
  bool startMatch = false, sendStart = false, leaveNet = false;
  bool isHost = true; // offline games count as hosting too
  bool typingName = false, touchMode = false;
  float matchTime = CLOCK_TIME, volume = 60.f, introTime = 0.f;
  // networking
  UdpSocket socket;
  bool networkOk = socket.bind(PORT) == Socket::Status::Done;
  socket.setBlocking(false);
  optional<IpAddress> localIp = IpAddress::getLocalAddress();
  unsigned int deviceId =
      localIp ? localIp->toInteger() // service id!
              : ((unsigned(rand()) << 16) | unsigned(rand()) | 1u); // random id
  // devices get named by the last number of their local ip
  string playerName = "Tank " + to_string(deviceId & 255u);
  unsigned char lobbyRole = 0; // 0: not in a lobby, 1: host, 2: client
  unsigned int hostAddr = 0, netW = width, netH = height, pendingId = 0;
  unsigned int inviteId = 0, inviteIp = 0;
  string inviteName;
  bool inviteHosting = false;
  float netTimer = 0.f, discoverTimer = 0.f, hostSeen = 0.f, inviteTimer = 0.f,
        pendingTimer = 0.f, netMatchTime = CLOCK_TIME;
  unsigned char shotSeq = 0;
  unsigned int peerId[MAX_PEERS], peerIp[MAX_PEERS];
  string peerName[MAX_PEERS];
  unsigned char peerStatus[MAX_PEERS], peerCount[MAX_PEERS];
  float peerSeen[MAX_PEERS];
  int peers = 0;
  unsigned int packetsRec = 0;   // amt of packets recieved
  bool touchActive[5] = {false}; // touch store
  Vector2i touchPos[5];

  // load stuff
  // matchTime, volume, playerName
  FileInputStream sFile;
  if (sFile.open("settings.ini")) {
    auto fileSize = sFile.getSize();
    if (fileSize.has_value() && fileSize.value() > 0) {
      string buffer(fileSize.value(), '\0');
      if (sFile.read(buffer.data(), fileSize.value())) {
        istringstream stream(buffer);
        string line;
        while (getline(stream, line)) {
          size_t delim = line.find("=");
          if (delim != string::npos) {
            string key = line.substr(0, delim);
            string val = line.substr(delim + 1);
            if (key == "playerName")
              playerName = val;
            else if (key == "volume")
              volume = stof(val);
            else if (key == "matchTime")
              matchTime = stof(val);
          }
        }
      }
    }
  }

  // button title and action is hardcoded to array 1
  // --- BUTTON DECLARATIONS ---
  // play button
  meow buttonLayer[0] = 0;
  buttonW[0] = 200;
  purrrr buttonH[0] = 40;
  buttonX[0] = width / 2 - buttonW[0] / 2;
  buttonY[0] = height / 2 - buttonH[0] / 2;
  buttons = 26;
  for (int b = 1; b < buttons; b++) {
    buttonW[b] = 200;
    buttonH[b] = 40;
    buttonX[b] = buttonX[0];
    buttonY[b] = buttonY[0];
    buttonLayer[b] = -1;
  }
  // 1 multiplayer, 2 settings, 3 credits, 4 quit
  for (int b = 1; b <= 4; b++) {
    buttonLayer[b] = 0;
    buttonY[b] = buttonY[0] + b * 50;
  }
  // 5 play again, 6 main menu
  buttonLayer[5] = 2;
  buttonLayer[6] = 2;
  buttonY[6] = buttonY[0] + 50;
  // 7 start match, 8 leave, 9 accept invite, 10 decline: layer 3
  buttonLayer[7] = 3;
  buttonY[7] = height - 150;
  buttonLayer[8] = 3;
  buttonY[8] = height - 100;
  buttonLayer[9] = 3;
  buttonX[9] = width / 2 - 210;
  buttonY[9] = 120;
  buttonLayer[10] = 3;
  buttonX[10] = width / 2 + 10;
  buttonY[10] = 120;
  // settings; 11-12 match time, 13-14 volume, 15 back,
  // 16 name box
  int sy = height / 2 - 120;
  buttonLayer[16] = 4;
  buttonW[16] = 300;
  buttonX[16] = width / 2 - 150;
  buttonY[16] = sy;
  for (int b = 11; b <= 14; b++) {
    buttonLayer[b] = 4;
    buttonW[b] = 40;
    buttonX[b] = (b % 2 == 1) ? width / 2 - 170 : width / 2 + 130;
    buttonY[b] = sy + (b < 13 ? 70 : 140);
  }
  buttonLayer[15] = 4;
  buttonY[15] = sy + 230;
  // 17 back
  buttonLayer[17] = 5;
  buttonY[17] = height - 100;
  // 18 resume, 19 main menu
  buttonLayer[18] = 6;
  buttonLayer[19] = 6;
  buttonY[19] = buttonY[0] + 50;
  // 20 to 25 network devices
  for (int b = 20; b < 20 + MAX_PEERS; b++) {
    buttonLayer[b] = 3;
    buttonW[b] = 360;
    buttonX[b] = width / 2 - 180;
    buttonY[b] = 190 + (b - 20) * 46;
  }
  // 26: lobby namebox, 27-28: match time buttons for host
  buttonLayer[26] = 3;
  buttonW[26] = 200;
  buttonH[26] = 35;
  buttonX[26] = 40;
  buttonY[26] = height - 120;
  buttonLayer[27] = 3;
  buttonLayer[28] = 3;
  buttonW[27] = buttonW[28] = 35;
  buttonH[27] = buttonH[28] = 35;
  buttonX[27] = width - 150;
  buttonX[28] = width - 70;
  buttonY[27] = buttonY[28] = height - 120;
  buttons = 29;

  Vector2f rocks[10] = {{width / 2.f, height * 2.f / 3.f},
                        {width * 1.f / 4.f, height * 2.f / 5.f},
                        {width * 2.f / 3.f, height * 4.f / 7.f},
                        {width * 4.f / 5.f, height * 1.f / 5.f},
                        {width * 1.f / 5.f, height * 4.f / 5.f},
                        {width * 1.f / 8.f, height * 1.f / 8.f},
                        {width * 7.f / 8.f, height * 7.f / 8.f},
                        {width * 1.f / 2.f, height * 1.f / 4.f},
                        {width * 3.f / 4.f, height * 3.f / 4.f},
                        {width * 3.f / 8.f, height * 7.f / 8.f}};

  // on screen buttons for touch screens (android). 0 up, 1 down, 2 left,
  // 3 right, 4 fire, 5 pause
  float pad = baseH / 7.f;
  FloatRect touchRects[6] = {
      FloatRect({pad * 1.1f, baseH - pad * 3.2f}, {pad, pad}),
      FloatRect({pad * 1.1f, baseH - pad * 1.1f}, {pad, pad}),
      FloatRect({pad * 0.0f, baseH - pad * 2.15f}, {pad, pad}),
      FloatRect({pad * 2.2f, baseH - pad * 2.15f}, {pad, pad}),
      FloatRect({baseW - pad * 2.2f, baseH - pad * 2.2f},
                {pad * 1.6f, pad * 1.6f}),
      FloatRect({baseW - pad * 0.9f, 10.f}, {pad * 0.7f, pad * 0.7f})};
#ifdef __ANDROID__
  touchMode = true;
#endif

  // make sure all devices have the same render size on a network
  View gameView(FloatRect({0.f, 0.f}, {float(width), float(height)}));

  // --- TEXTURES ---
  Texture greenTankTexture("assets/tank1.png");
  Texture redTankTexture("assets/tank2.png");
  Texture bulletTexture[4];
  Texture ammoT[6];
  Texture rockT("assets/rocks.png");
  Texture radialBlur("assets/radial.png");
  Texture sand("assets/sand.png");
  Texture trackT("assets/track.png");
  Texture scorchT("assets/scorch.png");
  Texture powerT("assets/powerups.png");
  Texture logoT("assets/logo.png");

  for (int i = 0; i < 4; i++) {
    if (bulletTexture[i].loadFromFile("assets/bullet" + to_string(i) +
                                      ".png")) {
      bulletTexture[i].setSmooth(false);
    }
  }
  for (int i = 5; i >= 0; i--) {
    if (ammoT[5 - i].loadFromFile("assets/ammo" + to_string(i) + ".png")) {
      ammoT[i].setSmooth(false);
    }
  }

  // --- OBJECTS ---
  // "i can forsee that this list is gonna becoem soooo long someday-"
  Font font("assets/font.ttf");
  Text fpsText(font);
  Text timeText(font);
  Text buttonText(font);
  Text greenText(font);
  Text redText(font);
  Text titleText(font);
  Text infoText(font); // loose text
  // Sprite cannonball(bulletTexture[0]);
  CircleShape cannonball(ballRadius, ballPts);
  Sprite ammoBar(ammoT[5]);
  Sprite greenTank(greenTankTexture);
  Sprite redTank(redTankTexture);
  Sprite glareSprite(radialBlur);
  Sprite bg(sand);
  Sprite rock1(rockT);
  Sprite rock2(rockT);
  Sprite rock3(rockT);
  Sprite trackSprite(trackT);
  Sprite scorchSprite(scorchT);
  Sprite powerSprite(powerT);
  Sprite logoSprite(logoT);
  CircleShape shieldCircle(TANK_HIT_RADIUS + 8.f, 30);
  RectangleShape dim({float(baseW), float(baseH)});
  RectangleShape touchBtn({10, 10});
  RectangleShape button({10, 10});

  // --- SOUNDS ---
  SoundBuffer shootBuf("assets/shoot.wav");
  SoundBuffer hitBuf("assets/hit.wav");
  SoundBuffer moveBuf("assets/move.wav");
  // multiple copies so more than one can play at a time
  Sound shootSnd[4] = {Sound(shootBuf), Sound(shootBuf), Sound(shootBuf),
                       Sound(shootBuf)};
  Sound hitSnd[2] = {Sound(hitBuf), Sound(hitBuf)};
  Sound moveSnd(moveBuf);
  Music music("assets/music.ogg");

  // --- OBJECT PROPERTIES ---
  cannonball.setOrigin({2.f, 2.f});
  cannonball.setFillColor(Color::Black);
  font.setSmooth(false);
  sand.setSmooth(false);
  sand.setRepeated(true);
  rockT.setSmooth(false);
  bg.setPosition({0.f, 0.f});
  bg.setScale({3.f, 3.f});
  bg.setTextureRect(IntRect({0, 0}, {int(width), int(height)}));
  greenTankTexture.setSmooth(false);
  redTankTexture.setSmooth(false);
  greenTank.setScale({4.0f, 4.0f});
  redTank.setScale({4.0f, 4.0f});
  ammoBar.setScale({1.3f, 1.3f});
  button.setOutlineThickness(-2);
  fpsText.setCharacterSize(24);
  rock1.setTextureRect(IntRect({0, 0}, {11, 9}));
  rock2.setTextureRect(IntRect({1, 11}, {11, 10}));
  rock3.setTextureRect(IntRect({0, 22}, {12, 10}));
  fpsText.setFillColor(sf::Color::Green);
  fpsText.setPosition({10.f, 10.f});
  rock1.setScale({3.f, 3.f});
  rock2.setScale({3.f, 3.f});
  rock3.setScale({3.f, 3.f});
  FloatRect glareBounds = glareSprite.getLocalBounds();
  FloatRect rock1Bounds = rock1.getLocalBounds();
  FloatRect rock2Bounds = rock2.getLocalBounds();
  FloatRect rock3Bounds = rock3.getLocalBounds();
  FloatRect ammoBounds = ammoBar.getLocalBounds();
  FloatRect greenBounds = greenTank.getLocalBounds();
  FloatRect redBounds = redTank.getLocalBounds();
  glareSprite.setOrigin({glareBounds.size.x / 2.f, glareBounds.size.y / 2.f});
  rock1.setOrigin({rock1Bounds.size.x / 2.f, rock1Bounds.size.y / 2.f});
  rock2.setOrigin({rock2Bounds.size.x / 2.f, rock2Bounds.size.y / 2.f});
  rock3.setOrigin({rock3Bounds.size.x / 2.f, rock3Bounds.size.y / 2.f});
  ammoBar.setOrigin({ammoBounds.size.x / 2.f, ammoBounds.size.y / 2.f});
  greenTank.setOrigin({greenBounds.size.x / 2.f, greenBounds.size.y / 2.f});
  redTank.setOrigin({redBounds.size.x / 2.f, redBounds.size.y / 2.f});
  trackT.setSmooth(false);
  scorchT.setSmooth(false);
  powerT.setSmooth(false);
  logoT.setSmooth(false);
  trackSprite.setScale({2.8f, 2.8f});
  scorchSprite.setScale({4.f, 4.f});
  trackSprite.setOrigin({trackSprite.getLocalBounds().size.x / 2.f,
                         trackSprite.getLocalBounds().size.y / 2.f});
  scorchSprite.setOrigin({scorchSprite.getLocalBounds().size.x / 2.f,
                          scorchSprite.getLocalBounds().size.y / 2.f});
  powerSprite.setOrigin({8.f, 8.f});
  logoSprite.setOrigin({logoT.getSize().x / 2.f, logoT.getSize().y / 2.f});
  logoSprite.setPosition({baseW / 2.f, baseH / 2.f - 40.f});
  logoSprite.setScale(
      {baseH / 3.f / logoT.getSize().y, baseH / 3.f / logoT.getSize().y});
  shieldCircle.setOrigin({TANK_HIT_RADIUS + 8.f, TANK_HIT_RADIUS + 8.f});
  shieldCircle.setFillColor(Color(80, 200, 255, 50));
  shieldCircle.setOutlineColor(Color(120, 230, 255, 200));
  shieldCircle.setOutlineThickness(2.f);
  dim.setFillColor(Color(0, 0, 0, 150));
  touchBtn.setFillColor(Color(255, 255, 255, 60));
  touchBtn.setOutlineColor(Color(255, 255, 255, 140));
  touchBtn.setOutlineThickness(-2.f);
  titleText.setCharacterSize(72);
  titleText.setFillColor(Color::White);
  titleText.setOutlineColor(Color::Black);
  titleText.setOutlineThickness(2);
  infoText.setOutlineColor(Color::Black);
  infoText.setOutlineThickness(1);
  moveSnd.setLooping(true);
  moveSnd.setVolume(35.f);
  music.setLooping(true);
  music.setVolume(40.f);
  Listener::setGlobalVolume(volume);
  music.play();

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
      if (event->is<Event::Closed>()) {
        FILE *outFile = fopen("settings.ini", "w");
        if (outFile) {
          fprintf(outFile, "playerName=%s\n", playerName.c_str());
          fprintf(outFile, "volume=%.2f\n", volume);
          fprintf(outFile, "matchTime=%.2f\n", matchTime);
          fclose(outFile);
        }
        window.close();
      } // mouse events
      else if (const auto *evnt = event->getIf<Event::MouseButtonPressed>()) {
        // convert screen pixel position to world coordinates
        // this is initially effect less, but as soon as we scale or resize the
        // view, the pixel position is no longer equivalent to world position
        // (menus always live in our own screen's coordinates, so we map with
        // the default view even while the game view is in use)
        Vector2f worldPos = window.mapPixelToCoords(
            evnt->position, window.getDefaultView()); // mapped mouse coords
        mouseX = worldPos.x;
        mouseY = worldPos.y;
        mousePressed = mouseJustPressed = true;
        if (layer == 7) {
          introTime = 99.f; // click skip intro
          mouseJustPressed = false;
        }
      } else if (const auto *evnt =
                     event->getIf<Event::MouseButtonReleased>()) {
        mousePressed = false;
      } else if (const auto *evnt = event->getIf<Event::MouseMoved>()) {
        Vector2f worldPos = window.mapPixelToCoords(
            evnt->position, window.getDefaultView()); // mapped mouse coords
        mouseX = worldPos.x;
        mouseY = worldPos.y;
        // touch screen support
      } else if (const auto *evnt = event->getIf<Event::TouchBegan>()) {
        touchMode = true;
        Vector2f worldPos =
            window.mapPixelToCoords(evnt->position, window.getDefaultView());
        mouseX = worldPos.x;
        mouseY = worldPos.y;
        mousePressed = mouseJustPressed = true;
        if (layer == 7) {
          introTime = 99.f;
          mouseJustPressed = false;
        }
        if (layer == 1 && touchRects[5].contains(worldPos))
          layer = 6; // pause button
        else if (layer == 1 && touchRects[4].contains(worldPos) &&
                 tankCooldown[myTank] <= 0.f && tankAmmo[myTank] > 0) {
          tankShoot[myTank] = true; // fire button
          tankCooldown[myTank] = TANK_COOLDOWN;
          tankAmmo[myTank]--;
        }
        if (evnt->finger < 5) {
          touchActive[evnt->finger] = true;
          touchPos[evnt->finger] = evnt->position;
        }
      } else if (const auto *evnt = event->getIf<Event::TouchMoved>()) {
        if (evnt->finger < 5) {
          touchPos[evnt->finger] = evnt->position;
        }
      } else if (const auto *evnt = event->getIf<Event::TouchEnded>()) {
        mousePressed = false;
        if (evnt->finger < 5) {
          touchActive[evnt->finger] = false;
        }
        // typing the player name
      } else if (const auto *evnt = event->getIf<Event::TextEntered>()) {
        if ((layer == 4 || layer == 3) && typingName) {
          if (evnt->unicode == 8) { // backspace
            if (!playerName.empty())
              playerName.pop_back();
          } else if (evnt->unicode == 13) { // enter
            typingName = false;
          } else if (evnt->unicode >= 32 && evnt->unicode < 127 &&
                     playerName.size() < 12) {
            playerName += static_cast<char>(evnt->unicode);
          }
          if (layer == 3 && lobbyRole != 0) {
            tankName[myTank] = playerName;
          }
        }
        // keyboard events
      } else if (const auto *evnt = event->getIf<Event::KeyPressed>()) {
        keysPressed++;
        keysJustPressed++;
        key = evnt->code;
        if (layer == 7)
          introTime = 99.f; // any key skips the intro
        if (evnt->code == Keyboard::Key::Escape) {
          if (layer == 1) {
            layer = 6; // pause
            window.setMouseCursorGrabbed(false);
          } else if (layer == 6) {
            layer = 1; // resume
            window.setMouseCursorGrabbed(true);
          } else if (layer == 3 || layer == 4 || layer == 5) {
            if (layer == 3 && lobbyRole != 0)
              leaveNet = true; // leave
            typingName = false;
            layer = 0;
          }
        }
      } else if (const auto *evnt = event->getIf<Event::KeyReleased>()) {
        keysPressed--;
        Keyboard::Key k = evnt->code;
        // shooting only raises the tankShoot flag, the flag is turned into
        // actual balls (one, two or three) a bit further down
        if (onlineMode) {
          if (layer == 1 && k == Keyboard::Key::W &&
              tankCooldown[myTank] <= 0.f &&
              tankAmmo[myTank] > 0) { // local green tank
            tankShoot[myTank] = true;
            tankCooldown[myTank] = TANK_COOLDOWN;
            tankAmmo[myTank]--;
          } else if (layer == 1 && (myTank + 1 < tanks) &&
                     tankDevice[myTank + 1] == deviceId &&
                     k == Keyboard::Key::Up &&
                     tankCooldown[myTank + 1] <= 0.f &&
                     tankAmmo[myTank + 1] > 0) { // local red tank
            tankShoot[myTank + 1] = true;
            tankCooldown[myTank + 1] = TANK_COOLDOWN;
            tankAmmo[myTank + 1]--;
          }
        } else {
          if (layer == 1 && k == Keyboard::Key::W &&
              tankCooldown[myTank] <= 0.f &&
              tankAmmo[myTank] >
                  0) { // in offline mode, shoot based on key pressed
            tankShoot[myTank] = true;
            tankCooldown[myTank] = TANK_COOLDOWN;
            tankAmmo[myTank]--;
          } else if (layer == 1 && k == Keyboard::Key::Up &&
                     tankCooldown[1] <= 0.f && tankAmmo[1] > 0) {
            tankShoot[1] = true;
            tankCooldown[1] = TANK_COOLDOWN;
            tankAmmo[1]--;
          }
        }
      }
    }
    // deltaTime calc
    float deltaTime = clock.restart().asSeconds();
    totalTime += deltaTime;
    frameCount++;

    // --- INTRO ---
    if (layer == 7) {
      introTime += deltaTime;
      if (introTime >= 3.5f)
        layer = 0;
    }

    // --- MATCH START ---
    // the play buttons and the network only raise startMatch, the setup that
    // used to sit inside the play button lives here so every way of starting
    // a match shares it
    if (startMatch) {
      startMatch = false;
      layer = 1;
      // our world is our own screen, except for online clients who borrow the
      // host's size so that positions line up on every device
      unsigned int newW = baseW, newH = baseH;
      if (onlineMode && !isHost) {
        newW = netW;
        newH = netH;
      }
      for (int r = 0; r < 10; r++) { // rocks follow the world size
        rocks[r].x *= float(newW) / width;
        rocks[r].y *= float(newH) / height;
      }
      width = newW;
      height = newH;
      bg.setTextureRect(IntRect({0, 0}, {int(newW > baseW ? newW : baseW),
                                         int(newH > baseH ? newH : baseH)}));
      // fit instead of stretch
      Vector2u winSize = window.getSize();
      float winRatio = float(winSize.x) / winSize.y;
      float worldRatio = float(width) / height;
      FloatRect vp({0.f, 0.f}, {1.f, 1.f});
      if (winRatio > worldRatio) {
        vp.size.x = worldRatio / winRatio;
        vp.position.x = (1.f - vp.size.x) / 2.f;
      } else {
        vp.size.y = winRatio / worldRatio;
        vp.position.y = (1.f - vp.size.y) / 2.f;
      }
      gameView.setSize({float(width), float(height)});
      gameView.setCenter({width / 2.f, height / 2.f});
      gameView.setViewport(vp);
      // Initiate match
      timeLeft = (onlineMode && !isHost) ? netMatchTime : matchTime;
      redText.setCharacterSize(36);
      greenText.setCharacterSize(36);
      timeText.setCharacterSize(36);
      redText.setFillColor(sf::Color::Red);
      redText.setOutlineColor(Color::Black);
      redText.setOutlineThickness(1);
      redText.setPosition({width - 10.f, height - 40.f});
      greenText.setFillColor(sf::Color::Green);
      greenText.setPosition({10.f, height - 40.f});
      greenText.setOutlineColor(Color::Black);
      greenText.setOutlineThickness(1);
      purrrr timeText.setFillColor(sf::Color::Black);
      timeText.setPosition({float(width) / 2.f, 20.f});
      if (!onlineMode) { // this would be offline mode, always two tanks
        tanks = 2;
        tankName[0] = "Green";
        tankName[1] = "Red";
      }
      for (int i = 0; i < tanks; i++) {
        isTankGreen[i] = i % 2 == 0; // even tanks are green, odd ones red
        tankSpawn[i] = true;
        tankX[i] = tankY[i] = tankLX[i] = tankLY[i] = tankA[i] = tankLA[i] =
            0.f;
        tankAmmo[i] = 5; // initial ammo and
        tankCooldown[i] = 0.1f;
        tankAmmoCooldown[i] = 5.f;
        tankDeaths[i] = tankKills[i] = 0;
        tankInv[i] = 120;
        tankV[i] = tankW[i] = 0.f;
        tankShield[i] = tankBoost[i] = tankShotsTime[i] = 0.f;
        tankShots[i] = 1;
        tankShoot[i] = false;
        tankShotSeq[i] = 0;
        tankSeen[i] = 0.f;
      }
      redKills = greenKills = 0;
      lowerTank = 0;
      upperTank = tanks > 0 ? tanks - 1 : 0;
      for (int i = 0; i < ballPool; i++)
        ballState[i] = 0;
      balls = nextBall = lowerBall = upperBall = queueSize = 0;
      for (int i = 0; i < MAX_GLARES; i++)
        glare[i] = 0;
      for (int i = 0; i < MAX_TRACKS; i++)
        trackLife[i] = 0.f;
      for (int i = 0; i < MAX_SCORCH; i++)
        scorchLife[i] = 0.f;
      for (int i = 0; i < MAX_POWERUPS; i++)
        powerType[i] = 0;
      powerTimer = 3.f;
      shotSeq = 0;
      netTimer = 0.f;
      hostSeen = 0.f;
      mouseJustPressed = false;
    }

    // --- NETWORK START / LEAVE ---
    bool inMatch = layer == 1 || layer == 2 || layer == 6;
    if (leaveNet) { // every way of leaving a lobby or match ends up here
      leaveNet = false;
      if (networkOk && lobbyRole == 1) { // host going away tells everyone
        for (int i = 1; i < tanks; i++) {
          Packet p;
          p << U8(9) << deviceId;
          if (socket.send(p, IpAddress(tankIp[i]), PORT) ==
              Socket::Status::Done)
            ;
        }
      } else if (networkOk && lobbyRole == 2) {
        Packet p;
        p << U8(9) << deviceId;
        if (socket.send(p, IpAddress(hostAddr), PORT) == Socket::Status::Done)
          ;
      }
      lobbyRole = 0;
      onlineMode = false;
      isHost = true;
      myTank = 0;
      tanks = 0;
      upperTank = 0;
      hostAddr = 0;
      inviteTimer = pendingTimer = 0.f;
      startMatch = false;
    }
    if (sendStart && networkOk) {
      sendStart = false; // the host tells every client the match begins
      for (int i = 1; i < tanks; i++) {
        Packet p;
        p << U8(6) << deviceId << U8(tanks) << U8(i) << matchTime
          << std::uint32_t(width) << std::uint32_t(height);
        for (int k = 0; k < tanks; k++)
          p << tankName[k];
        for (int rep = 0; rep < 3;
             rep++) // stream more than once to avoic drops
          if (socket.send(p, IpAddress(tankIp[i]), PORT) ==
              Socket::Status::Done)
            ;
      }
    }

    // --- NETWORK LISTENING ---
    if (networkOk) {
      netTimer += deltaTime;
      discoverTimer += deltaTime;
      hostSeen += deltaTime;
      if (inviteTimer > 0.f)
        inviteTimer -= deltaTime;
      if (pendingTimer > 0.f)
        pendingTimer -= deltaTime;
      // forget devices we haven't heard from in a while
      for (int p = 0; p < peers; p++) {
        peerSeen[p] += deltaTime;
        if (peerSeen[p] > 3.5f) {
          for (int q = p; q < peers - 1; q++) {
            peerId[q] = peerId[q + 1];
            peerIp[q] = peerIp[q + 1];
            peerName[q] = peerName[q + 1];
            peerStatus[q] = peerStatus[q + 1];
            peerCount[q] = peerCount[q + 1];
            peerSeen[q] = peerSeen[q + 1];
          }
          peers--;
          p--;
        }
      }

      // once a second, say hi to everyone, keep the lobby alive
      if (discoverTimer >= 1.f) {
        discoverTimer = 0.f;
        // hosts always broadcast, everyone else only while they're not in a
        // match or someone elses lobby
        if (lobbyRole == 1 || (lobbyRole == 0 && !inMatch)) {
          Packet p;
          p << U8(1) << deviceId << playerName
            << U8(lobbyRole == 1 ? (inMatch ? 2 : 1) : 0)
            << U8(lobbyRole == 1 ? tanks : 0);
          if (socket.send(p, IpAddress::Broadcast, PORT) ==
              Socket::Status::Done)
            ;
        }
        if (lobbyRole == 1 && layer != 1 && layer != 6) { // lobby list
          Packet p;
          p << U8(5) << deviceId << U8(tanks);
          for (int i = 0; i < tanks; i++)
            p << tankName[i];
          for (int i = 1; i < tanks; i++)
            if (socket.send(p, IpAddress(tankIp[i]), PORT) ==
                Socket::Status::Done)
              ;
        }
        if (lobbyRole == 2 && layer != 1 && layer != 6) { // heartbeat
          Packet p;
          p << U8(4) << deviceId << playerName;
          if (socket.send(p, IpAddress(hostAddr), PORT) == Socket::Status::Done)
            ;
        }
      }

      // read everything that arrived since last frame
      while (true) {
        Packet pkt;
        optional<IpAddress> from;
        unsigned short fromPort = 0;
        if (socket.receive(pkt, from, fromPort) != Socket::Status::Done ||
            !from)
          break;
        std::uint8_t magic = 0, type = 0;
        std::uint32_t id = 0;
        if (!(pkt >> magic >> type >> id) || magic != 42)
          continue; // drop malformed or wrong game version
        if (id == deviceId) {
          continue; // our own broadcast came back around
        }
        packetsRec++;
        unsigned int fromIp = from->toInteger();

        if (type == 1) { // somebody is broadcasting that they exist
          string name;
          std::uint8_t status = 0, count = 0;
          pkt >> name >> status >> count;
          int slot = -1;
          for (int p = 0; p < peers; p++)
            if (peerId[p] == id)
              slot = p;
          if (slot < 0 && peers < MAX_PEERS)
            slot = peers++;
          if (slot >= 0) {
            peerId[slot] = id;
            peerIp[slot] = fromIp;
            peerName[slot] = name;
            peerStatus[slot] = status;
            peerCount[slot] = count;
            peerSeen[slot] = 0.f;
          }
        } else if (type == 2) { // an invite
          string name;
          std::uint8_t hosting = 0;
          pkt >> name >> hosting;
          if (lobbyRole == 0 && !inMatch) {
            inviteId = id;
            inviteIp = fromIp;
            inviteName = name;
            inviteHosting = hosting != 0;
            inviteTimer = 20.f;
          }
        } else if (type == 3) { // our invite or join request got accepted
          if (lobbyRole == 0 && pendingTimer > 0.f && pendingId == id) {
            lobbyRole = 2;
            hostAddr = fromIp;
            hostSeen = 0.f;
            pendingTimer = 0.f;
            tanks = 0;
            layer = 3;
          }
        } else if (type == 4) { // join request, also the lobby heartbeat
          string name;
          pkt >> name;
          if (lobbyRole == 1 && layer == 3) {
            int idx = -1;
            for (int i = 0; i < tanks; i++) // only one tank per device id
              if (tankDevice[i] == id)
                idx = i;
            if (idx < 0 && tanks < MAX_PLAYERS) {
              idx = tanks++;
              tankDevice[idx] = id;
            }
            if (idx >= 0) {
              tankIp[idx] = fromIp;
              tankName[idx] = name;
              tankSeen[idx] = 0.f;
              Packet p;
              p << U8(3) << deviceId << playerName;
              if (socket.send(p, *from, PORT) == Socket::Status::Done)
                ;
            }
          }
        } else if (type == 5) { // lobby list from the host
          if (lobbyRole == 2 && fromIp == hostAddr) {
            std::uint8_t count = 0;
            pkt >> count;
            tanks = count < MAX_PLAYERS ? count : MAX_PLAYERS;
            for (int i = 0; i < tanks; i++)
              pkt >> tankName[i];
            hostSeen = 0.f;
          }
        } else if (type == 6) { // the host started a match
          if (lobbyRole == 2 && fromIp == hostAddr &&
              (layer == 3 || layer == 2)) {
            std::uint8_t count = 0, mine = 0;
            float mt = CLOCK_TIME;
            std::uint32_t ww = width, hh = height;
            pkt >> count >> mine >> mt >> ww >> hh;
            tanks = count < MAX_PLAYERS ? count : MAX_PLAYERS;
            myTank = mine < tanks ? mine : 0;
            for (int i = 0; i < tanks; i++)
              pkt >> tankName[i];
            netMatchTime = mt;
            netW = ww;
            netH = hh;
            onlineMode = true;
            isHost = false;
            startMatch = true;
            hostSeen = 0.f;
          }
        } else if (type == 7) { // a client tells us where its tank is
          if (lobbyRole == 1 && (layer == 1 || layer == 6)) {
            std::uint8_t seq = 0;
            std::uint16_t deaths = 0;
            float x = 0.f, y = 0.f, a = 0.f;
            pkt >> seq >> deaths >> x >> y >> a;
            for (int i = 1; i < tanks; i++) {
              if (tankDevice[i] != id)
                continue;
              tankSeen[i] = 0.f;
              // after a death we ignore its position until it has caught up
              // with the respawn
              if (deaths == tankDeaths[i]) {
                tankX[i] = x;
                tankY[i] = y;
                tankA[i] = a;
              }
              unsigned char fresh =
                  static_cast<unsigned char>(seq - tankShotSeq[i]);
              if (fresh > 0 && fresh < 128) {
                tankShotSeq[i] = seq;
                tankShoot[i] += fresh;
                tankCooldown[i] = TANK_COOLDOWN;
                if (tankAmmo[i] >= fresh)
                  tankAmmo[i] -= fresh;
                else
                  tankAmmo[i] = 0;
              }
            }
          }
        } else if (type == 8) { // the host's picture of the whole match
          if (lobbyRole == 2 && fromIp == hostAddr && onlineMode &&
              (layer == 1 || layer == 2 || layer == 6)) {
            std::uint8_t phase = 1, count = 0;
            float tl = 0.f;
            std::uint16_t gk = 0, rk = 0;
            pkt >> phase >> tl >> gk >> rk >> count;
            hostSeen = 0.f;
            timeLeft = tl;
            greenKills = gk;
            redKills = rk;
            if (phase == 2 && layer != 2)
              layer = 2;
            for (int i = 0; i < count && i < MAX_PLAYERS && i < tanks; i++) {
              float x, y, a, sh, bo, st;
              std::uint8_t ammo, inv, shots;
              std::uint16_t deaths;
              pkt >> x >> y >> a >> ammo >> inv >> sh >> bo >> shots >> st >>
                  deaths;
              if (deaths != tankDeaths[i]) { // that tank just blew up
                scorchX[nextScorch] = tankX[i];
                scorchY[nextScorch] = tankY[i];
                scorchA[nextScorch] = (rand() % 628) / 100.f;
                scorchLife[nextScorch] = SCORCH_LIFE;
                nextScorch = (nextScorch + 1) % MAX_SCORCH;
                hitSnd[hitIdx].play();
                hitIdx = (hitIdx + 1) % 2;
                tankDeaths[i] = deaths;
                if (i == myTank) { // we respawned, take the new spot
                  tankX[i] = x;    // not tankLX or Y as our own tank's
                  tankY[i] = y;    // -position won't be interpolated
                  tankA[i] = a;
                  tankV[i] = tankW[i] = 0.f;
                  tankLastX[i] = x;
                  tankLastY[i] = y;
                }
              }
              if (i != myTank) { // TODO: We are recieiving the position of
                                 // other tanks
                tankX[i] = x;
                tankY[i] = y;
                tankA[i] = a;
                if (ammo < tankAmmo[i]) { // ammo went down, so it shot
                  shootSnd[shootIdx].play();
                  shootIdx = (shootIdx + 1) % 4;
                }
                tankAmmo[i] = ammo;
              }
              tankInv[i] = inv;
              tankShield[i] = sh;
              tankBoost[i] = bo;
              tankShots[i] = shots;
              tankShotsTime[i] = st;
            }
            for (int k = 0; k < MAX_POWERUPS; k++) {
              std::uint8_t ty = 0;
              float px = 0.f, py = 0.f;
              pkt >> ty >> px >> py;
              powerType[k] = ty;
              powerX[k] = px;
              powerY[k] = py;
            }
            std::uint16_t nb = 0;
            pkt >> nb;
            if (nb > MAX_NET_BALLS)
              nb = MAX_NET_BALLS;
            for (int k = 0; k < ballPool; k++) {
              ballState[k] = 0; // reset all balls
            }
            for (int k = 0; k < nb; k++) {
              std::int16_t bx = 0, by = 0;
              pkt >> bx >> by;
              ballsX[k] = static_cast<float>(bx);
              ballsY[k] = static_cast<float>(by);
              ballsA[k] = 0.f;
              ballState[k] = 1;
            }
            lowerBall = 0;
            upperBall = nb > 0 ? nb - 1 : 0;
            balls = nb;
            nextBall = nb % ballPool;
            for (int k = nb; k <= upperBall; k++)
              ballState[k] = 0; // balls that are gone since the last snapshot
            lowerBall = 0;
            upperBall = nb > 0 ? nb - 1 : 0;
            balls = nb;
            for (int k = 0; k < MAX_GLARES; k++) {
              std::uint8_t g = 0;
              std::int16_t gx = 0, gy = 0;
              pkt >> g >> gx >> gy;
              if (g > glare[k]) { // a fresh flash, show it too
                glare[k] = g;
                glareX[k] = gx;
                glareY[k] = gy;
              }
            }
          }
        } else if (type == 9) { // somebody left
          if (lobbyRole == 2 && fromIp == hostAddr) {
            layer = 0; // the host closed the lobby or match
            leaveNet = true;
          } else if (lobbyRole == 1) {
            for (int i = 1; i < tanks; i++)
              if (tankDevice[i] == id)
                tankSeen[i] = 999.f; // handled with the timeouts just below
          }
        }
      }

      // remove disconnected players
      if (lobbyRole == 1) {
        for (int i = 1; i < tanks; i++) {
          tankSeen[i] += deltaTime;
          if (tankSeen[i] <= NET_TIMEOUT)
            continue;
          if (layer == 3) {
            for (int q = i; q < tanks - 1; q++) {
              tankName[q] = tankName[q + 1];
              tankDevice[q] = tankDevice[q + 1];
              tankIp[q] = tankIp[q + 1];
              tankSeen[q] = tankSeen[q + 1];
            }
            tanks--;
            i--;
          } else if (layer == 1 || layer == 6) {
            tankX[i] = tankY[i] = tankLX[i] = tankLY[i] =
                -1000.f; // gone :fahh:
          }
        }
      }
      // client that lost the host
      if (lobbyRole == 2 && hostSeen > NET_TIMEOUT) {
        layer = 0;
        leaveNet = true;
      }
    } // Aaand that is the end of the network code :fahh:

    // BALL QUEUE MANAGEMENT MWAHAHAHAHAH
    // online matches keep running while the pause menu is open
    bool simulating = layer == 1 || (layer == 6 && onlineMode);
    if (simulating) {
      // shoot requests become balls. the host makes them, clients only count
      // their shots and the host hears about it in the next packet
      for (int i = lowerTank; i <= upperTank; i++) {
        while (tankShoot[i] > 0) {
          tankShoot[i]--;
          shootSnd[shootIdx].play();
          shootIdx = (shootIdx + 1) % 4;
          if (!isHost) {
            shotSeq++;
            continue;
          }
        }
        // one ball normally, two or three fanned out with the power ups
        for (int s = 0; s < tankShots[i]; s++) {
          float a = tankA[i] + (s - (tankShots[i] - 1) / 2.f) * SHOT_SPREAD;
          _qBallX[queueSize] =
              tankX[i] + sin(a) * 15.f; // do not use display position
          _qBallY[queueSize] =
              tankY[i] - cos(a) * 15.f; // -as this is exec right after
                                        // recieving new cannonballs
          _qBallVX[queueSize] = sin(a) * BALL_SPEED;
          _qBallVY[queueSize] = -cos(a) * BALL_SPEED;
          _qBallSource[queueSize] = i;
          queueSize++;
        }
      }

      // move balls from QUEUE to main array
      while (isHost && queueSize > 0) {
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

      // --- ARRAY CLEANUP ---
      if (isHost && t % 600 == 0 && balls != nextBall && lowerBall != 0 &&
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
      if (touchMode) { // on screen pad, any finger on a button counts
        for (unsigned int f = 0; f < 5; f++) {
          if (!touchActive[f])
            continue;
          Vector2f tp =
              window.mapPixelToCoords(touchPos[f], window.getDefaultView());
          if (touchRects[0].contains(tp))
            velGreen.y += 1.f;
          if (touchRects[1].contains(tp))
            velGreen.y -= 1.f;
          if (touchRects[2].contains(tp))
            velGreen.x -= 1.f;
          if (touchRects[3].contains(tp))
            velGreen.x += 1.f;
        }
      }
      if (layer != 1) { // pause menu in an online match: tanks coast to a stop
        velGreen = Vector2f(0.f, 0.f);
        velRed = Vector2f(0.f, 0.f);
      }
      if (onlineMode &&
          (myTank + 1 >= tanks || tankDevice[myTank + 1] != deviceId)) {
        // condition for checking whether two players are ^
        // playing from this device

        velGreen +=
            velRed; // control tank with either keys if local with 1 tank
      } // input ends here

      // the keys now say what speed we WANT, the tank ramps up (or down) to
      // it using the acceleration arrays tankV and tankW
      float boostG = tankBoost[myTank] > 0.f ? BOOST_MULT : 1.f;
      float wantV = velGreen.y * TANK_SPEED * boostG;
      float wantW = velGreen.x * TANK_ROTATION_SPEED;
      float stepV = TANK_ACCEL * deltaTime, stepW = TANK_ROT_ACCEL * deltaTime;

      if (tankV[myTank] < wantV - stepV)
        tankV[myTank] += stepV;
      else if (tankV[myTank] > wantV + stepV)
        tankV[myTank] -= stepV;
      else
        tankV[myTank] = wantV;
      if (tankW[myTank] < wantW - stepW)
        tankW[myTank] += stepW;
      else if (tankW[myTank] > wantW + stepW)
        tankW[myTank] -= stepW;
      else
        tankW[myTank] = wantW;
      tankA[myTank] += tankW[myTank] * deltaTime;
      // starting here no display position will be used
      tankX[myTank] += sin(tankA[myTank]) * tankV[myTank] * deltaTime;
      tankY[myTank] -= cos(tankA[myTank]) * tankV[myTank] * deltaTime;
      if (tankX[myTank] < TANK_HIT_RADIUS)
        tankX[myTank] = TANK_HIT_RADIUS;
      if (tankY[myTank] < TANK_HIT_RADIUS)
        tankY[myTank] = TANK_HIT_RADIUS;
      if (tankX[myTank] > width - TANK_HIT_RADIUS)
        meow meow meow purrrr tankX[myTank] = width - TANK_HIT_RADIUS;
      if (tankY[myTank] > height - TANK_HIT_RADIUS)
        tankY[myTank] = height - TANK_HIT_RADIUS;

      if (!onlineMode ||
          ((myTank + 1 < tanks) &&
           tankDevice[myTank + 1] == deviceId)) { // uhhh // works
        unsigned short altTank = myTank + 1;
        float boostR = tankBoost[altTank] > 0.f ? BOOST_MULT : 1.f;
        float wantVR = velRed.y * TANK_SPEED * boostR;
        float wantWR = velRed.x * TANK_ROTATION_SPEED;
        if (tankV[altTank] < wantVR - stepV)
          tankV[altTank] += stepV;
        else if (tankV[altTank] > wantVR + stepV)
          tankV[altTank] -= stepV;
        else
          tankV[altTank] = wantVR;
        if (tankW[altTank] < wantWR - stepW)
          tankW[altTank] += stepW;
        else if (tankW[altTank] > wantWR + stepW)
          tankW[altTank] -= stepW;
        else
          tankW[altTank] = wantWR;
        tankA[altTank] += tankW[altTank] * deltaTime;
        tankX[altTank] += sin(tankA[altTank]) * tankV[altTank] * deltaTime;
        tankY[altTank] -= cos(tankA[altTank]) * tankV[altTank] * deltaTime;
        if (tankX[altTank] < TANK_HIT_RADIUS)
          tankX[altTank] = TANK_HIT_RADIUS;
        if (tankY[altTank] < TANK_HIT_RADIUS)
          tankY[altTank] = TANK_HIT_RADIUS;
        if (tankX[altTank] > width - TANK_HIT_RADIUS)
          tankX[altTank] = width - TANK_HIT_RADIUS;
        if (tankY[altTank] > height - TANK_HIT_RADIUS)
          tankY[altTank] = height - TANK_HIT_RADIUS;
        // ending here no display position will be used
      }

      // engine noise while one of our tanks is rolling
      bool moving =
          abs(tankV[myTank]) > 20.f || (!onlineMode && abs(tankV[1]) > 20.f);
      if (moving && moveSnd.getStatus() != SoundSource::Status::Playing)
        moveSnd.play();
      else if (!moving && moveSnd.getStatus() == SoundSource::Status::Playing)
        moveSnd.pause();

      // tank-rock collision
      for (int i = 0; i <= upperTank; i++) {
        for (int r = 0; r < 10; r++) {
          float dx = tankX[i] - rocks[r].x; // tank collisions
          float dy = tankY[i] - rocks[r].y;
          float d = dx * dx + dy * dy; // square distance
          float minD = TANK_HIT_RADIUS + ROCK_HIT_RADIUS;
          if (d < minD * minD && d > 0.0001f) {
            float dist = sqrt(d);
            float overlap = minD - dist;
            tankX[i] += (dx / dist) * overlap;
            tankY[i] += (dy / dist) * overlap;
          }
        }
      }

      // tank-tank collision, tanks that bump into each other get pushed apart
      for (int i = 0; i <= upperTank; i++) {
        for (int j = i + 1; j <= upperTank; j++) {
          float dx = tankX[j] - tankX[i];
          float dy = tankY[j] - tankY[i];
          float d = dx * dx + dy * dy;
          float minD = TANK_HIT_RADIUS * 2.f;
          if (d < minD * minD && d > 0.0001f) {
            float dist = sqrt(d);
            float half = (minD - dist) / 2.f;
            tankX[i] -= (dx / dist) * half;
            tankY[i] -= (dy / dist) * half;
            tankX[j] += (dx / dist) * half;
            tankY[j] += (dy / dist) * half;
          }
        }
      }

      // tire tracks, a mark under each tread every few pixels driven
      for (int i = 0; i <= upperTank; i++) {
        float dx = tankX[i] - tankLastX[i];
        float dy = tankY[i] - tankLastY[i];
        if (dx * dx + dy * dy < TRACK_SPACING * TRACK_SPACING)
          continue;
        tankLastX[i] = tankX[i];
        tankLastY[i] = tankY[i];
        if (tankX[i] < 0.f)
          continue; // a tank that left the match doesn't leave marks
        for (int side = -1; side <= 1; side += 2) {
          trackX[nextTrack] = tankLX[i] + cos(tankA[i]) * 13.f * side;
          trackY[nextTrack] = tankLY[i] + sin(tankA[i]) * 13.f * side;
          trackA[nextTrack] = tankLA[i];
          trackLife[nextTrack] = TRACK_LIFE;
          nextTrack = (nextTrack + 1) % MAX_TRACKS;
        }
      }
      for (int i = 0; i < MAX_TRACKS; i++)
        if (trackLife[i] > 0.f)
          trackLife[i] -= deltaTime;
      for (int i = 0; i < MAX_SCORCH; i++)
        if (scorchLife[i] > 0.f)
          scorchLife[i] -= deltaTime;

      // tank ammo and cooldowns
      for (int i = 0; i <= upperTank; i++) {
        if (tankInv[i] > 0)
          tankInv[i]--;
        // power up timers run out here (the host's numbers win on clients)
        if (tankShield[i] > 0.f)
          tankShield[i] -= deltaTime;
        if (tankBoost[i] > 0.f)
          tankBoost[i] -= deltaTime;
        if (tankShotsTime[i] > 0.f) {
          tankShotsTime[i] -= deltaTime;
          if (tankShotsTime[i] <= 0.f)
            tankShots[i] = 1;
        }
        // clients only run ammo for their own tank, the rest comes in
        // snapshots
        if (!isHost && i != myTank)
          continue;
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
      timeLeft -= deltaTime;
      if (timeLeft <= 0.f) {
        timeLeft = 0.f;
        layer = 2;
        moveSnd.pause();
      }

      // power ups, the host decides what shows up and who picks it up
      if (isHost) {
        powerTimer -= deltaTime;
        if (powerTimer <= 0.f) {
          powerTimer = POWERUP_INTERVAL;
          for (int k = 0; k < MAX_POWERUPS; k++) {
            if (powerType[k])
              continue; // slot is taken
            for (int tries = 0; tries < 20; tries++) {
              float px = 60.f + rand() % int(width - 120);
              float py = 60.f + rand() % int(height - 120);
              bool spotFree = true; // keep them off the rocks
              for (int r = 0; r < 10; r++) {
                float dx = px - rocks[r].x, dy = py - rocks[r].y;
                if (dx * dx + dy * dy < 70.f * 70.f)
                  spotFree = false;
              }
              if (spotFree) {
                powerX[k] = px;
                powerY[k] = py;
                powerType[k] = 1 + rand() % 4;
                break;
              }
            }
            break; // one new power up at a time
          }
        }
        for (int i = 0; i <= upperTank; i++) {
          for (int k = 0; k < MAX_POWERUPS; k++) {
            if (!powerType[k])
              continue;
            float dx = tankX[i] - powerX[k], dy = tankY[i] - powerY[k];
            float pr = TANK_HIT_RADIUS + 14.f;
            if (dx * dx + dy * dy > pr * pr)
              continue;
            switch (powerType[k]) {
            case 1:
              tankShield[i] = POWERUP_TIME;
              break;
            case 2:
              tankBoost[i] = POWERUP_TIME;
              break;
            case 3:
              tankShots[i] = 2;
              tankShotsTime[i] = POWERUP_TIME;
              break;
            case 4:
              tankShots[i] = 3;
              tankShotsTime[i] = POWERUP_TIME;
              break;
            }
            powerType[k] = 0; // picked up
          }
        }
      }

      // tank position interpolation
      for (int i = 0; i < tanks; i++) {
        bool isLocal =
            (i == myTank) || (onlineMode && tankDevice[i] == deviceId);
        if (!isLocal) {
          float lerpFactor = 12.f * deltaTime; // convergence factor
          if (lerpFactor > 1.f)
            lerpFactor = 1.f;
          tankLX[i] += (tankX[i] - tankLX[i]) * lerpFactor;
          tankLY[i] += (tankY[i] - tankLY[i]) * lerpFactor;
          tankLA[i] += (tankA[i] - tankLA[i]) * lerpFactor;
        } else {
          tankLX[i] = tankX[i];
          tankLY[i] = tankY[i];
          tankLA[i] = tankA[i];
        }
      }

      // MOVEMENT AND COLLISION DETECTION
      for (int i = lowerBall; isHost && i <= upperBall; i++) {
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

        for (int r = 0; r < 10; r++) {
          float dx = ballsX[i] - rocks[r].x;
          float dy = ballsY[i] - rocks[r].y;
          float d = dx * dx + dy * dy;
          float minD = ballRadius + ROCK_HIT_RADIUS;

          if (d < minD * minD && d > 0.0001f) {
            float dist = sqrt(d);
            float nx = dx / dist; // normal vector
            float ny = dy / dist;
            float dot =
                ballsVX[i] * nx +
                ballsVY[i] * ny; // dot of velocity and normal of collision
            ballsVX[i] -= 2.f * dot * nx;
            ballsVY[i] -= 2.f * dot * ny;
            float ovrlap = minD - dist;
            ballsX[i] += nx * ovrlap;
            ballsY[i] +=
                ny * ovrlap; // remove overlap to avoid balls being stuck
            ballState[i]++;
            ballsA[i] = atan2(-ballsVY[i], -ballsVX[i]);
          }
        }

        // tankCollide
        for (int j = lowerTank; j <= upperTank; j++) {
          if (isTankGreen[ballSource[i]] == isTankGreen[j])
            continue; // no friendly fire in online mode
          float dx = ballsX[i] - tankX[j];
          float dy = ballsY[i] - tankY[j];
          float distSquared = (dx * dx) + (dy * dy);
          float tRadius = TANK_HIT_RADIUS + ballRadius;
          if (distSquared <= (tRadius * tRadius)) {
            // invincible tanks and shielded tanks both bounce the ball away
            if (tankInv[j] > 0 || tankShield[j] > 0.f) {
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
              if (isTankGreen[ballSource[i]])
                greenKills++;
              else
                redKills++;
              tankDeaths[j]++;
              tankInv[j] = 149;
              // leave a burnt mark, flash and make some noise where it blew up
              scorchX[nextScorch] = tankX[j];
              scorchY[nextScorch] = tankY[j];
              scorchA[nextScorch] = (rand() % 628) / 100.f;
              scorchLife[nextScorch] = SCORCH_LIFE;
              nextScorch = (nextScorch + 1) % MAX_SCORCH;
              glare[nextGlare] = 10;
              glareX[nextGlare] = tankX[j];
              glareY[nextGlare] = tankY[j];
              nextGlare = (nextGlare + 1) % MAX_GLARES;
              glares++;
              hitSnd[hitIdx].play();
              hitIdx = (hitIdx + 1) % 2;
              // it loses its power ups and goes back to its spawn spot
              tankBoost[j] = tankShotsTime[j] = 0.f;
              tankShots[j] = 1;
              tankSpawn[j] = true;
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
    } else if (moveSnd.getStatus() == SoundSource::Status::Playing) {
      moveSnd.pause(); // paused or in a menu, engine goes quiet
    }

    // --- SPAWNING ---
    // the tank index decides the spawn spot: the four corners first, then
    // rings further in for more tanks. even indexes are green, odd are red.
    // used at the start of a match and whenever a tank dies
    for (int i = 0; i < tanks; i++) {
      if (!tankSpawn[i])
        continue;
      tankSpawn[i] = false;
      int corner = i % 4;
      float inset = 100.f + (i / 4.f) * 70.f;
      tankX[i] = (corner == 0 || corner == 3) ? inset : width - inset;
      tankY[i] = (corner == 0 || corner == 2) ? height - inset : inset;
      // face the middle of the arena, forward is (sin a, -cos a)
      tankA[i] = atan2(width / 2.f - tankX[i], tankY[i] - height / 2.f);
      tankV[i] = tankW[i] = 0.f;
      tankLastX[i] = tankLX[i] =
          tankX[i]; // so the jump doesn't draw a line of tracks
      tankLastY[i] = tankLY[i] = tankY[i];
    }

    // --- NETWORK: SENDING ---
    if (networkOk && onlineMode && netTimer >= NET_INTERVAL) {
      if (lobbyRole == 2 && (layer == 1 || layer == 6)) {
        // client: where our tank is, how many shots we fired and how many
        // times we died (so the host knows we saw the respawn)
        Packet p;
        p << U8(7) << deviceId << U8(shotSeq) << U16(tankDeaths[myTank])
          << tankX[myTank] << tankY[myTank] << tankA[myTank];
        if (socket.send(p, IpAddress(hostAddr), PORT) == Socket::Status::Done)
          ;

        // send state for secondary local tank if two players are on this device
        if (myTank + 1 < tanks && tankDevice[myTank + 1] == deviceId) {
          Packet p2;
          p2 << U8(7) << deviceId << U8(shotSeq) << U16(tankDeaths[myTank + 1])
             << tankX[myTank + 1] << tankY[myTank + 1] << tankA[myTank + 1];
          if (socket.send(p2, IpAddress(hostAddr), PORT) ==
              Socket::Status::Done)
            ;
        }
        netTimer = 0.f;
      } else if (lobbyRole == 1 && inMatch) {
        // host: the whole match in one go
        Packet p;
        p << U8(8) << deviceId << U8(layer == 2 ? 2 : 1) << timeLeft
          << U16(greenKills) << U16(redKills) << U8(tanks);
        for (int i = 0; i < tanks; i++)
          p << tankX[i] << tankY[i] << tankA[i] << U8(tankAmmo[i])
            << U8(tankInv[i]) << tankShield[i] << tankBoost[i]
            << U8(tankShots[i]) << tankShotsTime[i] << U16(tankDeaths[i]);
        for (int k = 0; k < MAX_POWERUPS; k++)
          p << U8(powerType[k]) << powerX[k] << powerY[k];
        int nb = 0;
        for (int k = lowerBall; k <= upperBall && nb < MAX_NET_BALLS; k++)
          if (ballState[k])
            nb++;
        p << U16(nb);
        int sent = 0;
        for (int k = lowerBall; k <= upperBall && sent < nb; k++) {
          if (!ballState[k])
            continue;
          p << I16(ballsX[k]) << I16(ballsY[k]);
          sent++;
        }
        for (int k = 0; k < MAX_GLARES; k++)
          p << U8(glare[k] > 0 ? glare[k] : 0) << I16(glareX[k])
            << I16(glareY[k]);
        for (int i = 1; i < tanks; i++)
          if (socket.send(p, IpAddress(tankIp[i]), PORT) ==
              Socket::Status::Done)
            ;
        netTimer = 0.f;
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

    meow window.clear(); // clear the window for next frame
    window.setView(window.getDefaultView());
    if (layer != 7)
      window.draw(bg);

    // --- RENDER ---
    // the game is drawn first, so the pause menu and the buttons can sit on
    // top of it
    if (layer == 1 || layer == 6) {
      window.setView(gameView);
      // tire tracks and explosion marks lie on the ground, under everything
      for (int i = 0; i < MAX_TRACKS; i++) {
        if (trackLife[i] <= 0.f)
          continue;
        trackSprite.setPosition({trackX[i], trackY[i]});
        trackSprite.setRotation(radians(trackA[i]));
        trackSprite.setColor(
            Color(255, 255, 255, int(trackLife[i] / TRACK_LIFE * 170.f)));
        window.draw(trackSprite);
      }
      for (int i = 0; i < MAX_SCORCH; i++) {
        if (scorchLife[i] <= 0.f)
          continue;
        scorchSprite.setPosition({scorchX[i], scorchY[i]});
        scorchSprite.setRotation(radians(scorchA[i]));
        float fade = scorchLife[i] / SCORCH_LIFE;
        scorchSprite.setColor(Color(255, 255, 255, int(fade * 220.f)));
        window.draw(scorchSprite);
      }
      for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerType[i])
          continue;
        float bob = 2.8f + sin(t * 0.08f + i) * 0.4f;
        powerSprite.setTextureRect(
            IntRect({0, (powerType[i] - 1) * 16}, {16, 16}));
        powerSprite.setScale({bob, bob});
        powerSprite.setPosition({powerX[i], powerY[i]});
        window.draw(powerSprite);
      }
      for (int i = lowerBall; i <= upperBall; i++) { // draw all cannon balls
        if (!ballState[i]) // if ballState is zero, skip
          continue;
        // cannonball.setTexture(bulletTexture[(t / 10 + i) % 4]);
        cannonball.setPosition({ballsX[i], ballsY[i]});
        cannonball.setRotation(radians(ballsA[i]));
        cannonball.setFillColor(isTankGreen[ballSource[i]]
                                    ? Color(10, 150, 10)
                                    : Color(150, 10, 10));
        window.draw(cannonball);
      }
      for (int i = lowerTank; i <= upperTank;
           i++) { // draw all green & red tanks
        // no interpol of local tank position
        if (i == myTank || !onlineMode) {
          tankLX[i] = tankX[i];
          tankLY[i] = tankY[i];
          tankLA[i] = tankA[i];
        }
        if (isTankGreen[i]) {
          // assign position and rotation from stored array to the sprites
          greenTank.setPosition({tankLX[i], tankLY[i]});
          greenTank.setRotation(radians(tankLA[i]));
          if (tankInv[i] > 0)
            greenTank.setColor(
                Color(255, 255, 255, (tankInv[i] / 10) % 2 ? 255 : 130));
          else
            greenTank.setColor(Color::White);
          // CircleShape c(TANK_HIT_RADIUS);
          // c.setOutlineColor(Color::White);
          // c.setOutlineThickness(1);
          // c.setPosition({tankLX[i], tankLY[i]});
          // FloatRect cB = c.getLocalBounds();
          // c.setOrigin({cB.position.x + cB.size.x / 2.f,
          //              cB.position.y + cB.size.y / 2.f}); // draw circle by
          //              CENTER
          // window.draw(c);
          window.draw(greenTank);
        } else {
          redTank.setPosition({tankLX[i], tankLY[i]});
          redTank.setRotation(radians(tankLA[i]));
          if (tankInv[i] > 0)
            redTank.setColor(
                Color(255, 255, 255, (tankInv[i] / 10) % 2 ? 255 : 130));
          else
            redTank.setColor(Color::White);
          window.draw(redTank);
        }
        // shield bubble, flickers when it's about to run out
        if (tankShield[i] > 0.f && (tankShield[i] > 2.f || (t / 6) % 2)) {
          shieldCircle.setPosition({tankLX[i], tankLY[i]});
          window.draw(shieldCircle);
        }
        // draw the ammobar only for our tank
        // TODO
        if (i == myTank || !onlineMode) {
          ammoBar.setTexture(ammoT[tankAmmo[i] % 6]);
          ammoBar.setPosition({tankLX[i], tankLY[i] + 40.f});
          ammoBar.setColor(
              Color(255, 255, 255, tankCooldown[i] > 0 ? 123 : 255));
          window.draw(ammoBar);
        }
        // little icons above the tank for the power ups it has right now
        int iconSlot = 0;
        for (int p = 0; p < 3; p++) {
          bool has = (p == 0 && tankShield[i] > 0.f) ||
                     (p == 1 && tankBoost[i] > 0.f) ||
                     (p == 2 && tankShots[i] > 1);
          if (!has)
            continue;
          int icon = p == 0 ? 0 : (p == 1 ? 1 : tankShots[i]); // 0, 1, 2 or 3
          powerSprite.setTextureRect(IntRect({0, icon * 16}, {16, 16}));
          powerSprite.setScale({1.4f, 1.4f});
          powerSprite.setPosition(
              {tankLX[i] - 22.f + iconSlot * 22.f, tankLY[i] - 44.f});
          window.draw(powerSprite);
          iconSlot++;
        }
        if (onlineMode) { // names above the tanks when there's more than us
          infoText.setCharacterSize(16);
          infoText.setFillColor(Color::White);
          infoText.setString(tankName[i]);
          FloatRect nb = infoText.getLocalBounds();
          infoText.setOrigin(
              {nb.position.x + nb.size.x / 2.f, nb.position.y + nb.size.y});
          infoText.setPosition({tankLX[i], tankLY[i] - 50.f});
          window.draw(infoText);
        }
      }
      for (int i = 0; i < 10; i++) {
        Sprite rockRef = i % 3 == 0 ? rock1 : (i % 3 == 1 ? rock2 : rock3);
        rockRef.setPosition(rocks[i]);
        window.draw(rockRef);
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
      window.draw(fpsText); // show framerate
      // show time
      int sec = static_cast<int>(timeLeft) % 60;
      int min = int(timeLeft / 60);
      timeText.setString((min < 10 ? "0" : "") + to_string(min) + ":" +
                         (sec < 10 ? "0" : "") + to_string(sec));
      FloatRect timeBounds = timeText.getLocalBounds();
      timeText.setOrigin({timeBounds.size.x / 2.f, timeBounds.size.y / 2.f});
      window.draw(timeText);
      // show wins
      greenText.setString(to_string(greenKills));
      redText.setString(to_string(redKills));
      redText.setOrigin({redText.getLocalBounds().size.x, 0.f}); // right align
      window.draw(greenText);
      window.draw(redText);

      // back to our own screen's coordinates for everything menu-like
      window.setView(window.getDefaultView());
      if (touchMode && layer == 1) { // on screen buttons
        for (int i = 0; i < 6; i++) {
          touchBtn.setPosition(touchRects[i].position);
          touchBtn.setSize(touchRects[i].size);
          window.draw(touchBtn);
        }
      }
      if (layer == 6) { // pause: dim the game and say so
        window.draw(dim);
        titleText.setString(onlineMode ? "Menu (match keeps running)"
                                       : "Paused");
        FloatRect pb = titleText.getLocalBounds();
        titleText.setOrigin(
            {pb.position.x + pb.size.x / 2.f, pb.position.y + pb.size.y / 2.f});
        titleText.setPosition({baseW / 2.f, baseH / 2.f - 110.f});
        window.draw(titleText);
      }
    }

    // BUTTONS
    for (int bi = 0; bi < buttons; bi++) {
      // which buttons show up depends on the layer, and for a few of them on
      // the state of the lobby
      bool shown = buttonLayer[bi] == layer;
      if (bi == 5)
        shown =
            layer == 2 && (!onlineMode || isHost); // only the host rematches
      if (bi == 7)
        shown = layer == 3 && lobbyRole == 1 && tanks >= 2;
      if (bi == 9 || bi == 10) // invites can show up in the menu too
        shown =
            (layer == 0 || layer == 3) && inviteTimer > 0.f && lobbyRole == 0;
      if (bi >= 20)
        shown = layer == 3 && lobbyRole != 2 && networkOk && (bi - 20) < peers;
      if (bi == 1 && !networkOk)
        shown = false; // no multiplayer without a socket
      if (shown) {     // draw buttons that are on the current layer
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
        // every button looks the same, so the colours are set once up here
        button.setFillColor(hovered ? fillHovered : fillNormal);
        button.setOutlineColor(hovered ? outlineHovered : outlineNormal);
        buttonText.setFillColor(hovered ? textHovered : textNormal);
        bool clicked = mouseJustPressed && hovered;
        switch (bi) {
        case 0:
          buttonText.setString("Play");
          if (clicked) {
            // offline match, the setup itself happens in the match start step
            onlineMode = false;
            isHost = true;
            myTank = 0;
            startMatch = true;
            mouseJustPressed = false;
          }
          break;
        case 1:
          buttonText.setString("Multiplayer");
          if (clicked) {
            layer = 3;
            mouseJustPressed = false;
          }
          break;
        case 2:
          buttonText.setString("Settings");
          if (clicked) {
            layer = 4;
            mouseJustPressed = false;
          }
          break;
        case 3:
          buttonText.setString("Credits");
          if (clicked) {
            layer = 5;
            mouseJustPressed = false;
          }
          break;
        case 4:
          buttonText.setString("Quit");
          if (clicked) {
            window.close();
            mouseJustPressed = false;
          }
          break;
        case 5: // win screen: another round
          buttonText.setString("Play Again");
          if (clicked) {
            if (onlineMode) // same players, the host tells everyone
              sendStart = true;
            startMatch = true;
            mouseJustPressed = false;
          }
          break;
        case 6:
          buttonText.setString("Main Menu");
          if (clicked) {
            layer = 0;
            leaveNet = true;
            mouseJustPressed = false;
          }
          break;
        case 7: // start the lobby's match
          buttonText.setString("Start Match");
          if (clicked) {
            onlineMode = true;
            isHost = true;
            myTank = 0;
            sendStart = true;
            startMatch = true;
            mouseJustPressed = false;
          }
          break;
        case 8:
          buttonText.setString(lobbyRole == 0 ? "Back" : "Leave");
          if (clicked) {
            if (lobbyRole != 0)
              leaveNet = true;
            layer = 0;
            mouseJustPressed = false;
          }
          break;
        case 9: // accept an invite
          buttonText.setString("Accept");
          if (clicked) {
            if (inviteHosting) {
              // they already host a lobby, so we join it as a client
              pendingId = inviteId;
              pendingTimer = 10.f;
              Packet p;
              p << U8(4) << deviceId << playerName;
              if (socket.send(p, IpAddress(inviteIp), PORT) ==
                  Socket::Status::Done)
                ;
            } else {
              // the person who sent the invite becomes the host.
              // joining user connects as a client
              lobbyRole = 2;
              isHost = false;
              hostAddr = inviteIp;
              hostSeen = 0.f;
              pendingId = inviteId;
              pendingTimer = 10.f;

              Packet p;
              p << U8(4) << deviceId << playerName; // send join request to host
              if (socket.send(p, IpAddress(inviteIp), PORT) ==
                  Socket::Status::Done)
                ;

              inviteTimer = 0.f;
              layer = 3;
              mouseJustPressed = false;
            }
          }
          break;
        case 10:
          buttonText.setString("Decline");
          if (clicked) {
            inviteTimer = 0.f;
            mouseJustPressed = false;
          }
          break;
        case 11:
        case 12:
          buttonText.setString(bi == 11 ? "-" : "+");
          if (clicked) {
            matchTime += bi == 11 ? -10.f : 10.f;
            if (matchTime < 10.f)
              matchTime = 10.f;
            if (matchTime > 600.f)
              matchTime = 600.f;
            mouseJustPressed = false;
          }
          break;
        case 13:
        case 14:
          buttonText.setString(bi == 13 ? "-" : "+");
          if (clicked) {
            volume += bi == 13 ? -10.f : 10.f;
            if (volume < 0.f)
              volume = 0.f;
            if (volume > 100.f)
              volume = 100.f;
            Listener::setGlobalVolume(volume);
            mouseJustPressed = false;
          }
          break;
        case 15:
        case 17:
          buttonText.setString("Back");
          if (clicked) {
            typingName = false;
            layer = 0;
            mouseJustPressed = false;
          }
          break;
        case 16: // the name box, click it and type
          buttonText.setString(playerName +
                               (typingName && (t / 30) % 2 ? "_" : ""));
          if (clicked) {
            typingName = !typingName;
            mouseJustPressed = false;
          }
          break;
        case 18:
          buttonText.setString("Resume");
          if (clicked) {
            layer = 1;
            mouseJustPressed = false;
          }
          break;
        case 19:
          buttonText.setString("Main Menu");
          if (clicked) {
            layer = 0;
            leaveNet = true;
            mouseJustPressed = false;
          }
          break;
        case 26:
          buttonText.setString(playerName +
                               (typingName && (t / 30) % 2 ? "_" : ""));
          if (clicked) {
            typingName = !typingName;
            mouseJustPressed = false;
          }
          break;
        case 27:
        case 28:
          buttonText.setString(bi == 27 ? "-" : "+");
          if (clicked && lobbyRole == 1) {
            matchTime += bi == 27 ? -10.f : 10.f;
            if (matchTime < 10.f)
              matchTime = 10.f;
            if (matchTime > 600.f)
              matchTime = 600.f;
            mouseJustPressed = false;
          }
          break;
        default: // 20 and up: devices found on the network
          if (bi >= 20) {
            int p = bi - 20;
            buttonText.setString(
                peerName[p] +
                (peerStatus[p] == 0
                     ? " - invite"
                     : (peerStatus[p] == 1
                            ? " - join (" + to_string(peerCount[p]) + ")"
                            : " - in a match")));
            if (clicked && peerStatus[p] != 2) {
              pendingId = peerId[p];
              pendingTimer = 10.f;
              Packet pk;
              // idle device: invite it. device with an open lobby: ask to
              // join
              if (peerStatus[p] == 0) {
                lobbyRole = 1;
                isHost = true;
                tanks = 1;
                tankDevice[0] = deviceId;
                tankName[0] = playerName;

                pk << U8(2) << deviceId << playerName << U8(1);
              } else {
                pk << U8(4) << deviceId << playerName;
              }
              if (socket.send(pk, IpAddress(peerIp[p]), PORT) ==
                  Socket::Status::Done)
                ;
              mouseJustPressed = false;
            }
          }
          break;
        }
        meow FloatRect bounds =
            buttonText.getLocalBounds(); // get size of rndered text
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

    // --- MENU TEXTS ---
    if (layer == 0 || layer == 3) { // an invite waiting for an answer
      if (inviteTimer > 0.f && lobbyRole == 0) {
        infoText.setCharacterSize(26);
        infoText.setFillColor(Color::White);
        infoText.setString(inviteName + (inviteHosting
                                             ? " wants you to join their match"
                                             : " invited you to play"));
        FloatRect ib = infoText.getLocalBounds();
        infoText.setOrigin(
            {ib.position.x + ib.size.x / 2.f, ib.position.y + ib.size.y / 2.f});
        infoText.setPosition({baseW / 2.f, 90.f});
        window.draw(infoText);
      }
    }
    if (layer == 0) {
      titleText.setString("TankMaster");
      FloatRect tb = titleText.getLocalBounds();
      titleText.setOrigin(
          {tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f});
      titleText.setPosition({baseW / 2.f, baseH / 2.f - 130.f});
      window.draw(titleText);
    }
    if (layer == 3) { // multiplayer menu
      string head;
      if (!networkOk)
        head = "network is unavailable (is port 6767 already in use?)";
      else if (lobbyRole == 1)
        head = "you are hosting, invite someone or wait for them to join";
      else if (lobbyRole == 2)
        head = "joined! waiting for the host to start the match";
      else if (pendingTimer > 0.f)
        head = "waiting for an answer...";
      else
        head = peers > 0 ? "click a player to invite or join them"
                         : "looking for players on your network...";
      infoText.setCharacterSize(28);
      infoText.setFillColor(Color::White);
      infoText.setString(head);
      FloatRect hb = infoText.getLocalBounds();
      infoText.setOrigin(
          {hb.position.x + hb.size.x / 2.f, hb.position.y + hb.size.y / 2.f});
      infoText.setPosition({baseW / 2.f, 50.f});
      window.draw(infoText);
      infoText.setOrigin({0.f, 0.f}); // players in the lobby down the left
      infoText.setCharacterSize(24);
      if (lobbyRole != 0) {
        for (int i = 0; i < tanks; i++) {
          infoText.setString(to_string(i + 1) + ". " + tankName[i] +
                             (i == 0 ? " (host)" : ""));
          infoText.setPosition({40.f, 190.f + i * 36.f});
          window.draw(infoText);
        }
      }
      infoText.setCharacterSize(20); // who we are
      infoText.setString("you are: " + playerName);
      infoText.setPosition({40.f, baseH - 60.f});
      window.draw(infoText);
      infoText.setString("packets recieved: " + to_string(packetsRec));
      infoText.setPosition({40.f, baseH - 90.f});
      window.draw(infoText);
      if (lobbyRole == 1) {
        infoText.setString("Time: " + to_string(int(matchTime)) + "s");
        infoText.setPosition({float(width - 260), float(height - 115)});
        window.draw(infoText);
      }
    }
    if (layer == 4) { // settings
      infoText.setCharacterSize(24);
      infoText.setFillColor(Color::White);
      string lines[3] = {"your name",
                         "match time: " + to_string(int(matchTime)) + "s",
                         "volume: " + to_string(int(volume))};
      for (int i = 0; i < 3; i++) {
        infoText.setString(lines[i]);
        FloatRect lb = infoText.getLocalBounds();
        infoText.setOrigin(
            {lb.position.x + lb.size.x / 2.f, lb.position.y + lb.size.y / 2.f});
        // the name label float above its box
        infoText.setPosition(
            {baseW / 2.f,
             i == 0 ? sy - 25.f
                    : sy + 70.f * i + 20.f}); // ts tweaking is so hard omg
        window.draw(infoText);
      }
    }
    if (layer == 5) { // credits
      string credits[9] = {"TankMaster",
                           "a 2d tank shooter game",
                           "made with C++ and SFML, by",
                           "",
                           "Muhammad Ali",
                           "Muhammad Hamza",
                           "Amaz al Muqtadir",
                           "",
                           "Thanks for playing :3c"};
      for (int i = 0; i < 9; i++) {
        infoText.setCharacterSize(i == 0 ? 48 : 28);
        infoText.setFillColor(Color::White);
        infoText.setString(credits[i]);
        FloatRect cb = infoText.getLocalBounds();
        infoText.setOrigin(
            {cb.position.x + cb.size.x / 2.f, cb.position.y + cb.size.y / 2.f});
        infoText.setPosition({baseW / 2.f, 120.f + i * 50.f});
        window.draw(infoText);
      }
    }
    if (layer == 7) { // intro: logo and title fade in, hold, fade out
      float a = 1.f;
      if (introTime < 1.f)
        a = introTime;
      else if (introTime > 2.5f)
        a = 3.5f - introTime;
      if (a < 0.f)
        a = 0.f;
      if (a > 1.f)
        a = 1.f;
      logoSprite.setColor(Color(255, 255, 255, int(a * 255.f)));
      window.draw(logoSprite);
      titleText.setString("Alimad Interactive Entertainment");
      titleText.setFillColor(Color(255, 255, 255, int(a * 255.f)));
      titleText.setOutlineColor(Color(0, 0, 0, int(a * 255.f)));
      FloatRect tb = titleText.getLocalBounds();
      titleText.setOrigin(
          {tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f});
      titleText.setPosition({baseW / 2.f, baseH / 2.f + baseH / 6.f + 30.f});
      window.draw(titleText);
      titleText.setFillColor(Color::White); // back to normal for the menus
      titleText.setOutlineColor(Color::Black);
    }
    if (layer == 2) {
      if (redKills > greenKills) {
        redText.setCharacterSize(64);
        redText.setString("Red won with " + to_string(redKills) + " kills!");
        FloatRect w = redText.getLocalBounds();
        redText.setOrigin({w.size.x / 2, w.size.y / 2});
        redText.setPosition({baseW / 2.f, baseH * 1.f / 3.f});
        window.draw(redText);
      } else if (redKills < greenKills) {
        greenText.setCharacterSize(64);
        greenText.setString("Green won with " + to_string(greenKills) +
                            " kills!");
        FloatRect w = greenText.getLocalBounds();
        greenText.setOrigin({w.size.x / 2, w.size.y / 2});
        greenText.setPosition({baseW / 2.f, baseH * 1.f / 3.f});
        window.draw(greenText);
      } else {
        timeText.setCharacterSize(64);
        timeText.setString("Draw between both sides with " +
                           to_string(redKills) + " kills!");
        FloatRect w = timeText.getLocalBounds();
        timeText.setOrigin({w.size.x / 2, w.size.y / 2});
        timeText.setPosition({baseW / 2.f, baseH * 1.f / 3.f});
        window.draw(timeText);
      }
    }
    window.display(); // push the draw buffer to the view
    t++;              // tick a frame
  }

  // in case of closed window broadcast leave msg
  if (networkOk && lobbyRole == 1) {
    for (int i = 1; i < tanks; i++) {
      Packet p;
      p << U8(9) << deviceId;
      if (socket.send(p, IpAddress(tankIp[i]), PORT) == Socket::Status::Done)
        ;
    }
  } else if (networkOk && lobbyRole == 2) {
    Packet p;
    p << U8(9) << deviceId;
    if (socket.send(p, IpAddress(hostAddr), PORT) == Socket::Status::Done)
      ;
  }

  meow purrrr
      // :3c
      return 0;
}

// end of file :P
// thanks for reading the entire code P:
// yes the meows and murrrps and purrrs were necessary
