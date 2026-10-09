/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// GDB, as a debug adapter of theDebugManager: the program runs under
// "gdb --interpreter=mi3 --nx", a child that lives as long as the session
// and is spoken to over its pipes.  Commands go in with increasing tokens;
// the answer comes back as a result record with the same token; what happens
// on gdb's own ("*stopped", "=thread-created", stream output) a reader
// thread turns into events for the listeners.  Mirrors gitvcs in how a tool
// is wrapped and errors are reported with the tool's words -- but where git
// is many short runs, gdb is one long conversation, so RunProcess
// (process.cxx), which closes the child's input at once, does not fit, and
// the pipes are held open here (PipedProcess below).
//
// Lines are 1-based on the gdb side and 0-based in the API (as
// core::Position counts); the translation happens here, nowhere else.

#include <com/sun/star/code/core/XPlugin.hpp>
#include <com/sun/star/code/debug/Breakpoint.hpp>
#include <com/sun/star/code/debug/DebugEvent.hpp>
#include <com/sun/star/code/debug/DebugException.hpp>
#include <com/sun/star/code/debug/LaunchDescriptor.hpp>
#include <com/sun/star/code/debug/StackFrame.hpp>
#include <com/sun/star/code/debug/DebugThread.hpp>
#include <com/sun/star/code/debug/Variable.hpp>
#include <com/sun/star/code/debug/XDebugAdapter.hpp>
#include <com/sun/star/code/debug/XDebugConsole.hpp>
#include <com/sun/star/code/debug/XDebugSession.hpp>
#include <com/sun/star/code/debug/XDebugSessionListener.hpp>
#include <com/sun/star/code/debug/XNativeDebugSession.hpp>
#include <com/sun/star/code/debug/theDebugManager.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/interfacecontainer4.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/weakref.hxx>
#include <osl/file.h>
#include <osl/file.hxx>
#include <osl/process.h>
#include <osl/security.h>
#include <rtl/character.hxx>
#include <rtl/ref.hxx>
#include <rtl/strbuf.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <csignal>
#include <unistd.h>
#endif

using namespace css;
using namespace css::code;
using namespace css::code::debug;

namespace codeservices
{
namespace
{
// what an answer of gdb may take; running the program itself is not limited,
// as "*stopped" is an event, not an answer
constexpr std::chrono::seconds AnswerLimit(20);
// how long -gdb-exit gets before the process is ended by force
constexpr std::chrono::seconds QuitLimit(3);
// how many lines of gdb's log stream are kept as detail for the next error
constexpr size_t LogKept = 50;

OUString ToSystemPath(const OUString& rUrl)
{
    OUString aPath;
    return osl::FileBase::getSystemPathFromFileURL(rUrl, aPath) == osl::FileBase::E_None
               ? aPath
               : OUString();
}

// Program may come as a file URL or as a system path already
OUString ProgramPath(const OUString& rProgram)
{
    return rProgram.startsWith("file:") ? ToSystemPath(rProgram) : rProgram;
}

OUString UrlOfPath(const OUString& rPath)
{
    OUString aUrl;
    return osl::FileBase::getFileURLFromSystemPath(rPath, aUrl) == osl::FileBase::E_None
               ? aUrl
               : OUString();
}

// where PATH finds rProgram, or empty: how canDebug knows beforehand what
// launching would find out the hard way (git never pre-checks; an adapter
// must, so that the manager can pick another one instead)
OUString FindInPath(const OUString& rProgram)
{
    rtl_uString* pUrl = nullptr;
    if (osl_searchFileURL(rProgram.pData, nullptr, &pUrl) == osl_File_E_None)
        return OUString(pUrl, SAL_NO_ACQUIRE);
    return OUString();
}

// An argument as MI wants it on a command line: in double quotes, the quote
// and the backslash escaped.
OString MiQuoted(const OUString& rText)
{
    const OString aUtf8 = OUStringToOString(rText, RTL_TEXTENCODING_UTF8);
    OStringBuffer aResult("\"");
    for (sal_Int32 i = 0; i < aUtf8.getLength(); ++i)
    {
        const char c = aUtf8[i];
        if (c == '"' || c == '\\')
            aResult.append('\\');
        aResult.append(c);
    }
    aResult.append('"');
    return aResult.makeStringAndClear();
}

OString HexOf(sal_Int64 nValue)
{
    return OString::number(static_cast<sal_uInt64>(nValue), 16);
}

// ---- MI records ------------------------------------------------------------
//
// gdb answers in records: a result record "token^done,var=value,..." for a
// command, out-of-band records "*stopped,...", "=thread-created,...", and
// stream records "~\"text\"".  A value is a c-string, a tuple "{...}" of
// name=value pairs, or a list "[...]" of values -- whose elements gdb also
// writes as name=value at times ("stack=[frame={...},frame={...}]"); the
// names there repeat the list's meaning and are dropped.

struct MiValue
{
    OUString aText; // when the value is a c-string
    std::vector<std::pair<OString, MiValue>> aMembers; // when it is a tuple
    std::vector<MiValue> aItems; // when it is a list

    const MiValue* Find(const char* pName) const
    {
        for (const auto& rMember : aMembers)
            if (rMember.first == pName)
                return &rMember.second;
        return nullptr;
    }
    OUString Str(const char* pName) const
    {
        const MiValue* pValue = Find(pName);
        return pValue ? pValue->aText : OUString();
    }
    sal_Int32 Int(const char* pName) const { return Str(pName).toInt32(); }
};

struct MiRecord
{
    sal_Int64 nToken = -1; // the number the command was sent with; -1: none
    OString aClass; // "done", "error", "stopped", "breakpoint-modified", ...
    MiValue aResults; // the name=value pairs after the class, as a tuple
};

// Reads MI's grammar from one line, in bytes: the c-strings carry UTF-8 (with
// non-ASCII as octal escapes), and text is only made of them once unescaped.
// A character that fits nothing ends the parse; what was read so far stands.
class MiParser
{
public:
    MiParser(const OString& rText, sal_Int32 nPos)
        : m_rText(rText)
        , m_nPos(nPos)
    {
    }

    // "var=value,var=value,..." into rInto.aMembers
    void ParseResults(MiValue& rInto)
    {
        while (m_nPos < m_rText.getLength())
        {
            OString aName = ParseName();
            if (!Eat('='))
                break;
            MiValue aValue;
            if (!ParseValue(aValue))
                break;
            rInto.aMembers.emplace_back(std::move(aName), std::move(aValue));
            if (!Eat(','))
                break;
        }
    }

