# GianGiacomoMora firmware

You can flash this firmware on an ESP32 using esp-idf and the following command.
```
idf.py flash monitor -p <your serial port>
```

The firmware will work in the following manner:

- for 10 seconds scan for beacons
- for 10 second advertise a beacon
