#include<SFML/Graphics.hpp>
#include<SFML/OpenGL.hpp>

using namespace sf;
using namespace std;

int main() {
    // define main window, rendering, settings & clock
    const int width = 500, height = 500;
    ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 4;
    settings.majorVersion = 3;
    settings.minorVersion = 0;
    RenderWindow window(VideoMode({width, height}), "TankMaster", Style::Default, State::Windowed, settings);
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);
    glEnable(GL_TEXTURE_2D);
    Clock clock;

    // all variable declarations go here
    float ballsX[2048], ballsY[2048], ballsVX[2048], ballsVY[2048], ballRadius = 5.f, ballSpeed = 50.f;
    int balls = 10, ballPts = 32;
    
    // all object declarations go here
    CircleShape ball(ballRadius, ballPts);
    RectangleShape glass({0.f, 0.f});
    Font mojangles("mojangles.ttf");

    // all object property edits go here
    ball.setFillColor(Color::White);
    glass.setFillColor(Color(0, 0, 0, 50));
    glass.setSize({width, height});
    
    for (int i = 0; i < balls; i++) {
        ballsX[i] = 100 + i * 10;
        ballsY[i] = 100 + i * 10;
        ballsVX[i] = ballSpeed;
        ballsVY[i] = ballSpeed + i * ballSpeed;
    }

    // main loop
    while (window.isOpen()){
        // events
        while (const optional event = window.pollEvent()) {
            if (event -> is<Event::Closed>()) window.close();
            else if (const auto* evnt = event->getIf<Event::MouseButtonPressed>())
            {
                if (evnt->button == Mouse::Button::Left)
                {
                    ballsX[balls] = evnt->position.x;
                    ballsY[balls] = evnt->position.y;
                    ballsVX[balls] = ballsVY[balls] = ballSpeed;
                    balls++;
                }
            }
        }
        // deltaTime calc
        float deltaTime = clock.restart().asSeconds();

        // processing goes here
        for (int i = 0; i < balls; i++) {
            // velocity operations
            ballsX[i] += ballsVX[i] * deltaTime;
            ballsY[i] += ballsVY[i] * deltaTime;
            // reflect from walls
            if (ballsX[i] + ballRadius > width) {
                ballsX[i] = width - ballRadius;
                ballsVX[i] *= -1.f;
            }
            if (ballsY[i] + ballRadius > height) {
                ballsY[i] = height - ballRadius;
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

        // shape updates go here
        

        // draw starts here
        window.draw(glass);
        for (int i = 0; i < balls; i++) {
            ball.setPosition({ballsX[i], ballsY[i]});
            window.draw(ball);
        }
        window.display();
    }

    return 0;
}
