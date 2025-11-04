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
// === NUEVO ===: Escaneo del teclado (identificación de tecla presionada)
char keypad_scan(keypad_handle_t* keypad, uint16_t col_pin) {
    // NO HAL_Delay(5) AQUÍ SI SE LLAMA DESDE UNA ISR O UN TIMER ISR
    // El debouncing se gestionaría ANTES de llamar a esta función.

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

    // --- Poner todas las filas en ALTO una vez antes de escanear ---
    for (int k = 0; k < KEYPAD_ROWS; k++) {
        HAL_GPIO_WritePin(keypad->row_ports[k], keypad->row_pins[k], GPIO_PIN_SET);
    }

    // --- Escanear filas ---
    for (int r = 0; r < KEYPAD_ROWS; r++) {
        // Activar la fila actual en BAJO
        HAL_GPIO_WritePin(keypad->row_ports[r], keypad->row_pins[r], GPIO_PIN_RESET);
        // Pequeño retardo si es absolutamente necesario, pero idealmente se evita en contexto de ISR
        // HAL_Delay(1); // Considera eliminar si se llama desde un timer ISR y el tiempo es crítico.

        // Si la columna sigue en bajo, esa es la tecla presionada
        if (HAL_GPIO_ReadPin(keypad->col_ports[col_index], keypad->col_pins[col_index]) == GPIO_PIN_RESET) {
            key_pressed = keypad_map[r][col_index];
            // NO HAGAS EL BLOQUEO 'while' AQUÍ.
            // La detección de la liberación de la tecla y el anti-rebote de la liberación
            // se manejarían en el bucle principal o con otro timer/flag.
            break; // Se encontró la tecla, salimos del bucle de filas
        }

        // Restaurar la fila actual en ALTO antes de pasar a la siguiente
        // Esto es crucial para un escaneo correcto
        HAL_GPIO_WritePin(keypad->row_ports[r], keypad->row_pins[r], GPIO_PIN_SET);
    }

    // Ya que el bucle `for (r...)` ha restaurado las filas una por una,
    // este bucle final no es estrictamente necesario si el bucle interno lo hace bien.
    // Pero es una buena medida de seguridad para garantizar que todas las filas queden en ALTO.
    // Opcional: Podría ser eliminado si el bucle interno maneja la restauración de cada fila.
    // Dejarlo aquí como precaución final.
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_SET);
    }

    return key_pressed;
}
