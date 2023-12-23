#include <SPI.h>
#include <MFRC522.h>
// #include <Servo.h>

#define RST_PIN         5        // Configurable, see typical pin layout above
#define SS_PIN          10         // Configurable, see typical pin layout above
#define buzzerPin A1
#define ledGPin A2
#define ledRPin A3
int pos = 0;

int relay = 2;
int value = 0;
bool isAllowed = false;

int firstI = -1;

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

String receivedString = ""; // Initialize with an empty string
bool isRemotelyOpened[7]; // Array to store conditions
String cardIDs[7];

void setup() {
  Serial.begin(9600);
  pinMode(relay, OUTPUT);
  pinMode(ledGPin, OUTPUT);
  pinMode(ledRPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(relay, HIGH);

  Serial.begin(9600);        // Initialize serial communications with the PC
  while (!Serial);        // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();            // Init SPI bus
  mfrc522.PCD_Init();        // Init MFRC522
  delay(4);                // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();    // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

void loop() {
  // Read new string from Serial
  digitalWrite(ledGPin, LOW);
  digitalWrite(ledRPin, LOW);

  if (Serial.available() > 0) {
    String newString = Serial.readStringUntil('\n');
    
    // Check if the new string is different from the previous one
    if (newString != receivedString) {
      receivedString = newString;
      
      // Run the code to process the received string
      processReceivedString(receivedString);
    }
  }

  for (int i = 0; i < 7; i++) {

    if (isRemotelyOpened[i]) {
        isAllowed = true;
        digitalWrite(ledGPin, HIGH);
        digitalWrite(relay, LOW);
        Serial.println("Successful");
        tone(buzzerPin, 1000, 100); 
        delay(5000);
        digitalWrite(relay, HIGH);
    } 

  }


  if ( ! mfrc522.PICC_IsNewCardPresent()) {
      return;
  }
  
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }else{
    tone(buzzerPin, 1000, 100);
  }

  // Dump debug info about the card; PICC_HaltA() is automatically called
  // mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

  String cardUID = "";
  for (byte i = 0; i < mfrc522.uid.size; ++i) {
    cardUID += String(mfrc522.uid.uidByte[i], HEX);
  }

  // Print the UID to the serial monitor
  Serial.print("Card UID: ");
  Serial.println(cardUID);


  for(int i = 0; i < 7; i++) {
    Serial.println(cardUID + " ID");
    Serial.println(cardIDs[i] + " adadfdsf");

    if(cardUID.equals(cardIDs[i])) {
      isAllowed = true;
      break;
    }
  }

  // for (const auto &allowedUID : cardIDs) {
  //   if (cardUID.equals(allowedUID)) {
  //     isAllowed = true;
  //     break;
  //   }
  // }

  // for(const auto &condition : isRemotelyOpened) {
  //   if(condition == true) {
  //     isAllowed = condition;
  //   }
  // }

  if (!isAllowed) {
    digitalWrite(ledRPin, HIGH);
    digitalWrite(relay, HIGH);
    Serial.println("Unsuccessful");
    delay(2000);
  } else {
    Serial.println("Works wkwk");
    digitalWrite(ledGPin, HIGH);
    digitalWrite(relay, LOW);
    Serial.println("Successful");

    delay(2000);
    digitalWrite(relay, HIGH);
  }

  // Halt the card to stop further reads
  mfrc522.PICC_HaltA();
  // Stop encryption on the card
  mfrc522.PCD_StopCrypto1();
  // Clear the UID to prepare for the next read
  mfrc522.uid.uidByte[0] = mfrc522.uid.uidByte[1] = mfrc522.uid.uidByte[2] = mfrc522.uid.uidByte[3] = 0;

  // Wait a moment before checking for a new card
  // delay(1000);

}

void processReceivedString(String str) {
  // Create arrays to store card IDs and conditions

  // Split the received string by commas
  int index = 0;
  int lastIndex = 0;

  for (int i = 0; i < 7; i++) {
    index = str.indexOf(',', lastIndex);

    if (index == -1) {
      // If no more commas are found, use the substring until the end of the string
      cardIDs[i] = addPrefixIfMissing(getCardID(str.substring(lastIndex)));
      isRemotelyOpened[i] = getCondition(str.substring(lastIndex));
    } else {
      // Use the substring between the last index and the current comma
      cardIDs[i] = addPrefixIfMissing(getCardID(str.substring(lastIndex, index)));
      isRemotelyOpened[i] = getCondition(str.substring(lastIndex, index));
      lastIndex = index + 1; // Move the last index to the character after the comma
    }

    // If the substring is empty (i.e., two consecutive commas), set it to "N/A"
    if (cardIDs[i].length() == 0) {
      cardIDs[i] = "N/A";
    }
  }

  // Print the full arrays
  for (int i = 0; i < 7; i++) {
    Serial.print(cardIDs[i]);
    Serial.print(":");
    Serial.print(isRemotelyOpened[i] ? "true" : "false");

    // Print a comma after each element (except the last one)
    if (i < 6) {
      Serial.print(",");
    }
  }

  // Print a new line at the end
  Serial.println();
}

bool getCondition(String cardInfo) {
  // Extract the condition part before the colon ':'
  int colonIndex = cardInfo.indexOf(':');
  if (colonIndex != -1) {
    String conditionStr = cardInfo.substring(colonIndex + 1);
    return conditionStr.equalsIgnoreCase("true");
  } else {
    // If no colon is found, assume condition is false
    return false;
  }
}

String getCardID(String cardInfo) {
  // Extract the cardID part before the colon ':'
  int colonIndex = cardInfo.indexOf(':');
  if (colonIndex != -1) {
    return cardInfo.substring(0, colonIndex);
  } else {
    // If no colon is found, return the entire string
    return cardInfo;
  }
}

String addPrefixIfMissing(String cardID) {
  // Check if the first character is alphanumeric, and add "k" if it's not
  if (cardID.length() > 0 && !isAlphaNumeric(cardID.charAt(0))) {
    cardID = "k" + cardID;
  }

  return cardID;
}

boolean isAlphaNumeric(char c) {
  return isAlpha(c) || isDigit(c);
}

boolean isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

boolean isDigit(char c) {
  return (c >= '0' && c <= '9');
}