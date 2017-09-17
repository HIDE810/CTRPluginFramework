#include "types.h"
#include "CTRPluginFrameworkImpl/Graphics/Renderer.hpp"
#include "CTRPluginFrameworkImpl/Graphics/PrivColor.hpp"
#include "CTRPluginFrameworkImpl/System/Screen.hpp"
#include "font6x10Linux.h"

namespace CTRPluginFramework
{
    void Renderer::DrawPixel(int posX, int posY, const Color& color)
    {
        PrivColor::ToFramebuffer(_screen->GetLeftFramebuffer(posX, posY), color);
    }

    // Draw Character without background
    void     Renderer::DrawCharacter(int c, int posX, int posY, const Color &fg)
    {
        u32 stride = _screen->GetStride();
        u32 bpp = _screen->GetBytesPerPixel();
        int posXX = posX - 10;
        int posYY = posY;

        for (int yy = 0; yy < 10; yy++)
        {
            u8  charPos = font[c * 10 + yy];
            u8  *fb = _screen->GetLeftFramebuffer(posX, posY++);

            for (int xx = 6; xx > 0; xx--)
            {
                if ((charPos >> xx) & 1)
                    PrivColor::ToFramebuffer(fb, fg);
                fb += stride;
            }
        }

        if (_screen->Is3DEnabled())
        {
            for (int yy = 0; yy < 10; yy++)
            {
                u8  charPos = font[c * 10 + yy];
                u8  *fb = _screen->GetRightFramebuffer(posXX, posYY++);

                for (int xx = 6; xx > 0; xx--)
                {
                    if ((charPos >> xx) & 1)
                        PrivColor::ToFramebuffer(fb, fg);
                    fb += stride;
                }
            }
        }
    }

    // Draw Character with background
    void     Renderer::DrawCharacter(int c, int posX, int posY, const Color &fg, const Color &bg)
    {
        u32 stride = _screen->GetStride();
        u32 bpp = _screen->GetBytesPerPixel();
        int posXX = posX - 10;
        int posYY = posY;

        for (int yy = 0; yy < 10; yy++)
        {
            u8  charPos = font[c * 10 + yy];
            u8  *fb = _screen->GetLeftFramebuffer(posX, posY++);

            for (int xx = 6; xx > 0; xx--)
            {
                PrivColor::ToFramebuffer(fb, (charPos >> xx) & 1 ? fg : bg);
                fb += stride;
            }
        }

        if (_screen->Is3DEnabled())
        {
            for (int yy = 0; yy < 10; yy++)
            {
                u8  charPos = font[c * 10 + yy];
                u8  *fb = _screen->GetRightFramebuffer(posXX, posYY++);

                for (int xx = 6; xx > 0; xx--)
                {
                    PrivColor::ToFramebuffer(fb, (charPos >> xx) & 1 ? fg : bg);
                    fb += stride;
                }
            }
        }
    }

    int    Renderer::DrawString(const char *str, int posX, int &posY, Color fg)
    {
        while (*str)
        {
            DrawCharacter(*str++, posX, posY, fg);
            posX += 6;
        }
        posY += 10;
        return (posY);
    }

    int    Renderer::DrawString(const char *str, int posX, int &posY, Color fg, Color bg)
    {
        u32 bpp = _screen->GetBytesPerPixel();

        for (int i = 0; i < 2; i++)
        {
            u8 *fb = _screen->GetLeftFramebuffer(posX + i, posY);
            for (int y = 0; y < 10; y++)
            {
                PrivColor::ToFramebuffer(fb, bg);
                fb -= bpp;
            }
        }

        if (_screen->Is3DEnabled())
        {
            for (int i = 0; i < 2; i++)
            {
                u8 *fb = _screen->GetRightFramebuffer(posX - 10 + i, posY);
                for (int y = 0; y < 10; y++)
                {
                    PrivColor::ToFramebuffer(fb, bg);
                    fb -= bpp;
                }
            }
        }

        posX += 2;
        while (*str)
        {
            DrawCharacter(*str++, posX, posY, fg, bg);
            posX += 6;
        }
        posY += 10;
        return (posY);
    }

    u32     Renderer::LinuxFontSize(const char* str)
    {
        u32 size = 0;

        for (; *str; str++)
        {
            char c = *str;
            if (c == 0x1B)
                str += 3;
            else if (c != 0x18)
                size += 6;
        }

        return (size);
    }
}