    bool ParseValue(MiValue& rValue)
    {
        if (m_nPos >= m_rText.getLength())
            return false;
        const char cOpen = m_rText[m_nPos];
        if (cOpen == '"')
            return ParseCString(rValue.aText);
        if (cOpen != '{' && cOpen != '[')
            return false;
        const char cClose = cOpen == '{' ? '}' : ']';
        ++m_nPos;
        while (m_nPos < m_rText.getLength() && m_rText[m_nPos] != cClose)
        {
            OString aName;
            const char cNext = m_rText[m_nPos];
            if (cNext != '"' && cNext != '{' && cNext != '[')
            {
                aName = ParseName();
                if (!Eat('='))
                    return false;
            }
            MiValue aInner;
            if (!ParseValue(aInner))
                return false;
            if (cOpen == '{')
                rValue.aMembers.emplace_back(std::move(aName), std::move(aInner));
            else
                rValue.aItems.push_back(std::move(aInner));
            Eat(',');
        }
        return Eat(cClose);
    }

    // between double quotes, with the escapes gdb writes; octal escapes are
    // the bytes of UTF-8
    bool ParseCString(OUString& rText)
    {
        if (!Eat('"'))
            return false;
        OStringBuffer aBytes;
        while (m_nPos < m_rText.getLength())
        {
            char c = m_rText[m_nPos++];
            if (c == '"')
            {
                rText = OUString::fromUtf8(aBytes);
                return true;
            }
            if (c == '\\' && m_nPos < m_rText.getLength())
            {
                c = m_rText[m_nPos++];
                if (c >= '0' && c <= '7')
                {
                    int nByte = c - '0';
                    for (int i = 0; i < 2 && m_nPos < m_rText.getLength()
                                    && m_rText[m_nPos] >= '0' && m_rText[m_nPos] <= '7';
                         ++i)
                        nByte = nByte * 8 + (m_rText[m_nPos++] - '0');
                    c = static_cast<char>(nByte);
                }
                else
                    switch (c)
                    {
                        case 'n': c = '\n'; break;
                        case 't': c = '\t'; break;
                        case 'r': c = '\r'; break;
                        case 'f': c = '\f'; break;
                        case 'v': c = '\v'; break;
                        case 'a': c = '\a'; break;
                        case 'b': c = '\b'; break;
                        default: break; // \" \\ and the rest: the character itself
                    }
            }
            aBytes.append(c);
        }
        return false; // the closing quote never came
    }

private:
    OString ParseName()
    {
        const sal_Int32 nStart = m_nPos;
        while (m_nPos < m_rText.getLength() && m_rText[m_nPos] != '=' && m_rText[m_nPos] != ','
               && m_rText[m_nPos] != '}' && m_rText[m_nPos] != ']')
            ++m_nPos;
        return m_rText.copy(nStart, m_nPos - nStart);
    }

    bool Eat(char c)
    {
        if (m_nPos < m_rText.getLength() && m_rText[m_nPos] == c)
        {
            ++m_nPos;
            return true;
        }
        return false;
    }

    const OString& m_rText;
    sal_Int32 m_nPos;
};

// ---- the gdb child ---------------------------------------------------------

// A child spoken with for the session's whole life, unlike RunProcess's
// run-to-the-end children: the input pipe stays open and carries the
// commands.  The handles live in this one shared object, held by the session
// and by the reader threads, so that whichever lets go last closes them.
struct PipedProcess
{
    oslProcess m_hProcess = nullptr;
    oslFileHandle m_hIn = nullptr;
    oslFileHandle m_hOut = nullptr;
    oslFileHandle m_hErr = nullptr;
    std::mutex m_aWriteMutex;

    ~PipedProcess()
    {
        for (oslFileHandle hPipe : { m_hIn, m_hOut, m_hErr })
            if (hPipe)
                osl_closeFile(hPipe);
        if (m_hProcess)
            osl_freeProcessHandle(m_hProcess);
    }

    // rEnvironment is "NAME=value" entries merged over ours, as RunProcess
    // does it; rWorkingDirectory is a file URL, or empty
    bool Start(const OUString& rProgram, const std::vector<OUString>& rArguments,
               const OUString& rWorkingDirectory, const std::vector<OUString>& rEnvironment,
               OUString& rError)
    {
        std::vector<OUString> aEnvironment;
#if defined(__linux__)
        if (!rEnvironment.empty())
        {
            auto nameOf
                = [](const OUString& rEntry) { return rEntry.copy(0, rEntry.indexOf('=') + 1); };
            for (char** ppEntry = environ; ppEntry && *ppEntry; ++ppEntry)
            {
                const OUString aEntry = OUString::fromUtf8(*ppEntry);
                if (std::none_of(rEnvironment.begin(), rEnvironment.end(),
                                 [&](const OUString& r) { return nameOf(r) == nameOf(aEntry); }))
                    aEnvironment.push_back(aEntry);
            }
            aEnvironment.insert(aEnvironment.end(), rEnvironment.begin(), rEnvironment.end());
        }
#endif
        std::vector<rtl_uString*> aEnvironmentData;
        for (const OUString& rEntry : aEnvironment)
            aEnvironmentData.push_back(rEntry.pData);
        std::vector<rtl_uString*> aArguments;
        for (const OUString& rArgument : rArguments)
            aArguments.push_back(rArgument.pData);

        oslSecurity hSecurity = osl_getCurrentSecurity();
        const oslProcessError eError = osl_executeProcess_WithRedirectedIO(
            rProgram.pData, aArguments.data(), static_cast<sal_uInt32>(aArguments.size()),
            osl_Process_SEARCHPATH | osl_Process_HIDDEN, hSecurity,
            rWorkingDirectory.isEmpty() ? nullptr : rWorkingDirectory.pData,
            aEnvironmentData.empty() ? nullptr : aEnvironmentData.data(),
            static_cast<sal_uInt32>(aEnvironmentData.size()), &m_hProcess, &m_hIn, &m_hOut,
            &m_hErr);
        osl_freeSecurityHandle(hSecurity);
        if (eError != osl_Process_E_None || !m_hIn || !m_hOut || !m_hErr)
        {
            if (eError == osl_Process_E_NotFound)
                rError = rProgram + " was not found (is it installed and on PATH?)";
            else
                rError = "cannot start " + rProgram;
            return false;
        }
        return true;
    }

