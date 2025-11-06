#include "keypad_driver.h"

// === NUEVO ===: Mapa de teclas del teclado matricial 4x4
static const char keypad_map[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// === NUEVO ===: Inicialización de pines (filas como salidas, columnas como entradas con EXTI)
void keypad_init(keypad_handle_t* keypad) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // --- Configurar FILAS como salidas ---
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    for (int i = 0; i < KEYPAD_ROWS; i++) {
        GPIO_InitStruct.Pin = keypad->row_pins[i];
        HAL_GPIO_Init(keypad->row_ports[i], &GPIO_InitStruct);
        // Mantener filas en ALTO (inactivas). Durante el escaneo se pone
        // una fila en BAJO para detectar la columna correspondiente.
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_SET);
    }
    // Nota: Los pines de columna y el NVIC deben configurarse desde MX_GPIO_Init() / CubeMX.
    // Aquí solo configuramos las FILAS para evitar duplicar o generar conflictos.
}

// === NUEVO ===: Escaneo del teclado (identificación de tecla presionada)
char keypad_scan(keypad_handle_t* keypad, uint16_t col_pin) {
    HAL_Delay(20); // debounce

    char key_pressed = '\0';

    // Identificar índice de columna a partir del pin recibido en la ISR
    int col_index = -1;
    for (int c = 0; c < KEYPAD_COLS; c++) {
        if (keypad->col_pins[c] == col_pin) {
            col_index = c;
            break;
        }
    }
    if (col_index == -1) return '\0'; // pin no válido

    // Escanear filas: las filas están inactivas en ALTO. Poner cada fila en BAJO
    // para comprobar si la columna queda en BAJO (tecla presionada).
    for (int r = 0; r < KEYPAD_ROWS; r++) {
    // Poner todas las filas en ALTO (estado inactivo)
        for (int k = 0; k < KEYPAD_ROWS; k++) {
            HAL_GPIO_WritePin(keypad->row_ports[k], keypad->row_pins[k], GPIO_PIN_SET);
        }

    // Activar la fila actual en BAJO
        HAL_GPIO_WritePin(keypad->row_ports[r], keypad->row_pins[r], GPIO_PIN_RESET);
        HAL_Delay(3);

    // Si la columna está en BAJO, la tecla (r, col_index) está presionada
        if (HAL_GPIO_ReadPin(keypad->col_ports[col_index], keypad->col_pins[col_index]) == GPIO_PIN_RESET) {
            key_pressed = keypad_map[r][col_index];

            // Restaurar filas a ALTO y esperar la liberación con timeout
            for (int k = 0; k < KEYPAD_ROWS; k++) {
                HAL_GPIO_WritePin(keypad->row_ports[k], keypad->row_pins[k], GPIO_PIN_SET);
            }

            int timeout = 500; // ms
            while (HAL_GPIO_ReadPin(keypad->col_ports[col_index], keypad->col_pins[col_index]) == GPIO_PIN_RESET && timeout-- > 0) {
                HAL_Delay(1);
            }
            break;
        }
    // No es esta fila: dejarla en ALTO y continuar
        HAL_GPIO_WritePin(keypad->row_ports[r], keypad->row_pins[r], GPIO_PIN_SET);
    }

    // Asegurar que las filas queden en ALTO al final
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_SET);
    }

    return key_pressed;
}

