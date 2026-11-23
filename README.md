# 🎮 Tetris Game – C++ DSA Project

A feature-rich Tetris game developed in **C++** using **Data Structures and Algorithms (DSA)** with SDL2 graphics libraries.

This project was developed as part of the Data Structures and Algorithms course to demonstrate the practical implementation of programming concepts through an interactive game.

## 📌 Project Overview

Tetris is a classic block puzzle game where players arrange falling blocks to complete horizontal lines. Completed lines are cleared to earn points and increase the game level.

The game features a graphical interface, scoring system, leaderboard, multiple block shapes, and interactive controls.

## ✨ Features

- Interactive graphical user interface.
- Seven different Tetris block shapes.
- Real-time piece movement and rotation.
- Collision detection.
- Ghost piece preview.
- Hold piece functionality.
- Next pieces preview.
- Soft drop and hard drop.
- Automatic line clearing.
- Score and level management.
- Top five leaderboard.
- Pause and resume functionality.
- Game over and restart system.
- Mouse drag-and-drop functionality.
- Automatic score saving and loading.

## 🛠️ Technologies Used

- C++
- SDL2
- SDL2_ttf
- Standard Template Library (STL)

## 🧠 Data Structures & Algorithms Used

| Concept | Implementation |
|---|---|
| Arrays | Game board and block shapes |
| Vectors | Leaderboard management |
| Deque | Next-piece queue |
| Insertion Sort | Leaderboard ranking |
| Greedy Algorithm | Ghost piece positioning |
| Collision Detection | Validating piece movements |
| State Machine | Managing game states |
| Randomization | Generating block sequences |

## 🎮 Game Controls

| Key | Action |
|---|---|
| ← | Move Left |
| → | Move Right |
| ↑ / X | Rotate Clockwise |
| Z | Rotate Counterclockwise |
| ↓ | Soft Drop |
| SPACE | Hard Drop |
| C | Hold Piece |
| P | Pause / Resume |
| R | Restart After Game Over |

**Mouse Control:** Drag and drop the active piece to position it on the board.

## 📂 Project Files

- `game.cpp` – Main C++ source code.
- `game.exe` – Compiled game executable.
- `font.ttf` – Game font.
- `Makefile` – Build configuration.
- `SDL2.dll` – SDL2 runtime library.
- `SDL2_ttf.dll` – SDL2_ttf runtime library.

## 🚀 How to Run

1. Download or clone this repository.
2. Keep the executable, font file, and required SDL2 DLL files in the same folder.
3. Run `game.exe` to start the game.

To compile from source, use a compatible C++ compiler with SDL2 and SDL2_ttf development libraries installed.

## 🎯 Project Purpose

The purpose of this project is to apply Data Structures and Algorithms concepts in a practical C++ application while developing problem-solving, logical thinking, and programming skills.

---
**Programming Language:** C++  
**Project Type:** Data Structures and Algorithms  
**Category:** Game Development
