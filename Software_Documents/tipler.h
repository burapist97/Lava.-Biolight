#pragma once
// BIOLIGHT veri türleri
// Bu dosya ayrı tutuluyor çünkü Arduino IDE fonksiyon tanımlarını otomatik olarak
// dosyanın başına taşır; türler burada olunca her zaman önce tanımlanmış olurlar.
#include <Arduino.h>

struct RGBf { float r, g, b; };
static inline RGBf rgb(float r, float g, float b) { return {r, g, b}; }
static inline RGBf mix(RGBf a, RGBf b, float t) { return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t}; }
static inline RGBf scale(RGBf a, float s) { return {a.r * s, a.g * s, a.b * s}; }
static inline RGBf add(RGBf a, RGBf b) { return {a.r + b.r, a.g + b.g, a.b + b.b}; }
static inline float clamp01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
static inline float sstep(float e0, float e1, float x) { float t = clamp01((x - e0) / (e1 - e0)); return t * t * (3 - 2 * t); }
static inline float randf() { return (esp_random() & 0xFFFFFF) / 16777216.0f; }
static inline float wrapf(float x, float m) { return x >= m ? x - m : x; }

enum Mode { M_SIRKADIYEN, M_MUM, M_SABIT, M_GOKKUSAGI, M_AKIS, M_GECIS, M_NEFES, M_COUNT };
enum Wx { WX_NONE, WX_GUNES, WX_PARCALI, WX_BULUTLU, WX_SIS, WX_YAGMUR, WX_SAGANAK, WX_KAR, WX_COUNT };
enum Phase { P_SAFAK, P_GUN, P_ALACAKARANLIK, P_GECE };

#define MAX_RULES 10
struct Rule { bool on; uint8_t h, m; int8_t mode; };   // mode -1 = kapalı

struct Settings {
  bool     power = true;
  uint8_t  mode  = M_SIRKADIYEN;
  uint8_t  bri   = 200;
  uint32_t col[3] = {0xFF8C00, 0xFF2D55, 0x5E5CE6};
  uint8_t  speed = 5;
  bool     wfx   = true;
  char     city[48] = "Istanbul,TR";
  long     tz    = 10800;
  Rule     rules[MAX_RULES];
  uint8_t  ruleCount = 0;
  char     name[32] = "Biolight";   // Odanın adı (örn. Salon)
  bool     sync = false;            // Diğer Biolight'larla birlikte çalış
};

struct WeatherData {
  bool  valid = false;
  int   id = 800;
  float temp = 0, wind = 3;
  char  desc[48] = "";
  char  name[48] = "";
  long  tz = 10800;
  long  sunrise = 0, sunset = 0;
  char  err[64] = "";
};

struct Lightning { bool active = false, on = false; int flashes = 0; unsigned long until = 0, nextStrike = 0; float ang = 0, inten = 0, glow = 0; };

struct Circ { Phase phase; RGBf base; float level; };

// Kurulum sırasında halkanın gösterdiği durumlar
enum SysState { SYS_NORMAL, SYS_SETUP, SYS_CONNECTING, SYS_OK, SYS_FAIL };
// Kurulum sayfasının adımları
enum PortalStep { PS_IDLE, PS_CONNECTING, PS_CHECKING, PS_DONE, PS_FAIL };

// Aynı ağdaki diğer Biolight'lar
#define MAX_PEERS 8
struct Peer { char id[8]; char name[32]; uint32_t ip; bool sync; };
