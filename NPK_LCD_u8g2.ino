#include <Arduino.h>
#include <U8g2lib.h>
#include <SoftwareSerial.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define BUTTON_UP_PIN 22     // PIN for UP button / Right Butotn
#define BUTTON_SELECT_PIN 24 // PIN for SELECT button
#define BUTTON_DOWN_PIN 23   // PIN for DOWN button / Left Button
#define BUTTON_MENU 20       // PIN for calling Menu
#define BUTTON_SCAN 21       // PIN scan data

int button_up_clicked = 0;     // Only perform action when button is clicked, and wait until another press
int button_select_clicked = 0; // Only perform action when button is clicked, and wait until another press
int button_down_clicked = 0;   // Only perform action when button is clicked, and wait until another press

U8G2_ST7565_ERC12864_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/13, /* data=*/11, /* cs=*/10, /* dc=*/9, /* reset=*/8);

const int NUM_ITEMS = 4;        // Number of Setting Sub Menu (Info, Connect, etc)
const int MAX_ITEM_LENGTH = 20; // Limit on the number of Sub Menu name characters
int current_screen = 0;         // Selected screen (0 = NPK Display Menu, 1 = Setting Menu)
int item_selected = 0;          // Selected Setting Sub Menu.
int current_page = 0;           // Status of the selected Page on the NPK Display Menu.
const int pinBright1 = 18;      // PIN for increasing Display Brightness
const int pinBright2 = 19;      // PIN for decreasing Screen Brightness
const int pinLCD = 7;           // LCD PIN for setting Brightness.
int brightness = 60;            // Initial value of brightness
int inMenu = 0;                 // Current Menu Location Status (0 = NPK Display Menu, 1 = Setting Menu)
int NPKcalled = 0;              // Sensor Status is taking data or not (0 = Sensor On, 1 = Sensor Off)

// NPK Sensor Settings
const byte code[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x07, 0x04, 0x08};
byte values[19];
SoftwareSerial mod(A8, A9); // Sensor Communication PIN

// Variable Initial Value Storage of Sensor Readings
int soiltemp = 0;
int soilhum = 0;
int ec = 0;
int ph = 0;
int nitrogen = 0;
int phosphorous = 0;
int potassium = 0;

// Variable Sensor Reading Value Results after being averaged (Filtered)
int tempbaru;
int humbaru;
int ecbaru;
int phbaru;
int nitrobaru;
int phosporbaru;
int potabaru;

// Variable Indicator Status NPK Value
String infoNitro;
String infoPhos;
String infoPotas;

// Contents of the Setting Menu
char menu_items[NUM_ITEMS][MAX_ITEM_LENGTH] = {
    {"1. INFO SSID"},
    {"2. CONNECT SSID"},
    {"3. KIRIM HP"},
    {"4. SIMPAN SD CARD"}};

// Box Frame on Setting Menu
void frame1()
{
    u8g2.drawFrame(3, 3, 120, 13);
}
void frame2()
{
    u8g2.drawFrame(3, 18, 120, 13);
}
void frame3()
{
    u8g2.drawFrame(3, 33, 120, 13);
}
void frame4()
{
    u8g2.drawFrame(3, 48, 120, 13);
}

// Box Frame Array
void (*frames[])() = {frame1, frame2, frame3, frame4};

// Function to Display NPK Value and NPK Value Status on NPK Display Menu
void displayNPKC()
{

    if (nitrobaru < 50)
    {
        infoNitro = "K";
    }
    else if (nitrobaru >= 50 && nitrobaru <= 200)
    {
        infoNitro = "I";
    }
    else
    {
        infoNitro = "B";
    }

    if (phosporbaru < 4)
    {
        infoPhos = "K";
    }
    else if (phosporbaru >= 4 && phosporbaru <= 14)
    {
        infoPhos = "I";
    }
    else
    {
        infoPhos = "B";
    }

    if (potabaru < 50)
    {
        infoPotas = "K";
    }
    else if (potabaru >= 50 && potabaru <= 200)
    {
        infoPotas = "I";
    }
    else
    {
        infoPotas = "B";
    }

    u8g2.firstPage();
    do
    {
        u8g2.setFont(u8g2_font_5x7_tr); // Using 8x13 font type with bold rendering
        u8g2.drawStr(3, 10, "Nitrogen: ");
        u8g2.drawStr(3, 19, "Phosphorus: ");
        u8g2.drawStr(3, 28, "Potasium(K): ");
        u8g2.drawStr(3, 37, "EC: ");

        u8g2.setCursor(65, 10);
        u8g2.print(nitrobaru);
        u8g2.setCursor(65, 19);
        u8g2.print(phosporbaru);
        u8g2.setCursor(65, 28);
        u8g2.print(potabaru);
        u8g2.setCursor(55, 37);
        u8g2.print(ecbaru);

        u8g2.drawStr(85, 10, "ppm");
        u8g2.drawStr(85, 19, "ppm");
        u8g2.drawStr(85, 28, "ppm");
        u8g2.drawStr(80, 37, "1/(Ohm.m)");

        u8g2.drawRFrame(10, 45, 106, 13, 5);
        u8g2.drawStr(16, 54, "N: ");
        u8g2.setCursor(30, 54);
        u8g2.print(infoNitro);
        u8g2.drawStr(46, 54, "P: ");
        u8g2.setCursor(66, 54);
        u8g2.print(infoPhos);
        u8g2.drawStr(86, 54, "K: ");
        u8g2.setCursor(106, 54);
        u8g2.print(infoPotas);
    } while (u8g2.nextPage());
}

