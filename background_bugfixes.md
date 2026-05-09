Background Transition Rendering Bugs – Root Cause Analysis & Fix Summary
Context

The background system uses a sliding window of BackgroundCycleState objects with per-layer offsets, wrapping, and transition logic. During transitions, both the outgoing and incoming states may be rendered simultaneously. A visual bug occurred where background layers overlapped incorrectly starting from the second transition onward (e.g. Beach Ruins → Beach Palms), despite the first transition appearing correct.

Bug 1: Transition-only state (extraSlice[]) leaked across transitions
Symptom

Background layers overlapped or duplicated during later transitions.

Bug only appeared on the second or later transition, not the first.

Root Cause

extraSlice[], which tracks the width of an additional wrapped segment during transition unwrapping, was:

Written during startBackgroundTransition() when two segments were visible

Never reset after the transition completed

Since BackgroundCycleState structs are reused, stale extraSlice[] values persisted into subsequent transitions, causing the renderer to believe multiple segments were still visible.

Fix

Explicitly reset extraSlice[]:

When a target state is prepared at transition start

When a transition completes and the new state becomes current

// Reset on transition start (target)
for (int i = 0; i < 3; i++) {
    target->extraSlice[i] = 0.0f;
}

// Reset on transition finalize (current)
for (int i = 0; i < 3; i++) {
    game->currentCycleState->extraSlice[i] = 0.0f;
}

Result

No stale transition state persists between transitions

Second and later transitions behave identically to the first

Bug 2: Target background state was not fully positioned off-screen
Symptom

Incoming background layers sometimes appeared partially visible or overlapped for a frame during transitions.

Root Cause

setupTargetStateUnwrapped() positioned the target state at screenWidth, which assumes the texture width is irrelevant.

If the texture was wider than the screen, part of it could already be visible.

Fix

Position the target state fully off-screen by including texture width:

target->offsets[i].x = screenWidth + target->textureRects[i].w;

Result

Incoming background states are guaranteed to start completely off-screen

No premature visibility or overlap at transition start

Bug 3: Mixed integer and floating-point math during transition scrolling
Symptom

Occasional jitter, misalignment, or incorrect off-screen detection during transitions.

Root Cause

Background scrolling during transitions used:

offsets[i].x -= (int)backgroundScrollSpeed;


Elsewhere, offsets were treated as floating-point values.

Casting to int caused truncation, breaking consistency with:

Transition progress calculations

Off-screen detection logic using totalW

Fix

Use floating-point math consistently:

offsets[i].x -= (float)backgroundScrollSpeed;

Result

Smooth, deterministic scrolling

Off-screen checks behave consistently across frames

Bug 4: Incorrect “unwrap current state” offset assignment
Symptom

Subtle visual instability and incorrect segment alignment during transition unwrapping.

Root Cause

During unwrapping, the code attempted to keep “Segment A untouched” but assigned:

offsets[i].x = -oldX;


This does not preserve screen-space position and depends on the previous offset value, which may not be aligned to a wrap boundary.

Fix

Assign the computed unwrapped offset directly:

offsets[i].x = unwrappedOffset;

Result

Deterministic and correct alignment of the outgoing background state

Cleaner separation between wrap math and rendering intent

Bug 5: Redundant and confusing target setup logic
Symptom

Transition code redundantly looped while also calling a function that itself looped over layers.

Root Cause

setupTargetStateUnwrapped() already handled all layers internally.

The caller additionally looped over layers, causing unnecessary repeated work and making intent unclear.

Fix

Centralised the loop inside setupTargetStateUnwrapped() and called it once:

setupTargetStateUnwrapped(game->targetCycleState, game->width);

Result

Clear ownership of responsibility

Less fragile code and easier future refactoring

Net Outcome

After applying these fixes:

✅ No background overlap or duplication

✅ Second and later transitions behave correctly

✅ Transition state is fully reset and deterministic

✅ Rendering logic is stable and safe to tune further

✅ Background system is now lifecycle-correct rather than “first-run correct”
