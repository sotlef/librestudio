/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// Git, as a version control system of theVcsManager: the working copy is
// recognised by its .git (a directory, or a file in a worktree or submodule),
// and everything is asked of the git program, which is run with an argument
// list (no shell), with its two streams apart, with messages in English, and
// with every prompt turned off, as there is nobody to answer it.
//
// What the branch widget shows is kept up to date by watching the files git
// changes (HEAD, the refs) and reading the state again when they change, so
// that a checkout made in a terminal shows as well.

#include "backgroundjob.hxx"
#include "process.hxx"

#include <com/sun/star/code/core/XPlugin.hpp>
#include <com/sun/star/code/core/theJobRegistry.hpp>
#include <com/sun/star/code/vcs/BranchInfo.hpp>
#include <com/sun/star/code/vcs/RepositoryEvent.hpp>
#include <com/sun/star/code/vcs/VcsException.hpp>
#include <com/sun/star/code/vcs/XBranchBrowser.hpp>
#include <com/sun/star/code/vcs/XRepository.hpp>
#include <com/sun/star/code/vcs/XRepositoryListener.hpp>
#include <com/sun/star/code/vcs/XVcsProvider.hpp>
#include <com/sun/star/code/vcs/git/ChangeKind.hpp>
#include <com/sun/star/code/vcs/git/CommitInfo.hpp>
#include <com/sun/star/code/vcs/git/FileChange.hpp>
#include <com/sun/star/code/vcs/git/StashEntry.hpp>
#include <com/sun/star/code/vcs/git/XGitRepository.hpp>
#include <com/sun/star/code/vcs/git/XGitWorkingCopy.hpp>
#include <com/sun/star/code/vcs/theVcsManager.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/interfacecontainer4.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/weakref.hxx>
#include <osl/file.hxx>
#include <rtl/ref.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::vcs;

namespace codeservices
{
namespace
{
// How often the files git changes are looked at; a branch switched elsewhere
// shows within this time.
constexpr std::chrono::seconds WatchEvery(2);
// What a question to git may take; the network operations are not limited, but
// can be cancelled.
constexpr std::chrono::seconds QueryLimit(20);

OUString ToSystemPath(const OUString& rUrl)
{
    OUString aPath;
    return osl::FileBase::getSystemPathFromFileURL(rUrl, aPath) == osl::FileBase::E_None
               ? aPath
               : OUString();
}

std::filesystem::path ToFsPath(const OUString& rSystemPath)
{
    const OString aUtf8 = OUStringToOString(rSystemPath, RTL_TEXTENCODING_UTF8);
    return std::filesystem::path(std::string(aUtf8.getStr(), aUtf8.getLength()));
}

OUString DirectoryOf(const OUString& rUrl)
{
    const sal_Int32 nSlash = rUrl.lastIndexOf('/');
    return nSlash > 0 ? rUrl.copy(0, nSlash) : rUrl;
}

bool Exists(const OUString& rUrl)
{
    osl::DirectoryItem aItem;
    return osl::DirectoryItem::get(rUrl, aItem) == osl::FileBase::E_None;
}

struct GitResult
{
    sal_Int32 nExit = -1; // -1: not started, or cancelled
    std::vector<OUString> aOut;
    std::vector<OUString> aErr;
    OUString aStartError;

