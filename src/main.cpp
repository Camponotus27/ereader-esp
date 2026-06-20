#include <Arduino.h>
#include <LittleFS.h>
#include "epd4in26.h"
#include "epdpaint.h"

#define COLORED     0
#define UNCOLORED   1

// --- PINES ---
#define BTN_UP      32
#define BTN_DOWN    33
#define BTN_SELECT  25

Epd epd;
#define PAGE_SIZE 48000 
unsigned char* frame_buffer;

enum Estado { MODO_MENU, MODO_LECTOR };
Estado estadoActual = MODO_MENU;

String libros[10];
int totalLibros = 0;
int seleccionMenu = 0;
int paginaActual = 0;
String libroAbierto = "";

// --- SISTEMA DE PANTALLA ---

void actualizarPantalla() {
  Serial.println("\n[EPD] Actualizando contenido...");
  unsigned long tiempoInicio = millis();
  
  epd.Display(frame_buffer); 
  
  unsigned long duracion = millis() - tiempoInicio;
  Serial.printf("[EPD] Refresco terminado en: %lu ms\n", duracion);
}

// --- LÓGICA DE ESTADO (MULTI-SAVE) ---

void guardarEstado() {
  // 1. Guardamos cuál fue el último libro abierto (para el auto-arranque)
  File fileGlobal = LittleFS.open("/ultimo.txt", "w");
  if (fileGlobal) {
    fileGlobal.print(libroAbierto);
    fileGlobal.close();
  }

  // 2. Guardamos la página específica de ESTE libro (ej: "/trono1.dat.sav")
  String pathSave = libroAbierto + ".sav";
  File fileBook = LittleFS.open(pathSave, "w");
  if (fileBook) {
    fileBook.print(paginaActual);
    fileBook.close();
  }
  Serial.printf("[Guardado] Libro: %s | Pág: %d\n", libroAbierto.c_str(), paginaActual);
}

int obtenerPagina(String libro) {
  String pathSave = libro + ".sav";
  if (LittleFS.exists(pathSave)) {
    File file = LittleFS.open(pathSave, "r");
    if (file) {
      int pag = file.readStringUntil('\n').toInt();
      file.close();
      return pag;
    }
  }
  return 0; // Si no hay guardado, empieza en 0
}

bool arrancarUltimoLibro() {
  if (LittleFS.exists("/ultimo.txt")) {
    File file = LittleFS.open("/ultimo.txt", "r");
    if (file) {
      String ultimo = file.readStringUntil('\n');
      file.close();
      
      // Verificamos si el libro aún existe en la memoria
      if (LittleFS.exists(ultimo)) {
        libroAbierto = ultimo;
        paginaActual = obtenerPagina(ultimo);
        Serial.printf("[Auto-Load] Retomando %s en pág %d\n", libroAbierto.c_str(), paginaActual);
        return true;
      }
    }
  }
  return false;
}

// --- ARCHIVOS ---

void listarLibros() {
  totalLibros = 0;
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file && totalLibros < 10) {
    String nombre = file.name();
    if (nombre.endsWith(".dat") || nombre.endsWith(".bin")) {
      libros[totalLibros] = "/" + nombre;
      totalLibros++;
    }
    file = root.openNextFile();
  }
}

void cargarPagina(String path, int numPagina) {
  File file = LittleFS.open(path, "r");
  if (!file) return;
  long offset = (long)numPagina * PAGE_SIZE;
  if (file.size() >= offset + PAGE_SIZE) {
    file.seek(offset);
    file.read(frame_buffer, PAGE_SIZE);
  } else {
    Serial.println("[Aviso] Fin del libro.");
  }
  file.close();
}

