#include "keypad_driver.h"

static const char keypad_map[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

void keypad_init(keypad_handle_t* keypad) {
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_RESET);
    }
}

char keypad_scan(keypad_handle_t* keypad, uint16_t col_pin) {
    for (int row = 0; row < KEYPAD_ROWS; row++) {
        // Activar una fila
        HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_SET);

        for (int col = 0; col < KEYPAD_COLS; col++) {
            if (keypad->col_pins[col] == col_pin) {
                GPIO_PinState state = HAL_GPIO_ReadPin(keypad->col_ports[col], keypad->col_pins[col]);

                // Desactivar fila justo después de leer
                HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_RESET);

                if (state == GPIO_PIN_SET) {
                    return keypad_map[row][col];
                }
            }
        }

        // Desactivar fila si no coincide
        HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_RESET);
    }
    return '\0';
}