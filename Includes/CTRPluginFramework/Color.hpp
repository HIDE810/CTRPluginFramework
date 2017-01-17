#ifndef COLOR_HPP
#define COLOR_HPP

#include "types.h"

namespace CTRPluginFramework
{
    class Color
    {
    public:
        Color(void);
        Color(u32 color);
        Color(u8 red, u8 green, u8 blue, u8 alpha = 255);

        static Color ColorFromMemory(u8 *array, bool isBGR = true, bool haveAlpha = false);
        static const Color Black;       
        static const Color White;       
        static const Color Red;         
        static const Color Green;       
        static const Color Blue;       
        static const Color Yellow;      
        static const Color Magenta;     
        static const Color Cyan;        
        static const Color Transparent; 

        u32     ToU32(void);
        Color   &Fade(double fading);    

        u8      r;
        u8      g;
        u8      b;
        u8      a;
    };
}

#endif