    bool WriteLine(const OString& rLine)
    {
        std::scoped_lock aGuard(m_aWriteMutex);
        if (!m_hIn)
            return false;
        const OString aData = rLine + "\n";
        sal_uInt64 nWritten = 0;
        return osl_writeFile(m_hIn, aData.getStr(), static_cast<sal_uInt64>(aData.getLength()),
                             &nWritten)
                   == osl_File_E_None
               && nWritten == static_cast<sal_uInt64>(aData.getLength());
    }

    void Terminate()
    {
        if (m_hProcess)
            osl_terminateProcess(m_hProcess);
    }

    sal_Int32 Pid()
    {
        oslProcessInfo aInfo;
        aInfo.Size = sizeof aInfo;
        if (m_hProcess
            && osl_getProcessInfo(m_hProcess, osl_Process_IDENTIFIER, &aInfo)
                   == osl_Process_E_None)
            return static_cast<sal_Int32>(aInfo.Ident);
        return -1;
    }

    // SIGINT: how gdb is told to stop the program when it reads no commands
    // (all-stop mode without mi-async); gdb takes it and stops the inferior
    void Interrupt()
    {
#if defined(__linux__)
        const sal_Int32 nPid = Pid();
        if (nPid > 0)
            kill(static_cast<pid_t>(nPid), SIGINT);
#endif
    }
};

// Reads one pipe to its end, calling rSink for every complete line: as
// process.cxx's ReadLines, but in bytes -- the MI grammar is parsed before
// any text is made of it.
void ReadMiLines(oslFileHandle hPipe, const std::function<void(const OString&)>& rSink)
{
    OStringBuffer aLine;
    auto flush = [&]
    {
        sal_Int32 nLength = aLine.getLength();
        if (nLength > 0 && aLine[nLength - 1] == '\r')
            aLine.setLength(--nLength);
        rSink(aLine.toString());
        aLine.setLength(0);
    };

    char aBuffer[4096];
    sal_uInt64 nRead = 0;
    while (osl_readFile(hPipe, aBuffer, sizeof aBuffer, &nRead) == osl_File_E_None && nRead > 0)
    {
        for (sal_uInt64 i = 0; i < nRead; ++i)
        {
            if (aBuffer[i] == '\n')
                flush();
            else
                aLine.append(aBuffer[i]);
        }
    }
    if (aLine.getLength() > 0)
        flush();
}

// ---- the session -----------------------------------------------------------

class GdbSession final
    : public comphelper::WeakComponentImplHelper<XDebugSession, XNativeDebugSession, XDebugConsole>
{
public:
    GdbSession() = default;
    ~GdbSession() override
    {
        // if nobody disposed us, gdb must still not outlive the session
        if (std::shared_ptr<PipedProcess> pProcess = std::move(m_pProcess))
        {
            pProcess->WriteLine("-gdb-exit"_ostr);
            pProcess->Terminate();
        }
    }

    // Starts gdb on what rDescriptor names; throws DebugException if not.
    void Launch(const LaunchDescriptor& rDescriptor);

    // XDebugSession
    DebugSessionState SAL_CALL getState() override;
    void SAL_CALL resume(sal_Int32 nThreadId) override;
    void SAL_CALL pause(sal_Int32 nThreadId) override;
    void SAL_CALL stepOver(sal_Int32 nThreadId) override;
    void SAL_CALL stepInto(sal_Int32 nThreadId) override;
    void SAL_CALL stepOut(sal_Int32 nThreadId) override;
    void SAL_CALL terminate() override;
    Breakpoint SAL_CALL addBreakpoint(const Breakpoint& rBreakpoint) override;
    void SAL_CALL removeBreakpoint(sal_Int32 nId) override;
    uno::Sequence<Breakpoint> SAL_CALL getBreakpoints() override;
    uno::Sequence<DebugThread> SAL_CALL getThreads() override;
    uno::Sequence<StackFrame> SAL_CALL getStackFrames(sal_Int32 nThreadId) override;
    uno::Sequence<Variable> SAL_CALL getVariables(sal_Int32 nFrameId,
                                                  sal_Int32 nReference) override;
    OUString SAL_CALL evaluate(sal_Int32 nFrameId, const OUString& rExpression) override;
    void SAL_CALL addListener(const uno::Reference<XDebugSessionListener>& xListener) override;
    void SAL_CALL removeListener(const uno::Reference<XDebugSessionListener>& xListener) override;

    // XNativeDebugSession
    void SAL_CALL attachToProcess(sal_Int32 nPid) override;
    OUString SAL_CALL readMemory(sal_Int64 nAddress, sal_Int32 nCount) override;

    // XDebugConsole
    OUString SAL_CALL executeCommand(const OUString& rCommand) override;

    // called from the reader threads
    void HandleLine(const OString& rLine);
    void HandleErrLine(const OString& rLine);
    void ProcessEnded();

private:
    // where a frame handle points; gdb addresses frames as thread + level
    struct FramePlace
    {
        sal_Int32 nThread;
        sal_Int32 nLevel;
    };

    void disposing(std::unique_lock<std::mutex>& rGuard) override;

    std::shared_ptr<PipedProcess> GetProcess();
    void StartThreads(const std::shared_ptr<PipedProcess>& pProcess);
    // Sends the command and waits for its result record.  rWhat is the "cannot
    // ..." half of the error; gdb's own words are the other half.  With
    // pConsole, the console stream lines that arrive until that result are
    // collected into it instead of going out as OUTPUT events.
    MiValue Command(const OString& rCommand, const OUString& rWhat,
                    std::chrono::seconds aLimit = AnswerLimit, OUString* pConsole = nullptr);
    [[noreturn]] void Fail(const OUString& rWhat, const OUString& rWhy);
    void Exec(const OString& rCommand, const OUString& rWhat, sal_Int32 nThreadId);
    void DropVariableObjects();
    FramePlace FindFrame(sal_Int32 nFrameId);
    Variable MakeVariableObject(const OString& rContext, Variable aVariable);
    sal_Int32 RememberVarObject(const OUString& rName);
    void SetState(DebugSessionState eState, sal_Int32 nThreadId);
    void FireOutput(const OUString& rText);
    void FireEvent(DebugEvent aEvent);
    void OnResult(const MiRecord& rRecord);
    void OnStopped(const MiValue& rResults);
    void OnRunning();
    void OnNotify(const MiRecord& rRecord);
    static Breakpoint ReadBreakpoint(const MiValue& rBkpt, Breakpoint aInto);
    OUString RecentLog();

    std::shared_ptr<PipedProcess> m_pProcess; // under m_aMutex
    bool m_bStarted = false; // the program ran at least once (or was attached)
    bool m_bStopAtEntry = false;
    bool m_bAsync = false; // gdb took "mi-async on": -exec-interrupt works
    DebugSessionState m_eState = DebugSessionState_STARTING; // under m_aMutex

    // one command at a time; its token and its reply
    std::mutex m_aCommandMutex;
    sal_Int64 m_nNextToken = 0; // under m_aCommandMutex
    std::mutex m_aReplyMutex;
    std::condition_variable m_aReplyWake;
    sal_Int64 m_nWaitToken = 0; // under m_aReplyMutex; 0: nobody waits
    bool m_bHaveReply = false; // under m_aReplyMutex
    bool m_bGone = false; // under m_aReplyMutex; gdb ended
    MiRecord m_aReply; // under m_aReplyMutex
    std::deque<OUString> m_aLog; // under m_aReplyMutex; gdb's recent log stream
    bool m_bCaptureConsole = false; // under m_aReplyMutex; executeCommand collects
    OUStringBuffer m_aConsole; // under m_aReplyMutex; what it collected so far

    // under m_aMutex
    std::map<sal_Int32, Breakpoint> m_aBreakpoints;
    std::map<sal_Int32, FramePlace> m_aFrames;
    sal_Int32 m_nNextFrameId = 0;
    std::map<sal_Int32, OUString> m_aVarObjects; // Variable::Reference -> varobj
    std::vector<OUString> m_aVarRoots; // the varobjs made here, to delete
    sal_Int32 m_nNextReference = 0;
    comphelper::OInterfaceContainerHelper4<XDebugSessionListener> m_aListeners;
};

std::shared_ptr<PipedProcess> GdbSession::GetProcess()
{
    std::unique_lock aGuard(m_aMutex);
    return m_pProcess;
}

void GdbSession::Fail(const OUString& rWhat, const OUString& rWhy)
{
    throw DebugException(rWhat + ": " + rWhy, getXWeak(), RecentLog());
}

OUString GdbSession::RecentLog()
{
    OUStringBuffer aBuffer;
    std::scoped_lock aGuard(m_aReplyMutex);
    for (const OUString& rLine : m_aLog)
        aBuffer.append(rLine);
    return aBuffer.makeStringAndClear();
}

void GdbSession::Launch(const LaunchDescriptor& rDescriptor)
{
    const OUString aProgram = ProgramPath(rDescriptor.Program);
    if (aProgram.isEmpty())
        Fail(u"cannot debug"_ustr, u"no program"_ustr);

    auto pProcess = std::make_shared<PipedProcess>();
    OUString aError;
    const std::vector<OUString> aArguments{ u"--interpreter=mi3"_ustr, u"--nx"_ustr,
                                            u"--quiet"_ustr, aProgram };
    // The entries land in gdb's own environment, which the program inherits:
    // the one simple way MI offers to hand all of them down.
    const std::vector<OUString> aEnvironment(rDescriptor.Environment.begin(),
                                             rDescriptor.Environment.end());
    if (!pProcess->Start(u"gdb"_ustr, aArguments, rDescriptor.WorkingDirectory, aEnvironment,
                         aError))
        throw DebugException(aError, getXWeak(), OUString());
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        m_pProcess = pProcess;
        m_bStopAtEntry = rDescriptor.StopAtEntry;
    }
    StartThreads(pProcess);

