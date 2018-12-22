#include <iostream>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <cmath>
#include "sdl_base.h"
using namespace std;
struct Image
{
    SDL_Texture *t;
    string name;
    double x, y;
    double w;
    Image(const char *file_name, string name, double x, double y, double w)
    {
        t = loadTexture(file_name, 0, 0, 0);
        SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
        this->name = name;
        for(auto &i: this->name)
            if(i == '_')
                i = ' ';
        this->x = x;
        this->y = y;
        this->w = w;
    }
};
struct Displayer
{
    vector<Image> images;
    double scale, end_scale;
    double scale_per_frame;
    bool is_paused;
    bool play()
    {
        if(is_paused)
        {
            return scale < end_scale;
        }
        scale *= scale_per_frame;
        if(scale > end_scale)
        {
            scale = end_scale;
            return true;
        }
        return false;
    }
    void render()
    {
        renderClear(0, 0, 0);
        for(int i=images.size()-1; i>=0; i--)
        {
            double w = getWindowW() * images[i].w / scale;
            int iW, iH;
            SDL_QueryTexture(images[i].t, NULL, NULL, &iW, &iH);
            double h = w * iH / iW;
            double x = getWindowW() * (images[i].x / scale + 0.5);
            double y = getWindowW() * (images[i].y / scale + getWindowH() / 2.0 / getWindowW());
            if(w<1e8 && h<1e8)
            {
                uint8_t alpha;
                if(w >= 1e5)
                    alpha = std::max(0.0, 255 - 85 * log10(w / 1e5));
                else alpha = 255;
                SDL_SetTextureAlphaMod(images[i].t, alpha);
                renderCopy(images[i].t, x, y, w, h);
                int fsz = sqrt(w * h) / 5;
                drawText(images[i].name, x, y + h - fsz, fsz, 255, 255, 255);
            }
        }
        fillRect(getWindowW() * 0.1, getWindowH() * 0.1, getWindowW() * 0.1, getWindowH() * 0.01, 255, 255, 255);
        int e = floor(log10(scale * 0.1));
        string b = to_str((int)(scale / pow(10, e)) / 10.0);
        if(b.size() == 1)
            b += '.';
        while(b.size() < 3)
            b += '0';
        drawText(b + "e" + to_str(e) + " m", getWindowW() * 0.1, getWindowH() * 0.11, getFontSize(0), 255, 255, 255);
    }
    Displayer(const char *file_name)
    {
        ifstream fin(file_name);
        string prefix;
        fin >> scale >> end_scale >> scale_per_frame >> prefix;
        string fname, name;
        is_paused = false;
        double x, y, w;
        while(!fin.eof())
        {
            fin >> fname >> name >> x >> y >> w; //h can be calculated from w
            images.emplace_back((prefix + "/" + fname).c_str(), name, x, y, w);
        }
    }
    Displayer(){}
};
int main(int argc, char **argv)
{
    sdl_settings::load_config();
    initSDL("Scale");
    atexit(SDL_Quit);
    atexit(sdl_settings::output_config);
    Displayer d;
    if(argc == 2)
        d = Displayer(argv[1]);
    else d = Displayer("data.txt");
    while(true)
    {
        while(SDL_PollEvent(&input))
        {
            switch(input.type)
            {
            case SDL_QUIT:
                exit(0);
                break;
            case SDL_KEYDOWN:
                if(input.key.keysym.sym == SDLK_SPACE)
                    d.is_paused = !d.is_paused;
                break;
            case SDL_MOUSEWHEEL:
                if(SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LSHIFT])
                    d.scale *= pow(d.scale_per_frame, -35 * input.wheel.y);
                else d.scale *= pow(d.scale_per_frame, -7 * input.wheel.y);
                break;
            }
        }
        d.play();
        d.render();
        updateScreen();
    }
    return 0;
}
