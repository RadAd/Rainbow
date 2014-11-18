#pragma once

class WinError
{
public:
    WinError(LPTSTR lpszFunction, DWORD dwError = GetLastError())
        : m_lpDisplayBuf(nullptr)
    {
        LPTSTR lpMsgBuf;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );

        m_lpDisplayBuf = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
        StringCchPrintf(m_lpDisplayBuf, 
            LocalSize(m_lpDisplayBuf) / sizeof(TCHAR),
            TEXT("%s failed with error %d: %s"), 
            lpszFunction, dwError, lpMsgBuf); 

        LocalFree(lpMsgBuf);
    }

    ~WinError()
    {
        LocalFree(m_lpDisplayBuf);
    }

    LPCTSTR what() const { return m_lpDisplayBuf; }

private:
    LPTSTR m_lpDisplayBuf;
};

void WIN(BOOL bTest, LPTSTR lpszFunction)
{
    if (!bTest)
        throw WinError(lpszFunction);
}

class Handle
{
public:
    Handle()
        : m_hHandle(NULL)
    {
    }

    explicit Handle(HANDLE hHandle)
        : m_hHandle(hHandle)
    {
    }

    ~Handle()
    {
        close();
    }

    void attach(HANDLE hHandle)
    {
        close();
        m_hHandle = hHandle;
    }

    void close()
    {
        if (m_hHandle != NULL)
            WIN(CloseHandle(m_hHandle), _T("CloseHandle"));
        m_hHandle = NULL;
    }

    void clear()
    {
        m_hHandle = NULL;
    }

    HANDLE* operator&()
    {
        return &m_hHandle;
    }

    HANDLE get()
    {
        return m_hHandle;
    }

private:
    Handle(const Handle&);
    Handle& operator=(const Handle&);
    HANDLE m_hHandle;
};

class Pipe
{
public:
    Pipe()
    {
        SECURITY_ATTRIBUTES saAttr = { sizeof(saAttr) };
        saAttr.bInheritHandle = TRUE;
        WIN(CreatePipe(&hRead, &hWrite, &saAttr, 0), _T("CreatePipe"));
        WIN(SetHandleInformation(hRead.get(), HANDLE_FLAG_INHERIT, 0), _T("SetHandleInformation"));
    }

    Handle& read() { return hRead; }
    Handle& write() { return hWrite; }

private:
    Handle hRead;
    Handle hWrite;
};