    // What may be missing is asked for and let go: an old gdb without
    // mi-async still debugs, only pause() then goes by a signal.
    try
    {
        Command("-gdb-set mi-async on"_ostr, u"cannot set mi-async"_ustr);
        std::unique_lock aGuard(m_aMutex);
        m_bAsync = true;
    }
    catch (const DebugException&)
    {
    }
    try
    {
        // nobody is there to answer a question
        Command("-gdb-set confirm off"_ostr, u"cannot turn confirmations off"_ustr);
    }
    catch (const DebugException&)
    {
    }

    if (rDescriptor.Arguments.hasElements())
    {
        OStringBuffer aCommand("-exec-arguments");
        for (const OUString& rArgument : rDescriptor.Arguments)
            aCommand.append(" " + MiQuoted(rArgument));
        Command(aCommand.makeStringAndClear(), u"cannot set the program's arguments"_ustr);
    }
}

void GdbSession::StartThreads(const std::shared_ptr<PipedProcess>& pProcess)
{
    // The threads hold the session weakly, so that it can go away: then the
    // next line finds nobody and the loop ends with the pipe.  The handles
    // they read stay valid through the shared PipedProcess.
    uno::Reference<XDebugSession> xSelf(this);
    uno::WeakReference<XDebugSession> xWeak(xSelf);
    std::thread(
        [pProcess, xWeak]
        {
            ReadMiLines(pProcess->m_hOut,
                        [&](const OString& rLine)
                        {
                            uno::Reference<XDebugSession> xSession(xWeak);
                            if (xSession.is())
                                static_cast<GdbSession*>(xSession.get())->HandleLine(rLine);
                        });
            // the end of the pipe: gdb is gone
            uno::Reference<XDebugSession> xSession(xWeak);
            if (xSession.is())
                static_cast<GdbSession*>(xSession.get())->ProcessEnded();
        })
        .detach();
    std::thread(
        [pProcess, xWeak]
        {
            ReadMiLines(pProcess->m_hErr,
                        [&](const OString& rLine)
                        {
                            uno::Reference<XDebugSession> xSession(xWeak);
                            if (xSession.is())
                                static_cast<GdbSession*>(xSession.get())->HandleErrLine(rLine);
                        });
        })
        .detach();
}

MiValue GdbSession::Command(const OString& rCommand, const OUString& rWhat,
                            std::chrono::seconds aLimit, OUString* pConsole)
{
    std::shared_ptr<PipedProcess> pProcess = GetProcess();
    if (!pProcess)
        Fail(rWhat, u"gdb is not running"_ustr);

    std::scoped_lock aSerial(m_aCommandMutex);
    const sal_Int64 nToken = ++m_nNextToken;
    // Fail() reads the log under m_aReplyMutex, so it is never called with
    // that lock held here.
    {
        std::unique_lock aGuard(m_aReplyMutex);
        if (m_bGone)
        {
            aGuard.unlock();
            Fail(rWhat, u"gdb ended"_ustr);
        }
        m_nWaitToken = nToken;
        m_bHaveReply = false;
        // m_aCommandMutex serializes the commands, so everything the console
        // stream carries from here to the reply is this command's
        m_bCaptureConsole = pConsole != nullptr;
        m_aConsole.setLength(0);
    }
    if (!pProcess->WriteLine(OString::number(nToken) + rCommand))
    {
        {
            std::scoped_lock aGuard(m_aReplyMutex);
            m_nWaitToken = 0;
            m_bCaptureConsole = false;
        }
        Fail(rWhat, u"cannot write to gdb"_ustr);
    }

    MiRecord aReply;
    bool bAnswered;
    bool bHaveReply;
    {
        std::unique_lock aGuard(m_aReplyMutex);
        bAnswered
            = m_aReplyWake.wait_for(aGuard, aLimit, [&] { return m_bHaveReply || m_bGone; });
        bHaveReply = m_bHaveReply;
        m_nWaitToken = 0;
        m_bCaptureConsole = false;
        if (pConsole)
            *pConsole = m_aConsole.makeStringAndClear();
        if (bHaveReply)
            aReply = std::move(m_aReply);
    }
    if (!bAnswered)
        Fail(rWhat, u"gdb did not answer"_ustr); // a late answer is ignored by its token
    if (!bHaveReply)
        Fail(rWhat, u"gdb ended"_ustr);
    if (aReply.aClass == "error")
    {
        const OUString aMessage = aReply.aResults.Str("msg");
        Fail(rWhat, aMessage.isEmpty() ? u"gdb refused"_ustr : aMessage);
    }
    return std::move(aReply.aResults);
}

// ---- what the reader threads bring -----------------------------------------

void GdbSession::HandleLine(const OString& rLine)
{
    sal_Int32 nPos = 0;
    sal_Int64 nToken = -1;
    while (nPos < rLine.getLength()
           && rtl::isAsciiDigit(static_cast<unsigned char>(rLine[nPos])))
    {
        if (nToken < 0)
            nToken = 0;
        nToken = nToken * 10 + (rLine[nPos] - '0');
        ++nPos;
    }
    if (nPos >= rLine.getLength())
    {
        if (!rLine.isEmpty())
            FireOutput(OUString::fromUtf8(rLine) + "\n");
        return;
    }

    switch (rLine[nPos])
    {
        case '^': // the result of a command
        case '*': // the program started or stopped
        case '=': // something of gdb's own changed
        {
            MiRecord aRecord;
            aRecord.nToken = nToken;
            const char cKind = rLine[nPos++];
            const sal_Int32 nComma = rLine.indexOf(',', nPos);
            if (nComma < 0)
                aRecord.aClass = rLine.copy(nPos);
            else
            {
                aRecord.aClass = rLine.copy(nPos, nComma - nPos);
                MiParser(rLine, nComma + 1).ParseResults(aRecord.aResults);
            }
            if (cKind == '^')
                OnResult(aRecord);
            else if (cKind == '*')
            {
                if (aRecord.aClass == "stopped")
                    OnStopped(aRecord.aResults);
                else if (aRecord.aClass == "running")
                    OnRunning();
            }
            else
                OnNotify(aRecord);
            break;
        }
        case '~': // gdb's console output
        case '@': // the target's output, where the system routes it here
        {
            OUString aText;
            if (MiParser(rLine, nPos + 1).ParseCString(aText))
            {
                // while executeCommand runs, the console stream is its answer
                if (rLine[nPos] == '~')
                {
                    std::unique_lock aGuard(m_aReplyMutex);
                    if (m_bCaptureConsole)
                    {
                        m_aConsole.append(aText);
                        break;
                    }
                }
                FireOutput(aText);
            }
            break;
        }
        case '&': // gdb's log: kept quiet, but it is the detail of an error
        {
            OUString aText;
            if (MiParser(rLine, nPos + 1).ParseCString(aText))
            {
                std::scoped_lock aGuard(m_aReplyMutex);
                m_aLog.push_back(aText);
                while (m_aLog.size() > LogKept)
                    m_aLog.pop_front();
            }
            break;
        }
        default:
            if (rLine.startsWith("(gdb)"))
                break; // the prompt
            // not MI at all: the program wrote it to its stdout
            FireOutput(OUString::fromUtf8(rLine) + "\n");
    }
}

void GdbSession::HandleErrLine(const OString& rLine)
{
    FireOutput(OUString::fromUtf8(rLine) + "\n");
}

void GdbSession::ProcessEnded()
{
    {
        std::scoped_lock aGuard(m_aReplyMutex);
        m_bGone = true;
    }
    m_aReplyWake.notify_all();
    SetState(DebugSessionState_EXITED, 0);
}

void GdbSession::OnResult(const MiRecord& rRecord)
{
    bool bWake = false;
    {
        std::scoped_lock aGuard(m_aReplyMutex);
        if (m_nWaitToken != 0 && rRecord.nToken == m_nWaitToken)
        {
            m_aReply = rRecord;
            m_bHaveReply = true;
            bWake = true;
        }
        else if (rRecord.aClass == "error")
            SAL_WARN("code.services", "gdb: " << rRecord.aResults.Str("msg"));
    }
    if (bWake)
        m_aReplyWake.notify_all();
}

void GdbSession::OnStopped(const MiValue& rResults)
{
    if (rResults.Str("reason").startsWith("exited"))
    {
        SetState(DebugSessionState_EXITED, 0);
        return;
    }
    SetState(DebugSessionState_STOPPED, rResults.Int("thread-id"));
}

void GdbSession::OnRunning()
{
    {
        std::unique_lock aGuard(m_aMutex);
        // the handles were places in a stack that is gone now
        m_aFrames.clear();
    }
    SetState(DebugSessionState_RUNNING, 0);
}

void GdbSession::OnNotify(const MiRecord& rRecord)
{
    if (rRecord.aClass == "breakpoint-created" || rRecord.aClass == "breakpoint-modified")
    {
        const MiValue* pBkpt = rRecord.aResults.Find("bkpt");
        if (!pBkpt)
            return;
        {
            std::unique_lock aGuard(m_aMutex);
            if (m_bDisposed)
                return;
            const sal_Int32 nId = pBkpt->Int("number");
            auto it = m_aBreakpoints.find(nId);
            const Breakpoint aBase = it != m_aBreakpoints.end() ? it->second : Breakpoint();
            m_aBreakpoints[nId] = ReadBreakpoint(*pBkpt, aBase);
        }
        DebugEvent aEvent;
        aEvent.Kind = DebugEventKind_BREAKPOINT_CHANGED;
        FireEvent(std::move(aEvent));
    }
    else if (rRecord.aClass == "breakpoint-deleted")
    {
        {
            std::unique_lock aGuard(m_aMutex);
            if (m_bDisposed)
                return;
            m_aBreakpoints.erase(rRecord.aResults.Int("id"));
        }
        DebugEvent aEvent;
        aEvent.Kind = DebugEventKind_BREAKPOINT_CHANGED;
        FireEvent(std::move(aEvent));
    }
    else if (rRecord.aClass == "thread-created" || rRecord.aClass == "thread-exited")
    {
        DebugEvent aEvent;
        aEvent.Kind = DebugEventKind_THREADS_CHANGED;
        aEvent.ThreadId = rRecord.aResults.Int("id");
        FireEvent(std::move(aEvent));
    }
}

void GdbSession::SetState(DebugSessionState eState, sal_Int32 nThreadId)
{
    {
        std::unique_lock aGuard(m_aMutex);
        if (m_bDisposed || m_eState == eState || m_eState == DebugSessionState_EXITED)
            return;
        m_eState = eState;
    }
    DebugEvent aEvent;
    aEvent.Kind = DebugEventKind_STATE_CHANGED;
    aEvent.ThreadId = nThreadId;
    FireEvent(std::move(aEvent));
}

void GdbSession::FireOutput(const OUString& rText)
{
    DebugEvent aEvent;
    aEvent.Kind = DebugEventKind_OUTPUT;
    aEvent.Text = rText;
    FireEvent(std::move(aEvent));
}

// The listeners are called without the lock, from whatever thread brought the
// news (a reader thread, mostly) -- as the vcs listeners are; a UI marshals.
void GdbSession::FireEvent(DebugEvent aEvent)
{
    std::vector<uno::Reference<XDebugSessionListener>> aListeners;
    {
        std::unique_lock aGuard(m_aMutex);
        if (m_bDisposed)
            return;
        aEvent.Source = getXWeak();
        aEvent.State = m_eState;
        aListeners = m_aListeners.getElements(aGuard);
    }
    for (const auto& xListener : aListeners)
        xListener->debugEvent(aEvent);
}

// ---- running ---------------------------------------------------------------

DebugSessionState GdbSession::getState()
{
    std::unique_lock aGuard(m_aMutex);
    return m_eState;
}

void GdbSession::Exec(const OString& rCommand, const OUString& rWhat, sal_Int32 nThreadId)
{
    // the frames and variable handles die with the stop they came from
    DropVariableObjects();
    OString aCommand = rCommand;
    if (nThreadId > 0)
        aCommand += " --thread " + OString::number(nThreadId);
    Command(aCommand, rWhat);
}

void GdbSession::resume(sal_Int32 nThreadId)
{
    bool bStart = false;
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        bStart = !m_bStarted;
        m_bStarted = true;
    }
    if (!bStart)
    {
        Exec("-exec-continue"_ostr, u"cannot continue"_ustr, nThreadId);
        return;
    }
    try
    {
        Command(m_bStopAtEntry ? "-exec-run --start"_ostr : "-exec-run"_ostr,
                u"cannot run the program"_ustr);
    }
    catch (const DebugException&)
    {
        std::unique_lock aGuard(m_aMutex);
        m_bStarted = false;
        throw;
    }
}

