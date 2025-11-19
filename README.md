# Plant_Discord_Bot_ESP-32
Embedded system project developed on ESP-IDF and leverageing FreeRTOS for event-driven architecture to provide real-time plant moisture monitoring and asynchronous (humorus) alerts via Discord.


## 🪴 ESP32 Plant Bot Configuration Guide

### **Overview**

This project configures an **ESP32** as a smart plant monitor that reads moisture levels using an Analog-to-Digital Converter (**ADC**) and sends real-time status updates using events read from the ADC to a specified **Discord** channel. It utilizes **FreeRTOS** tasks and the **ESP-IDF** event system for a decoupled, reliable operation.

---

### **Prerequisites**

* **ESP-IDF** (v4.x or later) installed and configured.
* A **Discord bot token** (obtained from the Discord Developer Portal).
* A Wi-Fi network connection.
* An ADC moisture sensor (or equivalent analog input) connected to **GPIO 36** on an **ESP-32**.

---

### **1. Adding the \`abobija/esp-discord\` Component**

The Discord API is provided by the \`esp-discord\` component. You must add this component to your project's component registry.

1.  **Create the \`idf_component.yml\` file:**
    In your project's main directory, create a file named \`idf_component.yml\` (if it doesn't exist).

2.  **Add the dependency:**
    Edit \`idf_component.yml\` to include the \`esp-discord\` component from the IDF component registry:

    ```yaml
    dependencies:
      abobija/esp-discord:
        version: '*'  # Use the latest stable version
    ```

3.  **Run the component installer:**
    The ESP-IDF build system will automatically fetch this component when you run \`idf.py build\` or \`idf.py menuconfig\`.

---

### **2. Configuring Network and Discord Secrets**

All configuration is handled using the ESP-IDF **\`menuconfig\`** system.

1.  Run the configuration tool:
    ```bash
    idf.py menuconfig
    ```

2.  **Setting Wi-Fi Credentials**
    * Navigate to: **Example Connection Configuration**
    * Set your Wi-Fi **SSID** and **Password**.

3.  **Setting Discord Configuration**
    * **Configure the Bot Token:**
        * Navigate to: **Component config** $\rightarrow$ **Discord**
        * Set the **Discord Bot Token** (your secret token from the Discord Developer Portal).

4.  **Configure Application-Specific Parameters**
    * Navigate to: **Application configuration**
    * **Set `CONFIG_MY_APP_DISCORD_CHANNEL_ID`:** Enter the **Discord Channel ID** where the bot should post messages (e.g., \`123456789012345678\`).
    * **Set `CONFIG_ADC_THRESHOLD_VAL`:** Enter the **Analog Reading Threshold** (e.g., \`1900\`). Readings **above** this value are considered **DRY** (needs water); readings below are **WET**.

5. **Change Partition Table size**
   * Navigate to: **Partition Table**
   * **Set `Single factory app, no OTA ` to `Single factory app` (large), no OTA.
     
6.  **Save and Exit:** Select **<Save>** and then **<Exit>** to save your configuration to \`sdkconfig\`.

---

### **3. Flashing the Program to the Device**

Once configured, use the provided \`esptool\` command for flashing, ensuring your device is connected and the correct port is specified.

1.  **Create and Activate a Python Virtual Environment (venv):**
    ```bash
    python3 -m venv venv
    source venv/bin/activate
    # Install esptool if not already installed globally or in venv
    pip install esptool pyserial
    ```

2.  **Build the Project:**
    ```bash
    idf.py build
    ```

3.  **Flash the Compiled Program:**
    Use the following command, making sure to replace the placeholder port with your actual device port.

    ```bash
    python -m esptool --port "<your_usb_connection_port_here>" --chip auto --baud 921600 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/plant_bot.bin
    ```

    ***Note: Windows port will look something like `COM_7` and Mac or Linux systems will look something like this `/dev/tty.usbserial-0001`

The device will now reboot and begin monitoring your plant and sending alerts to Discord.

To view the Serial output, `python -m serial.tools.miniterm "<your_usb_connection_port_here>" 115200`
