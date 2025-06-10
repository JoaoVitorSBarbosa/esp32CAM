#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include <Adafruit_NeoPixel.h>
#include "../libs/cameraPins.h"



// Replace with your network credentials
const char* ssid = "";
const char* password = "";

// LED RGB
#define PIN_PIXS 48
#define PIX_NUM 1
Adafruit_NeoPixel pixels(PIX_NUM, PIN_PIXS, NEO_GRB + NEO_KHZ800);

void startCameraServer();

void showPixelColor(uint32_t c) {
  pixels.setPixelColor(0, c);
  pixels.show();
}

void initCameraHighPerformance() {
  camera_config_t config;
  
  // Configurações básicas de pinos
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  
  // OTIMIZAÇÕES DE PERFORMANCE
  
  // 1. XCLK otimizado para ESP32-S3 (máximo suportado)
  config.xclk_freq_hz = 40000000;  // 40MHz para máxima performance
  
  // 2. Resolução otimizada para balance performance/qualidade
  config.frame_size = FRAMESIZE_VGA;  // 640x480 - melhor balance
  
  // 3. Formato JPEG para compressão e velocidade
  config.pixel_format = PIXFORMAT_JPEG;
  
  // 4. Configurações de buffer otimizadas
  config.grab_mode = CAMERA_GRAB_LATEST;  // Sempre pega o frame mais recente
  config.fb_location = CAMERA_FB_IN_PSRAM;  // Usar PSRAM para buffers
  
  // 5. Qualidade JPEG otimizada para velocidade
  config.jpeg_quality = 15;  // Qualidade menor = mais velocidade
  
  // 6. Múltiplos buffers para performance
  config.fb_count = 3;  // 3 buffers para pipeline otimizado
  
  // 7. Configurações de clock otimizadas
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  
  // Verificar PSRAM e ajustar configurações
  if (psramFound()) {
    Serial.printf("PSRAM encontrado: %d bytes\n", ESP.getPsramSize());
    
    // Com PSRAM: configurações de alta performance
    config.jpeg_quality = 12;  // Qualidade um pouco melhor
    config.fb_count = 4;       // Mais buffers
    
    // Para framerates ainda maiores, pode usar resolução menor
    // config.frame_size = FRAMESIZE_HVGA;  // 480x320 para FPS máximo
    
    showPixelColor(0x0000FF); // Azul para PSRAM OK
  } else {
    Serial.println("PSRAM não encontrado - usando configuração básica");
    config.frame_size = FRAMESIZE_QVGA;  // 320x240 sem PSRAM
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.fb_count = 2;
    config.jpeg_quality = 20;
    showPixelColor(0xFF0000); // Vermelho para sem PSRAM
  }
  
  // Inicializar câmera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Falha na inicialização da câmera: 0x%x\n", err);
    showPixelColor(0xFF0000); // Vermelho para erro
    return;
  }
  
  // OTIMIZAÇÕES DO SENSOR
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    // Configurações básicas
    //s->set_vflip(s, 1);      // Inverter verticalmente
    s->set_hmirror(s, 0);    // Espelho horizontal
    
    // OTIMIZAÇÕES DE PERFORMANCE DO SENSOR
    s->set_brightness(s, 0);     // Brilho neutro
    s->set_contrast(s, 0);       // Contraste neutro
    s->set_saturation(s, -1);    // Saturação reduzida (menos processamento)
    s->set_sharpness(s, -1);     // Nitidez reduzida (menos processamento)
    s->set_denoise(s, 0);        // Desabilitar denoise (mais velocidade)
    
    // Configurações de exposição para velocidade
    s->set_exposure_ctrl(s, 1);      // Controle automático de exposição
    s->set_aec2(s, 0);               // Desabilitar AEC2 avançado
    s->set_ae_level(s, 0);           // Nível de exposição neutro
    s->set_aec_value(s, 300);        // Valor de exposição fixo baixo
    
    // Configurações de ganho
    s->set_gain_ctrl(s, 1);          // Controle automático de ganho
    s->set_agc_gain(s, 0);           // Ganho baixo para menos ruído
    s->set_gainceiling(s, (gainceiling_t)2);  // Teto de ganho baixo
    
    // Configurações de white balance para velocidade
    s->set_whitebal(s, 1);           // White balance automático
    s->set_awb_gain(s, 1);           // AWB gain habilitado
    s->set_wb_mode(s, 0);            // Modo WB automático
    
    // Desabilitar recursos que consomem processamento
    s->set_dcw(s, 0);                // Desabilitar DCW
    s->set_bpc(s, 0);                // Desabilitar BPC
    s->set_wpc(s, 0);                // Desabilitar WPC
    s->set_lenc(s, 0);               // Desabilitar lens correction
    
    Serial.println("Sensor otimizado para alta performance");
    showPixelColor(0x00FF00); // Verde para sucesso
  } else {
    Serial.println("Falha ao obter sensor da câmera");
    showPixelColor(0xFF0000); // Vermelho para erro
  }
}

void startWifiConfig() {
  delay(500);
  WiFi.begin(ssid, password);
  
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi conectado!");
  Serial.print("IP da câmera: ");
  Serial.println(WiFi.localIP());
  showPixelColor(0x00FF00); // Verde para WiFi conectado
}

void testFramerate() {
  Serial.println("Testando framerate...");
  
  unsigned long start = millis();
  int frames = 0;
  
  for (int i = 0; i < 50; i++) {  // Testar 50 frames
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
      frames++;
      esp_camera_fb_return(fb);
    }
    delay(10);  // Pequeno delay
  }
  
  unsigned long elapsed = millis() - start;
  float fps = (frames * 1000.0) / elapsed;
  
  Serial.printf("Framerate: %.2f FPS (%d frames em %lu ms)\n", fps, frames, elapsed);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando ESP32-S3 Goouuu CAM - MODO ALTA PERFORMANCE");
  
  // Inicializar LED
  pixels.begin();
  pixels.setBrightness(8);
  showPixelColor(0x000000); // Desligado
  
  // Inicializar câmera com otimizações
  initCameraHighPerformance();
  
  // Conectar WiFi
  startWifiConfig();
  
  // Testar framerate
  testFramerate();
  
  // Iniciar servidor da câmera
  startCameraServer();
  
  Serial.println("Câmera otimizada pronta! Acesse:");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("Configurações aplicadas:");
  Serial.println("- XCLK: 40MHz");
  Serial.println("- Resolução: VGA (640x480)");
  Serial.println("- Qualidade JPEG: Otimizada para velocidade");
  Serial.println("- Buffers: Múltiplos para pipeline");
  Serial.println("- Sensor: Otimizado para performance");
}

void loop() {
  // Efeito rainbow rápido no LED para indicar alta performance
  static unsigned int cont = 0;
  pixels.rainbow(cont, -1, 255, 255);
  pixels.show();
  cont = (cont >= 65536) ? 0 : cont + 256;  // Incremento maior = mais rápido
  delay(5);  // Delay menor = mais responsivo
}

