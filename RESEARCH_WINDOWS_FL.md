# Dependable Windows x64 VST3 Path for FL Studio

**Scope.** This report describes a maintainable release path for a native Windows x64 VST3 built with Visual Studio/MSVC, CMake, and JUCE, then installed and validated in FL Studio. The recommendations distinguish the VST3 package contract from host-specific behavior. They use Steinberg’s VST 3 documentation as the format authority, JUCE’s CMake documentation for project generation, Microsoft documentation for the MSVC/CMake/signing/runtime toolchain, and Image-Line’s FL Studio manual for installation and scanning.

## Executive conclusion

The dependable path is to build a **Release x64** target with a pinned Visual Studio/MSVC and JUCE version, produce a **bundle-style `.vst3` directory rather than a legacy flat `.vst3` DLL**, validate that bundle with Steinberg’s validator, package the complete directory without changing its internal names, and install it to the standard machine-wide VST3 directory:

```text
C:\Program Files\Common Files\VST3\<ProductName>.vst3\
```

For a per-user development install, use:

```text
%LOCALAPPDATA%\Programs\Common\VST3\<ProductName>.vst3\
```

These are not arbitrary FL Studio search folders. Steinberg defines the Windows VST3 locations, and Image-Line says that VST3 plug-ins must be installed to the exact default locations to be found during a scan. A signed installer may offer a machine-wide install, but it should not encourage users to place a VST3 in an arbitrary custom directory.

Use JUCE’s `juce_add_plugin(... FORMATS VST3 ...)` and give it stable manufacturer/plugin identifiers. For an intentionally local development copy, `COPY_PLUGIN_AFTER_BUILD` and `VST3_COPY_DIR` are useful; for a release, an explicit staging directory and installer are safer because they make the artifact auditable and avoid writing into protected system folders during the build.

A release should pass **three separate gates**:

1. **Artifact gate:** the bundle has the expected `Contents/Resources` and `Contents/x86_64-win` layout, the binary is a 64-bit PE DLL, required resources are present, and the bundle and contained binary names agree.
2. **VST3 gate:** Steinberg’s validator and, where practical, the VST3 test host can load and exercise the plug-in.
3. **Host gate:** a clean Windows machine with FL Studio can discover, verify, instantiate, edit, process audio, save/reopen a project, and remove/upgrade the plug-in without stale copies masking the result.

## 1. Toolchain baseline

### Recommended versions and architecture

Use a supported Visual Studio release with the **Desktop development with C++** workload, the MSVC toolset, Windows SDK, CMake, and (for validation) the Steinberg VST3 SDK utilities. Steinberg’s current SDK system-requirements page lists Windows x64 support with MSVC 2022; the same page also lists other Windows architectures and toolsets, but this report targets native x64 because that is the normal FL Studio Windows deployment target. Pin the Visual Studio image and SDK/toolset in CI instead of relying on whichever version happens to be installed on a developer machine. [1] [2]

CMake’s Visual Studio generator is a straightforward baseline because it selects the MSVC environment through the IDE/generator. A reproducible command-line configure/build is:

```powershell
cmake -S . -B build\vs2022-x64 `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_CONFIGURATION_TYPES="Debug;Release"
cmake --build build\vs2022-x64 --config Release --parallel
```

Do not accidentally generate Win32. `-A x64` is the important target-architecture choice. For Ninja, Microsoft says that the MSVC environment must be initialized before CMake is invoked, commonly by calling `vcvarsall.bat x64`; Visual Studio generators do not require that separate step because the IDE supplies the environment. [3]

Prefer a checked-in `CMakePresets.json` with a named `windows-x64-release` configure/build preset. Microsoft documents presets as the shared mechanism for Visual Studio, command-line builds, and CI. Keep machine-specific paths, certificate references, and local install destinations in `CMakeUserPresets.json` or CI environment variables rather than committing them. [4]

### JUCE CMake target

A minimal release-oriented target looks like this:

```cmake
cmake_minimum_required(VERSION 3.22)
project(AfroVocal VERSION 1.2.3 LANGUAGES CXX)

add_subdirectory(JUCE) # or find_package(JUCE CONFIG REQUIRED)

