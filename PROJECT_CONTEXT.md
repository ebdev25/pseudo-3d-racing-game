# PROJECT_CONTEXT.md

Documentation for **future AI coding agents** maintaining this pseudo-3D racing game. The codebase is **C** with **SDL2** (video, geometry, image, TTF, mixer), a **single-threaded** loop, and an **OutRun-style** segment renderer.

---

## 1. Overall architecture

- **Central state**: One `Game` struct (`game.h`) holds the **borrowed** `SDL_Renderer*`, player state, road, AI, UI, **game-owned** GPU/audio handles (textures, fonts, `Mix_Music` / `Mix_Chunk`), background sliding window, **`LevelSceneryData loadedScenery`**, lap/race flags, and two memory arenas.
- **Module split** (rough layering):
  - **Bootstrap**: `main.c` — **owns** `SDL_Init` / `SDL_Quit`, `IMG_Init` / `IMG_Quit`, `TTF_Init` / `TTF_Quit`, `Mix_OpenAudio` / `Mix_CloseAudio`, **`SDL_Window`** / **`SDL_Renderer`**; calls `paths_init` / `paths_quit`; logical resolution; **`initGame` →** main loop → **`cleanupGame` →** subsystem quit (see §2 init/shutdown table).
  - **Paths**: `paths.c` / `paths.h` — resolve relative asset/level paths against executable dir, its parent, and cwd (see §8).
  - **Orchestration**: `game.c` — **`bool initGame`** / **`cleanupGame`** (game-owned resources only), race state machine (`RaceState`), physics hooks, parallax/LOD triggers, UI event routing.
  - **World**: `road.c` / `road.h` — segments, sprite instances, procedural road from level commands, `findSegment`, elevation helpers.
  - **Rendering**: `render.c` / `render.h` — background layers, road polygons, billboard/player/AI sprites, HUD text.
  - **Background systems**: `background.c`, `background_game.c` — parallax, biome cycling, transitions (`SlidingWindow`).
  - **Level data**: `level_loader.c` — JSON via `cJSON`, populates `LevelRoadData`, **`LevelSceneryData`** (into `game->loadedScenery`), and background; resolves paths with **`paths_resolve`**; loads textures/music; **`rollbackLevelLoadGpuResources`** on partial failure.
  - **AI**: `ai.c` / `ai.h` — opponent motion, behaviors, lap logic, off-road billboard hits.
  - **Collision**: `collision.c` — player↔AI, AI↔AI; player roadside overlap lives partly in `game.c` (`handleOffRoadCollisions`).
  - **UI**: `ui.c` / `ui.h` — menus and actions.
  - **Leaderboard**: `leaderboard.c` — end-race ordering, texture generation (arena-backed entries).
  - **Math/helpers**: `util.c` — `project`, `overlap`, `increase` (wrap), easing, clamps.
  - **Data/constants**: `constants.h` / `constants.c` — screen size, road constants, `Point`, `Sprite`, sprite atlases, color schemes.

There is **no ECS**; systems are **functions + the `Game` pointer**.

---

## 2. Game loop structure (`main.c` + `game.c`)

### Initialization and shutdown responsibilities

| Concern | Owner | Notes |
|--------|--------|--------|
| `SDL_Init` / `SDL_Quit`, `IMG_*`, `TTF_*`, `Mix_OpenAudio` / `Mix_CloseAudio` | **`main.c`** | Single place for SDL subsystem lifecycle. |
| `SDL_Window`, `SDL_Renderer` | **`main.c`** | Created after subsystems; destroyed after `cleanupGame`. |
| `paths_init` / `paths_quit` | **`main.c`** | After `SDL_Init` / before `SDL_Quit`. |
| `initGame` | **`game.c`** | Returns **`bool`**. On failure, calls **`cleanupGame`** internally (no second cleanup in `main`). |
| Textures, `TTF_Font`, `Mix_Music` / `Mix_Chunk` from level + bundled assets | **`Game` / `cleanupGame`** | Includes level background/spritesheet/music, HUD font, SFX, opponent/animated atlases. |
| `SlidingWindow` struct | **`malloc`** in `loadLevel`; **`free`** in **`cleanupGame`** | Cycle states’ **`texture`** fields are **non-owning** aliases of **`game->background`** (see §8). |
| Frame / level arenas | **`cleanupGame`** | Resets both arenas and **`SDL_free`** backing stores. |