    OUString First() const { return aOut.empty() ? OUString() : aOut.front(); }
    bool Ok() const { return nExit == 0; }
};

// the last line of what git said of a failure
OUString Reason(const GitResult& rResult)
{
    if (!rResult.aStartError.isEmpty())
        return rResult.aStartError;
    for (auto it = rResult.aErr.rbegin(); it != rResult.aErr.rend(); ++it)
        if (!it->trim().isEmpty())
            return *it;
    if (rResult.nExit < 0)
        return u"cancelled"_ustr;
    return "git exited with " + OUString::number(rResult.nExit);
}

OUString Joined(const GitResult& rResult)
{
    OUStringBuffer aBuffer;
    for (const auto* pLines : { &rResult.aOut, &rResult.aErr })
        for (const OUString& rLine : *pLines)
            aBuffer.append(rLine + "\n");
    return aBuffer.makeStringAndClear();
}

std::vector<OUString> Split(const OUString& rText, sal_Unicode cSeparator)
{
    std::vector<OUString> aParts;
    sal_Int32 nIndex = 0;
    for (;;)
    {
        aParts.push_back(rText.getToken(0, cSeparator, nIndex));
        if (nIndex < 0)
            break;
    }
    return aParts;
}

// "ahead 1, behind 2" (as for-each-ref says it)
void ParseTrack(const OUString& rTrack, sal_Int32& rAhead, sal_Int32& rBehind)
{
    rAhead = rBehind = 0;
    for (const OUString& rPart : Split(rTrack, ','))
    {
        const OUString aPart = rPart.trim();
        if (aPart.startsWith("ahead "))
            rAhead = aPart.copy(6).toInt32();
        else if (aPart.startsWith("behind "))
            rBehind = aPart.copy(7).toInt32();
    }
}

class GitRepository final
    : public comphelper::WeakComponentImplHelper<XRepository, XBranchBrowser, git::XGitRepository,
                                                 git::XGitWorkingCopy>
{
public:
    GitRepository(uno::Reference<uno::XComponentContext> xContext, OUString aRootUrl)
        : m_xContext(std::move(xContext))
        , m_aRootUrl(std::move(aRootUrl))
        , m_pWatch(std::make_shared<Watch>())
    {
    }
    ~GitRepository() override { StopWatching(); }

    // Reads the state the first time and starts watching; false (and the
    // reason) if git cannot be used on this working copy.
    bool Open(OUString& rReason);

    // XRepository
    OUString SAL_CALL getKind() override { return u"git"_ustr; }
    OUString SAL_CALL getRootUrl() override { return m_aRootUrl; }
    RepositorySummary SAL_CALL getSummary() override;
    void SAL_CALL refresh() override { Refresh(true); }
    void SAL_CALL addRepositoryListener(const uno::Reference<XRepositoryListener>& xListener) override;
    void SAL_CALL
    removeRepositoryListener(const uno::Reference<XRepositoryListener>& xListener) override;

    // XBranchBrowser
    uno::Sequence<BranchInfo> SAL_CALL getBranches() override;

    // XGitRepository
    void SAL_CALL checkout(const OUString& rBranch) override;
    void SAL_CALL createBranch(const OUString& rBranch, const OUString& rStartPoint,
                               sal_Bool bCheckout) override;
    void SAL_CALL fetch() override;
    void SAL_CALL pull() override;
    void SAL_CALL push() override;
    uno::Sequence<git::CommitInfo> SAL_CALL getOutgoingCommits() override;
    uno::Sequence<OUString> SAL_CALL getRemotes() override;

    // XGitWorkingCopy
    uno::Sequence<git::FileChange> SAL_CALL getChanges() override;
    void SAL_CALL stage(const uno::Sequence<OUString>& rPaths) override;
    void SAL_CALL unstage(const uno::Sequence<OUString>& rPaths) override;
    void SAL_CALL addUntracked(const uno::Sequence<OUString>& rPaths) override;
    void SAL_CALL rollback(const uno::Sequence<OUString>& rPaths) override;
    void SAL_CALL commit(const OUString& rMessage, sal_Bool bAll, sal_Bool bAmend) override;
    uno::Sequence<git::StashEntry> SAL_CALL getStashes() override;
    void SAL_CALL stash(const OUString& rMessage, sal_Bool bIncludeUntracked) override;
    void SAL_CALL stashPop(sal_Int32 nIndex) override;
    void SAL_CALL stashApply(sal_Int32 nIndex) override;
    void SAL_CALL stashDrop(sal_Int32 nIndex) override;

    // called by the watching thread
    void Poll();

private:
    struct Watch
    {
        std::mutex aMutex;
        std::condition_variable aWake;
        bool bStop = false;
    };

    void disposing(std::unique_lock<std::mutex>& rGuard) override;

    GitResult Git(const std::vector<OUString>& rArguments, bool bMutating,
                  const std::function<bool()>& rIsCancelled = {},
                  const std::function<void(const OUString&)>& rOnLine = {});
    [[noreturn]] void Fail(const OUString& rWhat, const GitResult& rResult);
    void Run(const OUString& rWhat, const std::vector<OUString>& rArguments);
    // as Run, but as a background job that can be seen and cancelled
    void RunAsJob(const OUString& rTitle, const std::vector<OUString>& rArguments);
    bool RefExists(const OUString& rRef);
    // bForce: tell the listeners even if the summary is the same (the working tree may differ)
    void Refresh(bool bForce = false);
    // the files as git status lists them, parsed
    std::vector<git::FileChange> ReadChanges();
    // git add/restore/... on a list of paths, never read as patterns
    void RunOnPaths(const OUString& rWhat, std::vector<OUString> aArguments,
                    const uno::Sequence<OUString>& rPaths);
    OUString StashName(sal_Int32 nIndex) const { return "stash@{" + OUString::number(nIndex) + "}"; }
    sal_uInt64 Signature();
    RepositorySummary Compose(const RepositorySummary& rBefore);
    void StartWatching();
    void StopWatching();
    void Changed(sal_Int32 nBusyDelta);

    uno::Reference<uno::XComponentContext> m_xContext;
    const OUString m_aRootUrl;
    OUString m_aRootPath;
    // where git keeps HEAD, and what is shared with other worktrees
    std::filesystem::path m_aGitDir;
    std::filesystem::path m_aCommonDir;

    std::shared_ptr<Watch> m_pWatch;
    std::mutex m_aRefreshMutex; // one refresh at a time, so events stay in order
    sal_uInt64 m_nSignature = 0; // under m_aRefreshMutex
    RepositorySummary m_aSummary; // under m_aMutex
    sal_Int32 m_nBusy = 0; // under m_aMutex
    comphelper::OInterfaceContainerHelper4<XRepositoryListener> m_aListeners;
};

GitResult GitRepository::Git(const std::vector<OUString>& rArguments, bool bMutating,
                             const std::function<bool()>& rIsCancelled,
                             const std::function<void(const OUString&)>& rOnLine)
{
    std::vector<OUString> aArguments{ u"-C"_ustr, m_aRootPath, u"--no-pager"_ustr };
    aArguments.insert(aArguments.end(), rArguments.begin(), rArguments.end());
    // Nobody can answer a question or edit a message: the operation must fail
    // (or go on) by itself.  Questions leave git's files alone.
    std::vector<OUString> aEnvironment{ u"LC_ALL=C"_ustr, u"GIT_TERMINAL_PROMPT=0"_ustr,
                                        u"GIT_LITERAL_PATHSPECS=1"_ustr };
    if (bMutating)
    {
        aEnvironment.push_back(u"GIT_EDITOR=true"_ustr);
        aEnvironment.push_back(u"GIT_MERGE_AUTOEDIT=no"_ustr);
    }
    else
        aEnvironment.push_back(u"GIT_OPTIONAL_LOCKS=0"_ustr);

    GitResult aResult;
    std::mutex aLines;
    const auto aDeadline = std::chrono::steady_clock::now() + QueryLimit;
    aResult.nExit = RunProcess(
        u"git"_ustr, aArguments, OUString(),
        [&](bool bStderr, const OUString& rLine)
        {
            {
                std::scoped_lock aGuard(aLines);
                (bStderr ? aResult.aErr : aResult.aOut).push_back(rLine);
            }
            if (rOnLine)
                rOnLine(rLine);
        },
        [&]
        {
            if (rIsCancelled)
                return rIsCancelled();
            return std::chrono::steady_clock::now() > aDeadline;
        },
        aResult.aStartError, aEnvironment);
    return aResult;
}

void GitRepository::Fail(const OUString& rWhat, const GitResult& rResult)
{
    throw VcsException(rWhat + ": " + Reason(rResult), getXWeak(), Joined(rResult));
}

void GitRepository::Run(const OUString& rWhat, const std::vector<OUString>& rArguments)
{
    const GitResult aResult = Git(rArguments, true, [] { return false; });
    if (!aResult.Ok())
        Fail(rWhat, aResult);
}

void GitRepository::RunAsJob(const OUString& rTitle, const std::vector<OUString>& rArguments)
{
    auto pPromise = std::make_shared<std::promise<GitResult>>();
    std::future<GitResult> aFuture = pPromise->get_future();
    rtl::Reference<BackgroundJob> xJob = BackgroundJob::Start(
        "code.vcs.git:" + rTitle + ":" + m_aRootUrl, "git " + rTitle,
        [xSelf = rtl::Reference<GitRepository>(this), pPromise, rArguments](BackgroundJob& rJob)
        {
            GitResult aResult;
            try
            {
                rJob.SetMessage(u"git"_ustr);
                aResult = xSelf->Git(rArguments, true, [&rJob] { return rJob.IsCancelled(); },
                                     [&rJob](const OUString& rLine) { rJob.SetMessage(rLine); });
            }
            catch (const std::exception&)
            {
                aResult = GitResult();
                aResult.aStartError = u"cannot run git"_ustr;
            }
            const bool bOk = aResult.Ok();
            pPromise->set_value(std::move(aResult));
            return bOk;
        });
    try
    {
        core::theJobRegistry::get(m_xContext)->registerJob(xJob);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.services", "cannot register the git job: " << rException.Message);
    }
    Changed(+1);
    const GitResult aResult = aFuture.get();
    Changed(-1);
    Refresh(); // what a pull or a fetch changed, without waiting for the watch
    if (!aResult.Ok())
        Fail("git " + rTitle, aResult);
}

bool GitRepository::RefExists(const OUString& rRef)
{
    return Git({ u"rev-parse"_ustr, u"--verify"_ustr, u"--quiet"_ustr, rRef }, false).Ok();
}

bool GitRepository::Open(OUString& rReason)
{
    m_aRootPath = ToSystemPath(m_aRootUrl);
    if (m_aRootPath.isEmpty())
    {
        rReason = u"not a local directory"_ustr;
        return false;
    }
    // where the files are; worktrees and submodules keep them elsewhere
    GitResult aDirs = Git({ u"rev-parse"_ustr, u"--path-format=absolute"_ustr, u"--git-dir"_ustr,
                            u"--git-common-dir"_ustr },
                          false);
    if (!aDirs.Ok() || aDirs.aOut.size() < 2)
    {
        // git older than 2.31 has no --path-format
        aDirs = Git({ u"rev-parse"_ustr, u"--absolute-git-dir"_ustr }, false);
        if (!aDirs.Ok() || aDirs.aOut.empty())
        {
            rReason = Reason(aDirs);
            return false;
        }
        aDirs.aOut.resize(2, aDirs.aOut.front());
    }
    m_aGitDir = ToFsPath(aDirs.aOut[0]);
    m_aCommonDir = ToFsPath(aDirs.aOut[1]);

    {
        std::scoped_lock aRefresh(m_aRefreshMutex);
        m_nSignature = Signature();
        m_aSummary = Compose(RepositorySummary());
    }
    StartWatching();
    return true;
}

// A number that changes when HEAD or a ref does.
sal_uInt64 GitRepository::Signature()
{
    sal_uInt64 nResult = 1469598103934665603ULL;
    auto mix = [&](const std::filesystem::path& rPath)
    {
        std::error_code aError;
        const auto aTime = std::filesystem::last_write_time(rPath, aError);
        if (aError)
            return;
        nResult = (nResult ^ static_cast<sal_uInt64>(aTime.time_since_epoch().count()))
                  * 1099511628211ULL;
        if (std::filesystem::is_regular_file(rPath, aError))
        {
            const auto nSize = std::filesystem::file_size(rPath, aError);
            if (!aError)
                nResult = (nResult ^ static_cast<sal_uInt64>(nSize)) * 1099511628211ULL;
        }
    };
    mix(m_aGitDir / "HEAD");
    mix(m_aGitDir / "index"); // what is staged
    mix(m_aCommonDir / "packed-refs");
    for (const char* pRoot : { "refs/heads", "refs/remotes" })
    {
        std::error_code aError;
        const auto aRoot = m_aCommonDir / pRoot;
        mix(aRoot);
        for (std::filesystem::recursive_directory_iterator it(
                 aRoot, std::filesystem::directory_options::skip_permission_denied, aError),
             end;
             !aError && it != end; it.increment(aError))
            mix(it->path());
    }
    return nResult;
}

// The summary as git says it now.  A question git cannot answer leaves what
// was shown.
RepositorySummary GitRepository::Compose(const RepositorySummary& rBefore)
{
    RepositorySummary aSummary = rBefore;
    OUString aDetail;
    const GitResult aBranch = Git({ u"symbolic-ref"_ustr, u"--short"_ustr, u"-q"_ustr, u"HEAD"_ustr },
                                  false);
    if (aBranch.Ok() && !aBranch.aOut.empty())
    {
        aSummary.Label = aBranch.First();
        aDetail = "Branch " + aSummary.Label;
    }
    else if (aBranch.nExit == 1) // not on a branch
    {
        const GitResult aHead = Git({ u"rev-parse"_ustr, u"--short"_ustr, u"HEAD"_ustr }, false);
        if (aHead.Ok() && !aHead.aOut.empty())
        {
            aSummary.Label = aHead.First();
            aDetail = "Detached HEAD at " + aSummary.Label;
        }
    }
    else
        aDetail = aSummary.Tooltip;

    aSummary.Ahead = aSummary.Behind = 0;
    const GitResult aUpstream = Git(
        { u"rev-parse"_ustr, u"--abbrev-ref"_ustr, u"--symbolic-full-name"_ustr, u"@{upstream}"_ustr },
        false);
    if (aUpstream.Ok() && !aUpstream.aOut.empty())
    {
        aDetail += " -> " + aUpstream.First();
        // "behind<TAB>ahead": the left side is the upstream's
        const GitResult aCounts = Git({ u"rev-list"_ustr, u"--left-right"_ustr, u"--count"_ustr,
                                        u"@{upstream}...HEAD"_ustr },
                                      false);
        if (aCounts.Ok() && !aCounts.aOut.empty())
        {
            const OUString aLine = aCounts.First().trim();
            const sal_Int32 nTab = aLine.indexOf('\t');
            if (nTab > 0)
            {
                aSummary.Behind = aLine.copy(0, nTab).toInt32();
                aSummary.Ahead = aLine.copy(nTab + 1).trim().toInt32();
            }
        }
        if (aSummary.Ahead > 0)
            aDetail += "\n" + OUString::number(aSummary.Ahead) + " to push";
        if (aSummary.Behind > 0)
            aDetail += "\n" + OUString::number(aSummary.Behind) + " to pull";
    }
    aSummary.Tooltip = aDetail;
    return aSummary;
}

RepositorySummary GitRepository::getSummary()
{
    std::unique_lock aGuard(m_aMutex);
    RepositorySummary aSummary = m_aSummary;
    aSummary.Busy = m_nBusy > 0;
    return aSummary;
}

void GitRepository::Refresh(bool bForce)
{
    RepositorySummary aNew;
    {
        std::scoped_lock aRefresh(m_aRefreshMutex);
        {
            std::unique_lock aGuard(m_aMutex);
            if (m_bDisposed)
                return;
            aNew = m_aSummary;
        }
        const sal_uInt64 nSignature = Signature();
        const bool bFilesChanged = nSignature != m_nSignature;
        m_nSignature = nSignature;
        aNew = Compose(aNew);
        std::unique_lock aGuard(m_aMutex);
        if (m_bDisposed)
            return;
        // the index moving is news to whoever shows the changes, though not to the summary
        if (!bForce && !bFilesChanged && aNew.Label == m_aSummary.Label
            && aNew.Tooltip == m_aSummary.Tooltip && aNew.Ahead == m_aSummary.Ahead
            && aNew.Behind == m_aSummary.Behind)
            return;
        m_aSummary = aNew;
    }
    Changed(0);
}

// Tells the listeners; nBusyDelta is a change of the number of running operations.
void GitRepository::Changed(sal_Int32 nBusyDelta)
{
    RepositoryEvent aEvent;
    std::vector<uno::Reference<XRepositoryListener>> aListeners;
    {
        std::unique_lock aGuard(m_aMutex);
        if (m_bDisposed)
            return;
        m_nBusy += nBusyDelta;
        aEvent.Source = getXWeak();
        aEvent.Kind = RepositoryEventKind_CHANGED;
        aEvent.RootUrl = m_aRootUrl;
        aEvent.Summary = m_aSummary;
        aEvent.Summary.Busy = m_nBusy > 0;
        aListeners = m_aListeners.getElements(aGuard);
    }
    for (const auto& xListener : aListeners)
        xListener->repositoryChanged(aEvent);
}

void GitRepository::Poll()
{
    sal_uInt64 nSignature;
    {
        std::scoped_lock aRefresh(m_aRefreshMutex);
        nSignature = Signature();
        if (nSignature == m_nSignature)
            return;
    }
    Refresh();
}

void GitRepository::StartWatching()
{
    // The thread holds the repository only while it looks at it, so that the
    // repository can go away; then the thread finds out and ends.
    uno::Reference<XRepository> xSelf(this);
    std::thread(
        [pWatch = m_pWatch, xWeak = uno::WeakReference<XRepository>(xSelf)]
        {
            for (;;)
            {
                {
                    std::unique_lock aGuard(pWatch->aMutex);
                    if (pWatch->aWake.wait_for(aGuard, WatchEvery, [&] { return pWatch->bStop; }))
                        return;
                }
                uno::Reference<XRepository> xRepository(xWeak);
                if (!xRepository.is())
                    return;
                try
                {
                    static_cast<GitRepository*>(xRepository.get())->Poll();
                }
                catch (const std::exception&)
                {
                }
                catch (const uno::Exception&)
                {
                }
            }
        })
        .detach();
}

void GitRepository::StopWatching()
{
    {
        std::scoped_lock aGuard(m_pWatch->aMutex);
        m_pWatch->bStop = true;
    }
    m_pWatch->aWake.notify_all();
}

void GitRepository::disposing(std::unique_lock<std::mutex>& rGuard)
{
    lang::EventObject aSource(getXWeak());
    m_aListeners.disposeAndClear(rGuard, aSource);
    StopWatching();
    comphelper::WeakComponentImplHelperBase::disposing(rGuard);
}

void GitRepository::addRepositoryListener(const uno::Reference<XRepositoryListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.addInterface(aGuard, xListener);
}

void GitRepository::removeRepositoryListener(const uno::Reference<XRepositoryListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.removeInterface(aGuard, xListener);
}

uno::Sequence<BranchInfo> GitRepository::getBranches()
{
    // refname, "*" on the current branch, the upstream, how far from it
    const GitResult aResult = Git(
        { u"for-each-ref"_ustr,
          u"--format=%(refname)%09%(HEAD)%09%(upstream:short)%09%(upstream:track,nobracket)"_ustr,
          u"refs/heads"_ustr, u"refs/remotes"_ustr },
        false);
    if (!aResult.Ok())
        Fail(u"cannot list the branches"_ustr, aResult);

    std::vector<BranchInfo> aBranches;
    for (const OUString& rLine : aResult.aOut)
    {
        const std::vector<OUString> aFields = Split(rLine, '\t');
        if (aFields.size() < 4)
            continue;
        BranchInfo aBranch;
        const OUString& rRef = aFields[0];
        if (rRef.startsWith("refs/heads/", &aBranch.Name))
        {
            aBranch.IsCurrent = aFields[1] == "*";
            aBranch.Upstream = aFields[2];
            ParseTrack(aFields[3], aBranch.Ahead, aBranch.Behind);
        }
        else if (rRef.startsWith("refs/remotes/", &aBranch.Name))
        {
            if (aBranch.Name.endsWith("/HEAD")) // origin/HEAD is an alias
                continue;
            aBranch.Remote = aBranch.Name.copy(0, std::max<sal_Int32>(aBranch.Name.indexOf('/'), 0));
        }
        else
            continue;
        aBranches.push_back(std::move(aBranch));
    }
    return comphelper::containerToSequence(aBranches);
}

void GitRepository::checkout(const OUString& rBranch)
{
    if (rBranch.isEmpty())
        throw lang::IllegalArgumentException(u"no branch"_ustr, getXWeak(), 0);
    OUString aTarget = rBranch;
    std::vector<OUString> aArguments{ u"switch"_ustr };
    if (!RefExists("refs/heads/" + rBranch) && RefExists("refs/remotes/" + rBranch))
    {
        // a remote branch: its local twin, made if there is none yet
        const OUString aLocal = rBranch.copy(rBranch.indexOf('/') + 1);
        if (RefExists("refs/heads/" + aLocal))
            aTarget = aLocal;
        else
            aArguments.push_back(u"--track"_ustr);
    }
    aArguments.push_back(aTarget);
    Run("cannot switch to " + rBranch, aArguments);
    Refresh();
}

void GitRepository::createBranch(const OUString& rBranch, const OUString& rStartPoint,
                                 sal_Bool bCheckout)
{
    if (rBranch.isEmpty())
        throw lang::IllegalArgumentException(u"no branch name"_ustr, getXWeak(), 0);
    std::vector<OUString> aArguments;
    if (bCheckout)
        aArguments = { u"switch"_ustr, u"-c"_ustr, rBranch };
    else
        aArguments = { u"branch"_ustr, rBranch };
    if (!rStartPoint.isEmpty())
        aArguments.push_back(rStartPoint);
    Run("cannot create " + rBranch, aArguments);
    Refresh();
}

void GitRepository::fetch() { RunAsJob(u"fetch"_ustr, { u"fetch"_ustr, u"--all"_ustr, u"--prune"_ustr }); }

void GitRepository::pull() { RunAsJob(u"pull"_ustr, { u"pull"_ustr }); }

void GitRepository::push()
{
    const GitResult aUpstream = Git({ u"rev-parse"_ustr, u"--abbrev-ref"_ustr,
                                      u"--symbolic-full-name"_ustr, u"@{upstream}"_ustr },
                                    false);
    if (aUpstream.Ok())
    {
        RunAsJob(u"push"_ustr, { u"push"_ustr });
        return;
    }
    // a branch that follows nothing yet: publish it
    const uno::Sequence<OUString> aRemotes = getRemotes();
    if (!aRemotes.hasElements())
        throw VcsException(u"cannot push: there is no remote"_ustr, getXWeak(), OUString());
    const OUString aRemote
        = std::find(aRemotes.begin(), aRemotes.end(), u"origin"_ustr) != aRemotes.end()
              ? u"origin"_ustr
              : aRemotes[0];
    RunAsJob(u"push"_ustr, { u"push"_ustr, u"--set-upstream"_ustr, aRemote, u"HEAD"_ustr });
}

uno::Sequence<git::CommitInfo> GitRepository::getOutgoingCommits()
{
    std::vector<OUString> aArguments{ u"log"_ustr, u"--format=%h%x09%an%x09%s"_ustr };
    if (Git({ u"rev-parse"_ustr, u"--abbrev-ref"_ustr, u"--symbolic-full-name"_ustr,
              u"@{upstream}"_ustr },
            false)
            .Ok())
        aArguments.push_back(u"@{upstream}..HEAD"_ustr);
    else
    {
        aArguments.push_back(u"HEAD"_ustr);
        aArguments.push_back(u"--not"_ustr);
        aArguments.push_back(u"--remotes"_ustr);
    }
    const GitResult aResult = Git(aArguments, false);
    if (!aResult.Ok())
        Fail(u"cannot list the commits to push"_ustr, aResult);
    std::vector<git::CommitInfo> aCommits;
    for (const OUString& rLine : aResult.aOut)
    {
        const std::vector<OUString> aFields = Split(rLine, '\t');
        if (aFields.size() < 3)
            continue;
        git::CommitInfo aCommit;
        aCommit.Hash = aFields[0];
        aCommit.Author = aFields[1];
        aCommit.Subject = aFields[2];
        aCommits.push_back(std::move(aCommit));
    }
    return comphelper::containerToSequence(aCommits);
}

uno::Sequence<OUString> GitRepository::getRemotes()
{
    const GitResult aResult = Git({ u"remote"_ustr }, false);
    if (!aResult.Ok())
        Fail(u"cannot list the remotes"_ustr, aResult);
    std::vector<OUString> aRemotes;
    for (const OUString& rLine : aResult.aOut)
        if (!rLine.trim().isEmpty())
            aRemotes.push_back(rLine.trim());
    return comphelper::containerToSequence(aRemotes);
}

// ---- the working copy ------------------------------------------------------

namespace
{
git::ChangeKind KindOf(sal_Unicode c)
{
    switch (c)
    {
        case 'A':
            return git::ChangeKind_ADDED;
        case 'D':
            return git::ChangeKind_DELETED;
        case 'R':
            return git::ChangeKind_RENAMED;
        case 'C':
            return git::ChangeKind_COPIED;
        case 'U':
            return git::ChangeKind_CONFLICTED;
        default: // M, and T (the type of the file changed)
            return git::ChangeKind_MODIFIED;
    }
}

// "a b c d" in at most nFields parts: the last takes the rest, spaces and all
std::vector<OUString> SplitFields(const OUString& rLine, int nFields)
{
    std::vector<OUString> aFields;
    sal_Int32 nFrom = 0;
    while (static_cast<int>(aFields.size()) + 1 < nFields)
    {
        const sal_Int32 nBlank = rLine.indexOf(' ', nFrom);
        if (nBlank < 0)
            break;
        aFields.push_back(rLine.copy(nFrom, nBlank - nFrom));
        nFrom = nBlank + 1;
    }
    aFields.push_back(rLine.copy(nFrom));
    return aFields;
}
}

std::vector<git::FileChange> GitRepository::ReadChanges()
{
    // -z: names are not quoted, and what separates the records is a NUL
    const GitResult aResult
        = Git({ u"status"_ustr, u"--porcelain=v2"_ustr, u"-z"_ustr, u"--untracked-files=all"_ustr },
              false, [] { return false; });
    if (!aResult.Ok())
        Fail(u"cannot read the changes"_ustr, aResult);

    OUStringBuffer aAll;
    for (size_t i = 0; i < aResult.aOut.size(); ++i)
        aAll.append((i ? u"\n"_ustr : u""_ustr) + aResult.aOut[i]);
    const std::vector<OUString> aRecords = Split(aAll.makeStringAndClear(), 0);

    auto urlOf = [&](const OUString& rPath)
    {
        OUString aUrl;
        osl::FileBase::getFileURLFromSystemPath(m_aRootPath + "/" + rPath, aUrl);
        return aUrl;
    };

    std::vector<git::FileChange> aChanges;
    for (size_t i = 0; i < aRecords.size(); ++i)
    {
        const OUString& rRecord = aRecords[i];
        if (rRecord.getLength() < 3)
            continue;
        git::FileChange aChange;
        const sal_Unicode cKind = rRecord[0];
        if (cKind == '?')
        {
            aChange.Path = rRecord.copy(2);
            aChange.HasUnstaged = true;
            aChange.Unstaged = git::ChangeKind_UNTRACKED;
        }
        else if (cKind == '1' || cKind == '2' || cKind == 'u')
        {
            const int nFields = cKind == '1' ? 9 : cKind == '2' ? 10 : 11;
            const std::vector<OUString> aFields = SplitFields(rRecord, nFields);
            if (static_cast<int>(aFields.size()) != nFields || aFields[1].getLength() != 2)
                continue;
            aChange.Path = aFields.back();
            if (cKind == '2' && i + 1 < aRecords.size())
                aChange.OldPath = aRecords[++i]; // the record that follows is where it was
            const sal_Unicode cStaged = aFields[1][0];
            const sal_Unicode cUnstaged = aFields[1][1];
            if (cKind == 'u')
            {
                aChange.HasUnstaged = true;
                aChange.Unstaged = git::ChangeKind_CONFLICTED;
            }
            else
            {
                if (cStaged != '.')
                {
                    aChange.HasStaged = true;
                    aChange.Staged = KindOf(cStaged);
                }
                if (cUnstaged != '.')
                {
                    aChange.HasUnstaged = true;
                    aChange.Unstaged = KindOf(cUnstaged);
                }
            }
        }
        else
            continue; // ignored files, headers
        aChange.Url = urlOf(aChange.Path);
        aChanges.push_back(std::move(aChange));
    }
    return aChanges;
}

uno::Sequence<git::FileChange> GitRepository::getChanges()
{
    return comphelper::containerToSequence(ReadChanges());
}

// A command on files: "--" ends the options, so a name that starts with a dash is a name.
void GitRepository::RunOnPaths(const OUString& rWhat, std::vector<OUString> aArguments,
                               const uno::Sequence<OUString>& rPaths)
{
    if (!rPaths.hasElements())
        return;
    aArguments.push_back(u"--"_ustr);
    aArguments.insert(aArguments.end(), rPaths.begin(), rPaths.end());
    Run(rWhat, aArguments);
}

void GitRepository::stage(const uno::Sequence<OUString>& rPaths)
{
    RunOnPaths(u"cannot stage"_ustr, { u"add"_ustr }, rPaths);
    Refresh(true);
}

void GitRepository::addUntracked(const uno::Sequence<OUString>& rPaths) { stage(rPaths); }

void GitRepository::unstage(const uno::Sequence<OUString>& rPaths)
{
    if (RefExists(u"HEAD"_ustr))
        RunOnPaths(u"cannot unstage"_ustr, { u"restore"_ustr, u"--staged"_ustr }, rPaths);
    else // no commit yet: there is nothing to restore from; forget the files
        RunOnPaths(u"cannot unstage"_ustr, { u"rm"_ustr, u"--cached"_ustr, u"-r"_ustr, u"-q"_ustr },
                   rPaths);
    Refresh(true);
}

void GitRepository::rollback(const uno::Sequence<OUString>& rPaths)
{
    // each kind of file is thrown away by its own command: restore knows only
    // what the last commit has, an untracked file is for clean, and a file that
    // was added and never committed has nothing to go back to but nothing
    const std::vector<git::FileChange> aChanges = ReadChanges();
    std::vector<OUString> aRestore, aRemove, aClean;
    for (const OUString& rPath : rPaths)
    {
        auto it = std::find_if(aChanges.begin(), aChanges.end(),
                               [&](const git::FileChange& r) { return r.Path == rPath; });
        if (it == aChanges.end())
            continue; // already as in the last commit
        if (it->HasUnstaged && it->Unstaged == git::ChangeKind_UNTRACKED)
            aClean.push_back(rPath);
        else if (it->HasStaged && it->Staged == git::ChangeKind_ADDED)
            aRemove.push_back(rPath);
        else
            aRestore.push_back(rPath);
    }
    if (!aRestore.empty())
        RunOnPaths(u"cannot roll back"_ustr,
                   { u"restore"_ustr, u"--source=HEAD"_ustr, u"--staged"_ustr, u"--worktree"_ustr },
                   comphelper::containerToSequence(aRestore));
    if (!aRemove.empty())
        RunOnPaths(u"cannot roll back"_ustr, { u"rm"_ustr, u"-f"_ustr, u"-r"_ustr, u"-q"_ustr },
                   comphelper::containerToSequence(aRemove));
    if (!aClean.empty())
        RunOnPaths(u"cannot roll back"_ustr, { u"clean"_ustr, u"-f"_ustr, u"-q"_ustr },
                   comphelper::containerToSequence(aClean));
    Refresh(true);
}

void GitRepository::commit(const OUString& rMessage, sal_Bool bAll, sal_Bool bAmend)
{
    if (rMessage.trim().isEmpty() && !bAmend)
        throw lang::IllegalArgumentException(u"a commit needs a message"_ustr, getXWeak(), 0);

    std::vector<OUString> aArguments{ u"commit"_ustr };
    if (bAll)
        aArguments.push_back(u"-a"_ustr);
    // The message goes by a file: no quoting, and any characters, whatever the
    // command line and the locale make of them.
    const std::filesystem::path aMessageFile = m_aGitDir / "CODE_COMMIT_MSG";
    const bool bMessage = !rMessage.trim().isEmpty();
    if (bAmend)
        aArguments.push_back(u"--amend"_ustr);
    if (bMessage)
    {
        const OString aUtf8 = OUStringToOString(rMessage, RTL_TEXTENCODING_UTF8);
        {
            std::ofstream aFile(aMessageFile, std::ios::binary | std::ios::trunc);
            aFile.write(aUtf8.getStr(), aUtf8.getLength());
            if (!aFile)
                throw VcsException(u"cannot write the commit message"_ustr, getXWeak(), OUString());
        }
        aArguments.push_back(u"-F"_ustr);
        aArguments.push_back(OUString::fromUtf8(aMessageFile.string()));
    }
    else
        aArguments.push_back(u"--no-edit"_ustr);

    const GitResult aResult = Git(aArguments, true, [] { return false; });
    if (bMessage)
    {
        std::error_code aError;
        std::filesystem::remove(aMessageFile, aError);
    }
    if (!aResult.Ok())
    {
        // "nothing to commit" says so on stdout
        OUString aWhy = Reason(aResult);
        for (const OUString& rLine : aResult.aOut)
            if (rLine.startsWith("nothing to commit") || rLine.startsWith("no changes added"))
            {
                aWhy = rLine;
                break;
            }
        throw VcsException("cannot commit: " + aWhy, getXWeak(), Joined(aResult));
    }
    Refresh(true);
}

uno::Sequence<git::StashEntry> GitRepository::getStashes()
{
    const GitResult aResult
        = Git({ u"stash"_ustr, u"list"_ustr, u"--format=%gd%x09%gs"_ustr }, false);
    if (!aResult.Ok())
        Fail(u"cannot list the stashes"_ustr, aResult);
    std::vector<git::StashEntry> aStashes;
    for (const OUString& rLine : aResult.aOut)
    {
        const sal_Int32 nTab = rLine.indexOf('\t');
        if (nTab < 0)
            continue;
        git::StashEntry aStash;
        // "stash@{N}"
        const OUString aName = rLine.copy(0, nTab);
        const sal_Int32 nOpen = aName.indexOf('{');
        aStash.Index = nOpen < 0 ? 0 : aName.copy(nOpen + 1).toInt32();
        // "WIP on <branch>: <commit> <subject>" or "On <branch>: <message>"
        OUString aSubject = rLine.copy(nTab + 1);
        const sal_Int32 nOn = aSubject.indexOf(" on ") >= 0 ? aSubject.indexOf(" on ") + 4
                              : aSubject.startsWith("On ") ? 3
                                                           : -1;
        const sal_Int32 nColon = nOn < 0 ? -1 : aSubject.indexOf(": ", nOn);
        if (nOn >= 0 && nColon > nOn)
        {
            aStash.Branch = aSubject.copy(nOn, nColon - nOn);
            aStash.Message = aSubject.copy(nColon + 2);
            if (aSubject.startsWith("WIP"))
                aStash.Message = u"WIP: "_ustr + aStash.Message;
        }
        else
            aStash.Message = aSubject;
        aStashes.push_back(std::move(aStash));
    }
    return comphelper::containerToSequence(aStashes);
}

void GitRepository::stash(const OUString& rMessage, sal_Bool bIncludeUntracked)
{
    std::vector<OUString> aArguments{ u"stash"_ustr, u"push"_ustr };
    if (bIncludeUntracked)
        aArguments.push_back(u"--include-untracked"_ustr);
    if (!rMessage.trim().isEmpty())
    {
        aArguments.push_back(u"-m"_ustr);
        aArguments.push_back(rMessage.trim());
    }
    Run(u"cannot stash"_ustr, aArguments);
    Refresh(true);
}

void GitRepository::stashPop(sal_Int32 nIndex)
{
    Run(u"cannot pop the stash"_ustr, { u"stash"_ustr, u"pop"_ustr, StashName(nIndex) });
    Refresh(true);
}

void GitRepository::stashApply(sal_Int32 nIndex)
{
    Run(u"cannot apply the stash"_ustr, { u"stash"_ustr, u"apply"_ustr, StashName(nIndex) });
    Refresh(true);
}

void GitRepository::stashDrop(sal_Int32 nIndex)
{
    Run(u"cannot drop the stash"_ustr, { u"stash"_ustr, u"drop"_ustr, StashName(nIndex) });
    Refresh(true);
}

class GitProvider final : public cppu::WeakImplHelper<XVcsProvider>
{
public:
    explicit GitProvider(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    OUString SAL_CALL getKind() override { return u"git"_ustr; }

    OUString SAL_CALL findRoot(const OUString& rUrl) override
    {
        // .git is a directory, or a file that points elsewhere (worktree, submodule)
        for (OUString aDir = rUrl; aDir.getLength() > 8 /* file:/// */; aDir = DirectoryOf(aDir))
        {
            if (Exists(aDir + "/.git"))
                return aDir;
            if (aDir == DirectoryOf(aDir))
                break;
        }
        return OUString();
    }

    uno::Reference<XRepository> SAL_CALL openRepository(const OUString& rRootUrl) override
    {
        rtl::Reference<GitRepository> xRepository(new GitRepository(m_xContext, rRootUrl));
        OUString aReason;
        if (!xRepository->Open(aReason))
            throw VcsException("cannot use git here: " + aReason, getXWeak(), OUString());
        return uno::Reference<XRepository>(xRepository);
    }

private:
    uno::Reference<uno::XComponentContext> m_xContext;
};

class GitPlugin final : public cppu::WeakImplHelper<core::XPlugin, lang::XServiceInfo>
{
public:
    explicit GitPlugin(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    void SAL_CALL activate() override
    {
        theVcsManager::get(m_xContext)->registerProvider(new GitProvider(m_xContext));
    }

    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.GitPlugin"_ustr;
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
com_sun_star_comp_code_GitPlugin_get_implementation(uno::XComponentContext* pContext,
                                                     uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::GitPlugin(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
