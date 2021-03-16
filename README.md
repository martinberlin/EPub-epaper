# An EPub Reader for the ESP32 epaper devices

Using [EPub-Inkplate project from Turgu1](https://github.com/turgu1/EPub-InkPlate) as a base. Please refer to his Repository readme to see the latest news.

## Goals of this project

- [ ] Refactor the epaper output code to make it multi-epaper display
- [ ] First display supported to be [LILYGO parallel eink ED047TC1](https://github.com/martinberlin/cale-idf/wiki/Model-parallel-ED047TC1.h)
- [ ] Support for 4-button actions
- [ ] Support for Tocuh (L58 IC)

Some pictures from the InkPlate-6 version:

<img src="doc/pictures/IMG_1377.JPG" alt="picture" width="300"/><img src="doc/pictures/IMG_1378.JPG" alt="picture" width="300"/>
<img src="doc/pictures/IMG_1381.JPG" alt="picture" width="300"/>


## Characteristics

The first release functionalities:

- TTF, and OTF embedded fonts support
- Normal, Bold, Italic, Bold+Italic face types
- Bitmap images dithering display (JPEG, PNG)
- EPub (V2, V3) book format subset
- UTF-8 characters
- InkPlate tactile keys (single and double click to get six buttons)
- Screen orientation (buttons located to the left, right, down positions from the screen)
- Left, center, right, and justify text alignments
- Indentation
- Some basic parameters and options
- Limited CSS formatting
- WiFi-based documents download
- Battery state and power management (light, deep sleep, battery level display)

Some elements to consider in the future (no specific order of priority):

- External keypad integration (through i2c)
- Various views for the E-Books list
- Table of content
- Hyperlinks (inside an e-book)
- Others document download method (Dropbox, Calibre, others)
- Multiple fonts choices selectable by the user
- More CSS formatting
- Footnote management
- Kerning
- TXT, MOBI book formats
- `<table>` formatting
- Page progression direction: Right then left
- Notes
- Bookmarks
- Other elements proposed by users

And potentially many more...

### Runtime environment

The EPub-InkPlate application requires that a micro-SD Card be present in the device. This micro-SD Card must be pre-formatted with a FAT32 partition. Two folders must be present in the partition: `fonts` and `books`. You must put the base fonts in the `fonts` folder and your EPub books in the `books` folder. The books must have the extension `.epub` in lowercase. 

Height font types are supplied with the application. For each type, there are four fonts supplied, to support regular, bold, oblique, and bold-italic glyphs. The application offers the user to select one of those font types as the default font. The fonts have been cleaned-up and contain only Latin-1 glyphs.

Another font is mandatory. It can be found in `SDCard/fonts/drawings.otf` and must also be located in the micro-SD Card `fonts` folder. It contains the icons presented in parameters/options menus.

The `SDCard` folder under GitHub reflects what the micro-SD Card should look like. One file is missing there is the `books_dir.db` that is managed by the application. It contains the meta-data required to display the list of available e-books on the card and is automatically maintained by the application. It is refreshed at boot time and when the user requires it to do so through the parameters menu. The refresh process takes some time (between 5 and 10 seconds per e-book) but is required to get fast e-book directory list on screen.


## Development environment

[Visual Studio Code](https://code.visualstudio.com/) is the code editor I'm using. The [PlatformIO](https://platformio.org/) extension is used to manage application configuration for both Linux and the ESP32.

All source code is located in various folders:

- Source code used by both Linux and InkPlate is located in the `include` and `src` folders
- Source code in support of Linux only is located in the `lib_linux` folder
- Source code in support of the InkPlate device (ESP32) only are located in the `lib_esp32` folder
- The FreeType library for ESP32 is in folder `lib_freetype`

The file `platformio.ini` contains the configuration options required to compile both Linux and InkPlate applications.

Note that source code located in folders `old` and `test` is not used. It will be deleted from the project when the application development will be completed.

### ESP-IDF configuration specifics

The EPub-InkPlate application requires some functionalities to be properly set up within the ESP-IDF. To do so, some parameters located in the `sdkconfig` file must be set accordingly. This must be done using the menuconfig application that is part of the ESP-IDF. 

The ESP-IDF SDK must be installed in the main user folder. Usually, it is in folder ~/esp. The following location documents the installation procedure: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html . Look at Steps 1 to 4 (Setting Up Development Environment).

The following command will launch the application (the current folder must be the main folder of EPub-InkPlate):

  ```
  $ idf.py menuconfig
  ```
  
The application will show a list of configuration aspects. 

The following elements have been done (No need to do it again):
  
- **PSRAM memory management**: The PSRAM is an extension to the ESP32 memory that offers 4MB+4MB of additional RAM. The first 4MB is readily available to integrate into the dynamic memory allocation of the ESP-IDF SDK. To configure PSRAM:

  - Select `Component Config` > `ESP32-Specific` > `Support for external, SPI-Connected RAM`
  - Select `SPI RAM config` > `Initialize SPI RAM during startup`
  - Select `Run memory test on SPI RAM Initialization`
  - Select `Enable workaround for bug in SPI RAM cache for Rev 1 ESP32s`
  - Select `SPI RAM access method` > `Make RAM allocatable using malloc() as well`

  Leave the other options as they are. 

- **ESP32 processor speed**: The processor must be run at 240MHz. The following line in `platformio.ini` request this speed:

    ```
    board_build.f_cpu = 240000000L
    ```
  You can also select the speed in the sdkconfig file:

  - Select `Component config` > `ESP32-Specific` > `CPU frequency` > `240 Mhz`

- **FAT Filesystem Support**: The application requires the usage of the micro SD card. This card must be formatted on a computer (Linux or Windows) with a FAT32 partition (maybe not required as this is the default format of brand new cards). The following parameters must be adjusted in `sdkconfig`:

  - Select `Component config` > `FAT Filesystem support` > `Max Long filename length` > `255`
  - Select `Number of simultaneously open files protected  by lock function` > `5`
  - Select `Prefer external RAM when allocating FATFS buffer`
  - Depending on the language to be used (My own choice is Latin-1 (CP850)), select the appropriate Code Page for filenames. Select `Component config` > `FAT Filesystem support` > `OEM Code Page...`. DO NOT use Dynamic as it will add ~480KB to the application!!
  - Also select `Component config` > `FAT Filesystem support` > `API character encoding` > `... UTF-8 ...`

- **HTTP Server**: The application is supplying a Web server (through the use of HTTP) to the user to modify the list of books present on the SDCard. The following parameters must be adjusted:
  - Select `Component config` > `HTTP Server` > `Max HTTP Request Header Length` > 1024
  - Select `Component config` > `HTTP Server` > `Max HTTP URI Length` > 1024

The following is not configured through *menuconfig:*

- **Flash memory partitioning**: the file `partitions.csv` contains the table of partitions required to support the application in the 4MB flash memory. The factory partition has been set to be ~2.4MB in size (OTA is not possible as the application is too large to accomodate this feature; the OTA related partitions have been commented out...). In the `platformio.ini` file, the line `board_build.partitions=...` is directing the use of these partitions configuration.
  
