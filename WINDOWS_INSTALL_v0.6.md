# AfroVocal Presets v0.6.0 — Windows VST3 installation

## Recommended installation

Download `AfroVocal-Presets-Setup-x64.exe` from the Windows installer artifact and run it. Windows will request administrator permission because VST3 plug-ins are installed for all users. The installer places the complete VST3 bundle in:

`C:\Program Files\Common Files\VST3\AfroVocal Presets.vst3`

The bundle must remain intact. In particular, do not remove or rename `Contents`, `Contents\Resources`, or `Contents\x86_64-win`. The installer creates a normal Windows uninstaller entry under **Settings → Apps → Installed apps** and **Control Panel → Programs and Features**.

The installer is x64-only. It is intended for 64-bit FL Studio, Ableton Live, and other 64-bit Windows DAWs. The VST3 bundle contains the native x86_64 Windows binary in the architecture directory required by the VST3 format.

## FL Studio

1. Close FL Studio before installing or updating the plug-in.
2. Run the installer and complete the setup wizard.
3. Open FL Studio and choose **Options → Manage plugins**.
4. Select **Find installed plugins**. If the plug-in is listed but marked as unverified, select it and choose **Verify plugins**.
5. Load **AfroVocal Presets** from the VST3 effects list.

FL Studio normally scans the system VST3 directory automatically. If a scan result is stale, close FL Studio completely, reopen it, and run the scan again.

## Ableton Live

1. Close Live before installing or updating the plug-in.
2. Run the installer.
3. In Live, open **Preferences → Plug-Ins**.
4. Enable **VST3 System Folders** and leave **VST3 Custom Folder** disabled unless a custom setup is required.
5. Restart Live and allow its Browser indexing to finish.
6. If the device still does not appear, toggle the relevant plug-in source off and on. For a deep rescan, hold **Alt** while clicking **Rescan**.

## Uninstalling

Close every DAW first. Then use **Settings → Apps → Installed apps → AfroVocal Presets → Uninstall**, or use **Control Panel → Programs and Features**. The uninstaller removes the installed `AfroVocal Presets.vst3` bundle and its installer metadata. It does not remove unrelated plug-ins or user project files.

## Important compatibility notes

The installer is deliberately VST3-only. VST3 has a dedicated Windows system location, unlike VST2, and the complete bundle is installed there rather than inside a DAW’s private program folder. This keeps the plug-in independent of a particular FL Studio or Live release. A DAW must be fully closed during installation or update so it does not retain a stale plug-in scan or a locked module.

The build is not digitally code-signed yet. The binary itself is produced by the native Microsoft Visual C++ Windows runner, but an unsigned installer may cause a Windows SmartScreen warning on first launch. A production commercial release should add an Authenticode certificate and publish the certificate fingerprint alongside the SHA-256 checksum.

## Portable ZIP fallback

The workflow also publishes `AfroVocal-Presets-Windows-x64.zip`. Extract it and copy the complete `AfroVocal Presets.vst3` folder to the same system VST3 location. The ZIP does not create an uninstaller, so the `.exe` installer is preferred.

## Verification performed by the build pipeline

The Windows workflow configures and builds the Release VST3 with MSVC, checks that `Contents\x86_64-win\AfroVocal Presets.vst3` exists, compiles the Inno Setup installer, checks that the `.exe` was created, and publishes SHA-256 checksum files. DAW registration and audio rendering must be completed on Windows because FL Studio and Ableton Live are not available in this Linux build environment.

## References

[1]: https://helpcenter.steinberg.de/hc/en-us/articles/115000177084-VST-plug-in-locations-on-Windows "Steinberg VST plug-in locations on Windows"
[2]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Format.html "Steinberg VST 3 plug-in format structure"
[3]: https://help.ableton.com/hc/en-us/articles/115000349184-VST-AU-plug-in-doesn-t-appear-in-Live-s-Browser "Ableton Live plug-in scanning and troubleshooting"
[4]: https://jrsoftware.org/ishelp.php "Inno Setup documentation"
