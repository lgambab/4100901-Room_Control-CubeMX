#include "keypad_driver.h"

/**
 * @brief Mapa de caracteres del teclado.
 * Define qué caracter corresponde a cada combinación de fila y columna.
 * La primera dimensión es la fila, la segunda es la columna.
 */
static const char keypad_map[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
/**
 * @brief Inicializa el teclado matricial.
 * @param keypad Puntero al manejador (handle) del teclado.
 *
 * Configura todos los pines de las filas como salida y los establece en estado BAJO (0V).
 * Esto prepara el teclado para el proceso de escaneo, asegurando que ninguna fila
 * esté activa por defecto.
 */
void keypad_init(keypad_handle_t* keypad) {
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_RESET);
    }
}

/**
 * @brief Escanea el teclado para identificar la tecla presionada.
 * @param keypad Puntero al manejador (handle) del teclado.
 * @param col_pin El pin de la columna que ha generado la interrupción.
 * @return El caracter de la tecla presionada, o '\0' si no se puede determinar.
 *
 * Esta función se llama desde el callback de la interrupción (`HAL_GPIO_EXTI_Callback`)
 * cuando se detecta que una columna ha pasado a estado BAJO.
 *
 * El proceso es el siguiente:
 * 1. Itera a través de cada fila una por una.
 * 2. Activa la fila actual poniéndola en estado ALTO (3.3V/5V).
 * 3. Comprueba si el pin de la columna que causó la interrupción ahora lee un estado ALTO.
 *    - Si la tecla en la intersección de la fila activa y la columna interrumpida está
 *      presionada, el estado ALTO de la fila se propagará a la columna.
 * 4. Si la columna lee ALTO, significa que hemos encontrado la fila correcta. Se devuelve
 *    el caracter correspondiente del `keypad_map`.
 * 5. Se desactiva la fila (se pone en BAJO) antes de pasar a la siguiente para evitar
 *    lecturas incorrectas (ghosting).
 */
char keypad_scan(keypad_handle_t* keypad, uint16_t col_pin) {
    for (int row = 0; row < KEYPAD_ROWS; row++) {
        // 1. Activa la fila actual poniéndola en ALTO.
        HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_SET);

        for (int col = 0; col < KEYPAD_COLS; col++) {
            // 2. Busca la columna que originó la interrupción.
            if (keypad->col_pins[col] == col_pin) {
                // 3. Lee el estado de esa columna.
                GPIO_PinState state = HAL_GPIO_ReadPin(keypad->col_ports[col], keypad->col_pins[col]);

                // 4. Desactiva la fila inmediatamente para prepararse para la siguiente lectura.
                HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_RESET);

                // 5. Si la columna está en ALTO, hemos encontrado la tecla.
                if (state == GPIO_PIN_SET) { // La tecla en la intersección (row, col) está presionada.
                    return keypad_map[row][col];
                }
            }
        }

        // Asegura que la fila quede desactivada antes de probar la siguiente.
        HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_RESET);
    }
    return '\0'; // No se encontró la tecla (caso improbable si fue llamado por una interrupción válida).
}