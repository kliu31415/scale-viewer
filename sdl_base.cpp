//A simple SDL2 wrapper by Kevin Liu
#include "sdl_base.h"
#include <sstream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>
#include <fstream>
#include <map>
#include <queue>
#include <utility>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <thread>
#include <algorithm>
#include <random>
#include <mutex>
#include <iostream> //for debugging
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
static const int SDL_BASE_NOT_SET = ((int)(1e9 + 23001));
static SDL_Renderer *renderer = NULL;
static SDL_Window *window = NULL;
SDL_Event input;
static const int NUM_FONT_SIZES = 8; //I assume no text with a size of more than 1,000 will be drawn
static TTF_Font *font[NUM_FONT_SIZES];
static int prevTick = 0, frameLength;
static int mouse_x, mouse_y;
//a text SDL_Texture cache greatly speeds up stuff because we don't have to create the SDL_Texture every time
struct text_info
{
    std::string s;
    uint8_t r, g, b;
    int sz;
    text_info(std::string ss, int size, uint8_t r1, uint8_t g1, uint8_t b1);
    bool operator<(text_info x) const;
};
static std::map<text_info, std::pair<int, SDL_Texture*> > text_textures; //text_info, <time last used, SDL_Texture>
namespace sdl_settings
{
    bool lowTextureQuality = true;
    bool vsync = true;
    bool acceleratedRenderer = true;
    const static int WINDOW_POS_NOT_SET = -1e9;
    int musicVolume = MIX_MAX_VOLUME, sfxVolume = MIX_MAX_VOLUME;
    int WINDOW_W = 2500, WINDOW_H = 1500, WINDOW_X = WINDOW_POS_NOT_SET, WINDOW_Y = WINDOW_POS_NOT_SET;
    int renderScaleQuality = 2;
    int fontQuality = 1;
    bool textBlended = true;
    double Rgamma = -1, Ggamma = -1, Bgamma = -1, brightness = -1;
    bool showFPS = false;
    bool IS_FULLSCREEN = false; //overrides WINDOW_W and WINDOW_H
    int FPS_CAP = 300; //FPS cap (300 is essentially uncapped)
    int TEXT_TEXTURE_CACHE_TIME = 1100; //number of milliseconds of being unused after a which a text SDL_Texture is destroyed
    double textSizeMult = 1;
    static std::queue<int> frameTimeStamp;
    //code for reading config
    static const char *const FOUT_FILE_NAME = "sdl_base_config.txt";
    static bool init = false;
    static std::map<std::string, std::pair<std::string, void*> > vals; //Config name, <Type name, memory address of variable>
    static void init_config()
    {
        init = true;
        vals["VSYNC"] = std::make_pair("bool", &vsync);
        vals["ACCELERATED_RENDERER"] = std::make_pair("bool", &acceleratedRenderer);
        vals["LOW_TEXTURE_QUALITY"] = std::make_pair("bool", &lowTextureQuality);
        vals["IS_FULLSCREEN"] = std::make_pair("bool", &IS_FULLSCREEN);
        vals["FPS_CAP"] = std::make_pair("int", &FPS_CAP);
        vals["HORIZONTAL_RESOLUTION"] = std::make_pair("int", &WINDOW_W);
        vals["VERTICAL_RESOLUTION"] = std::make_pair("int", &WINDOW_H);
        vals["SFX_VOLUME"] = std::make_pair("int", &sfxVolume);
        vals["MUSIC_VOLUME"] = std::make_pair("int", &musicVolume);
        vals["WINDOW_X"] = std::make_pair("int", &WINDOW_X);
        vals["WINDOW_Y"] = std::make_pair("int", &WINDOW_Y);
        vals["SHOW_FPS"] = std::make_pair("bool", &showFPS);
        vals["FONT_QUALITY"] = std::make_pair("int", &fontQuality);
        vals["RENDER_SCALE_QUALITY"] = std::make_pair("int", &renderScaleQuality);
        vals["TEXT_TEXTURE_CACHE_TIME"] = std::make_pair("int", &TEXT_TEXTURE_CACHE_TIME);
        vals["TEXT_BLENDED"] = std::make_pair("bool", &textBlended);
        vals["R_GAMMA"] = std::make_pair("double", &Rgamma);
        vals["G_GAMMA"] = std::make_pair("double", &Ggamma);
        vals["B_GAMMA"] = std::make_pair("double", &Bgamma);
        vals["BRIGHTNESS"] = std::make_pair("double", &brightness);
        vals["TEXT_SIZE"] = std::make_pair("double", &textSizeMult);
    }
    void output_config()
    {
        if(!init)
            init_config();
        std::remove(FOUT_FILE_NAME);
        std::ofstream fout(FOUT_FILE_NAME);
        if(WINDOW_W > 1e9) //ok reset things
        {
            WINDOW_X = getDisplayW() * 0.1;
            WINDOW_Y = getDisplayH() * 0.1;
            WINDOW_W = getDisplayW() * 0.8;
            WINDOW_H = getDisplayH() * 0.8;
        }
        else
        {
            SDL_GetWindowSize(getWindow(), &WINDOW_W, &WINDOW_H);
            SDL_GetWindowPosition(getWindow(), &WINDOW_X, &WINDOW_Y);
        }
        if(IS_FULLSCREEN && WINDOW_X==0 && WINDOW_Y==0)
        {
            WINDOW_X = SDL_BASE_NOT_SET;
            WINDOW_Y = SDL_BASE_NOT_SET;
        }
        for(auto &i: vals)
        {
            if(i.second.first == "int")
                fout << i.first << " = " << *((int*)i.second.second) << "\n";
            else if(i.second.first == "bool")
                fout << i.first << " = " << *((bool*)i.second.second) << "\n";
            else if(i.second.first == "double")
                fout << i.first << " = " << *((double*)i.second.second) << "\n";
        }
        fout.close();
    }
    void load_config()
    {
        if(!init)
            init_config();
        std::ifstream fin(FOUT_FILE_NAME);
        if(fin.fail())
        {
            println("Failed to load config file. Using default values.");
        }
        else
        {

            std::string s1, s2, s3;
            while(fin >> s1)
            {
                if(!(fin>>s2) || !(fin>>s3)) //s2 is just an equals sign and s3 contains a value
                    break;
                if(!vals.count(s1))
                {
                    println("Cannot find setting name " + (std::string)"\"" + s1 + "\"");
                }
                else
                {
                    if(vals[s1].first == "int")
                        *((int*)vals[s1].second) = atoi(s3.c_str());
                    else if(vals[s1].first == "bool")
                        *((bool*)vals[s1].second) = atoi(s3.c_str());
                    else if(vals[s1].first == "double")
                        *((double*)vals[s1].second) = atof(s3.c_str());
                }
            }
        }
        fin.close();
    }
}
//Non SDL functions
/**
This prints a string to stdout
*/
void print(std::string s)
{
    //stdoutMutex.lock();
    std::puts(("[" + seconds_to_str(getTicksS()) + "]" + s).c_str());
    std::fflush(stdout);
    //stdoutMutex.unlock();
}
/**
This prints a string and appends a newline to stdout
*/
void println(std::string s)
{
    //stdoutMutex.lock();
    std::puts(("[" + seconds_to_str(getTicksS()) + "]" + s).c_str());
    std::fflush(stdout);
    //stdoutMutex.unlock();
}
/**
Converts something to a string
*/
template<class T> std::string to_str(T x)
{
    std::stringstream ss;
    ss << x;
    return ss.str();
}
template std::string to_str<int>(int);
template std::string to_str<unsigned>(unsigned);
template std::string to_str<char>(char);
template std::string to_str<uint64_t>(uint64_t);
template std::string to_str<int64_t>(int64_t);
template std::string to_str<std::string>(std::string);
template std::string to_str<const char*>(const char*);
template std::string to_str<char*>(char*);
template std::string to_str<double>(double);
template std::string to_str<long double>(long double);
/**
Converts an integer number of seconds to time in the form H:MM:SS
*/
std::string seconds_to_str(int t)
{
    int s = t%60;
    t /= 60;
    int m = t%60;
    int h = t / 60;
    std::string res;
    if(h == 0)
        res += "0";
    else res += to_str(h);
    res += ":";
    if(m == 0)
        res += "00";
    else if(m < 10)
    {
        res += "0";
        res += to_str(m);
    }
    else res += to_str(m);
    res += ":";
    if(s == 0)
        res += "00";
    else if(s < 10)
    {
        res += "0";
        res += to_str(s);
    }
    else res += to_str(s);
    return res;
}
/**
Converts an integer number of seconds to time in the form M:SS
*/
std::string seconds_to_str_no_h(int t)
{
    int s = t%60;
    t /= 60;
    int m = t%60;
    std::string res;
    res += to_str(m);
    res += ":";
    if(s == 0)
        res += "00";
    else if(s < 10)
    {
        res += "0";
        res += to_str(s);
    }
    else res += to_str(s);
    return res;
}
/**
returns either -1 or 1 with 50% probability
*/
int randsign()
{
    return 1 - 2 * randz(0, 1);
}
/**
Generates a random integer from 0 to RANDUZ_MAX using std::mt19937
*/
int randuz()
{
    static std::mt19937 gen(time(NULL));
    static std::uniform_int_distribution<int> d(0, RANDUZ_MAX);
    return d(gen);
}
/**
Generates a random integer from 0 to m-1
*/
int randuzm(int m)
{
    return randuz() % m;
}
/**
Returns a random float from 0 to 1
*/
double randf()
{
    return randuz() / (double)RANDUZ_MAX;
}
/**
Returns a random integer from v1 to v2
*/
int randz(int v1, int v2)
{
    double diff = v2 - v1 + 0.99999999;
    return randf()*diff + v1;
}
/**
Rounds a double to the specified number of places (0 to 9)
*/
double round(double x, int places)
{
    double p = pow(10, places);
    if(x >= 0)
        return std::floor(x * p + 0.5) / p;
    else return std::floor(x * p - 0.5) / p;
}
/**
Returns the number of milliseconds since initSDL was called
*/
int getTicks()
{
    return getTicksNs() / 1000000;
}
/**
Returns the number of milliseconds since initSDL was called
*/
long long getTicksNs()
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto t = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(t - startTime).count();
}
/**
Returns the number of seconds (as a double) since initSDL was called
*/
double getTicksS()
{
    return getTicksNs() / 1e9;
}
/**
Takes in a double and returns a formatted string to the specified number of decimal places
*/
std::string format_to_places(double x, int places)
{
    std::string res = to_str(round(x, places));
    for(size_t i=0; i<res.size(); i++)
    {
        if(res[i] == '.')
        {
            while(res.size() < i + places + 1)
                res += '0';
            return res;
        }
    }
    res += '.';
    for(int i=0; i<places; i++)
        res += '0';
    return res;
}
//End non SDL functions
/**
Returns a pointer to the current SDL_Window
*/
SDL_Window *getWindow()
{
    return window;
}
/**
Initializes a window and renderer.
*/
static void createWindow(const char *name)
{
    using namespace sdl_settings;
    if(window)
        SDL_DestroyWindow(window);
    if(renderer)
        SDL_DestroyRenderer(renderer);
    window = SDL_CreateWindow(name, WINDOW_X, WINDOW_Y, WINDOW_W, WINDOW_H,
                                    SDL_WINDOW_SHOWN | (SDL_WINDOW_FULLSCREEN*(int)IS_FULLSCREEN) | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, (SDL_RENDERER_ACCELERATED*acceleratedRenderer) | (SDL_RENDERER_PRESENTVSYNC*vsync));
    /*if(IS_FULLSCREEN)
        SDL_RenderSetLogicalSize(renderer, WINDOW_W, WINDOW_H);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, to_str(renderScaleQuality).c_str());*/
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}
/**
Calculates the gamma given the 128th element of the gamma ramp
*/
static double calcGamma(double v128)
{
    return -log(2) / log(v128 / 65536.0);
}
/**
Sets the window brightness to an arbitrary value, but doesn't save it
*/
void setBrightness(double b)
{
    SDL_SetWindowBrightness(window, b);
}
/**
Sets the window brightness to the current stored value
*/
void resetBrightness()
{
    using namespace sdl_settings;
    setBrightness(brightness);
}
/**
Sets the window gamma RGB to an arbitrary value, but doesn't save it
*/
void setGamma(double r, double g, double b)
{
    uint16_t R[256], G[256], B[256];
    SDL_CalculateGammaRamp(r, R);
    SDL_CalculateGammaRamp(g, G);
    SDL_CalculateGammaRamp(b, B);
    SDL_SetWindowGammaRamp(window, R, G, B);
}
/**
Sets the window gamma RGB to the current stored gamma values
*/
void resetGamma()
{
    using namespace sdl_settings;
    setGamma(Rgamma, Ggamma, Bgamma);
}
/**
Initializes SDL with a given window name
*/
void initSDL(const char *name)
{
    bool alreadyInit = false;
    using namespace sdl_settings;
    if(!alreadyInit)
    {
        alreadyInit = true;
        srand(time(NULL));
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
            println((std::string)"SDL_GetError(): " + SDL_GetError());
        if(Mix_Init(MIX_INIT_MP3 | MIX_INIT_FLAC))
            println((std::string)"Mix_GetError(): " + Mix_GetError());
        if(Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) < 0)
            println((std::string)"Mix_GetError(): " + Mix_GetError());
        Mix_Volume(-1, sfxVolume);
        Mix_VolumeMusic(musicVolume);
        Mix_AllocateChannels(32);
        if(TTF_Init() < 0)
            println((std::string)"TTF_GetError(): " + TTF_GetError());
        if(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) < 0)
            println((std::string)"IMG_GetError(): " + IMG_GetError());
        for(int i=0, v=1; i<NUM_FONT_SIZES; i++, v*=2)
            font[i] = TTF_OpenFont("font.ttf", v);
        if(font[0] == NULL)
            println((std::string)"TTF_GetError(): " + TTF_GetError());
        else if(!TTF_FontFaceIsFixedWidth(font[0]))
            println("Warning: font may not be monospaced, which may cause rendering issues");
        getTicks();
    }
    else text_textures.clear();
    createWindow(name);
    //let's not mess with gammas and brightness
    /*if(brightness == -1) //not yet set
    {
        uint16_t R[256], G[256], B[256];
        brightness = SDL_GetWindowBrightness(window);
        SDL_GetWindowGammaRamp(window, R, G, B);
        Rgamma = calcGamma(R[128]);
        Ggamma = calcGamma(G[128]);
        Bgamma = calcGamma(B[128]);
    }
    else //already set
    {
        setBrightness(brightness);
        setGamma(Rgamma, Ggamma, Bgamma);
    }*/
}
/**
Calls SDL_Quit() and then restarts SDL
*/
void reinitSDL()
{
    createWindow(SDL_GetWindowTitle(window));
    text_textures.clear();
}
/**
Equivalent to SDL_SetRenderDrawColor
*/
void setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}
/**
Equivalent to SDL_RenderClear
*/
void renderClear()
{
    SDL_RenderClear(renderer);
}
/**
Equivalent to SDL_RenderClear
*/
void renderClear(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    setColor(r, g, b, a);
    SDL_RenderClear(renderer);
}
/**
Equivalent to SDL_RenderCopy
*/
void renderCopy(SDL_Texture *t, SDL_Rect *dst)
{
    SDL_RenderCopy(renderer, t, NULL, dst);
}
/**
Equivalent to SDL_RenderCopy
*/
void renderCopy(SDL_Texture *t, SDL_Rect *src, SDL_Rect *dst)
{
    SDL_RenderCopy(renderer, t, src, dst);
}
/**
Equivalent to SDL_RenderCopy
*/
void renderCopy(SDL_Texture *t, int x, int y, int w, int h)
{
    SDL_Rect r{x, y, w, h};
    renderCopy(t, &r);
}
/**
Equivalent to SDL_RenderCopyEx
*/
void renderCopyEx(SDL_Texture *t, int x, int y, int w, int h, double rot, SDL_Point *center, SDL_RendererFlip f)
{
    SDL_Rect r{x, y, w, h};
    SDL_RenderCopyEx(renderer, t, NULL, &r, rot, center, f);
}
/**
Returns the position in font[NUM_FONT_SIZES] that should be used to render a text of a given size.
*/
static int getFontSizePos(int s)
{
    int sz = sdl_settings::fontQuality + log2(s+1);
    sz = std::max(0, std::min(NUM_FONT_SIZES-1, sz));
    return sz;
}
/**
Converts a string into an SDL_Texture
*/
SDL_Texture *createText(std::string txt, int s, uint8_t r, uint8_t g, uint8_t b, uint8_t a) //creates but doesn't render text
{
    using namespace sdl_settings;
    SDL_Color col{r, g, b, a};
    SDL_Surface *__s;
    if(textBlended)
        __s = TTF_RenderText_Blended(font[getFontSizePos(s)], txt.c_str(), col);
    else __s = TTF_RenderText_Solid(font[getFontSizePos(s)], txt.c_str(), col);
    SDL_Texture *__t = SDL_CreateTextureFromSurface(renderer, __s);
    SDL_FreeSurface(__s);
    return __t;
}
text_info::text_info(std::string ss, int size, uint8_t r1, uint8_t g1, uint8_t b1)
{
    s = ss;
    sz = size;
    r = r1;
    g = g1;
    b = b1;
}
bool text_info::operator<(text_info x) const
{
    if(s != x.s)
        return s < x.s;
    if(sz != x.sz)
        return sz < x.sz;
    if(r != x.r)
        return r < x.r;
    if(g != x.g)
        return g < x.g;
    if(b != x.b)
        return b < x.b;
    return 0;
}
/**
Draws unwrapped text on the window. The SDL_Texture is cached for TEXT_TEXTURE_CACHE_TIME (1100ms by default).
*/
void drawText(std::string text, int x, int y, int s, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    text_info tInfo(text, getFontSizePos(s), r, g, b);
    auto z = text_textures.find(tInfo); //check if this is in the cache first
    SDL_Texture *__t;
    if(z != text_textures.end())
    {
        z->second.first = getTicks();
        __t = z->second.second;
    }
    else
    {
        __t = createText(text, s, r, g, b);
        text_textures[tInfo] = std::make_pair(getTicks(), __t);
    }
    SDL_Rect dst{x, y, (int)(text.size() * s/2), s};
    SDL_SetTextureAlphaMod(__t, a);
    renderCopy(__t, &dst);
}
/**
Draws wrapped text on the window, and breaks won't occur in the middle of words. The SDL_Texture is cached for TEXT_TEXTURE_CACHE_TIME.
*/
int drawMultilineTextUnbroken(std::string text, int x, int y, int w, int s, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    size_t maxLength = 2*w / s;
    int lines = 0;
    while(text.size())
    {
        std::string cur;
        for(size_t i=0; i<std::min(maxLength, text.size()); i++)
        {
            if(text[i] == '\n')
            {
                cur = text.substr(0, i);
                text.erase(0, i+1);
                break;
            }
        }
        if(cur.size() == 0)
        {
            if(text.size() <= maxLength)
            {
                cur = text;
                text.clear();
            }
            else
            {
                int l = maxLength;
                while(text[l] != ' ')
                    l--;
                cur = text.substr(0, l);
                text.erase(0, l+1);
            }
        }
        drawText(cur, x, y, s, r, g, b, a);
        y += s;
        lines++;
    }
    return lines;
}
/**
Fills in xpos and ypos with the x and y position of the last character drawn from the drawMultilineTextUnbroken function.
*/
void getMultilineTextUnbrokenInfo(std::string text, int w, int s, std::vector<std::string> &ltext)
{
    size_t maxLength = 2*w / s;
    int lines = 0;
    while(text.size())
    {
        std::string cur;
        for(size_t i=0; i<std::min(maxLength, text.size()); i++)
        {
            if(text[i] == '\n')
            {
                cur = text.substr(0, i);
                text.erase(0, i+1);
                break;
            }
        }
        if(cur.size() == 0)
        {
            if(text.size() <= maxLength)
            {
                cur = text;
                text.clear();
            }
            else
            {
                int l = maxLength;
                while(text[l] != ' ')
                    l--;
                cur = text.substr(0, l);
                text.erase(0, l+1);
            }
        }
        ltext.push_back(cur);
        /*if(xpos != NULL) //we could optimize this but whatever
            *xpos = (int)cur.size();
        if(ypos != NULL)
            *ypos = lines;
        if(xpos!=NULL && *xpos == (int)maxLength)
        {
            *xpos = 0;
            if(ypos != NULL)
                (*ypos)++;
        }*/
        lines++;
    }
}
/**
Draws wrapped text on the window. The SDL_Texture is cached for TEXT_TEXTURE_CACHE_TIME.
*/
int drawMultilineText(std::string text, int x, int y, int w, int s, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    size_t maxLength = 2*w / s;
    int lines = 0;
    while(text.size())
    {
        std::string cur;
        for(size_t i=0; i<std::min(maxLength, text.size()); i++)
        {
            if(text[i] == '\n')
            {
                cur = text.substr(0, i);
                text.erase(0, i+1);
                break;
            }
        }
        if(cur.size() == 0)
        {
            if(text.size() <= maxLength)
            {
                cur = text;
                text.clear();
            }
            else
            {
                cur = text.substr(0, maxLength);
                text.erase(0, maxLength);
            }
        }
        drawText(cur, x, y, s, r, g, b, a);
        y += s;
        lines++;
    }
    return lines;
}
/**
Fills in xpos and ypos with the x and y position of the last character drawn from the drawMultilineText function.
*/
void getMultilineTextPos(std::string text, int w, int s, int *xpos, int *ypos)
{
    size_t maxLength = 2*w / s;
    int lines = 0;
    while(text.size())
    {
        std::string cur;
        for(size_t i=0; i<std::min(maxLength, text.size()); i++)
        {
            if(text[i] == '\n')
            {
                cur = text.substr(0, i);
                text.erase(0, i+1);
                break;
            }
        }
        if(cur.size() == 0)
        {
            if(text.size() <= maxLength)
            {
                cur = text;
                text.clear();
            }
            else
            {
                cur = text.substr(0, maxLength);
                text.erase(0, maxLength);
            }
        }
        if(xpos != NULL) //we could optimize this but whatever, also note the wrapping
            *xpos = (int)cur.size();
        if(ypos != NULL)
            *ypos = lines;
        if(xpos!=NULL && *xpos == (int)maxLength)
        {
            *xpos = 0;
            if(ypos != NULL)
                (*ypos)++;
        }
        lines++;
    }
}
/**
Returns how many times a given text will be wrapped if it is drawn
*/
int multilineTextLength(std::string text, int w, int s)
{
    size_t maxLength = 2*w / s;
    int lines = 0;
    while(text.size())
    {
        std::string cur;
        if(text.size() <= maxLength)
        {
            cur = text;
            text.clear();
        }
        else
        {
            cur = text.substr(0, maxLength);
            text.erase(0, maxLength);
        }
        lines++;
    }
    return lines;
}
/**
Displays a loading screen
*/
void showLoadingScreen()
{
    using namespace sdl_settings;
    renderClear(0, 0, 0);
    drawText("Loading...", 0, 0, WINDOW_H/20, 255, 255, 255);
    SDL_RenderPresent(renderer);
}
/**
Equivalent to SDL_RenderFillRect
*/
void fillRect(SDL_Rect *x)
{
    SDL_RenderFillRect(renderer, x);
}
/**
Equivalent to SDL_RenderFillRect
*/
void fillRect(SDL_Rect *x, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    setColor(r, g, b, a);
    SDL_RenderFillRect(renderer, x);
}
/**
Equivalent to SDL_RenderFillRect
*/
void fillRect(int x, int y, int w, int h)
{
    SDL_Rect *temp = new SDL_Rect{x, y, w, h};
    SDL_RenderFillRect(renderer, temp);
    delete temp;
}
/**
Equivalent to SDL_RenderFillRect
*/
void fillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    setColor(r, g, b, a);
    fillRect(x, y, w, h);
}
/**
Equivalent to SDL_RenderDrawRect
*/
void drawRect(SDL_Rect *x)
{
    SDL_RenderDrawRect(renderer, x);
}
/**
Equivalent to SDL_RenderDrawRect
*/
void drawRect(SDL_Rect *x, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    setColor(r, g, b, a);
    SDL_RenderDrawRect(renderer, x);
}
/**
Equivalent to SDL_RenderDrawRect
*/
void drawRect(int x, int y, int w, int h)
{
    SDL_Rect temp{x, y, w, h};
    SDL_RenderDrawRect(renderer, &temp);
}
/**
Equivalent to SDL_RenderDrawRect
*/
void drawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    setColor(r, g, b, a);
    drawRect(x, y, w, h);
}
/**
Equivalent to SDL_SetTextureColorMod
*/
void setTextureColorMod(SDL_Texture *t, uint8_t r, uint8_t g, uint8_t b)
{
    SDL_SetTextureColorMod(t, r, g, b);
}
/**
Equivalent to SDL_SetTextureAlphaMod
*/
void setTextureAlphaMod(SDL_Texture *t, uint8_t a)
{
    SDL_SetTextureAlphaMod(t, a);
}
/**
Loads a SDL_Texture from an image file and color keys it
*/
SDL_Texture *loadTexture(const char *name, uint8_t r, uint8_t g, uint8_t b)
{
    SDL_Surface *s = IMG_Load(name);
    if(s == NULL)
    {
        println("IMG_GetError(): " + (std::string)SDL_GetError());
        return NULL;
    }
    SDL_SetColorKey(s, SDL_TRUE, SDL_MapRGB(s->format, r, g, b));
    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_FreeSurface(s);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    return t;
}
/**
Loads a SDL_Texture from an image file
*/
SDL_Texture *loadTexture(const char *name)
{
    SDL_Surface *s = IMG_Load(name);
    if(s == NULL)
        println("IMG_GetError(): " + (std::string)SDL_GetError());
    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_FreeSurface(s);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    return t;
}
/**
Checks if two rectangles intersect
*/
bool rectsIntersect(SDL_Rect a, SDL_Rect b)
{
    bool X = (a.x>=b.x && b.x+b.w>=a.x) || (b.x>=a.x && a.x+a.w>=b.x);
    bool Y = (a.y>=b.y && b.y+b.h>=a.y) || (b.y>=a.y && a.y+a.h>=b.y);
    return X && Y;
}
/**
Equivalent to SDL_RenderDrawLine
*/
void drawLine(int x1, int y1, int x2, int y2)
{
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}
/**
Equivalent to SDL_RenderDrawLine
*/
void drawLine(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    setColor(r, g, b, a);
    drawLine(x1, y1, x2, y2);
}
/**
Equivalent to SDL_RenderDrawPoint
*/
void drawPoint(int x, int y)
{
    SDL_RenderDrawPoint(renderer, x, y);
}
/**
Equivalent to SDL_RenderDrawPoint
*/
void drawPoint(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    setColor(r, g, b, a);
    SDL_RenderDrawPoint(renderer, x, y);
}
/**
Draws a filled circle
*/
void fillCircle(int x, int y, int r)
{
    int d = 0;
    int stop = r / sqrt(2);
    int v = 0;
    for(int i=r; i>stop; i--)
    {
        while(d*d < v)
            d++;
        drawLine(x+i, y-d, x+i, y+d); //right
        drawLine(x-i, y-d, x-i, y+d); //left
        drawLine(x-d, y+i, x+d, y+i); //up
        drawLine(x-d, y-i, x+d, y-i); //down
        v += (i + i - 1);
    }
    fillRect(x-stop, y-stop, stop*2+1, stop*2+1);
}
/**
Draws a filled circle
*/
void fillCircle(int x, int y, int rad, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    setColor(r, g, b, a);
    fillCircle(x, y, rad);
}
/**
Draws an unfilled circle
*/
void drawCircle(int x, int y, int r)
{
    int r2 = r*r + 0.000001; //taking the square root of a negative number might cacuse crashes
    for(int dX=0; dX<=r; dX++)
    {
        int dY = sqrt(r2 - dX*dX);
        drawPoint(x-dX, y-dY);
        drawPoint(x-dX, y+dY);
        drawPoint(x+dX, y-dY);
        drawPoint(x+dX, y+dY);
    }
}
/**
Draws an unfilled circle
*/
void drawCircle(int x, int y, int rad, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    setColor(r, g, b, a);
    drawCircle(x, y, rad);
}
/**
Checks if the mouse is in a given rectangle
*/
bool mouseInRect(int x, int y, int w, int h)
{
    return mouse_x>=x && mouse_x<x+w && mouse_y>=y && mouse_y<y+h;
}
/**
Checks if the mouse is in a given rectangle
*/
bool mouseInRect(SDL_Rect *x)
{
    return mouseInRect(x->x, x->y, x->w, x->h);
}
/**
Returns the x coordinate of the mouse. The mouse state is updated during the updateScreen() function.
*/
int getMouseX()
{
    return mouse_x;
}
/**
Returns the y coordinate of the mouse. The mouse state is updated during the updateScreen() function.
*/
int getMouseY()
{
    return mouse_y;
}
/**
Equivalent to SDL_RenderSetViewport
*/
void setViewport(SDL_Rect *x)
{
    SDL_RenderSetViewport(renderer, x);
}
/**
Equivalent to SDL_RenderSetViewport
*/
void setViewport(int x, int y, int w, int h)
{
    SDL_Rect r{x, y, w, h};
    SDL_RenderSetViewport(renderer, &r);
}
/*
Equivalent to SDL_RenderSetClipRect
*/
void setClipRect(SDL_Rect *x)
{
    SDL_RenderSetClipRect(renderer, x);
}
/**
Equivalent to SDL_RenderSetClipRect
*/
void setClipRect(int x, int y, int w, int h)
{
    SDL_Rect r{x, y, w, h};
    SDL_RenderSetClipRect(renderer, &r);
}
/**
Equivalent to SDL_SetWindowIcon
*/
void setWindowIcon(SDL_Surface *icon)
{
    SDL_SetWindowIcon(window, icon);
}
/**
Returns a pointer to TTF_Font of size 2^pos
*/
TTF_Font *getFont(int pos)
{
    return font[pos];
}
/**
Sets the color of the FPS counter
*/
static uint8_t fpsR = 0, fpsG = 0, fpsB = 0, fpsA = 255;
void setFPScolor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    fpsR = r;
    fpsG = g;
    fpsB = b;
    fpsA = a;
}
/**
Returns the time between this frame and the last frame
*/
int getFrameLength()
{
    return frameLength;
}
/**
Returns a pointer to the renderer
*/
SDL_Renderer *getRenderer()
{
    return renderer;
}
/**
Changes how long text textures are cached for
*/
void setTextTextureCacheTime(int ms)
{
    sdl_settings::TEXT_TEXTURE_CACHE_TIME = ms;
}
/**
Sets the quality of text rendering (-2=worst, 2=best)
*/
void setTextQuality(int q)
{
    sdl_settings::fontQuality = q;
}
/**
Sets the quality of the renderer's antialiasing (0 = worst, 2 = best)
*/
void setRendererAA(int q)
{
    sdl_settings::renderScaleQuality = q;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, to_str(q).c_str());
}
/**
Updates the screen and performs some other functions. This function is called to advance to the next frame.
*/
void updateScreen()
{
    int curTick = getTicks();
    //clear unused text SDL_Textures
    static int last_check = 0, check_interval = 1000;
    if(curTick - last_check > check_interval) //the SDL_Texture cache doesn't need to be cleared every frame
    {
        last_check = curTick;
        for(auto i = text_textures.begin(); i!=text_textures.end(); i++)
        {
            if(curTick - i->second.first > sdl_settings::TEXT_TEXTURE_CACHE_TIME)
            {
                SDL_DestroyTexture(i->second.second);
                i = text_textures.erase(i);
            }
        }
    }
    using namespace sdl_settings;
    frameTimeStamp.push(curTick); //manage FPS
    while(frameTimeStamp.size()>0 && curTick - frameTimeStamp.front() >= 1000) //count all the frame timestamps in the last second to determine FPS
        frameTimeStamp.pop();
    if((int)frameTimeStamp.size() >= FPS_CAP)
        std::this_thread::sleep_for((std::chrono::nanoseconds)(1000000000/FPS_CAP));
    if(showFPS)
        drawText(to_str(frameTimeStamp.size()) + " FPS", 0, 0, WINDOW_H/40, fpsR, fpsG, fpsB, fpsA);
    /*
    //for some reason in Windows 10, the program sometimes must be busy when minimizing or else it'll crash when restoring, which is obviously bad,
    //and sleeping for a few ms keeps it "busy." This doesn't happen all the time though... it's a bit inconsistent.
    unsigned wInfo = SDL_GetWindowFlags(window);
    //commented out because it may cause crashes
    if(wInfo & SDL_WINDOW_MINIMIZED) //reduce FPS when window is minimized
    {
        std::this_thread::sleep_for((std::chrono::milliseconds)20);
    }
    */
    frameLength = curTick - prevTick;
    prevTick = curTick;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    SDL_RenderPresent(getRenderer());
}
/**
Returns the window's width
*/
int getWindowW()
{
    int W;
    SDL_GetWindowSize(getWindow(), &W, NULL);
    return W;
}
/**
Returns the window's height
*/
int getWindowH()
{
    int H;
    SDL_GetWindowSize(getWindow(), NULL, &H);
    return H;
}
/**
Returns the window's area in pixels
*/
int getWindowArea()
{
    return getWindowW() * getWindowH();
}
/**
Returns the window's X position
*/
int getWindowX()
{
    int X;
    SDL_GetWindowPosition(getWindow(), &X, NULL);
    return X;
}
/**
Returns the window's Y position
*/
int getWindowY()
{
    int Y;
    SDL_GetWindowPosition(getWindow(), NULL, &Y);
    return Y;
}
/**
Returns a font size where lower is smaller and 0 is medium size
*/
int getFontSize(double sz)
{
    return sdl_settings::textSizeMult * std::pow(2.0, sz) * std::sqrt(getWindowW() * getWindowH()) / 40;
}
/**
Gets the width and height of a block of text
*/
void getTextSize(std::string text, int sz, int *w, int *h)
{
    /*int fsz = getFontSizePos(sz);
    TTF_SizeText(font[fsz], text.c_str(), w, h);
    if(w != NULL)
        *w = (*w * sz) >> fsz;
    if(h != NULL)
        *h = (*h * sz) >> fsz;*/
    //assume it's monospaced
    if(h != NULL)
        *h = sz;
    if(w != NULL)
        *w = text.size() * sz / 2;
}
/**
Checks if a mouse button is pressed
*/
bool isMouseButtonPressed(int button)
{
    return SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(button);
}
/**
Sets the renderer target
*/
bool setRenderTarget(SDL_Texture *t)
{
    return SDL_SetRenderTarget(renderer, t);
}
/**
Returns the intermediate texture that effectively represents the display
*/
SDL_Texture *getScreenTexture()
{
    int w = getWindowW(), h = getWindowH();
    SDL_Surface *s = SDL_CreateRGBSurface(0, w, h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
    SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGBA8888, s->pixels, s->pitch);
    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_FreeSurface(s);
    return t;
}
/**
Returns the display width
*/
int getDisplayW()
{
    SDL_DisplayMode d;
    SDL_GetCurrentDisplayMode(0, &d);
    return d.w;
}
/**
Returns the display height
*/
int getDisplayH()
{
    SDL_DisplayMode d;
    SDL_GetCurrentDisplayMode(0, &d);
    return d.h;
}
/**
Returns the display refresh rate
*/
int getDisplayHertz()
{
    SDL_DisplayMode d;
    SDL_GetCurrentDisplayMode(0, &d);
    if(d.refresh_rate == 0) //assume that it's 60 if it's unavailable
        return 60;
    return d.refresh_rate;
}
/**
Changes the base text size returned by getFontSize
*/
void setTextSizeMult(double m)
{
    sdl_settings::textSizeMult = m;
}
/**
Returns the current FPS
*/
int getFPS()
{
    return sdl_settings::frameTimeStamp.size();
}
/**
Loads a Mix_Chunk* from a file and checks for errors
*/
Mix_Chunk *loadMixChunk(const char *name)
{
    Mix_Chunk *t = Mix_LoadWAV(name);
    if(t == nullptr)
    {
        println("Error when loading audio file " + to_str(name));
        println("Mix_GetError(): " + to_str(Mix_GetError()));
    }
    return t;
}
/**
Loads a Mix_Music* from a file and checks for errors
*/
Mix_Music *loadMixMusic(const char *name)
{
    Mix_Music *t = Mix_LoadMUS(name);
    if(t == nullptr)
    {
        println("Error when loading audio file " + to_str(name));
        println("Mix_GetError(): " + to_str(Mix_GetError()));
    }
    return t;
}
/**
Sets the music (Mix_Music) volume
*/
void setMusicVolume(int v)
{
    using namespace sdl_settings;
    musicVolume = std::min(MIX_MAX_VOLUME, std::max(0, v));
    Mix_VolumeMusic(musicVolume);
}
/**
Sets sfx (Mix_Chunk) volume
*/
void setSfxVolume(int v)
{
    using namespace sdl_settings;
    sfxVolume = std::min(MIX_MAX_VOLUME, std::max(0, v));
    Mix_Volume(-1, sfxVolume);
}
