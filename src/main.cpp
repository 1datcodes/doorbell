#include <Arduino.h>
#include <SD.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

#define REED_PIN 14
#define SD_CS_PIN 5

AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;

bool lastDoorState = HIGH;
bool playing = false;
unsigned long lastTrigger = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(REED_PIN, INPUT_PULLUP);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card Mount Failed");
    while (true);
  }
  Serial.println("SD Card Mounted");

  out = new AudioOutputI2S();
  out->SetPinout(26, 25, 22); // BCLK, WCLK, DOUT
  out->SetGain(0.8);

  mp3 = new AudioGeneratorMP3();

  randomSeed(analogRead(0));
}

void playRandomMP3() {
  File root = SD.open("/");
  int fileCount = 0;

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    if (!entry.isDirectory() && String(entry.name()).endsWith(".mp3")) {
      fileCount++;
    }
    entry.close();
  }
  root.close();

  if (fileCount == 0) {
    Serial.println("No MP3 files found");
    return;
  }

  int chosen = random(1, fileCount + 1);
  Serial.printf("Playing track #%d of %d\n", chosen, fileCount);

  root = SD.open("/");
  int currentIndex = 0;
  String chosenName;
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    if (!entry.isDirectory() && String(entry.name()).endsWith(".mp3")) {
     currentIndex++;
      if (currentIndex == chosen) {
        chosenName = "/" + String(entry.name());
        break;
      }
    }
    entry.close();
  }
  root.close();

  if (chosenName.length() > 0) {
    Serial.printf("Playing: %s\n", chosenName.c_str());
    file = new AudioFileSourceSD(chosenName.c_str());
    mp3->begin(file, out);
    playing = true;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  bool doorOpen = digitalRead(REED_PIN);

  if (doorOpen != lastDoorState && millis() - lastTrigger > 500) {
    lastTrigger = millis();
    lastDoorState = doorOpen;

    if (doorOpen) {
      Serial.println("Door opened");
      playRandomMP3();
    } else {
      Serial.println("Door closed");
    }
  }

  if (playing && mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      delete file;
      playing = false;
      Serial.println("Playback finished");
    }
  }
}