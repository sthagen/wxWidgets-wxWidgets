/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/utilsexc.cpp
// Purpose:     wxExecute implementation for MSW
// Author:      Julian Smart
// Created:     04/01/98
// Copyright:   (c) 1998-2002 wxWidgets dev team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"


#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/app.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #if wxUSE_STREAMS
        #include "wx/stream.h"
    #endif
    #include "wx/module.h"
#endif

#include "wx/process.h"
#include "wx/thread.h"
#include "wx/apptrait.h"
#include "wx/evtloop.h"
#include "wx/vector.h"


#include "wx/msw/private.h"

#include <ctype.h>

#if !defined(__GNUWIN32__)
    #include <direct.h>
    #include <dos.h>
#endif

#if defined(__GNUWIN32__)
    #include <sys/stat.h>
#endif

#ifndef __UNIX__
    #include <io.h>
#endif

#ifndef __GNUWIN32__
    #include <shellapi.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#if wxUSE_IPC
    #include "wx/dde.h"         // for WX_DDE hack in wxExecute
#endif // wxUSE_IPC

#include "wx/msw/private/hiddenwin.h"
#include "wx/msw/private/event.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// this message is sent when the process we're waiting for terminates
#define wxWM_PROC_TERMINATED (WM_USER + 10000)

// ----------------------------------------------------------------------------
// this module globals
// ----------------------------------------------------------------------------

// we need to create a hidden window to receive the process termination
// notifications and for this we need a (Win) class name for it which we will
// register the first time it's needed
static const wxChar *wxMSWEXEC_WNDCLASSNAME = wxT("_wxExecute_Internal_Class");
static const wxChar *gs_classForHiddenWindow = nullptr;

// event used to wake up threads waiting in wxExecuteThread
static wxWinAPI::Event gs_heventShutdown;

// handles of all threads monitoring the execution of asynchronously running
// processes
static wxVector<HANDLE> gs_asyncThreads;

// ----------------------------------------------------------------------------
// private types
// ----------------------------------------------------------------------------

// structure describing the process we're being waiting for
struct wxExecuteData
{
public:
    wxExecuteData()
    {
        // The rest is initialized in the code creating the objects of this
        // class, but the thread handle can't be set until later, so initialize
        // it here to ensure we never use an uninitialized value in our dtor.
        hThread = 0;
    }

    ~wxExecuteData()
    {
        if ( !::CloseHandle(hProcess) )
        {
            wxLogLastError(wxT("CloseHandle(hProcess)"));
        }
    }

    HWND       hWnd;          // window to send wxWM_PROC_TERMINATED to
    HANDLE     hProcess;      // handle of the process
    HANDLE     hThread;       // handle of the thread monitoring its termination
    DWORD      dwProcessId;   // pid of the process
    wxProcess *handler;
    DWORD      dwExitCode;    // the exit code of the process
    bool       state;         // set to false when the process finishes
};

class wxExecuteModule : public wxModule
{
public:
    virtual bool OnInit() override { return true; }
    virtual void OnExit() override
    {
        if ( gs_heventShutdown.IsOk() )
        {
            // stop any threads waiting for the termination of asynchronously
            // running processes
            if ( !gs_heventShutdown.Set() )
            {
                wxLogDebug(wxT("Failed to set shutdown event in wxExecuteModule"));
            }

            gs_heventShutdown.Close();

            // now wait until they terminate
            if ( !gs_asyncThreads.empty() )
            {
                const size_t numThreads = gs_asyncThreads.size();

                if ( ::WaitForMultipleObjects
                       (
                        numThreads,
                        &gs_asyncThreads[0],
                        TRUE,   // wait for all of them to become signalled
                        3000    // long but finite value
                       ) == WAIT_TIMEOUT )
                {
                    wxLogDebug(wxT("Failed to stop all wxExecute monitor threads"));
                }

                for ( size_t n = 0; n < numThreads; n++ )
                {
                    ::CloseHandle(gs_asyncThreads[n]);
                }

                gs_asyncThreads.clear();
            }
        }

        if ( gs_classForHiddenWindow )
        {
            if ( !::UnregisterClass(wxMSWEXEC_WNDCLASSNAME, wxGetInstance()) )
            {
                wxLogLastError(wxT("UnregisterClass(wxExecClass)"));
            }

            gs_classForHiddenWindow = nullptr;
        }
    }

private:
    wxDECLARE_DYNAMIC_CLASS(wxExecuteModule);
};

