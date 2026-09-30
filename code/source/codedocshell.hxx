/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/docfac.hxx>
#include <sfx2/objsh.hxx>

#include <ifaceids.hxx>

// A document-less SfxObjectShell: "Code" has no persisted document model
// yet, this exists purely so the real sfx2/framework machinery (SfxViewFrame,
// LayoutManager, declarative uiconfig menus) has something to hang a
// SfxViewFrame off of. Modelled on basctl::DocShell (basctl/source/basicide/
// basdoc.hxx), but registered as a normal creatable factory (private:factory/
// scode) rather than basctl's own hand-rolled singleton-tool path.
namespace code
{

class DocShell final : public SfxObjectShell
{
protected:
    virtual void Draw(OutputDevice*, const JobSetup& rSetup, sal_uInt16 nAspect,
        bool bOutputForScreen) override;
    virtual void FillClass(SvGlobalName* pClassName, SotClipboardFormatId* pFormat,
        OUString* pFullTypeName, sal_Int32 nVersion, bool bTemplate = false) const override;

public:
    SFX_DECL_OBJECTFACTORY();
    SFX_DECL_INTERFACE(CODE_INTERFACE_DOCSH)

private:
    static void InitInterface_Impl();

public:
    DocShell();
    virtual ~DocShell() override;
};

} // namespace code

// The .sdi compiler (svidl) cannot parse C++ namespaces.
typedef code::DocShell code_DocShell;

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
