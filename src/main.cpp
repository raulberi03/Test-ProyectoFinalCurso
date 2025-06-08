#include <Arduino.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <HTTPClient.h>

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

const int MAX_USUARIOS = 10;
int NUM_USUARIOS = 3;

String userID = "";
String password = "";
String ADMIN_ID = "123";
String ADMIN_PASS = "123";
bool isEnteringUserID = true;
bool isAdminLoggedIn = false;
String comboBuffer = "";

const int lineasPorPagina = 3;
int paginaActual = 0;
int totalLineas = 0;
String lineasUsuarios[MAX_USUARIOS];

// KEYPAD
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte colPins[COLS] = {26, 25, 33, 32};
byte rowPins[ROWS] = {13, 12, 14, 27};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// RFID(RC522)
#define SS_PIN 4
#define RST_PIN 2

MFRC522 rfid(SS_PIN, RST_PIN);
Preferences prefs;
String rfidUID = "";

// DISPLAY
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
char ultimoTexto[64];

// IOT - Bot Telegram
const char* ssid = "DIGIFIBRA-Te6b_EXT";
const char* passwordWifi = "sECxNtNcdD6Q";
String botToken = "7668573039:AAGZl1hYgxTacgpy8VPPPZHqjFQVq72MuIg";
String chatID = "6421188587";

// PIN de activación
#define RELAY_PIN 15

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
void generarListadoUsuarios();
void mostrarPagina(int pagina);
void actualizarListadoConTeclado();
void cambiarContrasenaUsuario();
void cambiarIDAdministrador();

// GESTIÓN DE LA MEMORIA
void guardarUsuariosEnMemoria();
void cargarUsuariosDesdeMemoria();
void guardarAdministradorEnMemoria();
void cargarAdministradorDesdeMemoria();
void resetearDatosPorDefecto();

// KEYPAD
void handleKeypadInput();
bool salirSiTeclaCancelar(char key);

// RFID(RC522)
String leerUIDTarjeta();
void vincularTarjetaRFID();
void desvincularTarjetaRFID();
bool verificarAccesoPorTarjeta();

// DISPLAY
void actualizarPantalla(const char* texto, bool espera = false, int tiempoEspera = 0);
void actualizarPantallaConInputs(bool borrado = false);

// IOT - Bot Telegram
void enviarMensaje(String mensaje);

/*
  CÓDIGO GENERAL
*/
void setup() {
  Serial.begin(115200);
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
  delay(1000);
  SPI.begin();
  Wire.begin(21, 22);
  rfid.PCD_Init();
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Conexion al Wi-Fi
  WiFi.begin(ssid, passwordWifi);
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    // Serial.print(".");
    intentos++;
  }

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
  actualizarPantalla("Usuario:");
}

void promptPassword() {
  actualizarPantalla("Contraseña:");
}

void processLogin() {
  actualizarPantalla("Procesando login...");

  if (validarUsuario(userID, password)) {
    actualizarPantalla("Acceso concedido", true, 1000);
  } else {
    actualizarPantalla("Acceso denegado. Usuario o contraseña incorrectos.", true, 1000);
  }
}

bool validarUsuario(const String& id, const String& password) {
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].id == id && usuariosValidos[i].password == password) {
      digitalWrite(RELAY_PIN, HIGH);
      enviarMensaje("Acceso concedido a usuario:" + id);
      delay(2000);
      digitalWrite(RELAY_PIN, LOW);
      return true;
    }
  }

  enviarMensaje("Intento de acceso fallido");
  return false;
}