wxIMPLEMENT_DYNAMIC_CLASS(wxExecuteModule, wxModule);

#if wxUSE_STREAMS

#include "wx/private/pipestream.h"
#include "wx/private/streamtempinput.h"

// ----------------------------------------------------------------------------
// wxPipe represents a Win32 anonymous pipe
// ----------------------------------------------------------------------------

class wxPipe
{
public:
    // the symbolic names for the pipe ends
    enum Direction
    {
        Read,
        Write
    };

    // default ctor doesn't do anything
    wxPipe() { m_handles[Read] = m_handles[Write] = INVALID_HANDLE_VALUE; }

    // create the pipe, return true if ok, false on error
    bool Create()
    {
        // default secutiry attributes
        SECURITY_ATTRIBUTES security;

        security.nLength              = sizeof(security);
        security.lpSecurityDescriptor = nullptr;
        security.bInheritHandle       = TRUE; // to pass it to the child

        if ( !::CreatePipe(&m_handles[0], &m_handles[1], &security, 0) )
        {
            wxLogSysError(_("Failed to create an anonymous pipe"));

            return false;
        }

        return true;
    }

    // return true if we were created successfully
    bool IsOk() const { return m_handles[Read] != INVALID_HANDLE_VALUE; }

    // return the descriptor for one of the pipe ends
    HANDLE operator[](Direction which) const { return m_handles[which]; }

    // detach a descriptor, meaning that the pipe dtor won't close it, and
    // return it
    HANDLE Detach(Direction which)
    {
        HANDLE handle = m_handles[which];
        m_handles[which] = INVALID_HANDLE_VALUE;

        return handle;
    }

    // close the pipe descriptors
    void Close()
    {
        for ( size_t n = 0; n < WXSIZEOF(m_handles); n++ )
        {
            if ( m_handles[n] != INVALID_HANDLE_VALUE )
            {
                ::CloseHandle(m_handles[n]);
                m_handles[n] = INVALID_HANDLE_VALUE;
            }
        }
    }

    // dtor closes the pipe descriptors
    ~wxPipe() { Close(); }

private:
    HANDLE m_handles[2];
};

#endif // wxUSE_STREAMS

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// process termination detecting support
// ----------------------------------------------------------------------------

// thread function for the thread monitoring the process termination
static DWORD __stdcall wxExecuteThread(void *arg)
{
    wxExecuteData * const data = (wxExecuteData *)arg;

    // create the shutdown event if we're the first thread starting to wait
    if ( !gs_heventShutdown.IsOk() )
    {
        // create a manual initially non-signalled event object
        if ( !gs_heventShutdown.Create(wxWinAPI::Event::ManualReset, wxWinAPI::Event::Nonsignaled) )
        {
            wxLogDebug(wxT("CreateEvent() in wxExecuteThread failed"));
        }
    }

    HANDLE handles[2] = { data->hProcess, gs_heventShutdown };
    switch ( ::WaitForMultipleObjects(2, handles, FALSE, INFINITE) )
    {
        case WAIT_OBJECT_0:
            // process terminated, get its exit code
            if ( !::GetExitCodeProcess(data->hProcess, &data->dwExitCode) )
            {
                wxLogLastError(wxT("GetExitCodeProcess"));
            }

            wxASSERT_MSG( data->dwExitCode != STILL_ACTIVE,
                          wxT("process should have terminated") );

            // send a message indicating process termination to the window
            ::SendMessage(data->hWnd, wxWM_PROC_TERMINATED, 0, (LPARAM)data);
            break;

        case WAIT_OBJECT_0 + 1:
            // we're shutting down but the process is still running -- leave it
            // run but clean up the associated data
            if ( !data->state )
            {
                delete data;
            }
            //else: exiting while synchronously executing process is still
            //      running? this shouldn't happen...
            break;

        default:
            wxLogDebug(wxT("Waiting for the process termination failed!"));
    }

    return 0;
}

