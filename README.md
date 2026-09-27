Space Invaders
A high-performance, cooperative dual-pilot arcade space shooter built in C using Raylib 6.0 for macOS. Space Invaders: Earth Alliance reimagines the classic arcade shooter with modern combat game feel, multi-stage boss encounters, dynamic physical hazards, hit-stop impact kinetics, and an authentic exospheric tactical HUD.
Overview
When the orbital defenses of Sector 4 collapse under an overwhelming extraterrestrial invasion, Earth Alliance scrambles its final line of defense: Interceptors Aegis-1 (piloted by Md. Shoab Mahmud) and Shadow-2 (piloted by Nayemul Islam).
Players fight side-by-side through synchronized alien attack formations, survive electronic warfare and teleportation tactics from Elite Commanders, utilize tumbling metallic asteroid fields as cover, and raid the multi-pod Dreadnought Carrier Flagship to save Earth from total destruction.
Key Features:
1. Dual-Pilot Cooperative Interceptors
Pilot 1: Shoab Mahmud [Aegis-1]: Armed with twin plasma cannons and the devastating Hyper Laser Beam, an armor-piercing superweapon capable of penetrating shields and shredding lines of heavy hostiles.
Pilot 2: Nayemul Islam [Shadow-2]: A high-agility stealth interceptor armed with twin blasters and an 8-Way Guided Cluster Salvo that saturates the battlefield with expanding kinetic detonations.
2. Visceral Combat Feel & "Two Flavors of Freeze"
Activation Super-Pause (0.18s): Firing a special ability pauses world physics and renders a momentary blinding chromatic lens flash before projectiles unleash.
Impact Hit-Stop (0.04s – 0.05s): High-energy impacts against heavy pod armor, cluster detonations, and asteroid shatters temporarily freeze position updates to deliver bone-crunching physical weight.
Interceptor Canopy Specular Glints: Laser discharges reflect against the upper corners of the cockpit glass, creating the visual sensation of looking through a flight canopy.
Nebula Gas & Space Lightning: Ambient multi-layered cyan/purple gas clouds drift across space, flashing white whenever electronic jamming pulses detonate.
3. Cooperative Synergy Mechanics
Synchronized Team Shield Dome [ENTER]: Flying within 375 pixels of your wingman establishes a quantum tether. Activating the dome deploys an impenetrable 240px half-sphere energy barrier for 4.0 seconds that vaporizes grunt aliens and deflects all incoming ordnance (25-second cooldown).
Last Stand Overdrive: If a pilot is shot down, the surviving pilot immediately triggers a 2.5-second adrenaline surge with a 25% speed increase, rapid plasma overcharge, and shield flaring.
Combat Defibrillator & Revival: Downed pilots leave a crash beacon. A surviving partner with more than 1 life can hover over the beacon for 1.5–2.0 seconds to revive their companion back into the fight at the cost of one extra life.
4. Interactive Space Hazards & Electronic Warfare
Metallic Asteroids & Dynamic Cover: Every 13 seconds, metallic asteroids tumble diagonally across orbit. They absorb alien and boss laser fire as neutral shields. Ramming an asteroid incurs a 60% engine slowdown penalty for 3 seconds. Shattering an asteroid yields +100 score and shaves 1.0 second off special ability cooldowns.
Elite Alien Commanders: At 65% wave clearance, Commander Jammer (deploys EMP telemetry jams that glitched the cockpit HUD) and Commander Warp (micro-teleports evasively when targeted) deploy to break player formations.
Multi-Stage Dreadnought Carrier Raid: The final boss features bilateral deflector shield pods (75 HP each) that render the central core immune to damage until breached. Destroying pods prompts an orbital EMP airdrop before the reactor enrages into high-speed assault mode.
5. Authentic Real-Time Presentation & macOS Spaces Support
Native macOS Fullscreen (Cocoa Spaces): Integrated with the macOS Window Server via an Objective-C runtime bridge, enabling the green traffic light button and [F] / [F11] hotkeys to slide into macOS Spaces without resolution crashes or display aborts.
High-Resolution Virtual Canvas: Rendered directly onto an internal 1500×900 RenderTexture2D canvas with bilinear filtering, ensuring aspect-fit letterboxing across 16:9 and 16:10 MacBook Retina displays.
Real-Time Comms & Broadcasting: High-tech glassmorphic cockpit comms boxes, encrypted Alien Overlord glitch frequencies, and a 24/7 Global News Network (GNN) live ticker broadcast system.
Live Video Cutscenes: Uses FFmpeg stream piping to render opening launch and game-over/victory cinematic sequences directly to screen.
Flight Controls
Function	Pilot 1: Shoab (Aegis-1)	Pilot 2: Nayemul (Shadow-2)
Movement	[A] Left / [D] Right	[LEFT ARROW] / [RIGHT ARROW]
Primary Fire	[W] or [SPACE]	[UP ARROW]
Special Weapon	[Q] (Hyper Laser Beam)	[RIGHT SHIFT] (Cluster Salvo)
Team Shield Dome	[ENTER] (Requires Quantum Tether < 375px)	[ENTER] (Requires Quantum Tether < 375px)
Rescue / Revive	Hover over partner beacon for 2.0s	Hover over partner beacon for 1.5s
Universal Controls
Pause / Resume Game: [P]
Toggle Fullscreen (macOS Spaces): [F] or [F11] (or click the green title-bar button)
Return to Main Menu: [M]
Restart Campaign (After Game Over / Win): [R]
Advance Dialogues / Skip Cinematics: [ENTER] or [SPACE]

Project Architecture & Credits:
Developed as an academic and competitive engineering project at Bangladesh University of Engineering and Technology (BUET).
Core Contributors
Nayemul Islam (Lead Developer) — Roll 2505087, CSE Section B
Engineered macOS native Spaces borderless fullscreen engine and Cocoa window bridge.
Implemented unified [M] menu navigation, 1500×900 virtual canvas scaling, and display letterboxing.
Designed interactive drifting metallic asteroids, dynamic space cover, 60% slowdown penalties, and ricochet mechanics.
Implemented the "Two Flavors of Freeze" (0.18s Activation Super-Pause and kinetic impact hit-stop).
Designed combat visual effects: canopy glass sheen, Last Stand Overdrive, Nebula gas drift, and EMP space lightning.
Engineered high-tech glassmorphic comms framing, encrypted alien glitch boxes, and live news broadcast chyrons.
Designed the Hero 8-Way Guided Cluster Missile salvo and area-of-effect blast logic.
Directed 98% of in-game audio sound effects, multi-track combat BGMs, and voice acting pipelines.
Md. Shoab Mahmud (Co-Developer) — Roll 2505066, CSE Section B
Developed core game loop architecture and initial state machine design.
Implemented initial alien grid positioning, boundary bounce collision routines, and basic alien bullet mechanics.
Designed the Aegis-1 (Scorpion) interceptor sprite and flight envelope.
Implemented the Scorpion Hyper Laser Beam superweapon, beam sprite rendering, and sound effects.
