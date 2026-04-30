#include <Arduino.h>
#include <LittleFS.h>
#include "epd4in26.h"
#include "epdpaint.h"

#define COLORED     0
#define UNCOLORED   1

// --- PINES SEGUROS ---
#define BTN_UP      32
#define BTN_DOWN    33
#define BTN_SELECT  25

Epd epd;
#define PAGE_SIZE 48000 // 800 * 480 / 8
unsigned char* frame_buffer;

enum Estado { MODO_MENU, MODO_LECTOR };
Estado estadoActual = MODO_MENU;

String libros[10];
int totalLibros = 0;
int seleccionMenu = 0;
int paginaActual = 0;
String libroAbierto = "";

bool pantallaDormida = true;

// --- FUNCIÓN DE ACTUALIZACIÓN BLINDADA ---

void actualizarPantalla(bool dormirDespues) {
  // Si vamos a dormir la pantalla o si estaba dormida, hacemos un ciclo completo
  if (pantallaDormida || dormirDespues) {
    Serial.println("Forzando Reset de Hardware e-Ink...");
    // Esto asegura que el controlador salga de cualquier estado de error o bloqueo
    epd.Reset(); 
    
    Serial.println("Inicializando registros...");
    if (epd.Init() != 0) {
      Serial.println("ERROR: No se pudo inicializar la pantalla.");
      return;
    }
    pantallaDormida = false;
  } else {
    Serial.println("Refresco rápido (Pantalla ya activa).");
  }
  
  Serial.println("Transfiriendo datos al panel...");
  epd.Display(frame_buffer); 
  
  if (dormirDespues) { // No funciona dormir de momento
    // Pequeño delay para asegurar que el comando Display fue procesado
    delay(200); 
    Serial.println("Comando Sleep enviado.");
    epd.Sleep();
    pantallaDormida = true;
  }
}

// --- ARCHIVOS ---

void listarLibros() {
  Serial.println("--- Escaneando memoria interna ---");
  totalLibros = 0;
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file && totalLibros < 10) {
    String nombre = file.name();
    if (nombre.endsWith(".dat") || nombre.endsWith(".bin")) {
      libros[totalLibros] = "/" + nombre;
      Serial.print("Libro encontrado: ");
      Serial.println(libros[totalLibros]);
      totalLibros++;
    }
    file = root.openNextFile();
  }
  Serial.print("Total de libros: ");
  Serial.println(totalLibros);
}

void cargarPagina(String path, int numPagina) {
  Serial.printf("Cargando pág %d de %s\n", numPagina, path.c_str());
  File file = LittleFS.open(path, "r");
  if (!file) return;

  long offset = (long)numPagina * PAGE_SIZE;
  if (file.size() >= offset + PAGE_SIZE) {
    file.seek(offset);
    file.read(frame_buffer, PAGE_SIZE);
  }
  file.close();
}

// --- DIBUJO ---

void dibujarMenu() {
  Paint paint(frame_buffer, 800, 480);
  paint.Clear(UNCOLORED);
  paint.SetRotate(ROTATE_90);

  paint.DrawStringAt(20, 20, "BIBLIOTECA", &Font24, COLORED);
  paint.DrawLine(20, 50, 460, 50, COLORED);

  if (totalLibros == 0) {
    paint.DrawStringAt(20, 100, "No hay libros", &Font16, COLORED);
  } else {
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
  }
  actualizarPantalla(false); // Menú no se duerme para navegar rápido
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nSeba-Reader OS Iniciado");

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  if (!LittleFS.begin(true)) return;
  listarLibros();

  frame_buffer = (unsigned char*)malloc(PAGE_SIZE);
  dibujarMenu();
}

void loop() {
  if (digitalRead(BTN_DOWN) == LOW) {
    if (estadoActual == MODO_MENU && totalLibros > 0) {
      seleccionMenu = (seleccionMenu + 1) % totalLibros;
      dibujarMenu();
    } else if (estadoActual == MODO_LECTOR) {
      paginaActual++;
      cargarPagina(libroAbierto, paginaActual);
      actualizarPantalla(true); 
    }
    delay(400);
  }

  if (digitalRead(BTN_UP) == LOW) {
    if (estadoActual == MODO_MENU && totalLibros > 0) {
      seleccionMenu = (seleccionMenu - 1 + totalLibros) % totalLibros;
      dibujarMenu();
    } else if (estadoActual == MODO_LECTOR && paginaActual > 0) {
      paginaActual--;
      cargarPagina(libroAbierto, paginaActual);
      actualizarPantalla(true); 
    }
    delay(400);
  }

  if (digitalRead(BTN_SELECT) == LOW) {
    if (estadoActual == MODO_MENU && totalLibros > 0) {
      estadoActual = MODO_LECTOR;
      libroAbierto = libros[seleccionMenu];
      paginaActual = 0;
      cargarPagina(libroAbierto, paginaActual);
      actualizarPantalla(true); 
    } else if (estadoActual == MODO_LECTOR) {
      estadoActual = MODO_MENU;
      dibujarMenu();
    }
    delay(400);
  }
}