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
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_SET); // alto = inactivo
    }

    // --- Configurar COLUMNAS como entradas con interrupción por flanco de bajada ---
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;

    for (int i = 0; i < KEYPAD_COLS; i++) {
        GPIO_InitStruct.Pin = keypad->col_pins[i];
        HAL_GPIO_Init(keypad->col_ports[i], &GPIO_InitStruct);
    }

    // --- Habilitar interrupciones NVIC ---
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
}

// === NUEVO ===: Escaneo del teclado (identificación de tecla presionada)
char keypad_scan(keypad_handle_t* keypad, uint16_t col_pin) {
    HAL_Delay(5); // Anti-rebote básico

    char key_pressed = '\0';

    // --- Identificar columna que generó la interrupción ---
    int col_index = -1;
    for (int c = 0; c < KEYPAD_COLS; c++) {
        if (keypad->col_pins[c] == col_pin) {
            col_index = c;
            break;
        }
    }
    if (col_index == -1)
        return '\0'; // Pin no válido

    // --- Escanear filas ---
    for (int r = 0; r < KEYPAD_ROWS; r++) {
        // Poner todas las filas en ALTO antes de iniciar
        for (int k = 0; k < KEYPAD_ROWS; k++) {
            HAL_GPIO_WritePin(keypad->row_ports[k], keypad->row_pins[k], GPIO_PIN_SET);
        }

        // Activar la fila actual en BAJO
        HAL_GPIO_WritePin(keypad->row_ports[r], keypad->row_pins[r], GPIO_PIN_RESET);
        HAL_Delay(1);

        // Si la columna sigue en bajo, esa es la tecla presionada
        if (HAL_GPIO_ReadPin(keypad->col_ports[col_index], keypad->col_pins[col_index]) == GPIO_PIN_RESET) {
            key_pressed = keypad_map[r][col_index];

            // Esperar a que se suelte la tecla
            while (HAL_GPIO_ReadPin(keypad->col_ports[col_index], keypad->col_pins[col_index]) == GPIO_PIN_RESET);
            break;
        }
    }

    // Restaurar filas en ALTO
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_SET);
    }

    return key_pressed;
}
