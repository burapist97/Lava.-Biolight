/*
  ============================================================
   BIOLIGHT — Sirkadiyen Aydınlatma
   Bir LAVA. ürünü
  ============================================================
   Donanım : ESP32-C3 OLED veya ESP32-C3 Mini + WS2812B 12'li NeoPixel halka
             ESP32-C3 OLED → DI GPIO2'ye,  ESP32-C3 Mini → DI GPIO3'e
   Kütüphaneler (Kütüphane Yöneticisi'nden):
     - ArduinoJson  (v7.x)
     - Adafruit NeoPixel
   Klasördeki tipler.h, arayuz.h ve kurulum.h dosyaları da sketch ile aynı klasörde olmalı.

   İlk kurulum : Telefonla "LAVA_Setup" ağına bağlan, sayfa kendiliğinden açılır.
   Kontrol paneli: http://biolight.local  (veya Seri Monitör'deki IP)
   Wi-Fi sıfırlama: BOOT düğmesine 5 saniye basılı tut.
  ============================================================
*/

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <time.h>
#include <math.h>
#include "tipler.h"
#include "arayuz.h"
#include "kurulum.h"

// ---------------- DONANIM ----------------
// Kartına göre seç: ESP32-C3 OLED için 2, ESP32-C3 Mini için 3.
// Mini kartta GPIO2 kullanılırsa kart açılışta hataya girer.
#define LED_PIN        2
#define NUM_LEDS       12
#define POWER_LIMIT_MA 600     // Halkanın çekebileceği en yüksek akım (USB için güvenli)
#define FRAME_MS       20      // 50 FPS
#define FADE_MS        900     // Modlar arası yumuşak geçiş süresi
#define PREVIEW_MS     45000UL // Efekt önizleme süresi

// ---------------- API ----------------
// OpenWeather API anahtarını buraya yaz (openweathermap.org → hesabın → API keys)
const char* OWM_API_KEY = "BURAYA_API_ANAHTARINI_YAZ";
const unsigned long WEATHER_INTERVAL = 10UL * 60UL * 1000UL;
const unsigned long WEATHER_RETRY    = 60UL * 1000UL;

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);
Preferences prefs;

// ---------------- TİPLER ----------------
// Türler tipler.h dosyasında (Arduino IDE 1.8 uyumluluğu için)

const char* MODE_KEYS[M_COUNT] = {"sirkadiyen", "mum", "sabit", "gokkusagi", "akis", "gecis", "nefes"};

const char* WX_KEYS[WX_COUNT] = {"", "gunes", "parcali", "bulutlu", "sis", "yagmur", "saganak", "kar"};
const int PREVIEW_IDS[WX_COUNT] = {800, 800, 802, 804, 741, 501, 211, 601};

const char* PHASE_KEYS[]  = {"safak", "gun", "alacakaranlik", "gece"};
const char* PHASE_NAMES[] = {"Şafak", "Gün", "Alacakaranlık", "Gece"};

// ---------------- AYARLAR (kalıcı) ----------------
Settings S;

bool settingsDirty = false;
unsigned long dirtyAt = 0;
void markDirty() { settingsDirty = true; dirtyAt = millis(); }

// ---------------- HAVA DURUMU (iki çekirdek arasında paylaşılır) ----------------
WeatherData wxShared;
char cityShared[48];
portMUX_TYPE wxMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool wxRefresh = true;

WeatherData wxSnapshot() {
  WeatherData w;
  portENTER_CRITICAL(&wxMux); w = wxShared; portEXIT_CRITICAL(&wxMux);
  return w;
}

// ---------------- ÇALIŞMA ZAMANI DURUMU ----------------
RGBf tgt[NUM_LEDS], out[NUM_LEDS], fromBuf[NUM_LEDS];
float theta[NUM_LEDS], px[NUM_LEDS], py[NUM_LEDS];
float dt = 0.02f;
unsigned long lastFrame = 0, fadeStart = 0;
int lastKey = -999;

Phase curPhase = P_GUN;
float srMin = 420, ssMin = 1170;
Wx previewCat = WX_NONE;
unsigned long previewUntil = 0;
int lastSchedKey = -1;

Lightning L;

// ---------------- WI-FI ve KURULUM DURUMU ----------------
#if CONFIG_IDF_TARGET_ESP32C3
  #define RESET_BTN 9          // ESP32-C3 BOOT düğmesi
#else
  #define RESET_BTN 0          // Klasik ESP32 BOOT düğmesi
#endif
#define RESET_HOLD_MS 5000
const char* AP_NAME = "LAVA_Setup";

DNSServer dns;
bool portalMode = false, mdnsOn = false;
char savedSsid[33] = "", savedPass[65] = "";
char pSsid[33] = "", pPass[65] = "";
PortalStep ps = PS_IDLE;
char psErr[112] = "";
unsigned long psStart = 0, doneAt = 0, finishAt = 0, lastSavedTry = 0, pressStart = 0;
volatile int lastDiscReason = 0;
volatile uint32_t wxSeq = 0;
uint32_t wxSeqMark = 0;
String scanJson = "[]";
bool scanBusy = false;
SysState sys = SYS_NORMAL;
unsigned long sysSince = 0;
float holdProgress = 0, aSys = 0, aRay = 0;

// ---------------- ÇOKLU CİHAZ ----------------
char devId[8] = "";                 // MAC adresinden türetilen kısa kimlik
char hostName[24] = "biolight";     // mDNS adı: ilk cihaz "biolight", diğerleri "biolight-xxxxxx"
Peer peers[MAX_PEERS];
uint8_t peerCount = 0;
portMUX_TYPE peerMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool mdnsPending = false, txtPending = false, peerRefresh = false;
volatile bool pullPending = false, pullReady = false, syncReady = false;
bool syncDirty = false;
unsigned long syncDirtyAt = 0;
String syncBody, pulledBody;
SemaphoreHandle_t bodyMx;

// Efekt hafızaları
float drop[NUM_LEDS], flake[NUM_LEDS];
float candleCur[NUM_LEDS], candleTgt[NUM_LEDS];
unsigned long candleNext[NUM_LEDS];
float aSun = 0, aShim = 0, aCloud = 0, aCloudY = 0, aCloud2 = 0, aFog = 0, aFogB = 0, aRain = 0;
float aHue = 0, aFlow = 0, aFade = 0, aBreath = 0;



bool previewActive() { return previewCat != WX_NONE && (long)(previewUntil - millis()) > 0; }

// ============================================================
//  YARDIMCI FONKSİYONLAR
// ============================================================
RGBf hex2rgb(uint32_t c) { return {((c >> 16) & 255) / 255.f, ((c >> 8) & 255) / 255.f, (c & 255) / 255.f}; }
uint32_t parseHex(const char* h) { if (*h == '#') h++; return strtoul(h, nullptr, 16) & 0xFFFFFF; }
String hexStr(uint32_t c) { char b[8]; snprintf(b, sizeof b, "#%06X", (unsigned)c); return String(b); }

int modeFromKey(const char* k) { for (int i = 0; i < M_COUNT; i++) if (!strcmp(k, MODE_KEYS[i])) return i; return -1; }
Wx wxFromKey(const char* k) { for (int i = 0; i < WX_COUNT; i++) if (!strcmp(k, WX_KEYS[i])) return (Wx)i; return WX_NONE; }

float angDiff(float a, float b) { float d = fmodf(a - b + PI, TWO_PI); if (d < 0) d += TWO_PI; return d - PI; }

RGBf hsv(float h, float s, float v) {
  h -= floorf(h);
  float i = floorf(h * 6), f = h * 6 - i, p = v * (1 - s), q = v * (1 - f * s), t = v * (1 - (1 - f) * s);
  switch (((int)i) % 6) {
    case 0: return {v, t, p}; case 1: return {q, v, p}; case 2: return {p, v, t};
    case 3: return {p, q, v}; case 4: return {t, p, v}; default: return {v, p, q};
  }
}