void mostrarMenuAdministrador() {
  const char* opciones[] = {
    "1: Crear nuevo usuario",
    "2: Eliminar usuario",
    "3: Listar usuarios",
    "4: Vincular tarjeta RFID",
    "5: Desvincular tarjeta RFID",
    "6: Cambiar contrasena",
    "7: Cambiar ID admin",
    "8: Salir"
  };
  const int numOpciones = sizeof(opciones) / sizeof(opciones[0]);
  const int filasPorPagina = 3;

  int paginaActual = 0;
  unsigned long ultimoCambio = millis();
  unsigned long ultimoKeyTime = 0;
  char ultimaTecla = 0;

  while (true) {
    unsigned long ahora = millis();

    if (ahora - ultimoCambio >= 2000) {
      paginaActual++;
      if (paginaActual * filasPorPagina >= numOpciones) {
        paginaActual = 0;
      }
      ultimoCambio = ahora;
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);

    for (int i = 0; i < filasPorPagina; i++) {
      int idx = paginaActual * filasPorPagina + i;
      if (idx >= numOpciones) break;

      u8g2.drawUTF8(0, 15 + i * 20, opciones[idx]);
    }

    u8g2.sendBuffer();

    char tecla = keypad.getKey();
    if (tecla && tecla != ultimaTecla) {
      if (ahora - ultimoKeyTime > 300) {
        ultimoKeyTime = ahora;
        ultimaTecla = tecla;

        if (tecla >= '1' && tecla <= '8') {
          switch (tecla) {
            case '1': crearUsuario(); break;
            case '2': eliminarUsuario(); break;
            case '3': listarUsuarios(); break;
            case '4': vincularTarjetaRFID(); break;
            case '5': desvincularTarjetaRFID(); break;
            case '6': cambiarContrasenaUsuario(); break;
            case '7': cambiarIDAdministrador(); break;
            case '8':
              actualizarPantalla("Saliendo...");
              isAdminLoggedIn = false;
              return;
          }
        }
      }
    }

    if (!tecla) {
      ultimaTecla = 0;
    }

    delay(10);
  }
}

void crearUsuario() {
  String idTemp = "";
  String passwordTemp = "";

  promptUser();

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (salirSiTeclaCancelar(key)) return;

      if (key == '*' && idTemp.length() > 0) {
        idTemp.remove(idTemp.length() - 1);
      } else if (isDigit(key) && idTemp.length() < 8) {
        idTemp += key;
      }

      String texto = "Usuario: " + idTemp;
      actualizarPantalla(texto.c_str());
    }
  }

  // Validación: ID no puede ser igual al del superusuario
  if (idTemp == ADMIN_ID) {
    actualizarPantalla("Error: ID no disponible", true, 1000);
    return;
  }

  // Validación: ID no puede estar ya en uso por otro usuario
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].id == idTemp) {
      actualizarPantalla("Error: ID no disponible", true, 1000);
      return;
    }
  }

  promptPassword();

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;

      if (key == '*' && passwordTemp.length() > 0) {
        passwordTemp.remove(passwordTemp.length() - 1);
        actualizarPantallaConInputs(true);
      } else if (isDigit(key) && passwordTemp.length() < 8) {
        passwordTemp += key;
        actualizarPantallaConInputs(false);
      }
    }
  }

  // Guardar el nuevo usuario
  if (NUM_USUARIOS < 10) {
    usuariosValidos[NUM_USUARIOS++] = {idTemp, passwordTemp};
    actualizarPantalla("Usuario creado correctamente", true, 1000);
    guardarUsuariosEnMemoria();
  } else {
    actualizarPantalla("Límite de usuarios alcanzado", true, 1000);
  }
}

void eliminarUsuario() {
  String idTemp = "";

  promptUser();

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (salirSiTeclaCancelar(key)) return;

      if (key == '*' && idTemp.length() > 0) {
        idTemp.remove(idTemp.length() - 1);
      } else if (isDigit(key) && idTemp.length() < 8) {
        idTemp += key;
      }

      String texto = "Usuario: " + idTemp;
      actualizarPantalla(texto.c_str());
    }
  }

  bool encontrado = false;
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].id == idTemp) {
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
    actualizarPantalla("Usuario eliminado correctamente", true, 1000);
  } else {
    actualizarPantalla("Usuario no encontrado");
  }
}

void listarUsuarios() {
  paginaActual = 0;
  generarListadoUsuarios();
  mostrarPagina(paginaActual);

  while (true) {
    char key = keypad.getKey();

    if (key) {
      if (salirSiTeclaCancelar(key)) return;

      if (key == '#') {
        if ((paginaActual + 1) * lineasPorPagina < totalLineas) {
          paginaActual++;
          mostrarPagina(paginaActual);
        }
      } else if (key == '*') {
        if (paginaActual > 0) {
          paginaActual--;
          mostrarPagina(paginaActual);
        }
      }
    }

    delay(100);
  }
}

void generarListadoUsuarios() {
  totalLineas = 0;
  if (NUM_USUARIOS == 0) {
    lineasUsuarios[totalLineas++] = "No hay usuarios";
    return;
  }
  for (int i = 0; i < NUM_USUARIOS; i++) {
    String linea = usuariosValidos[i].id + ", Tarjeta = ";
    if (usuariosValidos[i].rfidUID != "") {
      linea += usuariosValidos[i].rfidUID;
    } else {
      linea += "No vinculada";
    }
    lineasUsuarios[totalLineas++] = linea;
  }
}

