# Build and test

- Configure: `cmake -S . -B build`
- Build everything: `cmake --build build`
- Build one target: `cmake --build build --target fcitx5` or `cmake --build build --target testemoji`
- Run the full test suite: `ctest --output-on-failure --test-dir build`
- Run one test: `ctest --output-on-failure --test-dir build -R '^testemoji$'`
- Tests depend on `ENABLE_TEST=On` and `ENABLE_TESTING_ADDONS=On`, which are enabled by default in the top-level `CMakeLists.txt`.
- There is no dedicated repo-local lint target; CI enforces `clang-format` using the root `.clang-format`.

# High-level architecture

- `src/lib/` provides the core library layers:
  - `fcitx-utils` for low-level utilities such as the event loop, paths, logging, UTF-8 helpers, and platform abstractions.
  - `fcitx-config` for config parsing and serialization.
  - `fcitx` for the runtime core: `Instance`, `AddonManager`, `InputContextManager`, `InputMethodManager`, `UserInterfaceManager`, and the event pipeline.
- `src/server/main.cpp` is the main server entry point. It constructs `fcitx::Instance`, registers the static addon registry, and starts the event loop.
- Most runtime functionality is implemented as addons under `src/`:
  - `frontend/` exposes client-facing protocols such as XIM, IBus, DBus, and Wayland IM.
  - `im/` contains input method engines. This repository itself only ships the keyboard layout engine; other language engines live in separate repositories.
  - `ui/` contains user interface addons such as classic UI, kimpanel, and virtual keyboard.
  - `modules/` contains supporting services such as DBus, Wayland, XCB, clipboard, spell, emoji, quickphrase, notifications, and unicode tools.
- `data/` installs desktop files, autostart assets, default configuration, and translated metadata. `po/` installs translations.
- `test/` contains the CTest executables. `testing/` contains helper addons (`testfrontend`, `testim`, `testui`) that many core tests depend on.

# Key conventions

- Prefer implementing new runtime behavior as an addon or module instead of adding special cases to the server binary. The common CMake pattern is:
  - declare the addon with `add_fcitx5_addon(...)`
  - generate translated addon metadata from `*.conf.in.in` via `configure_file(...)` and `fcitx5_translate_desktop_file(...)`
  - install the addon config under `${FCITX_INSTALL_PKGDATADIR}/addon`
  - export public module headers with `fcitx5_export_module(...)`
- Keep feature guards aligned across CMake, code, and tests. Optional pieces are consistently gated by options such as `ENABLE_DBUS`, `ENABLE_X11`, `ENABLE_WAYLAND`, `ENABLE_KEYBOARD`, `ENABLE_EMOJI`, and `ENABLE_SERVER`.
- Tests are registered one executable per case in `test/CMakeLists.txt`, and the CTest name usually matches the target name (`testemoji`, `testcompose`, `testspell`, etc.). Build or run the exact named target instead of looking for grouped test binaries.
- Some tests are conditional on platform features or host tools. For example, DBus-related tests depend on `dbus-daemon`, and the XIM/Xvfb path is intentionally disabled in `test/CMakeLists.txt`.
- Source files consistently carry SPDX copyright/license headers, and C/C++ formatting follows the repository `.clang-format`.
- Do not send pull requests that only update translations; translations are maintained separately and synced automatically.
