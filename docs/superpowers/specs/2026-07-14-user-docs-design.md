# User Docs Standard — Design Spec

**Date:** 2026-07-14
**Status:** Approved for implementation
**Owner:** GamepadMouseSim maintainers
**Sub-project:** 3 of 5 under the project-standards initiative
  (code-architecture ✓, commit-workflow ✓, config-release, testing — separate specs to follow)

---

## 1. Scope & Authority

### 1.1 In scope

- `actionToChinese()` as the canonical source for button→Chinese mapping
- ctest validation that all `ButtonAction` values have non-empty Chinese labels
- `help.png` version control (un-ignore from `.gitignore`, commit alongside `actionToChinese` changes)
- README_CN.md table auto-sync via `scripts/sync_docs.py`
- Document structure conventions and update checklists
- Code-to-documentation cross-references

### 1.2 Out of scope

| Concern | Owner sub-project |
|---|---|
| Commit message format, branch model | commit-workflow |
| Code review severity levels | code-architecture |
| help.png pixel styling or layout redesign | — (deferred / product decision) |
| Full automated generation of USER_MANUAL.md | — (manual document by nature) |
| CI integration for doc checks | testing |

### 1.3 Relationships

- The `PULL_REQUEST_TEMPLATE.md` (commit-workflow sub-project) gets an additional checklist item for doc sync.
- The `actionToChinese` ctest joins `test_config_autostart` / `test_autostart_apply` / `test_tray_icon` / `test_log_severity` in the `tests/` suite.

---

## 2. actionToChinese Canonical Source

### 2.1 Declaration

`src/core/Types.cpp:198-295` — `QString actionToChinese(ButtonAction action)` is the **single authoritative source** for all Chinese-language descriptions of gamepad button actions.

All user-facing Chinese documentation derives from this function:
- Help overlay (via `tools/generate_help_png.py` — manually kept in sync)
- README_CN.md button mapping table (via `scripts/sync_docs.py` — auto-synced)
- USER_MANUAL.md section references (via manual cross-reference comments)

### 2.2 ctest validation

New test file: `tests/test_action_to_chinese.cpp`

```cpp
class TestActionToChinese : public QObject {
    Q_OBJECT
private slots:
    void allActions_haveNonEmptyMapping();
    void everyEnumValue_hasChineseLabel();
};
```

- `allActions_haveNonEmptyMapping`: iterates every `ButtonAction` value (excluding sentinel/None variants), calls `actionToChinese()`, asserts result is not empty and not the default `"无"`.
- `everyEnumValue_hasChineseLabel`: same as above, but explicitly asserts the result does NOT equal `"无"` — catches newly added enum values that fall through to `default:`.

If a developer adds a new `ButtonAction` value without adding a corresponding `case` in `actionToChinese()`, the `default: return "无"` path triggers a test failure. This forces the developer to provide a Chinese label for every action.

### 2.3 Sync requirement

When `actionToChinese()` changes, the developer MUST:

1. Update `tools/generate_help_png.py` if the help overlay references the affected action.
2. Run `tools/generate_help_png.py` to regenerate `resources/icons/help.png`.
3. Run `scripts/sync_docs.py` to update `README_CN.md`'s button mapping table.
4. Verify `ctest` passes (including the new `test_action_to_chinese`).
5. Commit all changes (`.cpp` + `.py` + `.png` + `.md`) in a single PR.

This requirement is encoded in `PULL_REQUEST_TEMPLATE.md` (see §4).

---

## 3. help.png Version Control

### 3.1 Current behavior

- `resources/icons/help.png` is excluded by `.gitignore:33` (`resources/icons/help.png`).
- CMake `add_custom_command` (CMakeLists.txt:65-71) invokes `python tools/generate_help_png.py` at build time.
- If Python is unavailable, the build continues without regenerating the file.

### 3.2 New behavior

- **Remove** `resources/icons/help.png` from `.gitignore`.
- **Keep** the CMake build-time generation unchanged.
- The committed `help.png` serves two purposes:
  - Fallback: if Python is unavailable at build time, the committed version is used.
  - Traceability: git history shows when help.png changes alongside actionToChinese changes.
