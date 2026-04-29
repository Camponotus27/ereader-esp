#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <gdeq/GxEPD2_426_GDEQ0426T82.h> // El driver original que encontramos
#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoBold12pt7b.h>

// --- CONFIGURACIÓN DE PINES ---
#define EPD_CS    5
#define EPD_DC    17
#define EPD_RST   16
#define EPD_BUSY  4

// Instanciamos el objeto con el driver GDEQ
GxEPD2_BW<GxEPD2_426_GDEQ0426T82, GxEPD2_426_GDEQ0426T82::HEIGHT> display(GxEPD2_426_GDEQ0426T82(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Iniciando pantalla con driver GDEQ...");
  // Inicializamos a 115200
  display.init(115200); 

  display.setRotation(1); 
  display.setFont(&FreeMonoBold12pt7b);
  display.setTextColor(GxEPD_BLACK);

  // Ciclo de dibujo
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(50, 80);
    display.print("HOLA SEBA!");
    display.setCursor(50, 140);
    display.print("Driver GDEQ0426T82");
    display.setCursor(50, 200);
    display.print("Refresco perfecto.");
  } while (display.nextPage());

  Serial.println("¡Dibujo completado!");
  display.powerOff();
}

void loop() {
  // Nada por aquí
}