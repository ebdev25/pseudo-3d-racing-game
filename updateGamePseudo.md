Reset per-frame arena allocator.
Clear collisionResetOccurredThisFrame flag.

If race is in countdown:
    Update the traffic-light animation and exit if not finished.

If player control is not allowed OR UI menu is open:
    Exit early (pause all game logic).

Compute player's sprite width.
Compute speedPercent (player speed / max speed).
Compute lateral movement increment dx.
Store starting track position for lap timing.
If race running:
    Increase totalRaceTime and currentLapTime.
Find the player's current road segment; abort if missing.

Update all AI drivers (speed, behavior, movement).
Check AI vs AI collisions.
Check AI vs player collisions.

For each AI driver:
    If AI driver is in the same segment as the player:
        If AI is ramming AND lateral overlap occurs:
            Push the player sideways according to ramming force.

Advance player along the road based on speed.
If not being collision-pushed:
    Apply left/right input to modify playerX.
Apply centrifugal force from road curvature.
Handle throttle, braking, and natural deceleration.

If playerX is outside road bounds:
    If player is going too fast:
        Apply off-road deceleration.

        For each roadside sprite in current segment:
            Compute that sprite's collision-box center.
            If sprite overlaps player:
                Reduce speed to a low fallback.
                Rewind position so player “hits” the sprite.
                Mark collisionResetOccurredThisFrame to block lap increments.

Clamp playerX and speed to allowed ranges.

If player wrapped around the start line (position < startPosition):
If at least 1 second has passed since last lap start:
Increment completed laps.
Store last lap time.
Reset currentLapTime.

    If total laps reached AND race not yet marked finished:
        Mark race as finished.
        Disable player control.
        Stop race music.
        Generate the leaderboard entries.
        Show leaderboard overlay.
        Determine whether player won.
        Play victory or lose sound accordingly.

    Reset LOD transition threshold for next lap.

    Update horizon
    Compute dynamic horizonY based on visible road height.

    If not mid-transition:
Compute elevation at player’s position.
Update parallax offsets for all layers.
Repeat parallax update using current segment again (duplicate logic block).

If player passed next LOD threshold:
Trigger transition to next background state.
Advance LOD threshold to next third of track (wrap around track length).

If a background transition is happening:

Compute progress into the transition.
Update blended sky/road colors.

Compute scroll speed based on track movement this frame.

Scroll current background layers leftward.
    Mark whether all layers are off-screen.

If current layers are fully gone:
    Scroll target background layers into view.
    If all target layers are now visible:
        Switch active background state to target.
        Select next target state cyclically.
        Reset transition flags.
        Log transition completion.