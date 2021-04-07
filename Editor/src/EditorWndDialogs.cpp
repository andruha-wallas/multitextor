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
#include "utfcpp/utf8.h"
#include "EditorWnd.h"
#include "WndManager.h"
#include "EditorApp.h"
#include "Dialogs/StdDialogs.h"


bool EditorWnd::DlgGoto(input_t cmd)
{
    LOG(DEBUG) << __FUNC__ << " not implemented";
    return true;
}

bool EditorWnd::DlgFind(input_t cmd)
{
    LOG(DEBUG) << __FUNC__ << " not implemented";
    return true;
}

bool EditorWnd::DlgReplace(input_t cmd)
{
    LOG(DEBUG) << __FUNC__ << " not implemented";
    return true;
}

bool EditorWnd::SaveAs(input_t cmd)
{
    FileDialog dlg{ FileDlgMode::Save };
    auto rc = dlg.Activate();
    if (rc == ID_OK)
    {
        try
        {
            std::filesystem::path path{ utf8::utf8to16(dlg.s_vars.path) };
            path /= utf8::utf8to16(dlg.s_vars.file);
            
            auto& app = Application::getInstance();
            auto& editorApp = reinterpret_cast<EditorApp&>(app);

            if (auto wnd = editorApp.GetEditorWnd(path))
            {
                _assert(0);
            }

/*            
            if (auto wnd = GetEditorWnd(path))
                return WndManager::getInstance().SetTopWnd(wnd);

            auto editor = std::make_shared<EditorWnd>();
            editor->Show(true, -1);
            if (editor->SetFileName(path, false, dlg.s_vars.typeName, dlg.s_vars.cpName))
                m_editors[editor.get()] = editor;
*/        
        }
        catch (...)
        {
            _assert(0);
            LOG(ERROR) << "Error file saving as";
            EditorApp::SetErrorLine("Error file saving as");
        }
    }

    return true;
}

bool EditorWnd::CtrlProperties(input_t cmd)
{
    LOG(DEBUG) << __FUNC__ << " not implemented";
    return true;
}

bool EditorWnd::CtrlChangeCP(input_t cmd)
{
    LOG(DEBUG) << __FUNC__ << " not implemented";
    return true;
}

bool EditorWnd::CtrlFuncList(input_t cmd)
{
    LOG(DEBUG) << __FUNC__ << " not implemented";
    return true;
}

bool EditorWnd::TrackPopupMenu(input_t cmd)
{
    LOG(DEBUG) << __FUNC__ << " not implemented";
    return true;
}
