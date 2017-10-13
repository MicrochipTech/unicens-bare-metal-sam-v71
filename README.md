# UNICENS example for Atmel SAM V71

This project is a bare metal example integration of the [UNICENS library](https://github.com/MicrochipTech/unicens) for the Atmel SAM V71 controller.

### Needed components

__Hardware:__
* Atmel SAM V71 Xplained Ultra Evaluation Kit
* Atmel SAM-ICE (min. HW-Version 7)
* OS81118 Phy+Board Variant 3 or OS81210/OS81212/OS81214 Phy+Board Variant 3
* Depending on your use case, you can start with any devices of the K2L Slim Board Family
    https://www.k2l.de/products/34/MOST150%20Slim%20Board%20Family/

__Software:__
* Microsoft Windows 7 or newer
* Atmel Studio 7
    http://www.microchip.com/development-tools/atmel-studio-7
    
To get the source code, enter:
```bash
$ git clone --recurse-submodules https://github.com/MicrochipTech/unicens-bare-metal-sam-v71.git
```
### Build

* Open Atmel Studio 7
* __File -> Open -> Project/Solution... (Ctrl+Shift+O)__
* Select __[your-projects]\\audio-source\\SAMV71-UNICENS-example.atsln__
* Navigate to __Project -> Properties -> Tool__ and select your connected __SAM-ICE__.  
Use __SWD__ as interface and save the configuration file.
* Build the project with __F7__
* Build the project, flash it to the SAM V71 and start debugging with __F5__

### Change Network Configuration

The configuration of the entire network is done via a single XML file.  
It's located at __[your-projects]\\audio-source\config.xml__  

**Hint:** Edit the config.xml file within the Atmel Studio.  
It will validate it and help filling the correct tags and attributes by using the XML schema  __unicens.xsd__ in the same folder.

Once you edited it, double click the __[your-projects]\\audio-source\\convertXML.bat__ batch file.  
It will interpret the XML file and generate a static C-Source code file at  
__[your-projects]\\audio-source\\samv71-ucs\\src\default_config.c__  
After successful conversion, you need to build the project again and download it to the hardware in order to apply the new network configuration.