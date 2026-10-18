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
 * ## Core Architectural Concepts
 * 1. **Events**: Hardware triggers from the DualShock 2 controller (button presses, stick displacements).
 * 2. **Listeners**: The polling/update loop inside `update_pad_inputs` that checks controller states every frame.
 * 3. **Actions**: Uniform callback functions (`GamepadAction`) executed when specific event criteria are met.
 * 4. **GamepadContext**: An autonomous state container that holds analog stick axes, button bitmasks, 
 *    and a generic `extra_data` payload to securely pass custom user contexts without unsafe assumptions.
 * 5. **Action Machine**: A structure grouping related action callbacks representing a specific behavioral state (FSM-like).
 *
 * ## Extending the Action Machine with New Inputs
 * To support additional inputs, follow these steps:
 * 1. Add a new `GamepadAction` callback field inside the `GamepadActionMachine` structure 
 *    (e.g., `GamepadAction cross_button_pressed;`).
 * 2. Update the button bitmask check in `update_pad_inputs()` using the corresponding 
 *    PS2SDK macros (e.g., `PAD_CROSS`, `PAD_SQUARE`, `PAD_TRIANGLE`, `PAD_CIRCLE`).
 * 3. Evaluate and execute the callback inside the loop, passing the `GamepadContext` pointer.
 */

#ifndef LIB_GAMEPAD_EVENT_SYSTEM_H
#define LIB_GAMEPAD_EVENT_SYSTEM_H

#include <libpad.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

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
 * @brief Bitmask representing all 16 digital buttons available on the DualShock 2 controller.
 * @details Used to invert the active-low hardware bitmask provided by the PS2 controller.
 */
#define PAD_BUTTON_MASK_ALL 0xFFFF

/** 
 * @brief Neutral center position value for PlayStation 2 analog sticks.
 */
#define ANALOG_STICK_CENTER 128

/** 
 * @brief Minimum displacement threshold from the center required to trigger an analog stick action.
 */
#define ANALOG_STICK_DEADZONE 10

/**
 * @brief Gamepad Context Structure.
 *
 * @details Encapsulates autonomous gamepad state (analog axes and digital button bitmask) 
 *          alongside an `extra_data` field to securely forward user payloads to action callbacks.
 */
typedef struct __tag_gamepad_context__ {
    uint8_t ljoy_h;           ///< Left Analog Stick Horizontal Axis (range: 0 to 255, centered at 128)
    uint8_t ljoy_v;           ///< Left Analog Stick Vertical Axis (range: 0 to 255, centered at 128)
    uint8_t rjoy_h;           ///< Right Analog Stick Horizontal Axis (range: 0 to 255, centered at 128)
    uint8_t rjoy_v;           ///< Right Analog Stick Vertical Axis (range: 0 to 255, centered at 128)
    uint32_t current_buttons; ///< Processed active-high button bitmask representing currently pressed buttons
    void *extra_data;         ///< User-defined payload forwarded securely from the update call
} GamepadContext;

/**
 * @brief Global autonomous gamepad context instance.
 */
static GamepadContext internal_gamepad_context = {0};

/**
 * @brief Retrieves a pointer to the global gamepad context.
 * 
 * @return GamepadContext* Pointer to `internal_gamepad_context`.
 */
static inline GamepadContext* gamepad_get_context(void) {
    return &internal_gamepad_context;
}

/**
 * @brief Initializes the PS2 RPC subsystem, loads PAD drivers, and opens the pad port.
 * 
 * @param[in,out] data Generic user context pointer (unused).
 * @return `true` if initialization succeeded, `false` otherwise.
 */
static bool init_pads(void *data);

/**
 * @brief Polls the gamepad hardware state and dispatches actions to registered action machines.
 * 
 * @details Performs early returns if the controller is not in a stable state or if reading fails. 
 *          Updates internal analog axes and button states, assigns the incoming `data` pointer 
 *          to `internal_gamepad_context.extra_data`, and evaluates all digital and analog triggers.
 * 
 * @param[in,out] data External user data payload (safely encapsulated into `GamepadContext::extra_data`).
 */
static void update_pad_inputs(void *data);

/**
 * @brief Registers a new gamepad action machine into the gamepad event system.
 * 
 * @param[in] actions_machine Opaque handle to the `GamepadActionMachine` to be registered.
 */
static void add_pad_actions_machine(ActionMachine actions_machine);

/**
 * @brief Uniform function pointer prototype for all gamepad-driven actions.
 * 
 * @param[in,out] context Pointer to `GamepadContext` containing axes, bitmask, and `extra_data`.
 */
typedef void (*GamepadAction)(void *);

/**
 * @brief Gamepad-specific Action Machine structure containing all controller inputs.
 *
 * @details Groups related action callbacks mapped to directional pad, face buttons, 
 *          shoulder triggers, system buttons, L3/R3 stick clicks, and analog stick movements.
 */
