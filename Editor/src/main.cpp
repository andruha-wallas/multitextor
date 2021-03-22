/*
FreeBSD License

Copyright (c) 2020-2021 vikonix: valeriy.kovalev.software@gmail.com
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
#ifdef USE_VLD
  #include <vld.h>
#endif

#include "utils/logger.h"
#include "EditorApp.h"

#include <iconv.h>


/////////////////////////////////////////////////////////////////////////////
EditorApp app;
Application& Application::s_app{app};

int main()
{
    ConfigureLogger("m-%datetime{%Y%M%d}.log");
    LOG(INFO);
    LOG(INFO) << "Multitextor";

    iconv_t cd = iconv_open("UTF-8", "CP1251");
    iconv_close(cd);

    app.Init();
    WndManager::getInstance().SetScreenSize();

    app.SetLogo(g_logo);
    app.WriteAppName(L"Multitextor");

    app.SetMenu(g_mainMenu);
    app.SetAccessMenu(g_accessMenu);
    app.SetCmdParser(g_defaultAppKeyMap);
    app.SetClock(clock_pos::bottom);
    
    app.Refresh();
    app.MainProc(K_EXIT);

    app.Deinit();

    LOG(INFO) << "Exit";
    return 0;
}