#include "ring_buffer.h"

// La capacidad real es un elemento menos para distinguir entre lleno y vacío.
// Esto se manejará implícitamente en las funciones read/write.

void ring_buffer_init(ring_buffer_t *rb, uint8_t *buffer, uint16_t capacity)
{
    rb->buffer = buffer;        // Puntero al array de datos proporcionado
    rb->capacity = capacity;    // Capacidad máxima del buffer
    rb->head = 0;               // Puntero de escritura (próximo lugar a escribir)
    rb->tail = 0;               // Puntero de lectura (próximo lugar a leer)
}

bool ring_buffer_write(ring_buffer_t *rb, uint8_t data)
{
    // Verificar si el buffer está lleno
    // Si head + 1 (módulo capacity) es igual a tail, significa que estamos a punto de escribir sobre el dato más antiguo.
    // Esto es si decidimos que un buffer "lleno" tiene capacity-1 elementos.
    // Una forma más simple es: si el siguiente head es igual a tail, está lleno.
    if (((rb->head + 1) % rb->capacity) == rb->tail) {
        // Buffer lleno, no se puede escribir
        return false;
    }

    rb->buffer[rb->head] = data; // Escribir el dato en la posición actual de head
    rb->head = (rb->head + 1) % rb->capacity; // Mover head al siguiente espacio (circularmente)

    return true; // Escritura exitosa
}

bool ring_buffer_read(ring_buffer_t *rb, uint8_t *data)
{
    // Verificar si el buffer está vacío
    if (rb->head == rb->tail) {
        // Buffer vacío, no hay datos para leer
        return false;
    }

    *data = rb->buffer[rb->tail]; // Leer el dato de la posición actual de tail
    rb->tail = (rb->tail + 1) % rb->capacity; // Mover tail al siguiente espacio (circularmente)

    return true; // Lectura exitosa
}

uint16_t ring_buffer_count(ring_buffer_t *rb)
{
    // Calcular el número de elementos en el buffer
    // Maneja el caso en que tail es mayor que head (cuando ha habido un "wrap-around")
    if (rb->head >= rb->tail) {
        return rb->head - rb->tail;
    } else {
        return rb->capacity - (rb->tail - rb->head);
    }
}

bool ring_buffer_is_empty(ring_buffer_t *rb)
{
    // El buffer está vacío si head y tail apuntan a la misma posición
    return (rb->head == rb->tail);
}

bool ring_buffer_is_full(ring_buffer_t *rb)
{
    // El buffer está lleno si el siguiente lugar para escribir (head + 1) es igual a tail
    // Esto significa que siempre hay al menos un espacio vacío
    return (((rb->head + 1) % rb->capacity) == rb->tail);
}

void ring_buffer_flush(ring_buffer_t *rb)
{
    // Simplemente restablece head y tail a cero para vaciar el buffer
    // Los datos antiguos siguen en la memoria, pero ya no son accesibles
    rb->head = 0;
    rb->tail = 0;
}