/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/core/Position.hpp>
#include <com/sun/star/code/project/ProjectConfiguration.hpp>
#include <com/sun/star/code/core/XBackgroundJobListener.hpp>
#include <com/sun/star/code/frontend/XDialogs.hpp>
#include <com/sun/star/code/project/XProjectListener.hpp>
#include <com/sun/star/code/frontend/XEditorHost.hpp>
#include <com/sun/star/code/frontend/XNotificationSink.hpp>
#include <com/sun/star/code/frontend/XStatusBar.hpp>
#include <com/sun/star/code/frontend/XWorkbench.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <cppuhelper/implbase.hxx>
#include "uibridge.hxx"

#include <vcl/timer.hxx>

#include <vector>

namespace code
{
/// The editor area, as the workbench needs it.  Implemented by EditorMDIPanel,
/// which registers itself for as long as it exists.
/// What the shell asks of the editor of the tab that is showing.
enum class EditCommand { Copy, Cut, Paste, Undo, Redo, SelectAll };

class EditorSurface
{
public:
    virtual ~EditorSurface() = default;

    /// Shows the document in a tab, opening it first if it is not open yet, and puts the caret
    /// at *pPosition if one is given.
    virtual void ShowDocument(const OUString& rUrl,
                              const css::code::core::Position* pPosition) = 0;

    /// The same, with the range selected and the editor focused.
    virtual void ShowRange(const OUString& rUrl, const css::code::core::Range& rRange,
                           bool bTakeFocus) = 0;

    /// Replaces the range of the document, which is open in a tab (an edit that can be undone).
    virtual void ReplaceRange(const OUString& rUrl, const css::code::core::Range& rRange,
                              const OUString& rNewText)
        = 0;

    /// Saves the document of the tab that is showing; false if there is none.
    virtual bool SaveCurrentDocument() = 0;

    /// The caret of the tab that is showing; false if there is none.
    virtual bool CurrentCaret(css::code::core::Position& rCaret) = 0;

    /// An editing command on the tab that is showing; false if there is none.
    virtual bool DoEditCommand(EditCommand eCommand) = 0;

    /// The URL of the document of the tab that is showing; empty if there is none.
    virtual OUString CurrentDocumentUrl() = 0;
};

/// The application shell's side of the backend/front-end split: what the
/// com.sun.star.code.* services reach the user through (codeapi's
/// frontend/XWorkbench).  One object implements the whole family, the
/// workbench handing itself out for each of its parts.
///
/// The services call it from any thread; every method takes the solar mutex
/// before it touches VCL.  The shell's own code runs on the main thread and
/// reaches the workbench through Instance().
class Workbench final
    : public cppu::WeakImplHelper<css::code::frontend::XWorkbench,
                                  css::code::frontend::XStatusBar,
                                  css::code::frontend::XNotificationSink,
                                  css::code::frontend::XEditorHost,
                                  css::code::frontend::XDialogs>
{
public:
    explicit Workbench(css::uno::Reference<css::uno::XComponentContext> xContext);
    virtual ~Workbench() override;

    /// the one workbench of the process; null before it is created and after it is gone
    static Workbench* Instance();

    /// see EditorSurface::ReplaceRange; main thread
    void ReplaceRange(const OUString& rUrl, const css::code::core::Range& rRange,
                      const OUString& rNewText);

    /// The editor area registers itself here, and withdraws with nullptr.
    void SetEditorSurface(EditorSurface* pSurface);

    /// File > Save: the current tab's document.  False if no tab is showing.
    bool SaveCurrentDocument();

    /// The URL of the document in the tab that is showing; empty if no tab is.
    OUString CurrentDocumentUrl();

    /// The caret of the tab that is showing; false if no tab is.  Main thread.
    bool CurrentCaret(css::code::core::Position& rCaret);

    /// An editing command on the tab that is showing; false if no tab is.  Main thread.
    bool DoEditCommand(EditCommand eCommand);

    /// What the status bar's text item shows: the current message, else "Ready".
    OUString GetStatusText() const;

    /// How many background jobs run right now, and the popover text that lists them.
    OUString GetJobsText() const;

    // XWorkbench
    css::uno::Reference<css::code::frontend::XStatusBar> SAL_CALL getStatusBar() override;
    css::uno::Reference<css::code::frontend::XNotificationSink>
        SAL_CALL getNotificationSink() override;
    css::uno::Reference<css::code::frontend::XEditorHost> SAL_CALL getEditorHost() override;
    css::uno::Reference<css::code::frontend::XDialogs> SAL_CALL getDialogs() override;

    // XStatusBar
    void SAL_CALL showMessage(const OUString& rMessage, sal_Int32 nTimeoutMs) override;

    // XNotificationSink
    OUString SAL_CALL
    postNotification(const css::code::frontend::Notification& rNotification) override;
    void SAL_CALL dismissNotification(const OUString& rIdentifier) override;
    void SAL_CALL clearCategory(const OUString& rCategory) override;

    // XEditorHost
    void SAL_CALL openDocument(const OUString& rUrl) override;
    void SAL_CALL revealPosition(const OUString& rUrl,
                                 const css::code::core::Position& rPosition) override;
    void SAL_CALL revealRange(const OUString& rUrl, const css::code::core::Range& rRange,
                              sal_Bool bTakeFocus) override;
    OUString SAL_CALL getCurrentDocumentUrl() override { return CurrentDocumentUrl(); }

    // XDialogs
    css::code::frontend::SaveChoice SAL_CALL askSaveChanges(const OUString& rDocumentName) override;
    OUString SAL_CALL pickFolder(const OUString& rTitle) override;
    sal_Bool SAL_CALL configureProject(
        const OUString& rConfiguratorService, const OUString& rProjectUrl,
        css::uno::Sequence<css::code::project::ProjectConfiguration>& rConfigurations) override;
    OUString SAL_CALL pickFile(const OUString& rTitle,
                                const css::uno::Sequence<css::code::frontend::FileFilter>& rFilters) override;

private:
    DECL_LINK(StatusTimeoutHdl, Timer*, void);
    void SetStatusText(const OUString& rText, sal_Int32 nTimeoutMs);

    css::uno::Reference<css::uno::XComponentContext> m_xContext;
    EditorSurface* m_pEditorSurface = nullptr;

    // Tells the status bar to ask again when a background job starts, ends or
    // reports progress (any thread; the refresh itself runs on the main one).
    std::shared_ptr<UiNotifier> m_xJobNotifier;
    css::uno::Reference<css::code::core::XBackgroundJobListener> m_xJobListener;
    void InvalidateStatus();

    // Solution Explorer exists only while some open project has a structure to
    // show: a project of a format that must be configured counts only once it
    // is; one that needs no configuration (an opened folder) always does.  The
    // deck is hidden or shown as projects open, close and change.
    void UpdateSolutionDeck();
    // adds a project that has just been opened to the recent list
    void RememberOpenedProjects();
    std::vector<OUString> m_aKnownProjects;
    std::shared_ptr<UiNotifier> m_xProjectNotifier;
    css::uno::Reference<css::code::project::XProjectListener> m_xProjectListener;

    // Both only touched with the solar mutex held.
    OUString m_aStatusText;
    Timer m_aStatusTimer;
    // The notification currently shown in the status bar -- the shell has no
    // toast area, so a notification is a status message that remembers who
    // posted it (to be withdrawn by id or category).
    OUString m_aNotificationId;
    OUString m_aNotificationCategory;
    sal_Int64 m_nNextNotification = 1;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
