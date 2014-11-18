// LogView.cpp : Defines the entry point for the console application.
//

// See http://msdn.microsoft.com/en-us/library/windows/desktop/ms682499(v=vs.85).aspx

// See http://kassiopeia.juls.savba.sk/~garabik/software/grc.html

#include "stdafx.h"
#include "WinUtils.h"
#include <assert.h>

void Add(std::map<DWORD, WORD>& lc, DWORD dwFirst, DWORD dwSecond, WORD wAttributes, WORD wDefaultAttributes)
{
    assert(dwSecond > dwFirst);

    WORD wEndAttributes = wDefaultAttributes;
    std::map<DWORD, WORD>::iterator it = lc.lower_bound(dwFirst);
    if (it != lc.begin())
    {
        wEndAttributes = std::prev(it)->second;
    }

    while (it != lc.begin() && it != lc.end() && it->first <= dwSecond)
    {
        wEndAttributes = it->second;
        it = lc.erase(it);
    }

    lc[dwFirst] = wAttributes;
    lc[dwSecond] = wEndAttributes;
}

#if 0
void TestAdd()
{
    {
        std::map<DWORD, WORD> lc;
        Add(lc, 5, 10, 5, 1);
        assert(lc[5] == 5);
        assert(lc[10] == 1);

        Add(lc, 7, 8, 6, 1);
        assert(lc[5] == 5);
        assert(lc[7] == 6);
        assert(lc[8] == 5);
        assert(lc[10] == 1);

        Add(lc, 8, 9, 7, 1);
        assert(lc[5] == 5);
        assert(lc[7] == 6);
        assert(lc[8] == 7);
        assert(lc[9] == 5);
        assert(lc[10] == 1);

        Add(lc, 7, 9, 7, 1);
        assert(lc[5] == 5);
        assert(lc[7] == 7);
        assert(lc[9] == 5);
        assert(lc[10] == 1);
    }
}
#endif

struct RegExAttribute
{
    enum CountE
    {
        Once,
        More,
    };

    RegExAttribute(const std::wstring& expr, WORD wAttributes, CountE eCount)
        : regex(expr)
        , wAttributes(wAttributes)
        , eCount(eCount)
        , cont(true)
    {
    }

    std::wregex regex;
    WORD wAttributes;
    CountE eCount;
    mutable bool cont;
};

std::map<DWORD, WORD> ParseLine(const CHAR* chBuf, DWORD dwLength, const std::vector<RegExAttribute>& vRegExAttribute, WORD wDefaultAttributes)
{
    std::map<DWORD, WORD> lc;

    std::regex_constants::match_flag_type flags = std::regex_constants::match_prev_avail;

    for (std::vector<RegExAttribute>::const_iterator itRegExAttribute = vRegExAttribute.begin(); itRegExAttribute != vRegExAttribute.end(); ++itRegExAttribute)
    {
        const CHAR* pBegin = chBuf;

        if (pBegin[-1] == _T('\n'))
            itRegExAttribute->cont = true;
        std::cmatch match;
        while (itRegExAttribute->cont && std::regex_search(pBegin, chBuf + dwLength, match, itRegExAttribute->regex, flags))
        {
            for (std::cmatch::const_iterator it = match.begin(); it != match.end(); ++it)
            {
                const std::csub_match& sub_match = *it;
                const CHAR* first = sub_match.first;
                const CHAR* second = sub_match.second;
                Add(lc, (DWORD) (first - chBuf), (DWORD) (second - chBuf), itRegExAttribute->wAttributes, wDefaultAttributes);
                pBegin = second;
            }

            if (itRegExAttribute->eCount == RegExAttribute::Once)
                itRegExAttribute->cont = false;
        }
    }

    return lc;
}

