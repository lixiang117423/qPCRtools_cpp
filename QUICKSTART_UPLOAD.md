# 通过命令行上传 DMG 到 GitHub Release

## 方法 1: 使用自动化脚本（推荐）

### 步骤 1: 创建 GitHub Personal Access Token

1. 访问: https://github.com/settings/tokens/new
2. 配置 token:
   - **Note**: 输入 "qPCRtools Release Upload"
   - **Expiration**: 选择有效期（建议 90 days 或 No expiration）
   - **Scopes**: 勾选 `repo` (Full control of private repositories)
3. 点击 "Generate token"
4. **重要**: 立即复制 token（只显示一次！）

### 步骤 2: 设置环境变量

```bash
export GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

### 步骤 3: 运行上传脚本

```bash
bash scripts/upload_to_github.sh
```

脚本会自动：
- ✅ 检查 DMG 文件是否存在
- ✅ 检查 Release 是否已存在
- ✅ 如果不存在，自动创建 Release
- ✅ 上传 DMG 文件
- ✅ 提供下载链接

### 验证

上传成功后，访问:
```
https://github.com/lixiang117423/qPCRtools_cpp/releases/tag/v1.0.0
```

## 方法 2: 手动使用 curl

如果脚本失败，可以手动执行：

### 1. 创建 Release

```bash
export GITHUB_TOKEN=your_token_here

curl -X POST \
  -H "Authorization: token ${GITHUB_TOKEN}" \
  -H "Accept: application/vnd.github.v3+json" \
  https://api.github.com/repos/lixiang117423/qPCRtools_cpp/releases \
  -d '{
    "tag_name": "v1.0.0",
    "target_commitish": "main",
    "name": "qPCRtools v1.0.0",
    "body": "Release notes here...",
    "draft": false,
    "prerelease": false
  }'
```

### 2. 上传 DMG

```bash
# 获取 Release ID
RELEASE_ID=$(curl -s \
  -H "Authorization: token ${GITHUB_TOKEN}" \
  https://api.github.com/repos/lixiang117423/qPCRtools_cpp/releases/tags/v1.0.0 \
  | grep -m 1 '"id":' | head -n 1 | sed -E 's/.*"id": ([0-9]+).*/\1/')

# 上传文件
curl -X POST \
  -H "Authorization: token ${GITHUB_TOKEN}" \
  -H "Content-Type: application/octet-stream" \
  "https://uploads.github.com/repos/lixiang117423/qPCRtools_cpp/releases/${RELEASE_ID}/assets?name=qPCRtools-1.0.0-macOS.dmg" \
  --data-binary @build/qPCRtools-1.0.0-macOS.dmg
```

## 方法 3: 使用 GitHub CLI（如果已安装）

```bash
# 安装 GitHub CLI
brew install gh

# 认证
gh auth login

# 创建 Release
gh release create v1.0.0 \
  --title "qPCRtools v1.0.0" \
  --notes "Release notes..."

# 上传 DMG
gh release upload v1.0.0 build/qPCRtools-1.0.0-macOS.dmg
```

## 常见问题

### Q: Token 无效？
A: 检查 token 是否过期，或重新创建

### Q: 上传失败？
A: 检查：
- DMG 文件是否存在
- Token 权限是否包含 `repo`
- 文件大小是否超过限制（GitHub 限制单个文件 2GB）

### Q: Release 已存在？
A: 脚本会自动检测并使用现有 Release

### Q: 如何删除错误的 asset？
A:
```bash
# 先列出 assets
curl -H "Authorization: token ${GITHUB_TOKEN}" \
  https://api.github.com/repos/lixiang117423/qPCRtools_cpp/releases/tags/v1.0.0

# 删除 asset（需要 ASSET_ID）
curl -X DELETE \
  -H "Authorization: token ${GITHUB_TOKEN}" \
  https://api.github.com/repos/lixiang117423/qPCRtools_cpp/releases/assets/{ASSET_ID}
```

## 安全提示

⚠️ **重要**:
- 不要在脚本或代码中硬编码 token
- 使用环境变量存储 token
- Token 过期后及时删除
- 定期轮换 token

## 下一步

上传完成后：
1. ✅ 验证 Release 页面
2. ✅ 下载并测试 DMG
3. ✅ 分享 Release 链接给用户
4. 📧 发送发布通知（邮件、社交媒体等）

---

**现在就试试吧！** 🚀

```bash
export GITHUB_TOKEN=your_token_here
bash scripts/upload_to_github.sh
```
