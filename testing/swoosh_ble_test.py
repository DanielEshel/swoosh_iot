import asyncio
from bleak import BleakClient, BleakScanner

SERVICE_UUID = "12345678-1234-1234-1234-1234567890ab"
CHAR_UUID = "12345678-1234-1234-1234-1234567890ac"

# Maps user input to ESP32 servo commands
COMMANDS = {
    "left": "0",
    "right": "1",
    "stop": "2",
}


async def main():
    print("Scanning for ESP32...")
    devices = await BleakScanner.discover()

    esp_address = None
    for d in devices:
        if d is not None:
            if "SwooshESP32" in d.name:
                esp_address = d.address
                print(f"Found ESP32 at {esp_address}")
                break

    if esp_address is None:
        print("ERROR: ESP32 not found. Make sure it is powered and advertising.")
        return

    print("Connecting to ESP32...")
    async with BleakClient(esp_address) as client:
        print("Connected!")

        # Loop: keep reading user commands
        while True:
            cmd = input("Enter command (left/right/stop/exit): ").strip().lower()

            if cmd == "exit":
                print("Exiting...")
                break

            if cmd not in COMMANDS:
                print("Invalid command. Try: left, right, stop, exit")
                continue

            value = COMMANDS[cmd]

            print(f"Sending command '{value}' to ESP32...")
            await client.write_gatt_char(CHAR_UUID, value.encode())

        print("Disconnected.")


# Run the asyncio loop
asyncio.run(main())
