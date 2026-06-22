# AGENTS.md — Wywy-Website-Master-Database

## Implementation plans

Plans live in the control repo: [`internal/implementation-plans/`](/etc/Wywy-Website-Control/internal/implementation-plans/)

## This repo is part of a multi-service control system

All commands run from `/etc/Wywy-Website-Control/` root, NOT from inside this repo. See [`docs/wywy-website-control.mdx`](/etc/Wywy-Website-Control/docs/wywy-website-control.mdx) for control commands.

## Service documentation

See [`internal/services/master-database.mdx`](/etc/Wywy-Website-Control/internal/services/master-database.mdx) for architecture, datatype validation, submodules, tests, and conventions. Docker container details at [`internal/services/master-database/containers/`](/etc/Wywy-Website-Control/internal/services/master-database/containers/).

## Git workflow

After making changes, stage them with `git add` for the next commit.

## Language conventions

When writing code, ALWAYS check the applicable language convention files in [`internal/conventions/languages/`](/etc/Wywy-Website-Control/internal/conventions/languages/):

- [`_shared.mdx`](/etc/Wywy-Website-Control/internal/conventions/languages/_shared.mdx) — applies to all languages
- [`c.mdx`](/etc/Wywy-Website-Control/internal/conventions/languages/c.mdx)