**Success shutdown order** (`main.c`): `cleanupGame` → `paths_quit` → `Mix_CloseAudio` → `TTF_Quit` → `IMG_Quit` → destroy renderer → destroy window → `SDL_Quit`.

**Failure**: If `initGame` fails, `main` runs the same SDL teardown **without** calling `cleanupGame` again.

---

1. **Poll SDL events** → `handleEvent` (UI first if a menu is open; otherwise driving keys; global Escape handling).
2. **`updateGame(game, dt)`**:
   - **`arena_reset(&game->frameArena)`** every frame (scratch allocations for that frame).
   - If **`RACE_STATE_COUNTDOWN`**: only traffic-light animation; return.
   - If movement disallowed or UI active: return early.
   - Running race: accumulate `totalRaceTime`, `currentLapTime`.
   - **`findSegment`** for player; then **AI + collisions**, **player physics**, **off-road + roadside sprite collisions**, **lap logic**.
   - **`calculateHorizonY`**, **parallax** (`updateBackgroundParallax`), optional **LOD threshold** → `advanceToNextState`, **`updateBackgroundTransition`**.
3. **`renderGame`**:
   - **`render(game)`** — full frame (background, road, world sprites, AI, player, traffic light).
   - Overlay: **UI** if `ui.currentState != NONE`; else **leaderboard** if race finished; else **HUD** during countdown/running.
4. **`SDL_RenderPresent`**.

Delta time is clamped to **100 ms** max to avoid huge jumps after pauses.

---

## 3. Rendering pipeline and order

Implemented primarily in **`render()`** (`render.c`):

| Step | What |
|------|------|
| 1 | `SDL_RenderClear` (black). |
| 2 | **`renderSlidingWindow`** — fill sky from interpolated color; draw **3 parallax layers** per active biome state (current + target if transitioning). |
| 3 | **First segment loop** (`n = 0 … drawDistance-1`): project segment `p1`/`p2`, **`renderSegment`** (grass quad, rumble, road, lane markers), maintain **`highestRoadY` / `lowestRoadY`** and per-segment **`clip`** (descending Y for near segments). |
| 4 | **Second loop** (`n = drawDistance-1 … 1`, **descending**): for each visible segment, **roadside sprites** → **`renderAIOpponents`** → **player** (if this segment is the player segment) → **`renderTrafficLight`**. |

**Painter’s order**: sky/background → road (far→near in first loop builds clip) → world objects (far→near in second loop) so nearer objects draw on top.

**HUD/UI**: after `render()`, in `renderGame` — menus, leaderboard texture, or TTF HUD.

---

## 4. Pseudo-3D road projection

Classic **segment-based** approach (similar to popular JS tutorials for “pseudo 3D racer”):

- Each **`Segment`** has world **`p1` / `p2`** (z spaced by **`SEGMENT_LENGTH`**), **`curve`**, and colors.
- Each frame, for visible segments, **`project()`** (`util.c`) converts world to camera space then screen:
  - `scale = cameraDepth / cameraZ`
  - Screen X/Y centered on viewport; **road half-width** in pixels is `scale * ROAD_WIDTH * width/2`.
- **Horizontal displacement** accumulates **curve** along the chain so the road “bends.”
- **Player lateral** position **`playerX`** is in a normalized range (clamped ~**[-3, 3]** in physics); combined with **road width** when projecting.
- **Camera**: **`playerZ`** is derived from FOV and **`CAMERA_HEIGHT`** in `initGame` so the “camera” sits above the road; **`cameraDepth`** is `1/tan(FOV/2)`.

**Horizon**: `calculateHorizonY` uses **`highestRoadY`** from the last render pass (fallback: half screen height). Background layers’ Y positions are tied to this for alignment.

---

## 5. Sprite rendering and scaling