void GdbSession::pause(sal_Int32 nThreadId)
{
    bool bAsync;
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        bAsync = m_bAsync;
    }
    if (bAsync)
    {
        OString aCommand = "-exec-interrupt"_ostr;
        if (nThreadId > 0)
            aCommand += " --thread " + OString::number(nThreadId);
        Command(aCommand, u"cannot pause"_ustr);
        return;
    }
    // All-stop without mi-async: gdb reads no commands while the program
    // runs, so the signal is the message; "*stopped" follows as an event.
    std::shared_ptr<PipedProcess> pProcess = GetProcess();
    if (!pProcess)
        Fail(u"cannot pause"_ustr, u"gdb is not running"_ustr);
    pProcess->Interrupt();
}

void GdbSession::stepOver(sal_Int32 nThreadId)
{
    Exec("-exec-next"_ostr, u"cannot step over"_ustr, nThreadId);
}

void GdbSession::stepInto(sal_Int32 nThreadId)
{
    Exec("-exec-step"_ostr, u"cannot step into"_ustr, nThreadId);
}

void GdbSession::stepOut(sal_Int32 nThreadId)
{
    Exec("-exec-finish"_ostr, u"cannot step out"_ustr, nThreadId);
}

void GdbSession::terminate()
{
    std::shared_ptr<PipedProcess> pProcess = GetProcess();
    if (pProcess)
    {
        try
        {
            Command("-gdb-exit"_ostr, u"cannot end gdb"_ustr, QuitLimit); // answers "^exit"
        }
        catch (const DebugException&)
        {
            // then by force; the readers see the pipes close and tell EXITED
            pProcess->Terminate();
        }
    }
    SetState(DebugSessionState_EXITED, 0);
}

