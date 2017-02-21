def read():
	#Basic Setups
	import network
	import time
	import machine

	#Establish connnection to EEERover Wifi
 	ap_if = network.WLAN(network.AP_IF)
	ap_if.active(False)
	sta_if = network.WLAN(network.STA_IF)
	sta_if.connect('EEERover','exhibition')
	while (sta_if.isconnected() == False):#We use this while loop so we can keep track of the wifi connecting status on the screen
		time.sleep(0.1)
		print('Wifi Connecting...')
	print('Wifi Connected!!!')
	
	#MQTT Setup
	from umqtt.simple import MQTTClient
	import ujson as json

	#Try to subscribe the real time from MQTT broker
	def sub_cb(topic,msg):
		global timeo
		print((topic,msg))
		timep = json.loads(msg)
		timeo = timep['date']
		print(timeo)

	client = MQTTClient('TFBOYS','192.168.0.10')
	client.set_callback(sub_cb)
	client.connect()
	client.subscribe('esys/time')
	client.wait_msg()

	rtc = machine.RTC()#Translate the broker's time to the readable format with "year, month, weekdays, hour, minute, second, milisecond"
	rtc.datetime((int(timeo[0:4]),int(timeo[5:7]),int(timeo[8:10]),0,int(timeo[11:13]),int(timeo[14:16]),int(timeo[17:19]),int(timeo[20:22])))
	
	#PWM Setup (We use PWM to control the buzzer)
	p12 = machine.Pin(12)
	pwm12 = machine.PWM(p12)

	#I2C Setup
	from machine import Pin, I2C
	i2c = I2C(scl=Pin(5), sda=Pin(4), freq=100000)

       	#Main
	i = 1 #To enable a closed while loop (Infinite if not keyboard interrupted)
	m = -1 #Index for the fixed-size array to store the time values
	timearray = [None]*100#This fixed-size array is used to store all the time when the 'Warning' in proximity sensor is triggered
	try:
       
		while (i==1): #Loop won't stop except 'keyboardinterrupt'
			time.sleep(0.2) #Take measurement every 0.2s (Continously)
			
			#Start a single on-demand measurement of proximity
			i2c.writeto_mem(19,0x80,bytes([0x08]))
			data = i2c.readfrom_mem(19,0x87,2)
		        proximity = data[0]*256 + data[1] #Translate the measurement of proximity in hex to decimal
		   	print(proximity)
			distance = -0.002077*proximity+8.089#Convert the proximity data in decimal to the real distance in cm(This is NOT on the datasheet, the group measured the relationship ourself.Since on the data sheet, the reflector is KODAK GREY CARD, we need Humanhand in our product)
			if (distance < 0): #When the sensor is hardly pressed, the distance translated  will go below zero, so we will just refer negative values of proximity as zero distance
				distance = 0 
			

                        if (proximity >=2500): #When proximity too big, so the distance is small, so that the warning status should be triggered, kids may already touch the sensor.
				m = m + 1
				print('Less than 3cm')
				print('Warning!!!')
				print(distance)
				timearray[m] = rtc.datetime()#Only store the time when 'warning' is triggered. 
				print(rtc.datetime())
				payload = json.dumps({'distance':['%.2f'%distance,'cm'],'status':'Warning!!!'})#Only publish on MQTT broker when 'warning' is trigger(IN the real product, only transmit the warning on parent's phone when 'warning')
				client.publish('TFBOYS',bytes(payload,'utf-8'))
                                pwm12.freq(1000)#Buzzing a high frequency sound as alarm
                                pwm12.duty(512)

			if (proximity <=2500 and proximity >=2300):#Proximity in a intermediate range, so the distance is also intermediate, the "careful" signal will not be sent to the parent's phone. 
				print('Between 3cm and 5cm') 
				print('Careful!')
				pwm12.freq(500)#Buzzing a lower frequency sound to alert kids
				pwm12.duty(512)

			if (proximity < 2300):#Proximity is small, so distance is far away
				print('More than 5cm')
				print('Safe')
				pwm12.freq(0)
				pwm12.duty(0)
                
	except KeyboardInterrupt: #Keyboardinterrupt to break out the loop (In the real productm, this is like the action when parents come back home or to turn the whole product off)
		print('Interrupted')
		pwm12.freq(0) 
		pass
	
	print ('Warning recorded for day are:')	#When parents come back home, they can see all the times recorded when kids are getting to close to the sensors
	n = 0
	while (n <= m):
		print (timearray[n])
		n = n + 1
	return 
