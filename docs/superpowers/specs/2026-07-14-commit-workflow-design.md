# Commit & Workflow Standard — Design Spec

**Date:** 2026-07-14
**Status:** Approved for implementation
**Owner:** GamepadMouseSim maintainers
**Sub-project:** 2 of 5 under the project-standards initiative
  (code-architecture ✓ , user-docs, config-release, testing — separate specs to follow)

---

## 1. Scope & Authority

### 1.1 In scope

- Commit message format (types, scopes, body/footer conventions)
- Branch naming and lifecycle
- Merge checklist / PR template

### 1.2 Out of scope

| Concern | Owner sub-project |
|---|---|
| Code review severity levels (already covered by code-architecture spec §5) | code-architecture |
| CI/hook enforcement (no CI exists yet; reserved for testing sub-project) | testing |
| Changelog maintenance strategy (user chose no changelog) | — |
| Code style / clang-format (already covered) | code-architecture |

### 1.3 Relationship to other documents

- This spec refines the commit-type conventions that `AGENTS.md` and the code-architecture spec §6 use.
- The merge checklist references `AGENTS.md` Rule 1 (doc trail) and Rule 2 (simulated tests).
- The review level table in code-architecture spec §5.4 defines "standard" vs "critical" scope — this spec references it.

---

## 2. Commit Message Format

### 2.1 Template

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

All fields: lower-case. `scope` is optional. `type` is required.

### 2.2 Types

| Type      | When to use                                                                 |
|-----------|-----------------------------------------------------------------------------|
| `fix`     | Bug fix or behavior correction (no user-facing feature added)               |
| `feat`    | New user-visible feature (available for future use; not yet used in repo)   |
| `refactor`| Code restructuring with no behavior change                                  |
| `test`    | Adding or modifying tests                                                   |
| `chore`   | Tooling, config, or dependency changes; no `src/` logic touched             |
| `docs`    | Documentation (Markdown, comments, README, specs, plans)                    |
| `build`   | Build system (CMake, deploy, installer), not production code                |
| `style`   | Formatting only (clang-format, whitespace, include order with no logic change) |

### 2.3 Scope (optional)

Lower-case, matching the module that was primarily affected:

`app`, `core`, `ui`, `gamepad`, `input`, `win`, `config`, `build`, `test`, `installer`

If the change crosses multiple scopes, omit the scope entirely.

### 2.4 Description

- Imperative mood (e.g. "fix: **add** missing null check" not "fix: **added**").
- No more than 80 characters.
- No trailing period.

### 2.5 Body (optional)

- Separated from the subject by one blank line.
- Wrapped at 72 characters.
- Explains *what* and *why*, not *how*.

### 2.6 Footer (optional)

- `BREAKING CHANGE:` for incompatible API or config schema changes.
- `Co-authored-by:` for pair programming or multi-author commits.

### 2.7 Validation

No automated enforcement. Run `git log` before push to verify format if desired.

---

## 3. Branch Naming & Lifecycle

### 3.1 Naming pattern

```
<type>/<short-description>
```

Examples:
- `fix/autostart-registry-sync`
- `feat/multi-monitor-osd`
- `chore/clang-format-setup`
- `docs/code-architecture-spec`

- `<type>` matches one of the 8 commit types from §2.2.
- Description in kebab-case, no spaces, no more than 50 characters.

### 3.2 Lifecycle

```
main ◄────────────────────────────────────
  │
  └─ <type>/<desc> ← development
       │
       ├─ Implement (multiple commits allowed; each follows §2)
       ├─ Self-verify: ctest + scripts/check-format.ps1
       ├─ Review (standard or critical — see §4)
       └─ Squash-merge to main with a single §2-compliant commit message
```

**Not enforced.** Direct pushes to `main` are still permitted. The branch model is the *default recommendation*, not a gate.

### 3.3 Branch lifespan

- Short-lived branches (≤ 3 days) encouraged to minimise merge conflicts.
- After merging, delete the remote branch (GitHub deletes it automatically on merge if configured).

---

## 4. Merge Checklist (PR Template)

### 4.1 File location

`PULL_REQUEST_TEMPLATE.md` in the repository root.

### 4.2 Template content

```markdown
## Summary

<!-- One sentence describing what this PR does. -->

## Type

- <type>(<scope>): <description>

## Verification

- [ ] `ctest --test-dir build --output-on-failure` passes
- [ ] `scripts/check-format.ps1` passes (exit 0)
- [ ] AGENTS.md Rule 1: behavior changes include spec/doc updates in the same PR
- [ ] AGENTS.md Rule 2: new features include automated tests (or spec §7.4 carve-out)
- [ ] Review level: □ standard (code quality review only)
                   □ critical (spec compliance + code quality, per code-architecture spec §5.4)

## Reviewer Notes

<!-- Key areas for the reviewer to focus on. -->

## Follow-up

<!-- Known issues, pending work, or follow-up tasks. -->
```

### 4.3 Review levels

### 4.3 Review levels

- **Standard**: single code quality review (most changes).
- **Critical**: spec compliance review + code quality review. Required when touching:
  - Wiring in `src/app/Application.cpp` (new signal/slot connections)
  - Module boundaries (new subsystem, changed dependency direction)
  - Config schema (new key, removed key, changed default)
  - Public API (`ButtonAction`, `ModeManager`, etc.)

The code-architecture spec §2 (Module Dependency & Boundary Rules) defines the
boundary-triggers for critical review. The `superpowers:requesting-code-review`
skill serves as the default review checklist.

---

## 5. AGENTS.md Integration

This spec amends `AGENTS.md` to add a pointer to the merge checklist:

```markdown
## Conventions

- **Merge checklist**: see `PULL_REQUEST_TEMPLATE.md` in the repo root. Every
  PR or merge-to-master should verify each line before proceeding.
- **Commit format**: §2 of `docs/superpowers/specs/2026-07-14-commit-workflow-design.md`.
  Use `fix:`, `feat:`, `refactor:`, `test:`, `chore:`, `docs:`, `build:`, `style:`
  with an optional scope and a concise imperative description.
- **Branch naming** (recommended): `<type>/<kebab-case-description>`.
  Direct master pushes are still allowed.
```

---

## Appendix A — Cross-references

- **Code-architecture spec** (`2026-07-13-code-architecture-design.md`): defines
  critical-vs-standard review scopes (§5.4), companion PR structure (§6).
- **Testing spec** (future): will define CI hooks that automate the checklist items.
- **User-docs spec** (future): will define `README_CN.md` / `USER_MANUAL.md` update
  requirements referenced by Rule 1.