BOOL WriteLine(HANDLE hOut, const CHAR* chBuf, DWORD nNumberOfBytesToWrite, const std::vector<RegExAttribute>& vRegExAttribute, WORD wDefaultAttributes)
{
    DWORD dwWritten;
    DWORD dwOffset = 0;
    BOOL bSuccess = TRUE;

    std::map<DWORD, WORD> lc = ParseLine(chBuf, nNumberOfBytesToWrite, vRegExAttribute, wDefaultAttributes);

    SetConsoleTextAttribute(hOut, wDefaultAttributes);

    std::map<DWORD, WORD>::const_iterator it = lc.begin();
    while (it != lc.end() && dwOffset >= it->first)
    {
        SetConsoleTextAttribute(hOut, it->second);
        ++it;
    }
    while (it != lc.end())
    {
        DWORD dwWrite = std::min(it->first, nNumberOfBytesToWrite) - dwOffset;
        if (dwWrite <= 0)
            break;

        bSuccess = WriteFile(hOut, chBuf + dwOffset, dwWrite, &dwWritten, NULL);
        if (!bSuccess)
            break;
        dwOffset += dwWritten;

        while (it != lc.end() && dwOffset >= it->first)
        {
            SetConsoleTextAttribute(hOut, it->second);
            ++it;
        }
    }

    if (bSuccess)
        bSuccess = WriteFile(hOut, chBuf + dwOffset, nNumberOfBytesToWrite - dwOffset, &dwWritten, NULL);

    //WriteFile(hOut, "@", 1, &dwWritten, NULL);

    return bSuccess;
}

void CopyFile(HANDLE hIn, HANDLE hOut, const std::vector<RegExAttribute>& vRegExAttribute, WORD wAttributes)
{
    DWORD dwRead;
    CHAR chBuf[4096] = "\n"; 
    CHAR* pLineBegin, * pLineEnd;
    BOOL bSuccess = FALSE;

    for (;;)
    { 
        bSuccess = ReadFile(hIn, chBuf + 1, ARRAYSIZE(chBuf) - 1, &dwRead, NULL);
        if(!bSuccess || dwRead == 0)
            break;

        //SetConsoleTextAttribute(hOut, wAttributes);

        pLineBegin = chBuf + 1;
        while (pLineEnd = (CHAR*) memchr(pLineBegin, '\n', dwRead - (pLineBegin - (chBuf + 1))))
        {
            bSuccess = WriteLine(hOut, pLineBegin, (DWORD) (pLineEnd - pLineBegin + 1), vRegExAttribute, wAttributes);
            if (!bSuccess)
                break;
            pLineBegin = pLineEnd + 1;
        }

        if (!bSuccess)
            break;

        bSuccess = WriteLine(hOut, pLineBegin, (DWORD) (dwRead - (pLineBegin - (chBuf + 1))), vRegExAttribute, wAttributes);
        if (!bSuccess)
            break;

        //bSuccess = WriteFile(hOut, chBuf + 1, dwRead, &dwWritten, NULL);
        //if (!bSuccess)
            //break;

        chBuf[0] = chBuf[dwRead];
    }
}

struct CopyFileThreadData
{
    HANDLE hIn;
    HANDLE hOut;
    std::vector<RegExAttribute> vRegExAttribute;
    WORD wAttributes;
};

DWORD WINAPI CopyFileThread(LPVOID lpThreadParameter)
{
    const CopyFileThreadData* pData = (const CopyFileThreadData*) lpThreadParameter;
    CopyFile(pData->hIn, pData->hOut, pData->vRegExAttribute, pData->wAttributes);
    return 0;
}