// Function Displays Temperature, Humidity, pH and NPK Value Status on NPK Display Menu
void displayHumiTemp()
{
    u8g2.firstPage();
    do
    {
        u8g2.setFont(u8g2_font_6x13_tr); // Using 6x13 font type tr
        u8g2.drawStr(18, 20, "Suhu: ");
        u8g2.drawStr(18, 34, "Kelembaban: ");
        u8g2.drawStr(18, 48, "pH: ");

        u8g2.setCursor(85, 20);
        u8g2.print(tempbaru);
        u8g2.setCursor(85, 34);
        u8g2.print(humbaru);
        u8g2.setCursor(85, 48);
        u8g2.print(phbaru);

        u8g2.drawStr(100, 20, "C");
        u8g2.drawStr(100, 34, "%");
    } while (u8g2.nextPage());
}

// Function Display SSID Info on SSD Info Sub Menu in Setting Menu
void infoSSID()
{
    u8g2.setFont(u8g_font_6x13Br);
    u8g2.drawStr(7, 16, "Info SSID Perangkat");

    u8g2.setFont(u8g_font_6x12r);
    u8g2.drawStr(5, 37, "SSID: WIFIKU ");

    u8g2.setFont(u8g_font_6x12r);
    u8g2.drawStr(5, 53, "PASSWORD: IYAWIFIKU ");
}

// "Connecting SSID" function on SSID Info Sub Menu in Setting Menu
void connectSSID()
{
    u8g2.setFont(u8g_font_6x13Br);
    u8g2.drawStr(7, 37, "Connecting SSID");
}

// "Sending Readout Data to Cell Phone" Function on the SEND Cell Phone Sub Menu in the Setting Menu
void kirimHP() /
{
    u8g2.setFont(u8g_font_6x13Br);
    u8g2.drawStr(7, 37, "Mengirim Data");
}

// Function "save data to SD Card" on the SD CARD SAVE Sub Menu in the Setting Menu
void saveSD()
{
    u8g2.setFont(u8g_font_6x13Br);
    u8g2.drawStr(7, 37, "Menyimpan Data");
}

// Unnecessary
// void (*selectMenu[])() = {displayNPKC, displayHumiTemp, menu};

// Sensor Read Initial Value
void bacaSensor()
{
    if (mod.write(code, sizeof(code)) == 8)
    {
        for (byte i = 0; i < 19; i++)
        {

            values[i] = mod.read();
        }
    }
    soiltemp += values[6];
    soilhum += values[4];
    ec += values[8];
    ph += values[10];
    nitrogen += values[12];
    phosphorous += values[14];
    potassium += values[16];
}

// Filter Sensor Readings by Averaging 10 times the initial reading results.
void filterSensor()
{
    for (int i = 0; i < 10; i++)
    {
        bacaSensor();
        delay(100);
    }

    tempbaru = soilhum / 100.0;
    humbaru = soiltemp / 100.0;
    ecbaru = ec / 10.0;
    phbaru = ph / 100.0;
    nitrobaru = nitrogen / 10.0;
    phosporbaru = phosphorous / 10.0;
    potabaru = potassium / 10.0;

    soiltemp = 0;
    soilhum = 0;
    ec = 0;
    ph = 0;
    nitrogen = 0;
    phosphorous = 0;
    potassium = 0;
}

// Function Increase Screen Brightness
void brightnessup()
{
    brightness += 5;
    brightness = constrain(brightness, 0, 255);
    analogWrite(pinLCD, brightness);
    delay(20);
}

// Screen Brightness Lowering Function
void brightnessdown()
{
    brightness -= 5;
    brightness = constrain(brightness, 0, 255);
    analogWrite(pinLCD, brightness);
    delay(20);
}

// Function to Switch Menus (NPK Display Menu and Setting Menu)
void menuHandle()
{
    inMenu = 1;
    current_page = 2;
}

