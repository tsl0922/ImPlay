#pragma once
#include "view.h"
#include "config.h"
#include "mpv.h"

namespace ImPlay::Views {
    class Settings : public View {
    public:
        Settings(Config *config, Mpv *mpv);

        void draw() override;
        void drawGeneralTab();
        void drawFontTab();

    private:
        Config *config;
        Mpv *mpv;
    };
}