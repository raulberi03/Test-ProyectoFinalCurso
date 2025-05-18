#include <Arduino.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>

// Ctrl + K, Ctrl + 0 - Ctrl + 0, Ctrl + J

/*
  PARAMETROS
*/
// GESTIÓN DE USUARIOS(USUARIOS Y ADMINISTRADOR)
struct Usuario {
  String id;
  String password;
  String rfidUID;
};

Usuario usuariosValidos[10] = {
  {"1234", "00"},
  {"0011", "00"},
  {"1122", "00"}
};

int NUM_USUARIOS = 3;

String userID = "";
String password = "";
String ADMIN_ID = "123";
String ADMIN_PASS = "123";
bool isEnteringUserID = true;
bool isAdminLoggedIn = false;

// KEYPAD
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

// RFID(RC522)
#define SS_PIN  26
#define RST_PIN 33
#define SCK_PIN 14
#define MOSI_PIN 27
#define MISO_PIN 25

MFRC522 rfid(SS_PIN, RST_PIN);
Preferences prefs;

String rfidUID = "";

// DISPLAY

/*
  DECLARACIONES DE METODOS
*/
// GESTIÓN DE USUARIOS(USUARIOS Y ADMINISTRADOR)
void resetLogin();
void promptUser();
void promptPassword();
void processLogin();
bool validarUsuario(const String& id, const String& password);
void mostrarMenuAdministrador();
void crearUsuario();
void eliminarUsuario();
void listarUsuarios();
void cambiarContrasenaUsuario();
void cambiarIDAdministrador();

// GESTIÓN DE LA MEMORIA
void guardarUsuariosEnMemoria();
void cargarUsuariosDesdeMemoria();
void guardarAdministradorEnMemoria();
void cargarAdministradorDesdeMemoria();

// KEYPAD
void handleKeypadInput();

// RFID(RC522)
String leerUIDTarjeta();
void vincularTarjetaRFID();
void desvincularTarjetaRFID();
bool verificarAccesoPorTarjeta();

// DISPLAY

/*
  CÓDIGO
*/
void setup() {
  Serial.begin(115200);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();

  cargarAdministradorDesdeMemoria();
  cargarUsuariosDesdeMemoria();

  promptUser();
}

void loop() {
  handleKeypadInput();
  verificarAccesoPorTarjeta();
}

/*
  METODOS
*/
// GESTIÓN DE USUARIOS(USUARIOS Y ADMINISTRADOR)
void resetLogin() {
  userID = "";
  password = "";
  isEnteringUserID = true;
}

void promptUser() {
  Serial.print("\nIntroduce tu ID de usuario (hasta 8 dígitos, termina con '#'):");
}

void promptPassword() {
  Serial.print("\nIntroduce tu contraseña (hasta 8 dígitos, termina con '#'):");
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

void mostrarMenuAdministrador() {
  Serial.println("\n*** MODO ADMINISTRADOR ***");
  Serial.println("1: Crear nuevo usuario");
  Serial.println("2: Eliminar usuario");
  Serial.println("3: Listar usuarios");
  Serial.println("4: Vincular tarjeta RFID");
  Serial.println("5: Desvincular tarjeta RFID");
  Serial.println("6: Cambiar contraseña de usuario");
  Serial.println("7: Cambiar ID del administrador");
  Serial.println("8: Salir");

  while (true) {
    char opcion = keypad.getKey();
    if (opcion) {
      if (opcion == '1') {
        crearUsuario();
        break;
      } else if (opcion == '2') {
        eliminarUsuario();
        break;
      } else if (opcion == '3') {
        listarUsuarios();
        break;
      } else if (opcion == '4') {
        vincularTarjetaRFID();
        break;
      } else if (opcion == '5') {
        desvincularTarjetaRFID();
        break;
      } else if (opcion == '6') {
        cambiarContrasenaUsuario();
        break;
      } else if (opcion == '7') {
        cambiarIDAdministrador();
        break;
      }
      else if (opcion == '8') {
        Serial.println("Saliendo del modo administrador...");
        isAdminLoggedIn = false;
        break;
      }
    }
  }
}

void crearUsuario() {
  String nuevoID = "";
  String nuevaPass = "";

  Serial.println("Introduce nuevo ID de usuario (hasta 8 dígitos, termina con '#'):");

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (isDigit(key) && nuevoID.length() < 8) {
        nuevoID += key;
        Serial.print("*");
      }
    }
  }

  // Validación: ID no puede ser igual al del superusuario
  if (nuevoID == ADMIN_ID) {
    Serial.println("\nERROR: El ID ingresado está reservado para el superusuario.");
    return;
  }

  // Validación: ID no puede estar ya en uso por otro usuario
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].id == nuevoID) {
      Serial.println("\nERROR: Este ID ya está en uso por otro usuario.");
      return;
    }
  }

  Serial.println("\nIntroduce nueva contraseña (hasta 8 dígitos, termina con '#'):");

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (isDigit(key) && nuevaPass.length() < 8) {
        nuevaPass += key;
        Serial.print("*");
      }
    }
  }

  if (NUM_USUARIOS < 10) {
    usuariosValidos[NUM_USUARIOS++] = {nuevoID, nuevaPass};
    Serial.println("\nUsuario creado correctamente.");
    guardarUsuariosEnMemoria();
  } else {
    Serial.println("\nLímite de usuarios alcanzado.");
  }
}

