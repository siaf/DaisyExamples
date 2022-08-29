import struct

#file should be raw pcm 32bit float. Use Audacity to export a headerless copy of your wave table.
fileName="/Users/siavashfathi/Downloads/fmish.raw"
print("opening file")

with open(fileName, mode='rb') as file: # b is important -> binary
    data = file.read()

#i=float.from_bytes(data[0:4], byteorder='little', signed=False)

i=struct.unpack('f'*(len(data)//4), data)

print(i)