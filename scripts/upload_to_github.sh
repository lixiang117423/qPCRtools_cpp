#!/bin/bash

# Upload DMG to GitHub Release using GitHub API

set -e

VERSION="1.0.0"
REPO="lixiang117423/qPCRtools_cpp"
DMG_FILE="build/qPCRtools-${VERSION}-macOS.dmg"

echo "=== Uploading DMG to GitHub Release ==="

# Check if DMG exists
if [ ! -f "${DMG_FILE}" ]; then
    echo "❌ DMG file not found: ${DMG_FILE}"
    echo "Please run: bash scripts/macos_deploy.sh"
    exit 1
fi

echo "DMG file: ${DMG_FILE}"
echo "File size: $(ls -lh "${DMG_FILE}" | awk '{print $5}')"

# Check for GitHub token
if [ -z "$GITHUB_TOKEN" ]; then
    echo ""
    echo "❌ GitHub token not found!"
    echo ""
    echo "Please set GITHUB_TOKEN environment variable:"
    echo "  export GITHUB_TOKEN=your_token_here"
    echo ""
    echo "To create a GitHub token:"
    echo "  1. Visit: https://github.com/settings/tokens/new"
    echo "  2. Select scopes: repo (full repo access)"
    echo "  3. Click 'Generate token'"
    echo "  4. Copy the token (you won't see it again!)"
    echo ""
    exit 1
fi

# Check if release exists
echo "Checking if release exists..."
RELEASE_RESPONSE=$(curl -s \
    -H "Authorization: token ${GITHUB_TOKEN}" \
    "https://api.github.com/repos/${REPO}/releases/tags/v${VERSION}")

RELEASE_ID=$(echo "${RELEASE_RESPONSE}" | grep -m 1 '"id":' | head -n 1 | sed -E 's/.*"id": ([0-9]+).*/\1/')

if [ -z "${RELEASE_ID}" ]; then
    echo "Release not found. Creating release..."

    # Create release
    CREATE_RESPONSE=$(curl -s \
        -X POST \
        -H "Authorization: token ${GITHUB_TOKEN}" \
        -H "Accept: application/vnd.github.v3+json" \
        "https://api.github.com/repos/${REPO}/releases" \
        -d "{
            \"tag_name\": \"v${VERSION}\",
            \"target_commitish\": \"main\",
            \"name\": \"qPCRtools v${VERSION}\",
            \"body\": \"## qPCRtools v${VERSION} - 首次正式发布\\n\\n专业的 qPCR 数据分析软件，支持 ΔCt/ΔΔCt 分析方法和多种统计检验。\\n\\n### 主要功能\\n\\n✨ **分析方法**\\n- ΔCt 方法：归一化到参照基因\\n- ΔΔCt 方法：相对于对照组的相对定量\\n- 统计检验：t-test, Wilcoxon test, ANOVA\\n\\n🎨 **用户界面**\\n- 现代化 Web 界面\\n- 支持中英文双语\\n- 直观的数据导入和参数配置\\n\\n📊 **数据支持**\\n- CSV 格式导入\\n- Excel 格式导入\\n- 数据导出为 CSV/Excel\\n\\n### 系统要求\\n\\n**macOS:**\\n- macOS 10.15 或更高版本\\n- Apple Silicon (M1/M2/M3) 或 Intel 处理器\\n\\n**Windows:**\\n- Windows 10 或更高版本（即将推出）\\n\\n详见文档: https://github.com/${REPO}/blob/main/CREATE_RELEASE_GUIDE.md\",
            \"draft\": false,
            \"prerelease\": false
        }")

    RELEASE_ID=$(echo "${CREATE_RESPONSE}" | grep -m 1 '"id":' | head -n 1 | sed -E 's/.*"id": ([0-9]+).*/\1/')
    echo "✅ Release created (ID: ${RELEASE_ID})"
else
    echo "✅ Release exists (ID: ${RELEASE_ID})"
fi

# Upload DMG file
echo "Uploading DMG file..."
echo "This may take a few minutes (file size: $(ls -lh "${DMG_FILE}" | awk '{print $5}'))..."

UPLOAD_RESPONSE=$(curl -s \
    -X POST \
    -H "Authorization: token ${GITHUB_TOKEN}" \
    -H "Content-Type: application/octet-stream" \
    "https://uploads.github.com/repos/${REPO}/releases/${RELEASE_ID}/assets?name=$(basename "${DMG_FILE}")" \
    --data-binary @"${DMG_FILE}")

# Check for errors
if echo "${UPLOAD_RESPONSE}" | grep -q "errors"; then
    echo "❌ Upload failed!"
    echo "${UPLOAD_RESPONSE}"
    exit 1
fi

# Get download URL
DOWNLOAD_URL=$(echo "${UPLOAD_RESPONSE}" | grep -m 1 '"browser_download_url":' | head -n 1 | sed -E 's/.*"browser_download_url": "([^"]+)".*/\1/')

echo ""
echo "=== Upload successful! ==="
echo ""
echo "DMG file uploaded to GitHub Release"
echo "Release URL: https://github.com/${REPO}/releases/tag/v${VERSION}"
echo "Download URL: ${DOWNLOAD_URL}"
echo ""
echo "✅ Done! Users can now download the DMG from the Releases page."
