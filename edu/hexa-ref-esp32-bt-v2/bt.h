#pragma once
#ifndef BT_H
#define BT_H

#include "config.h"
#include "utils.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUID para servicio y característica de comunicación
#define SERVICE_UUID        "0000181a-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "00002a6e-0000-1000-8000-00805f9b34fb"

extern BLEServer* pServer;
extern BLECharacteristic* pCharacteristic;
extern bool deviceConnected;


// states for processing incoming bluetooth data

#define P_WAITING_FOR_HEADER      0
#define P_WAITING_FOR_VERSION     1
#define P_WAITING_FOR_LENGTH      2
#define P_READING_DATA            3
#define P_WAITING_FOR_CHECKSUM    4
#define P_SIMPLE_WAITING_FOR_DATA 5

extern int pulselen;

#define MAXPACKETDATA 48
extern unsigned char packetData[];
extern unsigned int packetLength;
extern unsigned int packetLengthReceived;
extern int packetState;

// Funciones que podés llamar desde tu main
void initBLE();
void updateBLE();  // opcional si en algún momento querés hacer cosas tipo enviar notificaciones


void packetErrorChirp(char c);
void dumpPacket();
void receiveDataHandler(String value);

// String lastReceivedMessage 
String checkIfData();
#endif
