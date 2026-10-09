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
    //window.setVerticalSyncEnabled(true);
    //window.setFramerateLimit(300);
    glEnable(GL_TEXTURE_2D);
    Clock clock;

    // all variable declarations go here
    float ballsX[2048], ballsY[2048], ballsVX[2048], ballsVY[2048], ballRadius = 5.f, ballSpeed = 50.f;
    int balls = 10, ballPts = 32, frameCount = 0;
    float totalTime = 0.0f;
    
    // all object declarations go here
    CircleShape ball(ballRadius, ballPts);
    RectangleShape glass({0.f, 0.f});
    Font mojangles("mojangles.ttf");
    Text fpsText(mojangles);

    
    // all object property edits go here
    ball.setFillColor(Color::White);
    ball.setOrigin({ballRadius, ballRadius}); // draw from center
    glass.setFillColor(Color(0, 0, 0, 5));
    glass.setSize({width, height});
    fpsText.setCharacterSize(24);
    fpsText.setFillColor(sf::Color::Green);
    fpsText.setPosition({10.f, 10.f});
    
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
            // manage close and resize
            if (event -> is<Event::Closed>()) window.close();
            else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                width = resized->size.x;
                height = resized->size.y;
                View view = window.getView();
                // apparently we gotta update the view size
                view.setSize({static_cast<float>(width), static_cast<float>(height)});
                view.setCenter({width / 2.f, height / 2.f}); // and set the center
                window.setView(view); // and then set the view to the window
                // and set the tracer size too
                glass.setSize({static_cast<float>(width), static_cast<float>(height)});
            }
            // mouse events
            else if (const auto* evnt = event->getIf<Event::MouseButtonPressed>())
            {
                if (evnt->button == Mouse::Button::Left)
                {
                    ballsX[balls] = evnt->position.x;
                    ballsY[balls] = evnt->position.y;
                    ballsVX[balls] = width/2 - evnt->position.x;
                    ballsVY[balls] = height/2 - evnt->position.y;
                    balls++;
                }
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
        // window.draw(glass);
        for (int i = 0; i < balls; i++) {
            ball.setPosition({ballsX[i], ballsY[i]});
            window.draw(ball);
        }
        window.draw(fpsText);
        window.display();
    }

    return 0;
}