// ---- breakpoints -----------------------------------------------------------

Breakpoint GdbSession::ReadBreakpoint(const MiValue& rBkpt, Breakpoint aInto)
{
    aInto.Id = rBkpt.Int("number");
    const OUString aFullname = rBkpt.Str("fullname");
    if (!aFullname.isEmpty())
    {
        const OUString aUrl = UrlOfPath(aFullname);
        if (!aUrl.isEmpty())
            aInto.Url = aUrl;
    }
    const OUString aLine = rBkpt.Str("line");
    if (!aLine.isEmpty())
        aInto.Line = aLine.toInt32() - 1; // gdb counts from one
    const OUString aEnabled = rBkpt.Str("enabled");
    if (!aEnabled.isEmpty())
        aInto.Enabled = aEnabled == "y";
    const OUString aCondition = rBkpt.Str("cond");
    if (!aCondition.isEmpty())
        aInto.Condition = aCondition;
    // "<PENDING>" is a place not found in any loaded code (yet); an address,
    // or "<MULTIPLE>" (one source place, several addresses), is resolved
    const OUString aAddr = rBkpt.Str("addr");
    aInto.Verified = !aAddr.isEmpty() && aAddr != "<PENDING>";
    return aInto;
}

Breakpoint GdbSession::addBreakpoint(const Breakpoint& rBreakpoint)
{
    const OUString aPath = rBreakpoint.Url.startsWith("file:") ? ToSystemPath(rBreakpoint.Url)
                                                               : rBreakpoint.Url;
    if (aPath.isEmpty())
        throw lang::IllegalArgumentException(u"no file"_ustr, getXWeak(), 0);
    // -f: a place not in the loaded code yet becomes a pending breakpoint
    // (Verified false) instead of a refusal
    OStringBuffer aCommand("-break-insert -f");
    if (!rBreakpoint.Enabled)
        aCommand.append(" -d");
    if (!rBreakpoint.Condition.isEmpty())
        aCommand.append(" -c " + MiQuoted(rBreakpoint.Condition));
    aCommand.append(" " + MiQuoted(aPath + ":" + OUString::number(rBreakpoint.Line + 1)));
    const MiValue aResults
        = Command(aCommand.makeStringAndClear(), u"cannot set the breakpoint"_ustr);
    const MiValue* pBkpt = aResults.Find("bkpt");
    if (!pBkpt)
        Fail(u"cannot set the breakpoint"_ustr, u"gdb said nothing of it"_ustr);
    const Breakpoint aResult = ReadBreakpoint(*pBkpt, rBreakpoint);
    {
        std::unique_lock aGuard(m_aMutex);
        m_aBreakpoints[aResult.Id] = aResult;
    }
    return aResult;
}

