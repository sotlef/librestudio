/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <sfx2/app.hxx>
#include <sfx2/objface.hxx>
#include <sfx2/sfxbasemodel.hxx>
#include <sfx2/sfxmodelfactory.hxx>
#include <sfx2/sfxsids.hrc>
#include <tools/globname.hxx>

#include "codedocshell.hxx"

// svidl only emits an extern for an SfxType0 singleton when some slot in
// the reachable .sdi subset uses that item type as an argument directly;
// our tiny 2-slot code.sdi doesn't, but the generated codeslots.hxx below
// still references this one as an array terminator sentinel (the real
// symbol lives in libsfx already -- this is just the missing declaration).
extern SFX2_DLLPUBLIC constinit const SfxType0 aSfxVoidItem_Impl;

#define ShellClass_code_DocShell
#include <codeslots.hxx>

namespace code
{

SFX_IMPL_OBJECTFACTORY(DocShell, SvGlobalName(), u"scode"_ustr)

// SUPERCLASS variant: no slots of DocShell's own, chains straight to
// SfxObjectShell's -- same choice basctl::DocShell makes (all the real
// slots live on the view shell, see codeviewshell.cxx).
SFX_IMPL_SUPERCLASS_INTERFACE(code_DocShell, SfxObjectShell)

void code_DocShell::InitInterface_Impl()
{
    // Without this, SfxWorkWindow::UpdateStatusBar_Impl() (sfx2/source/appl/
    // workwin.cxx) never dispatches "private:resource/statusbar/statusbar"
    // to the LayoutManager at all -- m_eStatusBarId stays StatusBarId::None
    // forever. basctl registers its own BasicIdeStatusBar id the same way,
    // from its DocShell::InitInterface_Impl(); "Code" has no status-bar
    // content of its own yet, so the generic id (just a resource-presence
    // switch here, not a content template) is enough.
    GetStaticInterface()->RegisterStatusBar(StatusBarId::GenericStatusBar);
}

DocShell::DocShell()
    : SfxObjectShell(SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY)
{
    SetPool(&SfxGetpApp()->GetPool());
    // Plain SfxBaseModel, not a custom subclass: "Code" has no persistence
    // and no document-specific UNO surface to add yet, so the generic model
    // (used by plenty of simple SfxObjectShell subclasses upstream) is
    // enough -- unlike basctl's own SIDEModel, which exists only to stub
    // out store()/storeAsURL()/storeToURL() as errors.
    SetBaseModel(new SfxBaseModel(this));
}

DocShell::~DocShell()
{
}

void DocShell::FillClass(SvGlobalName*, SotClipboardFormatId*, OUString*, sal_Int32, bool) const
{
}

void DocShell::Draw(OutputDevice*, const JobSetup&, sal_uInt16, bool)
{
}

} // namespace code

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
