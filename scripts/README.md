# Release Guide

This guide explains how to create releases for qPCRtools on different platforms.

## Prerequisites

### For macOS:
- Xcode Command Line Tools
- Qt 6.x installed
- CMake
- Git

### For Windows:
- Visual Studio (with C++ support)
- Qt 6.x installed
- CMake
- Git

### For both:
- GitHub CLI (gh) - [Install from here](https://cli.github.com/)

## Building Release Packages

### macOS DMG

1. **Build and package:**
   ```bash
   cd /path/to/qPCRtools_cpp
   bash scripts/package.sh 1.0.0
   ```

2. **The DMG will be created in `build/` directory:**
   ```
   qPCRtools-1.0.0-macOS.dmg
   ```

3. **Test the DMG:**
   - Mount it
   - Run the app
   - Verify all features work

### Windows EXE

1. **Open "Qt for Windows" command prompt**

2. **Build and package:**
   ```bash
   cd C:\path\to\qPCRtools_cpp
   bash scripts/package.sh 1.0.0
   ```

   Or use Git Bash/WSL with appropriate paths.

3. **The ZIP will be created in `build/` directory:**
   ```
   qPCRtools-1.0.0-Windows.zip
   ```

4. **Test the EXE:**
   - Extract ZIP
   - Run qPCRtools.exe on a clean Windows machine
   - Verify all features work

## Creating GitHub Release

### Method 1: Using GitHub CLI (Recommended)

1. **Create release draft:**
   ```bash
   bash scripts/create_release.sh 1.0.0
   ```

2. **Upload assets:**
   ```bash
   cd build
   gh release upload v1.0.0 qPCRtools-1.0.0-Windows.zip
   gh release upload v1.0.0 qPCRtools-1.0.0-macOS.dmg
   ```

3. **Publish the release:**
   - Go to https://github.com/lixiang117423/qPCRtools_cpp/releases
   - Edit the draft release
   - Review notes and assets
   - Click "Publish release"

### Method 2: Manual via Web Interface

1. **Go to GitHub Releases:**
   ```
   https://github.com/lixiang117423/qPCRtools_cpp/releases/new
   ```

2. **Fill in release information:**
   - Tag version: `v1.0.0`
   - Title: `qPCRtools v1.0.0`
   - Description: Copy from `scripts/release_notes_template.md`

3. **Upload assets:**
   - Drag and drop the DMG and ZIP files
   - Wait for upload to complete

4. **Publish:**
   - Click "Publish release" button

## Version Tagging

Before creating a release, make sure to tag the version:

```bash
# Create and push tag
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

## Testing Checklist

Before releasing:

- [ ] Build succeeds on both platforms
- [ ] Application launches without errors
- [ ] All features work correctly:
  - [ ] Data import (CSV/Excel)
  - [ ] ΔCt analysis
  - [ ] ΔΔCt analysis
  - [ ] Statistical tests
  - [ ] Data export
- [ ] UI displays correctly
- [ ] No console errors or warnings
- [ ] Performance is acceptable

## Troubleshooting

### macOS Issues

**Problem: `macdeployqt` fails**
```bash
# Make sure Qt is in PATH
export PATH="/path/to/Qt/6.x.x/clang_64/bin:$PATH"
```

**Problem: DMG won't open**
- Check security settings: System Preferences → Security & Privacy
- Right-click → Open to bypass Gatekeeper

### Windows Issues

**Problem: Missing DLLs**
```bash
# Run windeployqt manually
windeployqt.exe --release --no-translations qPCRtools.exe
```

**Problem: Application won't start**
- Install Visual C++ Redistributable
- Check Windows Event Viewer for errors

## Post-Release Tasks

1. **Update version numbers:**
   - CMakeLists.txt: `project(qPCRtools VERSION 1.0.1 ...)`
   - WebBridge.h: App version

2. **Create next milestone:**
   - Add issues to GitHub Projects
   - Plan next release features

3. **Announce release:**
   - Update README with latest version
   - Post on social media (if applicable)
   - Notify users

## Automation (Future)

Consider setting up GitHub Actions for:
- Automatic builds on push to main
- Automatic releases on tag creation
- Multi-platform CI/CD
