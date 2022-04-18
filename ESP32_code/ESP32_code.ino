#include <ESP32Servo.h>
#include <analogWrite.h>
#include <ESP32Tone.h>
#include <ESP32PWM.h>

//s for sweep function
//w for wait function
//g for get sweep angle function
//c for cam_center function
//l for turn left to get fire in frame function
//r for turn right to get fire in frame function
//m to start motor movements to home into fire
//p to pause move function
//t to trigger extinguisher


#define RXD2 16
#define TXD2 17

#define center_cam_pos 94
#define cam_servo_pwm 18
#define trigger_servo_pwm 19
#define Right_PWM 33
#define Left_PWM 32
#define Left_DC_motor_Pin1 14
#define Left_DC_motor_Pin2 27
#define Right_DC_motor_Pin1 25
#define Right_DC_motor_Pin2 26

#define cam_servo_sweep_millis_per_degree 60

#define motor_homing_min_pwm_map_range 170
#define motor_homing_max_pwm_map_range 210
#define motor_quick_turn_pwm_percentage 79
#define motor_forward_pwm_percentage 70
#define motor_power_right_boost_percentage 2

#define motor_center_brake_pwm_value 130

#define motor_align_pwm 175
#define trigger_angle 115
#define standby_angle 150

Servo cam_servo;
Servo trigger_servo;
char rec ='w';
int normalized_x_pos;

#define motor_brake_time 20
void setup() {
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  Serial.begin(115200);
  Serial.println("Serial to PC on");
  Serial.flush();
  cam_servo.attach(cam_servo_pwm);
  cam_servo.write(center_cam_pos);
  trigger_servo.attach(trigger_servo_pwm);
  trigger_servo.write(standby_angle);

  pinMode(Right_PWM, OUTPUT);
  pinMode(Left_PWM, OUTPUT);
  pinMode(Left_DC_motor_Pin1, OUTPUT);
  pinMode(Left_DC_motor_Pin2, OUTPUT);
  pinMode(Right_DC_motor_Pin1, OUTPUT);
  pinMode(Right_DC_motor_Pin2, OUTPUT);
  pinMode(cam_servo_pwm, OUTPUT);
  pinMode(trigger_servo_pwm, OUTPUT);

}

void loop() {
  while(!Serial2.available()){
  }
  if(Serial2.available()){
    rec=Serial2.read();
    Serial.println(rec);
  }
  switch (rec){
    case 's':
      cam_sweep();
      break;

    case 'c':
      cam_center();
      break;

    case 'w':
      delay(100);
      break;
      
    case 'm':
      move_ctrl();
      break;

    case 't':
      trigger();
      break;
      }
  
}


//use cam sweep to find nearby fires will stop once a fire is detected

void cam_sweep(){ 
  int cam_current_pos = cam_servo.read();
  Serial2.print("sweeping now"); 
  Serial.println("sweeping now");
  Serial2.flush();
  
//sweep left
  while(cam_current_pos<180){
    cam_current_pos++;
    cam_servo.write(cam_current_pos);
 
    delay(cam_servo_sweep_millis_per_degree);

    if(cam_current_pos % 2 ==0){
       digitalWrite(Right_DC_motor_Pin1, HIGH);
       digitalWrite(Right_DC_motor_Pin2, LOW);
       digitalWrite(Left_DC_motor_Pin1, LOW);
       digitalWrite(Left_DC_motor_Pin2, HIGH);
      analogWrite(Left_PWM, 210);
      analogWrite(Right_PWM, 210);

      delay(50);
       digitalWrite(Right_DC_motor_Pin1, LOW);
      digitalWrite(Right_DC_motor_Pin2, LOW);
      digitalWrite(Left_DC_motor_Pin1, LOW);
      digitalWrite(Left_DC_motor_Pin2, LOW);
      analogWrite(Left_PWM, 0);
      analogWrite(Right_PWM, 0);
    }
    Serial.println(cam_servo.read());
    
    if(Serial2.available()){
      rec = Serial2.read();
      if(rec =='g'){
        Serial.println("giving cam angle");
        Serial2.print(cam_servo.read());
      }
        Serial.println(rec);
        return;
        
    }
  }
  
//sweep right
  while(cam_current_pos>0){
    cam_current_pos--;
    cam_servo.write(cam_current_pos);

    delay(cam_servo_sweep_millis_per_degree);
    
   if(cam_current_pos % 2 ==0){
       digitalWrite(Right_DC_motor_Pin1, LOW);
       digitalWrite(Right_DC_motor_Pin2, HIGH);
       digitalWrite(Left_DC_motor_Pin1, HIGH);
       digitalWrite(Left_DC_motor_Pin2, LOW);
      analogWrite(Left_PWM, 200);
      analogWrite(Right_PWM, 200);

      delay(30);
       digitalWrite(Right_DC_motor_Pin1, LOW);
      digitalWrite(Right_DC_motor_Pin2, LOW);
      digitalWrite(Left_DC_motor_Pin1, LOW);
      digitalWrite(Left_DC_motor_Pin2, LOW);
      analogWrite(Left_PWM, 0);
      analogWrite(Right_PWM, 0);
    }
    
    Serial.println(cam_servo.read());
    if(Serial2.available()){
      rec = Serial2.read();
      if(rec =='g'){
        Serial.println("giving cam angle");
        Serial2.print(cam_servo.read());
      }
        Serial.println(rec);
        return;
    }
   }
   Serial.println("Nothing detected on sweep");
   Serial2.print("clear");
   return;
}

