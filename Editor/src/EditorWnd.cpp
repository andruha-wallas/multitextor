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
#include "utils/logger.h"
#include "utils/SymbolType.h"
#include "EditorWnd.h"
#include "WndManager.h"
#include "EditorApp.h"

#include <algorithm>
#include <iterator>
#include <cwctype>

std::u16string  EditorWnd::g_findStr;
bool            EditorWnd::g_noCase{};
bool            EditorWnd::g_up{};
bool            EditorWnd::g_replace{};
bool            EditorWnd::g_inSelected{};
bool            EditorWnd::g_word{};


bool EditorWnd::SetFileName(const std::filesystem::path& file, bool untitled, const std::string& parseMode)
{
    LOG(DEBUG) << "    SetFileName '" << file << "'";

    m_untitled = untitled;
    auto editor = std::make_shared<Editor>(file, parseMode);
    bool rc = editor->Load();
    if (!rc)
        return rc;
    return SetEditor(editor);
}

bool EditorWnd::SetEditor(EditorPtr editor) 
{ 
    LOG(DEBUG) << "    SetTextBuff";
    if (m_editor)
    {
        Save(K_ED(E_CTRL_SAVE));
        m_editor->UnlinkWnd(this);
        m_editor = nullptr;
    }

    m_editor = editor;
    m_editor->LinkWnd(this);

    m_saved = false;

    m_cursorx = 0;
    m_cursory = 0;

    m_xOffset = 0;
    m_firstLine = 0;
    m_sizeX = 0;
    m_sizeY = 0;

    m_selectState = select_state::no;
    m_beginX = 0;
    m_beginY = 0;
    m_endX = 0;
    m_endY = 0;

    m_foundX = 0;
    m_foundY = 0;
    m_foundSize = 0;

    Refresh();

    return true; 
}

bool EditorWnd::Refresh()
{
    LOG(DEBUG) << "    EditorWnd::Refresh " << this;

    if (!WndManager::getInstance().IsVisible(this))
        return true;

    m_sizeX = GetCSizeX();
    m_sizeY = GetCSizeY();
    //LOG(DEBUG) << "sx=" << m_sizeX << " sy=" << m_sizeY << " cx=" << m_cursorx << " cy=" << m_cursory;

    if (m_sizeX <= m_cursorx || m_sizeY <= m_cursory)
        _GotoXY(m_xOffset + m_cursorx, m_firstLine + m_cursory);

    bool rc = DrawBorder();

    if (!m_editor)
    {
        rc = Clr();
        return rc;
    }

    UpdateNameInfo();

    InvalidateRect(0, 0, m_sizeX, m_sizeY);
    Repaint();

    UpdateLexPair();

    return rc;
}

bool EditorWnd::UpdateAccessInfo()
{
    std::stringstream stream;
    stream << "[ " << GetAccessInfo() << "]";
    
    pos_t x = 0;
    if (m_border & BORDER_TOP)
        x += 2;

    WriteWnd(x, 0, stream.str(), ColorWindowInfo);
    return true;
}

bool EditorWnd::UpdateNameInfo()
{
    if (m_sizeX / 2 - 2 > 5)
    {
        pos_t x = 5;
        if (m_border & BORDER_TOP)
            x += 2;

        auto path = m_editor->GetFilePath().filename().u8string();
        path.resize(m_sizeX / 2 - 2, ' ');
        WriteWnd(x, 0, path, *m_pColorWindowTitle);
    }
    return true;
}

bool EditorWnd::UpdatePosInfo()
{
    size_t x = m_xOffset + m_cursorx + 1;
    size_t y = m_firstLine + m_cursory + 1;

    std::stringstream stream;

    //15 spaces
    if (m_sizeX > 40)
        stream << "               [" << m_editor->GetStrCount() << "] L:" << y << " C:" << x;
    else
        stream << "               L:" << y << " C:" << x;

    auto str = stream.str();

    size_t len = std::max(str.size(), m_infoStrSize);
    len = std::min({len, static_cast<size_t>(32), static_cast<size_t>(m_sizeX / 2 + 3)});

    m_infoStrSize = str.size() - 14;

    WriteWnd(m_sizeX - (pos_t)len, 0, str.substr(str.size() - len), *m_pColorWindowTitle);
    return true;
}

bool EditorWnd::_GotoXY(size_t x, size_t y, bool top)
{
    if (m_sizeX == 0 || m_sizeY == 0)
    {
        m_sizeX = GetCSizeX();
        m_sizeY = GetCSizeY();
    }

    size_t line = 0;
    size_t pos = 0;

    if (y > m_editor->GetStrCount())
        y = m_editor->GetStrCount();

    if (x == 0)
        m_cursorx = 0;
    else if (x < static_cast<size_t>(m_sizeX - 1))
        m_cursorx = static_cast<pos_t>(x);
    else
    {
        m_cursorx = m_sizeX - 5;//we will see 4 symbols
        pos = x - m_cursorx;
        if (pos >= static_cast<size_t>(m_editor->GetMaxStrLen() - m_sizeX))
        {
            pos = m_editor->GetMaxStrLen() - m_sizeX;
            m_cursorx = static_cast<pos_t>(x - pos);
        }
    }

    if (!top)
    {
        if (y <= 0)
            m_cursory = 0;
        else if (y >= m_firstLine && y < m_firstLine + m_sizeY)
        {
            line = m_firstLine;
            m_cursory = static_cast<pos_t>(y - line);
        }
        else if (y < static_cast<size_t>(m_sizeY / 2))
            m_cursory = static_cast<pos_t>(y);
        else
        {
            m_cursory = m_sizeY / 2;
            line = y - m_cursory;
        }
    }
    else
    {
        m_cursory = 0;
        line = y;
    }

    bool rc = true;
    if (m_xOffset != pos || m_firstLine != line)
    {
        m_xOffset = pos;
        m_firstLine = line;
        rc = InvalidateRect(0, 0, m_sizeX, m_sizeY);
    }

    return rc;
}

bool EditorWnd::InvalidateRect(pos_t x, pos_t y, pos_t sizex, pos_t sizey)
{
    if (0 == m_sizeX || 0 == m_sizeY)
    {
        m_sizeX = GetCSizeX();
        m_sizeY = GetCSizeY();
    }

    if (sizex == 0)
        sizex = m_sizeX;
    if (sizey == 0)
        sizey = m_sizeY;

    m_invalidate = true;

    if (m_invBeginX == m_invEndX)
    {
        m_invBeginX = x;
        m_invBeginY = y;
        m_invEndX = x + sizex;
        m_invEndY = y + sizey;
    }
    else
    {
        if (x < m_invBeginX)
            m_invBeginX = x;
        if (y < m_invBeginY)
            m_invBeginY = y;

        pos_t endx = x + sizex;
        pos_t endy = y + sizey;

        if (endx > m_invEndX)
            m_invEndX = endx;
        if (endy > m_invEndY)
            m_invEndY = endy;
    }
    return true;
}

