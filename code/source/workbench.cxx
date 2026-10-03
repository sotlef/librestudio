/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "workbench.hxx"

#include <codeids.hxx>

#include <com/sun/star/code/core/XBackgroundJob.hpp>
#include <com/sun/star/code/core/XJobRegistry.hpp>
#include <com/sun/star/code/core/theJobRegistry.hpp>
#include <sal/log.hxx>

#include <com/sun/star/code/frontend/Notification.hpp>
#include <com/sun/star/code/frontend/SaveChoice.hpp>
#include <com/sun/star/ui/dialogs/ExecutableDialogResults.hpp>
#include <com/sun/star/ui/dialogs/FilePicker.hpp>
#include <com/sun/star/ui/dialogs/TemplateDescription.hpp>
#include <com/sun/star/ui/dialogs/XFilePicker3.hpp>
#include <com/sun/star/ui/dialogs/XFolderPicker2.hpp>
#include <sfx2/bindings.hxx>
#include <sfx2/filedlghelper.hxx>
#include <sfx2/viewfrm.hxx>
#include <vcl/svapp.hxx>
#include <vcl/vclenum.hxx>
#include <vcl/weld/MessageDialog.hxx>
#include <vcl/weld/Window.hxx>

using namespace css;

namespace cf = css::code::frontend;

namespace code
{
namespace
{
Workbench* s_pInstance = nullptr;

// The window dialogs are parented to: the current Code frame, if there is one.
weld::Window* DialogParent()
{
    SfxViewFrame* pFrame = SfxViewFrame::Current();
    return pFrame ? pFrame->GetFrameWeld() : nullptr;
}
}

Workbench::Workbench(uno::Reference<uno::XComponentContext> xContext)
    : m_xContext(std::move(xContext))
    , m_aStatusTimer("code::Workbench status message")
{
    m_aStatusTimer.SetInvokeHandler(LINK(this, Workbench, StatusTimeoutHdl));
    s_pInstance = this;

    m_xJobNotifier = std::make_shared<UiNotifier>([this] { InvalidateStatus(); });
    m_xJobListener = new ServiceListener(m_xJobNotifier);
    try
    {
        css::code::core::theJobRegistry::get(m_xContext)->addJobListener(m_xJobListener);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "the status bar cannot follow background jobs: " << rException.Message);
    }
}

Workbench::~Workbench()
{
    m_xJobNotifier->Detach();
    try
    {
        css::code::core::theJobRegistry::get(m_xContext)->removeJobListener(m_xJobListener);
    }
    catch (const uno::Exception&)
    {
    }

    SolarMutexGuard aGuard;
    m_aStatusTimer.Stop();
    if (s_pInstance == this)
        s_pInstance = nullptr;
}

Workbench* Workbench::Instance() { return s_pInstance; }

void Workbench::SetEditorSurface(EditorSurface* pSurface)
{
    SolarMutexGuard aGuard;
    m_pEditorSurface = pSurface;
}

namespace
{
// "Building DemoProject (40%)"
OUString DescribeJob(const uno::Reference<css::code::core::XBackgroundJob>& xJob)
{
    OUString aText = xJob->getTitle();
    const sal_Int16 nProgress = xJob->getProgress();
    if (nProgress >= 0)
        aText += " (" + OUString::number(nProgress) + "%)";
    return aText;
}
}

bool Workbench::SaveCurrentDocument()
{
    SolarMutexGuard aGuard;
    return m_pEditorSurface && m_pEditorSurface->SaveCurrentDocument();
}

OUString Workbench::GetStatusText() const
{
    // An explicit message wins; otherwise what is running; otherwise "Ready".
    if (!m_aStatusText.isEmpty())
        return m_aStatusText;
    try
    {
        const auto aJobs = css::code::core::theJobRegistry::get(m_xContext)->getActiveJobs();
        if (aJobs.hasElements())
            return DescribeJob(aJobs[0]) + (aJobs.getLength() > 1 ? " ..." : u""_ustr);
    }
    catch (const uno::Exception&)
    {
    }
    return u"Ready"_ustr;
}

OUString Workbench::GetJobsText() const
{
    OUString aText;
    try
    {
        for (const auto& xJob : css::code::core::theJobRegistry::get(m_xContext)->getActiveJobs())
            aText += (aText.isEmpty() ? u""_ustr : u"\n"_ustr) + DescribeJob(xJob);
    }
    catch (const uno::Exception&)
    {
    }
    return aText.isEmpty() ? u"No background tasks running."_ustr : aText;
}

void Workbench::InvalidateStatus()
{
    if (SfxViewFrame* pFrame = SfxViewFrame::Current())
    {
        pFrame->GetBindings().Invalidate(SID_CODE_STATUS_READY);
        pFrame->GetBindings().Invalidate(SID_CODE_STATUS_TASKS);
    }
}

uno::Reference<cf::XStatusBar> Workbench::getStatusBar() { return this; }
uno::Reference<cf::XNotificationSink> Workbench::getNotificationSink() { return this; }
uno::Reference<cf::XEditorHost> Workbench::getEditorHost() { return this; }
uno::Reference<cf::XDialogs> Workbench::getDialogs() { return this; }