// Renk sıcaklığı (Kelvin) → RGB
RGBf kelvin(float K) {
  float t = K / 100.f, r, g, b;
  if (t <= 66) {
    r = 255;
    g = 99.4708025861f * logf(t) - 161.1195681661f;
    b = (t <= 19) ? 0 : 138.5177312231f * logf(t - 10) - 305.0447927307f;
  } else {
    r = 329.698727446f * powf(t - 60, -0.1332047592f);
    g = 288.1221695283f * powf(t - 60, -0.0755148492f);
    b = 255;
  }
  return {constrain(r, 0.f, 255.f) / 255.f, constrain(g, 0.f, 255.f) / 255.f, constrain(b, 0.f, 255.f) / 255.f};
}

// Dikişsiz (256'da tekrarlayan) 2B değer gürültüsü — bulut ve sis dokusu için
static inline float hash2(int x, int y) {
  uint32_t h = (uint32_t)(x & 255) * 374761393u + (uint32_t)(y & 255) * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u; h ^= h >> 16;
  return (h & 0xFFFF) / 65535.f;
}
float vnoise(float x, float y) {
  float fx = floorf(x), fy = floorf(y); int xi = (int)fx, yi = (int)fy;
  float u = x - fx, v = y - fy; u = u * u * (3 - 2 * u); v = v * v * (3 - 2 * v);
  float a = hash2(xi, yi), b = hash2(xi + 1, yi), c = hash2(xi, yi + 1), d = hash2(xi + 1, yi + 1);
  return a + (b - a) * u + (c - a) * v + (a - b - c + d) * u * v;
}
float fbm(float x, float y) { return 0.65f * vnoise(x, y) + 0.35f * vnoise(x * 2 + 17.3f, y * 2 - 9.1f); }

RGBf tri(RGBf a, RGBf b, RGBf c, float p) {
  p -= floorf(p); float s = p * 3; int i = (int)s; float t = s - i; t = t * t * (3 - 2 * t);
  RGBf A = i == 0 ? a : (i == 1 ? b : c), B = i == 0 ? b : (i == 1 ? c : a);
  return mix(A, B, t);
}

bool timeNow(struct tm& t) { time_t n = time(nullptr); if (n < 1700000000) return false; localtime_r(&n, &t); return true; }
float sunMin(long utc, long tz) { long s = ((utc + tz) % 86400 + 86400) % 86400; return s / 60.f; }

// OpenWeatherMap kodu → gökyüzü efekti
Wx classify(int id) {
  if (id >= 200 && id < 300) return WX_SAGANAK;                       // Gök gürültülü fırtına
  if (id >= 300 && id < 400) return WX_YAGMUR;                        // Çisenti
  if (id >= 500 && id < 600) return (id == 500 || id == 501 || id == 511) ? WX_YAGMUR : WX_SAGANAK;
  if (id >= 600 && id < 700) return WX_KAR;
  if (id == 771 || id == 781) return WX_SAGANAK;                      // Bora, hortum
  if (id >= 700 && id < 800) return WX_SIS;                           // Sis, pus, duman
  if (id == 800) return WX_GUNES;
  if (id == 801 || id == 802) return WX_PARCALI;
  if (id == 803 || id == 804) return WX_BULUTLU;
  return WX_NONE;
}
bool isThunder(int id) { return (id >= 200 && id < 300) || id == 771 || id == 781; }

// ============================================================
//  SİRKADİYEN HESAP
// ============================================================

Circ circadian(float now, float sr, float ss) {
  const float dawnS = sr - 30, dawnE = sr + 60, duskS = ss - 90, nightS = ss + 30;
  Circ c; float k, lv;
  if (now < dawnS || now >= nightS) { c.phase = P_GECE; k = 2000; lv = 0.38f; }
  else if (now < dawnE) {                                   // Şafak: sıcak ve loştan aydınlığa
    float t = (now - dawnS) / (dawnE - dawnS);
    c.phase = P_SAFAK; k = 2200 + t * 1800; lv = 0.15f + t * 0.7f;
  } else if (now < duskS) {                                 // Gün: öğlen en soğuk ve en parlak
    float t = (now - dawnE) / max(1.f, duskS - dawnE), s = sinf(PI * t);
    c.phase = P_GUN; k = 4000 + 2200 * s; lv = 0.85f + 0.15f * s;
  } else {                                                  // Alacakaranlık: sıcak ve kısık
    float t = (now - duskS) / (nightS - duskS);
    c.phase = P_ALACAKARANLIK; k = 4000 - t * 1800; lv = 0.85f - t * 0.7f;
  }
  c.base = kelvin(k); c.level = lv;
  return c;
}

// ============================================================
//  EFEKTLER  (tümü bekleme yapmaz, her karede tgt[] doldurur)
// ============================================================
void fill(RGBf c) { for (int i = 0; i < NUM_LEDS; i++) tgt[i] = c; }

// Güneş: halkada yavaşça dolaşan sıcak bir parıltı. cover > 0 ise aradan bulutlar geçer.
void fxGunes(RGBf base, float lv, float cover, float drift) {
  aSun   = wrapf(aSun + dt * TWO_PI / 90.f, TWO_PI);
  aShim  = wrapf(aShim + dt * 0.5f, 256);
  aCloud = wrapf(aCloud + dt * drift, 256);
  aCloudY = wrapf(aCloudY + dt * drift * 0.4f, 256);
  RGBf warm  = mix(base, rgb(1.f, 0.82f, 0.5f), 0.55f);
  RGBf shade = scale(mix(base, rgb(0.7f, 0.75f, 0.85f), 0.6f), 0.38f);
  for (int i = 0; i < NUM_LEDS; i++) {
    float d = angDiff(theta[i], aSun);
    float glow = expf(-(d * d) / 1.1f);
    float sh = 1.f + 0.06f * (vnoise(px[i] * 1.6f + aShim, py[i] * 1.6f) - 0.5f);
    RGBf c = scale(mix(scale(base, 0.62f), warm, glow), sh);
    if (cover > 0) {
      float n = fbm(px[i] * 1.1f + aCloud, py[i] * 1.1f + aCloudY);
      float cl = sstep(1.f - cover, 1.f - cover + 0.2f, n);
      c = mix(c, shade, cl * 0.9f);
    }
    tgt[i] = scale(c, lv);
  }
}

// Parçalı bulutlu: gökyüzünün yaklaşık yarısı bulut, yarısı güneş. Bulutlar rüzgârla akar,
// kenarları güneşle gümüş gibi parlar. Güneş bulutun kenarına geldiğinde aralıklardan
// ışık hüzmeleri süzülür; güneş bulutun arkasına girince oda hafifçe kararır.
float cloudAt(float x, float y, float center) {
  return sstep(center - 0.09f, center + 0.09f, fbm(x * 0.95f + aCloud, y * 0.95f + aCloudY));
}

void fxParcali(RGBf base, float lv, float center, float drift) {
  aSun    = wrapf(aSun + dt * TWO_PI / 120.f, TWO_PI);
  aCloud  = wrapf(aCloud + dt * drift, 256);
  aCloudY = wrapf(aCloudY + dt * drift * 0.35f, 256);
  aRay    = wrapf(aRay + dt * 0.45f, TWO_PI);

  float sunVis = 1.f - cloudAt(cosf(aSun), sinf(aSun), center);     // Güneşin önü ne kadar açık
  float rayStr = sstep(0.15f, 0.55f, sunVis) * (1.f - 0.45f * sstep(0.8f, 1.f, sunVis));
  float roomLv = 0.62f + 0.38f * sunVis;

  RGBf sunC   = mix(base, rgb(1.f, 0.86f, 0.58f), 0.5f);
  RGBf cloudC = scale(mix(base, rgb(0.72f, 0.77f, 0.88f), 0.65f), 0.30f);
  RGBf rimC   = rgb(1.f, 0.95f, 0.85f);

  for (int i = 0; i < NUM_LEDS; i++) {
    float cl  = cloudAt(px[i], py[i], center);
    float rim = 1.f - fabsf(cl - 0.5f) * 2.f; rim *= rim;            // Bulut kenarı
    float d   = angDiff(theta[i], aSun);
    float glow = expf(-(d * d) / 1.4f);
    float beam = powf(0.5f + 0.5f * cosf(4.f * d + aRay), 6.f) * expf(-(d * d) / 3.f);

    RGBf c = mix(scale(sunC, (0.7f + 0.3f * glow) * roomLv), cloudC, cl);
    c = add(c, scale(rimC, 0.28f * rim * roomLv));                        // Gümüş kenar
    c = add(c, scale(rimC, 0.55f * beam * rayStr * (1.f - cl * 0.7f)));   // Işık hüzmeleri
    tgt[i] = scale(c, lv);
  }
}

