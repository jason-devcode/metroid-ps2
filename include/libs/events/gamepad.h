#ifndef LIB_GAMEPAD_EVENT_SYSTEM_H
#define LIB_GAMEPAD_EVENT_SYSTEM_H

#include <libpad.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <stdint.h>
#include <stdbool.h>

#include <libs/events/event.h>

// El único buffer requerido (alineado a 64 bytes)
static int8_t padBuf[256] __attribute__((aligned(64)));
struct padButtonStatus buttons;

static bool init_pads(void *data);
static void update_pad_inputs(void *data);
static void add_pad_actions_machine(ActionMachine actions_machine);

typedef void (*GamepadAction)(void *);

typedef struct __tag_gamepad_action_machine__
{
    // Direction buttons
    GamepadAction left_button_pressed;
    GamepadAction right_button_pressed;
} GamepadActionMachine;

static EventSystem gamepad = {
    .init = init_pads,
    .update = update_pad_inputs,
    .add_actions_machine = add_pad_actions_machine,
    .action_machines_counter = 0};

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

static void update_pad_inputs(void *data)
{
    // Comprobamos directamente si el mando está listo para usarse
    if (padGetState(0, 0) == PAD_STATE_STABLE)
    {
        // Leemos el estado del mando de forma directa
        if (padRead(0, 0, &buttons) > 0)
        {
            // Combinamos los bytes y aplicamos máscara inversa
            uint32_t paddata = 0xffff ^ buttons.btns;

            for (uint8_t machine_iterator = 0; machine_iterator < gamepad.action_machines_counter; ++machine_iterator)
            {
                GamepadActionMachine *action_machine = (GamepadActionMachine *)gamepad.action_machines[machine_iterator];

                // -- CHECK DIRECTIONS
                if (paddata & PAD_LEFT)
                {
                    GamepadAction action = action_machine->left_button_pressed;
                    if (action)
                        action(data);
                }

                if (paddata & PAD_RIGHT)
                {
                    GamepadAction action = action_machine->right_button_pressed;
                    if (action)
                        action(data);
                }
            }
        }
    }
}

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