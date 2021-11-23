
from PIL import Image, ImageOps
import math

img_name="cat"
im = Image.open(img_name+".png")
# convert to grayscale
im = im.convert(mode='L')
#im.thumbnail((SCREEN_WIDTH, SCREEN_HEIGHT), Image.ANTIALIAS)


# Write out the output file.
with open(img_name+".h", 'w') as f:
    with open("test.txt","w") as testfile:
        f.write("const uint32_t {}_width = {};\n".format(img_name, im.size[0]))
        f.write("const uint32_t {}_height = {};\n".format(img_name, im.size[1]))
        f.write(
            "const uint8_t {}_data[({})] = {{\n".format(img_name, im.size[0]*im.size[1])
        )
        for y in range(0, im.size[1]):
            for x in range(im.size[0]):
                whole_byte=im.getpixel((x,y))
                nibble=whole_byte>>4
                testfile.write("*" if nibble>7 else " ")
                f.write("0x{:02X}, ".format(nibble))
            f.write("\n\t")
            testfile.write("\n")
        f.write("};\n")