bool EditorWnd::Invalidate(size_t line, invalidate_t type, size_t pos, size_t size)
{
    if (size == 0)
        size = m_editor->GetMaxStrLen();

    int y = static_cast<int>(line) - static_cast<int>(m_firstLine);
    int x = static_cast<int>(pos) - static_cast<int>(m_xOffset);

    int endx = x + static_cast<int>(size);
    if (endx >= m_sizeX)
        endx = m_sizeX;

    if (x < 0 && endx > 0)
        x = 0;

    //LOG(DEBUG) << "    Invalidate line=" << line << " type=" << static_cast<int>(type) << " pos=" << pos << " size=" << size << "; y=" << y << " x=" << x << " endx=" << endx;

    if (y < 0 && type != invalidate_t::find && type != invalidate_t::change)
    {
        y = 0;
        type = invalidate_t::full;
    }

    if (y < 0 || y >= m_sizeY || x < 0 || x >= m_sizeX || endx < 0)
        return true;

    switch (type)
    {
    case invalidate_t::find:
    case invalidate_t::change:
        InvalidateRect(static_cast<pos_t>(x), static_cast<pos_t>(y), static_cast<pos_t>(endx - x), 1);
        break;
    case invalidate_t::del:
        InvalidateRect(0, static_cast<pos_t>(y), m_sizeX, static_cast<pos_t>(m_sizeY - y));
        break;
    case invalidate_t::insert:
        InvalidateRect(0, static_cast<pos_t>(y), m_sizeX, static_cast<pos_t>(m_sizeY - y));
        break;
    case invalidate_t::full:
        InvalidateRect(0, 0, m_sizeX, m_sizeY);
        break;
    }

    return true;
}

bool EditorWnd::Repaint()
{
    if (!WndManager::getInstance().IsVisible(this))
        return true;

    bool rc;
    if (m_invalidate)
    {
        if (m_invBeginX > m_sizeX)
            m_invBeginX = 0;
        if (m_invEndX > m_sizeX)
            m_invEndX = m_sizeX;

        if (m_invBeginY > m_sizeY)
            m_invBeginY = 0;
        if (m_invEndY > m_sizeY)
            m_invEndY = m_sizeY;

        StopPaint();

        for (pos_t i = m_invBeginY; i < m_invEndY; ++i)
        {
            auto str = m_editor->GetStr(m_firstLine + i);// , 0, m_editor->GetMaxStrLen());//??? , 0, m_xOffset + m_invEndX);
            if (str.size() < m_xOffset + m_invEndX)
                str.resize(m_xOffset + m_invEndX, ' ');
            rc = PrintStr(m_invBeginX, i, str, m_xOffset + m_invBeginX, m_invEndX - m_invBeginX);
        }

        BeginPaint();
        rc = ShowBuff(m_invBeginX, m_invBeginY, m_invEndX - m_invBeginX, m_invEndY - m_invBeginY);

        m_invalidate = false;
        m_invBeginX = 0;
        m_invBeginY = 0;
        m_invEndX = 0;
        m_invEndY = 0;
    }

    StopPaint();
    UpdateAccessInfo();
    UpdatePosInfo();
    BeginPaint();
    //-1 for updating window caption
    rc = ShowBuff(0, static_cast<pos_t>(-1), m_sizeX, 1);

    return rc;
}

bool EditorWnd::IsNormalSelection(size_t bx, size_t by, size_t ex, size_t ey) const
{
    //true - normal selection
    //false - inverse selection
    if (by == ey)
    {
        if (bx <= ex)
            return true;
        else
            return false;
    }
    else if (by < ey)
        return true;
    else
        return false;
}

bool EditorWnd::PrintStr(pos_t x, pos_t y, const std::u16string& str, size_t offset, size_t len)
{
    size_t line = y + m_firstLine;
    auto MarkFound = [this, line]() {
        if (m_foundSize && line == m_foundY)
            //mark found
            Mark(m_foundX, m_foundY, m_foundX + m_foundSize - 1, m_foundY, ColorWindowFound);
    };

    bool rc{};
    if (!m_diff)
    {
        std::vector<color_t> colorBuff;
        rc = m_editor->GetColor(m_firstLine + y, str, colorBuff, offset + len);

        if (rc)
            rc = WriteColorStr(x, y, str.substr(offset, len), std::vector<color_t>(colorBuff.cbegin() + offset, colorBuff.cend()));
        else
            rc = WriteWStr(x, y, str.substr(offset, len));
    }
    else
    {
/*???
        if (m_diff->IsDiff(m_nDiffBuff, m_nFirstLine + y))//???
            SetTextAttr(ColorWindowDiff);
        else
            SetTextAttr(ColorWindowNotDiff);
        rc = WriteWStr(x, y, pStr + offset);
*/
    }

    if (!IsSelectVisible())
    {
        //not selected
        MarkFound();
        return true;
    }

    size_t bx, ex;
    [[maybe_unused]]select_line type;
    if (!GetSelectedPos(line, bx, ex, type))
    {
        //line not selected
        MarkFound();
        return true;
    }

    if (!m_diff)
        rc = Mark(bx, line, ex, line, ColorWindowSelect, m_selectType);
    else
        ;//???        rc = Mark(bx, posy, ex, posy, ColorWindowCurDiff, m_selectType);

    MarkFound();

    return rc;
}

