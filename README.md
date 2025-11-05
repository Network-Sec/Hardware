# Hardware
Some smalltime Hardware experiments, ESP, Raspi Pico, NRF24, ... nothing fancy, just trying to find new distractions, some relaxing projects away from real work.

## Jammers
### Inspiration
- https://cifertech.net/rf-clown-your-portable-ble-bluetooth-jamming-tool/
- https://github.com/cifertech/RF-Clown
- https://github.com/SpacehuhnTech/esp8266_deauther/

### Results
- bruteforceJammer.ino (similar to RF Clown)
- deauthJammer.ino (took code parts from SpaceHuhn deauther)
  
We only had ESP8266 available, so we built several custom versions with our own code. While we managed to get it all working, including nice little OLED, the results were... don't wanna hurt anyone's feelings, but these < 1mW RF adapters do exactly nothing on modern Wifi. I don't know why it's all over (my) Social Media - highly dig nicely designed boards, cool writeups, open source code, all the effort that people make and share. But I don't like it being framed as it would work for real. It does not. Even with 3 highpower Alfa Adapters and Bettercap or other sophisticated tools, it's getting harder to actually kick someone or make them fall for EvilTwin, lots of protections among regular technological advancements have found their way into consumer grade Hardware for over a decade, DNS-Rebind, 5 Ghz, ... so TL;DR: We found the best bet with Wifi is to a.) use it as surveillance tool (beacons can give you past info and a look into someone's living room) b.) crack hashes where it's still possible. Go with real adapters or SDR for stuff like replay attacks. We haven't yet found other attacks that would still work reliably.

![Jammers.jpg](./Jammers.jpg)

Since it's more of a spare time activity, we'll continue with Pi Pico and a few more adapters.