- **Atlases**: Level **`spritesheet`** for scenery/player; separate **`opponentSpritesheet`** for AI; **`animatedSpritesheet`** for lights.
- **`Sprite`** (`constants.h`): source rectangle in atlas + optional **`useCustomCollisionBox`**, **`collisionWidth`**, **`collisionOffsetX`**.
- **`renderSprite`**:
  - Scaled size uses segment **`scale`**, **`SPRITE_SCALE`** (derived from player sprite width), **`ROAD_WIDTH`**, and screen width — billboard size shrinks with distance.
  - **Anchor**: fractional **`offsetX` / `offsetY`** (typically **-0.5, -1.0** for bottom-center style placement).
  - **Clip**: **`clipY`** crops the bottom of the sprite so it doesn’t draw below the road segment horizon for that column of road.

**Player**: `renderPlayer` chooses frame from **steer** and **uphill** (`p2.world.y - p1.world.y`), adds a small random **bounce** scaled by speed.

**AI**: `renderAIOpponents` interpolates position within segment using **`driver->percent`**; **`pickAISprite`** uses **variant** (1–5) and steer vs **`targetLaneOffset`**; scale bumped by **1.25** vs road scale.

---

## 6. Collision system

| Subsystem | Location | Model |
|-----------|----------|--------|
| **Player vs AI** | `collision.c` — `checkPlayerAIOpponentCollisions` | Longitudinal distance **`deltaZ`** with **track wrap**; lateral intervals overlap; **rear** vs **front** branches; **`LONGITUDINAL_COLLISION_THRESHOLD`** (~ `SEGMENT_LENGTH * 4.75`). Uses **`computeAIScale`** vs a threshold for “side” resolution; **`easeInOut`** on **`playerX`**; AI **`isBeingPushed`** with interpolated offset. |
| **AI vs AI** | `checkAIDriverCollisions` | Pairwise; same Z threshold; lateral overlap triggers push targets. Gated early if **`totalRaceTime < 3`** or **`speed < 1`**. |
| **AI vs roadside** | `checkAIDriverBillboardCollisions` | When AI **off-road** (`|offset| > 1`); **`overlap`** on lateral axis; **`overlap`** expects **center** positions — billboard path uses **sprite center** = `offset + collisionOffsetX`. |
| **Player off-road vs props** | `game.c` — `handleOffRoadCollisions` | If **`|playerX| >= 1`** and speed above off-road limit: decel; **`overlap`** with roadside sprites can **snap position** and set **`collisionResetOccurredThisFrame`** to affect lap logic. |
| **AI ram push** | `game.c` — `updateAIAndCollisions` | If AI **`isRamming`** and overlapping player on same segment, nudge **`playerX`**. |

**`overlap(x1,w1,x2,w2,percent)`** (`util.c`): compares **|**x1 − x2**|** to **`(w1+w2)*percent/2`** — arguments are **centers** and **full widths** in the same lateral units as **`playerX` / offsets**.

**Note**: `game.h` exposes **`inCollisionPush`** but much of the collision response uses **AI push state** and **`easeInOut` on playerX**; naming may not match every code path.

---

## 7. AI system (`ai.c` / `ai.h`)

- **`AIController`**: array of **`AIDriver`**, tuned **`maxAcceleration`**, **`maxBraking`**, **`steeringSensitivity`**, **`catchupThreshold`**, **`cornerSpeedFactor`**, back-pointer to **`Game`**.
- **Integration order** (in `updateGame`): **`updateAIController`** → **`checkAIDriverCollisions`** → **`checkPlayerAIOpponentCollisions`** → ram nudge in `game.c`.
- **Motion**: **`z`** advanced with **`increase(..., trackLength)`** (wrap); **`percent`** for rendering; **targetSpeed** raised when far behind player (lap-aware distance hack).
- **Behaviors** (probabilistic FSM): **WOBBLE**, **HOLD_LINE**, **LANE_SWITCH**, **SPEEDUP**, **SLOWDOWN**, **RAMMING** (targets player or other AI on same segment within **`AI_RAM_DISTANCE_THRESHOLD`**). **`initialBehaviorDelayTimer`** defers rich behavior after start.
- **Corners**: lookahead sum of **`fabs(curve)`** reduces desired speed.
- **Off-road**: decel + **`checkAIDriverBillboardCollisions`**.
- **Push interpolation**: cubic ease; optional **speed transfer** from player; **unstuck** if speed very low and offset error large.