// window procedure of a hidden window which is created just to receive
// the notification message when a process exits
LRESULT APIENTRY
wxExecuteWindowCbk(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if ( message == wxWM_PROC_TERMINATED )
    {
        DestroyWindow(hWnd);    // we don't need it any more

        wxExecuteData * const data = (wxExecuteData *)lParam;
        if ( data->handler )
        {
            data->handler->OnTerminate((int)data->dwProcessId,
                                       (int)data->dwExitCode);
        }

        if ( data->state )
        {
            // we're executing synchronously, tell the waiting thread
            // that the process finished
            data->state = false;
        }
        else
        {
            // asynchronous execution - we should do the clean up
            for ( wxVector<HANDLE>::iterator it = gs_asyncThreads.begin();
                  it != gs_asyncThreads.end();
                  ++it )
            {
                if ( *it == data->hThread )
                {
                    gs_asyncThreads.erase(it);
                    if ( !::CloseHandle(data->hThread) )
                    {
                        wxLogLastError(wxT("CloseHandle(hThread)"));
                    }
                    break;
                }
            }

            delete data;
        }

        return 0;
    }
    else
    {
        return ::DefWindowProc(hWnd, message, wParam, lParam);
    }
}

// ============================================================================
// implementation of IO redirection support classes
// ============================================================================

#if wxUSE_STREAMS

// ----------------------------------------------------------------------------
// wxPipeInputStreams
// ----------------------------------------------------------------------------

wxPipeInputStream::wxPipeInputStream(HANDLE hInput)
{
    m_hInput = hInput;
}

wxPipeInputStream::~wxPipeInputStream()
{
    if ( m_hInput != INVALID_HANDLE_VALUE )
        ::CloseHandle(m_hInput);
}

bool wxPipeInputStream::CanRead() const
{
    // we can read if there's something in the put back buffer
    // even pipe is closed
    if ( m_wbacksize > m_wbackcur )
        return true;

    wxPipeInputStream * const self = wxConstCast(this, wxPipeInputStream);

    if ( !IsOpened() )
    {
        // set back to mark Eof as it may have been unset by Ungetch()
        self->m_lasterror = wxSTREAM_EOF;
        return false;
    }

    DWORD nAvailable;

    // function name is misleading, it works with anon pipes as well
    DWORD rc = ::PeekNamedPipe
                    (
                      m_hInput,     // handle
                      nullptr, 0,   // ptr to buffer and its size
                      nullptr,      // [out] bytes read
                      &nAvailable,  // [out] bytes available
                      nullptr       // [out] bytes left
                    );

    if ( !rc )
    {
        if ( ::GetLastError() != ERROR_BROKEN_PIPE )
        {
            // unexpected error
            wxLogLastError(wxT("PeekNamedPipe"));
        }

        // don't try to continue reading from a pipe if an error occurred or if
        // it had been closed
        ::CloseHandle(m_hInput);

        self->m_hInput = INVALID_HANDLE_VALUE;
        self->m_lasterror = wxSTREAM_EOF;

        nAvailable = 0;
    }

    return nAvailable != 0;
}

size_t wxPipeInputStream::OnSysRead(void *buffer, size_t len)
{
    if ( !IsOpened() )
    {
        m_lasterror = wxSTREAM_EOF;

        return 0;
    }

    DWORD bytesRead;
    if ( !::ReadFile(m_hInput, buffer, len, &bytesRead, nullptr) )
    {
        m_lasterror = ::GetLastError() == ERROR_BROKEN_PIPE
                        ? wxSTREAM_EOF
                        : wxSTREAM_READ_ERROR;
    }

    // bytesRead is set to 0, as desired, if an error occurred
    return bytesRead;
}

