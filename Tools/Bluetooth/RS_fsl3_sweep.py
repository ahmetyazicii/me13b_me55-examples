#! /usr/bin/env python3

###############################################################################
 #
 # Copyright (C) 2022-2023 Maxim Integrated Products, Inc. (now owned by
 # Analog Devices, Inc.),
 # Copyright (C) 2023-2024 Analog Devices, Inc.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #     http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 #
 ##############################################################################

## RS_fsl3_sweep.py
 #
 # Sweep through tests executed by RS_fsl3.py
 #

import sys
import argparse
from argparse import RawTextHelpFormatter
from pprint import pprint
from time import sleep
from BLE_hci import BLE_hci
from BLE_hci import Namespace
from RS_fsl3 import RS_fsl3

# Setup the command line description text
descText = """
RS_fsl3.py test sweep.
"""

# Parse the command line arguments
parser = argparse.ArgumentParser(description=descText, formatter_class=RawTextHelpFormatter)
parser.add_argument('ipAddress',help='Instrument IP address')
parser.add_argument('serialPort',help='DUT HCI serial port')
parser.add_argument("--tx_pwr", default=4, help="TX POWE (0, 4)")

args = parser.parse_args()
pprint(args)

# Create the BLE_hci objects
hciDUT = BLE_hci(Namespace(serialPort=args.serialPort, monPort="", baud=115200))
hciDUT.resetFunc(None)

# Wait for calibration
sleep(1)

# Create the RS_FSL3 spectrum analyzer object
sa = RS_fsl3(Namespace(ipAddress=args.ipAddress))

# Setup the DUT to TX at max power
hciDUT.txPowerFunc(Namespace(power=int(args.tx_pwr), handle=None))

channels = [0,1,2,10,19,30,36,37,38,39]

for channel in channels:
    # Start transmitting
    hciDUT.txTestFunc(Namespace(channel=channel, packetLength=255, payload=0, phy=1))
    sleep(1)

    if(channel > 36):
        # Start the OB2 test
        retval = sa.testRB2()

        if(retval != True):
            print("RB2 test failed")
            sys.exit(1)
    else:
        # Start the OBW test
        retval = sa.testOBW(ch=channel)

        if(retval != True):
            print("OBW test failed")
            sys.exit(1)

    if(channel == 0):
        # Start the OB1 test
        retval = sa.testRB1()

        if(retval != True):
            print("RB1 test failed")
            sys.exit(1)

print("Test passed!")
sys.exit(0)