// Function Displays the Setting Menu and Logic for the Button (Up/Left, Down/Right, and Select) in the Setting Menu.
void menu()
{
    // In the Setting Menu, the Up/Left, Down/Right, and Select buttons can be used.
    if (current_screen == 0 && inMenu == 1)
    {
        if ((digitalRead(BUTTON_DOWN_PIN) == LOW) && (button_down_clicked == 0))
        {
            item_selected += 1;
            button_down_clicked = 1;
            if (item_selected > 3)
            {
                item_selected = 0;
            }
        }

        if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked == 0))
        {
            item_selected -= 1;
            button_up_clicked = 1;
            if (item_selected < 0)
            {
                item_selected = 3;
            }
        }

        if ((digitalRead(BUTTON_DOWN_PIN) == HIGH) && (button_down_clicked == 1) && inMenu == 1)
        { // unclick
            button_down_clicked = 0;
        }
        if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked == 1))
        { // unclick
            button_up_clicked = 0;
        }
    }

    if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (button_select_clicked == 0))
    {                              // select button clicked, jump between screens
        button_select_clicked = 1; // set button to clicked to only perform the action once
        if (current_screen == 0)
        {
            current_screen = 1;
        } // menu items screen --> screenshots screen
        else
        {
            current_screen = 0;
        } // qr codes screen --> menu items screen
    }
    if ((digitalRead(BUTTON_SELECT_PIN) == HIGH) && (button_select_clicked == 1))
    { // unclick
        button_select_clicked = 0;
    }

    u8g2.firstPage();
    do
    {
        if (current_screen == 0)
        {
            frames[item_selected]();
            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.drawStr(5, 12, menu_items[0]);

            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.drawStr(5, 27, menu_items[1]);

            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.drawStr(5, 42, menu_items[2]);

            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.drawStr(5, 57, menu_items[3]);
        }

        else if (current_screen == 1)
        { 
            if (item_selected == 0)
            {
                infoSSID();
            }
            else if (item_selected == 1)
            {
                connectSSID();
            }
            else if (item_selected == 2)
            {
                // displayHumiTemp();
                kirimHP();
            }
            else
            {
                saveSD();
                // displayNPKC();
            }
        }
    } while (u8g2.nextPage());
}

// Function to activate the NPK Sensor (Take Readings)
void scanNPK()
{

    NPKcalled = 1;
}

void setup()
{
    Serial.begin(4800);
    u8g2.setColorIndex(1); // Set the color to white
    u8g2.begin();
    u8g2.setBitmapMode(1);
    u8g2.setContrast(30); // Adjusting LCD Screen Contrast

    mod.begin(4800);
    pinMode(pinBright1, INPUT_PULLUP);
    pinMode(pinBright2, INPUT_PULLUP);

    pinMode(BUTTON_MENU, INPUT_PULLUP);       // Menu Button / Berganti Menu
    pinMode(BUTTON_UP_PIN, INPUT_PULLUP);     // Up/Left button
    pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP); // Select button
    pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);   // Down/Right button
    pinMode(BUTTON_SCAN, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(pinBright1), brightnessup, FALLING);
    attachInterrupt(digitalPinToInterrupt(pinBright2), brightnessdown, FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON_MENU), menuHandle, FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON_SCAN), scanNPK, FALLING);

    brightness = constrain(brightness, 0, 255);
    analogWrite(pinLCD, brightness); // Set the Initial Brightness from the preset starting value.

    current_page = 0;
    inMenu = 0;
    NPKcalled = 0;
}

void loop()
{
    Serial.print("current_screen: ");
    Serial.print(current_screen);
    Serial.print(" | current_page: ");
    Serial.print(current_page);
    Serial.print(" | inMenu: ");
    Serial.print(inMenu);
    Serial.print(" | NPKcalled: ");
    Serial.println(NPKcalled);

    // Logic for Button (Up/Left and Down/Right) in Setting Menu.
    // On the NPK Button Display Menu can only use the Up/Left and Down/Right Buttons
    if (inMenu == 0)
    {
        if ((digitalRead(BUTTON_DOWN_PIN) == LOW) && (button_down_clicked == 0))
        {
            current_page += 1;
            button_down_clicked = 1;
            if (current_page > 1)
            {
                current_page = 0;
            }
        }

        if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked == 0))
        {
            current_page -= 1;
            button_up_clicked = 1;
            if (current_page < 0)
            {
                current_page = 1;
            }
        }

        if ((digitalRead(BUTTON_DOWN_PIN) == HIGH) && (button_down_clicked == 1))
        { // unclick
            button_down_clicked = 0;
        }
        if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked == 1))
        { // unclick
            button_up_clicked = 0;
        }

        if (NPKcalled == 1)
        {
            filterSensor();
        }
        if ((digitalRead(BUTTON_SCAN) == LOW))
        {
            NPKcalled = 0;
        }
    }

    if (current_page == 0)
    {
        displayNPKC();
    }
    else if (current_page == 1)
    {
        displayHumiTemp();
    }
    else if (current_page == 2)
    {
        menu();
        if ((digitalRead(BUTTON_MENU) == LOW))
        {
            inMenu = 0;
            current_page = 0;
        }
    }

    // delay(100);
}
