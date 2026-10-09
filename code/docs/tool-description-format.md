# Tool descriptions: format `code.tool/1`

A tool description says what a command (a compiler, a linker, any tool a build
runs) takes on its command line: its options, sorted into categories, with
their types and, for an enumeration, the values it allows.  Code uses it to
show the options of a command in a properties dialog (from the command a
build gives, the description recovers the properties), and later to make
a command from properties.

Files that come with Code: `code/tooldescriptions/` (installed to
`share/code/tooldescriptions`).  Schema: `code.tool-1.xsd` there.  The reader
is `code/source/tooldesc/tooldescription.{hxx,cxx}`.

## Rules of the format

* Names of elements, attributes and `Id`s are PascalCase.
* `Format="code.tool/N"` is the version of the *format*.  N grows only for a
  change an older reader would misread.  A new optional element or attribute
  stays in the same N.  A file with a larger N than the reader knows is
  refused with a message that says so, before anything else of it is read; a
  file without `Format` is refused.
* `Revision` is the version of the *content* of one description (a flag
  was added); it has nothing to do with the format.
* Anything of the form `X<Capital>...` (`XNote`, `XEditor`) is an extension:
  readers skip it.  Any other unknown element or attribute is an error of the
  file, so that a misspelling is not silently ignored.
* Whatever the format of a description says, a reader of `code.tool/1` takes
  the attributes and elements listed here, no others (but extensions).  What
  a later change of the format adds that an older reader would misunderstand
  gets a new N.  Modes and versions below are part of `/1` from the start.
* A description may build on another one, `Extends="gcc"`: its categories and
  properties are added to the base's, or replace the base's by the same `Id`.

## Elements

```xml
<ToolDescription Format="code.tool/1" Id="gcc" Name="GCC / G++" Revision="1"
                 Match="(^|/)(g\+\+|gcc)$" Extends="...">
  <Category Id="Optimization" Name="Optimization">
    <EnumProperty Id="OptimizationLevel" Name="Optimization Level" Default="-O0">
      <EnumValue Switch="-O2" Name="Maximize speed" Description="..."/>
    </EnumProperty>
    <BoolProperty Id="Rtti" Name="RTTI" On="-frtti" Off="-fno-rtti"/>
    <StringListProperty Id="Includes" Name="Include Directories" Prefix="-I" Kind="Path"/>
    <StringProperty Id="Sysroot" Name="Sysroot" Prefix="--sysroot" Joiner="Equals"/>
    <IntProperty Id="Jobs" Name="Jobs" Prefix="-j"/>
  </Category>
</ToolDescription>
```

* `Match`: an ECMAScript regular expression for the path of the executable.
* `Joiner` says how an option and its value are written: `Attached` (`-Ipath`,
  the default), `Separate` (`-I path`), `Equals` (`--sysroot=path`).  When a
  command is read, an `Attached` option is also met as `Separate` and the
  other way round, and the way it was written is kept.
* `Kind="Path"`: the value is a file or directory.

## Modes: one program, several jobs

`gcc` compiles with `-c`, assembles with `-S`, preprocesses with `-E`, and
links when none of them is given.  A description says so with `Mode` elements:

```xml
<Mode Id="Preprocess" Name="Preprocess" Switches="-E"/>
<Mode Id="Compile" Name="Compile" Switches="-c"/>
<Mode Id="Link" Name="Link" Default="true"/>
<Category Id="Linker" Name="Linker" Modes="Link"> ... </Category>
```

* `DetectMode()` tries the modes in the order of the description: the first
  whose `Switches` (blank separated) occurs among the arguments.  None: the
  `Default` one.  A caller that knows better (CMake's File API says whether
  it is a compile group or a link) passes its own mode.
* `Modes` (blank separated Ids) on a `Category` or a property limits it to
  those modes; without it it is for every mode.  `-O`, `-g`, `-flto`,
  `--sysroot` are for all.
* Recognising a switch does **not** depend on the mode.  `-I` in a link
  command is still the include directories, and a dialog can say that it has
  no effect there; it is not thrown among the unknown arguments.

## Versions

`Since`, `Until`, `Deprecated` on a property or an `EnumValue` (dotted
numbers, `Until` is the first version without it, so the range is
`[Since, Until)`).  On `ToolDescription` itself `Since` and `Until` say for
which versions of the tool the description is, so that a tool can have
several files.

* Recognising a switch does **not** depend on the version, so that a command
  of a newer or older compiler is not left in pieces.  `AvailabilityIn()` says
  what a version has (`NotYet`, `Removed`, `Deprecated`); it is for showing
  that and for an editor not to offer what the version lacks.
* The version comes from the build system when it has it (CMake's toolchains
  reply: `compiler.version`), else from the tool itself.

## Defaults that depend on other properties

What a tool does without being told often depends on another option:
`-fstrict-aliasing` is on from `-O2`, `-ftree-vectorize` from `-O3` (and from
`-O2` in GCC 12).  A property says so with `DefaultIf`, tried in order before
the property's plain `Default`:

```xml
<BoolProperty Id="Vectorize" Name="Vectorize Loops" Default="false" On="-ftree-vectorize" Off="-fno-tree-vectorize">
  <DefaultIf Property="OptimizationLevel" Is="-O3 -Ofast" Value="true"/>
  <DefaultIf Property="OptimizationLevel" Is="-O2" Since="12" Value="true"/>
</BoolProperty>
```

* `Property`: the Id of another property of the description (checked when the
  file is read; a property cannot depend on itself).  `Is`: blank separated
  values; a Bool is `true`/`false`, an Enum the switch, otherwise the text.
* The condition looks at what the command gives (the last value wins), and
  where the command gives nothing, at that property's own default: rules
  chain.  A loop of rules stops at a depth of 16.
* `Since`/`Until` limit a rule to versions of the tool, `[Since, Until)`.  When
  the version is not known such a rule is left out.
* `EffectiveDefault()` is the answer; the service puts it in the properties
  the command does not give, marked `IsDefault`.

## Reading and writing a command

* A command is read argument by argument.  An argument that is exactly an
  `EnumValue` `Switch` or a `BoolProperty` `On`/`Off` is that.  Otherwise the
  option with the longest `Prefix` it begins with takes it (`-isystem` before
  `-I`).  What is left is kept as it is, in its place, and shown as "Other".
* The result is a list of items in the order of the command.  Writing the list
  back gives the same arguments: spellings, order, repeats and unknown
  arguments are kept.  `code/qa/tooldesc` checks this.
