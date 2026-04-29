#include <Arduino.h>
#include "epd4in26.h"
#include "epdpaint.h"

#define COLORED     0
#define UNCOLORED   1

// --- CONFIGURACIÓN DE BOTONES ---
// (Conecta un lado del botón a estos pines y el otro a GND)
#define BTN_UP      25
#define BTN_DOWN    26
#define BTN_SELECT  27

Epd epd;
#define BUFFER_SIZE (800 * 480 / 8)

// Variable para saber en qué opción estamos
int opcionActual = 1; 
int totalOpciones = 3;
bool dibujarMenu = true; // Bandera para saber si hay que refrescar la pantalla

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Iniciando Seba-Reader...");

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  if (epd.Init() != 0) {
    Serial.println("Fallo al iniciar el módulo e-Paper.");
    return;
  }
  
  // ELIMINAMOS el epd.Clear() para que no choque con el menú
  // El menú igual pintará el fondo blanco por su cuenta.
}

void loop() {
  // ... (Tu código de lectura de botones queda igualito) ...
  if (digitalRead(BTN_DOWN) == LOW) {
    opcionActual++;
    if (opcionActual > totalOpciones) opcionActual = 1;
    dibujarMenu = true;
    Serial.println("Boton ABAJO presionado");
    delay(400); 
  }
  
  if (digitalRead(BTN_UP) == LOW) {
    opcionActual--;
    if (opcionActual < 1) opcionActual = totalOpciones;
    dibujarMenu = true;
    Serial.println("Boton ARRIBA presionado");
    delay(400);
  }

  if (digitalRead(BTN_SELECT) == LOW) {
    Serial.print("Seleccionaste la opcion: ");
    Serial.println(opcionActual);
    delay(400);
  }

  // --- 2. DIBUJADO DE LA PANTALLA ---
  if (dibujarMenu) {
    Serial.println("Generando graficos en RAM...");
    
    unsigned char* frame_buffer = (unsigned char*)malloc(BUFFER_SIZE);
    if (frame_buffer == NULL) {
      Serial.println("Error: No hay suficiente RAM.");
      return;
    }
    
    // CORRECCIÓN CLAVE: Inicializamos con dimensiones físicas (800x480)
    Paint paint(frame_buffer, 800, 480); 
    paint.Clear(UNCOLORED); 
    
    // AHORA rotamos a vertical. La librería sola entenderá que ahora es 480x800
    paint.SetRotate(ROTATE_90); 

    // --- DISEÑO DEL MENÚ ---
    paint.DrawStringAt(20, 30, "SEBA-READER", &Font24, COLORED);
    paint.DrawLine(20, 60, 460, 60, COLORED); 

    if (opcionActual == 1) {
      paint.DrawFilledRectangle(20, 100, 460, 160, COLORED);
      paint.DrawStringAt(30, 120, "> 1. Leer Libro", &Font24, UNCOLORED);
    } else {
      paint.DrawRectangle(20, 100, 460, 160, COLORED);
      paint.DrawStringAt(30, 120, "  1. Leer Libro", &Font24, COLORED);
    }

    if (opcionActual == 2) {
      paint.DrawFilledRectangle(20, 180, 460, 240, COLORED);
      paint.DrawStringAt(30, 200, "> 2. Config", &Font24, UNCOLORED);
    } else {
      paint.DrawRectangle(20, 180, 460, 240, COLORED);
      paint.DrawStringAt(30, 200, "  2. Config", &Font24, COLORED);
    }

    if (opcionActual == 3) {
      paint.DrawFilledRectangle(20, 260, 460, 320, COLORED);
      paint.DrawStringAt(30, 280, "> 3. Apagar", &Font24, UNCOLORED);
    } else {
      paint.DrawRectangle(20, 260, 460, 320, COLORED);
      paint.DrawStringAt(30, 280, "  3. Apagar", &Font24, COLORED);
    }

    Serial.println("Actualizando panel de tinta...");
    epd.Init(); 
    epd.Display(frame_buffer);
    epd.Sleep(); 

    free(frame_buffer); 
    dibujarMenu = false; 
  }
}