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
led_handle_t led1 = { .port = GPIOA, .pin = GPIO_PIN_5 }; // LED de la Nucleo

#define RING_BUFFER_SIZE 16    ////aqui decalro un buffer circular de cabeza y cola
uint8_t ring_buffer[RING_BUFFER_SIZE]; ////tamaño 16 para manejar datos de uart 
ring_buffer_t rb;

#define KEYPAD_BUFFER_LEN 16                 ////Aqui declaro un buffer circular para el teclado 4x4
uint8_t keypad_buffer[KEYPAD_BUFFER_LEN];
ring_buffer_t keypad_rb;
                                ///Estructura que contiene todos los pines de filas y columnas del teclado.
                               /// Sirve para identificar qué pines usar para escanearlo.
keypad_handle_t keypad = {
    .row_ports = {KEYPAD_R1_GPIO_Port, KEYPAD_R2_GPIO_Port, KEYPAD_R3_GPIO_Port, KEYPAD_R4_GPIO_Port},
    .row_pins  = {KEYPAD_R1_Pin, KEYPAD_R2_Pin, KEYPAD_R3_Pin, KEYPAD_R4_Pin},
    .col_ports = {KEYPAD_C1_GPIO_Port, KEYPAD_C2_GPIO_Port, KEYPAD_C3_GPIO_Port, KEYPAD_C4_GPIO_Port},
    .col_pins  = {KEYPAD_C1_Pin, KEYPAD_C2_Pin, KEYPAD_C3_Pin, KEYPAD_C4_Pin}
};

uint32_t last_key_time = 0;        /// Variable para manejar el antirebote de las teclas del teclado
                                /// Esta variable almacena el tiempo del último evento de tecla
/* USER CODE END PV */

void SystemClock_Config(void);   /// Configuración del reloj del sistema
void Error_Handler(void);        /// Manejo de errores
static void MX_GPIO_Init(void); /// Inicialización de GPIO
static void MX_USART2_UART_Init(void);  /// Inicialización del UART

/* USER CODE BEGIN 0 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) { //// Callback para interrupciones de GPIO
                                                 ////Esta función se llama automáticamente 
                                                 // cuando se detecta un flanco de bajada
                                                 //  en alguno de los pines configurados como 
                                                 // entrada con interrupción (las columnas del teclado)
    uint32_t now = HAL_GetTick();
    if (now - last_key_time < 200) return;  // Antirebote inteligente 
                                      /// Si el tiempo desde el último evento es menor a 200 ms, se ignora el evento actual
                                     /// Esto evita que se registren múltiples pulsaciones por un solo toque
    last_key_time = now;

    char key = keypad_scan(&keypad, GPIO_Pin);   ///// Escaneo del teclado para detectar la tecla presionada
    if (key != '\0') {             ///// Si se detecta una tecla válida, se escribe en el buffer circular
        ring_buffer_write(&keypad_rb, (uint8_t)key); //// solo si es diferente a la última, evitando rebotes
    }
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
/// Inicialización de la HAL y configuración del reloj del sistema
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
/// Inicialización de la UART y GPIO prioridades para interrupciones
  /* USER CODE BEGIN 2 */
  led_init(&led1);
  ring_buffer_init(&rb, ring_buffer, RING_BUFFER_SIZE);
  ring_buffer_init(&keypad_rb, keypad_buffer, KEYPAD_BUFFER_LEN);
  keypad_init(&keypad);
/// Inicialización del LED y los buffers circulares
///configura pines de filas(como salidas) y columnas del teclado ( como entradas)
  printf("Sistema listo. Introduzca clave de 4 dígitos:\r\n");

  char clave_correcta[5] = "1234";
  char clave_ingresada[5] = {0};
  int idx = 0;
  /* USER CODE END 2 */

  while (1)
  {
    uint8_t tecla;
    if (ring_buffer_read(&keypad_rb, &tecla)) { //// Lectura de la tecla del buffer circular del teclado
        printf("Tecla: %c\r\n", tecla); ///// Imprime la tecla leída en el buffer

        if (idx < 4) {                     ////Va llenando una clave de 4 dígitos. 
                                           // También parpadea el LED cada vez que se presiona una tecla.
            clave_ingresada[idx++] = tecla;
            printf("idx = %d\r\n", idx);

            led_on(&led1);
            HAL_Delay(50);
            led_off(&led1);
        }

        if (idx == 4) { //// Cuando se ingresan 4 dígitos, se compara con la clave correcta                 
            clave_ingresada[4] = '\0';  //// Asegura que la cadena esté terminada en nulo 
            printf("Clave ingresada: %s\r\n", clave_ingresada);         

            if (strcmp(clave_ingresada, clave_correcta) == 0) {   //// Compara la clave ingresada con la correcta
                printf("✔ Acceso permitido\r\n");                  //// Si coinciden, se enciende el LED 5 veces
                for (int i = 0; i < 5; i++) {
                    led_on(&led1);
                    HAL_Delay(500);
                    led_off(&led1);
                    HAL_Delay(500);
                }
            } else {  //// Si no coinciden, se enciende el LED 10 veces                       
                printf("X Acceso denegado\r\n");     
                for (int i = 0; i < 20; i++) {
                    led_toggle(&led1);
                    HAL_Delay(50);  //// Parpadea el LED rápidamente        
                }
                led_off(&led1);
            }

            idx = 0;  //// Reinicia el índice para permitir una nueva entrada de clave
            memset(clave_ingresada, 0, sizeof(clave_ingresada));    //// Limpia el buffer de la clave ingresada
        }
    }

    HAL_Delay(10);    //// Pequeña pausa para evitar saturar el bucle principal     
  }
}

void SystemClock_Config(void)       /// Configuración del reloj del sistema 
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