void eliminarUsuario() {
  String idEliminar = "";

  Serial.println("Introduce ID del usuario a eliminar (termina con '#'):");

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (isDigit(key) && idEliminar.length() < 8) {
        idEliminar += key;
        Serial.print("*");
      }
    }
  }

  bool encontrado = false;
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].id == idEliminar) {
      for (int j = i; j < NUM_USUARIOS - 1; j++) {
        usuariosValidos[j] = usuariosValidos[j + 1];
      }
      NUM_USUARIOS--;
      encontrado = true;
      guardarUsuariosEnMemoria();
      break;
    }
  }

  if (encontrado) {
    Serial.println("\nUsuario eliminado correctamente.");
  } else {
    Serial.println("\nUsuario no encontrado.");
  }
}

void listarUsuarios() {
  Serial.println("\nListado de usuarios registrados:");
  for (int i = 0; i < NUM_USUARIOS; i++) {
    Serial.print(usuariosValidos[i].id);
    Serial.print(", Tarjeta = ");

    if (usuariosValidos[i].rfidUID != "") {
      Serial.println(usuariosValidos[i].rfidUID);
    } else {
      Serial.println("No vinculada");
    }
  }
}

void cambiarContrasenaUsuario() {
  String idUsuario = "";
  String nuevaPass = "";

  Serial.println("Introduce ID del usuario para cambiar la contraseña (termina con '#'):");

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (isDigit(key) && idUsuario.length() < 8) {
        idUsuario += key;
        Serial.print("*");
      }
    }
  }

  // Cambio de contraseña para el administrador
  if (idUsuario == ADMIN_ID) {
    Serial.println("\nIntroduce nueva contraseña para el administrador (termina con '#'):");
    while (true) {
      char key = keypad.getKey();
      if (key) {
        if (key == '#') break;
        if (isDigit(key) && nuevaPass.length() < 8) {
          nuevaPass += key;
          Serial.print("*");
        }
      }
    }
    ADMIN_PASS = nuevaPass;
    guardarAdministradorEnMemoria();
    Serial.println("\nContraseña del administrador cambiada.");
    return;
  }

  // Buscar usuario común
  int index = -1;
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].id == idUsuario) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    Serial.println("\nUsuario no encontrado.");
    return;
  }

  Serial.println("\nIntroduce nueva contraseña (termina con '#'):");

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (isDigit(key) && nuevaPass.length() < 8) {
        nuevaPass += key;
        Serial.print("*");
      }
    }
  }

  usuariosValidos[index].password = nuevaPass;
  guardarUsuariosEnMemoria();
  Serial.println("\nContraseña actualizada correctamente.");
}

void cambiarIDAdministrador() {
  String nuevoID = "";

  Serial.println("Introduce el nuevo ID para el superadministrador (termina con '#'):");

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (isDigit(key) && nuevoID.length() < 8) {
        nuevoID += key;
        Serial.print("*");
      }
    }
  }

  // Verificar que no esté siendo usado por un usuario común
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].id == nuevoID) {
      Serial.println("\nERROR: Este ID ya está en uso por un usuario.");
      return;
    }
  }

  ADMIN_ID = nuevoID;
  prefs.begin("seguridad", false);        // abrir namespace
  prefs.putString("admin_id", ADMIN_ID);  // guardar persistente
  prefs.end();

  Serial.println("\nID del administrador cambiado correctamente.");
}

// GESTIÓN DE LA MEMORIA
void guardarUsuariosEnMemoria() {
  prefs.begin("usuarios", false);
  prefs.putInt("num_usuarios", NUM_USUARIOS);

  for (int i = 0; i < NUM_USUARIOS; i++) {
    String key = "usuario_" + String(i);
    String datos = usuariosValidos[i].id + "," + usuariosValidos[i].password + "," + usuariosValidos[i].rfidUID;
    prefs.putString(key.c_str(), datos);
  }

  prefs.end();
}

