#include <Windows.h>
#include <iostream>
#include <conio.h>
#include <string>
#pragma warning(disable:6031)

#define randomizeserial_code CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0920, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define nullserial_code CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0213, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define customserial_code CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0492, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define changeserialoneveryreq_code CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0428, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define initiatespoof_code CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0069, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

int selection = 0;
DWORD BytesReturned = 0;
std::string desired_serial;

struct CustomRequest_Struct {
    const char* customserial;
};

int main()
{
    SetConsoleTitleA("Spoof Control"); system("color 0b");

    HANDLE driver_handle = CreateFileA("\\\\.\\PhysicalDrive0", GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);

    if (driver_handle == INVALID_HANDLE_VALUE) {
        system("cls"); printf(" Invalid handle value... Make sure you ran as admin!\n"); _getch(); exit(0);
    }
    else {
        printf(" [1] Randomize Serial\n [2] Custom Serial\n [3] NULL serial\n [4] Initiate Spoofer\n\n > "); std::cin >> selection;
        if (selection == 1) {
            DeviceIoControl(driver_handle, randomizeserial_code, 0, 0, 0, 0, &BytesReturned, NULL);
            system("cls"); printf(" Serial randomized!\n"); system("wmic diskdrive get serialnumber"); _getch(); exit(0);
        }
        else if (selection == 2) {
            system("cls"); printf(" Custom serialnumber: "); std::cin >> desired_serial;
            CustomRequest_Struct request;
            request.customserial = desired_serial.c_str();
            DeviceIoControl(driver_handle, customserial_code, &request, sizeof(request), 0, 0, &BytesReturned, NULL);
            printf("\n Spoofed to custom serial!\n"); system("wmic diskdrive get serialnumber");
            _getch(); exit(0);
        }
        else if (selection == 3) {
            DeviceIoControl(driver_handle, nullserial_code, 0, 0, 0, 0, &BytesReturned, NULL); Sleep(1000);
            system("cls"); printf(" Nulled serial!\n"); system("wmic diskdrive get serialnumber"); _getch(); exit(0);
        }
        else if (selection == 4) {
            DeviceIoControl(driver_handle, initiatespoof_code, 0, 0, 0, 0, &BytesReturned, NULL); Sleep(1000);
            system("cls"); printf(" Sent request to driver, spoof should be initiated!\n"); system("wmic diskdrive get serialnumber"); _getch(); exit(0);
        }
        else {
            system("cls"); printf(" Entered number is not a valid option!\n"); _getch(); exit(0);
        }
    }

    return 0;
}