# OStim Hot Swap

An OStim NG plugin that lets you add, remove, and swap actors in an ongoing scene — without ending it.

## Requirements

- [OStim NG](https://www.nexusmods.com/skyrimspecialedition/mods/40725)
- [SKSE64](https://skse.silverlock.org/)

---

## For Users

### How to Use

Start an OStim scene, then open the **OStim options menu**. You will see a **Manage Actors** submenu with three options:

### Add Actor

Pick a nearby actor to join the scene(click for preview)

![](/docs/images/add_fps_15_85.gif)


### Remove Actor

Pick an actor in the scene to leave(click for preview)

![](/docs/images/remove_fps_15_85.gif)


### Swap Actors

Pick two actors in the scene to switch positions(click for preview)

![](/docs/images/swap_fps_15_85.gif)

The scene restarts seamlessly with the updated actor lineup.

### Scene Join

If you are not currently in a scene, aim your crosshair at an NPC who **is** in an active scene and press the **OStim scene-start key**. A prompt will appear asking if you want to join their scene. Selecting **Yes** adds you to that scene.

### Notes

- Only actors physically nearby can be added.
- Some actor combinations or animation constraints may prevent certain changes. If an action is unavailable, the option will not appear or will do nothing.
- Thread ID changes(except player's thread id which is always `0`) after each modification — this is expected behavior.

---

## For Modders

OStim Hot Swap exposes a Papyrus script (`OStimHotSwap`) with native global functions you can call from any mod.

### Validation Functions

Use these before acting to check whether an operation is currently allowed.

```papyrus
; Returns true if the actor can be added to the thread.
bool Function CanAddActor(int threadID, Actor actor) native global

; Returns true if the actor at the given position can be removed.
bool Function CanRemoveActor(int threadID, int position) native global

; Returns true if the actors at posA and posB can be swapped.
bool Function CanSwapActors(int threadID, int posA, int posB) native global
```

### Action Functions

These schedule the operation asynchronously. They return `0` if successfully scheduled, or `-1` on immediate failure. The thread migration completes in the background.

```papyrus
; Adds an actor to the thread. Returns 0 if scheduled, -1 on failure.
int Function AddActorToThread(int threadID, Actor actor) native global

; Removes the actor at the given position from the thread. Returns 0 if scheduled, -1 on failure.
int Function RemoveActorFromThread(int threadID, int position) native global

; Swaps the actors at posA and posB. Returns 0 if scheduled, -1 on failure.
int Function SwapActors(int threadID, int posA, int posB) native global

; Replaces the entire actor lineup with a new ordered array. Returns 0 if scheduled, -1 on immediate failure.
int Function MigrateThread(int threadID, Actor[] actors) native global
```

> **Important:** The thread ID changes after every successful operation. Since operations are now asynchronous, the new thread ID is **not** returned directly. Use `OActor.GetThreadID()` after the migration completes to retrieve the updated thread ID.

### Helper Functions

```papyrus
; Returns an array of position indices that the given actor can be swapped with.
int[] Function GetSwapPartners(int threadID, Actor actor) native global
```

### UI Flow Functions

These open the same message box dialogs that the options menu uses. Useful if you want to trigger the managed actor UI from your own menu or hotkey.

```papyrus
; Opens a popup to select a nearby actor to add.
Function AddActorWithUI(int threadID) native global

; Opens a popup to select an actor in the thread to remove.
Function RemoveActorWithUI(int threadID) native global

; Opens a two-step popup to select two actors to swap.
Function SwapActorsWithUI(int threadID) native global
```

### Example Usage

```papyrus
import OStimHotSwap

; Get the current thread ID from OStim (however your mod tracks it)
int threadID = myCurrentThreadID

; Add an actor if allowed
if CanAddActor(threadID, someActor)
    int result = AddActorToThread(threadID, someActor)
    ; result == 0 means migration was scheduled successfully
    ; poll OActor.GetThreadID() after a short delay to get the new thread ID
endif
```

---