---

## 8. Level loading (`level_loader.c` + `level.h`)

### Portable resource paths (`paths.c`)

- **`paths_init()`** / **`paths_quit()`** — called from **`main`** (after `SDL_Init`, before `SDL_Quit`).
- **`paths_resolve(out, out_sz, rel_path)`** — finds a readable file by trying, in order:
  1. **`PSEUDO_RACER_RESOURCE_ROOT`** (environment override),
  2. **`SDL_GetBasePath()`** (executable directory),
  3. Parent of that path (helps `build/` layouts),
  4. Current working directory.
- **Absolute** `rel_path` values are copied through and only checked for existence.
- **Level file**: `initGame` passes **`DEFAULT_LEVEL_REL_PATH`** (`constants.h`, e.g. `"levels/beach_test.json"`) or **`argv[1]`** from **`main`** when provided — all resolved via **`paths_resolve`** inside **`loadLevel`** before `fopen`.
- **JSON `resources.*` paths** and **game asset loads** in **`initGame`** (e.g. `resources/…`) use the same resolver.

### JSON sections

- **`levelName`**
- **`resources`**: `backgroundTexture`, `spriteSheet`, `musicFile` — **relative paths** preferred; resolved with **`paths_resolve`** before `IMG_LoadTexture` / `Mix_LoadMUS`.
- **`grassColors`**: optional hex overrides for **`COLOR_LIGHT.grass` / `COLOR_DARK.grass`**
- **`background.cycleStates[]`**: each state has **3** `textureRects`, `offsets`, `speeds`, `verticalSpeeds`, `layerTransitionSpeeds`, `overlapMargins`, `skyColor`, `roadColor`, `name`
- **`road.commands[]`**: `{ "type", "params" }` interpreted in **`resetRoad`** (`road.c`)
- **`scenery`** (optional): roadside placement consumed into **`game->loadedScenery`** (see below). Missing or invalid type → empty scenery (logged); allocation failure fails **`loadLevel`**.

### Scenery / data-driven roadside sprites

Parsed into **`LevelSceneryData`** (`level.h`): stored on **`Game`** as **`loadedScenery`**, backed by **`levelArena`** (sprite ids resolved to **`const Sprite*`** at parse time via **`resolveScenerySpriteName`** / `SDL_strcasecmp`).

**Two JSON shapes:**

1. **Array** — shorthand list of instances only:
   - `{ "sprite": "<id>", "segment": <int>, "offset": <number> }`
   - **`segment` < 0** counts from end of track (`totalSegments + segment` before clamp).

2. **Object**:
   - **`instances`**: same objects as above.
   - **`repeatAlong`**: rules `{ "from", "to", "step", "placements": [ { "sprite", "offset" }, ... ] }` — expands along segment indices with **`from <= n < to`** (**`to`** exclusive); negative **`from` / `to`** use `totalSegments + value` like instances.

**Sprite ids**: `palm_tree_left`, `palm_tree_right`, `rock`, `umbrella`, `billboard01`–`09` (also `billboard1`–`9`).

**`resetSprites`** (`road.c`): **no hardcoded props** — it only applies **`game->loadedScenery`** (instances + expanded repeats) via **`addSprite`**. If JSON has no **`scenery`**, the track has no roadside sprites from data.

### GPU / load failure and sliding window

- **`rollbackLevelLoadGpuResources`**: if loading fails after some of **`game->background`**, **`spritesheet`**, **`music`**, or **`slidingWindow`** are set, this tears them down and nulls **`currentCycleState` / `targetCycleState`** so **`initGame`** can **`cleanupGame`** safely.
- **`Game`** owns the **`SDL_Texture`** for the shared background atlas. Each **`BackgroundCycleState.texture`** is a **non-owning** copy of that pointer.
- **`freeBackgroundResources`**: only **clears** cycle-state **`texture`** fields (does not destroy the texture). **`cleanupGame`** clears those, destroys **`game->background`**, then **`free`**s **`SlidingWindow`**.
- **`loadLevel`** frees a **previous** **`SlidingWindow`** (if any) before allocating a new one when reloading.

