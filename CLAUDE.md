# CLAUDE.md

This file gives Claude Code guidance for working in this repository.

**All project guidance lives in [AGENTS.md](AGENTS.md).** Read it in full before making changes.

Quick reminders (see AGENTS.md and the constitution for detail):

- **Source of truth:** [`.specify/memory/constitution.md`](.specify/memory/constitution.md).
- **Simplicity first (YAGNI):** build only what is needed now; prefer the simpler design.
- **Test-first (non-negotiable):** every feature ships with unit + integration tests that run
  on the host without a physical ESP32.
- **Layering:** `Transport → Protocol → Control → Hardware`, one-way. Logic never calls
  hardware directly; hardware sits behind interfaces with fakes for tests.
- **Fail safe:** on disconnect, malformed command, or timeout — DC motor stops, servo neutral.