juce_add_plugin(AfroVocal
    COMPANY_NAME "Example Audio"
    PRODUCT_NAME "Afro Vocal"
    VERSION 1.2.3
    PLUGIN_MANUFACTURER_CODE ExAu
    PLUGIN_CODE AfVo
    FORMATS VST3
    VST3_CATEGORIES Fx EQ Dynamics
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(AfroVocal PRIVATE PluginProcessor.cpp PluginEditor.cpp)
target_compile_definitions(AfroVocal PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(AfroVocal PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

# A staging directory is preferable to modifying the developer’s system.
# The exact JUCE property/argument can be selected for the pinned JUCE release.
set_property(DIRECTORY PROPERTY JUCE_VST3_COPY_DIR
    "${CMAKE_BINARY_DIR}/staged-vst3")
```

The names and identifier values above are examples. The manufacturer code and plug-in code must be stable once a product ships. Changing identifiers can cause hosts to see a new plug-in rather than an update, which can break project/plugin identity and automation compatibility. Keep the product name, bundle name, contained binary name, displayed name, version, class IDs, and preset compatibility policy under explicit release control.

JUCE’s official example uses `juce_add_plugin`, `FORMATS ... VST3`, a manufacturer code, a unique plug-in code, and product metadata. The CMake API defines `COPY_PLUGIN_AFTER_BUILD` and `VST3_COPY_DIR`; the former copies/install the plug-in after building, while the latter chooses where that copy goes. JUCE warns that default Windows plug-in locations may be protected and may require elevation. [5] [6]

**Recommended distinction:** use `COPY_PLUGIN_AFTER_BUILD` only for a developer convenience target. For CI and release, build once, stage once, validate the staged tree, then package that exact tree. This avoids an elevated build, avoids silently testing a different copy than the one shipped, and makes it possible to hash and inspect the final payload.

## 2. The Windows VST3 bundle contract

### Required shape for x64

On Windows, Steinberg describes a VST3 as a bundle-like package: a directory with the `.vst3` extension. The legacy single DLL whose filename ends in `.vst3` has been deprecated since VST 3.6.10. The outer bundle directory and the contained binary must have the same name. For a native x64 build, the release payload should resemble:

```text
Afro Vocal.vst3/
├── Contents/
│   ├── Resources/
│   │   ├── moduleinfo.json                 # recommended; generated/validated by SDK tooling
│   │   ├── Snapshots/                      # optional UI snapshots
│   │   ├── Documentation/                  # optional product documentation
│   │   └── <other runtime resources>
│   └── x86_64-win/
│       └── Afro Vocal.vst3                 # 64-bit PE DLL; same basename as the bundle
├── desktop.ini                             # optional Windows Explorer presentation
└── Plugin.ico                               # optional Windows Explorer icon
```

`Contents/x86-win` is for 32-bit x86 and is not a substitute for `Contents/x86_64-win`. Do not put an x64 DLL directly in `Contents`, rename a DLL without updating the bundle structure, or flatten the package merely because a development host happens to accept an older format. The VST3 folder must remain intact when zipped, copied by an installer, or installed manually. [7]

The optional `moduleinfo.json` belongs in `Contents/Resources` in current SDK guidance. It describes the module factory and classes so a host can inspect module information without loading the component. Steinberg supplies `moduleinfotool` to create and validate it, and says SDK-built plug-ins create it automatically. Treat a missing or stale module-info file as a packaging/metadata problem, not as something FL Studio should be expected to repair. [8]

For a multi-platform artifact, Steinberg documents a merged bundle containing separate platform/architecture directories. For this release, ship only the x64 Windows branch unless there is a deliberate reason to include another supported architecture. Fewer branches reduce the risk of an installer copying an incompatible binary or a scan finding an unintended duplicate.

### Names, resources, and dependencies

Keep the outer bundle directory and `Contents/x86_64-win/<binary>` name exactly aligned. Preserve `Contents/Resources` paths and case. If JUCE or application code loads files at runtime, resolve them relative to the plug-in bundle, not the current working directory and not the host executable directory. A plug-in can be loaded by FL Studio from a different process working directory than the one used in a standalone test.

Use Release binaries. A Debug plug-in can depend on debug CRT components or contain assertions that are unavailable on a clean machine. If using `/MD`, ship or install the matching Microsoft Visual C++ Redistributable as an installer prerequisite rather than assuming the developer machine’s runtime is present. Microsoft recommends central deployment with the architecture-matched redistributable package because Windows Update can service it; local DLL deployment and static linking have different servicing trade-offs. For an x64 plug-in, the relevant prerequisite is the x64 Visual C++ Redistributable. [9]

Inspect the final PE binary with Microsoft tooling on a Windows runner. `dumpbin /headers` can confirm the machine field is x64 (`8664`) and `dumpbin /dependents` can expose accidental dependencies. A CI check should also reject a binary that is unexpectedly Debug, Win32, or dependent on a developer-only path.

## 3. Build, stage, and package

### Build sequence

A dependable local/release sequence is:

```powershell
# From a VS Developer PowerShell or Visual Studio generator workflow
cmake --preset windows-x64-release
cmake --build --preset windows-x64-release

# The following paths are illustrative; make them preset variables in the project.
# Stage the complete .vst3 directory, not only the inner DLL.
```

The stage directory should be empty before each release build. Copy the generated `Afro Vocal.vst3` directory into a versioned staging root such as `dist\AfroVocal-1.2.3-win-x64\VST3\`. Do not copy individual files into the install folder and do not merge an old `Resources` directory into a new binary. A clean stage makes missing files and stale resources visible.

After staging, run deterministic checks:

```powershell
$bundle = "dist\AfroVocal-1.2.3-win-x64\VST3\Afro Vocal.vst3"
Test-Path "$bundle\Contents\x86_64-win\Afro Vocal.vst3"
Test-Path "$bundle\Contents\Resources"
Get-ChildItem $bundle -Recurse
```

Add a manifest containing product version, source commit, compiler/toolset, JUCE/VST3 SDK versions, architecture, file hashes, and signing status. The manifest is useful when an FL Studio scan report later shows that a user is loading an older copy.

### Archive and installer options

A ZIP is the simplest developer/nightly distribution. It should contain a top-level product/version directory and the full `*.vst3` directory. A ZIP does not provide an uninstall entry, prerequisite handling, per-machine elevation UX, or a trusted installer signature, so it is usually not the best final consumer distribution.

For a product installer, two practical options are:

| Option | Best fit | Implementation notes |
|---|---|---|
| **WiX Toolset MSI or Burn bundle** | Enterprise/managed deployment, repairs, upgrades, prerequisites, ARP integration | WiX `Candle` compiles source, `Light` links the MSI, `Heat` can harvest files, `Smoke` validates MSI output, and Burn can create a bootstrapper. Author the VST3 directory tree explicitly or generate it from the clean staged artifact. [10] |
| **Inno Setup EXE** | Small/medium consumer installer with a simpler authoring model | Install the complete `.vst3` directory to the exact standard VST3 path. Inno Setup can invoke a configured Sign Tool for Setup and the uninstaller. [11] |

An installer should normally offer **per-machine** installation to `C:\Program Files\Common Files\VST3` and request elevation when needed. A developer-only per-user mode can target `%LOCALAPPDATA%\Programs\Common\VST3`; this is the path Steinberg defines for the user VST3 location. Do not install into `...\Image-Line\FL Studio\Plugins\VST`; Image-Line identifies that as a special folder for legacy native FL Studio plug-ins, not the normal third-party VST3 destination. [12]

If the product has large content, keep content outside the VST3 bundle only when the product has a clear, tested content-location mechanism. The plug-in binary and resources required for startup must travel with the bundle. An installer that writes the binary into the standard path but leaves required resources beside the installer, in `%TEMP%`, or in a developer checkout is incomplete.

### Signing

There are two different signing questions:

* **VST3 format compliance:** Steinberg’s Windows package and validator documentation do not state that a Windows VST3 must carry a Steinberg-issued signature. A plug-in can be unsigned and still be a valid VST3 format artifact. Validate the format and host behavior separately.
* **Windows trust and distribution:** Authenticode signing is recommended for user confidence, SmartScreen reputation, tamper evidence, and reduced “unknown publisher” friction. Sign the Windows PE payload(s) and the installer executable, using a code-signing certificate controlled by the release system. Sign after the final binary and resource files are in place; any post-sign modification invalidates the signature.

Microsoft’s SignTool supports `sign`, `verify`, and timestamp operations. Current SDKs require explicit digest algorithms and Microsoft recommends SHA-256. A typical command is conceptually:

```powershell
signtool sign /fd sha256 /td sha256 /tr https://<RFC3161-timestamp-service> `
  /f $env:SIGNING_CERT_PFX /p $env:SIGNING_CERT_PASSWORD `
  /d "Afro Vocal VST3" "$bundle\Contents\x86_64-win\Afro Vocal.vst3"

signtool verify /pa /all /v "$bundle\Contents\x86_64-win\Afro Vocal.vst3"
```

Use the organization’s approved certificate storage/signing service rather than committing a PFX or password. The exact certificate-selection options depend on whether the key is in a secure signing service, certificate store, or PFX. SignTool’s `/fd` and `/td` settings, certificate trust, timestamp reachability, and post-sign verification belong in CI. [13]

Sign the installer separately after it is built. Inno Setup documents Sign Tool integration for Setup and, when enabled, the uninstaller. WiX packages and Burn bootstrappers can be signed as release artifacts; the MSI/EXE signature is not a replacement for signing contained PE files if the product’s trust policy requires both. [10] [11]

For unsigned local builds, explicitly label the artifact as development-only. Do not let a developer certificate or a self-signed certificate enter the public release channel.

## 4. Installing and scanning in FL Studio

### Install path

Image-Line’s current FL Studio manual states that VST3 plug-ins on Windows must be installed to the exact VST3 locations. For 64-bit Windows, the relevant machine-wide locations are:

```text
C:\Program Files\Common Files\VST3
C:\Program Files\VST3
```

Image-Line also lists the 32-bit locations under `C:\Program Files (x86)`, but those are not the target for a native x64 plug-in. Steinberg’s canonical global path is `C:\Program Files\Common Files\VST3`; its user path is `%LOCALAPPDATA%\Programs\Common\VST3`. Prefer the common-files path and install the complete bundle below it. [12] [14]

Do not add the inner `Contents` directory, the `x86_64-win` directory, or the bundle itself as a custom VST2 search folder. The host scans the VST3 locations recursively. The custom search-path controls in FL Studio are primarily useful for legacy VST1/VST2-style locations; they do not turn an arbitrary VST3 location into a dependable installation contract.

### FL Studio scan workflow

In FL Studio, open **Options > File settings > Manage plugins**. Image-Line recommends enabling **Find installed plugins + Verify plugins**. `Verify plugins` helps FL Studio classify a plug-in as an Effect or Generator and prevents the wrong plug-in type from appearing in menus. Enable **Rescan previously verified plugins** after a build update so that FL Studio does not reuse a prior scan result. Newly scanned plug-ins appear under **Browser > Plugin database > Installed**, in the relevant Effects or Generators area, and in the VST3 folder. [15]

For a reliable manual test:

1. Close FL Studio before replacing a loaded bundle. Windows can keep a loaded DLL in the process, and a host may cache scan information.
2. Remove or rename all older copies of the product from the standard VST3 folders and any development link. Confirm there is one intended bundle.
3. Install the new complete bundle with the installer or copy it to the exact standard path.
4. Start FL Studio and run **Find installed plugins + Verify plugins**, with **Rescan previously verified plugins** enabled for updates.
5. Confirm the plug-in appears once, under the correct Effect/Generator category and VST3 location.
6. Instantiate it in an empty project, open and resize the editor, pass audio/MIDI appropriate to the product, automate representative parameters, save, close, and reopen the project.
7. Repeat on a clean VM or test machine with only the documented prerequisites. This catches missing runtimes, resources, permissions, and accidental reliance on a developer environment.

Image-Line explicitly says not to install third-party VST3s in the FL Studio installation’s legacy `Plugins\VST` directory. If a plug-in is missing, first verify the exact bundle path, architecture, and scan options before adding arbitrary folders. [12] [15]

## 5. Validation strategy

### Steinberg validation

Steinberg describes the validator as a small command-line host that checks VST3 conformity and is suitable for automatic build-server integration. The SDK’s CMake options include `SMTG_RUN_VST_VALIDATOR`, enabled by default for SDK projects, and `SMTG_CREATE_MODULE_INFO`. Use the validator built from the same pinned VST3 SDK family as the plug-in, and make a non-zero validator result fail CI. [1] [16]

The exact validator executable name and command-line options can vary with SDK packaging. In CI, discover the built validator from the SDK build output and call its `--help`/documented invocation rather than hard-coding a path from a developer machine. Validate the **staged final bundle**, not merely an intermediate build output. Also run `moduleinfotool` where available to create/validate `Contents/Resources/moduleinfo.json`.

Use the VST3 test host or inspector as an additional diagnostic layer. Steinberg’s SDK includes a VST3 plug-in test host and inspector; the validator is the automated conformity gate, while a host/inspector is useful for reproducing factory enumeration, editor creation, class metadata, and basic lifecycle issues. [2]

### Binary and package checks

A useful CI package-validation script should assert all of the following:

* The outer path ends in `.vst3` and is a directory.
* `Contents\x86_64-win\<same-name>.vst3` exists and is the only intended x64 plug-in binary.
* `Contents\Resources` exists, and `moduleinfo.json` is present when the build policy requires it.
* The bundle name and inner binary name are identical.
* `dumpbin /headers` reports x64 (`8664`), and the binary is Release rather than Debug.
* `dumpbin /dependents` contains only dependencies that the clean test machine will have or that the installer explicitly supplies.
* No absolute developer paths, `.pdb` files, source files, old binaries, or test assets are accidentally packaged.
* The installer/ZIP contains the same file list and hashes as the validated stage.
* The signed binary and installer pass `signtool verify` when signing is required.

A hash manifest should be calculated after signing, because signing changes the PE file. Verify the packaged artifact again after archive/installer generation; never assume a packaging tool copied every resource successfully.

## 6. CI build and release design

### Suggested pipeline

Use a Windows runner with a pinned Visual Studio image and these stages:

| Stage | Required work | Failure meaning |
|---|---|---|
| **Checkout** | Checkout the repository and JUCE/VST3 SDK at pinned revisions. Initialize submodules if used. | Non-reproducible dependency graph or missing SDK files. |
| **Configure** | Select the checked-in x64 Release preset. Record CMake, compiler, Windows SDK, JUCE, and VST3 SDK versions. | Wrong generator, architecture, or missing dependency. |
| **Build** | Build only the Release VST3 target and any test/validator utilities. | Compiler/linker/code problem. |
| **Stage** | Copy the complete bundle into a clean versioned staging directory. | Packaging or resource-copy problem. |
| **Static checks** | Assert bundle tree, names, architecture, dependencies, metadata, and no stale files. | Artifact is not safe to distribute. |
| **VST3 validation** | Run Steinberg validator and module-info validation against the staged bundle. | VST3 factory/lifecycle/metadata/package issue. |
| **Host smoke** | On a Windows VM/test runner with FL Studio installed, scan with Verify and instantiate the plug-in. | FL-specific path, runtime, UI, or host compatibility issue. |
| **Sign** | Sign inner PE files and installer using protected CI credentials; timestamp with SHA-256. | Release trust or tamper-evidence issue. |
| **Post-sign verification** | Verify signatures, re-check hashes/file list, and test the signed artifact. | Sign/package mutation or certificate problem. |
| **Publish** | Upload ZIP/installer and manifest; retain logs and validator output. | Release traceability problem. |

Microsoft documents the same CMake presets as usable from Visual Studio, the command line, and CI. This lets a developer reproduce the CI configure/build commands without copying IDE-generated settings. On Ninja-based Windows builds, initialize the MSVC environment with the correct architecture before invoking CMake; with a Visual Studio generator, use `-A x64` and let the generator manage the toolchain environment. [3] [4]

Run at least one matrix job for the supported Windows/Visual Studio baseline and one clean-machine host-smoke job. A broader matrix can test multiple Visual Studio toolsets or Windows versions, but the shipped binary should come from one declared release toolchain. Keep signing in a protected release job after all validation jobs; pull requests should build and validate unsigned artifacts without access to the signing key.

Cache CMake/JUCE/SDK downloads only by immutable revision and toolchain key. Do not cache the final staging directory. Build from a clean checkout, and make the validator/test-host step consume the same staged directory that will be archived. Upload the stage, manifest, validator logs, dependency report, and installer as CI artifacts for auditability.

## 7. Common failures and the fastest diagnosis

| Symptom | Likely cause | Fix |
|---|---|---|
| FL Studio does not list the plug-in | Bundle installed outside the exact VST3 location; installed under FL Studio’s legacy `Plugins\VST`; scan cache not refreshed | Put the complete bundle in `Program Files\Common Files\VST3` or the defined user path. Re-run Find installed plugins + Verify plugins and rescan previously verified plug-ins. [12] [15] |
| FL Studio finds a file but cannot load it | Flat legacy file, missing `Contents`, wrong architecture, or outer/inner names differ | Use the bundle layout and same-name rule. Verify `Contents\x86_64-win\<name>.vst3` and x64 PE headers. [7] |
| It appears twice or an old version keeps loading | Old machine-wide copy, per-user copy, symlink, or installer left a prior version | Search all Steinberg-defined locations and development staging/link locations. Remove duplicates before scanning. Check the installed path in the host’s plug-in details/log. |
| Build succeeds but validator fails | Factory/class metadata, UID, lifecycle, bus configuration, parameter contract, or module-info problem | Run the validator on the final staged artifact; inspect the first failing test. Do not treat a successful linker step as VST3 compliance. [16] |
| Build fails during bundle/module-info generation | Missing SDK submodules, incompatible JUCE/VST3 SDK revisions, helper tool failure, or stale CMake cache | Clone/init submodules, pin compatible revisions, delete the build directory, reconfigure, and preserve helper-tool logs. |
| Validator cannot load the binary | Missing MSVC runtime/dependency, wrong bitness, or binary copied without its resource tree | Check x64 headers and `dumpbin /dependents`; install/test with the x64 VC++ Redistributable; validate from the staged bundle. [9] |
| Works on the developer PC but not a clean VM | Debug runtime, local DLL on `PATH`, resource path uses working directory, or unshipped dependency | Use Release, inspect dependencies, install only documented prerequisites, and run from a clean VM. |
| Installation requires admin unexpectedly | Build step writes to protected Program Files path or symlink creation is attempted | Build/stage in the workspace; let the installer elevate. For development, use the per-user VST3 path or configure permissions deliberately. [1] [6] |
| Windows Explorer shows an unattractive bundle folder | Optional `desktop.ini`/`Plugin.ico` missing or attributes not set | Treat this as presentation only; preserve the functional bundle first. Add icon resources if desired following Steinberg’s documented optional structure. [7] |
| Signature verification fails after packaging | Files changed after signing, wrong digest/timestamp options, certificate chain unavailable, or installer was signed but contained PE was not | Sign last, use explicit SHA-256 `/fd` and `/td`, timestamp, and run `signtool verify` on every required signed artifact. [13] |
| A new plug-in appears instead of an update | Manufacturer/plugin codes or class IDs changed; product name or compatibility identifiers changed | Preserve shipped identifiers and maintain a documented migration policy. Treat identifiers as ABI/project compatibility data, not cosmetic strings. |
| UI opens blank or crashes only in FL Studio | Host-specific editor/lifecycle assumption, missing resource, DPI/focus issue, or thread violation | Reproduce in Steinberg test host and FL Studio; test editor creation/destruction, resize, DPI, automation, and project reload. Keep audio-thread code free of allocations/blocking calls. |

## 8. Release checklist

### Source and identity

- [ ] The release version, product name, manufacturer code, plug-in code, class IDs, category, and compatibility policy are recorded.
- [ ] JUCE, VST3 SDK, CMake, Visual Studio/MSVC, and Windows SDK versions are pinned or recorded.
- [ ] Any VST3 SDK/JUCE license and required notices are included in the release materials.
- [ ] The release was built from a clean, identified source commit.

### Build

- [ ] CMake configured with the intended Release preset and native `x64` target (`-A x64` or equivalent).
- [ ] No Win32, ARM, Debug, sanitizer, or developer-only build accidentally entered the release directory.
- [ ] The build uses a defined MSVC runtime policy and the installer/runtime prerequisite policy is documented.
- [ ] The final build log and compiler warnings are retained.

### Bundle and payload

- [ ] The outer artifact is a directory named `<Name>.vst3`, not a legacy flat `.vst3` DLL.
- [ ] `Contents\x86_64-win\<Name>.vst3` exists and outer/inner names match.
- [ ] `Contents\Resources` contains required resources and the expected `moduleinfo.json`.
- [ ] Runtime resources are resolved relative to the bundle and are included in the stage.
- [ ] `dumpbin /headers` confirms x64; `dumpbin /dependents` has no unplanned dependencies.
- [ ] The clean stage contains no stale binary, source, debug, PDB, temporary, or developer-only files.

### Validation

- [ ] Steinberg validator passes against the final staged bundle.
- [ ] `moduleinfotool` or equivalent module-info validation passes.
- [ ] VST3 test host/inspector can enumerate and load the plug-in.
- [ ] A clean Windows test machine can load it with the documented x64 VC++ runtime prerequisite.
- [ ] FL Studio scan uses **Find installed plugins + Verify plugins** and **Rescan previously verified plugins** for updates.
- [ ] FL Studio shows exactly one plug-in in the expected VST3 category.
- [ ] FL Studio smoke test covers editor, resize/DPI, audio/MIDI, automation, preset/state save, project close/reopen, and removal/reinstall.

### Install/package

- [ ] Installer copies the complete `.vst3` directory to `C:\Program Files\Common Files\VST3` for per-machine installs.
- [ ] Optional per-user mode uses `%LOCALAPPDATA%\Programs\Common\VST3`.
- [ ] Installer does not use `Image-Line\FL Studio\Plugins\VST` or an arbitrary custom VST3 folder.
- [ ] Upgrade removes/replaces the old bundle atomically and does not leave duplicate versions in another standard location.
- [ ] Uninstall removes the product bundle without deleting unrelated VST3 products.
- [ ] ZIP/installer file list and hashes match the validated stage.

### Signing and publication

- [ ] Inner Windows PE payloads are signed if the release policy requires signed plug-ins.
- [ ] Installer/bootstrapper and uninstaller are signed.
- [ ] SignTool uses explicit SHA-256 file and timestamp digests and an RFC 3161 timestamp.
- [ ] Every required signature passes `signtool verify` on a clean machine.
- [ ] No signing key, PFX password, or certificate secret is present in source or public CI logs.
- [ ] Release manifest includes version, commit, toolchain, architecture, file hashes, signing status, validator logs, and known prerequisites.
- [ ] Published artifact is the exact artifact that passed post-sign verification.

## References

[1]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Tutorials/Using+cmake+for+building+plug-ins.html "Using cmake for building VST 3 plug-ins"
[2]: https://steinbergmedia.github.io/vst3_dev_portal/pages/What+is+the+VST+3+SDK/Index.html "What is the VST SDK?"
[3]: https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-170 "CMake projects in Visual Studio"
[4]: https://learn.microsoft.com/en-us/cpp/build/cmake-presets-vs?view=msvc-170 "Configure and build with CMake Presets in Visual Studio"
[5]: https://github.com/juce-framework/JUCE/blob/master/examples/CMake/AudioPlugin/CMakeLists.txt "JUCE CMake Audio Plugin example"
[6]: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md "JUCE CMake API"
[7]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Format.html "VST 3 plug-in format structure"
[8]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/VST+Module+Architecture/ModuleInfo-JSON.html "VST 3 moduleinfo.json"
[9]: https://learn.microsoft.com/en-us/cpp/windows/deployment-in-visual-cpp?view=msvc-170 "Deployment in Microsoft C++"
[10]: https://wixtoolset.org/docs/v3/overview/alltools/ "WiX Toolset list of tools"
[11]: https://jrsoftware.org/ishelp/topic_setup_signtool.htm "Inno Setup SignTool directive"
[12]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_externalplugins.htm "FL Studio: Installing Plugins"
[13]: https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool "SignTool"
[14]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html "VST 3 plug-in locations"
[15]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/envsettings_files.htm "FL Studio file settings and plugin manager"
[16]: https://steinbergmedia.github.io/vst3_dev_portal/pages/What+is+the+VST+3+SDK/Validator.html "VST 3 validator command line"
