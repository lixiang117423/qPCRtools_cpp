# archive/

Superseded files, kept for traceability. **Nothing in this directory should be
reused** as a source for the software, the manuscript, or any submission.

| File | What it is | Why it is here |
|---|---|---|
| `manuscript_biorxiv_2026-03.md` | The March 2026 manuscript that was rejected at bioRxiv's submission screening | It contained claims without supporting outputs (a user survey, performance benchmarks, figures and supplements that do not exist). See the 2026-10-13 entry in `CHANGELOG.md`. Do not copy text, numbers or references from it — one of its references was also wrong. The current manuscript is in `manuscript/`. |
| `manuscript_biorxiv_2026-03.docx`, `manuscript_2026-06_word_version.docx` | Word versions of the same two older manuscripts | **Kept locally only — excluded from git** (see `.gitignore`: `archive/*.docx`, ~3 MB of binary that duplicates the tracked `.md` and the extracted screenshot). The Figure-1 screenshot inside the June file was extracted to `manuscript/figures/fig1_home_en.png` before archiving, so nothing is lost from the repository's point of view. |
| `PROGRESS_2025-01_obsolete.md` | Development-progress snapshot from January 2025 | The file itself states that it is out of date. Current status: `docs/08_status.md`. |
| `legacy_qpcr_tools_en_stub.ts` | Empty Qt translation stub (`<TS language="en"/>`) from the first day of the repository | CMake only uses `translations/qpcr_tools_zh_CN.ts` and `translations/qpcr_tools_en_US.ts`. |

Renamed/moved on 2026-10-13 during the repository tidy-up; the old → new mapping
is recorded in `CHANGELOG.md`.
