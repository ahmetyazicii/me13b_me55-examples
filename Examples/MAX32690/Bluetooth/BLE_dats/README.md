# BLE_dats

Refer to the [BLE_datc_dats](../../../Libraries/Cordio/docs/Applications/BLE_datc_dats.md) documentation in the Cordio Library.

## Software

### Project Usage

Universal instructions on building, flashing, and debugging this project can be found in the **[MSDK User Guide](https://analogdevicesinc.github.io/msdk/USERGUIDE/)**.

### Required Connections

If using the MAX32690EVKIT:
-   Connect a USB cable between the PC and the CN2 (USB/PWR) connector.
-   Install headers JP7(RX\_EN) and JP8(TX\_EN).
-   Open a terminal application on the PC and connect to the EV kit's console UART at 115200, 8-N-1.

If using the MAX32690FTHR:
-   Connect a USB cable between the PC and the J5 (USB/PWR) connector.
-   Open a terminal application on the PC and connect to the EV kit's console UART at 115200, 8-N-1.

### Project-Specific Build Notes
* Setting `TRACE=1` in [**project.mk**](project.mk) initializes the on-board USB-to-UART adapter for
viewing the trace messages and interacting with the application. Port uses settings:
    - Baud            : 115200  
    - Char size       : 8  
    - Parity          : None  
    - Stop bits       : 1  
    - HW Flow Control : No  
    - SW Flow Control : No  
* Setting `SBT=1` in [**project.mk**](project.mk) enables Secure Boot Tools for the project