void mostrarPagina(int pagina) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  int start = pagina * lineasPorPagina;
  for (int i = 0; i < lineasPorPagina; i++) {
    int idx = start + i;
    if (idx >= totalLineas) break;
    u8g2.drawStr(0, 12 * (i + 1), lineasUsuarios[idx].c_str());
  }

  // Indicador de página
  String indicador = "Pg " + String(pagina + 1) + "/" + String((totalLineas + lineasPorPagina - 1) / lineasPorPagina);
  u8g2.drawStr(80, 60, indicador.c_str());

  u8g2.sendBuffer();
}

void cambiarPagina() {
  char tecla = keypad.getKey();
  if (salirSiTeclaCancelar(tecla)) return;
  if (tecla == '#') { // página siguiente
    if ((paginaActual + 1) * lineasPorPagina < totalLineas) {
      paginaActual++;
      mostrarPagina(paginaActual);
    }
  } else if (tecla == '*') { // página anterior
    if (paginaActual > 0) {
      paginaActual--;
      mostrarPagina(paginaActual);
    }
  }
}

void cambiarContrasenaUsuario() {
  String idTemp = "";
  String passwordTemp = "";

  // Metemos ID
  promptUser();

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (salirSiTeclaCancelar(key)) return;

      if (key == '*' && idTemp.length() > 0) {
        idTemp.remove(idTemp.length() - 1);
      } else if (isDigit(key) && idTemp.length() < 8) {
        idTemp += key;
      }

      String texto = "Usuario: " + idTemp;
      actualizarPantalla(texto.c_str());
    }
  }

  // Metemos contraseña
  promptPassword();

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (salirSiTeclaCancelar(key)) return;

      if (key == '*' && passwordTemp.length() > 0) {
        passwordTemp.remove(passwordTemp.length() - 1);
        actualizarPantallaConInputs(true);
      } else if (isDigit(key) && passwordTemp.length() < 8) {
        passwordTemp += key;
        actualizarPantallaConInputs(false);
      }
    }
  }
  
  // Comprobamos si la contraseña a cambiar es de administrador
  if (idTemp == ADMIN_ID) {
    ADMIN_PASS = passwordTemp;
    guardarAdministradorEnMemoria();
    actualizarPantalla("Datos actualizados", true, 1000);
    return;
  }

  // Buscamos si el usuario existe y de no existir marcamos fallo
  int index = -1;
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].id == idTemp) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    actualizarPantalla("Usuario no encontrado", true, 1000);
    return;
  } else {
    usuariosValidos[index].password = passwordTemp;
    guardarUsuariosEnMemoria();
    actualizarPantalla("Datos actualizados", true, 1000);
  }
}

void cambiarIDAdministrador() {
  String nuevoID = "";

  actualizarPantalla("Nuevo ID superadministrador");

    while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (salirSiTeclaCancelar(key)) return;

      if (key == '*' && nuevoID.length() > 0) {
        nuevoID.remove(nuevoID.length() - 1);
      } else if (isDigit(key) && nuevoID.length() < 8) {
        nuevoID += key;
      }

      String texto = "Usuario: " + nuevoID;
      actualizarPantalla(texto.c_str());
    }
  }

  // Verificar que no esté siendo usado por un usuario común
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].id == nuevoID) {
      actualizarPantalla("ID en uso");
      return;
    }
  }

  ADMIN_ID = nuevoID;
  prefs.begin("seguridad", false);        // abrir namespace
  prefs.putString("admin_id", ADMIN_ID);  // guardar persistente
  prefs.end();

  actualizarPantalla("ID actualizado");
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

void resetearDatosPorDefecto() {
  // Reiniciar usuarios hardcodeados
  usuariosValidos[0] = {"1234", "00", ""};
  usuariosValidos[1] = {"0011", "00", ""};
  usuariosValidos[2] = {"1122", "00", ""};
  NUM_USUARIOS = 3;

  // Reiniciar admin hardcodeado
  ADMIN_ID = "123";
  ADMIN_PASS = "123";

  // Guardar los valores por defecto en memoria persistente
  guardarUsuariosEnMemoria();
  guardarAdministradorEnMemoria();

  Serial.println("\n*** Datos reseteados a valores por defecto ***");
}

