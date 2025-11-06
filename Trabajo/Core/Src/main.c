/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32l4xx_it.h
  * @brief   This file contains the headers of the interrupt handlers.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "keypad_driver.h"
#include "led_driver.h"
#include "ring_buffer.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart2;
/* USER CODE BEGIN PV */
// Estructura para manejar el LED
led_handle_t led1 = { .port = GPIOA, .pin = GPIO_PIN_5 }; 

// Define el tamaño del buffer circular
#define RING_BUFFER_SIZE 16    
// Buffer circular para datos generales
uint8_t ring_buffer[RING_BUFFER_SIZE]; 
// Estructura del buffer circular
ring_buffer_t rb;

// Define la longitud del buffer del teclado
#define KEYPAD_BUFFER_LEN 16                
// Buffer circular para las teclas presionadas
uint8_t keypad_buffer[KEYPAD_BUFFER_LEN];
// Estructura del buffer circular del teclado
ring_buffer_t keypad_rb;
                            
// Estructura para manejar el teclado
keypad_handle_t keypad = {
    .row_ports = {KEYPAD_R1_GPIO_Port, KEYPAD_R2_GPIO_Port, KEYPAD_R3_GPIO_Port, KEYPAD_R4_GPIO_Port}, // Puertos GPIO de las filas
    .row_pins  = {KEYPAD_R1_Pin, KEYPAD_R2_Pin, KEYPAD_R3_Pin, KEYPAD_R4_Pin}, // Pines GPIO de las filas
    .col_ports = {KEYPAD_C1_GPIO_Port, KEYPAD_C2_GPIO_Port, KEYPAD_C3_GPIO_Port, KEYPAD_C4_GPIO_Port}, // Puertos GPIO de las columnas
    .col_pins  = {KEYPAD_C1_Pin, KEYPAD_C2_Pin, KEYPAD_C3_Pin, KEYPAD_C4_Pin} // Pines GPIO de las columnas
};

// Tiempo de la última tecla presionada
uint32_t last_key_time = 0;

void SystemClock_Config(void);  
void Error_Handler(void);       
static void MX_GPIO_Init(void); 
static void MX_USART2_UART_Init(void); 

/* USER CODE BEGIN 0 */
/**
 * @brief Esta función es un "callback" de interrupción y NO se ejecuta continuamente.
 *        Funciona como un sistema de eventos.
 * 
 * 1. **Configuración:** En `MX_GPIO_Init()`, los pines de las columnas del teclado se configuran
 *    en modo `GPIO_MODE_IT_FALLING`. Esto le dice al microcontrolador que genere una
 *    interrupción de hardware (EXTI) cuando el voltaje en uno de esos pines caiga de ALTO a BAJO.
 * 
 * 2. **Disparo (Trigger):** Cuando se presiona una tecla, se conecta una fila (en estado BAJO) con
 *    una columna (en estado ALTO por una resistencia de pull-up). Esto provoca la caída de voltaje
 *    en el pin de la columna.
 * 
 * 3. **Respuesta del Hardware:** El hardware de interrupciones EXTI detecta este cambio y pausa
 *    temporalmente el bucle `while(1)` del `main`.
 * 
 * 4. **Salto a la ISR:** El procesador salta a la rutina de servicio de interrupción correspondiente
 *    (ej. `EXTI9_5_IRQHandler`), que a su vez llama a `HAL_GPIO_EXTI_IRQHandler()`. Finalmente,
 *    la función de la HAL llama a este callback (`HAL_GPIO_EXTI_Callback`), pasándole el pin
 *    que originó todo.
 */
