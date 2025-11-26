#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define I2C_SDA 8
#define I2C_SCL 9
#define SERVO_FREQ 50

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// 当前测试的舵机和角度
int current_servo = 0;
int current_angle = 90;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║   Single Servo Test & Calibration    ║");
  Serial.println("╚═══════════════════════════════════════╝\n");
  
  // 初始化 I2C
  Serial.print("Initializing I2C... ");
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);
  Serial.println("OK");
  
  // 扫描 I2C
  Serial.print("Scanning I2C bus... ");
  Wire.beginTransmission(0x40);
  if (Wire.endTransmission() == 0) {
    Serial.println("PCA9685 found!");
  } else {
    Serial.println("ERROR: PCA9685 not found!");
    while(1) {
      delay(1000);
      Serial.println("Check wiring and press RESET");
    }
  }
  
  // 初始化 PCA9685
  Serial.print("Initializing PCA9685... ");
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(100);
  Serial.println("OK\n");
  
  printHelp();
  setServo(current_servo, current_angle);
}

void printHelp() {
  Serial.println("========================================");
  Serial.println("COMMANDS:");
  Serial.println("  0-5     : Select servo (0-5)");
  Serial.println("  a       : Angle 0°");
  Serial.println("  s       : Angle 45°");
  Serial.println("  d       : Angle 90°");
  Serial.println("  f       : Angle 135°");
  Serial.println("  g       : Angle 180°");
  Serial.println("  +       : Increase angle by 10°");
  Serial.println("  -       : Decrease angle by 10°");
  Serial.println("  [       : Increase angle by 1°");
  Serial.println("  ]       : Decrease angle by 1°");
  Serial.println("  t       : Test sweep (0-180-0)");
  Serial.println("  h       : Show this help");
  Serial.println("  i       : Show current info");
  Serial.println("========================================\n");
}

uint16_t angleToPulse(int angle) {
  // 将角度映射到脉冲宽度
  // 0° = 150, 180° = 600
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, 150, 600);
}

void setServo(int servo_num, int angle) {
  if (servo_num < 0 || servo_num > 5) {
    Serial.println("ERROR: Servo must be 0-5");
    return;
  }
  
  angle = constrain(angle, 0, 180);
  uint16_t pulse = angleToPulse(angle);
  
  pwm.setPWM(servo_num, 0, pulse);
  
  Serial.println("─────────────────────────────────────");
  Serial.print("Servo #");
  Serial.print(servo_num);
  Serial.print(" → ");
  Serial.print(angle);
  Serial.print("° (pulse: ");
  Serial.print(pulse);
  Serial.println(")");
  Serial.println("─────────────────────────────────────");
  
  current_servo = servo_num;
  current_angle = angle;
}

void showInfo() {
  Serial.println("\n========================================");
  Serial.println("CURRENT STATUS:");
  Serial.print("  Selected Servo: #");
  Serial.println(current_servo);
  Serial.print("  Current Angle:  ");
  Serial.print(current_angle);
  Serial.println("°");
  Serial.print("  Current Pulse:  ");
  Serial.println(angleToPulse(current_angle));
  Serial.println("========================================\n");
}

void testSweep() {
  Serial.println("\n>>> Starting sweep test on Servo #" + String(current_servo) + " <<<");
  Serial.println("Moving: 0° → 180° → 0°\n");
  
  // 0 → 180
  for(int angle = 0; angle <= 180; angle += 10) {
    setServo(current_servo, angle);
    delay(200);
  }
  
  delay(500);
  
  // 180 → 0
  for(int angle = 180; angle >= 0; angle -= 10) {
    setServo(current_servo, angle);
    delay(200);
  }
  
  Serial.println("\n>>> Sweep test complete! <<<\n");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    // 清除缓冲区中的换行符
    while(Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r')) {
      Serial.read();
    }
    
    switch(cmd) {
      // 选择舵机
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
        current_servo = cmd - '0';
        Serial.println("\n>>> Servo #" + String(current_servo) + " selected <<<");
        setServo(current_servo, current_angle);
        break;
      
      // 预设角度
      case 'a':
      case 'A':
        setServo(current_servo, 0);
        break;
      
      case 's':
      case 'S':
        setServo(current_servo, 45);
        break;
      
      case 'd':
      case 'D':
        setServo(current_servo, 90);
        break;
      
      case 'f':
      case 'F':
        setServo(current_servo, 135);
        break;
      
      case 'g':
      case 'G':
        setServo(current_servo, 180);
        break;
      
      // 微调
      case '+':
      case '=':
        current_angle += 10;
        setServo(current_servo, current_angle);
        break;
      
      case '-':
      case '_':
        current_angle -= 10;
        setServo(current_servo, current_angle);
        break;
      
      case ']':
      case '}':
        current_angle += 1;
        setServo(current_servo, current_angle);
        break;
      
      case '[':
      case '{':
        current_angle -= 1;
        setServo(current_servo, current_angle);
        break;
      
      // 测试
      case 't':
      case 'T':
        testSweep();
        break;
      
      // 帮助和信息
      case 'h':
      case 'H':
      case '?':
        printHelp();
        break;
      
      case 'i':
      case 'I':
        showInfo();
        break;
      
      case '\n':
      case '\r':
        // 忽略换行符
        break;
      
      default:
        Serial.print("Unknown command: '");
        Serial.print(cmd);
        Serial.println("' (Press 'h' for help)");
        break;
    }
  }
}