// ----------------------------------------------------------------------------
// wxPipeOutputStream
// ----------------------------------------------------------------------------

wxPipeOutputStream::wxPipeOutputStream(HANDLE hOutput)
{
    m_hOutput = hOutput;

    // unblock the pipe to prevent deadlocks when we're writing to the pipe
    // from which the child process can't read because it is writing in its own
    // end of it
    DWORD mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
    if ( !::SetNamedPipeHandleState
            (
                m_hOutput,
                &mode,
                nullptr,       // collection count (we don't set it)
                nullptr        // timeout (we don't set it either)
            ) )
    {
        wxLogLastError(wxT("SetNamedPipeHandleState(PIPE_NOWAIT)"));
    }
}

bool wxPipeOutputStream::Close()
{
   return ::CloseHandle(m_hOutput) != 0;
}


size_t wxPipeOutputStream::OnSysWrite(const void *buffer, size_t len)
{
    m_lasterror = wxSTREAM_NO_ERROR;

    DWORD totalWritten = 0;
    while ( len > 0 )
    {
        DWORD chunkWritten;
        if ( !::WriteFile(m_hOutput, buffer, len, &chunkWritten, nullptr) )
        {
            m_lasterror = ::GetLastError() == ERROR_BROKEN_PIPE
                                ? wxSTREAM_EOF
                                : wxSTREAM_WRITE_ERROR;
            break;
        }

        if ( !chunkWritten )
            break;

        buffer = static_cast<const char*>(buffer) + chunkWritten;
        totalWritten += chunkWritten;
        len -= chunkWritten;
    }

    return totalWritten;
}

#endif // wxUSE_STREAMS

// ============================================================================
// wxExecute functions family
// ============================================================================

#if wxUSE_IPC

// connect to the given server via DDE and ask it to execute the command
bool
wxExecuteDDE(const wxString& ddeServer,
             const wxString& ddeTopic,
             const wxString& ddeCommand)
{
    bool ok wxDUMMY_INITIALIZE(false);

    wxDDEClient client;
    wxConnectionBase *
        conn = client.MakeConnection(wxEmptyString, ddeServer, ddeTopic);
    if ( !conn )
    {
        ok = false;
    }
    else // connected to DDE server
    {
        // the added complication here is that although most programs use
        // XTYP_EXECUTE for their DDE API, some important ones -- like Word
        // and other MS stuff - use XTYP_REQUEST!
        //
        // moreover, another important program (IE) understands both but
        // returns an error from Execute() so we must try Request() first
        // to avoid doing it twice
        {
            // we're prepared for this one to fail, so don't show errors
            wxLogNull noErrors;

            ok = conn->Request(ddeCommand) != nullptr;
        }

        if ( !ok )
        {
            // now try execute -- but show the errors
            ok = conn->Execute(ddeCommand);
        }
    }

    return ok;
}

#endif // wxUSE_IPC

