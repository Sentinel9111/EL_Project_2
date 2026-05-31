# Waterkwaliteitsboei

`pio run -t uploadfs -t upload -t monitor`

## Installatie

- `git clone git@github.com:Sentinel9111/EL_Project_2.git` 
- `git remote add origin git@github.com:Sentinel9111/EL_Project_2.git` 


- Stel Wi-Fi netwerk in via /include/credentials.h 
- Stel tijdsperiode in in main.cpp (41 `unsigned long delayTime = 60000; // 3600000 for 1 hour`) 
- en in /data/js/main.js (148 `setInterval(fetchData, 30000); // 3600000 for 1 hour`) 
- upload de code via de knop in de IDE 
- daarna, ``pio run -t uploadfs -t upload -t monitor`` 
- Zoek dan naar de serial monitor voor het IP, en open deze in de browser om de website te zien.

## Documentatie

> "The code speaks for itself"