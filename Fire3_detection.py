import jetson.utils
import jetson.inference
import time 
import serial

net=jetson.inference.detectNet(argv=['--model=models/fire3/ssd-mobilenet.onnx', '--labels=models/fire3/labels.txt', '--input-blob=input_0', '--output-cvg=scores', '--output-bbox=boxes'])
camera = jetson.utils.videoSource("csi://0")
display = jetson.utils.videoOutput()	

print('UART with ESP32')

serial_port = serial.Serial(
	port = '/dev/ttyTHS1',
	baudrate=115200,
	parity=serial.PARITY_NONE
)

sweep_cam_angle = 1
counter = 0
not_in_frame_timeout_flag = False
sweep_detect_flag =False

def sweeping_function():
	serial_port.write("s".encode())
	serial_port.flush()

	while serial_port.inWaiting()==0:
		print("waiting for esp response for sweep")
		time.sleep(0.1)

	if serial_port.inWaiting()>0:
		incoming_size = serial_port.inWaiting()
		receiving = serial_port.read(incoming_size)
		print(str(receiving))

	serial_port.reset_input_buffer()
	global sweep_cam_angle
	print('sweeping now on')
	sweep_flag = False

	while True:
			
			img = camera.Capture()
			detections = net.Detect(img)
			display.Render(img)
			display.SetStatus("Object detection | Network {:.0f}) FPS".format(net.GetNetworkFPS()))

			#stop sweeping when fire is detected and get cam angle from esp
			for detection in detections:
				print('detected')
				print('object center X:' + str(int(detection.Center[0])) + 'object center Y:' +str(int(detection.Center[1])))
				if int(detection.Center[0])>660:
					print('fire on the right')
					sweep_flag=True
								

				elif int(detection.Center[0])<620:
					print('fire on the left')
					sweep_flag=True
								
				else:
					print('fire straight ahead')
					sweep_flag=True
								
				
			# get cam angle only when the fire is detected. the cam servo will stop moving when angle is requested				
			if sweep_flag ==True:
				print("sweeping completed with detection, requesting cam angle from esp")
				serial_port.write("g".encode())
				
				while serial_port.inWaiting() ==0:
					time.sleep(0.01)
					print("waiting for esp response to get cam angle")

				if serial_port.inWaiting()>0:
					incoming_size = serial_port.inWaiting()
					receiving = serial_port.read(incoming_size)
					sweep_cam_angle = int(receiving)
					print("bytes received: " +str(incoming_size))
					print("fire detected at angle: "+str(sweep_cam_angle))

				print(" Fire detected at cam angle: "+ str(sweep_cam_angle))
				serial_port.write("w".encode())
				break	

			if  serial_port.inWaiting()>0:
				incoming_size = serial_port.inWaiting()
				receiving = serial_port.read(incoming_size)
				if str(receiving) == "b'clear'":
					print("nothing detected")
					serial_port.write("w".encode())
					serial_port.flush()
					break

			
	return sweep_flag
	#sweeping function done	

def cam_center():
	serial_port.write("c".encode())
	print("requesting cam to center pos")

	while serial_port.inWaiting()==0:
		print("waiting for esp response for cam center start")
		time.sleep(0.01)

	if serial_port.inWaiting()>0:
		incoming_size = serial_port.inWaiting()
		receiving = serial_port.read(incoming_size)

		print(str(receiving)+'\n\n')

	while serial_port.inWaiting()==0:
		print("waiting for esp response to cam center end")
		time.sleep(0.01)
	
	if serial_port.inWaiting()>0:
		incoming_size = serial_port.inWaiting()
		receiving = serial_port.read(incoming_size)
	
		print(str(receiving)+'\n\n')
	serial_port.write("w".encode())
	return


