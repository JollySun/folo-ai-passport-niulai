# Workspace changelog

This file records changes to the monorepo architecture, shared crates, BSP,
build tooling, and CI. Application behavior and release history are recorded in
the owning application changelog:

- [Niu Lai changelog](apps/niulai/CHANGELOG.md)
- [Diagnostics changelog](apps/diagnostics/CHANGELOG.md)

All notable changes follow [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Changed

- Reorganized documentation for the monorepo: cross-application guides now live
  under topic directories, while application and shared-module docs live beside
  their owning workspace members.
- Applications are independently selected, built, packaged, and released from
  `apps/<app>` without a shared registry.
- Split application sdkconfig and partition policy from board defaults, and
  divided the BSP into independently selectable hardware modules.
- Added `passport-core` for shared pure Rust calculations and
  `passport-platform` as the reusable Rust facade over C BSP capabilities.

### Fixed

- Application discovery rejects incomplete or invalid `apps/<app>` directories
  instead of silently omitting them from the CI firmware matrix.
