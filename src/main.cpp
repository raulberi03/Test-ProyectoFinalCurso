#include <Arduino.h>
#include <Keypad.h>

// SISTEMA DE USUARIOS
struct Usuario {
  String id;
  String password;
};

Usuario usuariosValidos[] = {
  {"12345678", "0000"},
  {"11112222", "1234"},
  {"87654321", "9999"},
  {"00001111", "4321"}
};
const int NUM_USUARIOS = sizeof(usuariosValidos) / sizeof(usuariosValidos[0]);

String userID = "";
String password = "";
bool isEnteringUserID = true;

// DECLARACIONES KEYPAD
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {5, 18, 19, 21};
byte colPins[COLS] = {22, 23, 13, 12};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// DECLARACIONES RFID RC522

// DECLARACIONES DISPLAY OLED - I2C




// DECLARACIÓN MÉTODOS KEYPAD
void handleKeypadInput();
void resetLogin();
void promptUser();
void promptPassword();
void processLogin();
bool validarUsuario(const String& id, const String& password);

// DECLARACIÓN MÉTODOS RFID RC522


// DECLARACIÓN MÉTODOS DISPLAY OLED - I2C


// CÓDIGO "NORMAL"
void setup() {
  Serial.begin(115200);
  promptUser();
}

void loop() {
  handleKeypadInput();
}

// MÉTODOS KEYPAD
void handleKeypadInput() {
  char key = keypad.getKey();

  if (key) {
    if (key == '#') {
      if (isEnteringUserID) {
        // Serial.println("\nID ingresado: " + userID);
        isEnteringUserID = false;
        promptPassword();
      } else {
        // Serial.println("\nContraseña ingresada: " + password);
        processLogin();
        resetLogin();
        promptUser();
      }
    } else if (key == '*') {
      if (isEnteringUserID && userID.length() > 0) {
        userID.remove(userID.length() - 1);
      } else if (!isEnteringUserID && password.length() > 0) {
        password.remove(password.length() - 1);
      }
    } else if (isDigit(key)) {
      if (isEnteringUserID && userID.length() < 8) {
        userID += key;
        Serial.print("*");
      } else if (!isEnteringUserID && password.length() < 8) {
        password += key;
        Serial.print("*");
      }
    }
  }
}

void resetLogin() {
  userID = "";
  password = "";
  isEnteringUserID = true;
}

void promptUser() {
  Serial.println("\nIntroduce tu ID de usuario (hasta 8 dígitos, termina con '#'):");
}

void promptPassword() {
  Serial.println("Introduce tu contraseña (hasta 8 dígitos, termina con '#'):");
}

void processLogin() {
  Serial.println("Procesando login...");
  if (validarUsuario(userID, password)) {
    Serial.println("Acceso concedido.");
  } else {
    Serial.println("Acceso denegado. Usuario o contraseña incorrectos.");
  }
}

bool validarUsuario(const String& id, const String& password) {
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].id == id && usuariosValidos[i].password == password) {
      return true;
    }
  }
  return false;
}

// MÉTODOS RFID RC522


// MÉTODOS DISPLAY OLED - I2C