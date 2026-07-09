# 🍏 Ncurses Fruit Ninja

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20WSL-lightgrey)](https://en.wikipedia.org/wiki/Unix-like)
[![Language](https://img.shields.io/badge/language-C99-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))

A fast-paced, high-density ASCII arcade game built for Unix terminal environments. 

By bypassing traditional, high-overhead mouse abstraction layers, this game directly intercepts raw ANSI escape sequences (`\033[?1003h`) from the terminal driver. This enables **sub-frame mouse hover tracking** and **click-and-drag mechanics** rendered at a locked 50 FPS.

---

##  Controls

| Action | Control | Mechanics |
| :--- | :--- | :--- |
|  **Move Blade** | **Mouse Hover** | Drag the blade row horizontally and vertically over the game space. |
|  **Slice** | **Left Click / Drag** | Press and hold to activate the cutting edge through flying targets. |
|  **Action** | **SPACEBAR / ENTER** | Start the game, skip menu overlays, or trigger a game retry. |
|  **Exit** | **Q / q** | Cleanly exits the game loop and safely restores native terminal states. |

---

##  Features

* **Frame-Perfect Mouse Tracking:** Raw packet interception bypasses buggy `getmouse()` libraries to ensure smooth cursor feedback on every loop iteration.
* **High-Density Unicode Sprites:** Custom 4-row block matrices designed for dynamic rendering of 6 unique fruit profiles (Apples, Bananas, Oranges, Watermelons, Grapes, and Strawberries).
* **Asymmetrical Slice Vectors:** Advanced slice-state handlers split complex objects like bananas into distinct left and right physical halves that drift apart using realistic x-axis offsets.
* **Particle & State Engine:** Features physics-based exploding debris particles, visual damage cooldown frames, and a modular entity system that transitions sliced fruits into automated robotic clean-up phases.

---

## Installation & Quick Start

### Prerequisites

Ensure your environment has a standard C compiler and development headers for `ncurses`.

```bash
# Ubuntu / Debian / WSL
sudo apt update
sudo apt install build-essential libncurses5-dev libncursesw5-dev

# macOS (via Homebrew)
brew install ncurses

Clone your repository, open a native shell wrapper, and run:

Bash
# Compile the executable using the system compiler flag definitions
gcc -O3 fruit_ninja.c -o fruit_ninja -lncurses -lm

# Run the game binary
./fruit_ninja
