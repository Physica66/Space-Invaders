# 🚀 Space Invaders : Earth Alliance

A cooperative, dual-pilot arcade space shooter engineered in C using **Raylib 6.0 for macOS**. *Space Invaders: Earth Alliance* upgrades the classic arcade formula with modern combat game feel, multi-stage boss encounters, dynamic physical hazards, hit-stop impact kinetics, and an authentic cockpit HUD.

---

## 🌌 Overview

When Sector 4's orbital defense line collapses under an extraterrestrial invasion, Earth Alliance scrambles its final line of defense: Interceptors **Aegis-1** (piloted by Md. Shoab Mahmud) and **Shadow-2** (piloted by Nayemul Islam).

Players fight side-by-side through synchronized alien attack formations, survive electronic warfare and teleportation tactics from Elite Commanders, utilize tumbling metallic asteroid fields as cover, and raid the multi-pod **Dreadnought Carrier Flagship** to save Earth from total destruction.

---

## ⚡ Key Features

### 1. Dual-Pilot Cooperative Interceptors
* **Pilot 1: Shoab Mahmud [Aegis-1]**  
  Armed with twin plasma cannons and the devastating **Hyper Laser Beam**, an armor-piercing superweapon capable of penetrating deflector shields and cutting through rows of heavy hostiles.
* **Pilot 2: Nayemul Islam [Shadow-2]**  
  A high-agility stealth interceptor armed with twin blasters and an **8-Way Guided Cluster Salvo** that blankets the battlefield with kinetic detonations.

---

### 2. Visceral Combat Feel & "Two Flavors of Freeze"
* **Activation Super-Pause (0.18s)**  
  Triggering a special ability pauses world physics and renders a momentary blinding chromatic lens flash before projectiles unleash.
* **Impact Hit-Stop (0.04s – 0.05s)**  
  High-energy impacts against heavy pod armor, cluster detonations, and asteroid shatters temporarily freeze position updates to deliver physical weight.
* **Interceptor Canopy Specular Glints**  
  Laser discharges reflect faintly against the upper corners of the screen, creating the visual sensation of looking through an interceptor's glass canopy.
* **Nebula Gas & Space Lightning**  
  Multi-layered cyan and purple cosmic gas clouds drift in the background, flashing white whenever electronic jamming pulses detonate.

---

### 3. Cooperative Synergy Mechanics
* **Synchronized Team Shield Dome `[ENTER]`**  
  Flying within 375 pixels of your partner establishes an energy tether. Activating the dome deploys an indestructible 240px half-sphere barrier for 4.0 seconds that vaporizes grunt aliens and deflects all incoming ordnance (25s recharge).
* **Last Stand Overdrive**  
  If one pilot is shot down, the surviving interceptor immediately gains 2.5 seconds of a 25% speed surge, rapid plasma overcharge, and shield flaring.
* **Combat Defibrillator & Revival**  
  Downed pilots deploy a crash beacon. A surviving partner with more than 1 life can hover over the beacon for 1.5–2.0 seconds to revive their companion at the cost of one shared extra life.

---

### 4. Space Hazards & Electronic Warfare
* **Metallic Asteroids & Dynamic Cover**  
  Every 13 seconds, metallic asteroids tumble diagonally across orbit. They absorb alien and boss fire as neutral shields. Ramming an asteroid incurs a 60% engine slowdown penalty for 3 seconds, while shattering one awards +100 points and shaves 1.0s off special weapon cooldowns.
* **Elite Alien Commanders (Spawn at 65% wave clearance)**  
  * **Commander Jammer (35 HP):** Deploys EMP telemetry waves that jam cockpit radar and HUD systems.
  * **Commander Warp (30 HP):** Armed with rapid blasters and automatic reactive micro-teleportation.
* **Multi-Stage Dreadnought Carrier Raid**  
  The final boss features bilateral deflector shield pods (75 HP each) that render the central core immune until breached. Destroying both pods triggers an orbital EMP airdrop before the reactor core enrages.

---

### 5. Display & Presentation Architecture
* **Native macOS Fullscreen (Cocoa Spaces)**  
  Integrated directly with macOS Window Server via an Objective-C runtime bridge, enabling the green title-bar traffic light button and `[F]` / `[F11]` hotkeys to slide into a native macOS Space without display aborts.
