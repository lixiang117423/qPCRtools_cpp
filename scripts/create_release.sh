#!/bin/bash

# Create GitHub Release script for qPCRtools

set -e

VERSION=${1:-"1.0.0"}
TITLE="qPCRtools v${VERSION}"
DESCRIPTION="Release of qPCRtools version ${VERSION}

Features:
- ΔCt and ΔΔCt analysis methods
- Statistical tests (t-test, Wilcoxon, ANOVA)
- Web-based interface with modern UI
- Support for CSV and Excel data import
- ggplot2-style visualizations

Changes:
- See commit history for detailed changes

Assets:
- Windows executable (ZIP)
- macOS application (DMG)

"

echo "=== Creating GitHub Release for v${VERSION} ==="

# Check if GitHub CLI is installed
if ! command -v gh &> /dev/null; then
    echo "❌ GitHub CLI (gh) is not installed"
    echo ""
    echo "Please install it from: https://cli.github.com/"
    echo ""
    echo "Or create release manually at:"
    echo "https://github.com/lixiang117423/qPCRtools_cpp/releases/new"
    exit 1
fi

# Check if user is authenticated
if ! gh auth status &> /dev/null; then
    echo "❌ Not authenticated with GitHub"
    echo "Please run: gh auth login"
    exit 1
fi

# Create the release
echo "Creating release..."
gh release create "v${VERSION}" \
    --title "${TITLE}" \
    --notes "${DESCRIPTION}" \
    --draft

echo ""
echo "✅ Release draft created successfully!"
echo ""
echo "Next steps:"
echo "1. Upload the packaged files:"
echo "   gh release upload v${VERSION} build/qPCRtools-${VERSION}-Windows.zip"
echo "   gh release upload v${VERSION} build/qPCRtools-${VERSION}-macOS.dmg"
echo ""
echo "2. Or upload them manually at:"
echo "   https://github.com/lixiang117423/qPCRtools_cpp/releases/edit/v${VERSION}"
echo ""
echo "3. Review and publish the release when ready"
