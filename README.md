# TankMaster

### A local multiplayer game made in C++ SFML!

Chat this is the first time i'm usign SFML, in fact, this is the first time i'm moving ahead from "console applications" in C! I've been planning to expand my C skillset for a long time, but only today do I start :3

This was basically an assignment posed to us by our university, but a great chance to learn too! I'm probably gonna overshoot to more than what's required by them hehe

## Play

You can download and play this game at https://alimad.itch.io/tankmaster

## Build & Run

Here are the steps on how to build and run this project

1. Clone the repository and open it.
1. Install CMake, gcc or any C++ version 17+ compiler, and set it up with SFML. Read complete guide in the getting started secion here: https://sfml-dev.org/tutorials/3.1 
1. In case of **Linux**, run `chmod +x run.sh` to allow the file to be executed
1. **Run `./run.sh` on Linux or open `run.bat` on Windows to build and run the file**, this is assuming you have gone with the `gcc` path. In case of any other compiler, remember to include the following flags while running:
`-lGL -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lsfml-audio`  

## Proposal as required in assignment

**Project Title**: TankMaster

**Group Members**:

- 26L-0732  Muhammad Ali
- 26L-0729  Muhammad Hamza
- 26L-0725  Amaz al Muqtadir

**Introduction**:

The project aims to develop a local multiplayer tank shooter game. A tank shooter in which each player controls their tank, and fires their cannon. The player to get most kills in a certain time wins!

**Mechanic**:

A tank can shoot a cannonball from it's ammo, which has a size of 5, and refills slowly after the ammo is over. There is a minimum delay between each shot. The shot cannonballs can reflect from the walls, can collide with each other and explode, can reflect from obstacles, or can collide with a tank and cause the other player to score. Each match has a timer set by the user, the player to do the most kills in the given time wins. In case of more than 2 players, players are divided into red and green teams. Friendly fire is disabled, and the team to get most kills wins!

**Objectives**:
- Apply the concepts of programming and math to create the core mechanics including cannonballs, tank movements, collision, and more.
- Apply the concepts of data and networking to serialize and send data accross a network to keep clients in sync.
- Engage the users in amazing deathmatches, provide them with enjoyment and have them keep playing for the maximum possible time.
- Learn to use all SFML modules, from window to graphics and sound to network.

**Features and Functionalities**:
- Players can make moves by using WASD or Arrow keys, in case of network multiplayer, both map to their tank, but in case of offline, WASD controls the green tank, arrow keys control the red tank.
- Realtime collision detection, optimization of collision detection, management of large arrays of positions and velocities of objects, and rendering of those objects in the right order.
- Data serialization and local network multiplayer using a self-made protocol that runs on port 6767. This protocol follows the decentralized peer-to-peer architecture, where initially, every device is broadcasting and listening for other devices, as soon as a device invites another to a match, the one that accepts would become the server. The other device would stop broadcasting, though, any other devices than these two on the network can be invited by the server, or can request the server to join the match, upon joining which, the devices become clients too.
- Win conditions and winning message, main menu screen, settings screen.
- Allow players to choose a username.
- Provide a pause menu and option to resume the game or exit to main menu, or provide further options. The gameplay doesn't pause in case of multiplayer.
- Management and dynamic scaling of window in case of WindowResize events.

**Limitations**:
- Use of functions is not allowed
- Use of arrays greater than 1D is not allowed
- Use of more than one file is impossible as functions are not allowed
- Use of custom classes or data structures is not allowed
- Use of any library except SFML/*.hpp is not allowed

**Use of AI**:

In all good faith, I declare that no part of the code present in this repository was written by AI. It was either written by me, or any of my two team members. Help from AI in case of errors may have been taken.