// Bulutlu: iki katman bulut farklı hızlarda halkanın üzerinden süzülür.
void fxBulutlu(RGBf base, float lv, bool dark, float drift) {
  aCloud  = wrapf(aCloud + dt * drift * 0.8f, 256);
  aCloudY = wrapf(aCloudY + dt * drift * 0.3f, 256);
  aCloud2 = wrapf(aCloud2 + dt * drift * 1.5f, 256);
  RGBf sky = mix(base, rgb(0.8f, 0.84f, 0.92f), 0.6f);
  for (int i = 0; i < NUM_LEDS; i++) {
    float n  = fbm(px[i] * 0.9f + aCloud, py[i] * 0.9f + aCloudY);
    float n2 = vnoise(px[i] * 1.8f + aCloud2, py[i] * 1.8f + 40.f);
    float b = 0.3f + 0.55f * sstep(0.2f, 0.8f, n) - 0.12f * n2;
    if (dark) b *= 0.7f;
    tgt[i] = scale(sky, lv * max(b, 0.08f));
  }
}

// Sis: düşük kontrastlı, sütümsü ve çok yavaş dalgalanan bir perde.
void fxSis(RGBf base, float lv) {
  aFog  = wrapf(aFog + dt * 0.035f, 256);
  aFogB = wrapf(aFogB + dt * TWO_PI / 16.f, TWO_PI);
  RGBf fog = mix(base, rgb(0.85f, 0.86f, 0.9f), 0.7f);
  float br = 0.5f + 0.5f * sinf(aFogB);
  for (int i = 0; i < NUM_LEDS; i++) {
    float n = vnoise(px[i] * 0.6f + aFog, py[i] * 0.6f + 13.7f);
    tgt[i] = scale(fog, lv * (0.4f + 0.12f * n + 0.06f * br));
  }
}

// Yağmur: serin bir zemin üzerine rastgele damlalar düşer ve komşu LED'lere sıçrar.
void rainCore(RGBf base, float lv, float rate, float bgLv) {
  aRain = wrapf(aRain + dt * 0.25f, 256);
  if (randf() < rate * dt) {
    int i = random(NUM_LEDS), l = (i + NUM_LEDS - 1) % NUM_LEDS, r = (i + 1) % NUM_LEDS;
    drop[i] = 1.f; drop[l] = max(drop[l], 0.28f); drop[r] = max(drop[r], 0.28f);
  }
  float k = expf(-dt / 0.15f);
  RGBf bg = mix(base, rgb(0.42f, 0.52f, 0.72f), 0.65f);
  RGBf dc = rgb(0.62f, 0.78f, 1.f);
  for (int i = 0; i < NUM_LEDS; i++) {
    drop[i] *= k;
    float n = vnoise(px[i] * 1.2f + aRain, py[i] * 1.2f);
    tgt[i] = add(scale(bg, lv * bgLv * (0.75f + 0.5f * n)), scale(dc, lv * 0.95f * drop[i]));
  }
}

void updateLightning(bool thunder) {
  unsigned long now = millis();
  if (!L.active) {
    if (L.nextStrike == 0) L.nextStrike = now + random(1500, 5000);
    if ((long)(now - L.nextStrike) >= 0) {
      L.active = true; L.flashes = random(2, 5); L.on = true;
      L.inten = 1.f; L.ang = randf() * TWO_PI; L.until = now + random(25, 70);
    }
  } else if ((long)(now - L.until) >= 0) {
    if (L.on) {
      L.on = false;
      if (--L.flashes <= 0) { L.active = false; L.nextStrike = now + (thunder ? random(3000, 10000) : random(7000, 18000)); }
      else L.until = now + random(50, 170);
    } else {
      L.on = true; L.inten = 0.45f + randf() * 0.55f; L.until = now + random(20, 60);
    }
  }
  if (L.on) L.glow = L.inten; else L.glow *= expf(-dt / 0.07f);
}

// Sağanak: yoğun yağmur + bir noktadan doğup halkaya yayılan şimşekler.
void fxSaganak(RGBf base, float lv, bool thunder) {
  rainCore(base, lv, 13.f, 0.2f);
  updateLightning(thunder);
  if (L.glow > 0.01f) {
    RGBf fl = rgb(0.9f, 0.93f, 1.f);
    float amp = 0.45f + 0.55f * lv;
    for (int i = 0; i < NUM_LEDS; i++) {
      float d = angDiff(theta[i], L.ang);
      float f = L.glow * amp * (0.35f + 0.65f * expf(-(d * d) / 0.9f));
      tgt[i] = mix(tgt[i], fl, clamp01(f));
    }
  }
}

// Kar: yumuşakça belirip kaybolan kar taneleri.
void fxKar(RGBf base, float lv) {
  if (randf() < 2.2f * dt) { int i = random(NUM_LEDS); if (flake[i] < 0) flake[i] = 0; }
  RGBf bg = mix(base, rgb(0.78f, 0.85f, 1.f), 0.6f);
  for (int i = 0; i < NUM_LEDS; i++) {
    float s = 0;
    if (flake[i] >= 0) { flake[i] += dt / 1.8f; if (flake[i] >= 1) flake[i] = -1; else s = sinf(PI * sqrtf(flake[i])); }
    tgt[i] = add(scale(bg, lv * 0.3f), scale(rgb(1, 1, 1), lv * 0.7f * s));
  }
}

// Mum: her LED kendi alevi gibi bağımsız ve yumuşak titrer.
void fxMum(float lv) {
  unsigned long now = millis();
  float k = min(1.f, dt * 12.f);
  for (int i = 0; i < NUM_LEDS; i++) {
    if ((long)(now - candleNext[i]) >= 0) { candleTgt[i] = 0.45f + randf() * 0.55f; candleNext[i] = now + random(60, 160); }
    candleCur[i] += (candleTgt[i] - candleCur[i]) * k;
    tgt[i] = scale(rgb(1.f, 0.30f + 0.16f * candleCur[i], 0.02f), lv * candleCur[i]);
  }
}

void runWeather(Wx cat, RGBf base, float lv, int id, float wind) {
  float drift = 0.05f + constrain(wind, 0.f, 20.f) * 0.012f;   // Rüzgâr arttıkça bulutlar hızlanır
  switch (cat) {
    case WX_GUNES:   fxGunes(base, lv, 0, drift); break;
    case WX_PARCALI: fxParcali(base, lv, id == 801 ? 0.57f : 0.5f, drift); break;   // 802: yarı yarıya
    case WX_BULUTLU: fxBulutlu(base, lv, id == 804, drift); break;
    case WX_SIS:     fxSis(base, lv); break;
    case WX_YAGMUR:  rainCore(base, lv, id < 400 ? 3.f : (id == 501 ? 8.f : 5.f), 0.32f); break;
    case WX_SAGANAK: fxSaganak(base, lv, isThunder(id)); break;
    case WX_KAR:     fxKar(base, lv); break;
    default:         fill(scale(base, lv)); break;
  }
}

