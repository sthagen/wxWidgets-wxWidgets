Installing wxWidgets for Windows       {#plat_msw_install}
--------------------------------

This is wxWidgets for Microsoft Windows 7 or later (up to 11)
including both 32 bit and 64 bit versions.

[TOC]

Installation                           {#msw_install}
============

If you are using one of the supported compilers, you can use
[pre-built binaries](@ref plat_msw_binaries).

In this case, just uncompress the binaries archive under any directory
and skip to [Building Applications Using wxWidgets](#msw_build_apps) part.

Otherwise, or if you want to build a configuration of the library
different from the default one, you need to build the library from
sources before using it.

If you use CMake, please see @ref overview_cmake for
building wxWidgets using it.

The first step, which you may have already performed, unless you are
reading this file online, is to download the source archive and
uncompress it in any directory. It is strongly advised to avoid using
spaces in the name of this directory, i.e. notably do *not* choose a
location under "C:\Program Files", as this risks creating problems
with makefiles and other command-line tools.

After choosing the directory location, please define WXWIN environment
variable containing the full path to this directory. While this is not
actually required, this makes using the library more convenient and
this environment variable is used in the examples below.


Building wxWidgets                     {#msw_build}
==================

The following sections explain how to compile wxWidgets with each supported
compiler, see the "Building Applications" section about the instructions for
building your application using wxWidgets.

All makefiles and project are located in `build\msw` directory.

Microsoft Visual C++ Compilation       {#msw_build_msvs}
----------------------------------------------------------------

### From the IDE

Ready to use project files are provided for VC++ versions 2015, 2017, 2019 and 2022.

Simply open `wx_vcN.sln` (for N=14, 15, 16 or 17) file,
select the appropriate configuration (Debug or Release, static or DLL)
and build the solution. Notice that when building a DLL configuration,
you may need to perform the build several times because the projects
are not always built in the correct order, and this may result in link
errors. Simply do the build again, up to 3 times, to fix this.

Note that targeting ARM64 requires VC++ 2017 or newer, while ARM64EC and ARM64X
require 2019 or newer and SDK 10.0.22621.0 or newer.

The custom build steps have not yet been tailored to support ARM64X, but it
seems to work well if you build with `Platform=ARM64` first and then
`Platform=ARM64EC` and `BuildAsX=true` (see the
[ARM64X build instructions](https://learn.microsoft.com/en-us/windows/arm/arm64x-build)).


### From the command line

wxWidgets can also be built from the command line using the provided makefiles.

This needs to be done from the "Visual Studio Command Prompt" window, which can
be opened using a shortcut installed to the "Start" menu or the "Start" screen
by MSVS installation.

In this window, change directory to `%%WXWIN%\build\msw` and type

        > msbuild /m /p:Configuration=Debug /p:Platform=x64 wx_vc17.sln

to build wxWidgets in the debug configuration as a static library using MSVS
2022 (MSVC 17) toolset. Use "Release" configuration instead of "Debug" for the
release version build and `wx_vc14.sln`, `wx_vc15.sln` or `wx_vc16.sln` for
MSVS 2015, 2017 or 2019 respectively.

After the build completes, open `%%WXWIN%\samples\samples_vc17.sln` solution
and try building and running the minimal sample to verify that your build is
functional.


### From the command line using nmake (legacy)

Note that using MSBuild is strongly recommended, as it can use multiple
processors for building, resulting in significant speedup, but it is also
possible to use `nmake` with the provided makefiles. For example, use

        > nmake /f makefile.vc

to build wxWidgets in the default debug configuration as a static library. You
can specify `BUILD=release`, `SHARED=1` and `TARGET_CPU=X86` to choose the
release, DLL and 32-bit builds respectively.

Note that `TARGET_CPU=ARM64` is supported while `TARGET_CPU=ARM64EC` is, at
present, not supported here.

See [Make Parameters](#msw_build_make_params) for more information about the
additional parameters that can be specified on the command line.

To verify your build, change the directory to `%%WXWIN%\samples\minimal` and
run the same nmake command (with the same parameters there), this should create
a working minimal wxWidgets sample.

If you need to rebuild, use "clean" target first or "nmake /a".


### Using vcpkg

You can download and install wxWidgets using the [vcpkg](https://github.com/Microsoft/vcpkg)
dependency manager:

    > git clone https://github.com/Microsoft/vcpkg.git
    > cd vcpkg
    > bootstrap-vcpkg.bat
    > vcpkg integrate install
    > vcpkg install wxwidgets
    > vcpkg install wxwidgets:x64-windows

The wxWidgets port in vcpkg is kept up to date by Microsoft team members and community
contributors. If the version is out of date, please [create an issue or pull request]
(https://github.com/Microsoft/vcpkg) on the vcpkg repository.



### Special notes for Visual Studio

For Visual Studio solutions it is possible to customize the build by
creating a `wx_local.props` file in the `build\msw` directory which is used, if it
exists, by the projects. The settings in that file override the default values
for the properties such as wxCfg (corresponding to the CFG makefile variable
described below) or wxVendor (corresponding to VENDOR). The typical way to
make the file is to copy `wx_setup.props` to `wx_local.props` and then edit local.

For example, if you are building wxWidgets libraries using multiple versions
of Visual Studio you could change wxCompilerPrefix to include the toolset:

    -    <wxCompilerPrefix>vc</wxCompilerPrefix>
    +    <wxCompilerPrefix>vc$(PlatformToolsetVersion)</wxCompilerPrefix>

Following that example if you are using Visual Studio 2015 and open
`wx_vc14.sln` it will build using the "vc140" prefix for the build directories
so to allow its build files to coexist with the files produced by the other
MSVC versions.

Keep in mind that by using a separate local props file you ensure that your
changes won't be lost when updating to a future wxWidgets version. But if
`wx_setup.props` is updated in some later commit your `wx_local.props` is not
updated with it. For example the version information in `wx_setup.props` could
change and the information in your `wx_local.props` would be outdated. It is
your responsibility to monitor for such situations.

### Improve debugging for Visual Studio

Debug visualizers which make inspecting various wxWidgets classes easier to view
while debugging are provided in file `%%WXWIN%\misc\msvc\wxWidgets.natvis`.
The visualisers can be either added to a project or installed system-wide.
See the [Visual Studio documentation](https://learn.microsoft.com/en-us/visualstudio/debugger/create-custom-views-of-native-objects)
for more information.


MinGW Compilation               {#msw_build_mingw}
----------------------------------------------------------------

wxWidgets supports several different gcc-based toolchains under Windows,
including:

- [MinGW-w64](http://mingw-w64.sourceforge.net/)
- [TDM-GCC](http://tdm-gcc.tdragon.net/)
- Classic [MinGW](http://www.mingw.org/)

Please retrieve and install the latest version of your preferred
tool chain by following the instructions provided by these packages.

Additionally note that MinGW-w64 can be used as a cross-compiler from Unix
systems, including [WSL](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux).

All of these tool chains can be used either with Unix-like configure+make build
process (preferred) or with the provided makefile.gcc makefiles without using
configure.

### Using configure

This method works in exactly the same way as under Unix systems, and requires a
Unix-like environment to work, i.e. one of MSYS, [MSYS2](https://www.msys2.org/)
or [Cygwin](https://www.cygwin.com/), so the following steps should be done
from MSYS or Cygwin shell prompt:

1. Create a build directory: it is strongly recommended to not
   build the library in the directory containing the sources (`$WXWIN`)
   but to create a separate build directory instead. The build
   directory can be placed anywhere (using the fastest available disk
   may be a good idea), but in this example we create it as a
   subdirectory of the source one:

        $ cd $WXWIN
        $ mkdir build-debug
        $ cd build-debug

2. Run configure passing it any of the options shown by `configure
   --help`. Notice that configure builds shared libraries by default,
   use `--disable-shared` to build static ones. For example:

        $ ../configure --enable-debug

3. Build the library:

        $ make

4. Test the library build by building the minimal sample:

        $ cd samples/minimal
        $ make

5. Optionally install the library in a global location

        $ make install

   Notice that there is not much benefice to installing under Windows
   so this step can usually be omitted.


### Using makefiles from Windows command line

The `makefile.gcc` makefiles are for compilation using MinGW using Windows
command interpreter (`cmd.exe`), they will *not* work if you use Unix
shell, as is the case with MSYS. Follow the instructions for using configure
above instead if you prefer to use Unix shell. The commands shown here must be
executed from a DOS command line window (cmd.exe, *not* Bash sh.exe).

1. Change directory to `%%WXWIN%\build\msw` and type

        > mingw32-make -f makefile.gcc

   to build wxWidgets in the default debug configuration as a static
   library. Add "BUILD=release" and/or "SHARED=1" to build the library
   in release configuration and/or as a shared library instead of the
   default static one, see [Make Parameters](#msw_build_make_params)
   for more details.

   NOTE: For parallel builds, i.e. using `-jN` make option, please run
         the make command first without the `-jN` option and with `setup_h`
         target specified, e.g. `mingw32-make ... setup_h`. Only after that
         run the make command with the same wxWidgets build options but now
         with the `-jN` option and without `setup_h` target, e.g. `mingw32-make -j4 ...`.
         All this is necessary to work around the bug in the makefile.

2. To verify your build, change the directory to `samples\minimal` and
   run the same mingw32-make command (with the same parameters there),
   this should create a working minimal wxWidgets sample.

3. If you need to rebuild, use "clean" target first.





Make Parameters                         {#msw_build_make_params}
----------------------------------------------------------------

NOTE: If you use configure to build the library with MinGW, the
      contents of this section does not apply, just pass the arguments
      to configure directly in this case.

### Library configuration

While it is never necessary to do it, you may want to change some of
the options in the `%%WXWIN%\include\wx\msw\setup.h` file before building
wxWidgets. This file is heavily commented, please read it and enable or disable
the features you would like to compile wxWidgets with[out].

Notice that this file is later copied into a directory under lib for
each of the build configurations which allows to have different
build options for different configurations too if you edit any
configuration-specific file.


### Makefile parameters

When building using makefiles, you can specify many build settings
(unlike when using the project files where you are limited to choosing
just the configuration and platform). This can be done either by
passing the values as arguments when invoking make or by editing
`build\msw\config.$compiler` file where `$compiler` is the same extension
as the makefile you use has (see below). The latter is good for
setting options that never change in your development process (e.g.
`GCC_VERSION` or `VENDOR`). If you want to build several versions of
wxWidgets and use them side by side, the former method is better.
Settings in `config.*` files are shared by all makefiles (including the
samples), but if you pass the options as arguments, you must use the same
arguments you used for the library when building samples!

For example, to build the library in release mode you can either
change the "BUILD" variable definition in `build\msw\config.$compiler`
or use

        > nmake -f makefile.vc BUILD=debug
        > mingw32-make -f makefile.gcc BUILD=debug

depending on the compiler used.

The full list of the build settings follows:

* `BUILD=release`

  Builds release version of the library. It differs from default 'debug' in
  lack of appended 'd' in name of library and uses the release CRT libraries
  instead of debug ones. Notice that even release builds do include debug
  information by default, see `DEBUG_FLAG` for more information about it.

* `SHARED=1`

  Build shared libraries (DLLs). By default, DLLs are not built
  (SHARED=0).

* `WXUNIV=1`

  Build wxUniversal instead of native wxMSW

* `MONOLITHIC=1`

  wxWidgets is by default built as several smaller libraries ("multilib build")
  instead of single big one as used to be the case in its much older versions.
  You can still build single library ("monolithic build") by setting MONOLITHIC variable to 1.

* `USE_GUI=0`

  Disable building GUI parts of the library, build only wxBase components used
  by console applications. Note that if you leave `USE_GUI=1` then both wxBase
  and GUI libraries are built.

* `USE_$LIBRARY=0`

  Do not build the corresponding library (all libraries are built by
  default). Library which can be disabled in this way are: AUI, HTML,
  MEDIA, GL (the option name is `USE_OPENGL` for this one), PROPGRID,
  QA, RIBBON, RICHTEXT, STC, WEBVIEW, XRC.

* `RUNTIME_LIBS=static`

  (VC++ only.) Links static version of C and C++ runtime libraries into the
  executable, so that the program does not depend on DLLs provided with the
  compiler.

  Caution: Do not use static runtime libraries when building DLL (SHARED=1)!

* `DEBUG_FLAG=0`
* `DEBUG_FLAG=1`
* `DEBUG_FLAG=2`

  Specifies the level of debug support in wxWidgets. Notice that
  this is independent from both BUILD and `DEBUG_INFO` options. By default
  always set to 1 meaning that debug support is enabled: asserts are compiled
  into the code (they are inactive by default in release builds of the
  application but can be enabled), wxLogDebug() and wxLogTrace() are available
  and `__WXDEBUG__` is defined. Setting it to 0 completely disables all
  debugging code in wxWidgets while setting it to 2 enables even the time
  consuming assertions and checks which are deemed to be unsuitable for
  production environment.

* `DEBUG_INFO=0`
* `DEBUG_INFO=1`

  This option affects whether debugging information is generated. If
  omitted or set to 'default' its value is determined the value of
  the BUILD option.

* `DEBUG_RUNTIME_LIBS=0`
* `DEBUG_RUNTIME_LIBS=1`

  (VC++ only.) If set to 1, msvcrtd.dll is used, if to 0, msvcrt.dll
  is used. By default msvcrtd.dll is used only if the executable
  contains debug info and msvcrt.dll if it doesn't. It is sometimes
  desirable to build with debug info and still link against msvcrt.dll
  (e.g. when you want to ship the app to customers and still have
  usable .pdb files with debug information) and this setting makes it
  possible.

* `TARGET_CPU=X64|ARM|ARM64|IA64`

  (VC++ only.) Set this variable to build for x86_64 systems. If unset, x86
  build is performed.

* `VENDOR=<your company name>`

  Set this to a short string identifying your company if you are planning to
  distribute wxWidgets DLLs with your application. Default value is 'custom'.
  This string is included as part of DLL name. wxWidgets DLLs contain compiler
  name, version information and vendor name in them. For example
  `wxmsw311u_core_vc_custom.dll` is one of DLLs build using Visual C++ with
  default settings. If you set VENDOR=mycorp, the name will change to
  `wxmsw311u_core_vc_mycorp.dll.`

* `CFG=<configuration name>`

  Sets configuration name so that you can have multiple wxWidgets builds with
  different setup.h settings coexisting in same tree. The value of
  this option is appended to the build directories names. This is
  useful for building the library in some non-default configuration,
  e.g. you could change `wxUSE_STD_CONTAINERS` to 0 in `%%WXWIN%\include\wx\msw\setup.h` and
  then build with `CFG=-nonstd`. Alternatively, you could build with e.g.
  `RUNTIME_LIBS=static CFG=-mt` when using MSVC.

* `COMPILER_PREFIX=<string>`

  If you build with multiple versions of the same compiler, you can put
  their outputs into directories like `vc14_lib`, `vc15_lib` etc. instead of
  `vc_lib` by setting this variable to e.g. `vc15`. This is merely a
  convenience variable, you can achieve the same effect (but different
  directory names) with the CFG option.

* CFLAGS
* CXXFLAGS
* CPPFLAGS
* LDFLAGS

  Additional flags to be used with C compiler, C++ compiler, C
  preprocessor (used for both C and C++ compilation) and linker,
  respectively.



Building Applications Using wxWidgets  {#msw_build_apps}
=====================================

Note: If you want to use CMake for building your project, please see
@ref overview_cmake.

Using Microsoft Visual C++ IDE         {#msw_build_apps_msvc}
------------------------------

If you use MSVS for building your project, simply add
`wxwidgets.props` property sheet to (all) your project(s) using wxWidgets
by using "View|Property Manager" menu item to open the property manager
window and then selecting "Add Existing Property Sheet..." from the context
menu in this window.

If you've created a new empty project (i.e. chose "Empty Project" in the
"Create a new project" window shown by MSVS rather than "Windows Desktop"),
you need to change "Linker|System|SubSystem" in the project properties to
"Windows", from the default "Console". You don't need to do anything else.

Using Other Compilers or Command Line  {#msw_build_apps_other}
-------------------------------------

We suppose that wxWidgets sources are under the directory `$WXWIN` (notice that
different tool chains refer to environment variables such as WXWIN in
different ways, e.g. MSVC users should use `$``(WXWIN)` instead of just
`$WXWIN`). And we will use `<wx-lib-dir>` as a shortcut for the subdirectory of
`$WXWIN\lib` which is composed from several parts separated by underscore:
first, a compiler-specific prefix (e.g. "vc" for MSVC, "gcc" for g++ or the
value of `COMPILER_PREFIX` if you set it explicitly), then "x64" if building in
64 bits using MSVC (but not any other compilers) and finally either "lib" or
"dll" depending on whether static or dynamic wx libraries are being used.

For example, WXWIN could be "c:\wxWidgets\3.4.5" and `<wx-lib-dir>` could be
`c:\wxWidgets\3.4.5\lib\vc_x64_lib` for 64-bit static libraries built with
MSVC but for shared libraries built with gcc it would be
`c:\wxWidgets\3.4.5\lib\gcc_dll` instead.

Here is what you need to do:

* Add `$WXWIN\include` to the
  - compiler
  - resource compiler
  include paths.
* Append `<wx-lib-dir>\mswu[d]` to the include paths, where "d" should
  be used for debug builds only.
  When using MSVC, there is a simpler alternative which allows to use the
  same compiler options for debug and release builds: just prepend
  `$WXWIN\include\msvc` to the include paths **instead** of the paths above.
* Define the following symbols for the preprocessor:
  - `__WXMSW__` to ensure you use the correct wxWidgets port.
  - `NDEBUG` if you want to build in release mode, i.e. disable asserts.
  - `WXUSINGDLL` if you are using DLL build of wxWidgets.
* Add `<wx-lib-dir>` directory described above to the libraries path.

When using MSVC, using `include\msvc` in the compiler include path has another
advantage: the header found in this directory ensures that all the required
libraries are linked automatically using `#pragma comment(lib)` feature of this
compiler. With the other compilers, or if you don't use `include\msvc` with
MSVC, you also need to:

* Add the list of libraries to link with to the linker input. The exact list
  depends on which libraries you use and whether you built wxWidgets in
  monolithic or default multi-lib mode and basically should include all the
  relevant libraries from the directory above, e.g. `wxmsw34ud_core.lib
  wxbase34ud.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxwebpd.lib wxzlibd.lib
  wxregexud.lib wxexpatd.lib` for a debug build of an application using the
  core library of wxWidgets 3.4 only (all wxWidgets applications use the base
  library).
* When using classes from non-core libraries, e.g. `wxPropertyGrid`, also link
  with the corresponding library, as indicated in the class documentation, i.e.
  `wxmsw34ud_propgrid.lib` in this case.
* When using `wxStyledTextCtrl`, if static (not DLL) wxWidgets libraries are
  used, then, in addition to linking with `wxmsw34ud_stc.lib`, you also need
  to add `wxlexilla[d].lib` and `wxscintilla[d].lib` the list of libraries
  to link with.
* Finally, when using static wxWidgets libraries you must also add all Windows
  libraries that are used by wxWidgets to the linker input. Currently this
  means linking with the following libraries (some of which might be
  unnecessary depending on your build configuration): `advapi32 comctl32
  comdlg32 gdi32 gdiplus imm32 kernel32 msimg32 ole32 oleacc oleaut32 opengl32
  rpcrt4 shell32 shlwapi user32 uuid uxtheme version wininet winmm winspool
  ws2_32`.

For example, to compile your program with gcc using debug wxWidgets DLLs
you would need to use the following options for the compiler (and `windres`
resource compiler):

    -I$WXWIN/include -I$WXWIN/lib/gcc_dll/mswud -D__WXMSW__ -DWXUSINGDLL

and

    -L$WXWIN/lib/gcc_dll

for the linker.

Finally, please notice that the makefiles and project files provided with
wxWidgets samples show which flags should be used when building applications
using wxWidgets and always work, so in case of a problem, e.g. if the
instructions here are out of date, you can always simply copy a makefile or
project file from `$WXWIN\samples\minimal` or some other sample and adapt it to
your application.


Using a Manifest                       {#msw_manifest}
----------------

### For MSVS Users

When using MSVS projects to build your application, the manifest is generated
automatically by default. However this default manifest doesn't mark the
application as being high-DPI aware, which is normally desirable, as otherwise
it would look blurry on high DPI monitors.

If this is not a problem for your application, you don't need to do anything at
all. However if you would like to fix this and make the application DPI-aware,
you need to choose one of the following options:

- Use wxWidgets manifest in addition to the default manifest generated by MSVC
  linker: for this, add `include\wx\msw\wx_dpi_aware_pmv2.manifest` to the
  "Additional manifest files" in the "Manifest Tool | Input and Output" section
  of the project options.

- Use wxWidgets manifest instead of the default manifest generated by MSVC
  linker: for this, turn off "Generate manifest" in the "Linker | Manifest
  File" section of the project options and define `wxUSE_RC_MANIFEST=1` and
  `wxUSE_DPI_AWARE_MANIFEST=2` in your `.rc` file before including
  `wx/msw/wx.rc`, e.g.:

        #define wxUSE_RC_MANIFEST 1
        #define wxUSE_DPI_AWARE_MANIFEST 2
        #include <wx/msw/wx.rc>

- Not use wxWidgets manifest at all but set the "DPI awareness" under
  "Manifest" in the project options to the desired value.


### For All Other Compilers

All Windows applications should use a "manifest", which is a special kind of
Windows resource containing information about the application compatibility,
required, among else, for the application UI to look correctly instead
of looking very outdated and different from other native applications.
Thus, if you see that your application looks like a Windows 95 application, it
is most likely because it doesn't have a manifest.

To fix this, you need to include `include\wx\msw\wx.rc` from your resource
file. You would typically also define `wxUSE_DPI_AWARE_MANIFEST=2` before doing
this to enable high DPI support, so your resource file should contain at least
the following lines:

        #define wxUSE_DPI_AWARE_MANIFEST 2
        #include <wx/msw/wx.rc>

Please note that if you already have a manifest in your application, you can
define `wxUSE_NO_MANIFEST` before including `wx/msw/wx.rc` to prevent using the
wxWidgets-provided manifest.


### Further Information

See [MSW Platform-Specific Build Issues](@ref high_dpi_platform_msw) section of
the high DPI overview for more information about high DPI support in wxWidgets.

See [MSDN manifest documentation][msw-manifest] for more information about
application manifests in general.

[msw-manifest]: https://docs.microsoft.com/en-us/windows/win32/sbscs/application-manifests


Advanced Library Configurations        {#msw_advanced}
-------------------------------
Build instructions to less common library configurations using different UI
backends are available here.

@subpage plat_msw_msys2 "Building with Win32 MSys2 backend"

@subpage plat_msw_msys2_gtk "Building with Win32 MSys2 GDK backend"

@subpage plat_msw_gtk "Building wxGTK port with Win32 GDK backend"

@subpage plat_msw_msys2_qt "Building with Win32 MSys2 Qt backend"