typedef struct __tag_gamepad_action_machine__
{
    // --- Directional Pad (D-Pad) ---
    GamepadAction left_button_pressed;    ///< Callback executed when D-Pad Left is active.
    GamepadAction right_button_pressed;   ///< Callback executed when D-Pad Right is active.
    GamepadAction up_button_pressed;      ///< Callback executed when D-Pad Up is active.
    GamepadAction down_button_pressed;    ///< Callback executed when D-Pad Down is active.

    // --- Face Buttons ---
    GamepadAction cross_button_pressed;   ///< Callback executed when Cross (X) is active.
    GamepadAction square_button_pressed;  ///< Callback executed when Square ([]) is active.
    GamepadAction circle_button_pressed;  ///< Callback executed when Circle (O) is active.
    GamepadAction triangle_button_pressed;///< Callback executed when Triangle (/\) is active.

    // --- Shoulder / Trigger Buttons ---
    GamepadAction l1_button_pressed;      ///< Callback executed when L1 Shoulder is active.
    GamepadAction l2_button_pressed;      ///< Callback executed when L2 Trigger is active.
    GamepadAction r1_button_pressed;      ///< Callback executed when R1 Shoulder is active.
    GamepadAction r2_button_pressed;      ///< Callback executed when R2 Trigger is active.

    // --- System Buttons ---
    GamepadAction start_button_pressed;   ///< Callback executed when Start Button is active.
    GamepadAction select_button_pressed;  ///< Callback executed when Select Button is active.

    // --- Analog Stick Clicks (L3 / R3) ---
    GamepadAction l3_button_pressed;      ///< Callback executed when Left Analog Stick Click is active.
    GamepadAction r3_button_pressed;      ///< Callback executed when Right Analog Stick Click is active.

    // --- Analog Stick Actions ---
    GamepadAction left_stick_moved;       ///< Callback executed when Left Stick moves beyond deadzone threshold.
    GamepadAction right_stick_moved;      ///< Callback executed when Right Stick moves beyond deadzone threshold.

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

    // 2. Populate autonomous context state and securely wrap external user payload in extra_data
    internal_gamepad_context.ljoy_h          = buttons.ljoy_h;
    internal_gamepad_context.ljoy_v          = buttons.ljoy_v;
    internal_gamepad_context.rjoy_h          = buttons.rjoy_h;
    internal_gamepad_context.rjoy_v          = buttons.rjoy_v;
    internal_gamepad_context.current_buttons = PAD_BUTTON_MASK_ALL ^ buttons.btns;
    internal_gamepad_context.extra_data      = data;

    uint32_t paddata = internal_gamepad_context.current_buttons;

    // 3. Iterate and dispatch actions to registered action machines
    for (size_t i = 0; i < gamepad.action_machines_counter; ++i)
    {
        GamepadActionMachine *machine = (GamepadActionMachine *)gamepad.action_machines[i];

        // --- Digital Buttons Evaluation ---
        if ((paddata & PAD_LEFT) && machine->left_button_pressed)
            machine->left_button_pressed(&internal_gamepad_context);
        if ((paddata & PAD_RIGHT) && machine->right_button_pressed)
            machine->right_button_pressed(&internal_gamepad_context);
        if ((paddata & PAD_UP) && machine->up_button_pressed)
            machine->up_button_pressed(&internal_gamepad_context);
        if ((paddata & PAD_DOWN) && machine->down_button_pressed)
            machine->down_button_pressed(&internal_gamepad_context);

        if ((paddata & PAD_CROSS) && machine->cross_button_pressed)
            machine->cross_button_pressed(&internal_gamepad_context);
        if ((paddata & PAD_SQUARE) && machine->square_button_pressed)
            machine->square_button_pressed(&internal_gamepad_context);
        if ((paddata & PAD_CIRCLE) && machine->circle_button_pressed)
            machine->circle_button_pressed(&internal_gamepad_context);
        if ((paddata & PAD_TRIANGLE) && machine->triangle_button_pressed)
            machine->triangle_button_pressed(&internal_gamepad_context);

        if ((paddata & PAD_L1) && machine->l1_button_pressed)
            machine->l1_button_pressed(&internal_gamepad_context);
        if ((paddata & PAD_L2) && machine->l2_button_pressed)
            machine->l2_button_pressed(&internal_gamepad_context);
        if ((paddata & PAD_R1) && machine->r1_button_pressed)
            machine->r1_button_pressed(&internal_gamepad_context);
        if ((paddata & PAD_R2) && machine->r2_button_pressed)
            machine->r2_button_pressed(&internal_gamepad_context);

        if ((paddata & PAD_START) && machine->start_button_pressed)
            machine->start_button_pressed(&internal_gamepad_context);
        if ((paddata & PAD_SELECT) && machine->select_button_pressed)
            machine->select_button_pressed(&internal_gamepad_context);

        if ((paddata & PAD_L3) && machine->l3_button_pressed)
            machine->l3_button_pressed(&internal_gamepad_context);
        if ((paddata & PAD_R3) && machine->r3_button_pressed)
            machine->r3_button_pressed(&internal_gamepad_context);

        // --- Analog Sticks Evaluation (using deadzone and center constants) ---
        if (machine->left_stick_moved && 
            (abs((int)internal_gamepad_context.ljoy_h - ANALOG_STICK_CENTER) > ANALOG_STICK_DEADZONE || 
             abs((int)internal_gamepad_context.ljoy_v - ANALOG_STICK_CENTER) > ANALOG_STICK_DEADZONE))
        {
            machine->left_stick_moved(&internal_gamepad_context);
        }

        if (machine->right_stick_moved && 
            (abs((int)internal_gamepad_context.rjoy_h - ANALOG_STICK_CENTER) > ANALOG_STICK_DEADZONE || 
             abs((int)internal_gamepad_context.rjoy_v - ANALOG_STICK_CENTER) > ANALOG_STICK_DEADZONE))
        {
            machine->right_stick_moved(&internal_gamepad_context);
        }
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