void renderSirkadiyen(const Circ& c, Wx cat, int id, float wind) {
  bool night = c.phase == P_GECE;
  if (cat == WX_NONE) { if (night) fxMum(c.level); else fill(scale(c.base, c.level)); return; }
  // Gece sakin havalarda mum ışığı; yağmur, sağanak ve kar ise loş olarak gösterilir.
  if (night && (cat == WX_GUNES || cat == WX_PARCALI || cat == WX_BULUTLU || cat == WX_SIS)) { fxMum(c.level); return; }
  runWeather(cat, night ? kelvin(2200) : c.base, night ? 0.3f : c.level, id, wind);
}

void renderManual() {
  RGBf c0 = hex2rgb(S.col[0]), c1 = hex2rgb(S.col[1]), c2 = hex2rgb(S.col[2]);
  float sp = 0.15f + (S.speed - 1) * 0.2f;
  switch (S.mode) {
    case M_MUM:   fxMum(1.f); break;
    case M_SABIT: fill(c0); break;
    case M_GOKKUSAGI:
      aHue = wrapf(aHue + dt * sp * 0.08f, 1.f);
      for (int i = 0; i < NUM_LEDS; i++) tgt[i] = hsv(aHue + i / (float)NUM_LEDS, 1, 1);
      break;
    case M_AKIS:
      aFlow = wrapf(aFlow + dt * sp * 0.05f, 1.f);
      for (int i = 0; i < NUM_LEDS; i++) tgt[i] = tri(c0, c1, c2, aFlow + i / (float)NUM_LEDS);
      break;
    case M_GECIS:
      aFade = wrapf(aFade + dt * sp * 0.035f, 1.f);
      fill(tri(c0, c1, c2, aFade));
      break;
    case M_NEFES:
      aBreath = wrapf(aBreath + dt * sp * 0.9f, TWO_PI);
      fill(scale(c0, 0.12f + 0.88f * (0.5f - 0.5f * cosf(aBreath))));
      break;
  }
}

// Kurulum sırasında halkanın müşteriye verdiği işaretler
void renderSystem() {
  unsigned long e = millis() - sysSince;
  switch (sys) {
    case SYS_SETUP: {        // Yavaş nefes alan kehribar: "seni bekliyorum"
      aSys = wrapf(aSys + dt * TWO_PI / 3.5f, TWO_PI);
      fill(scale(rgb(1.f, 0.42f, 0.06f), 0.18f + 0.32f * (0.5f - 0.5f * cosf(aSys))));
      break;
    }
    case SYS_CONNECTING: {   // Halkada dönen kuyruklu ışık: "bağlanıyorum"
      aSys = wrapf(aSys + dt * TWO_PI / 1.3f, TWO_PI);
      for (int i = 0; i < NUM_LEDS; i++) {
        float d = aSys - theta[i]; if (d < 0) d += TWO_PI;
        tgt[i] = scale(rgb(1.f, 0.55f, 0.16f), 0.06f + 0.9f * expf(-d * 1.6f));
      }
      break;
    }
    case SYS_OK: {           // Gün doğumu gibi dolan sıcak beyaz: "hazırım"
      float p = e / 1800.f; RGBf w = kelvin(3600);
      for (int i = 0; i < NUM_LEDS; i++) { int k = min(i, NUM_LEDS - i); tgt[i] = scale(w, 0.85f * clamp01(p * 7.f - k)); }
      break;
    }
    case SYS_FAIL: {         // İki yumuşak kızıl nabız: "olmadı, tekrar dene"
      float v = e < 1200 ? powf(0.5f - 0.5f * cosf(e / 600.f * TWO_PI), 2.f) : 0;
      fill(scale(rgb(1.f, 0.18f, 0.04f), 0.15f + 0.75f * v));
      if (e > 1300) { sys = SYS_SETUP; sysSince = millis(); }
      break;
    }
    default: break;
  }
}

// ============================================================
//  KARE ÇİZİMİ
// ============================================================
void renderFrame() {
  WeatherData w = wxSnapshot();
  struct tm t; bool tv = timeNow(t);
  srMin = (w.valid && w.sunrise) ? sunMin(w.sunrise, w.tz) : 420;
  ssMin = (w.valid && w.sunset)  ? sunMin(w.sunset,  w.tz) : 1170;
  float nowMin = tv ? t.tm_hour * 60 + t.tm_min + t.tm_sec / 60.f : 720;
  Circ c = circadian(nowMin, srMin, ssMin);
  curPhase = c.phase;
  Wx cat = w.valid ? classify(w.id) : WX_NONE;

  int key;
  if (holdProgress > 0.01f) {                 // BOOT düğmesine basılı tutuluyor
    key = 3000;
    float n = holdProgress * NUM_LEDS;
    for (int i = 0; i < NUM_LEDS; i++) tgt[i] = scale(rgb(1.f, 0.25f, 0.05f), clamp01(n - i));
  } else if (sys != SYS_NORMAL) {
    key = 2000 + sys; renderSystem();
  } else if (previewActive()) {
    key = 1000 + previewCat;
    runWeather(previewCat, kelvin(5000), 0.9f, PREVIEW_IDS[previewCat], 4);
  } else if (!S.power) {
    key = -1; fill(rgb(0, 0, 0));
  } else if (S.mode == M_SIRKADIYEN) {
    Wx eff = S.wfx ? cat : WX_NONE;
    key = 500 + eff * 10 + (c.phase == P_GECE ? 1 : 0);
    renderSirkadiyen(c, eff, w.id, w.wind);
  } else {
    key = S.mode; renderManual();
  }

  if (key != lastKey) { memcpy(fromBuf, out, sizeof(out)); fadeStart = millis(); lastKey = key; }

  // Geçiş + parlaklık + gama düzeltmesi + akım sınırı
  float f = clamp01((millis() - fadeStart) / (float)FADE_MS); f = f * f * (3 - 2 * f);
  float b = S.bri / 255.f;
  uint8_t R[NUM_LEDS], G[NUM_LEDS], B[NUM_LEDS]; uint32_t sum = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    out[i] = mix(fromBuf[i], tgt[i], f);
    float r = clamp01(out[i].r * b), g = clamp01(out[i].g * b), bl = clamp01(out[i].b * b);
    R[i] = r * r * 255 + 0.5f; G[i] = g * g * 255 + 0.5f; B[i] = bl * bl * 255 + 0.5f;
    sum += R[i] + G[i] + B[i];
  }
  float ma = sum * 20.f / 255.f;
  float k = ma > POWER_LIMIT_MA ? POWER_LIMIT_MA / ma : 1.f;
  for (int i = 0; i < NUM_LEDS; i++)
    pixels.setPixelColor(i, (uint8_t)(R[i] * k), (uint8_t)(G[i] * k), (uint8_t)(B[i] * k));
  pixels.show();

  // Saat dilimi hava servisinden gelir (yurt dışındaki kullanıcılar için)
  if (w.valid && w.tz != S.tz) {
    S.tz = w.tz;
    configTime(S.tz, 0, "pool.ntp.org", "time.google.com");
    markDirty();
  }
}

// ============================================================
//  HAVA DURUMU GÖREVİ (Çekirdek 0'da çalışır, animasyonu dondurmaz)
// ============================================================
String urlEncode(const char* s) {
  String o; char hx[4];
  while (*s) {
    uint8_t c = *s++;
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == ',') o += (char)c;
    else { snprintf(hx, sizeof hx, "%%%02X", c); o += hx; }
  }
  return o;
}