void move_ctrl(){
  int left_homing;
  int right_homing;
  bool from_left_turn_flag = false;
  bool from_right_turn_flag = false;
  Serial.println("ready to move");
  Serial2.print("ready to move");
  while(true){
    while(!Serial2.available()){
    }
    if(Serial2 .available()){
      rec = Serial2.read();
      Serial.println(rec);
    }
    switch (rec){
      case 'r':
      
        Serial.println("turning right");
        digitalWrite(Right_DC_motor_Pin1, LOW);
        digitalWrite(Right_DC_motor_Pin2, HIGH);
        digitalWrite(Left_DC_motor_Pin1, HIGH);
        digitalWrite(Left_DC_motor_Pin2, LOW);
        analogWrite(Left_PWM, map(motor_quick_turn_pwm_percentage, 0, 100, 0, 255));
        analogWrite(Right_PWM, map(motor_quick_turn_pwm_percentage,0,100,0,255));
        break;

      case 'l':
        Serial.println("turning left");
        digitalWrite(Right_DC_motor_Pin1, HIGH);
        digitalWrite(Right_DC_motor_Pin2, LOW);
        digitalWrite(Left_DC_motor_Pin1, LOW);
        digitalWrite(Left_DC_motor_Pin2, HIGH);
        analogWrite(Left_PWM, map(motor_quick_turn_pwm_percentage, 0, 100, 0, 255));
        analogWrite(Right_PWM, map(motor_quick_turn_pwm_percentage,0,100,0,255));
        break;

      case 'k':
        Serial.println("homing in"); 
          normalized_x_pos = Serial2.parseInt();
          Serial.print("normal: ");
          Serial.println(normalized_x_pos);

        // use function to turn right
        if(normalized_x_pos>10){
          Serial.println("normal is more than 3");
          from_right_turn_flag =true;

          left_homing = normalized_x_pos;

          if(left_homing>=motor_quick_turn_pwm_percentage){
            left_homing = motor_quick_turn_pwm_percentage;
          }

          right_homing = left_homing/1.1;

          left_homing = map(left_homing,0,100,motor_homing_min_pwm_map_range,motor_homing_max_pwm_map_range );
          right_homing = map(right_homing,0,100,motor_homing_min_pwm_map_range,motor_homing_max_pwm_map_range );

          digitalWrite(Right_DC_motor_Pin1, LOW);
          digitalWrite(Right_DC_motor_Pin2, HIGH);
          digitalWrite(Left_DC_motor_Pin1, HIGH);
          digitalWrite(Left_DC_motor_Pin2, LOW);
          analogWrite(Left_PWM, left_homing);
          analogWrite(Right_PWM, right_homing);
          delay(50);

          Serial.print("left homing: ");
          Serial.println(left_homing);
          Serial.print("right homing: ");
          Serial.println(right_homing);
        }

        //use function to turn left
        else if(normalized_x_pos<-10){
          Serial.println("less than -10");
          from_left_turn_flag =true;

          normalized_x_pos = -normalized_x_pos + motor_power_right_boost_percentage;
          right_homing = normalized_x_pos;

          if(right_homing>=motor_quick_turn_pwm_percentage){
            right_homing = motor_quick_turn_pwm_percentage;
          }
          
          left_homing = right_homing/1.1;

          left_homing = map(left_homing,0,100,motor_homing_min_pwm_map_range,motor_homing_max_pwm_map_range );
          right_homing = map(right_homing,0,100,motor_homing_min_pwm_map_range,motor_homing_max_pwm_map_range );
          
          digitalWrite(Right_DC_motor_Pin1, HIGH);
          digitalWrite(Right_DC_motor_Pin2, LOW);
          digitalWrite(Left_DC_motor_Pin1, LOW);
          digitalWrite(Left_DC_motor_Pin2, HIGH);
          analogWrite(Left_PWM, left_homing);
          analogWrite(Right_PWM, right_homing);
          delay(50);

          Serial.print("left homing: ");
          Serial.println(left_homing);
          Serial.print("right homing: ");
          Serial.println(right_homing);
        }
        else{
          Serial.println(" between -10 and 10");

          if (from_left_turn_flag == true){
            digitalWrite(Right_DC_motor_Pin1, LOW);
            digitalWrite(Right_DC_motor_Pin2, HIGH);
            digitalWrite(Left_DC_motor_Pin1, HIGH);
            digitalWrite(Left_DC_motor_Pin2, LOW);
            analogWrite(Left_PWM, motor_center_brake_pwm_value);
            analogWrite(Right_PWM, motor_center_brake_pwm_value);
            delay(motor_brake_time);
            from_left_turn_flag = false;

            digitalWrite(Right_DC_motor_Pin1, LOW);
            digitalWrite(Right_DC_motor_Pin2, LOW);
            digitalWrite(Left_DC_motor_Pin1, LOW);
            digitalWrite(Left_DC_motor_Pin2, LOW);
            analogWrite(Left_PWM, 0);
            analogWrite(Right_PWM, 0);
          }

          if (from_right_turn_flag == true){
            digitalWrite(Right_DC_motor_Pin1, HIGH);
            digitalWrite(Right_DC_motor_Pin2, LOW);
            digitalWrite(Left_DC_motor_Pin1, LOW);
            digitalWrite(Left_DC_motor_Pin2, HIGH);
            analogWrite(Left_PWM, motor_center_brake_pwm_value);
            analogWrite(Right_PWM, motor_center_brake_pwm_value);
            delay(motor_brake_time);
            from_right_turn_flag = false;

            digitalWrite(Right_DC_motor_Pin1, LOW);
            digitalWrite(Right_DC_motor_Pin2, LOW);
            digitalWrite(Left_DC_motor_Pin1, LOW);
            digitalWrite(Left_DC_motor_Pin2, LOW);
            analogWrite(Left_PWM, 0);
            analogWrite(Right_PWM, 0);
          }
          
          digitalWrite(Right_DC_motor_Pin1, HIGH);
          digitalWrite(Right_DC_motor_Pin2, LOW);
          digitalWrite(Left_DC_motor_Pin1, HIGH);
          digitalWrite(Left_DC_motor_Pin2, LOW);
          analogWrite(Left_PWM, map(motor_forward_pwm_percentage, 0, 100, 0, 255));
          analogWrite(Right_PWM, map(motor_forward_pwm_percentage,0,100,0,255));
          delay(50);
          }
        break;

        case 'a':
        Serial.println("align now"); 
          normalized_x_pos = Serial2.parseInt();
          Serial.print("normal: ");
          Serial.println(normalized_x_pos);

        // use function to turn right
        if(normalized_x_pos>1){
          Serial.println("normal is more than 1");

          digitalWrite(Right_DC_motor_Pin1, LOW);
          digitalWrite(Right_DC_motor_Pin2, HIGH);
          digitalWrite(Left_DC_motor_Pin1, HIGH);
          digitalWrite(Left_DC_motor_Pin2, LOW);
          analogWrite(Left_PWM, motor_align_pwm);
          analogWrite(Right_PWM, motor_align_pwm);
          delay(50);

          digitalWrite(Right_DC_motor_Pin1, LOW);
          digitalWrite(Right_DC_motor_Pin2, LOW);
          digitalWrite(Left_DC_motor_Pin1, LOW);
          digitalWrite(Left_DC_motor_Pin2, LOW);
          analogWrite(Left_PWM, 0);
          analogWrite(Right_PWM, 0);
        }

        //use function to turn left
        else if(normalized_x_pos<-1){
          Serial.println("less than -1");
         digitalWrite(Right_DC_motor_Pin1, HIGH);
          digitalWrite(Right_DC_motor_Pin2, LOW);
          digitalWrite(Left_DC_motor_Pin1, LOW);
          digitalWrite(Left_DC_motor_Pin2, HIGH);
          analogWrite(Left_PWM, motor_align_pwm);
          analogWrite(Right_PWM, motor_align_pwm);
          delay(50);

          digitalWrite(Right_DC_motor_Pin1, LOW);
          digitalWrite(Right_DC_motor_Pin2, LOW);
          digitalWrite(Left_DC_motor_Pin1, LOW);
          digitalWrite(Left_DC_motor_Pin2, LOW);
          analogWrite(Left_PWM, 0);
          analogWrite(Right_PWM, 0);
        }
        break;

        case 'p':
          Serial.println("pausing move function");
          Serial2.println("pausing move function");
          digitalWrite(Right_DC_motor_Pin1, LOW);
          digitalWrite(Right_DC_motor_Pin2, LOW);
          digitalWrite(Left_DC_motor_Pin1, LOW);
          digitalWrite(Left_DC_motor_Pin2, LOW);
         break;
        }
        if (rec =='w'){
          Serial.println("exiting move func");
          digitalWrite(Right_DC_motor_Pin1, LOW);
          digitalWrite(Right_DC_motor_Pin2, LOW);
          digitalWrite(Left_DC_motor_Pin1, LOW);
          digitalWrite(Left_DC_motor_Pin2, LOW);
          break;
      }
    }
  
}
void cam_center(){
  Serial2.print("cam going to center pos now");
  cam_servo.write(center_cam_pos);
  delay(500);
  Serial2.print("cam at center pos");
  Serial.println("cam_center function done");
}