### Allocation

- Parsed strings and scenery metadata use **`levelArena`**. JSON file buffer uses **`malloc`** then freed after parse.

### Post-load

- Textures assigned to each cycle state’s **`texture`** pointer (**same `game->background`**); **`SlidingWindow`** allocated with **`malloc`**; **`precomputeTransitionSpeeds`**.

**Road command types** (from `resetRoad`): `straight`, `hill`, `curve`, `lowRollingHills`, `sCurves`, `bumps`, `downhillToEnd`. **`addDownhillToEnd`** is appended after commands.

---

## 9. Memory management

| Allocator | Scope | Typical contents |
|-----------|--------|------------------|
| **`frameArena`** | Reset **every frame** in `updateGame` | Intended for per-frame scratch (16 MiB); ensure any long-lived pointer does **not** point into it. |
| **`levelArena`** | Reset in **`cleanupGame`** only | Road **`Segment`** array (cap **`MAX_SEGMENTS`**), per-segment **`SpriteInstance`** arrays, **`RoadCommand`** data, **`LevelSceneryItem` / repeat-rule arrays**, duplicated level strings, **AI drivers**, traffic-light animation arrays, leaderboard entries, etc. |
| **`malloc` / `free`** | Ad hoc | **`SlidingWindow`** struct; JSON read buffer; **`cleanupGame`** frees sliding window with **`free`** and destroys SDL textures. |

**Alignment**: `arena_alloc(..., alignof(T))` used for structured allocs.

---

## 10. Important source files (quick reference)

| File | Responsibility |
|------|------------------|
| `main.c` | SDL setup, vsync renderer, logical size, main loop, shutdown. |
| `game.h` / `game.c` | `Game` layout; init/cleanup; events; update pipeline; lap finish → leaderboard + audio; horizon; traffic light completion → `RACE_STATE_RUNNING`. |
| `render.c` / `render.h` | Full frame draw order; `renderSprite` / `renderSegment` / `renderPolygon`; HUD text helper. |
| `road.c` / `road.h` | Segments, **`resetRoad`** / **`resetSprites`** (data-driven from **`loadedScenery`**), `findSegment`, `calculateElevation`. |
| `paths.c` / `paths.h` | Resource-root discovery and **`paths_resolve`**. |
| `util.c` / `util.h` | Projection, overlap, wrap, easing. |
| `constants.h` / `constants.c` | Dimensions, FOV, speeds, sprite rects, colors. |
| `level_loader.c` / `level_loader.h` | JSON → level + `game` fields. |
| `background.h`, `background.c`, `background_game.c` | Biome parallax, transitions, color interpolation. |
| `ai.c` / `ai.h` | Opponents. |
| `collision.c` / `collision.h` | Player–AI and AI–AI; billboard vs AI. |
| `ui.c` / `ui.h` | Menus. |
| `leaderboard.c` / `leaderboard.h` | Race results presentation. |
| `mem_arena.h` / `mem_arena.c` | Bump allocator. |
| `cJSON.h` | Embedded JSON parser (single header). |

---

## 11. Data flow (high level)

```
paths_resolve →  open level JSON; resolve resource paths
JSON level   →  levelArena (road commands, background, scenery descriptors)
            →  GPU: background / spritesheet / music + SlidingWindow
            →  resetRoad builds segments; resetSprites fills roadside from loadedScenery
User input  →  handleEvent → UI actions or key flags
updateGame  →  findSegment, AI, collisions, physics, laps
           →  horizon + parallax + biome transition
render      →  sliding window + projected segments + sprites
           →  HUD / UI / leaderboard overlay
```

