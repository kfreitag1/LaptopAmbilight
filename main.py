import serial, math, time, mss, numpy, cv2
from PIL import Image, ImageStat
import serial.tools.list_ports as list_ports
from colorsys import rgb_to_hls, hls_to_rgb


# CONSTANTS
NUM_LED_SIDES = 0
NUM_LED_TOP = 0
FADE_SPEED = 0
REDUCTION_FACTOR = 6
COLOR_REGION = 150
ACKNOWLEDGE = 6

####################
# MAIN FUNCTION
####################

def main():
    # Pick a port
    ports = list_ports.comports()
    print([port.device for port in ports])

    # Get init data from user
    port = ports[int(input("Which port? (0-?)"))]

    global NUM_LED_SIDES
    global NUM_LED_TOP
    global FADE_SPEED
    NUM_LED_SIDES = int(input("How many LEDS on the sides?"))
    NUM_LED_TOP = int(input("How many LEDS on the top?"))
    FADE_SPEED = int(input("How fast to fade? (5-60)"))

    # Configure serial output
    ser = serial.Serial()
    ser.baudrate = 115200
    ser.port = port.device
    ser.writeTimeout = 30000
    ser.open()

    # Send init data
    init_bytes = bytearray()
    init_bytes.append(2*NUM_LED_SIDES + NUM_LED_TOP)
    init_bytes.append(FADE_SPEED)

    print(init_bytes)

    print("Sending init data...")
    ser.write("init\n".encode())
    print(ser.read())
    ser.write(init_bytes)
    
    return_byte = ser.read()
    print(return_byte)

    # Init image handler
    sct = mss.mss()

    while True:
        pixel_bytes = getByteArrayForScreen(sct)
        ser.write(pixel_bytes)

        time.sleep(0.03)

    ser.close()

####################
# FUNCTIONS
####################


def adjust_color_lightness(color):
    r, g, b = color
    h, l, s = rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)
    l = max(min(l**2, 1.0), 0.0)
    r, g, b = hls_to_rgb(h, l, s)
    return [int(r * 255), int(g * 255), int(b * 255)]

def avgColorFromArea(img, area):
    new_img = img.crop(area)
    color = ImageStat.Stat(new_img).median
    adj_color = adjust_color_lightness(color)
    if sum(adj_color) <= 10:
        adj_color = [0,0,0]

    return adj_color

def getByteArrayForScreen(sct):
    img_src = sct.grab(sct.monitors[0])
    img = Image.frombytes("RGB", img_src.size, img_src.bgra, "raw", "BGRX")
    img = img.reduce(REDUCTION_FACTOR)
    # img.show()

    w, h = img.size
    height_pixels = int(h / NUM_LED_SIDES)
    width_pixels = int(w / NUM_LED_TOP)
    depth_pixels = int(COLOR_REGION / REDUCTION_FACTOR)

    border_colors = []

    # Get colors for 3 borders of monitor
    for left_index in range(0, NUM_LED_SIDES):
        area = (0, left_index*height_pixels, depth_pixels, (left_index+1)*height_pixels)
        border_colors.append(avgColorFromArea(img, area))
    border_colors.reverse()

    for top_index in range(0, NUM_LED_TOP):
        area = (top_index*width_pixels, 0, (top_index+1)*width_pixels, depth_pixels)
        border_colors.append(avgColorFromArea(img, area))

    for right_index in range(0, NUM_LED_SIDES):
        area = (w-depth_pixels, right_index*height_pixels, w, (right_index+1)*height_pixels)
        border_colors.append(avgColorFromArea(img, area))

    # Create byte list
    byte_list = bytearray()

    for border_color in border_colors:
        for pixel in border_color:
            byte_list.append(pixel)
    # byte_list.append(10) # Terminate with newline character
    
    return byte_list

####################
# INITIALIZATION
####################

if __name__ == "__main__":
    main()