void cargarUsuariosDesdeMemoria() {
  prefs.begin("usuarios", true);
  NUM_USUARIOS = prefs.getInt("num_usuarios", 0);
  for (int i = 0; i < NUM_USUARIOS; i++) {
    String key = "usuario_" + String(i);
    String datos = prefs.getString(key.c_str(), "");
    if (datos != "") {
      int p1 = datos.indexOf(',');
      int p2 = datos.lastIndexOf(',');

      usuariosValidos[i].id = datos.substring(0, p1);
      usuariosValidos[i].password = datos.substring(p1 + 1, p2);
      usuariosValidos[i].rfidUID = datos.substring(p2 + 1);
    }
  }
  prefs.end();
}

void guardarAdministradorEnMemoria() {
  prefs.begin("seguridad", false);
  prefs.putString("admin_id", ADMIN_ID);
  prefs.putString("admin_pass", ADMIN_PASS);
  prefs.end();
}

void cargarAdministradorDesdeMemoria() {
  prefs.begin("seguridad", true);
  ADMIN_ID = prefs.getString("admin_id", "123");
  ADMIN_PASS = prefs.getString("admin_pass", "123");
  prefs.end();
}

// KEYPAD
void handleKeypadInput() {
  char key = keypad.getKey();

  if (key) {
    if (key == '#') {
      if (isEnteringUserID) {
        isEnteringUserID = false;
        promptPassword();
      } else {
        if (userID == ADMIN_ID && password == ADMIN_PASS) {
          isAdminLoggedIn = true;
          resetLogin();
          while (isAdminLoggedIn) {
            mostrarMenuAdministrador();
          }
          promptUser();
        } else {
          processLogin();
          resetLogin();
          promptUser();
        }
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

// RFID(RC522)
String leerUIDTarjeta() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return "";
  }

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    uid += String(rfid.uid.uidByte[i], HEX);
  }

  rfid.PICC_HaltA();  // detener la comunicación con la tarjeta
  rfid.PCD_StopCrypto1();

  uid.toUpperCase(); // uniforme
  return uid;
}

void vincularTarjetaRFID() {
  Serial.println("Introduce ID de usuario a vincular (termina con '#'):");

  String idUsuario = "";
  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (isDigit(key) && idUsuario.length() < 8) {
        idUsuario += key;
        Serial.print("*");
      }
    }
  }

  // Buscar usuario
  int index = -1;
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].id == idUsuario) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    Serial.println("\nUsuario no encontrado.");
    return;
  }

  Serial.println("\nAcerca una tarjeta RFID para vincularla...");

  String uid = "";
  unsigned long inicio = millis();
  while (millis() - inicio < 10000) {  // Esperar 10 segundos máx
    uid = leerUIDTarjeta();
    if (uid != "") break;
  }

  if (uid == "") {
    Serial.println("Tiempo agotado. No se leyó tarjeta.");
    return;
  }

  // Verificar si la UID ya está en uso
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].rfidUID == uid) {
      Serial.println("Esta tarjeta ya está asignada a otro usuario.");
      return;
    }
  }

  usuariosValidos[index].rfidUID = uid;
  guardarUsuariosEnMemoria();
  Serial.println("Tarjeta vinculada exitosamente.");
}

void desvincularTarjetaRFID() {
  Serial.println("Introduce ID de usuario a desvincular (termina con '#'):");

  String idUsuario = "";
  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (isDigit(key) && idUsuario.length() < 8) {
        idUsuario += key;
        Serial.print("*");
      }
    }
  }

  int index = -1;
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].id == idUsuario) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    Serial.println("\nUsuario no encontrado.");
    return;
  }

  usuariosValidos[index].rfidUID = "";
  guardarUsuariosEnMemoria();
  Serial.println("\nTarjeta desvinculada exitosamente.");
}

bool verificarAccesoPorTarjeta() {
  // Esperar a que se acerque una tarjeta
  if (!rfid.PICC_IsNewCardPresent()) {
    return false;  // No hay tarjeta
  }
  
  if (!rfid.PICC_ReadCardSerial()) {
    return false;  // Error leyendo tarjeta
  }

  // Leer UID y convertirlo a String HEX
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";  // para formato uniforme
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  rfid.PICC_HaltA();  // Finalizar comunicación
  rfid.PCD_StopCrypto1();

  // Buscar usuario vinculado
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].rfidUID == uid) {
      Serial.print("Acceso concedido a usuario: ");
      Serial.println(usuariosValidos[i].id);
      promptUser();
      return true;
    }
  }

  Serial.println("Acceso denegado: tarjeta no vinculada.");
  promptUser();
  return false;
}

// DISPLAY