/**
 * @file lib_gamepad_event_system.h
 * @brief PlayStation 2 Gamepad Event System Implementation.
 *
 * @details
 * This module implements the generic `EventSystem` interface specifically for the 
 * PlayStation 2 controller (DualShock 2) using Sony's native PS2SDK (`libpad`, `sifrpc`, and `loadfile`).
 * 
 * It initializes hardware modules (`SIO2MAN` and `PADMAN`), polls controller inputs per frame, 
 * evaluates active flags, and dispatches callbacks to registered `GamepadActionMachine` instances.
 *
 * ## Extending the Action Machine with New Buttons
 * To support additional buttons (such as face buttons or analog axes), follow these steps:
 * 1. Add a new `GamepadAction` callback field inside the `GamepadActionMachine` structure 
 *    (e.g., `GamepadAction cross_button_pressed;`).
 * 2. Update the button bitmask check in `update_pad_inputs()` using the corresponding 
 *    PS2SDK macros (e.g., `PAD_CROSS`, `PAD_SQUARE`, `PAD_TRIANGLE`, `PAD_CIRCLE`).
 * 3. Evaluate and execute the callback inside the loop just like the directional inputs.
 */

#ifndef LIB_GAMEPAD_EVENT_SYSTEM_H
#define LIB_GAMEPAD_EVENT_SYSTEM_H

#include <libpad.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <stdint.h>
#include <stdbool.h>

#include <libs/events/event.h>

/** 
 * @brief DMA-aligned buffer required by the PS2 PADMAN driver (64-byte alignment). 
 */
static int8_t padBuf[256] __attribute__((aligned(64)));

/** 
 * @brief Struct holding the raw status of the gamepad buttons from the current read cycle. 
 */
struct padButtonStatus buttons;

/**
 * @brief Initializes the PS2 RPC subsystem, loads PAD drivers, and opens the pad port.
 * 
 * @param[in,out] data Generic user context pointer (unused).
 * @return `true` upon successful initialization, `false` otherwise.
 */
static bool init_pads(void *data);

/**
 * @brief Polls the gamepad hardware state and dispatches actions to registered action machines.
 * 
 * @details Performs early returns if the controller is not in a stable state or if reading fails. 
 *          Inverts the button bitmask and evaluates active inputs (directional and face buttons) 
 *          against all registered `GamepadActionMachine` instances.
 * 
 * @param[in,out] data Generic game/system context passed directly into executed action callbacks.
 */
static void update_pad_inputs(void *data);

/**
 * @brief Registers a new gamepad action machine into the gamepad event system.
 * 
 * @param[in] actions_machine Opaque handle to the `GamepadActionMachine` to be registered.
 */
static void add_pad_actions_machine(ActionMachine actions_machine);

/**
 * @brief Function pointer prototype for gamepad-driven actions.
 * 
 * @param[in,out] context Generic user or system context passed down during execution.
 */
typedef void (*GamepadAction)(void *);

/**
 * @brief Gamepad-specific Action Machine structure.
 *
 * @details Groups related action callbacks mapped to directional and control inputs. 
 *          This structure acts as an FSM-like action container for the gamepad subsystem.
 *
 * @note To add new button actions, declare a new `GamepadAction` callback field here 
 *       and evaluate its corresponding `PAD_*` mask inside the update loop.
 */
typedef struct __tag_gamepad_action_machine__
{
    // Direction buttons
    GamepadAction left_button_pressed;  ///< Callback executed when the left directional button is active.
    GamepadAction right_button_pressed; ///< Callback executed when the right directional button is active.

    // Example of how to extend with face buttons:
    // GamepadAction cross_button_pressed;  ///< Callback executed when the Cross button is active.
    // GamepadAction square_button_pressed; ///< Callback executed when the Square button is active.
} GamepadActionMachine;

/**
 * @brief Global Gamepad EventSystem instance configuration.
 */
static EventSystem gamepad = {
    .init = init_pads,
    .update = update_pad_inputs,
    .add_actions_machine = add_pad_actions_machine,
    .action_machines_counter = 0
};

/**
 * @brief Implementation of `init_pads`.
 */
static bool init_pads(void *data)
{
    (void)data;
    SifInitRpc(0);

    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);

    padInit(0);
    padPortOpen(0, 0, padBuf);

    return true;
}

/**
 * @brief Implementation of `update_pad_inputs`.
 */
static void update_pad_inputs(void *data)
{
    // 1. Early guards: exit immediately if the controller is not ready or readable
    if (padGetState(0, 0) != PAD_STATE_STABLE)
        return;
    if (padRead(0, 0, &buttons) <= 0)
        return;

    // 2. Input processing: invert active-low bitmask from the DualShock controller
    uint32_t paddata = 0xffff ^ buttons.btns;

    for (size_t i = 0; i < gamepad.action_machines_counter; ++i)
    {
        GamepadActionMachine *machine = (GamepadActionMachine *)gamepad.action_machines[i];

        // 3. Direct execution evaluating condition and function pointer for each button

        if ((paddata & PAD_LEFT) && machine->left_button_pressed)
        {
            machine->left_button_pressed(data);
        }

        if ((paddata & PAD_RIGHT) && machine->right_button_pressed)
        {
            machine->right_button_pressed(data);
        }

        /* Example extension for face buttons:
        if ((paddata & PAD_CROSS) && machine->cross_button_pressed)
        {
            machine->cross_button_pressed(data);
        }
        */
    }
}

/**
 * @brief Implementation of `add_pad_actions_machine`.
 */
static void add_pad_actions_machine(ActionMachine actions_machine)
{
    if (gamepad.action_machines_counter < MAX_ACTION_MACHINES)
    {
        int8_t free_machine_index = gamepad.action_machines_counter;
        gamepad.action_machines[free_machine_index] = actions_machine;
        ++gamepad.action_machines_counter;
    }
}

#endif // LIB_GAMEPAD_EVENT_SYSTEM_H