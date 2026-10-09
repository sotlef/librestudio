/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

// The startup target (the Visual Studio notion: the one target Start runs)
// and its stored launch properties.  Header-only, because the consumers sit
// in different modules: the toolbar and the view shell in the executable,
// the panels in codesidebar, the writer of the properties in cmakeproj.

#include <com/sun/star/code/debug/XLaunchable.hpp>
#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectTarget.hpp>
#include <com/sun/star/code/project/XProjectTargets.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <rtl/ustring.hxx>

namespace code::startup
{
// The launch properties of one target, kept as properties of the project's
// current configuration (Debug and Release run with different paths and
// arguments).  Only these builders spell the key strings.
inline OUString CommandKey(std::u16string_view rTarget)
{
    return OUString::Concat("Target.") + rTarget + ".Debug.Command";
}
inline OUString ArgumentsKey(std::u16string_view rTarget)
{
    return OUString::Concat("Target.") + rTarget + ".Debug.Arguments";
}
inline OUString WorkingDirectoryKey(std::u16string_view rTarget)
{
    return OUString::Concat("Target.") + rTarget + ".Debug.WorkingDirectory";
}
inline OUString EnvironmentKey(std::u16string_view rTarget)
{
    return OUString::Concat("Target.") + rTarget + ".Debug.Environment";
}

// The project-level property the manager stores the choice under.
inline constexpr OUString PROP_STARTUP_TARGET = u"StartupTarget"_ustr;

inline bool IsLaunchable(const css::uno::Reference<css::code::project::XProjectTarget>& xTarget)
{
    css::uno::Reference<css::code::debug::XLaunchable> xLaunchable(xTarget, css::uno::UNO_QUERY);
    return xLaunchable.is() && xLaunchable->getLaunchable();
}

/** The target Start runs: the stored choice when it still names a launchable
    target of the project, otherwise the first launchable one (so a project
    with a single executable needs no choosing), otherwise null. */
inline css::uno::Reference<css::code::project::XProjectTarget>
ResolveStartupTarget(const css::uno::Reference<css::uno::XComponentContext>& xContext,
                     const css::uno::Reference<css::code::project::XProject>& xProject)
{
    if (!xProject.is())
        return {};
    css::uno::Reference<css::code::project::XProjectTargets> xTargets(xProject,
                                                                      css::uno::UNO_QUERY);
    if (!xTargets.is())
        return {};

    OUString aStored;
    try
    {
        aStored = css::code::project::theProjectManager::get(xContext)->getStartupTarget(
            xProject->getUrl());
    }
    catch (const css::uno::Exception&)
    {
    }

    css::uno::Reference<css::code::project::XProjectTarget> xFirst;
    for (const auto& rTarget : xTargets->getTargets())
    {
        if (!IsLaunchable(rTarget))
            continue;
        if (rTarget->getName() == aStored)
            return rTarget;
        if (!xFirst.is())
            xFirst = rTarget;
    }
    return xFirst;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