bool EditorWnd::Mark(size_t bx, size_t by, size_t ex, size_t ey, color_t color, select_t selectType)
{
    //LOG(DEBUG) << "    Mark bx=" << bx << " by=" << by << " ex=" << ex << " ey=" << ey << " color=" << color << " select=" << static_cast<int>(selectType);

    //check begin and end of selection
    size_t x1, x2;
    size_t y1, y2;

    if (selectType != select_t::column)
    {
        if (IsNormalSelection(bx, by, ex, ey))
        {
            x1 = bx;
            x2 = ex;
            y1 = by;
            y2 = ey;
        }
        else
        {
            x1 = ex;
            x2 = bx;
            y1 = ey;
            y2 = by;
        }

        if (y2 < m_firstLine || y1 >= m_firstLine + m_sizeY)
            //not visible
            return true;

        if (y1 < m_firstLine)
        {
            y1 = m_firstLine;
            x1 = 0;
        }
        if (y2 >= m_firstLine + m_sizeY)
        {
            y2 = m_firstLine + m_sizeY - 1;
            x2 = m_editor->GetMaxStrLen();
        }
    }
    else
    {
        //column
        if (bx <= ex)
        {
            x1 = bx;
            x2 = ex;
        }
        else
        {
            x1 = ex;
            x2 = bx;
        }

        if (by <= ey)
        {
            y1 = by;
            y2 = ey;
        }
        else
        {
            y1 = ey;
            y2 = by;
        }

        if (y2 < m_firstLine || y1 >= m_firstLine + m_sizeY)
            //not visible
            return true;

        if (y1 < m_firstLine)
            y1 = m_firstLine;

        if (y2 >= m_firstLine + m_sizeY)
            y2 = m_firstLine + m_sizeY - 1;
    }

    //LOG(DEBUG) << "    Mark1 bx=" << x1 << " by=" << y1 << " ex=" << x2 << " ey=" << y2;

    for (size_t y = y1; y <= y2; ++y)
    {
        if (selectType == select_t::stream)
        {
            //stream
            if (y == y1)
            {
                //first line
                if (y1 == y2)
                {
                    //fill x1-x2
                    bx = x1;
                    ex = x2;
                }
                else
                {
                    //fill x1-.
                    bx = x1;
                    ex = m_editor->GetMaxStrLen();
                }
            }
            else if (y == y2)
            {
                //last line
                //fill 0-x2
                bx = 0;
                ex = x2;
            }
            else
            {
                //full line
                //fill 0-.
                bx = 0;
                ex = m_editor->GetMaxStrLen();
            }
        }
        else if (selectType == select_t::line)
        {
            //line
            //fill 0-.
            bx = 0;
            ex = m_editor->GetMaxStrLen();
        }
        else
        {
            //column
            //fill x1-x2
            bx = x1;
            ex = x2;
        }

        if (ex < m_xOffset || bx >= m_xOffset + m_sizeX)
            //not visible
            continue;
        if (bx < m_xOffset)
            bx = m_xOffset;
        if (ex >= m_xOffset + m_sizeX)
            ex = m_xOffset + m_sizeX - 1;

        if (color)
            ColorRect(static_cast<pos_t>(bx - m_xOffset), static_cast<pos_t>(y - m_firstLine), static_cast<pos_t>(ex - bx + 1), 1, color);
        else
        {
            auto str = m_editor->GetStr(y);//??? , 0, ex + 1);
            std::vector<color_t> colorBuff;
            bool rc = m_editor->GetColor(y, str, colorBuff, ex + 1);
            if (rc)
            {
                //LOG(DEBUG) << "WriteColor bx=" << bx << " ex=" << ex << " y=" << y;
                WriteColor(static_cast<pos_t>(bx - m_xOffset), static_cast<pos_t>(y - m_firstLine), std::vector<color_t>(colorBuff.cbegin() + bx, colorBuff.cend()));
            }
            else
                ColorRect(static_cast<pos_t>(bx - m_xOffset), static_cast<pos_t>(y - m_firstLine), static_cast<pos_t>(ex - bx + 1), 1, *m_pColorWindow);
        }
    }

    return true;
}

input_t EditorWnd::EventProc(input_t code)
{
    //LOG(DEBUG) << "    EditorWnd::WndProc " << std::hex << code << std::dec;
    if (code == K_TIME)
    {
        //check for file changing by external program
    /* //???
        if (!m_Timer)
        {
            m_Timer = 5 * 2;//2 sec
            int rc = m_pTBuff->CheckAccess();
            if (rc)
            {
                int mode = m_pTBuff->GetAccessInfo();
                long long size = m_pTBuff->GetSize();

                TPRINT(("File was changed mode=%c size=%lld\n", mode, size));
                if (mode == 'N' || !size)
                {
                    //���� ������
                    if (!m_fDeleted)
                    {
                        //��� ���� ��������
                        //������ ��������� ������� ������� ����
                        //� ����� ���������������
                        TPRINT(("Check deleted\n"));
                        m_fDeleted = 1;
                    }
                    else
                    {
                        m_fDeleted = 0;
                        //����� ��������� ������� �� ����
                        //���� �������, ���� �� �� � ������
                        FLoadDialog Dlg(GetObjPath(), 1);
                        int code1 = Dlg.Activate();
                        if (code1 == ID_OK)
                        {
                            //TPRINT(("...Save1\n"));
                            Save(1);
                        }
                    }
                }
                else
                {
                    m_fDeleted = 0;
                    if (!m_fLog)
                    {
                        FLoadDialog Dlg(GetObjPath());
                        int code1 = Dlg.Activate();
                        if (code1 == ID_OK)
                        {
                            //TPRINT(("...Reload1\n"));
                            Reload(0);
                        }
                    }
                    else
                    {
                        //???
                        //TPRINT(("...Reload2\n"));
                        Reload(0);
                    }
                }
            }
            else if (m_fDeleted)
            {
                m_fDeleted = 0;
                //����� ��������� ������� �� ����
                //���� �������, ���� �� �� � ������
                FLoadDialog Dlg(GetObjPath(), 1);
                int code1 = Dlg.Activate();
                if (code1 == ID_OK)
                {
                    //TPRINT(("...Save2\n"));
                    Save(1);
                }
            }
        }
        --m_Timer;
*/
    }

    if ( code != K_TIME
      && code != K_EXIT
      && code != K_RELEASE + K_SHIFT
      && code != (K_ED(E_CTRL_FIND) | 1) 
      && !m_close)
        //hide search if Dialog window is visible
        HideFound();

    auto scan = m_cmdParser.ScanKey(code);
    if (scan == scancmd_t::wait_next)
        InputCapture();
    else if (scan == scancmd_t::not_found)
        ParseCommand(code);
    else
    {
        InputRelease();
        auto cmdlist = m_cmdParser.GetCommand();
        for(auto cmd : cmdlist)
            ParseCommand(cmd);
    }

    if (code != K_TIME
     && code != K_FOCUSLOST
     && code != K_FOCUSSET)
    {
        Repaint();

        UpdateLexPair();
        m_editor->SetUndoRemark("");

        //refresh all view
        m_editor->RefreshAllWnd(this);
    }

    if (m_close)
        Wnd::Destroy();

    return 0;
}


