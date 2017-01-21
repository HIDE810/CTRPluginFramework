#ifndef CTRPLUGINFRAMEWORK_SCREEN_HPP
#define CTRPLUGINFRAMEWORK_SCREEN_HPP

#include "CTRPluginFramework/Color.hpp"
#include "ctrulib/services/gspgpu.h"

namespace CTRPluginFramework
{
    class Screen
    {
    public:

        enum LCDSetup
        {
            WidthHeight = 0x5C,     ///< Framebuffer width & height  Lower 16 bits: width, upper 16 bits: height
            FramebufferA1 = 0x68,   ///< Framebuffer A first address For top screen, this is the left eye 3D framebuffer.
            FramebufferA2 = 0x6C,   ///< Framebuffer A second address    For top screen, this is the left eye 3D framebuffer.
            Format = 0x70,          ///< Framebuffer format  Bit0-15: framebuffer format, bit16-31: unknown
            Select = 0x78,          ///< Framebuffer select  Bit0: which framebuffer to display, bit1-7: unknown
            Stride = 0x90,          ///< Framebuffer stride  Distance in bytes between the start of two framebuffer rows (must be a multiple of 8).
            FramebufferB1 = 0x94,   ///< Framebuffer B first address For top screen, this is the right eye 3D framebuffer. Unused for bottom screen.
            FramebufferB2 = 0x98    ///< Framebuffer B second address    For top screen, this is the right eye 3D framebuffer. Unused for bottom screen.
        };

        static  Screen *Top; 
        static  Screen *Bottom;

        Screen(u32 lcdSetupInfo, u32 fillColorAddress, bool isTopScreen = false);

        void                        SetCtrulibScreen(void);

        bool                        IsTopScreen(void);
        bool                        Is3DEnabled(void);
        void                        Flash(Color &color);
        void                        Acquire(void);
        void                        SwapBuffer(bool flush = false);
        bool                        Update(void);
        GSPGPU_FramebufferFormats   GetFormat(void);
        u16                         GetWidth(void);
        u16                         GetHeight(void);
        u32                         GetStride(void);
        u32                         GetRowSize(void);
        u32                         GetBytesPerPixel(void);
        u32                         GetFramebufferSize(void);
        void                        GetLeftFramebufferRegisters(u32 *out);
        int                         Debug(int posX, int posY);

        u8                          *GetLeftFramebuffer(bool current = false);
        u8                          *GetRightFramebuffer(bool current = false);                    
        
        void                        Fade(float fade);

    private:
        friend class Renderer;
        friend void                 Initialize(void);
        static void                 Initialize(void);

        u8                          *GetLeftFramebufferP(bool current = false);
        u8                          *GetRightFramebufferP(bool current = false);

        u32                         _LCDSetup;
        u32                         _FillColor;
        u32                         _leftFramebuffersP[2];
        u32                         _leftFramebuffersV[2];
        u32                         _rightFramebuffersP[2];
        u32                         _rightFramebuffersV[2];
        u32                         _currentBuffer;
        u32                         *_currentBufferReg;
        u16                         _width;
        u16                         _height;
        u32                         _stride;
        u32                         _rowSize;
        u32                         _bytesPerPixel;
        bool                        _isTopScreen;
        GSPGPU_FramebufferFormats   _format;
    };
}

#endif