void fetchWeather() {
  if (!strlen(OWM_API_KEY) || !strncmp(OWM_API_KEY, "BURAYA", 6)) {
    portENTER_CRITICAL(&wxMux); strlcpy(wxShared.err, "API anahtarı girilmemiş", sizeof wxShared.err); portEXIT_CRITICAL(&wxMux);
    wxSeq++;
    Serial.println("Hava hatası: OWM_API_KEY tanımlı değil.");
    return;
  }
  char city[48];
  portENTER_CRITICAL(&wxMux); strlcpy(city, cityShared, sizeof city); portEXIT_CRITICAL(&wxMux);

  String url = String("http://api.openweathermap.org/data/2.5/weather?q=") + urlEncode(city) +
               "&appid=" + OWM_API_KEY + "&units=metric&lang=tr";
  WeatherData w; bool ok = false; char err[64] = "";
  HTTPClient http;
  http.setTimeout(7000);
  if (http.begin(url)) {
    int code = http.GET();
    if (code == 200) {
      JsonDocument doc;
      DeserializationError e = deserializeJson(doc, http.getString());
      if (!e && doc["weather"][0]["id"].is<int>()) {
        w.id   = doc["weather"][0]["id"];
        w.temp = doc["main"]["temp"] | 0.0f;
        w.wind = doc["wind"]["speed"] | 3.0f;
        strlcpy(w.desc, doc["weather"][0]["description"] | "", sizeof w.desc);
        strlcpy(w.name, doc["name"] | "", sizeof w.name);
        w.tz      = doc["timezone"] | 10800L;
        w.sunrise = doc["sys"]["sunrise"] | 0L;
        w.sunset  = doc["sys"]["sunset"] | 0L;
        w.valid = true; ok = true;
      } else strlcpy(err, "Hava verisi okunamadı", sizeof err);
    } else if (code == 404) strlcpy(err, "Şehir bulunamadı. Yazımı kontrol et.", sizeof err);
    else if (code == 401)   strlcpy(err, "API anahtarı geçersiz", sizeof err);
    else snprintf(err, sizeof err, "Bağlantı hatası (%d)", code);
    http.end();
  } else strlcpy(err, "Bağlantı kurulamadı", sizeof err);

  portENTER_CRITICAL(&wxMux);
  if (ok) wxShared = w; else strlcpy(wxShared.err, err, sizeof wxShared.err);
  portEXIT_CRITICAL(&wxMux);

  wxSeq++;
  if (ok) Serial.printf("Hava: %s, kod %d, %.1f°C\n", w.name, w.id, w.temp);
  else    Serial.printf("Hava hatası: %s\n", err);
}

// ============================================================
//  ÇOKLU CİHAZ: mDNS, keşif ve senkronizasyon (ağ görevinde çalışır)
// ============================================================
void setupMdns() {
  char tmp[24]; snprintf(tmp, sizeof tmp, "biolight-%s", devId);
  if (!MDNS.begin(tmp)) return;
  vTaskDelay(pdMS_TO_TICKS(random(200, 1500)));   // Elektrik gelince aynı anda açılan cihazlar çakışmasın
  IPAddress other = MDNS.queryHost("biolight", 2000);
  if (other == IPAddress() || other == WiFi.localIP()) {   // "biolight.local" boşta: bu cihaz alır
    MDNS.end();
    MDNS.begin("biolight");
    strlcpy(hostName, "biolight", sizeof hostName);
  } else strlcpy(hostName, tmp, sizeof hostName);
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("biolight", "tcp", 80);
  MDNS.addServiceTxt("biolight", "tcp", "id", (const char*)devId);
  MDNS.addServiceTxt("biolight", "tcp", "name", (const char*)S.name);
  MDNS.addServiceTxt("biolight", "tcp", "sync", S.sync ? "1" : "0");
  mdnsOn = true;
  Serial.printf("Ağ adı: http://%s.local\n", hostName);
}

void discoverPeers() {
  int n = MDNS.queryService("biolight", "tcp");
  Peer found[MAX_PEERS]; uint8_t m = 0;
  for (int i = 0; i < n && m < MAX_PEERS; i++) {
    String id = MDNS.txt(i, "id");
    if (!id.length() || id == devId) continue;
    Peer& p = found[m++];
    strlcpy(p.id, id.c_str(), sizeof p.id);
    strlcpy(p.name, MDNS.txt(i, "name").c_str(), sizeof p.name);
    p.ip = (uint32_t)MDNS.address(i);
    p.sync = MDNS.txt(i, "sync") == "1";
  }
  portENTER_CRITICAL(&peerMux); memcpy(peers, found, sizeof(Peer) * m); peerCount = m; portEXIT_CRITICAL(&peerMux);
}

uint8_t peerSnapshot(Peer* out) {
  portENTER_CRITICAL(&peerMux); uint8_t n = peerCount; memcpy(out, peers, sizeof(Peer) * n); portEXIT_CRITICAL(&peerMux);
  return n;
}

// Bu ışıktaki değişikliği diğer ışıklara gönderir. Senkron kapalı olan ışıklar isteği yok sayar.
void pushSync() {
  String b; xSemaphoreTake(bodyMx, portMAX_DELAY); b = syncBody; xSemaphoreGive(bodyMx);
  Peer list[MAX_PEERS]; uint8_t n = peerSnapshot(list);
  for (int i = 0; i < n; i++) {
    HTTPClient http; http.setTimeout(1500);
    if (!http.begin("http://" + IPAddress(list[i].ip).toString() + "/api/set")) continue;
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Biolight-Sync", "1");
    http.POST(b);
    http.end();
  }
}

// Senkron açılınca gruptaki bir ışığın ayarlarını alır, böylece hepsi aynı durumdan başlar
void pullFromGroup() {
  discoverPeers();
  Peer list[MAX_PEERS]; uint8_t n = peerSnapshot(list);
  for (int i = 0; i < n; i++) {
    if (!list[i].sync) continue;
    HTTPClient http; http.setTimeout(2000);
    if (!http.begin("http://" + IPAddress(list[i].ip).toString() + "/api/state")) continue;
    if (http.GET() == 200) {
      String b = http.getString();
      xSemaphoreTake(bodyMx, portMAX_DELAY); pulledBody = b; xSemaphoreGive(bodyMx);
      pullReady = true;
      Serial.printf("Ayarlar \"%s\" ışığından alındı.\n", list[i].name);
      http.end(); return;
    }
    http.end();
  }
}

void netTask(void*) {
  unsigned long last = 0, lastPeers = 0; bool lastOk = false, first = true;
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (mdnsPending) { mdnsPending = false; setupMdns(); }
      if (mdnsOn && txtPending) {
        txtPending = false;
        MDNS.addServiceTxt("biolight", "tcp", "name", (const char*)S.name);
        MDNS.addServiceTxt("biolight", "tcp", "sync", S.sync ? "1" : "0");
      }
      unsigned long wait = lastOk ? WEATHER_INTERVAL : WEATHER_RETRY;
      if (wxRefresh || first || millis() - last >= wait) {
        wxRefresh = false; first = false;
        fetchWeather();
        last = millis();
        portENTER_CRITICAL(&wxMux); lastOk = wxShared.err[0] == 0; portEXIT_CRITICAL(&wxMux);
      }
      if (mdnsOn && pullPending) { pullPending = false; pullFromGroup(); lastPeers = millis(); }
      if (mdnsOn && (peerRefresh || millis() - lastPeers > 45000UL)) { peerRefresh = false; discoverPeers(); lastPeers = millis(); }
      if (syncReady) { syncReady = false; pushSync(); }
    }
    vTaskDelay(pdMS_TO_TICKS(150));
  }
}

// ============================================================
//  KALICI AYARLAR
// ============================================================
void loadSettings() {
  prefs.begin("biolight", false);
  S.power = prefs.getBool("pwr", true);
  S.mode  = prefs.getUChar("mode", M_SIRKADIYEN); if (S.mode >= M_COUNT) S.mode = M_SIRKADIYEN;
  S.bri   = prefs.getUChar("bri", 200);
  S.speed = prefs.getUChar("spd", 5);
  S.wfx   = prefs.getBool("wfx", true);
  S.col[0] = prefs.getUInt("c0", S.col[0]);
  S.col[1] = prefs.getUInt("c1", S.col[1]);
  S.col[2] = prefs.getUInt("c2", S.col[2]);
  strlcpy(S.city, prefs.getString("city", "Istanbul,TR").c_str(), sizeof S.city);
  S.tz = prefs.getLong("tz", 10800);
  strlcpy(S.name, prefs.getString("name", "Biolight").c_str(), sizeof S.name);
  S.sync = prefs.getBool("sync", false);
  S.ruleCount = prefs.getUChar("rc", 0);
  if (S.ruleCount > MAX_RULES) S.ruleCount = 0;
  if (S.ruleCount) prefs.getBytes("rules", S.rules, sizeof(Rule) * S.ruleCount);
  prefs.end();
}

