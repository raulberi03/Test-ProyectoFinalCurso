#include <Arduino.h>
#include <Keypad.h>

/*
  PARAMETROS
*/

// GESTIÓN DE USUARIOS(USUARIOS Y ADMINISTRADOR)
struct Usuario {
  String id;
  String password;
};

Usuario usuariosValidos[10] = {
  {"1234", "00"},
  {"0011", "00"},
  {"1122", "00"}
};

int NUM_USUARIOS = 3;

String userID = "";
String password = "";
const String ADMIN_ID = "123";
const String ADMIN_PASS = "123";
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

// KEYPAD
void handleKeypadInput();

// RFID(RC522)

// DISPLAY

/*
  CÓDIGO
*/
void setup() {
  Serial.begin(115200);
  promptUser();
}

void loop() {
  handleKeypadInput();
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

void mostrarMenuAdministrador() {
  Serial.println("\n*** MODO ADMINISTRADOR ***");
  Serial.println("A: Crear nuevo usuario");
  Serial.println("B: Eliminar usuario");
  Serial.println("C: Listar usuarios");
  Serial.println("D: Salir");

  while (true) {
    char opcion = keypad.getKey();
    if (opcion) {
      if (opcion == 'A') {
        crearUsuario();
        break;
      } else if (opcion == 'B') {
        eliminarUsuario();
        break;
      } else if (opcion == 'C') {
        listarUsuarios();
        break;
      } else if (opcion == 'D') {  // <-- Manejo de nueva opción
        Serial.println("Saliendo del modo administrador...");
        isAdminLoggedIn = false;
        break;
      } else {
        Serial.println("Opción no válida. Usa A, B, C o D.");
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
    Serial.print("- Usuario ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(usuariosValidos[i].id);
  }
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

// DISPLAY