long wxExecute(const wxString& cmd, int flags, wxProcess *handler,
               const wxExecuteEnv *env)
{
    wxCHECK_MSG( !cmd.empty(), 0, wxT("empty command in wxExecute") );

#if wxUSE_THREADS
    // for many reasons, the code below breaks down if it's called from another
    // thread -- this could be fixed, but as Unix versions don't support this
    // either I don't want to waste time on this now
    wxASSERT_MSG( wxThread::IsMain(),
                    wxT("wxExecute() can be called only from the main thread") );
#endif // wxUSE_THREADS

    wxString command;

#if wxUSE_IPC
    // DDE hack: this is really not pretty, but we need to allow this for
    // transparent handling of DDE servers in wxMimeTypesManager. Usually it
    // returns the command which should be run to view/open/... a file of the
    // given type. Sometimes, however, this command just launches the server
    // and an additional DDE request must be made to really open the file. To
    // keep all this well hidden from the application, we allow a special form
    // of command: WX_DDE#<command>#DDE_SERVER#DDE_TOPIC#DDE_COMMAND in which
    // case we execute just <command> and process the rest below
    wxString ddeServer, ddeTopic, ddeCommand;
    static const size_t lenDdePrefix = 7;   // strlen("WX_DDE:")
    if ( cmd.Left(lenDdePrefix) == wxT("WX_DDE#") )
    {
        // speed up the concatenations below
        ddeServer.reserve(256);
        ddeTopic.reserve(256);
        ddeCommand.reserve(256);

        const wxChar *p = cmd.c_str() + 7;
        while ( *p && *p != wxT('#') )
        {
            command += *p++;
        }

        if ( *p )
        {
            // skip '#'
            p++;
        }
        else
        {
            wxFAIL_MSG(wxT("invalid WX_DDE command in wxExecute"));
        }

        while ( *p && *p != wxT('#') )
        {
            ddeServer += *p++;
        }

        if ( *p )
        {
            // skip '#'
            p++;
        }
        else
        {
            wxFAIL_MSG(wxT("invalid WX_DDE command in wxExecute"));
        }

        while ( *p && *p != wxT('#') )
        {
            ddeTopic += *p++;
        }

        if ( *p )
        {
            // skip '#'
            p++;
        }
        else
        {
            wxFAIL_MSG(wxT("invalid WX_DDE command in wxExecute"));
        }

        while ( *p )
        {
            ddeCommand += *p++;
        }

        // if we want to just launch the program and not wait for its
        // termination, try to execute DDE command right now, it can succeed if
        // the process is already running - but as it fails if it's not
        // running, suppress any errors it might generate
        if ( !(flags & wxEXEC_SYNC) )
        {
            wxLogNull noErrors;
            if ( wxExecuteDDE(ddeServer, ddeTopic, ddeCommand) )
            {
                // a dummy PID - this is a hack, of course, but it's well worth
                // it as we don't open a new server each time we're called
                // which would be quite bad
                return -1;
            }
        }
    }
    else
#endif // wxUSE_IPC
    {
        // no DDE
        command = cmd;
    }

    // the IO redirection is only supported with wxUSE_STREAMS
    BOOL redirect = FALSE;

#if wxUSE_STREAMS
    wxPipe pipeIn, pipeOut, pipeErr;

    // open the pipes to which child process IO will be redirected if needed
    if ( handler && handler->IsRedirected() )
    {
        // create pipes for redirecting stdin, stdout and stderr
        if ( !pipeIn.Create() || !pipeOut.Create() || !pipeErr.Create() )
        {
            wxLogSysError(_("Failed to redirect the child process IO"));

            // indicate failure: we need to return different error code
            // depending on the sync flag
            return flags & wxEXEC_SYNC ? -1 : 0;
        }

        redirect = TRUE;
    }
#endif // wxUSE_STREAMS

    // create the process
    STARTUPINFO si;
    wxZeroMemory(si);
    si.cb = sizeof(si);

#if wxUSE_STREAMS
    if ( redirect )
    {
        si.dwFlags = STARTF_USESTDHANDLES;

        si.hStdInput = pipeIn[wxPipe::Read];
        si.hStdOutput = pipeOut[wxPipe::Write];
        si.hStdError = pipeErr[wxPipe::Write];

        // We must set the handles to those sides of std* pipes that we won't
        // in the child to be non-inheritable. We must do this before launching
        // the child process as otherwise these handles will be inherited by
        // the child which will never close them and so the pipe will not
        // return ERROR_BROKEN_PIPE if the parent or child exits unexpectedly
        // causing the remaining process to potentially become deadlocked in
        // ReadFile() or WriteFile().
        if ( !::SetHandleInformation(pipeIn[wxPipe::Write],
                                     HANDLE_FLAG_INHERIT, 0) )
            wxLogLastError(wxT("SetHandleInformation(pipeIn)"));

        if ( !::SetHandleInformation(pipeOut[wxPipe::Read],
                                     HANDLE_FLAG_INHERIT, 0) )
            wxLogLastError(wxT("SetHandleInformation(pipeOut)"));

        if ( !::SetHandleInformation(pipeErr[wxPipe::Read],
                                     HANDLE_FLAG_INHERIT, 0) )
            wxLogLastError(wxT("SetHandleInformation(pipeErr)"));
    }
#endif // wxUSE_STREAMS

    // The default logic for showing the console is to show it only if the IO
    // is not redirected however wxEXEC_{SHOW,HIDE}_CONSOLE flags can be
    // explicitly specified to change it.
    if ( (flags & wxEXEC_HIDE_CONSOLE) ||
            (redirect && !(flags & wxEXEC_SHOW_CONSOLE)) )
    {
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }


    PROCESS_INFORMATION pi;
    DWORD dwFlags = CREATE_SUSPENDED;

    if ( (flags & wxEXEC_MAKE_GROUP_LEADER) )
        dwFlags |= CREATE_NEW_PROCESS_GROUP;

    dwFlags |= CREATE_DEFAULT_ERROR_MODE ;

    wxWCharBuffer envBuffer;
    bool useCwd = false;
    if ( env )
    {
        useCwd = !env->cwd.empty();

        // Translate environment variable map into NUL-terminated list of
        // NUL-terminated strings.
        if ( !env->env.empty() )
        {
            // Environment variables can contain non-ASCII characters. We could
            // check for it and not use this flag if everything is really ASCII
            // only but there doesn't seem to be any reason to do it so just
            // assume Unicode by default.
            dwFlags |= CREATE_UNICODE_ENVIRONMENT;

            wxEnvVariableHashMap::const_iterator it;

            size_t envSz = 1; // ending '\0'
            for ( it = env->env.begin(); it != env->env.end(); ++it )
            {
                // Add size of env variable name and value, and '=' char and
                // ending '\0'
                envSz += it->first.length() + it->second.length() + 2;
            }

            envBuffer.extend(envSz);

            wxChar *p = envBuffer.data();
            for ( it = env->env.begin(); it != env->env.end(); ++it )
            {
                const wxString line = it->first + wxS("=") + it->second;

                // Include the trailing NUL which will always terminate the
                // buffer returned by t_str().
                const size_t len = line.length() + 1;

                wxTmemcpy(p, line.t_str(), len);

                p += len;
            }

            // And another NUL to terminate the list of NUL-terminated strings.
            *p = 0;
        }
    }

    // Translate wxWidgets priority to Windows conventions.
    if ( handler )
    {
        unsigned prio = handler->GetPriority();
        if ( prio <= 20 )
            dwFlags |= IDLE_PRIORITY_CLASS;
        else if ( prio <= 40 )
            dwFlags |= BELOW_NORMAL_PRIORITY_CLASS;
        else if ( prio <= 60 )
            dwFlags |= NORMAL_PRIORITY_CLASS;
        else if ( prio <= 80 )
            dwFlags |= ABOVE_NORMAL_PRIORITY_CLASS;
        else if ( prio <= 99 )
            dwFlags |= HIGH_PRIORITY_CLASS;
        else if ( prio <= 100 )
            dwFlags |= REALTIME_PRIORITY_CLASS;
        else
        {
            wxFAIL_MSG(wxT("invalid value of thread priority parameter"));
            dwFlags |= NORMAL_PRIORITY_CLASS;
        }
    }

    bool ok = ::CreateProcess
                (
                 nullptr,            // application name (use only cmd line)
                 wxMSW_CONV_LPTSTR(command), // full command line
                 nullptr,            // security attributes: defaults for both
                 nullptr,            //   the process and its main thread
                 redirect,           // inherit handles if we use pipes
                 dwFlags,            // process creation flags
                 envBuffer.data(),   // environment (may be null which is fine)
                 useCwd              // initial working directory
                    ? wxMSW_CONV_LPTSTR(env->cwd)
                    : nullptr,       //     (or use the same)
                 &si,                // startup info (unused here)
                 &pi                 // process info
                ) != 0;

#if wxUSE_STREAMS
    // we can close the pipe ends used by child anyhow
    if ( redirect )
    {
        ::CloseHandle(pipeIn.Detach(wxPipe::Read));
        ::CloseHandle(pipeOut.Detach(wxPipe::Write));
        ::CloseHandle(pipeErr.Detach(wxPipe::Write));
    }
#endif // wxUSE_STREAMS

    if ( !ok )
    {
#if wxUSE_STREAMS
        // close the other handles too
        if ( redirect )
        {
            ::CloseHandle(pipeIn.Detach(wxPipe::Write));
            ::CloseHandle(pipeOut.Detach(wxPipe::Read));
            ::CloseHandle(pipeErr.Detach(wxPipe::Read));
        }
#endif // wxUSE_STREAMS

        wxLogSysError(_("Execution of command '%s' failed"), command);

        return flags & wxEXEC_SYNC ? -1 : 0;
    }

#if wxUSE_STREAMS
    // the input buffer bufOut is connected to stdout, this is why it is
    // called bufOut and not bufIn
    wxStreamTempInputBuffer bufOut,
                            bufErr;

    if ( redirect )
    {
        // We can now initialize the wxStreams
        wxPipeInputStream *
            outStream = new wxPipeInputStream(pipeOut.Detach(wxPipe::Read));
        wxPipeInputStream *
            errStream = new wxPipeInputStream(pipeErr.Detach(wxPipe::Read));
        wxPipeOutputStream *
            inStream = new wxPipeOutputStream(pipeIn.Detach(wxPipe::Write));

        handler->SetPipeStreams(outStream, inStream, errStream);

        bufOut.Init(outStream);
        bufErr.Init(errStream);
    }
#endif // wxUSE_STREAMS

    // create a hidden window to receive notification about process
    // termination
    HWND hwnd = wxCreateHiddenWindow
                (
                    &gs_classForHiddenWindow,
                    wxMSWEXEC_WNDCLASSNAME,
                    (WNDPROC)wxExecuteWindowCbk
                );

    wxASSERT_MSG( hwnd, wxT("can't create a hidden window for wxExecute") );

    // Alloc data
    wxExecuteData *data = new wxExecuteData;
    data->hProcess    = pi.hProcess;
    data->dwProcessId = pi.dwProcessId;
    data->hWnd        = hwnd;
    data->state       = (flags & wxEXEC_SYNC) != 0;

    if ( handler )
        handler->SetPid(pi.dwProcessId);

    if ( flags & wxEXEC_SYNC )
    {
        // handler may be non-null for capturing program output, but we don't use
        // it in wxExecuteData struct in this case because it's only needed
        // there for calling OnTerminate() on it and we don't do this when
        // executing synchronously
        data->handler = nullptr;
    }
    else
    {
        // may be null or not
        data->handler = handler;
    }

    DWORD tid;
    HANDLE hThread = ::CreateThread(nullptr,
                                    0,
                                    wxExecuteThread,
                                    (void *)data,
                                    0,
                                    &tid);

    // resume process we created now - whether the thread creation succeeded or
    // not
    if ( ::ResumeThread(pi.hThread) == (DWORD)-1 )
    {
        // ignore it - what can we do?
        wxLogLastError(wxT("ResumeThread in wxExecute"));
    }

    // close unneeded handle
    if ( !::CloseHandle(pi.hThread) )
    {
        wxLogLastError(wxT("CloseHandle(hThread)"));
    }

    if ( !hThread )
    {
        wxLogLastError(wxT("CreateThread in wxExecute"));

        DestroyWindow(hwnd);
        delete data;

        // the process still started up successfully...
        return pi.dwProcessId;
    }

    gs_asyncThreads.push_back(hThread);
    data->hThread = hThread;

#if wxUSE_IPC
    // second part of DDE hack: now establish the DDE conversation with the
    // just launched process
    if ( !ddeServer.empty() )
    {
        bool ddeOK;

        // give the process the time to init itself
        //
        // we use a very big timeout hoping that WaitForInputIdle() will return
        // much sooner, but not INFINITE just in case the process hangs
        // completely - like this we will regain control sooner or later
        switch ( ::WaitForInputIdle(pi.hProcess, 10000 /* 10 seconds */) )
        {
            default:
                wxFAIL_MSG( wxT("unexpected WaitForInputIdle() return code") );
                wxFALLTHROUGH;

            case WAIT_FAILED:
                wxLogLastError(wxT("WaitForInputIdle() in wxExecute"));
                wxFALLTHROUGH;

            case WAIT_TIMEOUT:
                wxLogDebug(wxT("Timeout too small in WaitForInputIdle"));

                ddeOK = false;
                break;

            case 0:
                // ok, process ready to accept DDE requests
                ddeOK = wxExecuteDDE(ddeServer, ddeTopic, ddeCommand);
        }

        if ( !ddeOK )
        {
            wxLogDebug(wxT("Failed to send DDE request to the process \"%s\"."),
                       cmd);
        }
    }
#endif // wxUSE_IPC

    if ( !(flags & wxEXEC_SYNC) )
    {
        // clean up will be done when the process terminates

        // return the pid
        return pi.dwProcessId;
    }

    wxAppTraits *traits = wxApp::GetTraitsIfExists();
    wxCHECK_MSG( traits, -1, wxT("no wxAppTraits in wxExecute()?") );

    void *cookie = nullptr;
    if ( !(flags & wxEXEC_NODISABLE) )
    {
        // disable all app windows while waiting for the child process to finish
        cookie = traits->BeforeChildWaitLoop();
    }

    // wait until the child process terminates
    while ( data->state )
    {
#if wxUSE_STREAMS
        if ( !bufOut.Update() && !bufErr.Update() )
#endif // wxUSE_STREAMS
        {
            // don't eat 100% of the CPU -- ugly but anything else requires
            // real async IO which we don't have for the moment
            ::Sleep(50);
        }

        // we must always process messages for our hidden window or we'd never
        // get wxWM_PROC_TERMINATED and so this loop would never terminate
        MSG msg;
        ::PeekMessage(&msg, data->hWnd, 0, 0, PM_REMOVE);

        // we may also need to process messages for all the other application
        // windows
        if ( !(flags & wxEXEC_NOEVENTS) )
        {
            wxEventLoopBase * const loop = wxEventLoopBase::GetActive();
            if ( loop )
                loop->Yield();
        }
    }

    if ( !(flags & wxEXEC_NODISABLE) )
    {
        // reenable disabled windows back
        traits->AfterChildWaitLoop(cookie);
    }

    DWORD dwExitCode = data->dwExitCode;
    delete data;

    // return the exit code
    return dwExitCode;
}