void saveSettings() {
  prefs.begin("biolight", false);
  prefs.putBool("pwr", S.power);   prefs.putUChar("mode", S.mode);
  prefs.putUChar("bri", S.bri);    prefs.putUChar("spd", S.speed);
  prefs.putBool("wfx", S.wfx);
  prefs.putUInt("c0", S.col[0]);   prefs.putUInt("c1", S.col[1]); prefs.putUInt("c2", S.col[2]);
  prefs.putString("city", S.city); prefs.putLong("tz", S.tz);
  prefs.putString("name", S.name); prefs.putBool("sync", S.sync);
  prefs.putUChar("rc", S.ruleCount);
  if (S.ruleCount) prefs.putBytes("rules", S.rules, sizeof(Rule) * S.ruleCount);
  prefs.end();
  Serial.println("Ayarlar kaydedildi.");
}

// ============================================================
//  ZAMANLAMA
// ============================================================
void checkSchedule() {
  struct tm t; if (!timeNow(t)) return;
  int key = t.tm_yday * 1440 + t.tm_hour * 60 + t.tm_min;
  if (key == lastSchedKey) return;
  lastSchedKey = key;
  for (int i = 0; i < S.ruleCount; i++) {
    const Rule& r = S.rules[i];
    if (r.on && r.h == t.tm_hour && r.m == t.tm_min) {
      if (r.mode < 0) S.power = false; else { S.power = true; S.mode = r.mode; }
      markDirty();
      Serial.printf("Zamanlama: %02d:%02d → %s\n", r.h, r.m, r.mode < 0 ? "kapalı" : MODE_KEYS[r.mode]);
    }
  }
}

// ============================================================
//  WEB SUNUCUSU
// ============================================================
// Senkron ışıklar arasında paylaşılan ayarlar
void fillShared(JsonDocument& d) {
  d["pwr"] = S.power; d["mode"] = MODE_KEYS[S.mode]; d["bri"] = S.bri; d["spd"] = S.speed; d["wfx"] = S.wfx;
  JsonArray c = d["c"].to<JsonArray>(); for (int k = 0; k < 3; k++) c.add(hexStr(S.col[k]));
  JsonArray r = d["sched"].to<JsonArray>();
  for (int i = 0; i < S.ruleCount; i++) {
    JsonObject o = r.add<JsonObject>();
    o["on"] = S.rules[i].on; o["h"] = S.rules[i].h; o["m"] = S.rules[i].m;
    o["mode"] = S.rules[i].mode < 0 ? "kapali" : MODE_KEYS[S.rules[i].mode];
  }
}

void fillDevice(JsonDocument& d) {
  d["id"] = devId; d["name"] = String(S.name); d["host"] = String(hostName); d["sync"] = S.sync;
  JsonArray a = d["peers"].to<JsonArray>();
  Peer list[MAX_PEERS]; uint8_t n = peerSnapshot(list);
  for (int i = 0; i < n; i++) {
    JsonObject o = a.add<JsonObject>();
    o["id"] = String(list[i].id); o["name"] = String(list[i].name);
    o["ip"] = IPAddress(list[i].ip).toString(); o["sync"] = list[i].sync;
  }
}

void sendState() {
  WeatherData w = wxSnapshot();
  struct tm t; bool tv = timeNow(t);
  JsonDocument d;
  fillShared(d);
  fillDevice(d);
  d["city"] = S.city;
  d["tv"] = tv;
  if (tv) { d["now"] = t.tm_hour * 60 + t.tm_min; d["sec"] = t.tm_sec; }
  d["sr"] = srMin; d["ss"] = ssMin;
  d["phase"] = PHASE_KEYS[curPhase]; d["phaseName"] = PHASE_NAMES[curPhase];
  JsonObject x = d["wx"].to<JsonObject>();
  x["ok"] = w.valid; x["id"] = w.id; x["temp"] = w.temp;
  x["desc"] = String(w.desc); x["name"] = String(w.name); x["err"] = String(w.err);
  x["cat"] = WX_KEYS[w.valid ? classify(w.id) : WX_NONE];
  d["preview"] = previewActive() ? WX_KEYS[previewCat] : "";
  d["ip"] = WiFi.localIP().toString();
  String s; serializeJson(d, s);
  server.send(200, "application/json", s);
}

bool readBody(JsonDocument& d) {
  if (deserializeJson(d, server.arg("plain"))) { server.send(400, "text/plain", "Gecersiz istek"); return false; }
  return true;
}

void applySched(JsonArray a) {
  uint8_t n = 0;
  for (JsonObject o : a) {
    if (n >= MAX_RULES) break;
    Rule r;
    r.on = o["on"] | true;
    r.h  = constrain((int)(o["h"] | 0), 0, 23);
    r.m  = constrain((int)(o["m"] | 0), 0, 59);
    const char* mk = o["mode"] | "sirkadiyen";
    if (!strcmp(mk, "kapali")) r.mode = -1;
    else { int m = modeFromKey(mk); r.mode = m < 0 ? M_SIRKADIYEN : m; }
    S.rules[n++] = r;
  }
  S.ruleCount = n;
}

void markSync() { if (S.sync) { syncDirty = true; syncDirtyAt = millis(); } }

// fromSync = true ise istek başka bir Biolight'tan gelmiştir; yalnızca ortak ayarlar uygulanır
void applyFromJson(JsonDocument& d, bool fromSync) {
  bool shared = false;
  if (d["pwr"].is<bool>()) { S.power = d["pwr"]; shared = true; }
  if (d["mode"].is<const char*>()) { int m = modeFromKey(d["mode"]); if (m >= 0) { S.mode = m; if (!fromSync) S.power = true; shared = true; } }
  if (d["bri"].is<int>()) { S.bri = constrain((int)d["bri"], 8, 255); shared = true; }
  if (d["spd"].is<int>()) { S.speed = constrain((int)d["spd"], 1, 10); shared = true; }
  if (d["wfx"].is<bool>()) { S.wfx = d["wfx"]; shared = true; }
  if (d["c"].is<JsonArray>()) {
    JsonArray a = d["c"];
    for (int k = 0; k < 3 && k < (int)a.size(); k++) { const char* h = a[k]; if (h) S.col[k] = parseHex(h); }
    shared = true;
  }
  if (d["sched"].is<JsonArray>()) { applySched(d["sched"].as<JsonArray>()); shared = true; }
  if (!fromSync) {
    if (d["preview"].is<const char*>()) { previewCat = wxFromKey(d["preview"]); previewUntil = millis() + PREVIEW_MS; }
    if (d["name"].is<const char*>()) {
      const char* n = d["name"];
      if (strlen(n)) { strlcpy(S.name, n, sizeof S.name); txtPending = true; }
    }
    if (d["sync"].is<bool>()) {
      bool was = S.sync; S.sync = d["sync"]; txtPending = true;
      if (S.sync && !was) pullPending = true;
    }
    if (shared) markSync();
  }
  markDirty();
}

void handleSet() {
  JsonDocument d; if (!readBody(d)) return;
  bool fromSync = server.hasHeader("X-Biolight-Sync");
  if (fromSync && !S.sync) { server.send(200, "application/json", "{\"ok\":false}"); return; }
  applyFromJson(d, fromSync);
  if (fromSync) server.send(200, "application/json", "{\"ok\":true}");
  else sendState();
}

void handleSched() {
  JsonDocument d; if (!readBody(d)) return;
  if (!d.is<JsonArray>()) { server.send(400, "text/plain", "Liste bekleniyordu"); return; }
  applySched(d.as<JsonArray>());
  markSync();
  markDirty();
  sendState();
}