input_t EditorWnd::ParseCommand(input_t cmd)
{
    if (cmd == K_TIME)
        return 0;
    else if (0 != (cmd & K_MOUSE))
    {
        pos_t x = K_GET_X(cmd);
        pos_t y = K_GET_Y(cmd);
        ScreenToClient(x, y);
        cmd = K_MAKE_COORD_CODE((cmd & ~K_CODEMASK), x, y);

        //mouse event
        if ((cmd & K_TYPEMASK) == K_MOUSE)
            return 0;
        else if ((cmd & K_TYPEMASK) == K_MOUSEKUP)
        {
            m_selectMouse = false;
            InputRelease();
            MovePos(cmd);
            SelectEnd(cmd);

            if (m_popupMenu)
            {
                m_popupMenu = true;
                TrackPopupMenu(0);
            }
        }
        else
        {
            InputCapture();
            m_selectMouse = true;
            if ((cmd & K_TYPEMASK) == K_MOUSEKL)
            {
                //mouse left key
                if ((cmd & K_MODMASK) == K_MOUSE2)
                {
                    SelectWord(cmd);
                }
                else if ((cmd & K_MODMASK) == K_MOUSE3)
                {
                    SelectLine(cmd);
                }
                else
                {
                    MovePos(cmd);
                    SelectBegin(0);
                    if (m_selectState == select_state::begin)
                    {
                        if (0 != (cmd & K_CTRL))
                            m_selectType = select_t::line;
                        else if (0 != (cmd & K_SHIFT))
                            m_selectType = select_t::column;
                    }
                }
            }
            else if ((cmd & K_TYPEMASK) == K_MOUSEKR && (cmd & (K_SHIFT | K_CTRL | K_ALT)) == 0)
            {
                m_popupMenu = true;
            }
        }
    }
    else if ((cmd & K_TYPEMASK) == 0 && ((cmd & K_MODMASK) & ~K_SHIFT) == 0)
    {
        if (m_selectMouse)
            return 0;

        PutMacro(cmd);

        SelectEnd(cmd);
        switch (cmd & K_CODEMASK)
        {
        case K_ESC:
            break;
        case K_ENTER:
            EditEnter(cmd);
            break;
        case K_TAB:
            EditTab(cmd);
            break;
        case K_BS:
            EditBS(cmd);
            break;
        default:
            EditC(cmd);
            break;
        }
    }
    else if(0 != (cmd &  EDITOR_CMD))
    {
        EditorCmd ecmd = static_cast<EditorCmd>((cmd - EDITOR_CMD) >> 16);

        if (m_selectMouse)
        {
            LOG(DEBUG) << "Mouse select for " << ecmd;
            if (ecmd == E_SELECT_MODE)
            {
                m_selectMouse = false;
                SelectEnd(cmd);
            }
            return 0;
        }

        auto it = s_funcMap.find(ecmd);
        if(it == s_funcMap.end())
        {
            LOG(ERROR) << "    ??? editor cmd=" << std::hex << cmd << std::dec;
        }
        else
        {
            PutMacro(cmd);

            auto& [func, select] = it->second;

            if (select == select_state::end)
            {
                SelectEnd(cmd);
            }

            [[maybe_unused]]bool rc = func(this, cmd);

            if (select == select_state::begin && IsSelectStarted())
            {
                SelectBegin(cmd);
            }
        }
    }
    else
    {
        if(cmd != K_FOCUSLOST && cmd != K_FOCUSSET)
            LOG(DEBUG) << "    ??? editor code=" << std::hex << cmd << std::dec;
    }

    return 0;
}

bool EditorWnd::HideFound()
{
    bool rc = true;
    if (m_lexX >= 0 && m_lexY >= 0)
    {
        LOG(DEBUG) << "    HideLex x=" << m_lexX << " y=" << m_lexY;
        if ( static_cast<size_t>(m_lexY) >= m_firstLine && static_cast<size_t>(m_lexY) < m_firstLine + m_sizeY
          && static_cast<size_t>(m_lexX) >= m_xOffset   && static_cast<size_t>(m_lexX) < m_xOffset + m_sizeX)
        {
            //if visible
            auto str = m_editor->GetStr(m_lexY);//??? , 0, m_lexX + 1);
            rc = PrintStr(static_cast<pos_t>(m_lexX - m_xOffset), static_cast<pos_t>(m_lexY - m_firstLine), str, m_lexX, 1);
        }

        m_lexX = -1;
        m_lexY = -1;
    }

    if (m_foundSize)
    {
        LOG(DEBUG) << "    HideFound x=" << m_foundX << " y=" << m_foundY << " size=" << m_foundSize;
        size_t size = m_foundSize;
        m_foundSize = 0;

        if (m_foundY >= m_firstLine && m_foundY < m_firstLine + m_sizeY)
        {
            auto str = m_editor->GetStr(m_foundY);//??? , 0, m_xOffset + m_foundX);

            int x = static_cast<int>(m_foundX) - static_cast<int>(m_xOffset);
            if (x < 0)
            {
                size += x;
                x = 0;
            }

            if (static_cast<size_t>(x + size) > static_cast<size_t>(m_sizeX))
            {
                size -= x + size - m_sizeX;
            }

            if (size > 0)
                rc = PrintStr(static_cast<pos_t>(x), static_cast<pos_t>(m_foundY - m_firstLine), str, x + m_xOffset, size);
        }
    }
    return rc;
}

bool EditorWnd::SelectClear()
{
    if (m_selectState != select_state::no)
    {
        EditorApp::StatusMark();

        m_selectState = select_state::no;
        m_beginX = m_endX = 0;
        m_beginY = m_endY = 0;
        m_selectType = select_t::stream;
    }

    return 0;
}

bool EditorWnd::FindWord(const std::u16string& str, size_t& begin, size_t& end)
{
    size_t x = m_xOffset + m_cursorx;
    if (str[x] == ' ')
        return false;

    auto type = GetSymbolType(str[x]);

    int b = static_cast<int>(x);
    for (; b >= 0; --b)
        if (GetSymbolType(str[b]) != type)
            break;
    ++b;

    size_t e;
    for (e = x; e < m_editor->GetMaxStrLen(); ++e)
        if (GetSymbolType(str[e]) != type)
            break;
    --e;

    begin = static_cast<size_t>(b);
    end = e;
    
    return true;
}