void GdbSession::removeBreakpoint(sal_Int32 nId)
{
    Command("-break-delete " + OString::number(nId), u"cannot remove the breakpoint"_ustr);
    std::unique_lock aGuard(m_aMutex);
    m_aBreakpoints.erase(nId);
}

uno::Sequence<Breakpoint> GdbSession::getBreakpoints()
{
    std::unique_lock aGuard(m_aMutex);
    std::vector<Breakpoint> aResult;
    for (const auto& rEntry : m_aBreakpoints)
        aResult.push_back(rEntry.second);
    return comphelper::containerToSequence(aResult);
}

// ---- looking around while stopped ------------------------------------------

uno::Sequence<DebugThread> GdbSession::getThreads()
{
    const MiValue aResults = Command("-thread-info"_ostr, u"cannot list the threads"_ustr);
    std::vector<DebugThread> aThreads;
    if (const MiValue* pThreads = aResults.Find("threads"))
        for (const MiValue& rThread : pThreads->aItems)
        {
            DebugThread aThread;
            aThread.Id = rThread.Int("id");
            aThread.Name = rThread.Str("name");
            if (aThread.Name.isEmpty())
                aThread.Name = rThread.Str("target-id");
            aThread.Stopped = rThread.Str("state") == "stopped";
            aThreads.push_back(std::move(aThread));
        }
    return comphelper::containerToSequence(aThreads);
}

uno::Sequence<StackFrame> GdbSession::getStackFrames(sal_Int32 nThreadId)
{
    if (nThreadId <= 0)
        throw lang::IllegalArgumentException(u"no thread"_ustr, getXWeak(), 0);
    const MiValue aResults = Command("-stack-list-frames --thread " + OString::number(nThreadId),
                                     u"cannot read the stack"_ustr);
    std::vector<StackFrame> aFrames;
    const MiValue* pStack = aResults.Find("stack");
    if (pStack)
    {
        std::unique_lock aGuard(m_aMutex);
        for (const MiValue& rFrame : pStack->aItems)
        {
            StackFrame aFrame;
            aFrame.Id = ++m_nNextFrameId;
            m_aFrames[aFrame.Id] = { nThreadId, rFrame.Int("level") };
            aFrame.FunctionName = rFrame.Str("func");
            const OUString aFullname = rFrame.Str("fullname");
            if (!aFullname.isEmpty())
                aFrame.Url = UrlOfPath(aFullname);
            const OUString aLine = rFrame.Str("line");
            aFrame.Line = aLine.isEmpty() ? 0 : aLine.toInt32() - 1;
            aFrame.Column = 0; // MI tells no column
            aFrame.ModuleName = rFrame.Str("from"); // the shared object, if not the program
            aFrames.push_back(std::move(aFrame));
        }
    }
    return comphelper::containerToSequence(aFrames);
}

GdbSession::FramePlace GdbSession::FindFrame(sal_Int32 nFrameId)
{
    std::unique_lock aGuard(m_aMutex);
    throwIfDisposed(aGuard);
    auto it = m_aFrames.find(nFrameId);
    if (it == m_aFrames.end())
        throw lang::IllegalArgumentException(u"unknown frame (the program ran since?)"_ustr,
                                             getXWeak(), 0);
    return it->second;
}

sal_Int32 GdbSession::RememberVarObject(const OUString& rName)
{
    std::unique_lock aGuard(m_aMutex);
    const sal_Int32 nReference = ++m_nNextReference;
    m_aVarObjects[nReference] = rName;
    return nReference;
}

// An aggregate local has no value in -stack-list-variables; a variable object
// renders it and numbers its children, and Reference keeps the varobj's name.
Variable GdbSession::MakeVariableObject(const OString& rContext, Variable aVariable)
{
    const MiValue aResults
        = Command("-var-create" + rContext + " - * " + MiQuoted(aVariable.Name),
                  "cannot look into " + aVariable.Name);
    const OUString aName = aResults.Str("name");
    {
        std::unique_lock aGuard(m_aMutex);
        m_aVarRoots.push_back(aName); // ours to -var-delete before running on
    }
    aVariable.Value = aResults.Str("value");
    if (aVariable.TypeName.isEmpty())
        aVariable.TypeName = aResults.Str("type");
    if (aResults.Int("numchild") > 0 || aResults.Str("dynamic") == "1")
    {
        aVariable.HasChildren = true;
        aVariable.Reference = RememberVarObject(aName);
    }
    return aVariable;
}

uno::Sequence<Variable> GdbSession::getVariables(sal_Int32 nFrameId, sal_Int32 nReference)
{
    std::vector<Variable> aVariables;
    if (nReference != 0)
    {
        OUString aName;
        {
            std::unique_lock aGuard(m_aMutex);
            throwIfDisposed(aGuard);
            auto it = m_aVarObjects.find(nReference);
            if (it == m_aVarObjects.end())
                throw lang::IllegalArgumentException(
                    u"unknown reference (the program ran since?)"_ustr, getXWeak(), 1);
            aName = it->second;
        }
        const MiValue aResults = Command("-var-list-children --simple-values " + MiQuoted(aName),
                                         u"cannot expand the variable"_ustr);
        if (const MiValue* pChildren = aResults.Find("children"))
            for (const MiValue& rChild : pChildren->aItems)
            {
                Variable aVariable;
                aVariable.Name = rChild.Str("exp");
                aVariable.Value = rChild.Str("value");
                aVariable.TypeName = rChild.Str("type");
                if (rChild.Int("numchild") > 0 || rChild.Str("dynamic") == "1")
                {
                    aVariable.HasChildren = true;
                    // a child varobj already exists under its own name
                    aVariable.Reference = RememberVarObject(rChild.Str("name"));
                }
                aVariables.push_back(std::move(aVariable));
            }
        return comphelper::containerToSequence(aVariables);
    }

    const FramePlace aPlace = FindFrame(nFrameId);
    const OString aContext = " --thread " + OString::number(aPlace.nThread) + " --frame "
                             + OString::number(aPlace.nLevel);
    const MiValue aResults = Command("-stack-list-variables" + aContext + " --simple-values",
                                     u"cannot read the variables"_ustr);
    if (const MiValue* pVariables = aResults.Find("variables"))
        for (const MiValue& rLocal : pVariables->aItems)
        {
            Variable aVariable;
            aVariable.Name = rLocal.Str("name");
            aVariable.TypeName = rLocal.Str("type");
            if (const MiValue* pValue = rLocal.Find("value"))
            {
                aVariable.Value = pValue->aText;
                aVariables.push_back(std::move(aVariable));
                continue;
            }
            try
            {
                aVariables.push_back(MakeVariableObject(aContext, aVariable));
            }
            catch (const DebugException&)
            {
                // show it nameless-valued rather than not at all
                aVariables.push_back(std::move(aVariable));
            }
        }
    return comphelper::containerToSequence(aVariables);
}

