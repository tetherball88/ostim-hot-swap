ScriptName OStimHotSwap

; -- Validation --

; Returns true if the actor can be added to the thread.
bool Function CanAddActor(int threadID, Actor actor) native global

; Returns true if the actor at the given position can be removed from the thread.
bool Function CanRemoveActor(int threadID, int position) native global

; Returns true if the actors at posA and posB can be swapped in the thread.
bool Function CanSwapActors(int threadID, int posA, int posB) native global

; -- Actions --

; Adds the actor to the thread. Returns the new thread ID on success, or -1 on failure.
int Function AddActorToThread(int threadID, Actor actor) native global

; Removes the actor at the given position from the thread. Returns the new thread ID on success, or -1 on failure.
int Function RemoveActorFromThread(int threadID, int position) native global

; Swaps the actors at posA and posB in the thread. Returns the new thread ID on success, or -1 on failure.
int Function SwapActors(int threadID, int posA, int posB) native global

; Returns positions of all valid swap partners for the given actor in the thread.
int[] Function GetSwapPartners(int threadID, Actor actor) native global

; -- UI flows --

; Opens a message box to select an actor nearby to add to the thread.
Function AddActorWithUI(int threadID) native global

; Opens a message box to select an actor in the thread to remove.
Function RemoveActorWithUI(int threadID) native global

; Opens a two-step message box to select two actors in the thread to swap.
Function SwapActorsWithUI(int threadID) native global
