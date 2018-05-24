#include "BluetoothControl.h"
#include "EthanolSensorControl.h"
#include "PowerSourceControl.h"
#include "EEPROMManager.h"
#include "main.h"

#include <util/delay.h>

int main(void)
{
    while (1) {
        uint8_t initcode = Initialize();
        if (initcode == 0x00) {
            Life();
        } else {
            Death();
        }
    }
}

uint8_t Initialize (void)
{
    uint8_t initcode = 0x00;
    initcode |= BluetoothInitialize();
    PowerSourceInitialize();
    EthanolSensorInitialize();

    return initcode;
}

void Life (void)
{
    while (1) {
        Activity();
        Standby();
    }
}

void Activity (void)
{
    BluetoothSend({ ACK, OVER });
    while (BluetoothIsPaired()) {
        BluetoothSend({ RDY, OVER });

        char *cmd;
        if (BluetoothReceive(cmd)) {
            ExecuteCommand(cmd);
        }
    }
    BluetoothSend({ NACK, OVER });
}

void unloadScores (void)
{
    uint8_t dataSize = sizeof(double) + 1;
    double *scores = ReadScores();
    unsigned char val [dataSize - 1];
    unsigned char *msg = malloc(dataSize);
    msg = val;
    msg[dataSize - 1] = RDY;

    for (int i = 0; i < EEPROM_SCORE_SIZE; i++) {
        memcpy(&val, &(scores[i]), sizeof(double));
        BluetoothSend(*msg);
    }

    BluetoothSend({ OVER }); // I don't think this is valid
}

void ExecuteCommand (char *cmd)
{
    if (cmd[0] == CMD_OK) {
        BluetoothSend({ ACK, OVER }); // I don't think this is valid
    } else if (cmd[0] == CMD_MEASUREBAC) {
        double val = EthanolSensorMeasureBAC();
        TransmitDouble(val);
    } else if (cmd[0] == CMD_MEASUREBAT) {
        double val = PowerSourceMeasureBattery();
        TransmitDouble(val);
    } else if (cmd[0] == CMD_UNLOADSCORES) {
        unloadScores();
    } else {
        BluetoothSend({ ERR, OVER }); // I don't think this is valid
    }
}

void TransmitDouble (double *d)
{
    size_t dataSize = sizeof(double) + 1;
    char *data = (char *)malloc(dataSize);
    memcpy(data, d, sizeof(double));
    data[dataSize - 1] = OVER;

    BluetoothSend(data);
}

void Standby (void)
{
    while (!BluetoothIsPaired()) {
        //Standby
        _delay_ms(STANDBY_TIMEOUT);
    }
}

void Death (void)
{
    while (1) {
        if (initcode & (1 << RQST_BLUETOOTH_BAUD_INDEX)) {
            //BAUD error
        } else if (initcode & (1 << RQST_BLUETOOTH_NAME_INDEX)) {
            //Name error
        } else if (initcode & (1 << RQST_BLUETOOTH_PIN_INDEX)) {
            //PIN error
        } else {
            //Other error
        }
        _delay_ms(STANDBY_TIMEOUT);
    }
}