// Lets the varobjs of the last stop go, before the program runs on: gdb would
// keep them (and their frames' memory) for ever otherwise.
void GdbSession::DropVariableObjects()
{
    std::vector<OUString> aRoots;
    {
        std::unique_lock aGuard(m_aMutex);
        aRoots.swap(m_aVarRoots);
        m_aVarObjects.clear();
    }
    for (const OUString& rName : aRoots)
    {
        try
        {
            Command("-var-delete " + MiQuoted(rName), u"cannot drop a variable object"_ustr);
        }
        catch (const DebugException&)
        {
            // gdb may have lost it with its frame; nothing hangs on it
        }
    }
}

OUString GdbSession::evaluate(sal_Int32 nFrameId, const OUString& rExpression)
{
    const FramePlace aPlace = FindFrame(nFrameId);
    const MiValue aResults
        = Command("-data-evaluate-expression --thread " + OString::number(aPlace.nThread)
                      + " --frame " + OString::number(aPlace.nLevel) + " "
                      + MiQuoted(rExpression),
                  "cannot evaluate " + rExpression);
    return aResults.Str("value");
}

// ---- XNativeDebugSession ---------------------------------------------------

void GdbSession::attachToProcess(sal_Int32 nPid)
{
    Command("-target-attach " + OString::number(nPid),
            "cannot attach to " + OUString::number(nPid));
    std::unique_lock aGuard(m_aMutex);
    m_bStarted = true; // resume() continues now, it does not -exec-run
}

OUString GdbSession::readMemory(sal_Int64 nAddress, sal_Int32 nCount)
{
    if (nCount <= 0)
        return OUString();
    const MiValue aResults = Command("-data-read-memory-bytes 0x" + HexOf(nAddress) + " "
                                         + OString::number(nCount),
                                     u"cannot read the memory"_ustr);
    OUStringBuffer aDump;
    if (const MiValue* pMemory = aResults.Find("memory"))
        for (const MiValue& rBlock : pMemory->aItems)
            aDump.append(rBlock.Str("contents")); // hex digits, two per byte
    return aDump.makeStringAndClear();
}

// ---- XDebugConsole ---------------------------------------------------------

OUString GdbSession::executeCommand(const OUString& rCommand)
{
    // What the command prints comes over the console stream, not in the
    // result record: Command() collects that stream until the ^done.
    OUString aConsole;
    Command("-interpreter-exec console " + MiQuoted(rCommand), "cannot run " + rCommand,
            AnswerLimit, &aConsole);
    return aConsole;
}

// ---- listeners and the end -------------------------------------------------

void GdbSession::addListener(const uno::Reference<XDebugSessionListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.addInterface(aGuard, xListener);
}

void GdbSession::removeListener(const uno::Reference<XDebugSessionListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.removeInterface(aGuard, xListener);
}

void GdbSession::disposing(std::unique_lock<std::mutex>& rGuard)
{
    lang::EventObject aSource(getXWeak());
    m_aListeners.disposeAndClear(rGuard, aSource);
    std::shared_ptr<PipedProcess> pProcess = std::move(m_pProcess);
    m_aBreakpoints.clear();
    m_aFrames.clear();
    m_aVarObjects.clear();
    m_aVarRoots.clear();
    if (pProcess)
    {
        pProcess->WriteLine("-gdb-exit"_ostr); // politely first
        pProcess->Terminate();
    }
    {
        std::scoped_lock aReply(m_aReplyMutex);
        m_bGone = true;
    }
    m_aReplyWake.notify_all();
    comphelper::WeakComponentImplHelperBase::disposing(rGuard);
}

// ---- the adapter -----------------------------------------------------------

class GdbAdapter final : public cppu::WeakImplHelper<XDebugAdapter>
{
public:
    OUString SAL_CALL getName() override { return u"gdb"_ustr; }

    uno::Sequence<OUString> SAL_CALL getLanguages() override
    {
        return { u"c"_ustr, u"c++"_ustr };
    }

    sal_Bool SAL_CALL canDebug(const LaunchDescriptor& rDescriptor) override
    {
        if (FindInPath(u"gdb"_ustr).isEmpty())
            return false;
        const OUString aProgram = ProgramPath(rDescriptor.Program);
        if (aProgram.isEmpty())
            return false;
        const OUString aUrl = UrlOfPath(aProgram);
        osl::DirectoryItem aItem;
        return !aUrl.isEmpty() && osl::DirectoryItem::get(aUrl, aItem) == osl::FileBase::E_None;
    }

    uno::Reference<XDebugSession> SAL_CALL launch(const LaunchDescriptor& rDescriptor) override
    {
        rtl::Reference<GdbSession> xSession(new GdbSession);
        try
        {
            xSession->Launch(rDescriptor);
        }
        catch (const uno::Exception&)
        {
            xSession->dispose();
            throw;
        }
        return uno::Reference<XDebugSession>(xSession);
    }
};

class GdbDebugPlugin final : public cppu::WeakImplHelper<core::XPlugin, lang::XServiceInfo>
{
public:
    explicit GdbDebugPlugin(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    void SAL_CALL activate() override
    {
        theDebugManager::get(m_xContext)->registerAdapter(new GdbAdapter);
    }

    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.GdbDebugPlugin"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override
    {
        return { u"com.sun.star.code.core.Plugin"_ustr };
    }

private:
    uno::Reference<uno::XComponentContext> m_xContext;
};
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_GdbDebugPlugin_get_implementation(uno::XComponentContext* pContext,
                                                          uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::GdbDebugPlugin(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
