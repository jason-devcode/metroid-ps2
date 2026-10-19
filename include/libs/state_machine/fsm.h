/**
 * @file finite_state_machine.h
 * @brief A lightweight, header-only Finite State Machine (FSM) library for C.
 * @author 
 * @date 2026
 *
 * @details 
 * This library provides a clean, dynamic-allocation-free finite state machine implementation
 * tailored for embedded systems and retro consoles (such as PS2SDK). It supports up to a 
 * fixed maximum number of states, individual lifecycle hooks (`enter`, `step`, `close`), 
 * and a custom user context pointer for state data encapsulation.
 *
 * ## Usage Examples
 *
 * ### Example 1: Basic Initialization and State Execution
 * @code{.c}
 * #include "finite_state_machine.h"
 * #include <stdio.h>
 *
 * enum { STATE_IDLE = 0, STATE_RUN };
 *
 * void idle_enter(FSM *fsm) { printf("Entered Idle\n"); }
 * void idle_step(FSM *fsm)  { 
 *     // Switch to run state conditionally
 *     fsm_change(fsm, STATE_RUN); 
 * }
 *
 * void run_enter(FSM *fsm) { printf("Entered Run\n"); }
 * void run_step(FSM *fsm)  { printf("Running...\n"); }
 *
 * int main(void) {
 *     FSM my_fsm = {0};
 *
 *     // Setup states
 *     my_fsm.states[STATE_IDLE] = (StateFSM){ .enter = idle_enter, .step = idle_step };
 *     my_fsm.states[STATE_RUN]  = (StateFSM){ .enter = run_enter,  .step = run_step  };
 *
 *     // Start machine at STATE_IDLE
 *     fsm_change(&my_fsm, STATE_IDLE);
 *
 *     // Main loop simulation
 *     for (int i = 0; i < 3; i++) {
 *         fsm_step(&my_fsm);
 *     }
 *     return 0;
 * }
 * @endcode
 *
 * ### Example 2: Using Context to Share State Data
 * @code{.c}
 * typedef struct {
 *     int health;
 * } Player;
 *
 * void attack_step(FSM *fsm) {
 *     Player *p = (Player*)fsm->context;
 *     printf("Player health: %d\n", p->health);
 * }
 *
 * int main(void) {
 *     Player player_data = { .health = 100 };
 *     FSM fsm = { .context = &player_data };
 *     // ... configure and run steps ...
 * }
 * @endcode
 */

#ifndef FINITE_STATE_MACHINE_H
#define FINITE_STATE_MACHINE_H

#include <stdint.h>

/** 
 * @brief Maximum number of states allowed per FSM instance. 
 */
#define MAX_STATES_PER_MACHINE 16

/**
 * @brief Identifier type for states within the machine.
 */
typedef int8_t StateID;

/**
 * @brief Generic pointer type for passing custom context data into states.
 */
typedef void* StateContext;

// Forward declaration so StateAction can reference the FSM type
typedef struct __tag_finite_state_machine_struct__ FiniteStateMachine, FSM;

/**
 * @brief Function pointer signature for FSM state lifecycle callbacks.
 * @param fsm Pointer to the active FSM instance, allowing access to context and state changes.
 */
typedef void (*StateAction)(FSM *fsm);

/**
 * @brief Structure representing an individual state and its lifecycle hooks.
 */
typedef struct __tag_fsm_state_struct__
{
    StateAction enter; /**< Called once when transitioning into this state. */
    StateAction step;  /**< Called repeatedly during the main loop while in this state. */
    StateAction close; /**< Called once when transitioning out of this state. */
} StateFSM;

/**
 * @brief Structure representing the Finite State Machine container.
 */
struct __tag_finite_state_machine_struct__
{
    StateFSM *current_state;               /**< Pointer to the currently active state. */
    StateFSM *previous_state;              /**< Pointer to the previously active state. */
    StateFSM states[MAX_STATES_PER_MACHINE]; /**< Fixed array of registered states. */
    StateContext context;                  /**< User-defined context data pointer. */
};

/**
 * @brief Registers a state into the FSM using a StateFSM structure passed by value.
 * 
 * @param fsm Pointer to the FSM instance.
 * @param id The StateID where the state will be registered.
 * @param state StateFSM structure containing the lifecycle hooks (passed by copy).
 * @return int Returns 0 on success, or -1 if parameters are invalid.
 */
static inline int fsm_register(FSM *fsm, StateID id, StateFSM state)
{
    if (!fsm || id < 0 || id >= MAX_STATES_PER_MACHINE)
        return -1;

    fsm->states[id] = state;
    return 0;
}

/**
 * @brief Changes the active state of the FSM, executing lifecycle functions.
 * 
 * @details This function invokes the `close` method of the current state (if present),
 * updates the internal pointers, and then invokes the `enter` method of the new state 
 * (if present).
 * 
 * @param fsm Pointer to the FSM instance.
 * @param next_state The Target StateID to switch to.
 */
static inline void fsm_change(FSM *fsm, StateID next_state)
{
    if (!fsm || next_state < 0 || next_state >= MAX_STATES_PER_MACHINE)
        return;

    // 1. Close current state before exiting
    if (fsm->current_state && fsm->current_state->close)
        fsm->current_state->close(fsm);

    // 2. Switch to the new state
    fsm->previous_state = fsm->current_state;
    fsm->current_state = &fsm->states[next_state];

    // 3. Execute entry method for the new state
    if (fsm->current_state && fsm->current_state->enter)
        fsm->current_state->enter(fsm);
}

/**
 * @brief Executes the `step` function of the current active state.
 * 
 * @param fsm Pointer to the FSM instance.
 */
static inline void fsm_step(FSM *fsm)
{
    if (!fsm || !fsm->current_state)
        return;

    if (fsm->current_state->step)
        fsm->current_state->step(fsm);
}

#endif // FINITE_STATE_MACHINE_H