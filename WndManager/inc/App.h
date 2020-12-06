/*
FreeBSD License

Copyright (c) 2020 vikonix: valeriy.kovalev.software@gmail.com
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include "WndManager.h"
#include "Menu.h"


//////////////////////////////////////////////////////////////////////////////
enum class stat_color
{
    normal = 0,
    grayed = 1,
    error = 2
};

//0 info from begin; other from end
struct Statline
{
    std::string text;
    std::string textAlt;
    stat_color  color;
};
using sline_list = std::vector<Statline>;

enum class clock_pos
{
    off = 0,
    top = 1,
    bottom = 2
};

//////////////////////////////////////////////////////////////////////////////
class Application// : public CaptureInput
{
public:
    std::string m_pAppName;

protected:
    WndManager&                 m_wndManager;
    std::shared_ptr<LineMenu>   m_mainMenu;
    std::vector<menu_list>      m_menuArray;
    menu_list                   m_accessMenu;
    sline_list                  m_sLine;
    std::string                 m_line;

    clock_pos                   m_clock{ clock_pos::off };
    bool                        m_inited{ false };
    bool                        m_insert{ true };
    bool                        m_mouseCapture{ false };
    bool                        m_recordMacro{ false };
    time_t                      m_prevTime{};

    CaptureInput*               m_capturedInput{};
    //    KeyConv*      m_pKeyConv;

    Application() : m_wndManager{ WndManager::getInstance() } {};

public:
    Application(const Application&) = delete;
    void operator= (const Application&) = delete;

    static Application& getInstance()
    {
        static Application instance;
        return instance;
    }

    virtual ~Application() = default;
    virtual input_t AppProc(input_t code)                       { return code; } //input treatment in user function
    virtual bool    SaveCfg([[maybe_unused]] input_t code = 0)  { return true; } //configuration saving
    virtual bool    LoadCfg()                                   { return true; } //configuration loading

    bool Init();
    void Deinit();
    
    input_t MainProc(input_t exit_code = K_EXIT);//input treatment loop
    input_t CheckMouse(input_t code);
    input_t ParseCommand(input_t code);
    input_t EventProc(input_t code);

    bool             SetMenu(const std::vector<menu_list>& menu) { m_menuArray = menu; return true; }
    const menu_list& GetMenu(size_t n) { return m_menuArray[n]; }

    bool    IsInsertMode() {return m_insert;}
    bool    SetLogo(const Logo& logo);
    bool    SetAccessMenu(const menu_list& menu);
    void    SetClock(clock_pos set = clock_pos::off) {m_clock = set;}
    bool    SetStatusLine(const sline_list& line);
    bool    ChangeStatusLine(size_t n, std::optional<std::string_view> text = std::nullopt, stat_color color = stat_color::normal);
    bool    ChangeStatusLine(size_t n, color_t color);
    bool    SwapStatusLine(size_t n);
    bool    ShowProgressBar(uint16_t n = 100);
    bool    SetHelpLine(std::optional<std::string_view> help = std::nullopt, stat_color color = stat_color::normal) {return ChangeStatusLine(0, help, color);}
    bool    SetErrorLine(const std::string& error) {return ChangeStatusLine(0, error, stat_color::error);}
    bool    PrintClock(bool print = false);
    bool    PrintStatusLine();

    bool    Repaint();
    bool    Refresh();

    bool    PutCode(input_t cmd);
    bool    RecordMacro();
    bool    PutMacro(input_t cmd);
    input_t PlayMacro();
    bool    IsRecordMacro() {return m_recordMacro;}

    CaptureInput* Capture(CaptureInput* input)
    {
        CaptureInput* prev = m_capturedInput;
        m_capturedInput = input;
        return prev;
    }

    //int SetKeyConv(int* pConv);//???
    std::string GetKeyName(input_t code);
};