// Callback para la interrupción EXTI (teclado matricial)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) { 
    // Evita rebotes
    uint32_t now = HAL_GetTick();
    if (now - last_key_time < 200) return;
    last_key_time = now;

    char key = keypad_scan(&keypad, GPIO_Pin);   // Escanea el teclado para obtener la tecla presionada
    if (key != '\0') {             // Si la tecla es válida
        ring_buffer_write(&keypad_rb, (uint8_t)key); // Escribe la tecla en el buffer circular del teclado
    }
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init(); // Inicializa la HAL (Hardware Abstraction Layer) de STM32
  SystemClock_Config(); // Configura el reloj del sistema
  MX_GPIO_Init(); // Inicializa los pines GPIO
  MX_USART2_UART_Init(); // Inicializa la UART2

  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0); // Configura la prioridad de la interrupción del SysTick
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0); // Configura la prioridad de la interrupción EXTI9_5 (columnas del teclado)
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0); // Configura la prioridad de la interrupción EXTI15_10 (columnas del teclado)

  // Inicializa el LED, los buffers circulares y el teclado
  led_init(&led1);
  ring_buffer_init(&rb, ring_buffer, RING_BUFFER_SIZE);
  ring_buffer_init(&keypad_rb, keypad_buffer, KEYPAD_BUFFER_LEN);
  keypad_init(&keypad);

  printf("Sistema listo. Introduzca clave de 4 dígitos:\r\n");

  char clave_correcta[5] = "1234";
  char clave_ingresada[5] = {0};
  int idx = 0;
  /* USER CODE END 2 */

  while (1)
  {
    // Lee una tecla del buffer circular del teclado
    uint8_t tecla;
    if (ring_buffer_read(&keypad_rb, &tecla)) { 
        printf("Tecla: %c\r\n", tecla); // Imprime la tecla presionada por la UART

        if (idx < 4) {                     // Si aún no se han ingresado 4 dígitos
            clave_ingresada[idx++] = tecla;
            printf("idx = %d\r\n", idx);

            led_on(&led1);  // Enciende el LED para indicar que se ha ingresado un dígito
            HAL_Delay(100);
            led_off(&led1); // Apaga el LED después de un breve retardo
        }
        // Si se han ingresado 4 dígitos
        if (idx == 4) {                
            clave_ingresada[4] = '\0';  
            printf("Clave ingresada: %s\r\n", clave_ingresada);         
            // Compara la clave ingresada con la clave correcta
            if (strcmp(clave_ingresada, clave_correcta) == 0) {   
                printf("✔ Acceso permitido\r\n");                 
                for (int i = 0; i < 3; i++) {
                    led_on(&led1);  // Enciende el LED varias veces para indicar acceso permitido
                    HAL_Delay(500);
                    led_off(&led1); // Apaga el LED
                    HAL_Delay(500);
                }
            } else {                       
                printf("X Acceso denegado\r\n");     
                for (int i = 0; i < 5; i++) {
                    led_toggle(&led1); // Hace parpadear el LED varias veces para indicar acceso denegado
                    HAL_Delay(200);          
                }
                led_off(&led1);
            }

            printf("Sistema listo. Introduzca clave de 4 dígitos:\r\n"); // Pide al usuario que ingrese la clave nuevamente
            // Reinicia el sistema para que el usuario pueda ingresar la clave de nuevo
            idx = 0; // Reinicia el índice
            memset(clave_ingresada, 0, sizeof(clave_ingresada));  // Limpia la clave ingresada
        }
    }

    HAL_Delay(10);   
  }
}

void SystemClock_Config(void)      
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  HAL_UART_Init(&huart2);
}

/**
  * @brief Retargets the C library printf function to the USART.
  * @param ch Character to send
  * @param f File stream
  * @retval The character sent
  */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
// función para la inicializacion de los puertos GPIO

static void MX_GPIO_Init(void)            /// Inicialización de los pines GPIO
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  GPIO_InitStruct.Pin = KEYPAD_R1_Pin;
  HAL_GPIO_Init(KEYPAD_R1_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = KEYPAD_R2_Pin | KEYPAD_R3_Pin | KEYPAD_R4_Pin;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;

  GPIO_InitStruct.Pin = KEYPAD_C1_Pin;
  HAL_GPIO_Init(KEYPAD_C1_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = KEYPAD_C2_Pin | KEYPAD_C3_Pin;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = KEYPAD_C4_Pin;
  HAL_GPIO_Init(KEYPAD_C4_GPIO_Port, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void Error_Handler(void)        /// Manejo de errores 
{ 
  __disable_irq();                  /// Deshabilita todas las interrupciones
  while (1) {}      /// Bucle infinito para indicar un error              
}