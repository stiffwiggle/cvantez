# cvantez
Arduino project to convert CV to MIDI CC. 

Similar to my other project triggercut, but more complicated. Also Arduino Uno based. This project accepts voltages on six different inputs and converts them to MIDI CC messages, output via the MIDI port. It's also designed to use a 2x16 LCD screen of the typical variety, as well as two buttons - one to change the selection, and one to change the value. 

I use this device to convert Eurorack control voltages (CVs) to MIDI CCs destined for an MFB Tanzbär. The buttons allow the user to cycle between the CCs that are pre-mapped by the Tanzbär to adjust the sounds each drum voice produces. 
