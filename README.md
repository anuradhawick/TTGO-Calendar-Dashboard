# TTGO Calendar Dashboard

This was built as I was getting busy a schedule and Google calendar was becoming an important part of me being organized.
However, it is often quite distracting to check the calendar on phone throughout the day. There were plenty of occassions that I almost missed meetings. Here's the solution.

## Features

* Fetch events from the google calendar using a gscript.
* Show each event one by one on TFT display.
* Show time.
* Act as a display clock.

## Compiling

You need to have platform io installed. Compilation should follow the `platformio.ini` configuration.

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 112500
build_flags = 
	-'DWIFI_SSID="SSID"'
	-'DWIFI_PASSWD="PW"'
	-'DEAP_IDENTITY="ID"'
	-'DEAP_PASSWORD="PW"'
	-'DEAP_SSID="E-SSID"'
	-'DEAP_MODE=1'
	-'DCORE_DEBUG_LEVEL=4'
lib_deps = 
	bblanchon/ArduinoJson@^6.17.3
board_build.partitions = huge_app.csv
```

Simply commend out the `DEAP_MODE` flag if you're using home WiFi. In that case you can forget about all flags starting with `EAP`. If you're at work or university, consider the `EAP` flags.

## Notes
* Using HTTP/1.0 since the data payloads over HTTP is short and no need of chunking is needed.
* Implemented a simple and short HTTPS redirect manager to reduce program memeory usage.
* Animation takes the most Flash space ~ 800kb.
* Use python CV2 to convert JPEG frames to RGB565 images. You need to swap R,B channels to obtain proper colours. Check below for code!

## TODO

* WiFi Manager
* Buzzer Alarms
* Implement on M5Stick Plus

## Code for image conversion

```python
import cv2
from glob import glob
import numpy as np

# start configs
scale_percent = 1
jpeg_images_path = ""
# end configs

final_txt = "const int ani_height = {0};\nconst int ani_width = {1};\nconst int ani_frames = {2};\nconst unsigned short PROGMEM ani_imgs[][{3}]= {{"

texts = []
total_pixels = 0
image_shape = None

for image in glob(jpeg_images_path + "*.jpg"):
    img = cv2.imread(image)
    width = int(img.shape[1] * scale_percent)
    height = int(img.shape[0] * scale_percent)
    dim = (width, height)
    img = cv2.resize(img, dim, interpolation = cv2.INTER_AREA)
    image_shape = img.shape[:2]
    break

for n, image in enumerate(glob("*.jpg")):
    img = cv2.imread(image)
    
    width = int(img.shape[1] * scale_percent)
    height = int(img.shape[0] * scale_percent)
    dim = (width, height)
    img = cv2.resize(img, dim, interpolation = cv2.INTER_AREA)
    
    red = img[:,:,2].copy()
    blue = img[:,:,0].copy()

    # Channel correction
    img[:,:,0] = red
    img[:,:,2] = blue
    
    R5 = (img[...,0]>>3).astype(np.uint16) << 11
    G6 = (img[...,1]>>2).astype(np.uint16) << 5
    B5 = (img[...,2]>>3).astype(np.uint16)
    
    
    
    # Assemble components into RGB565 uint16 image
    RGB565 = R5 | G6 | B5
    
    total_pixels = len(RGB565.ravel())
    
    img_txt ="{" + ",".join([hex(x if x > 54 else 0) for x in RGB565.ravel()]) + "}"
    texts.append(img_txt)

final_txt = final_txt.format(image_shape[0], image_shape[1], len(texts), total_pixels) + ",\n".join(texts) + "};"

print(final_txt)
```