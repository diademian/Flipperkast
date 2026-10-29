// ESP32 code voor een flipperkast controller.
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>

const int SDA_PIN = 21;
const int SCL_PIN = 22;

const int SENSOR_50_PUNTEN = 14;
const int SENSOR_30_PUNTEN = 27; 
const int SENSOR_GAME_OVER = 12;

const int AFSCHIET_KNOP = 32;
const int FLIPPER_KNOP_L = 35;
const int FLIPPER_KNOP_R = 34;

const int AFSCHIET_CILINDER = 33;
const int FLIPPER_L = 25;
const int FLIPPER_R = 26;

const int SENSOR_ACTIEF = HIGH;
const int KNOP_ACTIEF = LOW;
const int VENTIEL_AAN = HIGH;
const int VENTIEL_UIT = LOW;

const unsigned long SENSOR_COOLDOWN_MS = 300;

const char* TOEMOEDIGEN[] = {
  "Goed bezig!!",
  "Ga zo door!!",
  "Zo mooi!!",
  "Fantastisch!!",
  "Super!!!",
  "Geweldig!",
  "Top!!"
};

const int AANTAL_ZINNEN = sizeof(TOEMOEDIGEN) / sizeof(TOEMOEDIGEN[0]);

LiquidCrystal_I2C lcd(0x27, 16, 4);
Preferences opslag;

int punten = 0;
int highscore = 0;

bool sensor50WasActief = false;
bool sensor30WasActief = false;
bool gameOverWasActief = false;

unsigned long laatsteSensor50Hit = 0;
unsigned long laatsteSensor30Hit = 0;
unsigned long laatsteGameOverHit = 0;

void printRegel(byte rij, const char* tekst)
{
  lcd.setCursor(0, rij);

  for (byte kolom = 0; kolom < 16; kolom++)
  {
    if (tekst[kolom] == '\0')
    {
      for (; kolom < 16; kolom++)
      {
        lcd.print(' ');
      }
      return;
    }

    lcd.print(tekst[kolom]);
  }
}

void printWaardeRegel(byte rij, const char* label, int waarde)
{
  char buffer[17];
  snprintf(buffer, sizeof(buffer), "%s%d", label, waarde);
  printRegel(rij, buffer);
}

void toonStartscherm()
{
  printRegel(0, "Welkom op het");
  printRegel(1, "ROC Mondriaan");
  printWaardeRegel(2, "High: ", highscore);
  printRegel(3, "Druk afschiet");
}

void toonScore(const char* bericht)
{
  printWaardeRegel(0, "Score: ", punten);
  printWaardeRegel(1, "High: ", highscore);
  printRegel(2, bericht);
  printRegel(3, "");
}

bool nieuweTrigger(int pin, bool& wasActief, unsigned long& laatsteHit, unsigned long cooldown)
{
  bool actief = digitalRead(pin) == SENSOR_ACTIEF;
  unsigned long nu = millis();
  bool getriggerd = actief && !wasActief && (laatsteHit == 0 || nu - laatsteHit >= cooldown);

  if (getriggerd)
  {
    laatsteHit = nu;
  }

  wasActief = actief;
  return getriggerd;
}

void voegPuntenToe(int aantal)
{
  int zin = random(0, AANTAL_ZINNEN);
  punten += aantal;
  toonScore(TOEMOEDIGEN[zin]);
}

void controleerPunten()
{
  if (nieuweTrigger(SENSOR_50_PUNTEN, sensor50WasActief, laatsteSensor50Hit, SENSOR_COOLDOWN_MS))
  {
    voegPuntenToe(50);
  }

  if (nieuweTrigger(SENSOR_30_PUNTEN, sensor30WasActief, laatsteSensor30Hit, SENSOR_COOLDOWN_MS))
  {
    voegPuntenToe(30);
  }
}

void zetAlleVentielenUit()
{
  digitalWrite(AFSCHIET_CILINDER, VENTIEL_UIT);
  digitalWrite(FLIPPER_L, VENTIEL_UIT);
  digitalWrite(FLIPPER_R, VENTIEL_UIT);
}

void controleerGameOver()
{
  if (!nieuweTrigger(SENSOR_GAME_OVER, gameOverWasActief, laatsteGameOverHit, SENSOR_COOLDOWN_MS))
  {
    return;
  }

  bool nieuweHighscore = punten > highscore;

  if (nieuweHighscore)
  {
    highscore = punten;
    opslag.putInt("highscore", highscore);
  }

  zetAlleVentielenUit();

  printRegel(0, "Game Over");
  printWaardeRegel(1, "Score: ", punten);
  printWaardeRegel(2, "High: ", highscore);
  printRegel(3, nieuweHighscore ? "Nieuwe highscore" : "Probeer opnieuw");

  punten = 0;
  delay(2000);
  toonStartscherm();
}

void afschieten()
{
  bool knopIngedrukt = digitalRead(AFSCHIET_KNOP) == KNOP_ACTIEF;
  digitalWrite(AFSCHIET_CILINDER, knopIngedrukt ? VENTIEL_AAN : VENTIEL_UIT);
}

void flipperen()
{
  bool linksIngedrukt = digitalRead(FLIPPER_KNOP_L) == KNOP_ACTIEF;
  bool rechtsIngedrukt = digitalRead(FLIPPER_KNOP_R) == KNOP_ACTIEF;

  digitalWrite(FLIPPER_L, linksIngedrukt ? VENTIEL_AAN : VENTIEL_UIT);
  digitalWrite(FLIPPER_R, rechtsIngedrukt ? VENTIEL_AAN : VENTIEL_UIT);
}

void setup()
{
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  pinMode(SENSOR_50_PUNTEN, INPUT);
  pinMode(SENSOR_30_PUNTEN, INPUT);
  pinMode(SENSOR_GAME_OVER, INPUT);

  // Volgens het schema schakelen de knoppen naar GND.
  pinMode(AFSCHIET_KNOP, INPUT_PULLUP);

  // GPIO34 en GPIO35 hebben geen interne pull-up/down.
  // In het schema hebben deze knoppen externe pull-up weerstanden naar 3V3.
  pinMode(FLIPPER_KNOP_L, INPUT);
  pinMode(FLIPPER_KNOP_R, INPUT);

  pinMode(AFSCHIET_CILINDER, OUTPUT);
  pinMode(FLIPPER_L, OUTPUT);
  pinMode(FLIPPER_R, OUTPUT);
  zetAlleVentielenUit();

  opslag.begin("flipper", false);
  highscore = opslag.getInt("highscore", 0);

  randomSeed(micros());

  lcd.init();
  lcd.backlight();
  lcd.clear();
  toonStartscherm();
}

void loop()
{
  flipperen();
  afschieten();
  controleerPunten();
  controleerGameOver();
}
