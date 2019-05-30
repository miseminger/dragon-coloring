#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
Created on Sun Jul  8 22:03:48 2018

@author: madeline
"""

#!/usr/bin/python2.7
import serial
import numpy as np 
from scipy import stats
import matplotlib.pyplot as plt
import matplotlib.image as mpimg

#define the serial port:
port = "/dev/ttyACM0"  #for Linux

filename = '/home/madeline/Desktop/PHYS319/grayscaledragon.png'
startimg = mpimg.imread(filename)

filename2 = '/home/madeline/Desktop/PHYS319/plaindragon.png'
startimg2 = mpimg.imread(filename2)

wings = ((startimg[:, :, 0] > 133/255.) & (startimg[:, :, 0] < 153/255.))
body = ((startimg[:, :, 0] > 197/255.) & (startimg[:, :, 0] < 217/255.))
tummy = ((startimg[:, :, 0] > 229/255.) & (startimg[:, :, 0] < 249/255.))

#open the serial port
try:
    ser = serial.Serial(port,2400,timeout = 10.0) 
    ser.baudrate=9600

except:
    print ("Opening serial port",port,"failed")
    print ("Edit program to point to the correct port.")
    print ("Hit enter to exit")
    raw_input()
    quit()
    
def mode(arr):
    return int(stats.mode(np.asarray(arr))[0])

def average(arr):
    elements = np.array(arr)
    mean = np.mean(elements, axis=0)
    sd = np.std(elements, axis=0)
    okdata = [x for x in arr if (x >= mean - 2*sd) and (x <= mean + 2*sd) ]
    average = int(np.mean(np.array(okdata)))
    return average


ser.flushInput()


def colorread(area, img):  #arguments are the color you want to add next and the np array of the image you want to start with
    
	shade_1 = []
    
	while(True):  #loop forever
		while(len(shade_1) < 10000): #loop forever (to fill list, anyway)
			data = ser.read() # look for a character from serial port - will wait for up to 50ms (specified above in timeout)
			if (len(data) > 0): #was there a byte to read?
				shade_1.append(ord(data))
			else:
				break    
		if len(shade_1) == 0:
			print "Oops! Please try pushing the button again."
		else:
			break
    
    #now cut off all the bytes received before and including the start sequence
	UARTstring = " ".join(str(x) for x in shade_1)
	begincutoff = UARTstring.find('255 0 255 0 255 0 255 0')
	shade_1 = shade_1[begincutoff + 10:]
	start = shade_1.index(128) + 2 #this is the index of the first occurence of RDATAL after the placeholder byte
    
    #combine low and high bytes and collect all data in one array for each color
	clearsl = np.array(shade_1[start::18])  # CDATAL + CDATAH*256 
	clearsh = np.array(shade_1[start + 2::18]) * 256
	redsl = np.array(shade_1[start + 4::18])  # RDATAL + RDATAH*256 
	redsh = np.array(shade_1[start + 6::18]) * 256
	greensl = np.array(shade_1[start + 8::18])  # GDATAL + GDATAH*256 
	greensh = np.array(shade_1[start + 10::18]) * 256
	bluesl = np.array(shade_1[start + 12::18])  # BDATAL + BDATAH*256 
	bluesh = np.array(shade_1[start + 14::18]) * 256
    
    #now, since each value was sent ten times over UART to account for UART instability, take the mode of each group of ten and put that into a new list
	cdatal = []
    	cdatah = []
    	rdatal = []
    	rdatah = []
	gdatal = []
    	gdatah = []
    	bdatal = []
    	bdatah = []
                	
	for x in np.arange(0,len(bluesh),10):   #bluesh is chosen because it's the last, so if anything gets cut off it'll be here, most likely
		cdatal.append(mode(clearsl[x:x+10]))
		cdatah.append(mode(clearsh[x:x+10]))
		rdatal.append(mode(redsl[x:x+10]))
		rdatah.append(mode(redsh[x:x+10]))
		gdatal.append(mode(greensl[x:x+10])) 
		gdatah.append(mode(greensh[x:x+10])) 
		bdatal.append(mode(bluesl[x:x+10])) 
		bdatah.append(mode(bluesh[x:x+10])) 
     
    #now make everything a numpy array and join the high and low bytes
    	cdata = np.array(cdatal) + np.array(cdatah)
    	rdata = np.array(rdatal) + np.array(rdatah)
    	gdata = np.array(gdatal) + np.array(gdatah)    
    	bdata = np.array(bdatal) + np.array(bdatah)
        
	redval = average(rdata)
	greenval = average(gdata)
	blueval = average(bdata)
    
	#colors = [redval, greenval, blueval]
	top = max([redval, greenval, blueval])
	r = float(redval)/top
	g = float(greenval)/top
	b = float(blueval)/top
    
	img[area] = [r, g, b]
    
	return img

print
print "Welcome to the dragon coloring program!"
print
print "Here's your dragon.  His name is George, and he is eager to be colored in."
plt.figure(figsize = (80,6))
plt.imshow(startimg2)  
plt.axis('off')
plt.show()

print
print "Put a color for his wings and tail over the sensor, then push button S2."
wingimg = colorread(wings, startimg2)
plt.figure(figsize = (80,6))
plt.imshow(wingimg)
plt.axis('off')
plt.show()

ser.flushInput()

print
print "Now put a color for the main part of his body over the sensor, and push button S2."
bodyimg = colorread(body, wingimg)
plt.figure(figsize = (80,6))
plt.imshow(bodyimg)
plt.axis('off')
plt.show()

ser.flushInput()

print
print "Lastly, put a color for his front over the sensor, and push button S2."
tummyimg = colorread(tummy, bodyimg)
plt.figure(figsize = (80,6))
plt.imshow(tummyimg)
plt.axis('off')
plt.show()

print
print "TA-DA!"
