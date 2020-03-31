// Includes
#include <SPI.h>
#include <OPL2.h>

/** 
 * configurations 
 */
 
// core
#define LOOP_DELAY 50           // delay in the main loop per iteration

// channels
#define NONE -1                 // the value of no key pressed

// notes
#define BASE_NOTE 36            // the note the synthesizer starts at (C3)
#define LOWEST_NOTE 0           // the lowest note possible (C0)
#define HIGHEST_NOTE 96         // the highest note possible (C8)

// pins
#define FIRST_KEY_PIN 2         // first piano key pin
#define MOVE_UP_1_PIN 6         // the pin of the key wich highers the base note by 1
#define MOVE_DOWN_1_PIN 5       // the pin of the key wich lowers the base note by 1

// amounts
#define AMOUNT_OF_KEYS 3        // amount of piano keys
#define AMOUNT_OF_EXTRA_KEYS 2  // extra keys with other functionalities (moving up and down)
#define AMOUNT_OF_CHANNELS 8    // the amount of channels
#define AMOUNT_OF_SETTINGS 5    // the amount of settings

/**
 * @struct channel
 * @description the channel struct manages notes played and keys pressed.
 */
typedef struct {

  int id;
  int pressedKey;
  
  bool playing;
} Channel;

// base 
OPL2 opl2;
byte base = BASE_NOTE;
Channel channels[AMOUNT_OF_CHANNELS];

// settings
bool tremolo = true;    // currently can't be changed
bool vibrato = true;    // currently can't be changed
byte settings[AMOUNT_OF_SETTINGS] = {0x08, 0x0A, 0x04, 0x04, 0x0F};

/**
 * @function setup
 * @description this function setups the synthesizer. initializing the OPL2, starting the serial, initializing the settings, 
 * initializing the channels and making the keys ready to press.
 */
void setup() {
  
  opl2.init();

  Serial.begin(9600);

  manageSettings();

  for (byte i = 0; i < AMOUNT_OF_CHANNELS; i++)
    channels[i] = (Channel) { i, NONE, false };
 
  for (byte i = 0; i < AMOUNT_OF_KEYS + AMOUNT_OF_EXTRA_KEYS; i++)
    pinMode(i + FIRST_KEY_PIN, OUTPUT);
}

/**
 * @function loop
 * @description this function gets triggered every (LOOP_DELAY) and checks if keys are pressed and/or any settings have changed
 */
void loop() {

  manageSettings();
  manageKeys();

  delay(LOOP_DELAY);
}

/**
 * @function manageSettings
 * @description this function reads out the settings with the serial ports and sets these in the not played channels.
 */
void manageSettings() {

  for (byte i = 0; i < AMOUNT_OF_SETTINGS; i++) {

    byte analog = analogRead(i);
    byte value = analog / 64;

    if (settings[i] != value && analog != 0) {

      settings[i] = value;
      
      Serial.println(String(value) + " on " + String(i));
    }
  }

  for (byte i = 0; i < AMOUNT_OF_CHANNELS; i++)
    if (!channels[i].playing) {
      opl2.setTremolo   (i, CARRIER, tremolo);
      opl2.setVibrato   (i, CARRIER, vibrato);
      opl2.setMultiplier(i, CARRIER, settings[0]);
      opl2.setAttack    (i, CARRIER, settings[1]);
      opl2.setDecay     (i, CARRIER, settings[2]);
      opl2.setSustain   (i, CARRIER, settings[3]);
      opl2.setRelease   (i, CARRIER, settings[4]); 
    }
}

/**
 * @function manageKeys
 * @description this function manages the keys. the relation between key and channel is quite weird. when a key gets pressed it starts 
 * playing over a certain channel. No other key is allowed to be played over the channel as long as the original key is pressed. therefore
 * the number of the key is saved on the channel and used for controle. the getChannel function will always return a channel with -1 as key
 * or the channel with the number of the key.
 */
void manageKeys() {

  for (byte key = 0; key < AMOUNT_OF_KEYS; key++) {

      Channel channel = getChannel(key);

      if (digitalRead(key + FIRST_KEY_PIN) == HIGH && channel.pressedKey == NONE) {

        playNoteFromBase(channel.id, key);
        channels[channel.id].pressedKey = key;
        
        Serial.println(" key " + String(key) + " pressed!");
        
      } else if (digitalRead(key + FIRST_KEY_PIN) == LOW && channel.pressedKey == key) {

        channels[channel.id].pressedKey = NONE;

        Serial.println("key " + String(key) + " released!");
      }        
  }

  if (digitalRead(MOVE_DOWN_1_PIN) == HIGH) altBase(-1); // move base one note down
  if (digitalRead(MOVE_UP_1_PIN) == HIGH) altBase(1);  // move base one note up
}

/**
 * @function altBase
 * @param int alt
 * @description this function alters the base note with its 2 limits. these are given in the configurations. the highest limit gets reduced by
 * the amount of keys present. otherwise the offset of a key pressed could push beyond this limit.
 */
void altBase(int alt) {

  if ((base + alt) >= LOWEST_NOTE && 
      (base + alt) < (HIGHEST_NOTE - AMOUNT_OF_KEYS)) {

    base += alt;
  }
}

/**
 * @function playNoteFromBase
 * @param byte channel
 * @param byte offset
 * @description plays a note from the base. the offset is given to each key. this will then play the note + the amount of notes it gets offset. 
 * magine pressing key 1, it will play the C3 (0 offset), and key 3, it will play a D3 (2 offset).
 */
void playNoteFromBase(byte channel, byte offset) {

  byte note = base + offset;

  byte noteInScale = note % 12;
  byte octive = (note - noteInScale) / 12;

  opl2.playNote(channel, octive, noteInScale);
}

/**
 * @function getChannel
 * @param byte key
 * @return Channel
 * @description this function will return the correct channel. At first, it will look if it can find a channel with the same key. if it finds
 * nothing, it will look for a channel that is still empty and has zo key. If that fails too it will simply return the first one, wich most will
 * likely contain data saying it is already in use.
 */
Channel getChannel(byte key) {

  for (int a = 0; a < AMOUNT_OF_CHANNELS; a++)
    if (channels[a].pressedKey == key && !channels[a].playing)
      return channels[a];

  for (int b = 0; b < AMOUNT_OF_CHANNELS; b++)
    if (channels[b].pressedKey == NONE && !channels[b].playing)
      return channels[b];

  return channels[0];
}
