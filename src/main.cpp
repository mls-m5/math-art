#include <SDL.h>
#include <SDL_opengl.h>

#include <cmath>
#include <iostream>
#include <vector>

struct Point
{
    float x;
    float y;
};

struct Line
{
    Point start;
    Point end;
};

int main(int, char **)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
        return 1;
    }

    // OpenGL 2.1 keeps this demo small: no shaders or function loader required.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window *window = SDL_CreateWindow(
        "Blumme - drag to draw | C: clear | Space: pause | Esc: quit",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 640,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        std::cerr << "OpenGL context creation failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    const bool vsync = SDL_GL_SetSwapInterval(1) == 0;

    std::vector<Line> lines;
    Point start{};
    Point cursor{};
    bool drawing = false;
    bool running = true;
    bool paused = false;
    float angle = 0.0f;
    Uint64 previous = SDL_GetPerformanceCounter();
    const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());

    // Mouse coordinates are logical pixels; OpenGL's viewport uses drawable pixels.
    const auto worldPoint = [window](int x, int y) -> Point {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        if (width <= 0 || height <= 0)
            return {0.0f, 0.0f};
        const float aspect = static_cast<float>(width) / height;
        return {(2.0f * x / width - 1.0f) * aspect, 1.0f - 2.0f * y / height};
    };

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    running = false;
                if (!event.key.repeat && event.key.keysym.sym == SDLK_SPACE)
                    paused = !paused;
                if (event.key.keysym.sym == SDLK_c) {
                    lines.clear();
                    drawing = false;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    start = cursor = worldPoint(event.button.x, event.button.y);
                    drawing = true;
                }
                break;
            case SDL_MOUSEMOTION:
                cursor = worldPoint(event.motion.x, event.motion.y);
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT && drawing) {
                    lines.push_back({start, worldPoint(event.button.x, event.button.y)});
                    drawing = false;
                }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
                    drawing = false;
                break;
            default:
                break;
            }
        }
        if (!running)
            break;

        const Uint64 now = SDL_GetPerformanceCounter();
        const double elapsed = static_cast<double>(now - previous) / frequency;
        previous = now;
        if (!paused)
            angle = std::fmod(angle + static_cast<float>(elapsed * 30.0), 360.0f);

        int width, height;
        SDL_GL_GetDrawableSize(window, &width, &height);
        if (width <= 0 || height <= 0) {
            SDL_Delay(10);
            continue;
        }
        const float aspect = static_cast<float>(width) / height;
        glViewport(0, 0, width, height);
        glClearColor(0.035f, 0.045f, 0.075f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Background grid and coordinate axes, all drawn with GL_LINES.
        glColor3f(0.11f, 0.14f, 0.20f);
        glBegin(GL_LINES);
        const int columns = static_cast<int>(std::ceil(aspect / 0.2f));
        for (int i = -columns; i <= columns; ++i) {
            const float x = i * 0.2f;
            glVertex2f(x, -1.0f);
            glVertex2f(x, 1.0f);
        }
        for (int i = -5; i <= 5; ++i) {
            const float y = i * 0.2f;
            glVertex2f(-aspect, y);
            glVertex2f(aspect, y);
        }
        glColor3f(0.30f, 0.35f, 0.45f);
        glVertex2f(-aspect, 0.0f);
        glVertex2f(aspect, 0.0f);
        glVertex2f(0.0f, -1.0f);
        glVertex2f(0.0f, 1.0f);
        glEnd();

        glPushMatrix();
        glRotatef(angle, 0.0f, 0.0f, 1.0f);
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.30f, 0.40f);
        glVertex2f(0.0f, 0.60f);
        glColor3f(0.25f, 0.90f, 0.65f);
        glVertex2f(-0.52f, -0.30f);
        glColor3f(0.30f, 0.55f, 1.0f);
        glVertex2f(0.52f, -0.30f);
        glEnd();

        glColor3f(0.95f, 0.95f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(0.0f, 0.60f);
        glVertex2f(-0.52f, -0.30f);
        glVertex2f(0.52f, -0.30f);
        glEnd();
        glPopMatrix();

        glColor3f(1.0f, 0.80f, 0.25f);
        glBegin(GL_LINES);
        for (const Line &line : lines) {
            glVertex2f(line.start.x, line.start.y);
            glVertex2f(line.end.x, line.end.y);
        }
        if (drawing) {
            glVertex2f(start.x, start.y);
            glVertex2f(cursor.x, cursor.y);
        }
        glEnd();

        SDL_GL_SwapWindow(window);
        if (!vsync)
            SDL_Delay(16);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
