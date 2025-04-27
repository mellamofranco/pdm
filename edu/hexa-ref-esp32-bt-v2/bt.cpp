#include "bt.h"

// Variables globales
BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;

int pulselen = SERVOMIN;

// #define MAXPACKETDATA 48
unsigned char packetData[MAXPACKETDATA];
unsigned int packetLength = 0;
unsigned int packetLengthReceived = 0;
int packetState = P_WAITING_FOR_HEADER;


void dumpPacket() { // this is purely for debugging, it can cause timing problems so only use it for debugging
  Serial.print("DMP:");
  for (unsigned int i = 0; i < packetLengthReceived; i++) {
    Serial.write(packetData[i]); Serial.print("("); Serial.print(packetData[i]); Serial.print(")");
  }
  Serial.println("");
}

void packetErrorChirp(char c) {
  //beep(70, 8);
  Serial.print(" BTER:"); Serial.print(packetState); Serial.print(c);
  //Serial.print("("); Serial.print(c,DEC); Serial.print(")");
  Serial.print("A");
  // Serial.println(BlueTooth.available());
  packetState = P_WAITING_FOR_HEADER; // reset to initial state if any error occurs
}

// Callbacks del servidor
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
      deviceConnected = true;
      Serial.println("Cliente conectado");
    }

    void onDisconnect(BLEServer* pServer) override {
      deviceConnected = false;
      Serial.println("Cliente desconectado");

      // Reiniciar publicidad
      pServer->startAdvertising();
    }
};

// Callbacks de la característica
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      //      std::string rxValue = pCharacteristic->getValue();
      String rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        receiveDataHandler(rxValue);
      }
    }
};

// Inicialización BLE
void initBLE() {
  // Crear el dispositivo BLE
  BLEDevice::init("HEXAPOD-01");

  // Crear el servidor BLE
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Crear el servicio BLE
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Crear una característica BLE
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Agregar descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Establecer callback para la característica
  pCharacteristic->setCallbacks(new MyCallbacks());

  // Iniciar el servicio
  pService->start();

  // Iniciar la publicidad
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE iniciado, esperando conexiones de clientes...");
}

// Función opcional para actualizar o enviar notificaciones
void updateBLE() {
  if (deviceConnected) {
    // Ejemplo: pCharacteristic->setValue("Hola");
    // pCharacteristic->notify();
  }
}

/////////////////////////
void receiveDataHandler(String rxValue) {
  Serial.println("*****");
  Serial.print("Mensaje recibido: ");
  for (size_t i = 0; i < rxValue.length(); i++) {
    Serial.print((char)rxValue[i]);
  }
  Serial.println();
  Serial.println("*****");

  // for (int i = 0; i < rxValue.length(); i++) {
  //     unsigned int c = (unsigned int)rxValue[i];

  //     // Copio la lógica de receiveDataHandler() que procesaba cada 'c'
  //     switch (packetState) {
  //       case P_WAITING_FOR_HEADER:
  //         if (c == 'V') {
  //           packetState = P_WAITING_FOR_VERSION;
  //         } else if (c == '@') {
  //           packetState = P_SIMPLE_WAITING_FOR_DATA;
  //           packetLengthReceived = 0;
  //         } else {
  //           int flushcount = 0;
  //           // En el modelo de callback no tenemos BlueTooth.available(), así que no podemos "flushear" lo que ya vino.
  //           // Simplemente ignoramos bytes hasta próximo header válido.
  //           if (c != 0) {
  //             packetErrorChirp(c);
  //           } else {
  //             NullCount++;
  //             if (NullCount > 100) {
  //               HC05_pad = 1;
  //             }
  //           }
  //         }
  //         break;

  //       case P_WAITING_FOR_VERSION:
  //         if (c == '1') {
  //           packetState = P_WAITING_FOR_LENGTH;
  //           NullCount = 0;
  //         } else if (c == 'V') {
  //           // quedarse en este estado
  //         } else {
  //           packetErrorChirp(c);
  //           packetState = P_WAITING_FOR_HEADER;
  //         }
  //         break;

  //       case P_WAITING_FOR_LENGTH:
  //         packetLength = c;
  //         if (packetLength > MAXPACKETDATA) {
  //           packetErrorChirp(c);
  //           Serial.print("Bad Length="); Serial.println(c);
  //           packetState = P_WAITING_FOR_HEADER;
  //           return;
  //         }
  //         packetLengthReceived = 0;
  //         packetState = P_READING_DATA;
  //         break;

  //       case P_READING_DATA:
  //         if (packetLengthReceived >= MAXPACKETDATA) {
  //           Serial.println("ERROR: PacketDataLen out of bounds!");
  //           packetState = P_WAITING_FOR_HEADER;
  //           packetLengthReceived = 0;
  //           return;
  //         }
  //         packetData[packetLengthReceived++] = c;
  //         if (packetLengthReceived == packetLength) {
  //           packetState = P_WAITING_FOR_CHECKSUM;
  //         }
  //         break;

  //       case P_WAITING_FOR_CHECKSUM:
  //         {
  //           unsigned int sum = packetLength;
  //           for (unsigned int j = 0; j < packetLength; j++) {
  //             sum += packetData[j];
  //           }
  //           sum = (sum % 256);
  //           if (sum != c) {
  //             packetErrorChirp(c);
  //             Serial.print("CHECKSUM FAIL "); Serial.print(sum); Serial.print("!="); Serial.println((int)c);
  //             packetState = P_WAITING_FOR_HEADER;
  //           } else {
  //             LastValidReceiveTime = millis();
  //             processPacketData();
  //             packetState = P_WAITING_FOR_HEADER;
  //           }
  //         }
  //         break;

  //       case P_SIMPLE_WAITING_FOR_DATA:
  //         packetData[packetLengthReceived++] = c;
  //         if (packetLengthReceived == 3) {
  //           packetState = P_WAITING_FOR_HEADER;
  //           if ((packetData[0] != 'W' && packetData[0] != 'D' && packetData[0] != 'F' && packetData[0] != 'X' && packetData[0] != 'Y' && packetData[0] != 'Z') ||
  //               (packetData[1] != '1' && packetData[1] != '2' && packetData[1] != '3' && packetData[1] != '4') ||
  //               (packetData[2] != 'f' && packetData[2] != 'b' && packetData[2] != 'l' && packetData[2] != 'r' &&
  //                packetData[2] != 'w' && packetData[2] != 's')) {
  //             // packet malo
  //             return;
  //           } else {
  //             processPacketData();
  //             return;
  //           }
  //         }
  //         break;
  //     } // fin del switch
  //   } // fin
}