bool EditorWnd::ChangeSelected(select_change type, size_t line, size_t pos, size_t size)
{
    if (m_selectState != select_state::complete)
        return true;

    LOG(DEBUG) << "    ChangeSelected type=" << static_cast<int>(type);
    CorrectSelection();

    switch (type)
    {
    case select_change::clear: //clear
        m_beginX = m_endX = 0;
        m_beginY = m_endY = 0;
        m_selectType = select_t::stream;
        m_selectState = select_state::no;
        break;

    case select_change::split_str:
        if (line == m_beginY)
        {
            if (m_selectType != select_t::line)//stream column
            {
                if (pos <= m_beginX)
                {
                    ++m_beginY;
                    if (m_selectType == select_t::stream)
                        m_beginX += size - pos;
                }
            }
        }
        else if (line < m_beginY)
            ++m_beginY;

        if (line == m_endY)
        {
            if (m_selectType != select_t::line)//stream column
            {
                if (pos <= m_endX)
                {
                    ++m_endY;
                    if (m_selectType == select_t::stream)
                        m_endX += size - pos;
                }
            }
            else
                ++m_endY;
        }
        else if (line < m_endY)
            ++m_endY;
        break;

    case select_change::merge_str:
        if (line < m_beginY)
            --m_beginY;

        if (line < m_endY)
            --m_endY;

        if (m_selectType == select_t::stream)
        {
            size = m_endX - m_beginX;
            m_beginX = pos;
            m_endX = pos + size;
        }
        break;

    case select_change::insert_str:
        if (line <= m_beginY)
            ++m_beginY;

        if (line <= m_endY)
            ++m_endY;
        break;

    case select_change::delete_str:
        if (line == m_beginY && line == m_endY)
        {
            //delete marked line
            m_beginX = m_endX = 0;
            m_beginY = m_endY = 0;
            m_selectType = select_t::stream;
            m_selectState = select_state::no;
            LOG(DEBUG) << "End mark";
        }
        else
        {
            if (line < m_beginY)
                --m_beginY;

            if (line <= m_endY)
                --m_endY;
        }
        break;

    case select_change::insert_ch:
        if (m_selectType == select_t::stream)
        {
            if (line == m_beginY)
                if (pos <= m_beginX)
                    m_beginX += size;

            if (line == m_endY)
                if (pos <= m_endX)
                    m_endX += size;
        }
        break;

    case select_change::delete_ch:
        if (m_selectType == select_t::stream)
        {
            if (line == m_beginY && line == m_endY
                && pos <= m_beginX && pos + size >= m_endX)
            {
                //delete marked ch
                m_beginX = m_endX = 0;
                m_beginY = m_endY = 0;
                m_selectType = select_t::stream;
                m_selectState = select_state::no;
                InvalidateRect();
                LOG(DEBUG) << "End mark";
            }
            else
            {
                if (line == m_beginY && pos <= m_beginX)
                {
                    if (pos + size < m_beginX)
                        m_beginX -= size;
                    else
                        m_beginX = pos;
                }

                if (line == m_endY && pos <= m_endX)
                {
                    if (pos + size <= m_endX)
                        m_endX -= size;
                    else
                        m_endX = pos - 1;
                }
            }
        }
        break;
    }

    return true;
}

bool EditorWnd::CorrectSelection()
{
    bool rev{};
    if (m_beginY > m_endY)
    {
        rev = true;
        size_t y = m_beginY;
        m_beginY = m_endY;
        m_endY = y;
    }

    if (m_selectType == select_t::stream)
    {
        if (rev || (m_beginY == m_endY && m_beginX > m_endX))
        {
            size_t x = m_beginX;
            m_beginX = m_endX;
            m_endX = x;
        }
    }
    else if (m_selectType == select_t::line)
    {
        m_beginX = 0;
        m_endX = m_editor->GetMaxStrLen();
    }
    else
    {
        //column
        if (m_beginX > m_endX)
        {
            size_t x = m_beginX;
            m_beginX = m_endX;
            m_endX = x;
        }
    }

    return true;
}

bool EditorWnd::GetSelectedPos(size_t line, size_t& begin, size_t& end, select_line& type) const
{
    //check begin and end of selection
    size_t x1, x2;
    size_t y1, y2;

    if (IsNormalSelection(m_beginX, m_beginY, m_endX, m_endY))
    {
        x1 = m_beginX;
        x2 = m_endX;
        y1 = m_beginY;
        y2 = m_endY;
    }
    else
    {
        x1 = m_endX;
        x2 = m_beginX;
        y1 = m_endY;
        y2 = m_beginY;
    }

    if (line < y1 || line > y2)
        return false;

    if (m_selectType == select_t::stream)
    {
        //stream
        if (line == y1)
        {
            //first line
            if (y1 == y2)
            {
                //[x1,x2]
                begin = x1;
                end = x2;
                type = select_line::substr;
            }
            else
            {
                //[x1,).
                begin = x1;
                end = m_editor->GetMaxStrLen();
                type = select_line::end;
            }
        }
        else if (line == y2)
        {
            //last line (,x2]
            begin = 0;
            end = x2;
            type = select_line::begin;
        }
        else
        {
            //[0,)
            begin = 0;
            end = m_editor->GetMaxStrLen();
            type = select_line::full;
        }
    }
    else if (m_selectType == select_t::line)
    {
        //line [0,)
        begin = 0;
        end = m_editor->GetMaxStrLen();
        type = select_line::full;
    }
    else
    {
        //columns [x1,x2]
        if (x1 > x2)
            std::swap(x1, x2);

        begin = x1;
        end = x2;
        type = select_line::substr;
    }

    return true;
}

bool EditorWnd::InsertStr(const std::u16string& str, size_t y, bool save)
{
    if (m_readOnly)
        return true;

    LOG(DEBUG) << "    InsertStr";
    if (y == STR_NOTDEFINED)
        y = m_firstLine + m_cursory;

    bool rc = m_editor->AddLine(save, y, str);
    if (!rc)
    {
        //something wrong
        Beep();
    }

    return rc;
}

bool EditorWnd::CopySelected(std::vector<std::u16string>& strArray, select_t& selType)
{
    LOG(DEBUG) << "    CopySelected";

    CorrectSelection();
    strArray.clear();

    size_t n = m_endY - m_beginY;
    for (size_t i = 0; i <= n; ++i)
    {
        size_t bx, ex;
        [[maybe_unused]] select_line type;
        GetSelectedPos(m_beginY + i, bx, ex, type);

        size_t srcY = m_beginY + i;
        size_t size = ex - bx + 1;
        
        auto str = m_editor->GetStr(srcY, bx, size);
        strArray.push_back(str.substr(0, Editor::UStrLen(str)));
    }

    selType = m_selectType;

    return true;
}

