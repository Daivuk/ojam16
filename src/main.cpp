#include <Windows.h>
#include <onut/onut.h>
#include <onut/Settings.h>
#include <onut/Font.h>
#include <onut/Timing.h>
#include <onut/Renderer.h>

void init();
void update();
void render();
void postRender();

OFontRef g_pFont;

int WINAPI WinMain(_In_ HINSTANCE hInstance,
                   _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR lpCmdLine,
                   _In_ int nShowCmd)
{
    oSettings->setIsFixedStep(true);
    oSettings->setGameName("Ottawa Game Jam 2016");
    oSettings->setResolution({800, 600});

    ORun(init, update, render, postRender);

    return 0;
}

void init()
{
    oTiming->setUpdateFps(60);
    g_pFont = OGetFont("font.fnt");
}

void update()
{
}

void render()
{
    oRenderer->clear(Color::Black);

    g_pFont->draw("FPS: " + std::to_string(oTiming->getFPS()), Vector2::Zero);
}

void postRender()
{
}