void handleCity() {
  JsonDocument d; if (!readBody(d)) return;
  const char* c = d["city"] | "";
  if (!strlen(c)) { server.send(400, "text/plain", "Sehir bos"); return; }
  strlcpy(S.city, c, sizeof S.city);
  portENTER_CRITICAL(&wxMux); strlcpy(cityShared, S.city, sizeof cityShared); portEXIT_CRITICAL(&wxMux);
  wxRefresh = true;
  markDirty();
  sendState();
}

void handleWifiReset() {
  server.send(200, "application/json", "{\"ok\":true}");
  delay(600);
  clearWifi();
  ESP.restart();
}

// ============================================================
//  WI-FI BİLGİLERİ
// ============================================================
void loadWifi() {
  prefs.begin("wifi", false);
  strlcpy(savedSsid, prefs.getString("ssid", "").c_str(), sizeof savedSsid);
  strlcpy(savedPass, prefs.getString("pass", "").c_str(), sizeof savedPass);
  prefs.end();
}

void saveWifi(const char* s, const char* p) {
  prefs.begin("wifi", false);
  prefs.putString("ssid", s); prefs.putString("pass", p);
  prefs.end();
  strlcpy(savedSsid, s, sizeof savedSsid); strlcpy(savedPass, p, sizeof savedPass);
}

void clearWifi() {
  prefs.begin("wifi", false); prefs.clear(); prefs.end();
}

// Ağ listesini hazırlar: aynı isimli ağlardan en güçlüsü, güçlüden zayıfa sıralı.
// En güçlü ağın kanalını döndürür (kurulum ağı bu kanalda açılır).
int buildScan(int n) {
  int idx[40], m = 0;
  for (int i = 0; i < n && m < 40; i++) {
    String s = WiFi.SSID(i);
    if (!s.length()) continue;
    bool dup = false;
    for (int j = 0; j < m; j++) if (WiFi.SSID(idx[j]) == s) { dup = true; if (WiFi.RSSI(i) > WiFi.RSSI(idx[j])) idx[j] = i; break; }
    if (!dup) idx[m++] = i;
  }
  for (int i = 1; i < m; i++) {
    int v = idx[i], j = i - 1;
    while (j >= 0 && WiFi.RSSI(idx[j]) < WiFi.RSSI(v)) { idx[j + 1] = idx[j]; j--; }
    idx[j + 1] = v;
  }
  JsonDocument d; JsonArray a = d.to<JsonArray>();
  for (int k = 0; k < m && k < 20; k++) {
    JsonObject o = a.add<JsonObject>();
    o["s"] = WiFi.SSID(idx[k]);
    o["r"] = WiFi.RSSI(idx[k]);
    o["l"] = WiFi.encryptionType(idx[k]) != WIFI_AUTH_OPEN;
  }
  scanJson = ""; serializeJson(d, scanJson);
  return m ? WiFi.channel(idx[0]) : 1;
}

// ============================================================
//  KARE ZAMANLAYICI
// ============================================================
void tick() {
  unsigned long now = millis();
  if (now - lastFrame >= FRAME_MS) {
    dt = min((now - lastFrame) / 1000.f, 0.1f);
    lastFrame = now;
    renderFrame();
  }
}

// ============================================================
//  KURULUM PORTALI
// ============================================================
void startNetworkServices() {
  configTime(S.tz, 0, "pool.ntp.org", "time.google.com");
  if (!mdnsOn) mdnsPending = true;
  wxRefresh = true; peerRefresh = true;
  Serial.println("Kontrol paneli: http://" + WiFi.localIP().toString());
}

void startPortal() {
  portalMode = true; ps = PS_IDLE;
  sys = SYS_SETUP; sysSince = millis();
  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);

  // Ağları sayfa açılmadan önce tara: müşteri listeyi beklemeden görür
  int ch = 1;
  if (WiFi.scanNetworks(true) == WIFI_SCAN_RUNNING) {
    unsigned long t0 = millis();
    while (WiFi.scanComplete() == WIFI_SCAN_RUNNING && millis() - t0 < 8000) { tick(); delay(1); }
    int n = WiFi.scanComplete();
    if (n > 0) ch = buildScan(n);
    WiFi.scanDelete();
  }

  // Kurulum ağı en güçlü ağın kanalında açılır; böylece bağlanırken kanal değişmez
  // ve telefonun kurulum ağından düşme ihtimali azalır.
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_NAME, nullptr, ch);
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", WiFi.softAPIP());
  lastSavedTry = millis();
  Serial.printf("Kurulum modu: \"%s\" ağı açıldı (kanal %d)\n", AP_NAME, ch);
}

void leavePortal() {
  dns.stop();
  WiFi.softAPdisconnect(true);
  portalMode = false; ps = PS_IDLE;
  startNetworkServices();
  sys = SYS_OK; sysSince = millis();
  Serial.println("Kurulum ağı kapatıldı.");
}

void portalFail(const char* msg) {
  WiFi.disconnect();
  ps = PS_FAIL;
  strlcpy(psErr, msg, sizeof psErr);
  sys = SYS_FAIL; sysSince = millis();
  Serial.printf("Kurulum hatası: %s\n", msg);
}

void portalLoop() {
  dns.processNextRequest();

  if (scanBusy) {
    int n = WiFi.scanComplete();
    if (n >= 0) { buildScan(n); WiFi.scanDelete(); scanBusy = false; }
    else if (n == WIFI_SCAN_FAILED) scanBusy = false;
  }

  unsigned long e = millis() - psStart;
  int r = lastDiscReason;
  char msg[112];
  switch (ps) {
    case PS_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        ps = PS_CHECKING; psStart = millis();
        saveWifi(pSsid, pPass);
        configTime(S.tz, 0, "pool.ntp.org", "time.google.com");
        wxSeqMark = wxSeq; wxRefresh = true;
        if (!mdnsOn) mdnsPending = true;
        Serial.println("Ev ağına bağlandı: " + WiFi.localIP().toString());
      } else if (r == 15 || r == 202 || r == 204) {
        portalFail("Şifre hatalı görünüyor. Kontrol edip tekrar dene.");
      } else if ((r == 201 || (r >= 210 && r <= 212)) && e > 8000) {
        snprintf(msg, sizeof msg, "“%s” ağına ulaşılamadı. Biolight'ı modeme biraz yaklaştırıp tekrar dene.", pSsid);
        portalFail(msg);
      } else if (e > 25000) {
        portalFail("Bağlantı kurulamadı. Şifreyi kontrol edip tekrar dene.");
      }
      break;
    case PS_CHECKING:       // İnternet ve şehir, ilk hava durumu sorgusuyla doğrulanır
      if (wxSeq != wxSeqMark || e > 12000) { ps = PS_DONE; doneAt = millis(); sys = SYS_OK; sysSince = millis(); }
      break;
    case PS_DONE:           // Müşteri "Kurulumu tamamla"ya basmazsa 5 dakika sonra kendiliğinden kapanır
      if (!finishAt && millis() - doneAt > 300000UL) finishAt = millis();
      break;
    default: break;
  }

  if (finishAt && (long)(millis() - finishAt) >= 0) { finishAt = 0; leavePortal(); return; }

  // Elektrik kesintisinden sonra modem geç açıldıysa: kayıtlı ağ geri gelince kurulumdan kendiliğinden çık
  if (ps == PS_IDLE && savedSsid[0]) {
    if (WiFi.status() == WL_CONNECTED) leavePortal();
    else if (millis() - lastSavedTry > 60000UL && WiFi.softAPgetStationNum() == 0) {
      lastSavedTry = millis();
      WiFi.begin(savedSsid, savedPass);
    }
  }
}

// --- Kurulum sayfasının API uçları ---
void handleScan() {
  if (server.hasArg("yenile") && !scanBusy && ps != PS_CONNECTING && ps != PS_CHECKING)
    scanBusy = WiFi.scanNetworks(true) == WIFI_SCAN_RUNNING;
  server.send(200, "application/json", String("{\"scanning\":") + (scanBusy ? "true" : "false") + ",\"list\":" + scanJson + "}");
}