- The PULL_REQUEST_TEMPLATE checklist item `[ ] help.png regenerated if actionToChinese changed` ensures every help-text change includes the PNG delta.

### 3.3 Verification in ctest

The `test_action_to_chinese` test also asserts that every `ButtonAction` referenced in the help overlay's text (determined by grepping `generate_help_png.py` for known enum names) maps to a non-empty Chinese string. This catches sync drift between the Python script and the C++ enum.

---

## 4. README_CN.md Sync Script

### 4.1 Script: `scripts/sync_docs.py`

A Python helper that:

1. Compiles a minimal C++ program that calls `actionToChinese()` for every `ButtonAction` value and writes a newline-delimited JSON mapping.
2. Reads the JSON output.
3. Locates the `## 基本操作` to `## ❓` (or next heading) section boundary in `README_CN.md`.
4. Replaces the button mapping table within that section with a freshly generated table derived from the JSON mapping.
5. Preserves all other content (header, intro paragraph, feature list, etc.).

### 4.2 Table format

The generated table matches the existing format in `README_CN.md`:

```
| 按键 | 功能 |
|------|------|
| A | 左键单击 |
| B | 右键单击 |
...
```

The script only overwrites the table rows; it does not modify surrounding prose or section headings.

### 4.3 Usage

```bash
python scripts/sync_docs.py
```

Run after any `actionToChinese()` change, before committing.

---

## 5. Document Structure Conventions

### 5.1 File responsibilities

| File | Content | Update trigger | Automation |
|---|---|---|---|
| `src/core/Types.cpp` (`actionToChinese`) | Chinese labels for all ButtonAction values | New action / label change | Manual + ctest validates coverage |
| `tools/generate_help_png.py` | Help overlay image layout + text | help overlay content changes | Manual (references actionToChinese via comment) |
| `resources/icons/help.png` | Pre-rendered help image (1920×1080) | help.py changes | CMake build step + committed |
| `README_CN.md` | Chinese README with button mapping table | actionToChinese changes | Semi-auto via `scripts/sync_docs.py` |
| `USER_MANUAL.md` | Detailed Chinese user manual | Behavior changes | Manual + PR checklist reminder |

### 5.2 Cross-reference conventions

- In `USER_MANUAL.md`, every paragraph that describes a button mapping or behavior MUST include a `<sup>[参考代码](../src/core/Types.cpp)</sup>`-style cross-reference link to the relevant source file.
- In `README_CN.md`, the `## 📖 文档` section links to all spec/plan files under `docs/superpowers/`.
- In `tools/generate_help_png.py`, a comment block at the top points to `actionToChinese()` as the canonical source for action descriptions.

---

## 6. PULL_REQUEST_TEMPLATE Amendment

Add to the existing `PULL_REQUEST_TEMPLATE.md` checklist:

```markdown
- [ ] User docs sync: `actionToChinese` → help.png → README_CN.md updated
      (skip if no action/help-text changes)
```

---

## 7. Commit & Deliver

Tasks in execution order:

1. Create `tests/test_action_to_chinese.{cpp,h,main.cpp}` + register in `tests/CMakeLists.txt`
2. Build and run `ctest`; confirm the new test passes
3. Remove `resources/icons/help.png` from `.gitignore`
4. Create `scripts/sync_docs.py`
5. Update `PULL_REQUEST_TEMPLATE.md` with the new checklist item
6. Update `USER_MANUAL.md` with cross-reference comments (first pass)
7. Re-run `ctest` to confirm no regressions
8. Commit all changes

---

## Appendix A — Cross-references

- **Code-architecture spec** (§2.2): defines the `ButtonAction` canonical enum and the 5-step addition checklist referenced here.
- **Commit-workflow spec** (§4): defines the PR template structure that §6 above amends.
- **Testing spec** (future): will define CI hooks that run `sync_docs.py` on relevant changes.