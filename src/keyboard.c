#include "keyboard.h"

#include <stdio.h>
#include <string.h>

void keyboard_poll(KeyboardState* keyboard, retro_input_state_t input_state)
{
    unsigned key = 0;
    keyboard->held_count = 0;
    for (key = 1; key < RETROK_LAST; key++)
    {
        keyboard->held[key] = input_state(0, RETRO_DEVICE_KEYBOARD, 0, key) != 0;
        if (keyboard->held[key])
        {
            keyboard->held_count++;
            keyboard->polled_any = true;
        }
    }
}

void keyboard_log_event(KeyboardState* keyboard, bool down, unsigned keycode, uint32_t character, uint16_t modifiers)
{
    unsigned i = 0;
    unsigned kept = keyboard->log_count < KEYBOARD_LOG_SIZE ? keyboard->log_count : KEYBOARD_LOG_SIZE - 1;
    for (i = kept; i > 0; i--)
    {
        keyboard->log[i] = keyboard->log[i - 1];
    }
    keyboard->log[0].down = down;
    keyboard->log[0].keycode = keycode;
    keyboard->log[0].character = character;
    keyboard->log[0].modifiers = modifiers;
    keyboard->log_count = kept + 1;
    keyboard->events++;
}

const char* keyboard_mode_name(KeyboardMode mode)
{
    return mode == KEYBOARD_POLLED ? "Polled (RETRO_DEVICE_KEYBOARD)" : "Callback (SET_KEYBOARD_CALLBACK)";
}

static const char* special_name(unsigned keycode)
{
    switch (keycode)
    {
        case RETROK_BACKSPACE:   return "Backspace";
        case RETROK_TAB:         return "Tab";
        case RETROK_CLEAR:       return "Clear";
        case RETROK_RETURN:      return "Return";
        case RETROK_PAUSE:       return "Pause";
        case RETROK_ESCAPE:      return "Escape";
        case RETROK_SPACE:       return "Space";
        case RETROK_DELETE:      return "Delete";
        case RETROK_KP_PERIOD:   return "Keypad .";
        case RETROK_KP_DIVIDE:   return "Keypad /";
        case RETROK_KP_MULTIPLY: return "Keypad *";
        case RETROK_KP_MINUS:    return "Keypad -";
        case RETROK_KP_PLUS:     return "Keypad +";
        case RETROK_KP_ENTER:    return "Keypad Enter";
        case RETROK_KP_EQUALS:   return "Keypad =";
        case RETROK_UP:          return "Up";
        case RETROK_DOWN:        return "Down";
        case RETROK_RIGHT:       return "Right";
        case RETROK_LEFT:        return "Left";
        case RETROK_INSERT:      return "Insert";
        case RETROK_HOME:        return "Home";
        case RETROK_END:         return "End";
        case RETROK_PAGEUP:      return "Page Up";
        case RETROK_PAGEDOWN:    return "Page Down";
        case RETROK_NUMLOCK:     return "Num Lock";
        case RETROK_CAPSLOCK:    return "Caps Lock";
        case RETROK_SCROLLOCK:   return "Scroll Lock";
        case RETROK_RSHIFT:      return "Right Shift";
        case RETROK_LSHIFT:      return "Left Shift";
        case RETROK_RCTRL:       return "Right Ctrl";
        case RETROK_LCTRL:       return "Left Ctrl";
        case RETROK_RALT:        return "Right Alt";
        case RETROK_LALT:        return "Left Alt";
        case RETROK_RMETA:       return "Right Meta";
        case RETROK_LMETA:       return "Left Meta";
        case RETROK_LSUPER:      return "Left Super";
        case RETROK_RSUPER:      return "Right Super";
        case RETROK_MODE:        return "Mode";
        case RETROK_COMPOSE:     return "Compose";
        case RETROK_HELP:        return "Help";
        case RETROK_PRINT:       return "Print";
        case RETROK_SYSREQ:      return "SysRq";
        case RETROK_BREAK:       return "Break";
        case RETROK_MENU:        return "Menu";
        case RETROK_POWER:       return "Power";
        case RETROK_EURO:        return "Euro";
        case RETROK_UNDO:        return "Undo";
        case RETROK_OEM_102:     return "OEM 102";
        default:                 return NULL;
    }
}

void keyboard_key_name(unsigned keycode, char* out, unsigned out_size)
{
    const char* special = special_name(keycode);
    if (special != NULL)
    {
        snprintf(out, out_size, "%s", special);
    }
    else if (keycode >= RETROK_a && keycode <= RETROK_z)
    {
        snprintf(out, out_size, "%c", (char)('A' + (keycode - RETROK_a)));
    }
    else if (keycode > RETROK_SPACE && keycode < RETROK_DELETE)
    {
        snprintf(out, out_size, "%c", (char)keycode);
    }
    else if (keycode >= RETROK_KP0 && keycode <= RETROK_KP9)
    {
        snprintf(out, out_size, "Keypad %u", keycode - RETROK_KP0);
    }
    else if (keycode >= RETROK_F1 && keycode <= RETROK_F15)
    {
        snprintf(out, out_size, "F%u", keycode - RETROK_F1 + 1);
    }
    else
    {
        snprintf(out, out_size, "Key %u", keycode);
    }
}