**`position`**: player arc length along track **[0, trackLength)** with wrap via **`increase`**. **`playerZ`**: camera offset along Z used when resolving **which segment** the player “sits” on for rendering/collision.

---

## 12. Technical limitations (identifiable)

- **`extern Game game`** in `game.h` with **no** obvious single definition in-repo — actual instance is **local** in `main.c`; avoid relying on a global unless a `.c` defines it.
- **Traffic light**: `renderTrafficLight` is invoked **inside** the per-segment loop in `render()` (potential redundant draws / wrong segment coupling); segment index **16** is magic.
- **Sprite pass** loop **`n > 0`** skips the nearest indexed slice — may be intentional for clip ordering but is easy to misread.
- **Scenery JSON**: no procedural “random billboard” equivalent unless expressed as many **`instances`** / **`repeatAlong`** rules; large tracks may need tooling to author dense placement.
- **Performance**: **`DRAW_DISTANCE`** 300 × geometry + many sprites can be heavy on low-end devices; no LOD for sprites.
- **Collision**: mixed use of **segment-scale** vs **screen-scale** heuristics (`computeAIScale` magic constants); not a unified physics collider model.
- **Restart / reload**: `cleanupGame` resets **`levelArena`** and frees arena backing store — a full **`loadLevel` + `initGame`-style re-bootstrap** is not implemented after teardown; typical workflow is restart the process.

---

## 13. Inferred design goals

- **Arcade feel**: high max speed, simple controls, rubber-bandy AI catch-up, ramming, off-road penalties.
- **Visual variety**: multi-state backgrounds with **parallax** and **biome transitions** tied to track progress.
- **Race structure**: laps, countdown start, end leaderboard, win/lose stinger sounds.
- **Low dependency footprint**: SDL2 + stb-style JSON, no engine middleware.
- **Data-driven track shape** via JSON commands and **roadside scenery** via **`scenery`**, while keeping **art** in PNGs and **audio** in files.

---

## 14. Coding conventions (observed)

- **C99-style** structs, `typedef struct`, heavy use of **`double`** for simulation.
- **Prefix naming**: module-prefixed functions (`renderSprite`, `findSegment`, `updateAIDriver`).
- **Logging**: `SDL_Log` for warnings/errors.
- **Magic numbers** often local `#define` or inline (road builder, AI, collision).
- **Headers**: some use `SDL2/SDL.h`, `background.h` uses `SDL.h` — minor inconsistency.
- **Comments**: explanatory blocks in places; not uniformly strict Doxygen.

---

## 15. Engine / platform constraints

- **SDL2 renderer** with **`SDL_RenderGeometry`** for road quads (requires SDL2 builds with geometry support).
- **Logical resolution** `1024×768` scaled by fullscreen desktop mode.
- **Single-threaded**; all SDL calls from main thread.
- **Wrap-around track**: many systems must normalize **`deltaZ`** with **`trackLength/2`**.
- **Arena discipline**: anything that must survive past the current frame must use **`levelArena`** or heap/`Game` fields — not **`frameArena`**.

---

## 16. Sensible extension points

- **Level JSON**: new **`road.commands`** types; extend **`scenery`** (e.g. jitter, biome-tied sets, or sprite ids tied to atlas regions in data).
- **Paths**: extra roots (e.g. XDG data dir), manifest file listing search roots, or packaging layout conventions.
- **`DRAW_DISTANCE` / FOV / SPRITE_SCALE**: expose as per-level or global tuning (JSON or `constants`).
- **AI**: new behaviors in the FSM; cleaner separation of **decision** vs **steering**; difficulty presets via controller fields.
- **Collision**: unify lateral units; optional **hitboxes** per sprite in data; segment-accurate longitudinal tests.
- **Rendering**: batch sprites; optional depth sort key; fog using `FOG_DENSITY` constant (if wired).
- **Multi-level / hot reload**: after **`cleanupGame`**, re-run arena setup + **`loadLevel`** + **`resetRoad`** / AI re-init without `exit`, or document a single supported “restart process” flow.

---

*Generated from repository snapshot; line references may drift as code changes. When editing, grep for the symbols above rather than trusting exact line numbers.*