bool EditorWnd::PasteSelected(const std::vector<std::u16string>& strArray, select_t selType)
{
    if (m_readOnly)
        return true;

    LOG(DEBUG) << "    PasteSelected";

    size_t posX = m_xOffset + m_cursorx;
    size_t posY = m_firstLine + m_cursory;

    EditCmd edit { cmd_t::CMD_BEGIN, posY, posX };
    EditCmd undo { cmd_t::CMD_BEGIN, posY, posX };
    m_editor->SetUndoRemark("Paste block");
    m_editor->AddUndoCommand(edit, undo);

    size_t n = strArray.size();
    bool save{true};

    bool rc{true};
    size_t bx1, ex1{};
    for(size_t i = 0; i < n; ++i)
    {
        const auto& str = strArray[i];

        int copyLine{};

        if (selType == select_t::stream)
        {
            if (n == 1)
            {
                //only 1 string
                bx1 = posX;
                ex1 = posX + str.size() - 1;
            }
            else if (i == 0)
            {
                //first line
                bx1 = posX;
                ex1 = m_editor->GetMaxStrLen() - 1;
                copyLine = 1;
            }
            else if (i == n - 1)
            {
                //last line
                bx1 = 0;
                ex1 = str.size() - 1;
            }
            else
            {
                bx1 = 0;
                ex1 = m_editor->GetMaxStrLen() - 1;
                copyLine = 2;
            }
        }
        else if (selType == select_t::line)
        {
            bx1 = 0;
            ex1 = m_editor->GetMaxStrLen() - 1;
            copyLine = 2;
        }
        else
        {
            bx1 = posX;
            ex1 = posX + str.size() - 1;
        }

        if (static_cast<int>(ex1) < 0)//???
            ex1 = 0;

        size_t dstY = posY + i;

        if (copyLine == 1)
        {
            LOG(DEBUG) << "     Copy first line dy=" << dstY;
            //split and change first line
            if (!bx1)
            {
                rc = InsertStr(str, dstY);
            }
            else
            {
                //split line
                rc = m_editor->SplitLine(save, dstY, bx1);
                rc = m_editor->AddSubstr(save, dstY, bx1, str);
            }
        }
        else if (copyLine == 2)
        {
            LOG(DEBUG) << "     Copy line dy=" << dstY;
            //insert full line
            rc = InsertStr(str, dstY, save);
        }
        else
        {
            LOG(DEBUG) << "     Copy substr dx=" << bx1 << " dy=" << dstY;
            //change line
            m_editor->AddSubstr(save, dstY, bx1, str);
        }
    }

    edit.command = cmd_t::CMD_END;
    undo.command = cmd_t::CMD_END;
    m_editor->AddUndoCommand(edit, undo);

    //new selection
    m_beginX = posX;
    m_beginY = posY;
    m_endX = ex1;
    m_endY = posY + n - 1;
    m_selectType = selType;
    m_selectState = select_state::complete;

    InvalidateRect();

    return rc;
}

bool EditorWnd::DelSelected()
{
    if (m_readOnly)
        return true;

    LOG(DEBUG) << "    DelSelected";

    CorrectSelection();

    EditCmd edit { cmd_t::CMD_BEGIN, m_beginY, m_beginX };
    EditCmd undo { cmd_t::CMD_BEGIN, m_beginY, m_beginX };
    m_editor->SetUndoRemark("Delete block");
    m_editor->AddUndoCommand(edit, undo);

    size_t n = m_endY - m_beginY;
    bool save{true};
    bool rc{true};

    size_t bx, ex;
    size_t dy = 0;

    for (size_t i = 0; i <= n; ++i)
    {
        select_line type;
        GetSelectedPos(m_beginY + i, bx, ex, type);

        size_t srcY = m_beginY + i;
        size_t size = ex - bx + 1;

        if (type == select_line::end)
        {
            LOG(DEBUG) << "     Del first line dy=" << m_beginY;
            dy = 1;
            rc = m_editor->DelEnd(save, m_beginY, bx);
            auto str = m_editor->GetStr(m_beginY + n, m_endX + 1);
            rc = m_editor->ChangeSubstr(save, m_beginY, bx, str);
        }
        else if (type != select_line::substr)
        {
            LOG(DEBUG) << "     Del line dy=" << m_beginY + dy;
            //del full line
            rc = m_editor->DelLine(save, m_beginY + dy);
        }
        else
        {
            LOG(DEBUG) << "     Del substr dx=" << bx << " dy=" << srcY;
            //change line
            rc = m_editor->DelSubstr(save, srcY, bx, size);
        }
    }

    edit.command = cmd_t::CMD_END;
    undo.command = cmd_t::CMD_END;
    m_editor->AddUndoCommand(edit, undo);

    //correct cursor position
    size_t x = m_xOffset + m_cursorx;
    size_t y = m_firstLine + m_cursory;

    if (y >= m_beginY)
    {
        if (m_selectType == select_t::stream)
        {
            //stream
            if (y == m_beginY && m_beginY == m_endY)
            {
                //only substr
                if (x >= m_beginX)
                {
                    if (x <= m_endX)
                        x = m_beginX;
                    else
                        x -= m_endX - m_beginX + 1;
                }
            }
            else  if (y == m_beginY)
            {
                //first line
                if (x >= m_beginX)
                    x = m_beginX;
            }
            else if (y == m_endY)
            {
                //last line
                if (x <= m_endX)
                    x = m_beginX;
                else
                    x = m_beginX + x - m_endX - 1;
            }

            if (y <= m_endY)
                y = m_beginY;
            else
                y -= m_endY - m_beginY;
        }
        else if (m_selectType == select_t::line)
        {
            //line
            if (y <= m_endY)
                y = m_beginY;
            else
                y -= m_endY - m_beginY + 1;
        }
        else
        {
            //column
            if (y <= m_endY && y >= m_beginY)
                if (x >= m_beginX)
                {
                    if (x <= m_endX)
                        x = m_beginX;
                    else
                        x -= m_endX - m_beginX + 1;
                }
        }
    }

    int cx = static_cast<int>(x - m_xOffset);
    if (cx < 0 || cx >= m_sizeX)
        cx = -1;

    int cy = static_cast<int>(y - m_firstLine);
    if (cy < 0 || cy >= m_sizeY)
        cy = -1;

    if (cx < 0 || cy < 0)
        rc = _GotoXY(x, y);
    else
    {
        m_cursorx = static_cast<pos_t>(cx);
        m_cursory = static_cast<pos_t>(cy);
    }

    //del mark
    ChangeSelected(select_change::clear);
    InvalidateRect();

    return rc;
}

bool EditorWnd::UpdateLexPair()
{
    if (m_diff)
        return true;

    size_t x = m_xOffset + m_cursorx;
    size_t y = m_firstLine + m_cursory;
    if (m_editor->CheckLexPair(y, x))
    {
        //TPRINT(("Match pair x=%d y=%d\n", x, y));

        if (x >= m_xOffset   && x < m_xOffset + m_sizeX
         && y >= m_firstLine && y < m_firstLine + m_sizeY)
        {
            //if visible
            m_lexX = static_cast<int>(x);
            m_lexY = static_cast<int>(y);
            
            size_t bx, ex;
            select_line type;
            bool sel = GetSelectedPos(y, bx, ex, type);
            if(sel && bx <= x && x <= ex)
                Mark(x, y, x, y, ColorWindowSelectLMatch);
            else
                Mark(x, y, x, y, ColorWindowLMatch);
        }
        else
        {
            m_lexX = -1;
            m_lexY = -1;
        }
    }
    else
    {
        m_lexX = -1;
        m_lexY = -1;
    }

    return true;
}

