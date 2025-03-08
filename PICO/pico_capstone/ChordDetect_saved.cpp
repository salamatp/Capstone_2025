#include <LCD_I2C.hpp>
#include <memory>
#include "pico/stdlib.h"

int main() {
    constexpr auto I2C = i2c0; // Define the I2C instance
    constexpr auto SDA = 1;     // GPIO for SDA
    constexpr auto SCL = 2;     // GPIO for SCL
    constexpr auto I2C_ADDRESS = 0x27; // LCD I2C address
    constexpr auto LCD_COLUMNS = 16;
    constexpr auto LCD_ROWS = 2;

    // Initialize the LCD object using smart pointer
    auto lcd = std::make_unique<LCD_I2C>(I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS, I2C, SDA, SCL);

    // Define a custom heart character
    constexpr LCD_I2C::array HEART = {0x00, 0x0A, 0x1F, 0x1F, 0x1F, 0x0E, 0x04, 0x00};
    constexpr auto HEART_LOC = 0;
    lcd->CreateCustomChar(HEART_LOC, HEART);

    constexpr size_t PAUSE_MS = 2000; // Delay in milliseconds

    lcd->BacklightOn();

    while (true) {
        lcd->SetCursor(0, 0);
        lcd->PrintString("RaspberryPi Pico");
        lcd->SetCursor(1, 0);
        lcd->PrintString("I2C LCD Library ");
        sleep_ms(PAUSE_MS);

        lcd->SetCursor(0, 0);
        lcd->PrintString("I2C LCD Library ");
        lcd->SetCursor(1, 0);
        lcd->PrintString("Made with love ");
        lcd->PrintCustomChar(HEART_LOC);
        sleep_ms(PAUSE_MS);

        lcd->SetCursor(0, 0);
        lcd->PrintString("Made with love ");
        lcd->PrintCustomChar(HEART_LOC);
        lcd->SetCursor(1, 0);
        lcd->PrintString("       by       ");
        sleep_ms(PAUSE_MS);

        lcd->SetCursor(0, 0);
        lcd->PrintString("       by       ");
        lcd->SetCursor(1, 0);
        lcd->PrintString("Arthur Morgan");
        lcd->PrintCustomChar(HEART_LOC);
        sleep_ms(PAUSE_MS);
    }

    return 0;
}