void dibujarMenu() {
  Paint paint(frame_buffer, 800, 480);
  paint.Clear(UNCOLORED);
  paint.SetRotate(ROTATE_90);
  paint.DrawStringAt(20, 20, "BIBLIOTECA SEBA", &Font24, COLORED);
  paint.DrawLine(20, 50, 460, 50, COLORED);
  
  for (int i = 0; i < totalLibros; i++) {
    int yPos = 80 + (i * 60);
    if (i == seleccionMenu) {
      paint.DrawFilledRectangle(20, yPos, 460, yPos + 40, COLORED);
      paint.DrawStringAt(30, yPos + 10, libros[i].substring(1).c_str(), &Font16, UNCOLORED);
    } else {
      paint.DrawRectangle(20, yPos, 460, yPos + 40, COLORED);
      paint.DrawStringAt(30, yPos + 10, libros[i].substring(1).c_str(), &Font16, COLORED);
    }
  }
  actualizarPantalla(); 
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Seba-Reader OS: Multi-Save Mode ---");
  
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  if (!LittleFS.begin(true)) {
    Serial.println("Error LittleFS");
    return;
  }
  
  listarLibros();
  frame_buffer = (unsigned char*)malloc(PAGE_SIZE);

  epd.Reset();
  epd.Init();
  
  // Auto-arranque
  if (arrancarUltimoLibro()) {
    estadoActual = MODO_LECTOR;
    cargarPagina(libroAbierto, paginaActual);
    actualizarPantalla();
  } else {
    dibujarMenu();
  }
}

void loop() {
  // BOTON DOWN
  if (digitalRead(BTN_DOWN) == LOW) {
    delay(50);
    if (digitalRead(BTN_DOWN) == LOW) {
      Serial.println(">>> CLICK: DOWN");
      if (estadoActual == MODO_MENU && totalLibros > 0) {
        seleccionMenu = (seleccionMenu + 1) % totalLibros;
        dibujarMenu();
      } else if (estadoActual == MODO_LECTOR) {
        paginaActual++;
        cargarPagina(libroAbierto, paginaActual);
        guardarEstado();
        actualizarPantalla();
      }
      while(digitalRead(BTN_DOWN) == LOW);
    }
  }

  // BOTON UP
  if (digitalRead(BTN_UP) == LOW) {
    delay(50);
    if (digitalRead(BTN_UP) == LOW) {
      Serial.println(">>> CLICK: UP");
      if (estadoActual == MODO_MENU && totalLibros > 0) {
        seleccionMenu = (seleccionMenu - 1 + totalLibros) % totalLibros;
        dibujarMenu();
      } else if (estadoActual == MODO_LECTOR && paginaActual > 0) {
        paginaActual--;
        cargarPagina(libroAbierto, paginaActual);
        guardarEstado();
        actualizarPantalla();
      }
      while(digitalRead(BTN_UP) == LOW);
    }
  }

  // BOTON SELECT
  if (digitalRead(BTN_SELECT) == LOW) {
    delay(50);
    if (digitalRead(BTN_SELECT) == LOW) {
      unsigned long inicioPresion = millis();
      bool volviendoAlMenu = false;

      while(digitalRead(BTN_SELECT) == LOW) {
        // Mantener para volver al menú
        if (estadoActual == MODO_LECTOR && millis() - inicioPresion > 1000 && !volviendoAlMenu) {
          Serial.println(">>> MANTENIDO: SALIENDO AL MENU");
          volviendoAlMenu = true;
          estadoActual = MODO_MENU;
          dibujarMenu();
        }
        delay(10);
      }

      // Click normal para ENTRAR al libro
      if (!volviendoAlMenu) {
        if (estadoActual == MODO_MENU && totalLibros > 0) {
          Serial.println(">>> CLICK: SELECT (Entrar a libro)");
          estadoActual = MODO_LECTOR;
          libroAbierto = libros[seleccionMenu];
          
          // AQUÍ ESTABA EL BUG: ¡Ya no lo reiniciamos a 0!
          // Leemos la página guardada de ese libro específico
          paginaActual = obtenerPagina(libroAbierto); 
          
          cargarPagina(libroAbierto, paginaActual);
          guardarEstado();
          actualizarPantalla();
        }
      }
    }
  }
} 