* **High-Resolution Virtual Canvas**  
  Rendered onto an internal 1500 × 900 `RenderTexture2D` canvas with bilinear filtering, ensuring aspect-fit letterboxing across 16:9 and 16:10 MacBook Retina displays.
* **Real-Time Comms & Broadcasting**  
  Features high-tech glassmorphic comms framing, encrypted alien glitch frequencies, and a live Global News Network (GNN) lower-third ticker tape.

---

## 🎮 Flight Controls

### Pilot Controls

| Function | Pilot 1: Shoab (Aegis-1) | Pilot 2: Nayemul (Shadow-2) |
| :--- | :--- | :--- |
| **Movement** | `[A]` Left / `[D]` Right | `[LEFT ARROW]` / `[RIGHT ARROW]` |
| **Primary Fire** | `[W]` or `[SPACE]` | `[UP ARROW]` |
| **Special Weapon** | `[Q]` (Hyper Laser Beam) | `[RIGHT SHIFT]` (Cluster Salvo) |
| **Team Shield Dome** | `[ENTER]` *(Tether < 375px)* | `[ENTER]` *(Tether < 375px)* |
| **Revive Partner** | Hover beacon for 2.0s | Hover beacon for 1.5s |

### Universal Controls
* **Pause / Resume Game**: `[P]`
* **Toggle Fullscreen (macOS Spaces)**: `[F]` or `[F11]` (or click the green title-bar button)
* **Return to Main Menu**: `[M]`
* **Restart Campaign (After Game Over / Win)**: `[R]`
* **Advance Dialogues / Skip Cutscenes**: `[ENTER]` or `[SPACE]`

---

👥 Developers & Project Credits:
Developed as an academic and competitive engineering project at Bangladesh University of Engineering and Technology (BUET).
Core Contributors:
Nayemul Islam (Lead Developer) — Roll 2505087, CSE Section B:
1.)Engineered the MacBook native Spaces borderless fullscreen engine and Cocoa window bridge.
2.)Implemented unified [M] key navigation across all game states, menus, and debriefing screens.
3.)Designed interactive drifting metallic asteroids, dynamic space cover, and physical deflections.
4.)Added asteroid 60% engine slowdown penalties, collision sparks, and shatter rewards.
5.)Implemented the "Two Flavors of Freeze": 0.18s Activation Super-Pause with chromatic screen flare.
6.)Implemented kinetic impact hit-stop frame freezing on laser piercing and missile impacts.
7.)Created interceptor canopy glass specular reflections responding to laser fire.
8.)Designed Last Stand Overdrive: 2.5s rapid plasma surge and shield flaring on wingman crash.
9.)Implemented layered dynamic Nebula gas clouds drifting across exospheric deep space.
10.)Added atmospheric space lightning flashes triggered by enemy telemetry pulses.
11.)Built high-tech glassmorphic cockpit comms dialogue boxes and cybernetic corner brackets.
12.)Designed real-time international breaking news broadcast chyron interface with live bugs.
13.)Created Alien Dreadnought Overlord encrypted glitch-frequency warning boxes.
14.)Authored full interactive Story Mode, narrative, and emergency defense broadcasts.
15.)Built Dreadnought Carrier Boss mechanics, bilateral deflector pods, and enrage modes.
16.)Designed Alien Commanders: Jammer HUD jamming and Warp micro-teleportation systems.
17.)Engineered Hero 8-Way Guided Cluster Salvo with expanding destructive kinetic blast radii.
18.)Directed and balanced 98% of in-game audio sound effects and multi-track combat BGM.
19.)Built 3-layer parallax star engine, CRT scanlines, and End-of-Run Rank Plaque Grading.
Md. Shoab Mahmud (Co-Developer) — Roll 2505066, CSE Section B:
1.)Built the basic core foundation and base loop of the game.
2.)Worked with regular alien sprites, grid positioning, and sound fx.
3.)Added the sprite and flight mechanics of the Scorpion Hero (Aegis-1).
4.)Implemented the special Hyper Laser Beam feature of Scorpion.
5.)Added custom sprites and sound effects for the Scorpion Laser Beam.
6.)Coordinated and assisted Nayemul in feature implementations and debugging.
7.)Defined initial functions and mechanisms reused throughout the game.
8.)Identified and fixed the primary alien movement and boundary bounce bug.
