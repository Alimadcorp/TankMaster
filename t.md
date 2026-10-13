Here are the step-by-step exact code changes required for each of your requests.

---

### Step 1: Two Players Same Device Online, In-Lobby Username Edits, & Host Match Time Control

#### 1.1 Support 2 Local Tanks in Online Mode

To allow two local players (Green tank `myTank` and Red tank `myTank + 1`) to participate online from a single device, we need to adapt input handling and movement logic when `onlineMode` is active.

**Find in `main.cpp` (Key Released Event Handling):**

```cpp
        if (onlineMode) { // in onlineMode, only shoot from our tank
          if (layer == 1 && (k == Keyboard::Key::W || k == Keyboard::Up) &&
              tankCooldown[myTank] <= 0.f && tankAmmo[myTank] > 0) {
            tankShoot[myTank] = true;
            tankCooldown[myTank] = TANK_COOLDOWN;
            tankAmmo[myTank]--;
          }
        } else {

```

**Replace with:**

```cpp
        if (onlineMode) {
          // Green tank (local primary)
          if (layer == 1 && k == Keyboard::Key::W &&
              tankCooldown[myTank] <= 0.f && tankAmmo[myTank] > 0) {
            tankShoot[myTank] = true;
            tankCooldown[myTank] = TANK_COOLDOWN;
            tankAmmo[myTank]--;
          }
          // Red tank (local secondary on same device, if assigned)
          else if (layer == 1 && (myTank + 1 < tanks) && tankDevice[myTank + 1] == deviceId &&
                   k == Keyboard::Key::Up && tankCooldown[myTank + 1] <= 0.f && tankAmmo[myTank + 1] > 0) {
            tankShoot[myTank + 1] = true;
            tankCooldown[myTank + 1] = TANK_COOLDOWN;
            tankAmmo[myTank + 1]--;
          }
        } else {

```

**Find in `main.cpp` (Tank Movement logic):**

```cpp
      if (onlineMode)
        velGreen += velRed;

      // the keys now say what speed we WANT, the tank ramps up (or down) to
      // it using the acceleration arrays tankV and tankW
      float boostG = tankBoost[myTank] > 0.f ? BOOST_MULT : 1.f;
      float wantV = velGreen.y * TANK_SPEED * boostG;

```

**Replace with:**

```cpp
      if (onlineMode && (myTank + 1 >= tanks || tankDevice[myTank + 1] != deviceId)) {
        velGreen += velRed; // Single local player using either control set
      }

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
      tankX[myTank] += sin(tankA[myTank]) * tankV[myTank] * deltaTime;
      tankY[myTank] -= cos(tankA[myTank]) * tankV[myTank] * deltaTime;
      if (tankX[myTank] < TANK_HIT_RADIUS) tankX[myTank] = TANK_HIT_RADIUS;
      if (tankY[myTank] < TANK_HIT_RADIUS) tankY[myTank] = TANK_HIT_RADIUS;
      if (tankX[myTank] > width - TANK_HIT_RADIUS) tankX[myTank] = width - TANK_HIT_RADIUS;
      if (tankY[myTank] > height - TANK_HIT_RADIUS) tankY[myTank] = height - TANK_HIT_RADIUS;

      // Update secondary local player tank on same device if present online
      if (onlineMode && (myTank + 1 < tanks) && tankDevice[myTank + 1] == deviceId) {
        unsigned short altTank = myTank + 1;
        float boostR = tankBoost[altTank] > 0.f ? BOOST_MULT : 1.f;
        float wantVR = velRed.y * TANK_SPEED * boostR;
        float wantWR = velRed.x * TANK_ROTATION_SPEED;
        if (tankV[altTank] < wantVR - stepV) tankV[altTank] += stepV;
        else if (tankV[altTank] > wantVR + stepV) tankV[altTank] -= stepV;
        else tankV[altTank] = wantVR;

        if (tankW[altTank] < wantWR - stepW) tankW[altTank] += stepW;
        else if (tankW[altTank] > wantWR + stepW) tankW[altTank] -= stepW;
        else tankW[altTank] = wantWR;

        tankA[altTank] += tankW[altTank] * deltaTime;
        tankX[altTank] += sin(tankA[altTank]) * tankV[altTank] * deltaTime;
        tankY[altTank] -= cos(tankA[altTank]) * tankV[altTank] * deltaTime;
        if (tankX[altTank] < TANK_HIT_RADIUS) tankX[altTank] = TANK_HIT_RADIUS;
        if (tankY[altTank] < TANK_HIT_RADIUS) tankY[altTank] = TANK_HIT_RADIUS;
        if (tankX[altTank] > width - TANK_HIT_RADIUS) tankX[altTank] = width - TANK_HIT_RADIUS;
        if (tankY[altTank] > height - TANK_HIT_RADIUS) tankY[altTank] = height - TANK_HIT_RADIUS;
      }

```