template <typename CharType>
long wxExecuteImpl(const CharType* const* argv, int flags, wxProcess* handler,
                   const wxExecuteEnv *env)
{
    wxString command;
    command.reserve(1024);

    wxString arg;
    for ( ;; )
    {
        arg = *argv++;

        bool quote;
        if ( arg.empty() )
        {
            // we need to quote empty arguments, otherwise they'd just
            // disappear
            quote = true;
        }
        else // non-empty
        {
            // escape any quotes present in the string to avoid interfering
            // with the command line parsing in the child process
            arg.Replace("\"", "\\\"", true /* replace all */);

            // and quote any arguments containing the spaces to prevent them from
            // being broken down
            quote = arg.find_first_of(" \t") != wxString::npos;
        }

        if ( quote )
            command += '\"' + arg + '\"';
        else
            command += arg;

        if ( !*argv )
            break;

        command += ' ';
    }

    return wxExecute(command, flags, handler, env);
}

long wxExecute(const char* const* argv, int flags, wxProcess* handler,
               const wxExecuteEnv *env)
{
    return wxExecuteImpl(argv, flags, handler, env);
}

long wxExecute(const wchar_t* const* argv, int flags, wxProcess* handler,
               const wxExecuteEnv *env)
{
    return wxExecuteImpl(argv, flags, handler, env);
}
