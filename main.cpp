#include<SFML/Graphics.hpp>
#include<SFML/OpenGL.hpp>

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
    RenderWindow window(VideoMode({width, height}), "TankMaster", Style::Default, State::Windowed, settings);
    // window.setVerticalSyncEnabled(true); 
    // // enabled in prod, but disabled beforehand to keep track of optimizations
    // window.setFramerateLimit(300);
    glEnable(GL_TEXTURE_2D);
    Clock clock;

    // all variable declarations go here
    float ballsX[2048], ballsY[2048], ballsVX[2048], ballsVY[2048], ballRadius = 5.f, ballSpeed = 50.f;
    short int balls = 10, nextBall = 0, ballPts = 32, frameCount = 0;
    float totalTime = 0.0f;
    float mouseX = 0.f, mouseY = 0.f; // will be used a lot so declared here
    
    // all object declarations go here
    CircleShape ball(ballRadius, ballPts);
    Font mojangles("mojangles.ttf");
    Text fpsText(mojangles);

    
    // all object property edits go here
    ball.setFillColor(Color::White);
    ball.setOrigin({ballRadius, ballRadius}); // draw from center
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
    while (window.isOpen()){
        // events
        while (const optional event = window.pollEvent()) {
            // manage close and resize
            if (event -> is<Event::Closed>()) window.close();
            // mouse events
            else if (const auto* evnt = event->getIf<Event::MouseMoved>())
            {
                    // convert screen pixel position to world coordinates
                    // this is initially effect less, but as soon as we scale or resize the view,
                    // the pixel position is no longer equivalent to world position
                    Vector2f worldPos = window.mapPixelToCoords(evnt->position);
                    mouseX = worldPos.x; // hehe
                    mouseY = worldPos.y;
                    if (nextBall >= 2048) { nextBall = 0; }
            }
        }
        // deltaTime calc
        float deltaTime = clock.restart().asSeconds();
        totalTime += deltaTime; frameCount++;

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
            ball.setPosition({ballsX[i], ballsY[i]});
            window.draw(ball);
        }
        window.draw(fpsText);
        window.display();
    }

    return 0;
}