bool EditorWnd::GetWord(std::u16string& str)
{
    size_t y = m_firstLine + m_cursory;
    auto curStr = m_editor->GetStr(y);
    if (curStr.empty())
        return false;

    str.clear();
    if (m_selectState == select_state::complete && m_selectType != select_t::line)
    {
        if (y == m_beginY && y == m_endY && m_beginX != m_endX)
        {
            CorrectSelection();

            for (size_t i = m_beginX; i <= m_endX; ++i)
            {
                auto c = curStr[i];
                str += c != 0x9 ? c : ' ';
            }

            return true;
        }
    }

    size_t b, e;
    if (!FindWord(curStr, b, e))
        return false;

    for (size_t i = b; i <= e; ++i)
    {
        auto c = curStr[m_beginX + i];
        str += c != 0x9 ? c : ' ';
    }

    return true;
}

bool EditorWnd::UpdateProgress(size_t step)
{
    if (m_progress != step)
    {
        std::stringstream sstr;
        sstr << "[" << std::setw(2) << step << "]";
        
        pos_t x{};
        if (m_border & BORDER_TOP)
            x += 2;

        WriteWnd(x, 0, sstr.str(), ColorWindowInfo);
        EditorApp::ShowProgressBar(step);

        WndManager::getInstance().Flush();

        m_progress = step;
        return CheckInput();
    }

    return false;
}

bool EditorWnd::FindUp(bool silence)
{
    if (g_findStr.empty())
    {
        EditorApp::SetErrorLine("Nothing to find");
        return false;
    }

    if (!silence)
        EditorApp::SetHelpLine("Search. Press any key for cancel");

    time_t t{ time(NULL) };
    std::u16string find{ g_findStr };
    if (g_noCase)
    {
        std::transform(find.begin(), find.end(), find.begin(),
            [](char16_t c) { return std::towupper(c); }
        );
    }
    size_t size = find.size();

    //search diaps
    size_t line{ m_firstLine + m_cursory };
    size_t end{};
    if (g_inSelected && m_selectState == select_state::complete)
    {
        if (m_beginY <= m_endY)
        {
            if (line > m_endY)
                line = m_endY;
            end = m_beginY;
        }
        else
        {
            if (line > m_beginY)
                line = m_beginY;
            end = m_endY;
        }
    }

    size_t offset{ m_xOffset + m_cursorx };
    if (offset == 0)
    {
        if (line)
            --line;
        else
        {
            EditorApp::SetErrorLine("Nothing to find");
            return false;
        }
    }

    size_t begin{ line };
    size_t progress{};
    bool userBreak{};
    while (line >= end)
    {
        auto str = m_editor->GetStr(line);
        for (auto it = str.begin(); it < str.end(); ++it)
        {
            if (*it == 0x9)
                *it = ' ';
            else if (g_noCase)
                *it = std::towupper(*it);
        }

        auto itBegin = offset ? std::next(str.rbegin(), str.size() - offset) : str.rbegin();
        offset = 0;
        if (auto itFound = std::search(itBegin, str.rend(), std::boyer_moore_horspool_searcher(find.rbegin(), find.rend())); 
            itFound != str.rend())
        {
            offset = std::distance(itFound, str.rend()) - size;
            //???if (!g_word || IsWord(pStr, offset, n))
            {
                LOG_IF(time(NULL) - t, DEBUG) << "    Found time=" << time(NULL) - t;
                _GotoXY(offset, line);

                m_foundX = offset;
                m_foundY = line;
                if (offset - m_xOffset + size < static_cast<size_t>(m_sizeX))
                    m_foundSize = size;
                else
                    m_foundSize = m_sizeX - (offset - m_xOffset);

                Invalidate(m_foundY, invalidate_t::find, m_foundX, m_foundSize);

                if (!silence)
                    EditorApp::SetHelpLine();
                return true;
            }
        }

        if (line)
            --line;
        else
            break;

        if (++progress == 1000)
        {
            progress = 0;
            if (userBreak = UpdateProgress((begin - line) * 99 / begin); userBreak)
                break;
        }
    }

    if (!silence)
    {
        HideFound();
        if (!userBreak)
            EditorApp::SetErrorLine("String not found");
        else
            EditorApp::SetHelpLine("User abort", stat_color::grayed);
    }

    return false;
}

bool EditorWnd::FindDown(bool silence)
{
    if (g_findStr.empty())
    {
        EditorApp::SetErrorLine("Nothing to find");
        return false;
    }

    if (!silence)
        EditorApp::SetHelpLine("Search. Press any key for cancel");

    time_t t{ time(NULL) };
    std::u16string find{ g_findStr };
    if (g_noCase)
    {
        std::transform(find.begin(), find.end(), find.begin(),
            [](char16_t c) { return std::towupper(c); }
        );
    }
    size_t size = find.size();

    //search diaps
    size_t line{ m_firstLine + m_cursory };
    size_t end{m_editor->GetStrCount()};
    if (g_inSelected && m_selectState == select_state::complete)
    {
        if (m_beginY <= m_endY)
        {
            if (line < m_beginY)
                line = m_beginY;
            end = m_endY;
        }
        else
        {
            if (line < m_endY)
                line = m_endY;
            end = m_beginY;
        }
    }

    size_t offset{ m_xOffset + m_cursorx };
    if (m_foundY == line && m_foundX == offset)
        offset += size;
    else
        ++offset;

    size_t begin{ line };
    size_t progress{};
    bool userBreak{};
    while (line < end)
    {
        auto str = m_editor->GetStr(line);
        for (auto it = str.begin(); it < str.end(); ++it)
        {
            if (*it == 0x9)
                *it = ' ';
            else if (g_noCase)
                *it = std::towupper(*it);
        }

        auto itBegin = offset ? std::next(str.begin(), offset) : str.begin();
        offset = 0;
        if (auto itFound = std::search(itBegin, str.end(), std::boyer_moore_horspool_searcher(find.begin(), find.end()));
            itFound != str.end())
        {
            offset = std::distance(str.begin(), itFound);
            //???if (!g_word || IsWord(pStr, offset, n))
            {
                LOG_IF(time(NULL) - t, DEBUG) << "    Found time=" << time(NULL) - t;
                _GotoXY(offset, line);

                m_foundX = offset;
                m_foundY = line;
                if (offset - m_xOffset + size < static_cast<size_t>(m_sizeX))
                    m_foundSize = size;
                else
                    m_foundSize = m_sizeX - (offset - m_xOffset);

                Invalidate(m_foundY, invalidate_t::find, m_foundX, m_foundSize);

                if (!silence)
                    EditorApp::SetHelpLine();
                return true;
            }
        }

        ++line;

        if (++progress == 1000)
        {
            progress = 0;
            if (userBreak = UpdateProgress((line - begin) * 99 / (end - begin)); userBreak)
                break;
        }
    }

    if (!silence)
    {
        HideFound();
        if (!userBreak)
            EditorApp::SetErrorLine("String not found");
        else
            EditorApp::SetHelpLine("User abort", stat_color::grayed);
    }

    return false;
}

