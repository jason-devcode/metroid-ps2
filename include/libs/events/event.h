/**
 * @file event.h
 * @brief Generic Event System Architecture based on Action Machines.
 *
 * @details
 * ## Core Concept & Philosophical Overview
 * The Event System provides a lightweight, modular interface to capture hardware 
 * or software events and dispatch them to specific handlers. It is designed around 
 * the concept of **Action Machines**—an abstraction analogous to Finite State Machines (FSMs).
 *
 * Rather than tying input checking directly to game logic or holding monolithic switch/if statements, 
 * an **Action Machine** acts as a stateful or context-aware container of function pointers 
 * (callbacks). Depending on the current context (e.g., Gameplay State, Pause Menu, Inventory), 
 * a specific Action Machine is registered with the Event System to map raw input flags directly 
 * to actionable behaviors.
 *
 * ### Architectural Components:
 * 1. **Events**: Hardware or system triggers (e.g., button presses, timer ticks, network packets).
 * 2. **Listeners**: The polling/update loop inside `EventSystem::update` that checks for active events.
 * 3. **Actions**: Function pointers (`GamepadAction`, `CustomAction`) executed when an event criteria is met.
 * 4. **EventContext**: The generic data payload (`void* data`) passed down to action callbacks.
 * 5. **Action Machine**: A struct grouping related action callbacks representing a specific behavioral state.
 */

#ifndef LIB_EVENT_SYSTEM_MANAGEMENT_H
#define LIB_EVENT_SYSTEM_MANAGEMENT_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Maximum number of Action Machines that can be attached to an Event System.
 * 
 * @note Adjust this limit according to your hardware memory constraints 
 *       or system requirements.
 */
#define MAX_ACTION_MACHINES 1

/**
 * @brief Opaque handle representing an Action Machine.
 *
 * @details An Action Machine is a pointer to a domain-specific structure containing 
 *          callback function pointers (e.g., `GamepadActionMachine*`). Using an opaque 
 *          `void*` handle allows the generic `EventSystem` to remain decoupled from 
 *          specific action machine layouts.
 */
typedef void* ActionMachine;

/**
 * @brief Generic Event System Interface Structure.
 *
 * @details Defines the lifecycle and registration API for any event system implementation 
 *          (e.g., Gamepad, Keyboard, Network, AI state triggers).
 */
typedef struct __tag_event_system_structure__
{
    /**
     * @brief Initializes low-level hardware modules, RPCs, or software drivers.
     * 
     * @param[in,out] data Generic initialization context or system data pointer.
     * @return `true` if initialization succeeded, `false` otherwise.
     */
    bool (*init)( void* data );

    /**
     * @brief Polls events and dispatches callbacks to registered Action Machines.
     * 
     * @details This method should be called every frame or game loop iteration. It checks 
     *          hardware/system status, reads raw state data, iterates through attached 
     *          `action_machines`, and invokes non-NULL action callbacks.
     * 
     * @param[in,out] data Generic game/system context passed directly to triggered action callbacks.
     */
    void (*update)( void* data );

    /**
     * @brief Registers an Action Machine into the system's dispatch table.
     * 
     * @param[in] actions_machine Pointer to the specific Action Machine structure.
     */
    void (*add_actions_machine)( ActionMachine actions_machine );

    /**
     * @brief Current count of registered Action Machines in the system.
     */
    size_t action_machines_counter;

    /**
     * @brief Fixed array holding registered Action Machine pointers.
     */
    ActionMachine action_machines[ MAX_ACTION_MACHINES ];
} EventSystem;

#endif // LIB_EVENT_SYSTEM_MANAGEMENT_H