**Find in `main.cpp` (Client Network Sending Packet Type 7):**

```cpp
    if (networkOk && onlineMode && netTimer >= NET_INTERVAL) {
      if (lobbyRole == 2 && (layer == 1 || layer == 6)) {
        // client: where our tank is, how many shots we fired and how many
        // times we died (so the host knows we saw the respawn)
        Packet p;
        p << U8(7) << deviceId << U8(shotSeq) << U16(tankDeaths[myTank])
          << tankX[myTank] << tankY[myTank] << tankA[myTank];
        if (socket.send(p, IpAddress(hostAddr), PORT) == Socket::Status::Done)
          ;
        netTimer = 0.f;
      }

```

**Replace with:**

```cpp
    if (networkOk && onlineMode && netTimer >= NET_INTERVAL) {
      if (lobbyRole == 2 && (layer == 1 || layer == 6)) {
        // Send state for primary local tank
        Packet p;
        p << U8(7) << deviceId << U8(shotSeq) << U16(tankDeaths[myTank])
          << tankX[myTank] << tankY[myTank] << tankA[myTank];
        if (socket.send(p, IpAddress(hostAddr), PORT) == Socket::Status::Done);

        // Send state for secondary local tank if two players are on this device
        if (myTank + 1 < tanks && tankDevice[myTank + 1] == deviceId) {
          Packet p2;
          p2 << U8(7) << deviceId << U8(shotSeq) << U16(tankDeaths[myTank + 1])
             << tankX[myTank + 1] << tankY[myTank + 1] << tankA[myTank + 1];
          if (socket.send(p2, IpAddress(hostAddr), PORT) == Socket::Status::Done);
        }
        netTimer = 0.f;
      }

```

---

#### 1.2 Changing Username in Multiplayer Screen & Match Time Control for Host

Add interactive buttons on `layer 3` (Multiplayer Menu) for name editing and match duration adjustments.

**Find button initialization section:**

```cpp
  // 20 to 25 network devices
  for (int b = 20; b < 20 + MAX_PEERS; b++) {
    buttonLayer[b] = 3;
    buttonW[b] = 360;
    buttonX[b] = width / 2 - 180;
    buttonY[b] = 190 + (b - 20) * 46;
  }

```

**Add after it:**

```cpp
  // 26: Name box in lobby, 27-28: Match time buttons for host
  buttonLayer[26] = 3;
  buttonW[26] = 200;
  buttonH[26] = 35;
  buttonX[26] = 40;
  buttonY[26] = height - 120;

  buttonLayer[27] = 3; // Time minus
  buttonLayer[28] = 3; // Time plus
  buttonW[27] = buttonW[28] = 35;
  buttonH[27] = buttonH[28] = 35;
  buttonX[27] = width - 150;
  buttonX[28] = width - 70;
  buttonY[27] = buttonY[28] = height - 120;

  buttons = 29;

```

**In `TextEntered` Event Handler, update layer check:**

```cpp
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

```

**Inside the Button `switch (bi)` block, add cases 26, 27, and 28:**

```cpp
        case 26: // Name Box in Lobby
          buttonText.setString(playerName + (typingName && (t / 30) % 2 ? "_" : ""));
          if (clicked) {
            typingName = !typingName;
            mouseJustPressed = false;
          }
          break;
        case 27:
        case 28: // Host Match Time Adjust
          buttonText.setString(bi == 27 ? "-" : "+");
          if (clicked && lobbyRole == 1) {
            matchTime += bi == 27 ? -10.f : 10.f;
            if (matchTime < 10.f) matchTime = 10.f;
            if (matchTime > 600.f) matchTime = 600.f;
            mouseJustPressed = false;
          }
          break;

```

**Inside `layer == 3` UI rendering section, draw time control labels:**

```cpp
      if (lobbyRole == 1) {
        infoText.setString("Time: " + to_string(int(matchTime)) + "s");
        infoText.setPosition({float(width - 260), float(height - 115)});
        window.draw(infoText);
      }

```
