# Laser Monitor 

This circuit will monitor a laser cutter's filter control and air input, and ensure both are operating properly when the laser is on. If not, warn the user.

## Setup
* Connect the fume extractor control line to the female DB9 of the laser monitor.
  * If using V1.0.**0** hardware, must invert the pinout with an adapter cable. 
* Connect the male DB9 on the laser monitor to the laser cutter's fume control output.
* Cut the air line to the laser cutter, and install the flex host T-joint.
* Connect 3mm ID tube to the air transducer on the PCB, and the other side to the T-joint.
* Connect 12v power supply.
* Connect laser monitor via USB to computer, open serial terminal with baud rate 115200.
* TODO: Configuration via USB.

## Bill of Materials
* Assembled PCB (See Electronics Folder)
* 6x 3mm Plywood Laser Cut Case Parts (See Mechanical Folder)
* 12v >3A Barrel Jack Power Supply
* Andon Light Tower (12v) With Buzzer
  * Suggested that a blinking red light is used
* DB9 Extension Cable (Connect Laser to Laser Monitor)
* Flex Hose T-Joint
* 3mm ID Flex Hose (OD sized to fit current system, 8mm normal)
* SparkFun 14 Segment Dispaly COM-16916 (Optional) + QWIIC to 2.54mm DuPont cable
* Threaded Rubber Magnet (1/4-20 thread)
* Hardware
  * 4x M2.5 25mm Standoffs
  * 4x M2.5 12mm Standoffs
  * 4x M2.5 6mm Spacers
  * 8x M2.5 Screw
  * 8x M2.5 Nut
  * 4x M5 Bolt
  * 4x M5 Nut