// ---- status bar -----------------------------------------------------------

void Workbench::SetStatusText(const OUString& rText, sal_Int32 nTimeoutMs)
{
    m_aStatusText = rText;
    m_aStatusTimer.Stop();
    if (!rText.isEmpty() && nTimeoutMs > 0)
    {
        m_aStatusTimer.SetTimeout(nTimeoutMs);
        m_aStatusTimer.Start();
    }

    // The status bar item is fed by Shell::GetState(); tell it to ask again.
    if (SfxViewFrame* pFrame = SfxViewFrame::Current())
        pFrame->GetBindings().Invalidate(SID_CODE_STATUS_READY);
}

IMPL_LINK_NOARG(Workbench, StatusTimeoutHdl, Timer*, void)
{
    m_aNotificationId.clear();
    m_aNotificationCategory.clear();
    SetStatusText(OUString(), 0);
}

void Workbench::showMessage(const OUString& rMessage, sal_Int32 nTimeoutMs)
{
    SolarMutexGuard aGuard;
    // A plain message replaces whatever notification was showing.
    m_aNotificationId.clear();
    m_aNotificationCategory.clear();
    SetStatusText(rMessage, nTimeoutMs);
}

// ---- notifications --------------------------------------------------------

OUString Workbench::postNotification(const cf::Notification& rNotification)
{
    SolarMutexGuard aGuard;
    m_aNotificationId = "notification-" + OUString::number(m_nNextNotification++);
    m_aNotificationCategory = rNotification.Category;

    const OUString aText = rNotification.Title.isEmpty()
                               ? rNotification.Message
                               : rNotification.Title + ": " + rNotification.Message;
    // An error stays until something replaces it; milder ones fade.
    SetStatusText(aText, rNotification.Level >= css::code::core::Severity_ERROR ? 0 : 8000);
    return m_aNotificationId;
}

void Workbench::dismissNotification(const OUString& rIdentifier)
{
    SolarMutexGuard aGuard;
    if (m_aNotificationId != rIdentifier)
        return;
    m_aNotificationId.clear();
    m_aNotificationCategory.clear();
    SetStatusText(OUString(), 0);
}

void Workbench::clearCategory(const OUString& rCategory)
{
    SolarMutexGuard aGuard;
    if (m_aNotificationId.isEmpty() || m_aNotificationCategory != rCategory)
        return;
    m_aNotificationId.clear();
    m_aNotificationCategory.clear();
    SetStatusText(OUString(), 0);
}

// ---- editor ---------------------------------------------------------------

void Workbench::openDocument(const OUString& rUrl)
{
    SolarMutexGuard aGuard;
    if (m_pEditorSurface)
        m_pEditorSurface->ShowDocument(rUrl, nullptr);
}

void Workbench::revealPosition(const OUString& rUrl, const css::code::core::Position& rPosition)
{
    SolarMutexGuard aGuard;
    if (m_pEditorSurface)
        m_pEditorSurface->ShowDocument(rUrl, &rPosition);
}

// ---- dialogs --------------------------------------------------------------

cf::SaveChoice Workbench::askSaveChanges(const OUString& rDocumentName)
{
    SolarMutexGuard aGuard;
    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(
        DialogParent(), VclMessageType::Question, VclButtonsType::NONE,
        "Do you want to save the changes to " + rDocumentName + "?"));
    xBox->add_button(u"Save"_ustr, RET_YES);
    xBox->add_button(u"Don't Save"_ustr, RET_NO);
    xBox->add_button(u"Cancel"_ustr, RET_CANCEL);
    xBox->set_default_response(RET_YES);

    switch (xBox->run())
    {
        case RET_YES:
            return cf::SaveChoice_SAVE;
        case RET_NO:
            return cf::SaveChoice_DISCARD;
        default: // RET_CANCEL, and closing the dialog with the window's own button
            return cf::SaveChoice_CANCEL;
    }
}

OUString Workbench::pickFolder(const OUString& rTitle)
{
    SolarMutexGuard aGuard;
    uno::Reference<ui::dialogs::XFolderPicker2> xPicker
        = sfx2::createFolderPicker(m_xContext, DialogParent());
    xPicker->setTitle(rTitle);
    if (xPicker->execute() != ui::dialogs::ExecutableDialogResults::OK)
        return OUString();
    return xPicker->getDirectory();
}

OUString Workbench::pickFile(const OUString& rTitle)
{
    SolarMutexGuard aGuard;
    uno::Reference<ui::dialogs::XFilePicker3> xPicker = ui::dialogs::FilePicker::createWithMode(
        m_xContext, ui::dialogs::TemplateDescription::FILEOPEN_SIMPLE);
    xPicker->setTitle(rTitle);
    if (xPicker->execute() != ui::dialogs::ExecutableDialogResults::OK)
        return OUString();
    const uno::Sequence<OUString> aFiles = xPicker->getSelectedFiles();
    return aFiles.hasElements() ? aFiles[0] : OUString();
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