void trigger(){
  Serial.println("trigger on");
  Serial2.print("trigger on");
  trigger_servo.write(trigger_angle);

  for(int i=0; i<=4; i++){
    digitalWrite(Right_DC_motor_Pin1, HIGH);
    digitalWrite(Right_DC_motor_Pin2, LOW);
    digitalWrite(Left_DC_motor_Pin1, LOW);
    digitalWrite(Left_DC_motor_Pin2, HIGH);
    analogWrite(Left_PWM, map(motor_quick_turn_pwm_percentage, 0, 100, 0, 255));
    analogWrite(Right_PWM, map(motor_quick_turn_pwm_percentage,0,100,0,255));

    delay(150);

    digitalWrite(Right_DC_motor_Pin1, LOW);
    digitalWrite(Right_DC_motor_Pin2, HIGH);
    digitalWrite(Left_DC_motor_Pin1, HIGH);
    digitalWrite(Left_DC_motor_Pin2, LOW);
    analogWrite(Left_PWM, map(motor_quick_turn_pwm_percentage, 0, 100, 0, 255));
    analogWrite(Right_PWM, map(motor_quick_turn_pwm_percentage,0,100,0,255));

    delay(300);

    digitalWrite(Right_DC_motor_Pin1, HIGH);
    digitalWrite(Right_DC_motor_Pin2, LOW);
    digitalWrite(Left_DC_motor_Pin1, LOW);
    digitalWrite(Left_DC_motor_Pin2, HIGH);
    analogWrite(Left_PWM, map(motor_quick_turn_pwm_percentage, 0, 100, 0, 255));
    analogWrite(Right_PWM, map(motor_quick_turn_pwm_percentage,0,100,0,255));

    delay(150);
  }
  trigger_servo.write(standby_angle);
  Serial.println("done triggering");
  Serial2.println("done triggering");
  digitalWrite(Right_DC_motor_Pin1, LOW);
  digitalWrite(Right_DC_motor_Pin2, LOW);
  digitalWrite(Left_DC_motor_Pin1, LOW);
  digitalWrite(Left_DC_motor_Pin2, LOW);
  analogWrite(Left_PWM,0);
  analogWrite(Right_PWM, 0);
  Serial.flush();
  Serial2.flush();
}
