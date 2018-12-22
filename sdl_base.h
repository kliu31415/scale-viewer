/*A simple SDL2 wrapper by Kevin Liu
*/
#pragma once
#include <string>
#include <vector>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#ifndef SDL_main
#undef main
#endif
#define RANDUZ_MAX 0x0fffffff
extern SDL_Event input;
namespace sdl_settings
{
    extern bool lowTextureQuality, vsync, acceleratedRenderer, textBlended;
    extern int WINDOW_W, WINDOW_H, WINDOW_X, WINDOW_Y;
    extern int renderScaleQuality, fontQuality, musicVolume, sfxVolume;
    extern double Rgamma, Ggamma, Bgamma, brightness, textSizeMult;
    extern bool showFPS, IS_FULLSCREEN; //overrides WINDOW_W and WINDOW_H
    extern int FPS_CAP; //FPS cap (300 is essentially uncapped)
    extern int TEXT_SDL_Texture_CACHE_TIME;
    /**
    Reads sdl_settings variables from a file
    */
    void output_config();
    /**
    Writes sdl_settings variables from a file
    */
    void load_config();
}
/**
This prints a string to stdout
*/
void print(std::string s);
/**
This prints a string and appends a newline to stdout
*/
void println(std::string s);
/**
Converts something to a string
*/
template<class T> std::string to_str(T x);
/**
Converts an integer number of seconds to time in the form H:MM:SS
*/
std::string seconds_to_str(int t);
/**
Converts an integer number of seconds to time in the form M:SS or M:SS
*/
std::string seconds_to_str_no_h(int t);
/**
returns either -1 or 1 with 50% probability
*/
int randsign();
/**
Generates a random integer from 0 to RANDUZ_MAX using std::mt19937
*/
int randuz();
/**
Generates random numbers using randuz(). Can be passed to stdlib functions.
*/
struct generateRandZ
{
    typedef size_t result_type;
    static size_t min()
    {
        return 0;
    }
    static size_t max()
    {
        return RANDUZ_MAX;
    }
    size_t operator()()
    {
        return randuz();
    }
};
/**
Generates a random integer from 0 to m-1
*/
int randuzm(int m);
/**
Returns a random float from 0 to 1
*/
double randf();
/**
Returns a random integer from v1 to v2
*/
int randz(int v1, int v2);
/**
Rounds a double to the specified number of places (0 to 9)
*/
double round(double x, int places);
/**
Returns the number of milliseconds since initSDL was called
*/
int getTicks();
/**
Returns the number of nanoseconds since initSDL was called
*/
long long getTicksNs();
/**
Returns the number of seconds (as a double) since initSDL was called
*/
double getTicksS();
/**
Takes in a double and returns a formatted string to the specified number of decimal places
*/
std::string format_to_places(double x, int places);
/**
Returns a pointer to the current SDL_Window
*/
SDL_Window *getWindow();
/**
Sets the window brightness to an arbitrary value, but doesn't save it
*/
void setBrightness(double b);
/**
Sets the window brightness to the current stored value
*/
void resetBrightness();
/**
Sets the window gamma RGB to an arbitrary value, but doesn't save it
*/
void setGamma(double r, double g, double b);
/**
Sets the window gamma RGB to the current stored gamma values
*/
void resetGamma();
/**
Initializes SDL
*/
void initSDL(const char *name = "Program");
/**
Calls SDL_Quit() and then restarts SDL
*/
void reinitSDL();
/**
Equivalent to SDL_SetRenderDrawColor
*/
void setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Equivalent to SDL_RenderClear
*/
void renderClear();
/**
Equivalent to SDL_RenderClear
*/
void renderClear(uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Equivalent to SDL_RenderCopy
*/
void renderCopy(SDL_Texture *t, SDL_Rect *dst);
/**
Equivalent to SDL_RenderCopy
*/
void renderCopy(SDL_Texture *t, SDL_Rect *src, SDL_Rect *dst);
/**
Equivalent to SDL_RenderCopy
*/
void renderCopy(SDL_Texture *t, int x, int y, int w, int h);
/**
Equivalent to SDL_RenderCopyEx
*/
void renderCopyEx(SDL_Texture *t, int x, int y, int w, int h, double rot, SDL_Point *center = NULL, SDL_RendererFlip f = SDL_FLIP_NONE);
/**
Converts a string into an SDL_Texture
*/
SDL_Texture *createText(std::string txt, int s, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Draws unwrapped text on the window. The SDL_Texture is cached for TEXT_TEXTURE_CACHE_TIME (1100ms by default).
*/
void drawText(std::string text, int x, int y, int s, uint8_t r=0, uint8_t g=0, uint8_t b=0, uint8_t a=255);
/**
Draws wrapped text on the window, and breaks won't occur in the middle of words. The SDL_Texture is cached for TEXT_TEXTURE_CACHE_TIME.
*/
int drawMultilineTextUnbroken(std::string text, int x, int y, int w, int s, uint8_t r=0, uint8_t g=0, uint8_t b=0, uint8_t a=255);
/**
Fills in xpos and ypos with the x and y position of the character AFTER the last character drawn from the drawMultilineTextUnbroken function.
*/
void getMultilineTextUnbrokenInfo(std::string text, int w, int s, std::vector<std::string> &lines);
/**
Draws wrapped text on the window. The SDL_Texture is cached for TEXT_TEXTURE_CACHE_TIME.
*/
int drawMultilineText(std::string text, int x, int y, int w, int s, uint8_t r=0, uint8_t g=0, uint8_t b=0, uint8_t a=255);
/**
Fills in xpos and ypos with the x and y position of the last character drawn from the drawMultilineText function.
*/
void getMultilineTextPos(std::string text, int w, int s, int *xpos, int *ypos);
/**
Returns how many times a given text will be wrapped if it is drawn
*/
int multilineTextLength(std::string text, int w, int s);
/**
Displays a loading screen
*/
void showLoadingScreen();
/**
Equivalent to SDL_RenderFillRect
*/
void fillRect(SDL_Rect *x);
/**
Equivalent to SDL_RenderFillRect
*/
void fillRect(SDL_Rect *x, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Equivalent to SDL_RenderFillRect
*/
void fillRect(int x, int y, int w, int h);
/**
Equivalent to SDL_RenderFillRect
*/
void fillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Equivalent to SDL_RenderDrawRect
*/
void drawRect(int x, int y, int w, int h);
/**
Equivalent to SDL_RenderDrawRect
*/
void drawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Equivalent to SDL_RenderFillRect
*/
void drawRect(SDL_Rect *x);
/**
Equivalent to SDL_RenderFillRect
*/
void drawRect(SDL_Rect *x, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Equivalent to SDL_SetTextureColorMod
*/
void setTextureColorMod(SDL_Texture *t, uint8_t r, uint8_t g, uint8_t b);
/**
Equivalent to SDL_SetTextureAlphaMod
*/
void setTextureAlphaMod(SDL_Texture *t, uint8_t a);
/**
Loads a SDL_Texture from an image file and color keys it
*/
SDL_Texture *loadTexture(const char *name, uint8_t r, uint8_t g, uint8_t b);
/**
Loads a SDL_Texture from an image file
*/
SDL_Texture *loadTexture(const char *name);
/**
Checks if two SDL_Rects intersect
*/
bool rectsIntersect(SDL_Rect a, SDL_Rect b);
/**
Equivalent to SDL_RenderDrawLine
*/
void drawLine(int x1, int y1, int x2, int y2);
/**
Equivalent to SDL_RenderDrawLine
*/
void drawLine(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Equivalent to SDL_RenderDrawPoint
*/
void drawPoint(int x, int y);
/**
Equivalent to SDL_RenderDrawPoint
*/
void drawPoint(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a=0);
/**
Draws a filled circle
*/
void fillCircle(int x, int y, int r);
/**
Draws a filled circle
*/
void fillCircle(int x, int y, int rad, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Draws an unfilled circle
*/
void drawCircle(int x, int y, int r);
/**
Draws an unfilled circle
*/
void drawCircle(int x, int y, int rad, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
/**
Checks if the mouse is in a given SDL_Rect
*/
bool mouseInRect(int x, int y, int w, int h);
/**
Checks if the mouse is in a given SDL_Rect
*/
bool mouseInRect(SDL_Rect *x);
/**
Returns the x coordinate of the mouse. The mouse state is updated during the updateScreen() function.
*/
int getMouseX();
/**
Returns the y coordinate of the mouse. The mouse state is updated during the updateScreen() function.
*/
int getMouseY();
/**
Equivalent to SDL_RenderSetViewport
*/
void setViewport(SDL_Rect *x);
/**
Equivalent to SDL_RenderSetViewport
*/
void setViewport(int x, int y, int w, int h);
/**
Equivalent to SDL_RenderSetClipRect
*/
void setClipRect(SDL_Rect *x);
/**
Equivalent to SDL_RenderSetClipRect
*/
void setClipRect(int x, int y, int w, int h);
/**
Equivalent to SDL_SetWindowIcon
*/
void setWindowIcon(SDL_Surface *icon);
/**
Returns a pointer to TTF_Font of size 2^pos
*/
TTF_Font *getFont(int pos);
/**
Sets the color of the FPS counter
*/
void setFPScolor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
/**
Returns the time between this frame and the last frame
*/
int getFrameLength();
/**
Returns a pointer to the renderer
*/
SDL_Renderer *getRenderer();
/**
Changes how long text textures are cached for
*/
void setTextTextureCacheTime(int ms);
/**
Sets the quality of text rendering (-2=worst, 2=best)
*/
void setTextQuality(int q);
/**
Sets the quality of the renderer's antialiasing (0 = worst, 2 = best)
*/
void setRendererAA(int q);
/**
Updates the screen and performs some other functions. This function is called to advance to the next frame.
*/
void updateScreen();
/**
Returns the window's width
*/
int getWindowW();
/**
Returns the window's height
*/
int getWindowH();
/**
Returns the window's area in pixels
*/
int getWindowArea();
/**
Returns the window's X position
*/
int getWindowX();
/**
Returns the window's Y position
*/
int getWindowY();
/**
Returns a font size where lower is smaller and 0 is medium size
*/
int getFontSize(double sz = 0);
/**
Gets the width and height of a block of text
*/
void getTextSize(std::string text, int sz, int *w, int *h);
/**
Checks if a mouse button is pressed
*/
bool isMouseButtonPressed(int button);
/**
Sets the renderer target.
*/
bool setRenderTarget(SDL_Texture *t);
/**
Returns the intermediate texture that effectively represents the display
*/
SDL_Texture *getScreenTexture();
/**
Returns the display width
*/
int getDisplayW();
/**
Returns the display height
*/
int getDisplayH();
/**
Returns the display refresh rate (or 60 by default if the function fails)
*/
int getDisplayHertz();
/**
Changes the base text size returned by getFontSize
*/
void setTextSizeMult(double m);
/**
Returns the current FPS
*/
int getFPS();
/**
Loads a Mix_Chunk* from a file and checks for errors
*/
Mix_Chunk *loadMixChunk(const char *name);
/**
Loads a Mix_Music* from a file and checks for errors
*/
Mix_Music *loadMixMusic(const char *name);
/**
Sets the music (Mix_Music) volume
*/
void setMusicVolume(int v);
/**
Sets sfx (Mix_Chunk) volume
*/
void setSfxVolume(int v);