void handleConnect() {
  JsonDocument d; if (!readBody(d)) return;
  const char* s = d["ssid"] | "";
  if (!strlen(s)) { server.send(400, "text/plain", "Ag adi bos"); return; }
  strlcpy(pSsid, s, sizeof pSsid);
  strlcpy(pPass, d["pass"] | "", sizeof pPass);
  const char* nm = d["name"] | "";
  if (strlen(nm)) { strlcpy(S.name, nm, sizeof S.name); markDirty(); }
  const char* c = d["city"] | "";
  if (strlen(c)) {
    strlcpy(S.city, c, sizeof S.city);
    portENTER_CRITICAL(&wxMux); strlcpy(cityShared, S.city, sizeof cityShared); portEXIT_CRITICAL(&wxMux);
    markDirty();
  }
  server.send(200, "application/json", "{\"ok\":true}");

  if (scanBusy) { WiFi.scanDelete(); scanBusy = false; }
  lastDiscReason = 0; psErr[0] = 0;
  WiFi.disconnect(); delay(50);
  WiFi.begin(pSsid, pPass);
  ps = PS_CONNECTING; psStart = millis();
  sys = SYS_CONNECTING; sysSince = millis();
  Serial.printf("\"%s\" ağına bağlanılıyor...\n", pSsid);
}

void handleSetupStatus() {
  static const char* STEPS[] = {"idle", "connecting", "checking", "done", "fail"};
  WeatherData w = wxSnapshot();
  JsonDocument d;
  d["step"] = STEPS[ps];
  d["err"]  = String(psErr);
  d["ssid"] = String(pSsid);
  d["city"] = String(S.city);
  d["ip"]   = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String("");
  JsonObject x = d["wx"].to<JsonObject>();
  x["ok"] = w.valid; x["name"] = String(w.name); x["temp"] = w.temp;
  x["desc"] = String(w.desc); x["err"] = String(w.err);
  d["sr"] = (w.valid && w.sunrise) ? sunMin(w.sunrise, w.tz) : 420.f;
  d["ss"] = (w.valid && w.sunset)  ? sunMin(w.sunset,  w.tz) : 1170.f;
  fillDevice(d);
  String o; serializeJson(d, o);
  server.send(200, "application/json", o);
}

void handleFinish() {
  server.send(200, "application/json", "{\"ok\":true}");
  if (ps == PS_DONE) finishAt = millis() + 1500;
}

// BOOT düğmesine 5 saniye basılı tutmak Wi-Fi ayarlarını siler (halka kızıl dolar)
void handleResetButton() {
  if (digitalRead(RESET_BTN) == LOW) {
    if (!pressStart) pressStart = millis();
    unsigned long held = millis() - pressStart;
    holdProgress = held < 800 ? 0 : clamp01((held - 800) / (float)(RESET_HOLD_MS - 800));
    if (held >= RESET_HOLD_MS) {
      Serial.println("Wi-Fi ayarları sıfırlanıyor...");
      clearWifi();
      for (int i = 0; i < NUM_LEDS; i++) pixels.setPixelColor(i, 60, 8, 0);
      pixels.show(); delay(400);
      ESP.restart();
    }
  } else { pressStart = 0; holdProgress = 0; }
}

// ============================================================
//  BAŞLANGIÇ
// ============================================================
void setupRoutes() {
  static const char* hdrs[] = {"X-Biolight-Sync"};
  server.collectHeaders(hdrs, 1);
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/html; charset=utf-8", portalMode ? KURULUM_HTML : INDEX_HTML);
  });
  server.on("/favicon.svg", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "image/svg+xml", FAVICON_SVG);
  });
  // Kontrol paneli
  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/set", HTTP_POST, handleSet);
  server.on("/api/sched", HTTP_POST, handleSched);
  server.on("/api/city", HTTP_POST, handleCity);
  server.on("/api/wifireset", HTTP_POST, handleWifiReset);
  // Kurulum
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/connect", HTTP_POST, handleConnect);
  server.on("/api/setup-status", HTTP_GET, handleSetupStatus);
  server.on("/api/finish", HTTP_POST, handleFinish);
  // Telefonların "internet var mı" kontrolleri dahil bilinmeyen her adres ana sayfaya yönlenir;
  // bu sayede kurulum sayfası LAVA_Setup ağına bağlanınca kendiliğinden açılır.
  server.onNotFound([]() {
    server.sendHeader("Location", portalMode ? "http://192.168.4.1/" : "/");
    server.send(302, "text/plain", "");
  });
}

void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.setBrightness(255);
  pixels.clear(); pixels.show();
  pinMode(RESET_BTN, INPUT_PULLUP);

  loadSettings();
  loadWifi();
  bodyMx = xSemaphoreCreateMutex();
  snprintf(devId, sizeof devId, "%06x", (unsigned)((ESP.getEfuseMac() >> 24) & 0xFFFFFF));
  Serial.printf("Biolight \"%s\" (kimlik %s)\n", S.name, devId);
  strlcpy(cityShared, S.city, sizeof cityShared);
  for (int i = 0; i < NUM_LEDS; i++) {
    theta[i] = i * TWO_PI / NUM_LEDS; px[i] = cosf(theta[i]); py[i] = sinf(theta[i]);
    flake[i] = -1; candleCur[i] = 0.7f; candleTgt[i] = 0.7f; candleNext[i] = 0; drop[i] = 0;
  }
  lastFrame = millis();

  WiFi.persistent(false);
  WiFi.setHostname("Biolight");
  WiFi.onEvent([](WiFiEvent_t e, WiFiEventInfo_t info) {
    if (e == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) lastDiscReason = info.wifi_sta_disconnected.reason;
  });
  WiFi.mode(WIFI_STA);

  setupRoutes();
  xTaskCreatePinnedToCore(netTask, "ag", 10240, nullptr, 1, nullptr, 0);

  bool ok = false;
  if (savedSsid[0]) {
    Serial.printf("\"%s\" ağına bağlanılıyor...\n", savedSsid);
    WiFi.begin(savedSsid, savedPass);
    sys = SYS_CONNECTING; sysSince = millis();
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) { tick(); delay(1); }
    ok = WiFi.status() == WL_CONNECTED;
  }

  if (ok) {
    Serial.println("WiFi bağlandı! IP: " + WiFi.localIP().toString());
    startNetworkServices();
    sys = SYS_OK; sysSince = millis();
  } else {
    startPortal();
  }
  server.begin();
}

// ============================================================
//  ANA DÖNGÜ
// ============================================================
void loop() {
  server.handleClient();

  if (portalMode) portalLoop();
  else if (sys == SYS_OK && millis() - sysSince > 2600) sys = SYS_NORMAL;   // Açılış ışığından sonra normal moda geç

  handleResetButton();
  tick();
  checkSchedule();

  // Değişiklikleri 250 ms biriktirip senkron ışıklara tek seferde gönder (kaydırıcılar için)
  if (syncDirty && millis() - syncDirtyAt > 250) {
    syncDirty = false;
    JsonDocument d; fillShared(d);
    String b; serializeJson(d, b);
    xSemaphoreTake(bodyMx, portMAX_DELAY); syncBody = b; xSemaphoreGive(bodyMx);
    syncReady = true;
  }
  if (pullReady) {
    pullReady = false;
    String b; xSemaphoreTake(bodyMx, portMAX_DELAY); b = pulledBody; xSemaphoreGive(bodyMx);
    JsonDocument d;
    if (!deserializeJson(d, b)) applyFromJson(d, true);
  }

  if (settingsDirty && millis() - dirtyAt > 2500) { saveSettings(); settingsDirty = false; }
  delay(1);   // Tek çekirdekli C3'te Wi-Fi ve hava durumu görevine nefes aldırır
}