#if 0
int WndEdit::FindDown(int fSilence)
{
    TPRINT(("    FindDown for %s\n", g_sFind));
    if (!g_sFind[0])
    {
        TPRINT(("    Nothing to find\n"));
        SetErrorLine(STR_S(SS_NothingToFind));
        return 0;
    }

    if (!fSilence)
        SetHelpLine(STR_S(SS_SearchPressAnyKeyForCancel));

    int t = time(NULL);
    int fBreak = 0;

    static wchar sFind[MAX_STRLEN + 1];
    int n = 0; //����� ������ ������
    int cs = 0; //����������� ����� ������ ������

    //��������� � ������ � ������ ��������
    char* pSrc = g_sFind;
    wchar* pDest = sFind;
    while (*pSrc)
    {
        wchar wc = char2wchar(g_textCP, *pSrc++);
        if (g_fCaseSensitive)
            cs += wc;
        else
            cs += wc = wc2upper(wc);

        *pDest++ = wc;
        ++n;
    }
    *pDest = 0;
    int n2 = n * 2;

    /*
      ����� ������� �������, ������������ � ��������� ������ - ����,
      �� ����� ���������� ��� ���������� ��������, ��, ����� ������ �������� �������
      �� ��������� � ������ �������, ��� ��� ����� ����� ����� � �������� ASCII �
      ��� ������� ������ � ��������� ���������, �� ���������� ����������� �������.
      ������������� � ��������� ������ ��� ������ ����� ���� ������ �����������.

      ����� ������� ���������� x � y [ i , i + m - 1 ], ����� ������ - �� ����� 1.
      ����� �������, ������ y [ i + m ] ����������� ����� �������� � ���������
      �������, � ������, ����� ���� ����������� � ������� ������� ��� ������ �������
      �������. ������������ ������� ������� �������, ����� ������� � ������
      ��������� ������ �:

      bc[ a ] = min { j | 0 <= j <= m � x[ m - 1 - j ] = a },
        ���� a ����������� � x,
      bc[ a ] = m
        � ��������������� ������.

      void QS( char* y, char* x, int n, int m)
      {
        int i, qs_bc[ ASIZE ];
        // Preprocessing
        for ( i = 0; i < ASIZE; i++ )
          qs_bc[ i ] = m + 1;
        for ( i = 0; i < m; i ++ )
          qs_bc[ x[i] ] = m - i;

        // Searching
        i = 0;
        while ( i <= n - m )
        {
          if ( memcmp( &y[i], x, m ) == 0 )
            OUTPUT( i );

          // shift
          i += qs_bc[ y[ i + m ] ];
        }
      }
    */

    //#define USE_BCS

#ifdef USE_BCS
    unsigned char* pBadCharShift = new unsigned char[0x10000];
    if (!pBadCharShift)
    {
        TPRINT(("    ERROR no memory for BC\n"));
        return 0;
    }
    memset(pBadCharShift, n, 0x10000);
    for (int i = 0; i < n; ++i)
        pBadCharShift[sFind[i]] = n - i;
#endif

    //set search diaps
    int l = m_nFirstLine + m_cursory;//cur line
    int end = m_pTBuff->GetStrCount();
    if (g_fInMarked && m_nSelectState == 0x103)
    {
        if (m_nBeginY <= m_nEndY)
        {
            if (l < m_nBeginY)
                l = m_nBeginY;
            end = m_nEndY;
        }
        else
        {
            if (l < m_nEndY)
                l = m_nEndY;
            end = m_nBeginY;
        }
    }

    int Begin = l;
    int offset = m_nXOffset + m_cursorx + 1;

    int progress = 0;
    while (l <= end)
    {
        wchar* pStr = m_pTBuff->GetStr(l, 0, MAX_STRLEN);
        int slen = m_pTBuff->GetStrLen(pStr);
        static wchar sStr[MAX_STRLEN + 1];

        int i = 0;
        if (!g_fCaseSensitive)
            for (; i < slen; ++i)
                sStr[i] = pStr[i] != 0x9 ? wc2upper(pStr[i]) : ' ';
        else
            for (; i < slen; ++i)
                sStr[i] = pStr[i] != 0x9 ? pStr[i] : ' ';
        sStr[i] = 0;
        pStr = sStr;

        wchar* pEndStr = pStr + offset;
        int cs1 = 0;
        if (offset + n <= slen)
            for (i = 0; i < n; ++i)
                cs1 += *pEndStr++;

        while (slen - offset >= n)
        {
#ifndef USE_BCS
            if (cs == cs1 && !memcmp(pStr + offset, sFind, n2))
#else
            if (!memcmp(pStr + offset, sFind, n2))
#endif
            {
                if (!g_fWholeWord || IsWord(pStr, offset, n))
                {
                    TPRINT(("    Found t=%d\n", time(NULL) - t));
                    _GotoXY(offset, l);

                    m_nFoundX = offset;
                    m_nFoundY = l;
                    if (offset - m_nXOffset + n < m_nSizeX)
                        m_nFoundSize = n;
                    else
                        m_nFoundSize = m_nSizeX - (offset - m_nXOffset);
                    Invalidate(m_nFoundY, 0, m_nFoundX, m_nFoundSize);

                    if (!fSilence)
                        SetHelpLine();
                    return 1;
                }
            }

#ifndef USE_BCS
            cs1 += *pEndStr++ - pStr[offset];
            ++offset;
#else
            offset += pBadCharShift[pStr[offset + n]];
#endif
        }
        offset = 0;
        ++l;

        //if(!fSilence)
        if (++progress == 1000)
        {
            progress = 0;
            if ((fBreak = UpdateProgress((l - Begin) * 99 / (end - Begin))) != 0)
                break;
        }
    }

    if (!fSilence)
    {
        HideFound();
        if (!fBreak)
            SetErrorLine(STR_S(SS_StringNotFound));
        else
            SetHelpLine(STR_S(SS_UserAbort), 1);
    }

    if (!fBreak)
        return 0;
    else
        return -1;
}
#endif