// KEYPAD
void handleKeypadInput() {
  char key = keypad.getKey();

  if (key) {
    comboBuffer += key;
    if (comboBuffer.length() > 4) {
      comboBuffer.remove(0, comboBuffer.length() - 4);
    }

    if (comboBuffer == "9999") {
      resetearDatosPorDefecto();
      resetLogin();
      promptUser();
      comboBuffer = "";
      return;
    }

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
    } 
    else if (key == '*') {
      if (isEnteringUserID && userID.length() > 0) {
        userID.remove(userID.length() - 1);
        String texto = "Usuario: " + userID;
        actualizarPantalla(texto.c_str());
      } else if (!isEnteringUserID && password.length() > 0) {
        password.remove(password.length() - 1);
        actualizarPantallaConInputs(true);
      }
    } 
    else if (isDigit(key)) {
      if (isEnteringUserID && userID.length() < 8) {
        userID += key;
        String texto = "Usuario: " + userID;
        actualizarPantalla(texto.c_str());
      } else if (!isEnteringUserID && password.length() < 8) {
        password += key;
        actualizarPantallaConInputs();
      }
    }
  }
}

bool salirSiTeclaCancelar(char key) {
  if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
    actualizarPantalla("Cancelando...", true, 250);
    return true;
  }
  return false;
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
  promptUser();

  String idUsuario = "";
  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (salirSiTeclaCancelar(key)) return;

      if (key == '*' && idUsuario.length() > 0) {
        idUsuario.remove(idUsuario.length() - 1);
      } else if (isDigit(key) && idUsuario.length() < 8) {
        idUsuario += key;
      }

      String texto = "Usuario: " + idUsuario;
      actualizarPantalla(texto.c_str());
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
    actualizarPantalla("Usuario no encontrado");
    return;
  }

  actualizarPantalla("Acercar tarjeta RFID...");

  String uid = "";
  unsigned long inicio = millis();
  while (millis() - inicio < 10000) {  // Esperar 10 segundos máx
    uid = leerUIDTarjeta();
    if (uid != "") break;
  }

  if (uid == "") {
    actualizarPantalla("Tiempo agotado");
    return;
  }

  // Verificar si la UID ya está en uso
  for (int i = 0; i < NUM_USUARIOS; i++) {
    if (usuariosValidos[i].rfidUID == uid) {
      actualizarPantalla("Tarjeta asignada a otro usuario");
      return;
    }
  }

  usuariosValidos[index].rfidUID = uid;
  guardarUsuariosEnMemoria();
  actualizarPantalla("Tarjeta vinculada");
}

void desvincularTarjetaRFID() {
  promptUser();

  String idUsuario = "";

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      if (salirSiTeclaCancelar(key)) return;

      if (key == '*' && idUsuario.length() > 0) {
        idUsuario.remove(idUsuario.length() - 1);
      } else if (isDigit(key) && idUsuario.length() < 8) {
        idUsuario += key;
      }

      String texto = "Usuario: " + idUsuario;
      actualizarPantalla(texto.c_str());
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
    actualizarPantalla("Usuario no encontrado");
    return;
  }

  usuariosValidos[index].rfidUID = "";
  guardarUsuariosEnMemoria();
  actualizarPantalla("Tarjeta desvinculada");
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
      String texto = "Acceso concedido a usuario: " + usuariosValidos[i].id;
      enviarMensaje(texto + " mediante tarjeta");
      digitalWrite(RELAY_PIN, HIGH);
      actualizarPantalla(texto.c_str(), true, 1000);
      delay(2000);
      digitalWrite(RELAY_PIN, LOW);
      promptUser();
      return true;
    }
  }

  enviarMensaje("Acceso denegado: tarjeta no vinculada");
  actualizarPantalla("Acceso denegado: tarjeta no vinculada", true, 1000);
  promptUser();
  return false;
}

// DISPLAY
void actualizarPantalla(const char* texto, bool espera, int tiempoEspera) {
  strncpy(ultimoTexto, texto, sizeof(ultimoTexto));
  
  u8g2.clearBuffer();
  u8g2.drawUTF8(0, 15, texto);
  u8g2.sendBuffer();

  if(espera) {
    delay(tiempoEspera);
  }
}

void actualizarPantallaConInputs(bool borrado) {
  size_t longitud = strlen(ultimoTexto);

  if (borrado && longitud > 0 && ultimoTexto[longitud - 1] == '*') {
    ultimoTexto[longitud - 1] = '\0';
  } else {
    ultimoTexto[longitud] = '*';
    ultimoTexto[longitud + 1] = '\0';
  }

  actualizarPantalla(ultimoTexto);
}

// IOT - Bot Telegram
void enviarMensaje(String mensaje) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + mensaje;
    
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.println("Mensaje enviado: " + mensaje);
    } else {
      Serial.print("Error al enviar: ");
      Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("WiFi no conectado");
  }
}