def move():
	# determine if the fire is on the left or on the right
	#only run this when the camear is aligned straight 90 degrees 
	serial_port.write("m".encode())

	while serial_port.inWaiting()==0:
		print("waiting for esp to get ready to move")
		time.sleep(0.01)

	if serial_port.inWaiting()>0:
		incoming_size = serial_port.inWaiting()
		receiving = serial_port.read(incoming_size)
		print(str(receiving))
	
	global sweep_cam_angle
	global not_in_frame_timeout_flag

	#assume that the fire is not the frame in before moving and not in position to extinguish 
	initial_in_frame_flag =False
	extinguish_pos_flag =False
	pause_flag =False
	not_in_frame_flag = True
	not_in_frame_timeout_flag =False

	while initial_in_frame_flag == False:
		#check if there is fire in frame
		print("checking for fire in the frame")
		not_in_frame_timeout =time.time() +5
		print('next time out at : ' +str(not_in_frame_timeout))
		img = camera.Capture()
		detections = net.Detect(img)
		display.Render(img)
		display.SetStatus("Object detection | Network {:.0f}) FPS".format(net.GetNetworkFPS()))

		for detection in detections:
				initial_in_frame_flag =True

		#get the fire in frame based on the angle of the servo sweep recorded
		if sweep_cam_angle >90 and initial_in_frame_flag == False:
			print("turning left until fire is in frame")
			serial_port.write("l".encode())

		if sweep_cam_angle <90 and initial_in_frame_flag ==False:
			print("turning right until fire is in frame")
			serial_port.write("r".encode())	
		
		# keep turning until thee fire in in the frame
		while initial_in_frame_flag == False:
			print("continue checking for initial fire in frame")
			img = camera.Capture()
			detections = net.Detect(img)
			display.Render(img)
			display.SetStatus("Object detection | Network {:.0f}) FPS".format(net.GetNetworkFPS()))
			
			if time.time()>= not_in_frame_timeout:
				print('not in frame time out, going to sweep again')
				serial_port.write('p'.encode())
				serial_port.flush()
				serial_port.write('w'.encode())
				serial_port.flush()
				not_in_frame_timeout_flag =True
				return

			for detection in detections:
				initial_in_frame_flag=True

	serial_port.write("p".encode())
	print('move pause fire is in frame')
	time.sleep(1)
	not_in_frame_timeout = time.time()+5

		#now that the fire is in the frame, move towards it
	while True:
		in_frame_flag=False
		img = camera.Capture()
		detections = net.Detect(img)
		display.Render(img)
		display.SetStatus("Object detection | Network {:.0f}) FPS".format(net.GetNetworkFPS()))

		for detection in detections:
	
		#if there is 2 fires detected in frame, only consider the first one
			if detection.Instance ==1:
				break
			in_frame_flag =True
			pause_flag =False
		# record current time +5 seconds to check for detection timeout
			not_in_frame_timeout = time.time()+5
			print('next no detection timeout at: ' + str(not_in_frame_timeout))
			print("going to fire now")

		#move until the fire is approx 60cm away and aligned properly
			if detection.Center[1]<=425:
				print('detection Y: ' + str(detection.Center[1]))
				normalized_x_pos = int((detection.Center[0]-640)*100/640)
				print('normalized x:' +str(normalized_x_pos))
				x_move = "k"+ str(normalized_x_pos)
				serial_port.write(x_move.encode())
				serial_port.flush()

		# stop moving when there is fire in the frame and when in range to extinguish and align the center
			if detection.Center[1]>=425:
				
				print('detection Y: ' + str(detection.Center[1]))
				normalized_x_pos = int((detection.Center[0]-640)*100/640)
				print('normalized x:' +str(normalized_x_pos))
				align = "a" + str(normalized_x_pos)
				serial_port.write(align.encode())
				serial_port.flush()
				print('aligning now')
				if 1>=normalized_x_pos>=-1:
					extinguish_pos_flag =True
					serial_port.write('w'.encode())
					serial_port.flush()
					break

		if extinguish_pos_flag ==True:
			print('Ready to extinguish')
			break
			return

		#there might be some frames where the fire is not detected so getting the next frame will help, pause moving while waiting for the next detection
		if in_frame_flag ==False and pause_flag ==False :

			print("stopping until the next detection")
			
			serial_port.write('p'.encode())
			serial_port.flush()
			
			while serial_port.inWaiting() ==0:
				print('waiting for esp to pause')
				time.sleep(0.01)

			if serial_port.inWaiting()>0:
				incoming_size = serial_port.inWaiting()
				receiving = serial_port.read(incoming_size)
				print(str(receiving))
				pause_flag =True

		if time.time() >= not_in_frame_timeout:
			print('not in frame timeout')
			not_in_frame_timeout_flag = True
			serial_port.write('w'.encode())
			serial_port.flush()
			return
				
	return

def extinguish():
	print("asking esp 32 to start trigerring")
	serial_port.write('t'.encode())
	serial_port.flush()
	
	while serial_port.inWaiting() ==0:
		print("waiting for esp to start triggering")
		time.sleep(0.01)
	
	if serial_port.inWaiting()>0:
		incoming_size = serial_port.inWaiting()
		receiving = serial_port.read(incoming_size)
		print(str(receiving))

	while serial_port.inWaiting()==0:
		print('waiting for esp to finish trigerring')
		time.sleep(0.01)

	if serial_port.inWaiting()>0:
		incoming_size = serial_port.inWaiting()
		receiving = serial_port.read(incoming_size)
		print(str(receiving))

counter =0
while True:
	img = camera.Capture()
	detections = net.Detect(img)
	display.Render(img)
	display.SetStatus("Object detection | Network {:.0f}) FPS".format(net.GetNetworkFPS()))
	#begin by checking the surroundings for fire
	#call the sweep camera sweep function
	while sweep_detect_flag ==False:
		sweep_detect_flag = sweeping_function()

	sweep_detect_flag =False
		
	cam_center()
	while True:
		move()
		if not_in_frame_timeout_flag == True:
			sweeping_function()
			cam_center()
		if not_in_frame_timeout_flag == False:
			break

	extinguish()
	time.sleep(1)

	#if no fire is detected during the sweep, sweep 1 more time
	
	
	counter +=1
	print('done ' + str(counter)+' fires')