void LoadConf(std::vector<RegExAttribute>& vRegExAttributeOut, std::vector<RegExAttribute>& vRegExAttributeErr, const TCHAR* conf)
{
    //vRegExAttributeOut.push_back(RegExAttribute("[0-9]{2}:[0-9]{2} [AP]M", FOREGROUND_RED | FOREGROUND_INTENSITY));
    //vRegExAttributeOut.push_back(RegExAttribute("[0-9]{1}:[0-9]{1}", FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY));
    //vRegExAttributeOut.push_back(RegExAttribute("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}", FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY));   // IP Address
    //_tprintf(_T("Load conf: %s\n"), conf);

    TCHAR pathname[MAX_PATH];
    GetModuleFileName(NULL, pathname, ARRAYSIZE(pathname));
    {
        TCHAR* e = _tcsrchr(pathname, _T('\\'));
        _tcscpy_s(e + 1, ARRAYSIZE(pathname) - (e + 1 - pathname), conf);
    }

    std::wstring expr;
    WORD wAttributes = 0;
    RegExAttribute::CountE eCount = RegExAttribute::More;

    const std::wstring regex(_T("regexp="));
    const std::wstring colours(_T("colours="));
    const std::wstring count(_T("count="));

    std::wifstream f;
    f.open(pathname);

    std::wstring line;
    while (std::getline(f, line))
    {
        if (!line.empty() && line[0] == '#')
        {
            // ignore
        }
        else if (line.empty() || !std::isalnum(line[0], std::locale::classic()))
        {
            if (!expr.empty())
            {
                //_tprintf(_T("Add: %s\n"), expr.c_str());
                vRegExAttributeOut.push_back(RegExAttribute(expr, wAttributes, eCount));
                expr.clear();
                wAttributes = 0;
                eCount = RegExAttribute::More;
            }
        }
        else if (line.compare(0, regex.size(), regex) == 0)
        {
            expr = line.substr(regex.size());
        }
        else if (line.compare(0, colours.size(), colours) == 0)
        {
            std::wistringstream ss(line.substr(colours.size()));
            std::vector<std::wstring> c;
            c.insert(c.end(), std::istream_iterator<std::wstring, wchar_t, std::char_traits<wchar_t> >(ss), std::istream_iterator<std::wstring, wchar_t, std::char_traits<wchar_t> >());
            for (std::vector<std::wstring>::const_iterator it = c.begin(); it != c.end(); ++it)
            {
                if (*it == _T("red"))
                    wAttributes |= FOREGROUND_RED;
                else if (*it == _T("green"))
                    wAttributes |= FOREGROUND_GREEN;
                else if (*it == _T("blue"))
                    wAttributes |= FOREGROUND_BLUE;
                else if (*it == _T("yellow"))
                    wAttributes |= FOREGROUND_RED | FOREGROUND_GREEN;
                else if (*it == _T("cyan"))
                    wAttributes |= FOREGROUND_GREEN | FOREGROUND_BLUE;
                else if (*it == _T("magenta"))
                    wAttributes |= FOREGROUND_RED | FOREGROUND_BLUE;
                else if (*it == _T("white"))
                    wAttributes |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                else if (*it == _T("bold"))
                    wAttributes |= FOREGROUND_INTENSITY;
                else if (*it == _T("on_red"))
                    wAttributes |= BACKGROUND_RED;
                else if (*it == _T("on_green"))
                    wAttributes |= BACKGROUND_GREEN;
                else if (*it == _T("on_blue"))
                    wAttributes |= BACKGROUND_BLUE;
                else if (*it == _T("on_yellow"))
                    wAttributes |= BACKGROUND_RED | BACKGROUND_GREEN;
                else if (*it == _T("on_cyan"))
                    wAttributes |= BACKGROUND_GREEN | BACKGROUND_BLUE;
                else if (*it == _T("on_magenta"))
                    wAttributes |= BACKGROUND_RED | BACKGROUND_BLUE;
                else if (*it == _T("on_white"))
                    wAttributes |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
                else if (*it == _T("on_bold"))
                    wAttributes |= BACKGROUND_INTENSITY;
            }
        }
        else if (line.compare(0, count.size(), count) == 0)
        {
            std::wstring eCountStr = line.substr(count.size());
            if (eCountStr == _T("once"))
                eCount = RegExAttribute::Once;
            else if (eCountStr == _T("more"))
                eCount = RegExAttribute::More;
        }
    }

    if (!expr.empty())
    {
        //_tprintf(_T("Add: %s\n"), expr.c_str());
        vRegExAttributeOut.push_back(RegExAttribute(expr, wAttributes, eCount));
        expr.clear();
        wAttributes = 0;
        eCount = RegExAttribute::More;
    }

    f.close();
}

void FindConf(std::wstring& sConf, const TCHAR* sCmd)
{
    const TCHAR* rconf = _T("rainbow.conf");

    TCHAR pathname[MAX_PATH];
    GetModuleFileName(NULL, pathname, ARRAYSIZE(pathname));
    {
        TCHAR* e = _tcsrchr(pathname, _T('\\'));
        _tcscpy_s(e + 1, ARRAYSIZE(pathname) - (e + 1 - pathname), rconf);
    }

    std::wstring expr;

    const std::wstring regex(_T("regexp="));
    const std::wstring conf(_T("conf="));

    std::wifstream f;
    f.open(pathname);

    std::wstring line;
    while (std::getline(f, line))
    {
        if (!line.empty() && line[0] == '#')
        {
            // ignore
        }
        else if (line.compare(0, regex.size(), regex) == 0)
        {
            expr = line.substr(regex.size());
        }
        else if (line.compare(0, conf.size(), conf) == 0)
        {
            std::wregex tregex(expr);
            if (std::regex_match(sCmd, tregex))
                sConf = line.substr(conf.size());
        }
    }

    f.close();
}

