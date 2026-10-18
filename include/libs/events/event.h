#ifndef LIB_EVENT_SYSTEM_MANAGEMENT_H
#define LIB_EVENT_SYSTEM_MANAGEMENT_H

/**
 * Problema: Capturar y reaccionar a los eventos del control del PS2.
 * 
 * Elementos identificables:
 * 
 * 1. Events - Eventos.
 * 2. Listeners - Escuchadores.
 * 3. Actions - Acciones.
 * 4. EventContext - Contexto del evento. 
*/

#define MAX_ACTION_MACHINES 1

typedef void* ActionMachine;

typedef struct __tag_event_system_structure__
{
    bool (*init  )( void* data );
    void (*update)( void* data );
    void (*add_actions_machine)( void* data );

    size_t action_machines_counter;
    ActionMachine action_machines[ MAX_ACTION_MACHINES ];
} EventSystem;

#endif // LIB_EVENT_SYSTEM_MANAGEMENT_H