void ColorizeProcess(HANDLE hStdOut, WORD wDefaultAttributes, const TCHAR* conf, std::wstring& sCmdLine)
{
    CopyFileThreadData OutData;
    OutData.hOut = hStdOut;
    OutData.wAttributes = wDefaultAttributes;

    CopyFileThreadData ErrData;
    ErrData.hOut = hStdOut;
    ErrData.wAttributes = FOREGROUND_RED | FOREGROUND_INTENSITY;

    LoadConf(OutData.vRegExAttribute, ErrData.vRegExAttribute, conf);

    Pipe hChildStd_OUT;
    Pipe hChildStd_ERR;

    OutData.hIn = hChildStd_OUT.read().get();
    ErrData.hIn = hChildStd_ERR.read().get();

    Handle hProcess;
    Handle hThread;
    {
        STARTUPINFO            siStartupInfo = { sizeof(siStartupInfo) };
        PROCESS_INFORMATION    piProcessInfo = { 0 };
 
        siStartupInfo.dwFlags = STARTF_USESTDHANDLES;
        siStartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        siStartupInfo.hStdOutput = hChildStd_OUT.write().get();
        siStartupInfo.hStdError = hChildStd_ERR.write().get();

        WIN(CreateProcess(NULL, const_cast<LPTSTR>(sCmdLine.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &siStartupInfo, &piProcessInfo), _T("CreateProcess"));

        hProcess.attach(piProcessInfo.hProcess);
        hThread.attach(piProcessInfo.hThread);
    }

    hChildStd_OUT.write().close();
    hChildStd_ERR.write().close();

    Handle hThreadOut(CreateThread(NULL, 0, &CopyFileThread, &OutData, 0, NULL));
    Handle hThreadErr(CreateThread(NULL, 0, &CopyFileThread, &ErrData, 0, NULL));

    HANDLE hWait[] = { hThreadOut.get(), hThreadErr.get(), hProcess.get() };
    WIN(WaitForMultipleObjects(ARRAYSIZE(hWait), hWait, TRUE, INFINITE) == WAIT_OBJECT_0, _T("WaitForMultipleObjects"));
}

int _tmain(int argc, _TCHAR* argv[])
{
    //TestAdd();

    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };
    GetConsoleScreenBufferInfo(hStdOut, &csbi);

    // TODO Handle no command line parameters - ie show help
    // TODO Handle colorizing hStdIn

    try
    {
        std::wstring sConf(_T("default.rgc"));
        bool bDefaultConf = true;
        std::wstring sCmdLine;

        int arg = 1;

        if (arg < argc && _tcscmp(argv[arg], _T("-conf")) == 0)
        {
            ++arg;
            sConf = argv[arg];
            bDefaultConf = false;
            ++arg;
        }

        if (arg < argc
            && (_tcscmp(argv[arg], _T("dir")) == 0
            || _tcscmp(argv[arg], _T("echo")) == 0
            || _tcscmp(argv[arg], _T("copy")) == 0
            || _tcscmp(argv[arg], _T("del")) == 0
            || _tcscmp(argv[arg], _T("move")) == 0
            || _tcscmp(argv[arg], _T("type")) == 0))
            sCmdLine += _T("cmd /c ");

        if (bDefaultConf && arg < argc)
            FindConf(sConf, argv[arg]);

        for (; arg < argc; ++arg)
        {
            if (!sCmdLine.empty())
                sCmdLine += _T(' ');

            bool hasSpaces = _tcschr(argv[arg], _T(' ')) != nullptr;
            if (hasSpaces)
                sCmdLine += _T('\"');
            sCmdLine += argv[arg];
            if (hasSpaces)
                sCmdLine += _T('\"');
        }

        if (sCmdLine.empty())
            DisplayAboutMessage(NULL);
        else
            ColorizeProcess(hStdOut, csbi.wAttributes, sConf.c_str(), sCmdLine);
    }
    catch (const std::exception& ex)
    {
        printf("Exception: %s.\n", ex.what());
    }
    catch (const WinError& ex)
    {
        _tprintf(_T("WinError: %s.\n"), ex.what());
    }
    catch (...)
    {
        _tprintf(_T("Unknown exception.\n"));
    }

    SetConsoleTextAttribute(hStdOut, csbi.wAttributes);

    return 0;
}
